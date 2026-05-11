#include "postgres.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "access/amapi.h"
#include "access/genam.h"
#include "access/htup_details.h"
#include "access/reloptions.h"
#include "access/table.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xloginsert.h"
#include "catalog/index.h"
#include "catalog/pg_class.h"
#include "catalog/pg_authid_d.h"
#include "catalog/pg_database_d.h"
#include "catalog/pg_type_d.h"
#include "catalog/storage.h"
#include "commands/defrem.h"
#include "executor/spi.h"
#include "executor/tuptable.h"
#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/parsenodes.h"
#include "nodes/tidbitmap.h"
#include "portability/instr_time.h"
#include "postmaster/bgworker.h"
#include "storage/bufmgr.h"
#include "storage/bufpage.h"
#include "storage/buffile.h"
#include "storage/dsm.h"
#include "storage/ipc.h"
#include "storage/itemptr.h"
#include "storage/latch.h"
#include "storage/lock.h"
#include "storage/lmgr.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "storage/spin.h"
#include "storage/proc.h"
#include "utils/acl.h"
#include "utils/array.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "utils/selfuncs.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/typcache.h"

#include "psql_bm25s_core.h"
#include "psql_bm25s_pg_common.h"
#include "psql_bm25s_query.h"
#include "psql_bm25s_storage.h"
#include "psql_bm25s_text.h"

#define PSQL_BM25S_AM_MAGIC UINT32_C(0x53323542)
#define PSQL_BM25S_AM_VERSION 1

#define PSQL_BM25S_AM_PAGE_META 1
#define PSQL_BM25S_AM_PAGE_DELTA 3
#define PSQL_BM25S_AM_PAGE_GENERATION_DATA 4

#define PSQL_BM25S_AM_FLAG_STALE UINT16_C(0x0001)
#define PSQL_BM25S_AM_FLAG_CORRUPT UINT16_C(0x0002)
#define PSQL_BM25S_AM_FLAG_REBUILD_REQUIRED UINT16_C(0x0004)
#define PSQL_BM25S_AM_STORAGE_APPEND_ONLY 1
#define PSQL_BM25S_AM_DEFAULT_EVENTUAL_REBUILD_THRESHOLD 50000
#define PSQL_BM25S_AM_DEFAULT_WORKSPACE_CACHE_BYTES (32 * 1024 * 1024)
#define PSQL_BM25S_AM_DEFAULT_WORKSPACE_IDLE_TIMEOUT_MS 60000
#define PSQL_BM25S_AM_MAX_WORKSPACE_CACHE_BYTES INT_MAX
#define PSQL_BM25S_AM_MAX_WORKSPACE_IDLE_TIMEOUT_MS INT_MAX
#define PSQL_BM25S_AM_GENERATION_MAGIC UINT32_C(0x47323542)
#define PSQL_BM25S_AM_GENERATION_VERSION 1
#define PSQL_BM25S_STORAGE_MAGIC UINT32_C(0x424D3235)
#define PSQL_BM25S_STORAGE_CURRENT_VERSION 2U
#define PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE UINT16_C(0x0001)
#define PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB UINT16_C(0x0002)
#define PSQL_BM25S_STORAGE_FLAG_HAS_EMPTY_TOKEN UINT16_C(0x0004)
#define PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS UINT16_C(0x0008)
#define PSQL_BM25S_STORAGE_FLAG_KNOWN_MASK                                  \
    (PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE |                            \
     PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB |                                    \
     PSQL_BM25S_STORAGE_FLAG_HAS_EMPTY_TOKEN |                              \
     PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS)
#define PSQL_BM25S_AM_DESCRIPTOR_MAGIC UINT32_C(0x44323542)
#define PSQL_BM25S_AM_DESCRIPTOR_VERSION 1
#define PSQL_BM25S_AM_SHARED_GENERATION_MIN_SIZE (256 * 1024 * 1024)
#define PSQL_BM25S_AM_GENERATION_ADVISORY_LOCK 1
#define PSQL_BM25S_AM_GENERATION_LOCK_TAG UINT32_C(0x32534247)
#define PSQL_BM25S_AM_MAINTENANCE_LOCK_TAG UINT32_C(0x3253424D)
#define PSQL_BM25S_AM_MAINTENANCE_WORKER_LOCK_TAG UINT32_C(0x32534257)
#define PSQL_BM25S_AM_SHARED_PRELOAD_MAGIC UINT32_C(0x50323542)
#define PSQL_BM25S_AM_SHARED_PRELOAD_VERSION 5
#define PSQL_BM25S_AM_SHARED_PRELOAD_MIN_ENTRIES 1024
#define PSQL_BM25S_AM_SHARED_PRELOAD_MAX_ENTRIES 65536
#define PSQL_BM25S_AM_SHARED_PRELOAD_ENTRY_TARGET_BYTES (4 * 1024 * 1024)
#define PSQL_BM25S_AM_SHARED_PRELOAD_MAX_MB 1048576
#define PSQL_BM25S_AM_DEFAULT_MAINTENANCE_TIMER_INTERVAL_MS 60000
#define PSQL_BM25S_AM_DEFAULT_PRELOAD_TIMER_INTERVAL_MS 1000
#define PSQL_BM25S_AM_MIN_MAINTENANCE_TIMER_INTERVAL_MS 1000
#define PSQL_BM25S_AM_MIN_PRELOAD_TIMER_INTERVAL_MS 1000
#define PSQL_BM25S_AM_DEFAULT_MAINTENANCE_WORKER_LIMIT 1
#define PSQL_BM25S_AM_DEFAULT_REBUILD_MEMORY_BUDGET_MB 32768
#define PSQL_BM25S_AM_REBUILD_MEMORY_ESTIMATE_MULTIPLIER 6
#define PSQL_BM25S_AM_COMPACT_REBUILD_MEMORY_ESTIMATE_MULTIPLIER 4
#define PSQL_BM25S_AM_SPILL_REBUILD_MEMORY_ESTIMATE_MULTIPLIER 2
#define PSQL_BM25S_AM_STANDARD_REBUILD_BUDGET_HEADROOM_NUM 3
#define PSQL_BM25S_AM_STANDARD_REBUILD_BUDGET_HEADROOM_DEN 5
#define PSQL_BM25S_AM_COMPACT_REBUILD_BUDGET_HEADROOM_NUM 3
#define PSQL_BM25S_AM_COMPACT_REBUILD_BUDGET_HEADROOM_DEN 4
#define PSQL_BM25S_AM_STANDARD_REBUILD_MAX_PAYLOAD_BYTES \
    (UINT64_C(1024) * 1024 * 1024)
#define PSQL_BM25S_AM_COMPACT_REBUILD_MAX_PAYLOAD_BYTES \
    (UINT64_C(512) * 1024 * 1024)
#define PSQL_BM25S_AM_BACKGROUND_LAUNCH_STAMP_LIMIT 1024
#define PSQL_BM25S_AM_MALLOC_TRIM_THRESHOLD (64 * 1024 * 1024)

static void
psql_bm25s_am_release_unused_malloc(void)
{
#ifdef __GLIBC__
    (void) malloc_trim(0);
#endif
}

typedef struct psql_bm25s_tid_builder
{
    ItemPointerData *tids;
    size_t len;
    size_t capacity;
} psql_bm25s_tid_builder;

typedef struct psql_bm25s_docid_builder
{
    uint32_t *doc_ids;
    size_t len;
    size_t capacity;
} psql_bm25s_docid_builder;

typedef struct psql_bm25s_am_tid_doc_map_entry
{
    ItemPointerData tid;
    uint32_t doc_id;
} psql_bm25s_am_tid_doc_map_entry;

typedef struct psql_bm25s_am_doc_id_map_entry
{
    uint32_t old_doc_id;
    uint32_t new_doc_id;
} psql_bm25s_am_doc_id_map_entry;

typedef struct psql_bm25s_am_term_entry_builder
{
    psql_bm25s_term_entry *entries;
    size_t len;
    size_t capacity;
} psql_bm25s_am_term_entry_builder;

typedef struct psql_bm25s_am_doc_length_builder
{
    uint32_t *lengths;
    size_t len;
    size_t capacity;
} psql_bm25s_am_doc_length_builder;

typedef struct psql_bm25s_am_vocab_map_slot
{
    char *key;
    uint32_t value;
    bool used;
} psql_bm25s_am_vocab_map_slot;

typedef struct psql_bm25s_am_vocab_map
{
    psql_bm25s_am_vocab_map_slot *slots;
    size_t size;
    size_t capacity;
} psql_bm25s_am_vocab_map;

typedef enum psql_bm25s_am_build_mode
{
    PSQL_BM25S_AM_BUILD_MODE_IDS = 0,
    PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_SINGLE,
    PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_MULTI,
    PSQL_BM25S_AM_BUILD_MODE_SCALAR_SINGLE,
    PSQL_BM25S_AM_BUILD_MODE_SCALAR_MULTI
} psql_bm25s_am_build_mode;

typedef enum psql_bm25s_am_rebuild_builder
{
    PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD = 0,
    PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT,
    PSQL_BM25S_AM_REBUILD_BUILDER_SPILL
} psql_bm25s_am_rebuild_builder;

typedef enum psql_bm25s_am_consistency
{
    PSQL_BM25S_AM_CONSISTENCY_REALTIME = 0,
    PSQL_BM25S_AM_CONSISTENCY_EVENTUAL,
    PSQL_BM25S_AM_CONSISTENCY_MANUAL
} psql_bm25s_am_consistency;

typedef struct psql_bm25s_am_policy_recommendation
{
    const char *profile;
    const char *confidence;
    const char *recommended_options;
    const char *recommended_consistency;
    int recommended_auto_rebuild_threshold;
    int recommended_auto_rebuild_delta_bytes;
    double recommended_auto_rebuild_churn_ratio;
    bool has_recommended_auto_rebuild_threshold;
    bool has_recommended_auto_rebuild_delta_bytes;
    bool has_recommended_auto_rebuild_churn_ratio;
    bool matches_current;
    bool refresh_now;
    uint32 docs;
    uint64 pending_total;
    const char *reason;
} psql_bm25s_am_policy_recommendation;

typedef struct psql_bm25s_am_build_state
{
    Oid source_type;
    int natts;
    psql_bm25s_am_build_mode build_mode;
    psql_bm25s_text_options text_options;
    char **text_stopwords;
    size_t text_stopword_len;
    psql_bm25s_doc_tokens_builder token_docs;
    psql_bm25s_doc_ids_builder id_docs;
    psql_bm25s_tid_builder tids;
    psql_bm25s_am_term_entry_builder term_entries;
    psql_bm25s_am_doc_length_builder doc_lengths;
    psql_bm25s_am_vocab_map vocab_map;
    BufFile *spill_entries;
    uint64 spill_entry_count;
    psql_bm25s_am_rebuild_builder rebuild_builder;
    uint32_t max_token_id;
    bool has_terms;
    bool zero_present;
    double index_tuples;
} psql_bm25s_am_build_state;

typedef struct psql_bm25s_am_options
{
    int32 varlena_header_;
    int method;
    int idf_method;
    int consistency;
    int auto_rebuild_threshold;
    int auto_rebuild_delta_bytes;
    int query_overlay_max_records;
    int query_overlay_max_bytes;
    int auto_preload;
    double auto_rebuild_churn_ratio;
    double k1;
    double b;
    double delta;
    bool create_empty_token;
    bool auto_rebuild_threshold_is_set;
    bool text_lowercase;
    bool text_stem_english;
    bool text_fold_diacritics;
    bool field_aware;
    int text_stopwords;
} psql_bm25s_am_options;

typedef struct psql_bm25s_am_bgworker_args
{
    Oid db_oid;
    Oid user_oid;
    bool allow_maintenance;
} psql_bm25s_am_bgworker_args;

typedef struct psql_bm25s_am_background_launch_stamp
{
    Oid target_index_oid;
    TimestampTz last_launch;
} psql_bm25s_am_background_launch_stamp;

typedef struct psql_bm25s_am_maintenance_candidate
{
    Oid index_oid;
    uint64 debt_records;
    uint64 debt_bytes;
    bool hot;
    bool stale;
    bool urgent;
} psql_bm25s_am_maintenance_candidate;

typedef enum psql_bm25s_am_background_worker_phase
{
    PSQL_BM25S_AM_WORKER_PHASE_IDLE = 0,
    PSQL_BM25S_AM_WORKER_PHASE_PRELOAD,
    PSQL_BM25S_AM_WORKER_PHASE_INDEX_MAINTENANCE
} psql_bm25s_am_background_worker_phase;

typedef enum psql_bm25s_am_auto_preload_attempt
{
    PSQL_BM25S_AM_AUTO_PRELOAD_NONE = 0,
    PSQL_BM25S_AM_AUTO_PRELOAD_DONE,
    PSQL_BM25S_AM_AUTO_PRELOAD_BUSY
} psql_bm25s_am_auto_preload_attempt;

typedef struct psql_bm25s_am_meta_page
{
    uint32 magic;
    uint16 version;
    uint16 page_kind;
    uint16 flags;
    uint16 cache_epoch;
    Oid source_type;
    uint32 num_docs;
    uint64 tid_bytes_len;
    uint64 index_bytes_len;
    uint32 delta_record_count;
    uint64 delta_bytes_len;
    uint32 pending_write_tuples;
    uint32 pending_delete_tuples;
    uint64 rebuild_count;
    uint32 storage_version;
    uint32 active_generation;
    BlockNumber active_start_blkno;
    uint32 active_data_pages;
    BlockNumber delta_start_blkno;
} psql_bm25s_am_meta_page;

typedef struct psql_bm25s_am_data_page
{
    uint32 magic;
    uint16 version;
    uint16 page_kind;
    uint32 used_bytes;
} psql_bm25s_am_data_page;

typedef struct psql_bm25s_am_generation_data_page
{
    uint32 magic;
    uint16 version;
    uint16 page_kind;
    uint32 used_bytes;
    uint32 generation;
    uint32 ordinal;
} psql_bm25s_am_generation_data_page;

typedef struct psql_bm25s_am_payload_health_state
{
    bool corrupt;
    bool rebuild_required;
    const char *status;
    const char *reason;
    uint64 expected_bytes;
    uint64 capacity_bytes;
} psql_bm25s_am_payload_health_state;

typedef struct psql_bm25s_am_delta_record_header
{
    ItemPointerData heap_tid;
    uint32 value_bytes_len;
} psql_bm25s_am_delta_record_header;

typedef struct psql_bm25s_am_payload
{
    psql_bm25s_am_meta_page meta;
    ItemPointerData *doc_tids;
    uint8_t *index_bytes;
    size_t index_bytes_len;
} psql_bm25s_am_payload;

typedef struct psql_bm25s_am_replacement
{
    Oid source_type;
    ItemPointerData *doc_tids;
    size_t num_docs;
    uint8_t *index_bytes;
    size_t index_bytes_len;
    psql_bm25s_index index;
    bool index_valid;
    double heap_tuples;
    double index_tuples;
} psql_bm25s_am_replacement;

typedef struct psql_bm25s_am_delta_tail_record
{
    ItemPointerData heap_tid;
    uint8_t *value_bytes;
    uint32 value_bytes_len;
} psql_bm25s_am_delta_tail_record;

typedef struct psql_bm25s_am_delta_tail
{
    psql_bm25s_am_delta_tail_record *records;
    size_t len;
    size_t capacity;
    uint32 pending_writes;
    uint32 pending_deletes;
} psql_bm25s_am_delta_tail;

typedef struct psql_bm25s_search_state
{
    ItemPointerData *tids;
    psql_bm25s_topk_result topk;
    size_t pos;
} psql_bm25s_search_state;

typedef enum psql_bm25s_hybrid_fusion_method
{
    PSQL_BM25S_HYBRID_FUSION_RRF = 0,
    PSQL_BM25S_HYBRID_FUSION_SCORE
} psql_bm25s_hybrid_fusion_method;

typedef enum psql_bm25s_hybrid_normalizer
{
    PSQL_BM25S_HYBRID_NORMALIZER_IDENTITY = 0,
    PSQL_BM25S_HYBRID_NORMALIZER_NEGATIVE_DISTANCE,
    PSQL_BM25S_HYBRID_NORMALIZER_INVERSE_DISTANCE,
    PSQL_BM25S_HYBRID_NORMALIZER_MINMAX,
    PSQL_BM25S_HYBRID_NORMALIZER_ZSCORE,
    PSQL_BM25S_HYBRID_NORMALIZER_RANK
} psql_bm25s_hybrid_normalizer;

typedef enum psql_bm25s_hybrid_direction
{
    PSQL_BM25S_HYBRID_DIRECTION_HIGHER = 0,
    PSQL_BM25S_HYBRID_DIRECTION_LOWER
} psql_bm25s_hybrid_direction;

typedef struct psql_bm25s_hybrid_candidate_state
{
    char *source_name;
    char *tid_text;
    ItemPointerData tid;
    double raw_value;
    int32 source_rank_input;
    bool source_rank_is_null;
    int32 source_rank;
    double weight;
    psql_bm25s_hybrid_normalizer normalizer;
    psql_bm25s_hybrid_direction direction;
    size_t source_id;
    double normalized_score;
    double weighted_score;
    bool scored;
} psql_bm25s_hybrid_candidate_state;

typedef struct psql_bm25s_hybrid_source_state
{
    char *source_name;
    double min_value;
    double max_value;
    double sum_value;
    double sumsq_value;
    size_t count;
} psql_bm25s_hybrid_source_state;

typedef struct psql_bm25s_hybrid_hit_state
{
    ItemPointerData tid;
    char *tid_text;
    float4 score;
    int32 source_count;
    char **source_names;
    float4 *raw_values;
    float4 *normalized_scores;
    float4 *weighted_scores;
    int32 *ranks;
} psql_bm25s_hybrid_hit_state;

typedef struct psql_bm25s_hybrid_search_state
{
    psql_bm25s_hybrid_hit_state *hits;
    size_t len;
    size_t pos;
} psql_bm25s_hybrid_search_state;

typedef struct psql_bm25s_am_vocab_item
{
    const char *token;
    uint32_t token_id;
} psql_bm25s_am_vocab_item;

typedef struct psql_bm25s_am_generation_block_header
{
    uint32 magic;
    uint32 version;
    psql_bm25s_params params;
    uint32_t num_docs;
    uint32_t vocab_size;
    uint64_t data_len;
    bool has_empty_token;
    uint32_t empty_token_id;
    size_t total_size;
    size_t data_offset;
    size_t indices_offset;
    size_t indptr_offset;
    size_t term_frequencies_offset;
    size_t doc_lengths_offset;
    size_t doc_frequencies_offset;
    size_t nonoccurrence_offset;
    size_t doc_tids_offset;
    size_t vocab_offsets_offset;
    size_t vocab_bytes_offset;
    size_t vocab_bytes_len;
    size_t sorted_vocab_ids_offset;
} psql_bm25s_am_generation_block_header;

typedef struct psql_bm25s_am_generation_descriptor
{
    uint32 magic;
    uint32 version;
    Oid database_oid;
    Oid index_oid;
    RelFileLocator locator;
    psql_bm25s_am_meta_page meta;
    dsm_handle handle;
    Size mapped_size;
} psql_bm25s_am_generation_descriptor;

typedef struct psql_bm25s_am_shared_preload_entry
{
    bool in_use;
    bool ready;
    bool obsolete;
    Oid database_oid;
    Oid index_oid;
    RelFileLocator locator;
    psql_bm25s_am_meta_page meta;
    Size offset;
    Size mapped_size;
    uint32 refcount;
} psql_bm25s_am_shared_preload_entry;

typedef struct psql_bm25s_am_shared_preload_control
{
    uint32 magic;
    uint32 version;
    slock_t mutex;
    Size arena_size;
    Size used;
    uint32 active_maintenance_workers;
    uint32 active_preload_workers;
    uint32 active_index_maintenance_workers;
    Oid last_maintenance_db_oid;
    TimestampTz last_maintenance_cycle;
    uint32 entry_capacity;
    psql_bm25s_am_shared_preload_entry entries[FLEXIBLE_ARRAY_MEMBER];
} psql_bm25s_am_shared_preload_control;

typedef struct psql_bm25s_am_cache_generation
{
    RelFileLocator locator;
    psql_bm25s_am_meta_page meta;
    void *block;
    dsm_segment *segment;
    ItemPointerData *doc_tids;
    psql_bm25s_index index;
    size_t *vocab_offsets;
    uint32_t *sorted_vocab_ids;
    psql_bm25s_am_vocab_item *sorted_vocab;
    bool delta_overlay_materialized;
    bool index_owns_allocations;
    bool block_owns_allocation;
    bool mapping_pinned;
    bool shared_preload_attached;
    int shared_preload_slot;
} psql_bm25s_am_cache_generation;

typedef struct psql_bm25s_am_cache_workspace
{
    float *score_workspace;
    uint8_t *candidate_workspace;
    uint32_t *touched_doc_ids;
    size_t touched_doc_capacity;
    size_t touched_doc_len;
    TimestampTz last_used;
} psql_bm25s_am_cache_workspace;

typedef struct psql_bm25s_am_cache_entry
{
    Oid index_oid;
    uint32 lease_count;
    MemoryContext mcxt;
    psql_bm25s_am_cache_generation generation;
    psql_bm25s_am_cache_workspace workspace;
    struct psql_bm25s_am_cache_entry *delta_overlay;
} psql_bm25s_am_cache_entry;

#define PSQL_BM25S_WORKSPACE_CANDIDATE 0x01
#define PSQL_BM25S_WORKSPACE_TOUCHED 0x02

typedef struct psql_bm25s_am_cache_state
{
    MemoryContext mcxt;
    psql_bm25s_am_cache_entry *entries;
    size_t len;
    size_t capacity;
} psql_bm25s_am_cache_state;

typedef struct psql_bm25s_am_query_clause
{
    psql_bm25s_query_occur occur;
    uint32_t *token_ids;
    size_t len;
} psql_bm25s_am_query_clause;

typedef struct psql_bm25s_am_query_plan
{
    psql_bm25s_am_query_clause *clauses;
    size_t len;
    uint32_t *positive_ids;
    size_t positive_len;
    size_t positive_capacity;
    size_t positive_clause_count;
    size_t must_clause_count;
    bool has_must_not;
    bool positive_ids_aliases_clause;
} psql_bm25s_am_query_plan;

typedef struct psql_bm25s_am_visibility_ctx
{
    IndexFetchTableData *fetch;
    Snapshot snapshot;
    TupleTableSlot *slot;
} psql_bm25s_am_visibility_ctx;

typedef struct psql_bm25s_am_insert_state
{
    bool needs_refresh;
    bool refresh_running;
} psql_bm25s_am_insert_state;

typedef struct psql_bm25s_am_pending_activity
{
    Oid index_oid;
    uint32 pending_writes;
    uint32 pending_deletes;
} psql_bm25s_am_pending_activity;

typedef struct psql_bm25s_am_vacuum_stats
{
    IndexBulkDeleteResult base;
    bool needs_refresh;
    bool delete_recorded;
    uint32 pending_delete_tuples;
} psql_bm25s_am_vacuum_stats;

typedef struct psql_bm25s_am_scan_opaque
{
    psql_bm25s_am_cache_entry *entry;
    Relation heap_relation;
    bool heap_relation_owned;
    psql_bm25s_am_visibility_ctx visibility;
    psql_bm25s_topk_result ranked;
    psql_bm25s_query verify_query;
    uint32_t *deferred_candidate_ids;
    uint32_t *deferred_query_ids;
    bool initialized;
    bool use_sparse;
    bool verify_query_active;
    bool deferred_rank_active;
    size_t ranked_pos;
    uint32_t next_zero_doc_id;
    AttrNumber verify_attnum;
    Oid verify_source_type;
    psql_bm25s_text_options verify_text_options;
    char **verify_stopwords;
    size_t verify_stopword_len;
    size_t deferred_candidate_len;
    size_t deferred_query_len;
    size_t deferred_rank_limit;
    size_t deferred_rank_step;
} psql_bm25s_am_scan_opaque;

static void psql_bm25s_am_scan_opaque_reset(
    psql_bm25s_am_scan_opaque *opaque
);
static bool psql_bm25s_am_cache_entry_matches(
    const psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
);
static void psql_bm25s_am_cache_build_sorted_vocab(
    psql_bm25s_am_cache_entry *entry
);
static char *psql_bm25s_am_generation_ptr(void *block, size_t offset);
static bool psql_bm25s_am_shared_preload_available(void);
static bool psql_bm25s_am_shared_preload_cache_available(void);
static char *psql_bm25s_am_shared_preload_arena_base(void);
static void psql_bm25s_am_shared_preload_advise_hugepage(void);
static uint32 psql_bm25s_am_shared_preload_entry_capacity(void);
static void psql_bm25s_am_shared_preload_release_ref(int slot);
static void psql_bm25s_am_prewarm_delta_pages(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
);
Datum psql_bm25s_generation_cache_clear(PG_FUNCTION_ARGS);
Datum psql_bm25s_generation_cache_state(PG_FUNCTION_ARGS);
Datum psql_bm25s_generation_cache_preload(PG_FUNCTION_ARGS);
Datum psql_bm25s_field_aware_query_tokens(PG_FUNCTION_ARGS);
static void psql_bm25s_am_scan_init(
    IndexScanDesc scan,
    psql_bm25s_am_scan_opaque *opaque
);
static bool psql_bm25s_am_scan_next_hit(
    psql_bm25s_am_scan_opaque *opaque,
    ItemPointerData *tid_out,
    uint32_t *doc_id_out,
    float *score_out
);
static bool psql_bm25s_am_scan_doc_matches_verify_query(
    psql_bm25s_am_scan_opaque *opaque,
    const ItemPointerData *tid
);
static void psql_bm25s_am_rank_candidate_ids_prefix(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    size_t limit,
    psql_bm25s_topk_result *ranked_out
);
static bool psql_bm25s_am_scan_expand_deferred_ordered_ranking(
    psql_bm25s_am_scan_opaque *opaque
);
static void psql_bm25s_am_scan_init_query_match(
    IndexScanDesc scan,
    psql_bm25s_am_scan_opaque *opaque
);
static void psql_bm25s_am_scan_init_query_match_ordered(
    IndexScanDesc scan,
    psql_bm25s_am_scan_opaque *opaque
);
static void psql_bm25s_am_parse_prepared_query_datum(
    Datum prepared_datum,
    Oid expected_index_oid,
    psql_bm25s_query *query_out
);
static void psql_bm25s_am_parse_scan_query_key(
    Relation indexRelation,
    ScanKey key,
    psql_bm25s_query *query_out
);
static void psql_bm25s_am_init_ranked_from_doc_ids(
    const uint32_t *doc_ids,
    size_t doc_len,
    psql_bm25s_topk_result *ranked_out
);
static int psql_bm25s_am_cmp_uint32_asc(const void *lhs, const void *rhs);
static void psql_bm25s_am_free_query_tokens(
    char **query_tokens,
    size_t query_len
);
static void psql_bm25s_am_read_index_text_options(
    Relation indexRelation,
    psql_bm25s_text_options *options_out,
    char ***stopwords_out,
    size_t *num_stopwords_out
);
static ArrayType *psql_bm25s_am_text_array_from_cstrings(
    char **tokens,
    size_t len
);
static void psql_bm25s_am_prepare_phrase_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const psql_bm25s_am_query_plan *plan,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_prepare_verified_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const uint32_t *query_ids,
    size_t query_len,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_prepare_tokens_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_prepare_field_tokens_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    ArrayType *weight_mask,
    size_t requested_k,
    bool include_delta,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_init_ranked_storage(
    size_t candidate_len,
    psql_bm25s_topk_result *ranked_out
);
static void psql_bm25s_am_merge_textlike_delta_tokens(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_merge_field_delta_tokens(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
);
static void psql_bm25s_am_read_all_field_specs(
    Relation indexRelation,
    int **field_indexes_out,
    float **field_weights_out,
    size_t *num_fields_out
);
static void psql_bm25s_am_read_meta(
    Relation indexRelation,
    psql_bm25s_am_meta_page *meta_out
);
static BlockNumber psql_bm25s_am_relation_nblocks(Relation indexRelation);
static void psql_bm25s_am_load_payload(
    Relation indexRelation,
    bool suppress_stale_warning,
    psql_bm25s_am_payload *payload_out
);
static void psql_bm25s_am_payload_health(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    psql_bm25s_am_payload_health_state *health_out
);
static bool psql_bm25s_am_meta_uses_append_only(
    const psql_bm25s_am_meta_page *meta
);
static void psql_bm25s_am_write_generation_pages(
    Relation indexRelation,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const uint8_t *index_bytes,
    size_t index_bytes_len,
    uint32 generation,
    BlockNumber *start_blkno_out,
    uint32 *data_pages_out
);
static bool psql_bm25s_am_rebuild_memory_budget_choose(
    const psql_bm25s_am_meta_page *meta,
    psql_bm25s_am_rebuild_builder *builder_out,
    uint64 *standard_estimated_bytes_out,
    uint64 *compact_estimated_bytes_out,
    uint64 *spill_estimated_bytes_out,
    uint64 *budget_bytes_out
);
static psql_bm25s_am_rebuild_builder
psql_bm25s_am_choose_explicit_build_builder(
    Relation heapRelation,
    Relation indexRelation
);
static void psql_bm25s_am_replacement_free(
    psql_bm25s_am_replacement *replacement
);
static void psql_bm25s_am_delta_tail_free(
    psql_bm25s_am_delta_tail *tail
);
static void psql_bm25s_am_build_replacement(
    Relation heapRelation,
    Relation indexRelation,
    IndexInfo *indexInfo,
    psql_bm25s_am_rebuild_builder builder,
    bool keep_index_for_streaming,
    psql_bm25s_am_replacement *replacement_out
);
static IndexBuildResult *psql_bm25s_am_build_common(
    Relation heapRelation,
    Relation indexRelation,
    IndexInfo *indexInfo
);
static void psql_bm25s_am_xact_callback(XactEvent event, void *arg);
static bool psql_bm25s_am_should_defer_foreground_refresh_oid(Oid index_oid);
static bool psql_bm25s_am_can_use_delta_overlay(
    const psql_bm25s_am_meta_page *meta
);
static void psql_bm25s_am_load_delta_state(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *snapshot_meta,
    Oid source_type,
    uint32 expected_delta_records,
    psql_bm25s_doc_ids_builder *id_docs_out,
    psql_bm25s_doc_tokens_builder *token_docs_out,
    psql_bm25s_tid_builder *tids_out,
    psql_bm25s_docid_builder *deleted_doc_ids_out
);
static psql_bm25s_am_cache_entry *psql_bm25s_am_get_delta_overlay_entry(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
);
static bool psql_bm25s_am_meta_has_pending_maintenance(
    const psql_bm25s_am_meta_page *meta
);
static bool psql_bm25s_am_eventual_background_maintenance_due(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    const psql_bm25s_am_payload_health_state *health
);
static void psql_bm25s_am_reconstruct_ids_from_index(
    const psql_bm25s_index *index,
    psql_bm25s_doc_ids_builder *docs_out
);
static void psql_bm25s_am_reconstruct_tokens_from_index(
    const psql_bm25s_index *index,
    psql_bm25s_doc_tokens_builder *docs_out
);
static void psql_bm25s_am_collect_query_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_am_query_plan *plan,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
);
static void psql_bm25s_am_collect_candidate_ids_from_query_ids(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *query_ids,
    size_t query_len,
    bool all_docs_if_empty,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
);
static void psql_bm25s_am_collect_phrase_term_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query_term *term,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
);
static void psql_bm25s_am_rank_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    psql_bm25s_topk_result *ranked_out
);
static void psql_bm25s_am_cache_load_overlay(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
);
static void psql_bm25s_am_require_index_relation(Relation indexRelation);
static void psql_bm25s_am_require_index_owner(Relation indexRelation);
static text *psql_bm25s_am_describe_relation(Relation indexRelation);
static int psql_bm25s_am_auto_preload_priority(Relation indexRelation);
static void psql_bm25s_am_mark_replaced_docs(
    Relation indexRelation,
    const psql_bm25s_am_payload *payload,
    const psql_bm25s_tid_builder *delta_tids,
    bool *deleted_docs,
    size_t total_docs
);
static bool psql_bm25s_am_meta_equal_for_online_swap(
    const psql_bm25s_am_meta_page *start,
    const psql_bm25s_am_meta_page *current
);
static void psql_bm25s_am_note_maintenance_activity(
    Relation indexRelation,
    uint32 pending_write_add,
    uint32 pending_delete_add
);
static void psql_bm25s_am_note_pending_maintenance_activity(
    Relation indexRelation,
    uint32 pending_write_add,
    uint32 pending_delete_add
);
static void psql_bm25s_am_flush_pending_maintenance_activity(void);
static void psql_bm25s_am_clear_pending_maintenance_activity(void);
static text *psql_bm25s_am_online_maintain_relation(
    Relation indexRelation,
    IndexInfo *indexInfo
);
static text *psql_bm25s_am_try_maintain_index_oid(Oid index_oid);
static text *psql_bm25s_am_try_blocking_maintain_oid(Oid index_oid);
static void psql_bm25s_am_schedule_background_maintenance(
    Relation indexRelation
);
static void psql_bm25s_am_touch_background_maintenance(void);
static bool psql_bm25s_am_launch_background_maintenance(
    Oid db_oid,
    Oid user_oid,
    bool allow_maintenance
);
static int psql_bm25s_am_effective_maintenance_worker_limit(void);
static int psql_bm25s_am_maintenance_timer_interval(void);
static int psql_bm25s_am_preload_timer_interval(void);
static bool psql_bm25s_am_try_maintenance_worker_slot(int *slot_out);
static void psql_bm25s_am_release_maintenance_worker_slot(int slot);
static void psql_bm25s_am_register_maintenance_worker_exit(void);
static int psql_bm25s_am_maintenance_worker_due_indexes(void);
static void psql_bm25s_am_note_background_worker_phase(
    psql_bm25s_am_background_worker_phase phase
);
static void psql_bm25s_am_validate_policy_reloptions(
    Datum reloptions,
    const psql_bm25s_am_options *options
);
static void psql_bm25s_am_set_explicit_reloption_flags(
    Datum reloptions,
    psql_bm25s_am_options *options
);
static bool psql_bm25s_am_maintenance_result_counts(text *result);
static bool psql_bm25s_am_eventual_policy_enabled(Relation indexRelation);
static bool psql_bm25s_am_foreground_maintenance_enabled(
    Relation indexRelation
);
static void psql_bm25s_am_lock_maintenance(Relation indexRelation);
static void psql_bm25s_am_unlock_maintenance(Relation indexRelation);
static void psql_bm25s_am_cache_reset_all(void);
static void psql_bm25s_am_cache_shmem_exit(int code, Datum arg);

PGDLLEXPORT void psql_bm25s_maintenance_worker_main(Datum main_arg);
PGDLLEXPORT void psql_bm25s_maintenance_supervisor_main(Datum main_arg);

static relopt_kind psql_bm25s_relopt_kind = RELOPT_KIND_LOCAL;
static bool psql_bm25s_relopts_initialized = false;
static psql_bm25s_am_cache_state psql_bm25s_am_cache = {0};
static List *psql_bm25s_am_pending_refresh_oids = NIL;
static List *psql_bm25s_am_pinned_maintenance_oids = NIL;
static List *psql_bm25s_am_pending_maintenance_activity = NIL;
static List *psql_bm25s_am_background_launch_stamps = NIL;
static bool psql_bm25s_am_pending_background_maintenance = false;
static bool psql_bm25s_am_xact_callback_registered = false;
static bool psql_bm25s_am_flush_running = false;
static bool psql_bm25s_am_workspace_gucs_initialized = false;
static bool psql_bm25s_am_maintenance_gucs_initialized = false;
static bool psql_bm25s_am_shared_preload_gucs_initialized = false;
static bool psql_bm25s_am_maintenance_worker_counted_active = false;
static bool psql_bm25s_am_maintenance_worker_exit_registered = false;
static bool psql_bm25s_am_cache_exit_registered = false;
static bool psql_bm25s_am_preload_publisher_active = false;
static psql_bm25s_am_background_worker_phase
    psql_bm25s_am_current_worker_phase =
        PSQL_BM25S_AM_WORKER_PHASE_IDLE;
static int psql_bm25s_shared_generation_cache_size_mb = 0;
static int psql_bm25s_maintenance_worker_limit =
    PSQL_BM25S_AM_DEFAULT_MAINTENANCE_WORKER_LIMIT;
static int psql_bm25s_maintenance_timer_interval_ms =
    PSQL_BM25S_AM_DEFAULT_MAINTENANCE_TIMER_INTERVAL_MS;
static int psql_bm25s_preload_timer_interval_ms =
    PSQL_BM25S_AM_DEFAULT_PRELOAD_TIMER_INTERVAL_MS;
static int psql_bm25s_maintenance_rebuild_memory_budget_mb =
    PSQL_BM25S_AM_DEFAULT_REBUILD_MEMORY_BUDGET_MB;
static int psql_bm25s_workspace_cache_bytes =
    PSQL_BM25S_AM_DEFAULT_WORKSPACE_CACHE_BYTES;
static int psql_bm25s_workspace_idle_timeout_ms =
    PSQL_BM25S_AM_DEFAULT_WORKSPACE_IDLE_TIMEOUT_MS;
static shmem_request_hook_type psql_bm25s_prev_shmem_request_hook = NULL;
static shmem_startup_hook_type psql_bm25s_prev_shmem_startup_hook = NULL;
static psql_bm25s_am_shared_preload_control *psql_bm25s_shared_preload = NULL;

static const relopt_enum_elt_def psql_bm25s_method_members[] = {
    {"robertson", PSQL_BM25S_METHOD_ROBERTSON},
    {"lucene", PSQL_BM25S_METHOD_LUCENE},
    {"atire", PSQL_BM25S_METHOD_ATIRE},
    {"bm25l", PSQL_BM25S_METHOD_BM25L},
    {"bm25+", PSQL_BM25S_METHOD_BM25PLUS},
    {NULL, 0}
};

static const relopt_enum_elt_def psql_bm25s_consistency_members[] = {
    {"realtime", PSQL_BM25S_AM_CONSISTENCY_REALTIME},
    {"eventual", PSQL_BM25S_AM_CONSISTENCY_EVENTUAL},
    {"manual", PSQL_BM25S_AM_CONSISTENCY_MANUAL},
    {NULL, 0}
};

static const relopt_parse_elt psql_bm25s_relopt_elems[] = {
    {"method", RELOPT_TYPE_ENUM, offsetof(psql_bm25s_am_options, method)},
    {"idf_method", RELOPT_TYPE_ENUM, offsetof(psql_bm25s_am_options, idf_method)},
    {
        "consistency",
        RELOPT_TYPE_ENUM,
        offsetof(psql_bm25s_am_options, consistency)
    },
    {
        "auto_rebuild_threshold",
        RELOPT_TYPE_INT,
        offsetof(psql_bm25s_am_options, auto_rebuild_threshold)
    },
    {
        "auto_rebuild_delta_bytes",
        RELOPT_TYPE_INT,
        offsetof(psql_bm25s_am_options, auto_rebuild_delta_bytes)
    },
    {
        "query_overlay_max_records",
        RELOPT_TYPE_INT,
        offsetof(psql_bm25s_am_options, query_overlay_max_records)
    },
    {
        "query_overlay_max_bytes",
        RELOPT_TYPE_INT,
        offsetof(psql_bm25s_am_options, query_overlay_max_bytes)
    },
    {
        "auto_preload",
        RELOPT_TYPE_INT,
        offsetof(psql_bm25s_am_options, auto_preload)
    },
    {
        "auto_rebuild_churn_ratio",
        RELOPT_TYPE_REAL,
        offsetof(psql_bm25s_am_options, auto_rebuild_churn_ratio)
    },
    {"k1", RELOPT_TYPE_REAL, offsetof(psql_bm25s_am_options, k1)},
    {"b", RELOPT_TYPE_REAL, offsetof(psql_bm25s_am_options, b)},
    {"delta", RELOPT_TYPE_REAL, offsetof(psql_bm25s_am_options, delta)},
    {
        "create_empty_token",
        RELOPT_TYPE_BOOL,
        offsetof(psql_bm25s_am_options, create_empty_token)
    },
    {
        "text_lowercase",
        RELOPT_TYPE_BOOL,
        offsetof(psql_bm25s_am_options, text_lowercase)
    },
    {
        "text_stem_english",
        RELOPT_TYPE_BOOL,
        offsetof(psql_bm25s_am_options, text_stem_english)
    },
    {
        "text_fold_diacritics",
        RELOPT_TYPE_BOOL,
        offsetof(psql_bm25s_am_options, text_fold_diacritics)
    },
    {
        "field_aware",
        RELOPT_TYPE_BOOL,
        offsetof(psql_bm25s_am_options, field_aware)
    },
    {
        "text_stopwords",
        RELOPT_TYPE_STRING,
        offsetof(psql_bm25s_am_options, text_stopwords)
    }
};

static int
psql_bm25s_am_cmp_vocab_item(const void *lhs, const void *rhs)
{
    const psql_bm25s_am_vocab_item *left = lhs;
    const psql_bm25s_am_vocab_item *right = rhs;

    return strcmp(left->token, right->token);
}

static size_t
psql_bm25s_am_generation_vocab_size(
    const psql_bm25s_am_cache_entry *entry
)
{
    return entry->generation.index.vocab_size;
}

static const char *
psql_bm25s_am_generation_vocab_token(
    const psql_bm25s_am_cache_entry *entry,
    uint32_t token_id
)
{
    if (entry->generation.vocab_offsets != NULL)
    {
        return psql_bm25s_am_generation_ptr(
            entry->generation.block,
            entry->generation.vocab_offsets[token_id]
        );
    }
    return entry->generation.index.vocab[token_id];
}

static size_t
psql_bm25s_am_vocab_lower_bound(
    const psql_bm25s_am_cache_entry *entry,
    const char *token
)
{
    size_t left = 0;
    size_t right;

    right = psql_bm25s_am_generation_vocab_size(entry);
    while (left < right)
    {
        size_t mid = left + (right - left) / 2;
        const char *mid_token;

        if (entry->generation.sorted_vocab_ids != NULL)
        {
            mid_token = psql_bm25s_am_generation_vocab_token(
                entry,
                entry->generation.sorted_vocab_ids[mid]
            );
        }
        else
        {
            mid_token = entry->generation.sorted_vocab[mid].token;
        }

        if (strcmp(mid_token, token) < 0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

static const char *
psql_bm25s_am_sorted_vocab_token(
    const psql_bm25s_am_cache_entry *entry,
    size_t pos
)
{
    if (entry->generation.sorted_vocab_ids != NULL)
    {
        return psql_bm25s_am_generation_vocab_token(
            entry,
            entry->generation.sorted_vocab_ids[pos]
        );
    }
    return entry->generation.sorted_vocab[pos].token;
}

static uint32_t
psql_bm25s_am_sorted_vocab_token_id(
    const psql_bm25s_am_cache_entry *entry,
    size_t pos
)
{
    if (entry->generation.sorted_vocab_ids != NULL)
    {
        return entry->generation.sorted_vocab_ids[pos];
    }
    return entry->generation.sorted_vocab[pos].token_id;
}

static bool
psql_bm25s_am_checked_mul_size(size_t a, size_t b, size_t *out)
{
    if (a == 0 || b == 0)
    {
        *out = 0;
        return true;
    }

    if (a > ((size_t) -1) / b)
    {
        return false;
    }

    *out = a * b;
    return true;
}

static bool
psql_bm25s_am_checked_add_size(size_t a, size_t b, size_t *out)
{
    if (a > ((size_t) -1) - b)
    {
        return false;
    }

    *out = a + b;
    return true;
}

static bool
psql_bm25s_am_generation_reserve(
    size_t *cursor,
    size_t bytes,
    size_t *offset_out
)
{
    size_t aligned;
    size_t next;

    aligned = MAXALIGN(*cursor);
    if (!psql_bm25s_am_checked_add_size(aligned, bytes, &next))
    {
        return false;
    }

    *offset_out = aligned;
    *cursor = next;
    return true;
}

static char *
psql_bm25s_am_generation_ptr(void *block, size_t offset)
{
    if (offset == 0)
    {
        return NULL;
    }

    return ((char *) block) + offset;
}

static uint16_t
psql_bm25s_am_read_u16_le(const uint8_t *src)
{
    return (uint16_t) src[0] |
           (uint16_t) ((uint16_t) src[1] << 8);
}

static uint32_t
psql_bm25s_am_read_u32_le(const uint8_t *src)
{
    return (uint32_t) src[0] |
           ((uint32_t) src[1] << 8) |
           ((uint32_t) src[2] << 16) |
           ((uint32_t) src[3] << 24);
}

static uint64_t
psql_bm25s_am_read_u64_le(const uint8_t *src)
{
    uint64_t value = 0;
    size_t i;

    for (i = 0; i < 8; i++)
    {
        value |= ((uint64_t) src[i]) << (i * 8);
    }

    return value;
}

static float
psql_bm25s_am_read_f32_le(const uint8_t *src)
{
    union
    {
        float f;
        uint32_t u;
    } conv;

    conv.u = psql_bm25s_am_read_u32_le(src);
    return conv.f;
}

static void
psql_bm25s_am_oom(void)
{
    ereport(
        ERROR,
        (
            errcode(ERRCODE_OUT_OF_MEMORY),
            errmsg("out of memory")
        )
    );
}

static void
psql_bm25s_am_cache_init(void)
{
    if (psql_bm25s_am_cache.mcxt != NULL)
    {
        return;
    }

    psql_bm25s_am_cache.mcxt = AllocSetContextCreate(
        CacheMemoryContext,
        "psql_bm25s cache",
        ALLOCSET_DEFAULT_SIZES
    );
    if (!psql_bm25s_am_cache_exit_registered)
    {
        before_shmem_exit(psql_bm25s_am_cache_shmem_exit, 0);
        psql_bm25s_am_cache_exit_registered = true;
    }
}

static void
psql_bm25s_am_cache_entry_reset(psql_bm25s_am_cache_entry *entry)
{
    if (entry == NULL)
    {
        return;
    }

    if (entry->generation.shared_preload_attached)
    {
        psql_bm25s_am_shared_preload_release_ref(
            entry->generation.shared_preload_slot
        );
    }
    if (entry->delta_overlay != NULL)
    {
        psql_bm25s_am_cache_entry_reset(entry->delta_overlay);
    }
    if (entry->generation.block != NULL &&
        entry->generation.block_owns_allocation)
    {
        free(entry->generation.block);
    }
    if (entry->generation.segment != NULL)
    {
        if (entry->generation.mapping_pinned)
        {
            dsm_unpin_mapping(entry->generation.segment);
        }
        dsm_detach(entry->generation.segment);
    }
    if (entry->mcxt != NULL)
    {
        MemoryContextDelete(entry->mcxt);
    }
    if (entry->generation.index_owns_allocations)
    {
        psql_bm25s_index_free(&entry->generation.index);
    }
    else
    {
        psql_bm25s_index_init(&entry->generation.index);
    }
    memset(&entry->generation.locator, 0, sizeof(entry->generation.locator));
    memset(&entry->generation.meta, 0, sizeof(entry->generation.meta));
    entry->mcxt = NULL;
    entry->generation.block = NULL;
    entry->generation.segment = NULL;
    entry->generation.doc_tids = NULL;
    entry->generation.vocab_offsets = NULL;
    entry->generation.sorted_vocab_ids = NULL;
    entry->generation.sorted_vocab = NULL;
    entry->workspace.score_workspace = NULL;
    entry->workspace.candidate_workspace = NULL;
    entry->workspace.touched_doc_ids = NULL;
    entry->workspace.touched_doc_capacity = 0;
    entry->workspace.touched_doc_len = 0;
    entry->workspace.last_used = 0;
    entry->delta_overlay = NULL;
    entry->generation.delta_overlay_materialized = false;
    entry->generation.index_owns_allocations = false;
    entry->generation.block_owns_allocation = false;
    entry->generation.mapping_pinned = false;
    entry->generation.shared_preload_attached = false;
    entry->generation.shared_preload_slot = -1;
    entry->lease_count = 0;
}

static void
psql_bm25s_am_cache_acquire_lease(psql_bm25s_am_cache_entry *entry)
{
    if (entry == NULL)
    {
        return;
    }
    if (entry->lease_count == UINT32_MAX)
    {
        ereport(ERROR, (errmsg("psql_bm25s cache lease count overflow")));
    }
    entry->lease_count++;
}

static void
psql_bm25s_am_cache_release_lease(psql_bm25s_am_cache_entry *entry)
{
    Oid index_oid;

    if (entry == NULL || entry->lease_count == 0)
    {
        return;
    }
    entry->lease_count--;
    if (entry->lease_count > 0 || !entry->generation.shared_preload_attached)
    {
        return;
    }

    /*
     * Shared-preload generations are cheap to reattach from the resident
     * arena. Release them as soon as the scan/SRF/preload statement is done
     * so idle backends cannot pin obsolete generations indefinitely.
     */
    index_oid = entry->index_oid;
    psql_bm25s_am_cache_entry_reset(entry);
    entry->index_oid = index_oid;
}

static Size
psql_bm25s_am_cache_workspace_bytes(
    const psql_bm25s_am_cache_entry *entry
)
{
    Size bytes = 0;

    if (entry == NULL)
    {
        return 0;
    }
    if (entry->workspace.score_workspace != NULL)
    {
        bytes += sizeof(*entry->workspace.score_workspace) *
            entry->generation.index.num_docs;
    }
    if (entry->workspace.candidate_workspace != NULL)
    {
        bytes += sizeof(*entry->workspace.candidate_workspace) *
            entry->generation.index.num_docs;
    }
    if (entry->workspace.touched_doc_ids != NULL)
    {
        bytes += sizeof(*entry->workspace.touched_doc_ids) *
            entry->workspace.touched_doc_capacity;
    }

    return bytes;
}

static void
psql_bm25s_am_cache_release_workspace(psql_bm25s_am_cache_entry *entry)
{
    if (entry == NULL)
    {
        return;
    }
    if (entry->workspace.score_workspace != NULL)
    {
        pfree(entry->workspace.score_workspace);
    }
    if (entry->workspace.candidate_workspace != NULL)
    {
        pfree(entry->workspace.candidate_workspace);
    }
    if (entry->workspace.touched_doc_ids != NULL)
    {
        pfree(entry->workspace.touched_doc_ids);
    }

    entry->workspace.score_workspace = NULL;
    entry->workspace.candidate_workspace = NULL;
    entry->workspace.touched_doc_ids = NULL;
    entry->workspace.touched_doc_capacity = 0;
    entry->workspace.touched_doc_len = 0;
    entry->workspace.last_used = 0;
}

static void
psql_bm25s_am_cache_maybe_shrink_workspace(
    psql_bm25s_am_cache_entry *entry
)
{
    Size bytes;

    bytes = psql_bm25s_am_cache_workspace_bytes(entry);
    if (bytes == 0)
    {
        return;
    }
    if (psql_bm25s_workspace_cache_bytes >= 0 &&
        bytes > (Size) psql_bm25s_workspace_cache_bytes)
    {
        psql_bm25s_am_cache_release_workspace(entry);
        return;
    }

    entry->workspace.last_used = GetCurrentTimestamp();
}

static void
psql_bm25s_am_cache_maybe_shrink_idle_workspace(
    psql_bm25s_am_cache_entry *entry
)
{
    Size bytes;
    TimestampTz now;

    bytes = psql_bm25s_am_cache_workspace_bytes(entry);
    if (bytes == 0)
    {
        return;
    }
    if (psql_bm25s_workspace_cache_bytes >= 0 &&
        bytes > (Size) psql_bm25s_workspace_cache_bytes)
    {
        psql_bm25s_am_cache_release_workspace(entry);
        return;
    }
    if (entry == NULL ||
        entry->workspace.last_used == 0 ||
        psql_bm25s_workspace_idle_timeout_ms < 0)
    {
        return;
    }

    now = GetCurrentTimestamp();
    if (psql_bm25s_workspace_idle_timeout_ms == 0 ||
        TimestampDifferenceExceeds(
            entry->workspace.last_used,
            now,
            psql_bm25s_workspace_idle_timeout_ms
        ))
    {
        psql_bm25s_am_cache_release_workspace(entry);
    }
}

static void
psql_bm25s_am_cache_reset_all(void)
{
    size_t i;

    if (psql_bm25s_am_cache.mcxt == NULL)
    {
        return;
    }
    for (i = 0; i < psql_bm25s_am_cache.len; i++)
    {
        psql_bm25s_am_cache_entry_reset(&psql_bm25s_am_cache.entries[i]);
        memset(
            &psql_bm25s_am_cache.entries[i],
            0,
            sizeof(psql_bm25s_am_cache.entries[i])
        );
    }
    psql_bm25s_am_cache.len = 0;
}

static void
psql_bm25s_am_cache_release_shared_preload_leases(void)
{
    size_t i;

    if (psql_bm25s_am_cache.mcxt == NULL)
    {
        return;
    }
    for (i = 0; i < psql_bm25s_am_cache.len; i++)
    {
        Oid index_oid;

        if (!psql_bm25s_am_cache.entries[i].generation.shared_preload_attached)
        {
            continue;
        }
        index_oid = psql_bm25s_am_cache.entries[i].index_oid;
        psql_bm25s_am_cache_entry_reset(&psql_bm25s_am_cache.entries[i]);
        psql_bm25s_am_cache.entries[i].index_oid = index_oid;
    }
}

static void
psql_bm25s_am_cache_shmem_exit(int code, Datum arg)
{
    (void) code;
    (void) arg;

    psql_bm25s_am_cache_reset_all();
}

static void
psql_bm25s_am_search_state_reset(psql_bm25s_search_state *state)
{
    if (state == NULL)
    {
        return;
    }

    if (state->tids != NULL)
    {
        pfree(state->tids);
        state->tids = NULL;
    }
    psql_bm25s_topk_result_free(&state->topk);
    state->pos = 0;
}

static bool
psql_bm25s_am_generation_block_layout_serialized(
    const psql_bm25s_am_payload *payload,
    uint16_t flags,
    psql_bm25s_am_generation_block_header *header
)
{
    size_t cursor = sizeof(*header);
    size_t bytes;
    size_t vocab_bytes = 0;
    const uint8_t *ptr;
    size_t remaining;
    uint32_t i;

    if (payload == NULL || payload->index_bytes == NULL || header == NULL ||
        payload->index_bytes_len < PSQL_BM25S_HEADER_SIZE)
    {
        return false;
    }

    ptr = payload->index_bytes + PSQL_BM25S_HEADER_SIZE;
    remaining = payload->index_bytes_len - PSQL_BM25S_HEADER_SIZE;

    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->data_len,
            sizeof(float),
            &bytes) ||
        bytes > remaining ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->data_offset))
    {
        return false;
    }
    ptr += bytes;
    remaining -= bytes;

    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->data_len,
            sizeof(uint32_t),
            &bytes) ||
        bytes > remaining ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->indices_offset))
    {
        return false;
    }
    ptr += bytes;
    remaining -= bytes;

    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->vocab_size + 1,
            sizeof(uint64_t),
            &bytes) ||
        bytes > remaining ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->indptr_offset))
    {
        return false;
    }
    ptr += bytes;
    remaining -= bytes;

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->data_len,
                sizeof(uint32_t),
                &bytes) ||
            bytes > remaining ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->term_frequencies_offset))
        {
            return false;
        }
        ptr += bytes;
        remaining -= bytes;

        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->num_docs,
                sizeof(uint32_t),
                &bytes) ||
            bytes > remaining ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->doc_lengths_offset))
        {
            return false;
        }
        ptr += bytes;
        remaining -= bytes;

        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(uint32_t),
                &bytes) ||
            bytes > remaining ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->doc_frequencies_offset))
        {
            return false;
        }
        ptr += bytes;
        remaining -= bytes;
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(float),
                &bytes) ||
            bytes > remaining ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->nonoccurrence_offset))
        {
            return false;
        }
        ptr += bytes;
        remaining -= bytes;
    }

    if (payload->meta.tid_bytes_len > 0)
    {
        if (!psql_bm25s_am_generation_reserve(
                &cursor,
                (size_t) payload->meta.tid_bytes_len,
                &header->doc_tids_offset))
        {
            return false;
        }
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(size_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->vocab_offsets_offset))
        {
            return false;
        }

        for (i = 0; i < header->vocab_size; i++)
        {
            uint32_t token_len;
            size_t token_bytes;

            if (remaining < sizeof(uint32_t))
            {
                return false;
            }
            token_len = psql_bm25s_am_read_u32_le(ptr);
            ptr += sizeof(uint32_t);
            remaining -= sizeof(uint32_t);
            if (token_len > remaining)
            {
                return false;
            }
            token_bytes = (size_t) token_len + 1;
            if (!psql_bm25s_am_checked_add_size(
                    vocab_bytes,
                    token_bytes,
                    &vocab_bytes))
            {
                return false;
            }
            ptr += token_len;
            remaining -= token_len;
        }
        header->vocab_bytes_len = vocab_bytes;
        if (!psql_bm25s_am_generation_reserve(
                &cursor,
                vocab_bytes,
                &header->vocab_bytes_offset))
        {
            return false;
        }
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(uint32_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->sorted_vocab_ids_offset))
        {
            return false;
        }
    }

    if (remaining != 0)
    {
        return false;
    }

    header->total_size = MAXALIGN(cursor);
    return true;
}

static uint16_t
psql_bm25s_am_generation_flags_from_index(const psql_bm25s_index *index)
{
    uint16_t flags = 0;

    if (index->nonoccurrence != NULL)
    {
        flags |= PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE;
    }
    if (index->vocab != NULL)
    {
        flags |= PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB;
    }
    if (index->has_empty_token)
    {
        flags |= PSQL_BM25S_STORAGE_FLAG_HAS_EMPTY_TOKEN;
    }
    if (index->term_frequencies != NULL &&
        index->doc_lengths != NULL &&
        index->doc_frequencies != NULL)
    {
        flags |= PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS;
    }
    return flags;
}

static bool
psql_bm25s_am_generation_block_layout_index(
    const psql_bm25s_index *index,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    uint16_t flags,
    psql_bm25s_am_generation_block_header *header
)
{
    size_t cursor = sizeof(*header);
    size_t bytes;
    size_t vocab_bytes = 0;
    uint32_t i;

    if (index == NULL || header == NULL ||
        (num_docs > 0 && doc_tids == NULL) ||
        num_docs != (size_t) index->num_docs)
    {
        return false;
    }

    memset(header, 0, sizeof(*header));
    header->magic = PSQL_BM25S_AM_GENERATION_MAGIC;
    header->version = PSQL_BM25S_AM_GENERATION_VERSION;
    header->params = index->params;
    header->num_docs = index->num_docs;
    header->vocab_size = index->vocab_size;
    header->data_len = index->data_len;
    header->has_empty_token = index->has_empty_token;
    header->empty_token_id = index->empty_token_id;

    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->data_len,
            sizeof(float),
            &bytes) ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->data_offset))
    {
        return false;
    }
    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->data_len,
            sizeof(uint32_t),
            &bytes) ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->indices_offset))
    {
        return false;
    }
    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->vocab_size + 1,
            sizeof(uint64_t),
            &bytes) ||
        !psql_bm25s_am_generation_reserve(
            &cursor,
            bytes,
            &header->indptr_offset))
    {
        return false;
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->data_len,
                sizeof(uint32_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->term_frequencies_offset))
        {
            return false;
        }
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->num_docs,
                sizeof(uint32_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->doc_lengths_offset))
        {
            return false;
        }
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(uint32_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->doc_frequencies_offset))
        {
            return false;
        }
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(float),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->nonoccurrence_offset))
        {
            return false;
        }
    }

    if (num_docs > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                num_docs,
                sizeof(*doc_tids),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->doc_tids_offset))
        {
            return false;
        }
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB) != 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(size_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->vocab_offsets_offset))
        {
            return false;
        }
        for (i = 0; i < header->vocab_size; i++)
        {
            size_t token_len;
            size_t token_bytes;

            if (index->vocab[i] == NULL)
            {
                return false;
            }
            token_len = strlen(index->vocab[i]);
            token_bytes = token_len + 1;
            if (!psql_bm25s_am_checked_add_size(
                    vocab_bytes,
                    token_bytes,
                    &vocab_bytes))
            {
                return false;
            }
        }
        header->vocab_bytes_len = vocab_bytes;
        if (!psql_bm25s_am_generation_reserve(
                &cursor,
                vocab_bytes,
                &header->vocab_bytes_offset))
        {
            return false;
        }
        if (!psql_bm25s_am_checked_mul_size(
                (size_t) header->vocab_size,
                sizeof(uint32_t),
                &bytes) ||
            !psql_bm25s_am_generation_reserve(
                &cursor,
                bytes,
                &header->sorted_vocab_ids_offset))
        {
            return false;
        }
    }

    header->total_size = MAXALIGN(cursor);
    return true;
}

static bool
psql_bm25s_am_generation_parse_serialized_header(
    const psql_bm25s_am_payload *payload,
    uint16_t *flags_out,
    psql_bm25s_am_generation_block_header *header
)
{
    uint16_t version;
    uint16_t flags;

    if (payload == NULL || payload->index_bytes == NULL ||
        payload->index_bytes_len < PSQL_BM25S_HEADER_SIZE ||
        flags_out == NULL || header == NULL)
    {
        return false;
    }
    if (psql_bm25s_am_read_u32_le(payload->index_bytes) !=
        PSQL_BM25S_STORAGE_MAGIC)
    {
        return false;
    }

    version = psql_bm25s_am_read_u16_le(payload->index_bytes + 4);
    if (version != 1U && version != PSQL_BM25S_STORAGE_CURRENT_VERSION)
    {
        return false;
    }

    flags = psql_bm25s_am_read_u16_le(payload->index_bytes + 6);
    if ((flags & ~PSQL_BM25S_STORAGE_FLAG_KNOWN_MASK) != 0)
    {
        return false;
    }

    memset(header, 0, sizeof(*header));
    header->magic = PSQL_BM25S_AM_GENERATION_MAGIC;
    header->version = PSQL_BM25S_AM_GENERATION_VERSION;
    header->params.method =
        (psql_bm25s_method) psql_bm25s_am_read_u32_le(
            payload->index_bytes + 8
        );
    header->params.idf_method =
        (psql_bm25s_method) psql_bm25s_am_read_u32_le(
            payload->index_bytes + 12
        );
    header->params.k1 = psql_bm25s_am_read_f32_le(payload->index_bytes + 16);
    header->params.b = psql_bm25s_am_read_f32_le(payload->index_bytes + 20);
    header->params.delta =
        psql_bm25s_am_read_f32_le(payload->index_bytes + 24);
    header->num_docs = psql_bm25s_am_read_u32_le(payload->index_bytes + 28);
    header->vocab_size = psql_bm25s_am_read_u32_le(payload->index_bytes + 32);
    header->data_len = psql_bm25s_am_read_u64_le(payload->index_bytes + 36);
    header->empty_token_id =
        psql_bm25s_am_read_u32_le(payload->index_bytes + 44);
    header->has_empty_token =
        (flags & PSQL_BM25S_STORAGE_FLAG_HAS_EMPTY_TOKEN) != 0;

    if (header->data_len > (uint64_t) UINT32_MAX ||
        header->vocab_size == UINT32_MAX ||
        (header->has_empty_token &&
         header->empty_token_id >= header->vocab_size))
    {
        return false;
    }

    *flags_out = flags;
    return psql_bm25s_am_generation_block_layout_serialized(
        payload,
        flags,
        header
    );
}

static void
psql_bm25s_am_generation_descriptor_init(
    psql_bm25s_am_generation_descriptor *descriptor,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    dsm_handle handle,
    Size mapped_size
)
{
    memset(descriptor, 0, sizeof(*descriptor));
    descriptor->magic = PSQL_BM25S_AM_DESCRIPTOR_MAGIC;
    descriptor->version = PSQL_BM25S_AM_DESCRIPTOR_VERSION;
    descriptor->database_oid = MyDatabaseId;
    descriptor->index_oid = RelationGetRelid(indexRelation);
    descriptor->locator = indexRelation->rd_locator;
    descriptor->meta = *meta;
    descriptor->handle = handle;
    descriptor->mapped_size = mapped_size;
}

static bool
psql_bm25s_am_generation_identity_matches(
    const psql_bm25s_am_meta_page *cached,
    const psql_bm25s_am_meta_page *current
)
{
    if (cached == NULL || current == NULL)
    {
        return false;
    }

    /*
     * Shared generation blocks contain the immutable base BM25 payload.
     * Eventual-consistency delta counters and stale flags live outside that
     * payload and can change on every write. Including them here invalidates
     * shared-preload/DSM cache entries during normal ingest and forces query
     * backends to rebuild multi-GB generations.
     */
    return cached->magic == current->magic &&
        cached->version == current->version &&
        cached->page_kind == current->page_kind &&
        cached->cache_epoch == current->cache_epoch &&
        cached->source_type == current->source_type &&
        cached->num_docs == current->num_docs &&
        cached->tid_bytes_len == current->tid_bytes_len &&
        cached->index_bytes_len == current->index_bytes_len;
}

static bool
psql_bm25s_am_generation_descriptor_matches(
    const psql_bm25s_am_generation_descriptor *descriptor,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    return descriptor->magic == PSQL_BM25S_AM_DESCRIPTOR_MAGIC &&
        descriptor->version == PSQL_BM25S_AM_DESCRIPTOR_VERSION &&
        descriptor->database_oid == MyDatabaseId &&
        descriptor->index_oid == RelationGetRelid(indexRelation) &&
        RelFileLocatorEquals(descriptor->locator, indexRelation->rd_locator) &&
        psql_bm25s_am_generation_identity_matches(
            &descriptor->meta,
            meta
        ) &&
        descriptor->mapped_size > sizeof(psql_bm25s_am_generation_block_header);
}

static bool
psql_bm25s_am_generation_descriptor_path(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    char *path,
    size_t path_len
)
{
    RelFileLocator locator = indexRelation->rd_locator;
    int written;

    written = snprintf(
        path,
        path_len,
        "%s/psql_bm25s_generation_cache/%u_%u_%u_%u_%u_%u.desc",
        DataDir,
        MyDatabaseId,
        RelationGetRelid(indexRelation),
        locator.spcOid,
        locator.dbOid,
        locator.relNumber,
        meta->cache_epoch
    );
    return written > 0 && (size_t) written < path_len;
}

static bool
psql_bm25s_am_generation_publish_failure_path(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    char *path,
    size_t path_len
)
{
    RelFileLocator locator = indexRelation->rd_locator;
    int written;

    written = snprintf(
        path,
        path_len,
        "%s/psql_bm25s_generation_cache/%u_%u_%u_%u_%u_%u.fail",
        DataDir,
        MyDatabaseId,
        RelationGetRelid(indexRelation),
        locator.spcOid,
        locator.dbOid,
        locator.relNumber,
        meta->cache_epoch
    );
    return written > 0 && (size_t) written < path_len;
}

static void
psql_bm25s_am_generation_descriptor_ensure_dir(void)
{
    char dir[MAXPGPATH];
    int written;

    written = snprintf(
        dir,
        sizeof(dir),
        "%s/psql_bm25s_generation_cache",
        DataDir
    );
    if (written <= 0 || (size_t) written >= sizeof(dir))
    {
        ereport(ERROR, (errmsg("psql_bm25s generation cache path is too long")));
    }
    if (mkdir(dir, S_IRWXU) != 0 && errno != EEXIST)
    {
        ereport(
            ERROR,
            (
                errcode_for_file_access(),
                errmsg("could not create psql_bm25s generation cache directory")
            )
        );
    }
}

static bool
psql_bm25s_am_generation_dsm_share_eligible(
    const psql_bm25s_am_meta_page *meta
)
{
    return meta->index_bytes_len >= PSQL_BM25S_AM_SHARED_GENERATION_MIN_SIZE;
}

static bool
psql_bm25s_am_generation_share_required(
    const psql_bm25s_am_meta_page *meta
)
{
    /*
     * When the postmaster has a shared-preload arena, share-capable queries
     * must not silently fall back to a private backend-local generation. A
     * connection pool with many backends can otherwise create one large copy
     * per backend. Missing residents should be loaded through the shared path:
     * auto-preload indexes wait for the background preloader, and unmarked
     * indexes use single-flight shared publication on first use.
     */
    return psql_bm25s_am_shared_preload_cache_available() ||
        psql_bm25s_am_generation_dsm_share_eligible(meta);
}

static bool
psql_bm25s_am_generation_local_selected(
    const psql_bm25s_am_meta_page *meta
)
{
    return !psql_bm25s_am_generation_share_required(meta);
}

static int
psql_bm25s_am_generation_try_publish_lock(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    LOCKTAG tag;
    LockAcquireResult result;

    SET_LOCKTAG_ADVISORY(
        tag,
        MyDatabaseId,
        PSQL_BM25S_AM_GENERATION_LOCK_TAG,
        RelationGetRelid(indexRelation),
        meta->cache_epoch
    );
    result = LockAcquire(&tag, ExclusiveLock, false, true);
    if (result == LOCKACQUIRE_NOT_AVAIL)
    {
        return -1;
    }
    return PSQL_BM25S_AM_GENERATION_ADVISORY_LOCK;
}

static void
psql_bm25s_am_generation_publish_unlock(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    int lock_id
)
{
    LOCKTAG tag;

    if (lock_id < 0)
    {
        return;
    }

    SET_LOCKTAG_ADVISORY(
        tag,
        MyDatabaseId,
        PSQL_BM25S_AM_GENERATION_LOCK_TAG,
        RelationGetRelid(indexRelation),
        meta->cache_epoch
    );
    LockRelease(&tag, ExclusiveLock, false);
}

static int
psql_bm25s_am_generation_wait_for_publish_lock(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    LOCKTAG tag;

    SET_LOCKTAG_ADVISORY(
        tag,
        MyDatabaseId,
        PSQL_BM25S_AM_GENERATION_LOCK_TAG,
        RelationGetRelid(indexRelation),
        meta->cache_epoch
    );
    LockAcquire(&tag, ExclusiveLock, false, false);
    return PSQL_BM25S_AM_GENERATION_ADVISORY_LOCK;
}

static bool
psql_bm25s_am_try_maintenance_lock(Oid index_oid)
{
    LOCKTAG tag;
    LockAcquireResult result;

    SET_LOCKTAG_ADVISORY(
        tag,
        MyDatabaseId,
        PSQL_BM25S_AM_MAINTENANCE_LOCK_TAG,
        index_oid,
        0
    );
    result = LockAcquire(&tag, ExclusiveLock, false, true);
    return result != LOCKACQUIRE_NOT_AVAIL;
}

static bool
psql_bm25s_am_try_maintenance_worker_slot(int *slot_out)
{
    int limit;
    int slot;

    limit = psql_bm25s_am_effective_maintenance_worker_limit();
    for (slot = 0; slot < limit; slot++)
    {
        LOCKTAG tag;
        LockAcquireResult result;

        SET_LOCKTAG_ADVISORY(
            tag,
            /* Cluster-wide worker slots: do not scope this limit per database. */
            InvalidOid,
            PSQL_BM25S_AM_MAINTENANCE_WORKER_LOCK_TAG,
            (uint32) slot,
            0
        );
        result = LockAcquire(&tag, ExclusiveLock, false, true);
        if (result != LOCKACQUIRE_NOT_AVAIL)
        {
            if (slot_out != NULL)
            {
                *slot_out = slot;
            }
            return true;
        }
    }

    return false;
}

static void
psql_bm25s_am_release_maintenance_worker_slot(int slot)
{
    LOCKTAG tag;

    if (slot < 0)
    {
        return;
    }

    SET_LOCKTAG_ADVISORY(
        tag,
        InvalidOid,
        PSQL_BM25S_AM_MAINTENANCE_WORKER_LOCK_TAG,
        (uint32) slot,
        0
    );
    LockRelease(&tag, ExclusiveLock, false);
}

static void
psql_bm25s_am_maintenance_unlock(Oid index_oid)
{
    LOCKTAG tag;

    SET_LOCKTAG_ADVISORY(
        tag,
        MyDatabaseId,
        PSQL_BM25S_AM_MAINTENANCE_LOCK_TAG,
        index_oid,
        0
    );
    LockRelease(&tag, ExclusiveLock, false);
}

static void
psql_bm25s_am_generation_publish_failure_unlink(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    char path[MAXPGPATH];

    if (!psql_bm25s_am_generation_publish_failure_path(
            indexRelation,
            meta,
            path,
            sizeof(path)))
    {
        return;
    }
    if (unlink(path) != 0 && errno != ENOENT)
    {
        ereport(
            WARNING,
            (
                errcode_for_file_access(),
                errmsg("could not remove psql_bm25s generation failure marker")
            )
        );
    }
}

static void
psql_bm25s_am_generation_publish_failure_write(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    char path[MAXPGPATH];
    int fd;

    psql_bm25s_am_generation_descriptor_ensure_dir();
    if (!psql_bm25s_am_generation_publish_failure_path(
            indexRelation,
            meta,
            path,
            sizeof(path)))
    {
        return;
    }

    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        return;
    }
    close(fd);
}

static bool
psql_bm25s_am_generation_descriptor_read(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    psql_bm25s_am_generation_descriptor *descriptor
)
{
    char path[MAXPGPATH];
    int fd;
    ssize_t bytes_read;

    if (!psql_bm25s_am_generation_descriptor_path(
            indexRelation,
            meta,
            path,
            sizeof(path)))
    {
        return false;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return false;
    }
    bytes_read = read(fd, descriptor, sizeof(*descriptor));
    close(fd);
    if (bytes_read != (ssize_t) sizeof(*descriptor))
    {
        return false;
    }

    return psql_bm25s_am_generation_descriptor_matches(
        descriptor,
        indexRelation,
        meta
    );
}

static void
psql_bm25s_am_generation_descriptor_unlink(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    char path[MAXPGPATH];

    if (!psql_bm25s_am_generation_descriptor_path(
            indexRelation,
            meta,
            path,
            sizeof(path)))
    {
        return;
    }
    if (unlink(path) != 0 && errno != ENOENT)
    {
        ereport(
            WARNING,
            (
                errcode_for_file_access(),
                errmsg("could not remove stale psql_bm25s generation descriptor")
            )
        );
    }
}

static bool
psql_bm25s_am_generation_descriptor_read_file(
    const char *path,
    psql_bm25s_am_generation_descriptor *descriptor
)
{
    int fd;
    ssize_t bytes_read;

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return false;
    }
    bytes_read = read(fd, descriptor, sizeof(*descriptor));
    close(fd);
    if (bytes_read != (ssize_t) sizeof(*descriptor))
    {
        return false;
    }
    return descriptor->magic == PSQL_BM25S_AM_DESCRIPTOR_MAGIC &&
        descriptor->version == PSQL_BM25S_AM_DESCRIPTOR_VERSION;
}

static void
psql_bm25s_am_generation_descriptor_prune(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    char dir_path[MAXPGPATH];
    DIR *dir;
    struct dirent *de;
    int written;

    written = snprintf(
        dir_path,
        sizeof(dir_path),
        "%s/psql_bm25s_generation_cache",
        DataDir
    );
    if (written <= 0 || (size_t) written >= sizeof(dir_path))
    {
        return;
    }

    dir = opendir(dir_path);
    if (dir == NULL)
    {
        return;
    }

    while ((de = readdir(dir)) != NULL)
    {
        psql_bm25s_am_generation_descriptor descriptor;
        char path[MAXPGPATH];
        size_t name_len = strlen(de->d_name);
        if (name_len < 6 || strcmp(de->d_name + name_len - 5, ".desc") != 0)
        {
            continue;
        }
        written = snprintf(path, sizeof(path), "%s/%s", dir_path, de->d_name);
        if (written <= 0 || (size_t) written >= sizeof(path))
        {
            continue;
        }
        if (!psql_bm25s_am_generation_descriptor_read_file(
                path,
                &descriptor))
        {
            continue;
        }
        if (descriptor.database_oid != MyDatabaseId ||
            descriptor.index_oid != RelationGetRelid(indexRelation))
        {
            continue;
        }
        if (RelFileLocatorEquals(descriptor.locator, indexRelation->rd_locator) &&
            descriptor.meta.cache_epoch == meta->cache_epoch)
        {
            continue;
        }

        PG_TRY();
        {
            dsm_unpin_segment(descriptor.handle);
        }
        PG_CATCH();
        {
            FlushErrorState();
        }
        PG_END_TRY();

        unlink(path);
    }
    closedir(dir);
}

static void
psql_bm25s_am_generation_descriptor_write(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    const psql_bm25s_am_generation_descriptor *descriptor
)
{
    char path[MAXPGPATH];
    char tmp_path[MAXPGPATH];
    int written;
    int fd;
    ssize_t bytes_written;

    psql_bm25s_am_generation_descriptor_ensure_dir();
    psql_bm25s_am_generation_descriptor_prune(indexRelation, meta);
    if (!psql_bm25s_am_generation_descriptor_path(
            indexRelation,
            meta,
            path,
            sizeof(path)))
    {
        ereport(ERROR, (errmsg("psql_bm25s generation cache path is too long")));
    }
    written = snprintf(tmp_path, sizeof(tmp_path), "%s.%d.tmp", path, MyProcPid);
    if (written <= 0 || (size_t) written >= sizeof(tmp_path))
    {
        ereport(ERROR, (errmsg("psql_bm25s generation cache path is too long")));
    }

    fd = open(tmp_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        ereport(
            ERROR,
            (
                errcode_for_file_access(),
                errmsg("could not create psql_bm25s generation descriptor")
            )
        );
    }
    bytes_written = write(fd, descriptor, sizeof(*descriptor));
    if (close(fd) != 0 || bytes_written != (ssize_t) sizeof(*descriptor))
    {
        unlink(tmp_path);
        ereport(
            ERROR,
            (
                errcode_for_file_access(),
                errmsg("could not write psql_bm25s generation descriptor")
            )
        );
    }
    if (rename(tmp_path, path) != 0)
    {
        unlink(tmp_path);
        ereport(
            ERROR,
            (
                errcode_for_file_access(),
                errmsg("could not publish psql_bm25s generation descriptor")
            )
        );
    }
}

static void
psql_bm25s_am_generation_attach_index(
    psql_bm25s_am_cache_generation *generation,
    MemoryContext mcxt
)
{
    psql_bm25s_am_generation_block_header *header;

    (void) mcxt;

    header = (psql_bm25s_am_generation_block_header *) generation->block;
    psql_bm25s_index_init(&generation->index);

    generation->index.params = header->params;
    generation->index.num_docs = header->num_docs;
    generation->index.vocab_size = header->vocab_size;
    generation->index.data_len = header->data_len;
    generation->index.has_empty_token = header->has_empty_token;
    generation->index.empty_token_id = header->empty_token_id;
    generation->index.data = (float *) psql_bm25s_am_generation_ptr(
        generation->block,
        header->data_offset
    );
    generation->index.indices = (uint32_t *) psql_bm25s_am_generation_ptr(
        generation->block,
        header->indices_offset
    );
    generation->index.indptr = (uint64_t *) psql_bm25s_am_generation_ptr(
        generation->block,
        header->indptr_offset
    );
    generation->index.term_frequencies =
        (uint32_t *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->term_frequencies_offset
        );
    generation->index.doc_lengths =
        (uint32_t *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->doc_lengths_offset
        );
    generation->index.doc_frequencies =
        (uint32_t *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->doc_frequencies_offset
        );
    generation->index.nonoccurrence =
        (float *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->nonoccurrence_offset
        );
    generation->doc_tids = (ItemPointerData *) psql_bm25s_am_generation_ptr(
        generation->block,
        header->doc_tids_offset
    );
    generation->sorted_vocab_ids =
        (uint32_t *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->sorted_vocab_ids_offset
        );

    if (header->vocab_offsets_offset == 0)
    {
        generation->index.vocab = NULL;
        generation->vocab_offsets = NULL;
    }
    else
    {
        generation->index.vocab = NULL;
        generation->vocab_offsets = (size_t *) psql_bm25s_am_generation_ptr(
            generation->block,
            header->vocab_offsets_offset
        );
    }

    generation->index_owns_allocations = false;
}

static void
psql_bm25s_am_generation_copy_array(
    void *block,
    size_t offset,
    const void *src,
    size_t bytes
)
{
    if (offset == 0 || src == NULL || bytes == 0)
    {
        return;
    }

    memcpy(psql_bm25s_am_generation_ptr(block, offset), src, bytes);
}

static void
psql_bm25s_am_generation_read_f32_array(
    void *block,
    size_t offset,
    const uint8_t **ptr,
    size_t count
)
{
    float *dst;
    size_t i;

    if (offset == 0 || count == 0)
    {
        return;
    }

    dst = (float *) psql_bm25s_am_generation_ptr(block, offset);
    for (i = 0; i < count; i++)
    {
        if ((i & 0xFFFFF) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        dst[i] = psql_bm25s_am_read_f32_le(*ptr);
        *ptr += sizeof(float);
    }
}

static void
psql_bm25s_am_generation_read_u32_array(
    void *block,
    size_t offset,
    const uint8_t **ptr,
    size_t count
)
{
    uint32_t *dst;
    size_t i;

    if (offset == 0 || count == 0)
    {
        return;
    }

    dst = (uint32_t *) psql_bm25s_am_generation_ptr(block, offset);
    for (i = 0; i < count; i++)
    {
        if ((i & 0xFFFFF) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        dst[i] = psql_bm25s_am_read_u32_le(*ptr);
        *ptr += sizeof(uint32_t);
    }
}

static void
psql_bm25s_am_generation_read_u64_array(
    void *block,
    size_t offset,
    const uint8_t **ptr,
    size_t count
)
{
    uint64_t *dst;
    size_t i;

    if (offset == 0 || count == 0)
    {
        return;
    }

    dst = (uint64_t *) psql_bm25s_am_generation_ptr(block, offset);
    for (i = 0; i < count; i++)
    {
        if ((i & 0xFFFFF) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        dst[i] = psql_bm25s_am_read_u64_le(*ptr);
        *ptr += sizeof(uint64_t);
    }
}

static bool
psql_bm25s_am_generation_validate_block_header(
    const psql_bm25s_am_generation_block_header *header
)
{
    size_t bytes;
    size_t end;

    if (header == NULL ||
        header->magic != PSQL_BM25S_AM_GENERATION_MAGIC ||
        header->version != PSQL_BM25S_AM_GENERATION_VERSION ||
        header->total_size <= sizeof(*header))
    {
        return false;
    }

#define PSQL_BM25S_AM_CHECK_RANGE(offset, count, type)                       \
    do                                                                       \
    {                                                                        \
        if ((count) == 0)                                                     \
        {                                                                    \
            break;                                                           \
        }                                                                    \
        if ((offset) == 0 ||                                                 \
            !psql_bm25s_am_checked_mul_size((count), sizeof(type), &bytes) ||\
            !psql_bm25s_am_checked_add_size((offset), bytes, &end) ||        \
            end > header->total_size)                                        \
        {                                                                    \
            return false;                                                    \
        }                                                                    \
    } while (0)

    PSQL_BM25S_AM_CHECK_RANGE(
        header->data_offset,
        (size_t) header->data_len,
        float
    );
    PSQL_BM25S_AM_CHECK_RANGE(
        header->indices_offset,
        (size_t) header->data_len,
        uint32_t
    );
    PSQL_BM25S_AM_CHECK_RANGE(
        header->indptr_offset,
        (size_t) header->vocab_size + 1,
        uint64_t
    );
    PSQL_BM25S_AM_CHECK_RANGE(
        header->doc_tids_offset,
        (size_t) header->num_docs,
        ItemPointerData
    );
    if (header->vocab_offsets_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->vocab_offsets_offset,
            (size_t) header->vocab_size,
            size_t
        );
    }
    if (header->vocab_bytes_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->vocab_bytes_offset,
            header->vocab_bytes_len,
            char
        );
    }
    if (header->sorted_vocab_ids_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->sorted_vocab_ids_offset,
            (size_t) header->vocab_size,
            uint32_t
        );
    }
    if (header->term_frequencies_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->term_frequencies_offset,
            (size_t) header->data_len,
            uint32_t
        );
    }
    if (header->doc_lengths_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->doc_lengths_offset,
            (size_t) header->num_docs,
            uint32_t
        );
    }
    if (header->doc_frequencies_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->doc_frequencies_offset,
            (size_t) header->vocab_size,
            uint32_t
        );
    }
    if (header->nonoccurrence_offset != 0)
    {
        PSQL_BM25S_AM_CHECK_RANGE(
            header->nonoccurrence_offset,
            (size_t) header->vocab_size,
            float
        );
    }

#undef PSQL_BM25S_AM_CHECK_RANGE

    return true;
}

static bool
psql_bm25s_am_generation_validate_postings(
    const psql_bm25s_am_cache_generation *generation
)
{
    const psql_bm25s_index *index = &generation->index;
    size_t i;
    uint64_t last = 0;

    if (index->indptr == NULL || index->indptr[0] != 0)
    {
        return false;
    }

    for (i = 0; i < (size_t) index->vocab_size + 1; i++)
    {
        uint64_t current = index->indptr[i];

        if (current < last || current > index->data_len)
        {
            return false;
        }
        last = current;
    }
    if (last != index->data_len)
    {
        return false;
    }

    for (i = 0; i < index->data_len; i++)
    {
        if (index->indices[i] >= index->num_docs)
        {
            return false;
        }
    }

    return true;
}

static void
psql_bm25s_am_generation_fill_sorted_vocab_ids(
    void *block,
    const psql_bm25s_am_generation_block_header *header
)
{
    psql_bm25s_am_vocab_item *items;
    uint32_t *sorted_ids;
    size_t *vocab_offsets;
    size_t bytes;
    uint32_t i;

    if (header->sorted_vocab_ids_offset == 0 || header->vocab_size == 0)
    {
        return;
    }
    if (!psql_bm25s_am_checked_mul_size(
            (size_t) header->vocab_size,
            sizeof(*items),
            &bytes))
    {
        psql_bm25s_am_oom();
    }
    items = malloc(bytes);
    if (items == NULL)
    {
        psql_bm25s_am_oom();
    }
    sorted_ids = (uint32_t *) psql_bm25s_am_generation_ptr(
        block,
        header->sorted_vocab_ids_offset
    );
    vocab_offsets = (size_t *) psql_bm25s_am_generation_ptr(
        block,
        header->vocab_offsets_offset
    );

    for (i = 0; i < header->vocab_size; i++)
    {
        if ((i & 0xFFFF) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        items[i].token = psql_bm25s_am_generation_ptr(block, vocab_offsets[i]);
        items[i].token_id = i;
    }
    CHECK_FOR_INTERRUPTS();
    qsort(
        items,
        header->vocab_size,
        sizeof(*items),
        psql_bm25s_am_cmp_vocab_item
    );
    CHECK_FOR_INTERRUPTS();
    for (i = 0; i < header->vocab_size; i++)
    {
        if ((i & 0xFFFF) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        sorted_ids[i] = items[i].token_id;
    }
    free(items);
}

static void
psql_bm25s_am_generation_fill_block_from_payload(
    void *block,
    const psql_bm25s_am_generation_block_header *header,
    uint16_t flags,
    const psql_bm25s_am_payload *payload
)
{
    const uint8_t *ptr;

    ptr = payload->index_bytes + PSQL_BM25S_HEADER_SIZE;
    psql_bm25s_am_generation_read_f32_array(
        block,
        header->data_offset,
        &ptr,
        (size_t) header->data_len
    );
    psql_bm25s_am_generation_read_u32_array(
        block,
        header->indices_offset,
        &ptr,
        (size_t) header->data_len
    );
    psql_bm25s_am_generation_read_u64_array(
        block,
        header->indptr_offset,
        &ptr,
        (size_t) header->vocab_size + 1
    );

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS) != 0)
    {
        psql_bm25s_am_generation_read_u32_array(
            block,
            header->term_frequencies_offset,
            &ptr,
            (size_t) header->data_len
        );
        psql_bm25s_am_generation_read_u32_array(
            block,
            header->doc_lengths_offset,
            &ptr,
            (size_t) header->num_docs
        );
        psql_bm25s_am_generation_read_u32_array(
            block,
            header->doc_frequencies_offset,
            &ptr,
            (size_t) header->vocab_size
        );
    }

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE) != 0)
    {
        psql_bm25s_am_generation_read_f32_array(
            block,
            header->nonoccurrence_offset,
            &ptr,
            (size_t) header->vocab_size
        );
    }

    psql_bm25s_am_generation_copy_array(
        block,
        header->doc_tids_offset,
        payload->doc_tids,
        (size_t) payload->meta.tid_bytes_len
    );

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB) != 0)
    {
        size_t *vocab_offsets;
        char *vocab_bytes;
        size_t vocab_pos = 0;
        uint32_t i;

        vocab_offsets = (size_t *) psql_bm25s_am_generation_ptr(
            block,
            header->vocab_offsets_offset
        );
        vocab_bytes = psql_bm25s_am_generation_ptr(
            block,
            header->vocab_bytes_offset
        );
        for (i = 0; i < header->vocab_size; i++)
        {
            uint32_t token_len = psql_bm25s_am_read_u32_le(ptr);

            ptr += sizeof(uint32_t);
            vocab_offsets[i] = header->vocab_bytes_offset + vocab_pos;
            memcpy(vocab_bytes + vocab_pos, ptr, token_len);
            vocab_bytes[vocab_pos + token_len] = '\0';
            vocab_pos += (size_t) token_len + 1;
            ptr += token_len;
        }
        psql_bm25s_am_generation_fill_sorted_vocab_ids(block, header);
    }
}

static void
psql_bm25s_am_generation_fill_block_from_index(
    void *block,
    const psql_bm25s_am_generation_block_header *header,
    uint16_t flags,
    const psql_bm25s_index *index,
    const ItemPointerData *doc_tids
)
{
    psql_bm25s_am_generation_copy_array(
        block,
        header->data_offset,
        index->data,
        (size_t) header->data_len * sizeof(*index->data)
    );
    psql_bm25s_am_generation_copy_array(
        block,
        header->indices_offset,
        index->indices,
        (size_t) header->data_len * sizeof(*index->indices)
    );
    psql_bm25s_am_generation_copy_array(
        block,
        header->indptr_offset,
        index->indptr,
        ((size_t) header->vocab_size + 1) * sizeof(*index->indptr)
    );
    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_EXACT_STATS) != 0)
    {
        psql_bm25s_am_generation_copy_array(
            block,
            header->term_frequencies_offset,
            index->term_frequencies,
            (size_t) header->data_len * sizeof(*index->term_frequencies)
        );
        psql_bm25s_am_generation_copy_array(
            block,
            header->doc_lengths_offset,
            index->doc_lengths,
            (size_t) header->num_docs * sizeof(*index->doc_lengths)
        );
        psql_bm25s_am_generation_copy_array(
            block,
            header->doc_frequencies_offset,
            index->doc_frequencies,
            (size_t) header->vocab_size * sizeof(*index->doc_frequencies)
        );
    }
    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_NONOCCURRENCE) != 0)
    {
        psql_bm25s_am_generation_copy_array(
            block,
            header->nonoccurrence_offset,
            index->nonoccurrence,
            (size_t) header->vocab_size * sizeof(*index->nonoccurrence)
        );
    }
    psql_bm25s_am_generation_copy_array(
        block,
        header->doc_tids_offset,
        doc_tids,
        (size_t) header->num_docs * sizeof(*doc_tids)
    );

    if ((flags & PSQL_BM25S_STORAGE_FLAG_HAS_VOCAB) != 0)
    {
        size_t *vocab_offsets;
        char *vocab_bytes;
        size_t vocab_pos = 0;
        uint32_t i;

        vocab_offsets = (size_t *) psql_bm25s_am_generation_ptr(
            block,
            header->vocab_offsets_offset
        );
        vocab_bytes = psql_bm25s_am_generation_ptr(
            block,
            header->vocab_bytes_offset
        );
        for (i = 0; i < header->vocab_size; i++)
        {
            size_t token_len = strlen(index->vocab[i]);

            if ((i & 0xFFFF) == 0)
            {
                CHECK_FOR_INTERRUPTS();
            }
            vocab_offsets[i] = header->vocab_bytes_offset + vocab_pos;
            memcpy(vocab_bytes + vocab_pos, index->vocab[i], token_len);
            vocab_bytes[vocab_pos + token_len] = '\0';
            vocab_pos += token_len + 1;
        }
        psql_bm25s_am_generation_fill_sorted_vocab_ids(block, header);
    }
}

static void
psql_bm25s_am_generation_build_block_from_payload(
    psql_bm25s_am_cache_generation *generation,
    MemoryContext mcxt,
    const psql_bm25s_am_payload *payload
)
{
    psql_bm25s_am_generation_block_header header;
    psql_bm25s_am_generation_block_header *block_header;
    uint16_t flags;
    void *block;

    if (!psql_bm25s_am_generation_parse_serialized_header(
            payload,
            &flags,
            &header))
    {
        ereport(ERROR, (errmsg("invalid serialized bm25 index payload")));
    }

    block = malloc(header.total_size);
    if (block == NULL)
    {
        psql_bm25s_am_oom();
    }
    memset(block, 0, header.total_size);
    CHECK_FOR_INTERRUPTS();
    block_header = (psql_bm25s_am_generation_block_header *) block;
    *block_header = header;
    psql_bm25s_am_generation_fill_block_from_payload(
        block,
        &header,
        flags,
        payload
    );

    generation->block = block;
    generation->block_owns_allocation = true;
    psql_bm25s_am_generation_attach_index(generation, mcxt);
    if (!psql_bm25s_am_generation_validate_postings(generation))
    {
        ereport(ERROR, (errmsg("invalid bm25 postings in generation payload")));
    }
}

static bool
psql_bm25s_am_generation_attach_shared_internal(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    bool unlink_on_failure
)
{
    psql_bm25s_am_generation_descriptor descriptor;
    psql_bm25s_am_generation_block_header *header;
    dsm_segment *segment;
    void *block;

    if (!psql_bm25s_am_generation_descriptor_read(
            indexRelation,
            meta,
            &descriptor))
    {
        return false;
    }

    segment = dsm_attach(descriptor.handle);
    if (segment == NULL)
    {
        if (unlink_on_failure)
        {
            psql_bm25s_am_generation_descriptor_unlink(indexRelation, meta);
        }
        return false;
    }
    dsm_pin_mapping(segment);
    block = dsm_segment_address(segment);
    if (block == NULL ||
        dsm_segment_map_length(segment) != descriptor.mapped_size)
    {
        dsm_unpin_mapping(segment);
        dsm_detach(segment);
        if (unlink_on_failure)
        {
            psql_bm25s_am_generation_descriptor_unlink(indexRelation, meta);
        }
        return false;
    }

    header = (psql_bm25s_am_generation_block_header *) block;
    if (header->total_size != descriptor.mapped_size ||
        !psql_bm25s_am_generation_validate_block_header(header))
    {
        dsm_unpin_mapping(segment);
        dsm_detach(segment);
        if (unlink_on_failure)
        {
            psql_bm25s_am_generation_descriptor_unlink(indexRelation, meta);
        }
        return false;
    }

    entry->mcxt = AllocSetContextCreate(
        psql_bm25s_am_cache.mcxt,
        "psql_bm25s cache entry",
        ALLOCSET_START_SMALL_SIZES
    );
    entry->generation.block = block;
    entry->generation.segment = segment;
    entry->generation.mapping_pinned = true;
    entry->generation.locator = indexRelation->rd_locator;
    entry->generation.meta = *meta;
    entry->generation.delta_overlay_materialized = false;
    psql_bm25s_am_generation_attach_index(&entry->generation, entry->mcxt);
    psql_bm25s_am_cache_build_sorted_vocab(entry);
    return true;
}

static bool
psql_bm25s_am_generation_attach_shared(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    return psql_bm25s_am_generation_attach_shared_internal(
        entry,
        indexRelation,
        meta,
        true
    );
}

static bool
psql_bm25s_am_generation_build_shared_from_payload(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_payload *payload
)
{
    psql_bm25s_am_generation_block_header header;
    psql_bm25s_am_generation_block_header *block_header;
    psql_bm25s_am_generation_descriptor descriptor;
    uint16_t flags;
    dsm_segment *segment;
    void *block;

    if (!psql_bm25s_am_generation_parse_serialized_header(
            payload,
            &flags,
            &header))
    {
        ereport(ERROR, (errmsg("invalid serialized bm25 index payload")));
    }

    if (header.total_size < PSQL_BM25S_AM_SHARED_GENERATION_MIN_SIZE)
    {
        return false;
    }
    CHECK_FOR_INTERRUPTS();
    segment = dsm_create((Size) header.total_size, DSM_CREATE_NULL_IF_MAXSEGMENTS);
    if (segment == NULL)
    {
        return false;
    }
    dsm_pin_segment(segment);
    dsm_pin_mapping(segment);
    block = dsm_segment_address(segment);
    memset(block, 0, header.total_size);
    CHECK_FOR_INTERRUPTS();
    block_header = (psql_bm25s_am_generation_block_header *) block;
    *block_header = header;
    psql_bm25s_am_generation_fill_block_from_payload(
        block,
        &header,
        flags,
        payload
    );

    entry->mcxt = AllocSetContextCreate(
        psql_bm25s_am_cache.mcxt,
        "psql_bm25s cache entry",
        ALLOCSET_START_SMALL_SIZES
    );
    entry->generation.block = block;
    entry->generation.segment = segment;
    entry->generation.mapping_pinned = true;
    entry->generation.locator = indexRelation->rd_locator;
    entry->generation.meta = payload->meta;
    entry->generation.delta_overlay_materialized = false;
    psql_bm25s_am_generation_attach_index(&entry->generation, entry->mcxt);
    if (!psql_bm25s_am_generation_validate_postings(&entry->generation))
    {
        psql_bm25s_am_cache_entry_reset(entry);
        return false;
    }
    psql_bm25s_am_cache_build_sorted_vocab(entry);

    psql_bm25s_am_generation_descriptor_init(
        &descriptor,
        indexRelation,
        &payload->meta,
        dsm_segment_handle(segment),
        (Size) header.total_size
    );
    psql_bm25s_am_generation_descriptor_write(
        indexRelation,
        &payload->meta,
        &descriptor
    );
    return true;
}

static bool
psql_bm25s_am_shared_preload_entry_has_block(
    const psql_bm25s_am_shared_preload_entry *entry
)
{
    if (entry == NULL ||
        !psql_bm25s_am_shared_preload_cache_available() ||
        entry->mapped_size <= sizeof(psql_bm25s_am_generation_block_header) ||
        entry->offset > psql_bm25s_shared_preload->arena_size ||
        entry->mapped_size >
            psql_bm25s_shared_preload->arena_size - entry->offset)
    {
        return false;
    }

    return true;
}

static void
psql_bm25s_am_shared_preload_try_rewind_locked(void)
{
    for (;;)
    {
        Size new_used = 0;
        int tail_slot = -1;
        uint32 i;

        for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
        {
            psql_bm25s_am_shared_preload_entry *entry;
            Size end;

            entry = &psql_bm25s_shared_preload->entries[i];
            if (entry->mapped_size == 0)
            {
                continue;
            }
            if (!psql_bm25s_am_shared_preload_entry_has_block(entry))
            {
                if (!entry->in_use)
                {
                    memset(entry, 0, sizeof(*entry));
                }
                continue;
            }
            end = MAXALIGN(entry->offset + entry->mapped_size);
            if (end > new_used)
            {
                new_used = end;
                tail_slot = (int) i;
            }
        }

        if (tail_slot >= 0 &&
            !psql_bm25s_shared_preload->entries[tail_slot].in_use)
        {
            memset(
                &psql_bm25s_shared_preload->entries[tail_slot],
                0,
                sizeof(psql_bm25s_shared_preload->entries[tail_slot])
            );
            continue;
        }

        psql_bm25s_shared_preload->used = new_used;
        break;
    }
}

static void
psql_bm25s_am_shared_preload_retire_entry_locked(
    psql_bm25s_am_shared_preload_entry *entry
)
{
    if (entry == NULL || !entry->in_use)
    {
        return;
    }

    entry->ready = false;
    if (entry->refcount == 0)
    {
        entry->in_use = false;
        entry->obsolete = false;
    }
    else
    {
        entry->obsolete = true;
    }
}

static bool
psql_bm25s_am_shared_preload_entry_same_generation(
    const psql_bm25s_am_shared_preload_entry *shared_entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    if (shared_entry == NULL ||
        !shared_entry->in_use ||
        shared_entry->obsolete)
    {
        return false;
    }

    return shared_entry->database_oid == MyDatabaseId &&
        shared_entry->index_oid == RelationGetRelid(indexRelation) &&
        RelFileLocatorEquals(shared_entry->locator, indexRelation->rd_locator) &&
        psql_bm25s_am_generation_identity_matches(
            &shared_entry->meta,
            meta
        ) &&
        shared_entry->mapped_size >
            sizeof(psql_bm25s_am_generation_block_header);
}

static bool
psql_bm25s_am_shared_preload_entry_matches(
    const psql_bm25s_am_shared_preload_entry *shared_entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    if (shared_entry == NULL || !shared_entry->ready)
    {
        return false;
    }

    return psql_bm25s_am_shared_preload_entry_same_generation(
        shared_entry,
        indexRelation,
        meta
    );
}

static void
psql_bm25s_am_shared_preload_retire_obsolete_locked(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    uint32 i;
    Oid index_oid = RelationGetRelid(indexRelation);

    for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
    {
        psql_bm25s_am_shared_preload_entry *entry;

        entry = &psql_bm25s_shared_preload->entries[i];
        if (!entry->in_use ||
            entry->database_oid != MyDatabaseId ||
            entry->index_oid != index_oid)
        {
            continue;
        }
        if (!psql_bm25s_am_shared_preload_entry_same_generation(
                entry,
                indexRelation,
                meta))
        {
            psql_bm25s_am_shared_preload_retire_entry_locked(entry);
        }
    }
    psql_bm25s_am_shared_preload_try_rewind_locked();
}

static void
psql_bm25s_am_shared_preload_retire_obsolete(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    if (!psql_bm25s_am_shared_preload_available())
    {
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_am_shared_preload_retire_obsolete_locked(
        indexRelation,
        meta
    );
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
}

static void
psql_bm25s_am_shared_preload_release_ref(int slot)
{
    psql_bm25s_am_shared_preload_entry *entry;

    if (!psql_bm25s_am_shared_preload_available() ||
        slot < 0 ||
        (uint32) slot >= psql_bm25s_am_shared_preload_entry_capacity())
    {
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    entry = &psql_bm25s_shared_preload->entries[slot];
    if (entry->refcount > 0)
    {
        entry->refcount--;
    }
    if (entry->refcount == 0 && entry->obsolete)
    {
        entry->in_use = false;
        entry->ready = false;
        entry->obsolete = false;
    }
    psql_bm25s_am_shared_preload_try_rewind_locked();
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
}

static bool
psql_bm25s_am_generation_attach_preload(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    psql_bm25s_am_generation_block_header *header;
    Size offset = 0;
    Size mapped_size = 0;
    char *arena_base;
    void *block;
    uint32 i;
    int slot = -1;

    if (!psql_bm25s_am_shared_preload_available())
    {
        return false;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
    {
        psql_bm25s_am_shared_preload_entry *shared_entry;

        shared_entry = &psql_bm25s_shared_preload->entries[i];
        if (psql_bm25s_am_shared_preload_entry_matches(
                shared_entry,
                indexRelation,
                meta))
        {
            if (shared_entry->refcount == UINT32_MAX)
            {
                continue;
            }
            offset = shared_entry->offset;
            mapped_size = shared_entry->mapped_size;
            shared_entry->refcount++;
            slot = (int) i;
            break;
        }
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);

    if (mapped_size == 0)
    {
        return false;
    }

    arena_base = psql_bm25s_am_shared_preload_arena_base();
    if (arena_base == NULL ||
        offset > psql_bm25s_shared_preload->arena_size ||
        mapped_size > psql_bm25s_shared_preload->arena_size - offset)
    {
        psql_bm25s_am_shared_preload_release_ref(slot);
        return false;
    }

    block = arena_base + offset;
    header = (psql_bm25s_am_generation_block_header *) block;
    if (header->total_size != mapped_size ||
        !psql_bm25s_am_generation_validate_block_header(header))
    {
        psql_bm25s_am_shared_preload_release_ref(slot);
        return false;
    }

    entry->mcxt = AllocSetContextCreate(
        psql_bm25s_am_cache.mcxt,
        "psql_bm25s cache entry",
        ALLOCSET_START_SMALL_SIZES
    );
    entry->generation.block = block;
    entry->generation.locator = indexRelation->rd_locator;
    entry->generation.meta = *meta;
    entry->generation.delta_overlay_materialized = false;
    entry->generation.shared_preload_attached = true;
    entry->generation.shared_preload_slot = slot;
    psql_bm25s_am_generation_attach_index(&entry->generation, entry->mcxt);
    psql_bm25s_am_cache_build_sorted_vocab(entry);
    return true;
}

static bool
psql_bm25s_am_shared_preload_index_state(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    bool *resident_out,
    bool *loading_out
)
{
    uint32 i;
    bool resident = false;
    bool loading = false;

    if (resident_out != NULL)
    {
        *resident_out = false;
    }
    if (loading_out != NULL)
    {
        *loading_out = false;
    }
    if (!psql_bm25s_am_shared_preload_available())
    {
        return false;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
    {
        psql_bm25s_am_shared_preload_entry *shared_entry;

        shared_entry = &psql_bm25s_shared_preload->entries[i];
        if (!psql_bm25s_am_shared_preload_entry_same_generation(
                shared_entry,
                indexRelation,
                meta))
        {
            continue;
        }
        if (shared_entry->ready)
        {
            resident = true;
        }
        else
        {
            loading = true;
        }
        break;
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);

    if (resident_out != NULL)
    {
        *resident_out = resident;
    }
    if (loading_out != NULL)
    {
        *loading_out = loading;
    }
    return resident || loading;
}

static bool
psql_bm25s_am_shared_preload_should_load(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    uint32 i;
    bool free_slot = false;
    bool reusable_slot = false;
    Size used;
    Size arena_size;
    Size end;

    if (!psql_bm25s_am_shared_preload_cache_available() ||
        meta->index_bytes_len == 0)
    {
        return false;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_am_shared_preload_retire_obsolete_locked(
        indexRelation,
        meta
    );
    used = MAXALIGN(psql_bm25s_shared_preload->used);
    arena_size = psql_bm25s_shared_preload->arena_size;
    for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
    {
        psql_bm25s_am_shared_preload_entry *shared_entry;

        shared_entry = &psql_bm25s_shared_preload->entries[i];
        if (psql_bm25s_am_shared_preload_entry_same_generation(
                shared_entry,
                indexRelation,
                meta))
        {
            SpinLockRelease(&psql_bm25s_shared_preload->mutex);
            return false;
        }
        if (!shared_entry->in_use)
        {
            free_slot = true;
            if (psql_bm25s_am_shared_preload_entry_has_block(shared_entry) &&
                shared_entry->mapped_size >= (Size) meta->index_bytes_len)
            {
                reusable_slot = true;
            }
        }
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);

    if (!free_slot)
    {
        return false;
    }
    if (reusable_slot)
    {
        return true;
    }
    /*
     * This is a cheap admission check, not the final allocator. It avoids
     * obviously oversized candidates without reading the full payload; the
     * reserve path still validates the exact resident block size from the
     * payload header before publishing.
     */
    if (!psql_bm25s_am_checked_add_size(
            used,
            (Size) meta->index_bytes_len,
            &end))
    {
        return false;
    }
    return MAXALIGN(end) <= arena_size;
}

static bool
psql_bm25s_am_shared_preload_reserve_entry(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    Size mapped_size,
    Size *offset_out,
    int *slot_out,
    bool *reserved_new_out
)
{
    Size offset;
    Size end;
    int free_slot = -1;
    int reusable_slot = -1;
    uint32 i;

    if (!psql_bm25s_am_shared_preload_available())
    {
        return false;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_am_shared_preload_retire_obsolete_locked(
        indexRelation,
        meta
    );
    for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
    {
        psql_bm25s_am_shared_preload_entry *shared_entry;

        shared_entry = &psql_bm25s_shared_preload->entries[i];
        if (psql_bm25s_am_shared_preload_entry_matches(
                shared_entry,
                indexRelation,
                meta))
        {
            *offset_out = shared_entry->offset;
            *slot_out = (int) i;
            *reserved_new_out = false;
            SpinLockRelease(&psql_bm25s_shared_preload->mutex);
            return true;
        }
        if (!shared_entry->in_use && free_slot < 0)
        {
            free_slot = (int) i;
        }
        if (!shared_entry->in_use &&
            psql_bm25s_am_shared_preload_entry_has_block(shared_entry) &&
            shared_entry->mapped_size >= mapped_size &&
            (reusable_slot < 0 ||
             shared_entry->mapped_size <
                psql_bm25s_shared_preload->entries[reusable_slot].mapped_size))
        {
            reusable_slot = (int) i;
        }
    }

    if (reusable_slot >= 0)
    {
        free_slot = reusable_slot;
        offset = psql_bm25s_shared_preload->entries[free_slot].offset;
    }
    else
    {
        offset = MAXALIGN(psql_bm25s_shared_preload->used);
        if (free_slot < 0 ||
            !psql_bm25s_am_checked_add_size(offset, mapped_size, &end) ||
            end > psql_bm25s_shared_preload->arena_size)
        {
            SpinLockRelease(&psql_bm25s_shared_preload->mutex);
            return false;
        }
        psql_bm25s_shared_preload->used = MAXALIGN(end);
    }
    if (free_slot < 0)
    {
        SpinLockRelease(&psql_bm25s_shared_preload->mutex);
        return false;
    }

    memset(
        &psql_bm25s_shared_preload->entries[free_slot],
        0,
        sizeof(psql_bm25s_shared_preload->entries[free_slot])
    );
    psql_bm25s_shared_preload->entries[free_slot].in_use = true;
    psql_bm25s_shared_preload->entries[free_slot].ready = false;
    psql_bm25s_shared_preload->entries[free_slot].database_oid = MyDatabaseId;
    psql_bm25s_shared_preload->entries[free_slot].index_oid =
        RelationGetRelid(indexRelation);
    psql_bm25s_shared_preload->entries[free_slot].locator =
        indexRelation->rd_locator;
    psql_bm25s_shared_preload->entries[free_slot].meta = *meta;
    psql_bm25s_shared_preload->entries[free_slot].offset = offset;
    psql_bm25s_shared_preload->entries[free_slot].mapped_size = mapped_size;
    psql_bm25s_shared_preload->entries[free_slot].refcount = 1;
    *offset_out = offset;
    *slot_out = free_slot;
    *reserved_new_out = true;
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    return true;
}

static void
psql_bm25s_am_shared_preload_entry_publish(int slot, bool ready)
{
    if (!psql_bm25s_am_shared_preload_available() ||
        slot < 0 ||
        (uint32) slot >= psql_bm25s_am_shared_preload_entry_capacity())
    {
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    if (ready)
    {
        psql_bm25s_shared_preload->entries[slot].ready = true;
        psql_bm25s_shared_preload->entries[slot].obsolete = false;
    }
    else
    {
        psql_bm25s_am_shared_preload_retire_entry_locked(
            &psql_bm25s_shared_preload->entries[slot]
        );
        psql_bm25s_am_shared_preload_try_rewind_locked();
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
}

static bool
psql_bm25s_am_generation_build_preload_from_payload(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_payload *payload
)
{
    psql_bm25s_am_generation_block_header header;
    psql_bm25s_am_generation_block_header *block_header;
    uint16_t flags;
    Size offset = 0;
    int slot = -1;
    bool reserved_new = false;
    void *block;
    char *arena_base;

    if (!psql_bm25s_am_shared_preload_cache_available() ||
        payload == NULL ||
        payload->meta.index_bytes_len == 0)
    {
        return false;
    }
    if (!psql_bm25s_am_generation_parse_serialized_header(
            payload,
            &flags,
            &header))
    {
        ereport(ERROR, (errmsg("invalid serialized bm25 index payload")));
    }

    if (!psql_bm25s_am_shared_preload_reserve_entry(
            indexRelation,
            &payload->meta,
            (Size) header.total_size,
            &offset,
            &slot,
            &reserved_new))
    {
        return false;
    }

    if (psql_bm25s_am_generation_attach_preload(
            entry,
            indexRelation,
            &payload->meta))
    {
        return true;
    }
    if (!reserved_new)
    {
        return false;
    }

    arena_base = psql_bm25s_am_shared_preload_arena_base();
    if (arena_base == NULL)
    {
        psql_bm25s_am_shared_preload_entry_publish(slot, false);
        psql_bm25s_am_shared_preload_release_ref(slot);
        return false;
    }
    block = arena_base + offset;
    memset(block, 0, header.total_size);
    block_header = (psql_bm25s_am_generation_block_header *) block;
    *block_header = header;
    psql_bm25s_am_generation_fill_block_from_payload(
        block,
        &header,
        flags,
        payload
    );

    entry->mcxt = AllocSetContextCreate(
        psql_bm25s_am_cache.mcxt,
        "psql_bm25s cache entry",
        ALLOCSET_START_SMALL_SIZES
    );
    entry->generation.block = block;
    entry->generation.locator = indexRelation->rd_locator;
    entry->generation.meta = payload->meta;
    entry->generation.delta_overlay_materialized = false;
    entry->generation.shared_preload_attached = true;
    entry->generation.shared_preload_slot = slot;
    psql_bm25s_am_generation_attach_index(&entry->generation, entry->mcxt);
    if (!psql_bm25s_am_generation_validate_postings(&entry->generation))
    {
        psql_bm25s_am_shared_preload_entry_publish(slot, false);
        psql_bm25s_am_cache_entry_reset(entry);
        return false;
    }
    psql_bm25s_am_cache_build_sorted_vocab(entry);
    psql_bm25s_am_shared_preload_entry_publish(slot, true);
    return true;
}

static bool
psql_bm25s_am_generation_build_preload_from_index(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    const psql_bm25s_index *index,
    const ItemPointerData *doc_tids,
    size_t num_docs
)
{
    psql_bm25s_am_generation_block_header header;
    psql_bm25s_am_generation_block_header *block_header;
    psql_bm25s_am_cache_generation generation;
    uint16_t flags;
    Size offset = 0;
    int slot = -1;
    bool reserved_new = false;
    void *block;
    char *arena_base;

    if (!psql_bm25s_am_shared_preload_cache_available() ||
        meta == NULL ||
        index == NULL ||
        meta->index_bytes_len == 0 ||
        num_docs != (size_t) index->num_docs)
    {
        return false;
    }

    flags = psql_bm25s_am_generation_flags_from_index(index);
    if (!psql_bm25s_am_generation_block_layout_index(
            index,
            doc_tids,
            num_docs,
            flags,
            &header))
    {
        return false;
    }

    if (!psql_bm25s_am_shared_preload_reserve_entry(
            indexRelation,
            meta,
            (Size) header.total_size,
            &offset,
            &slot,
            &reserved_new))
    {
        return false;
    }
    if (!reserved_new)
    {
        return true;
    }

    arena_base = psql_bm25s_am_shared_preload_arena_base();
    if (arena_base == NULL)
    {
        psql_bm25s_am_shared_preload_entry_publish(slot, false);
        psql_bm25s_am_shared_preload_release_ref(slot);
        return false;
    }

    block = arena_base + offset;
    memset(block, 0, header.total_size);
    block_header = (psql_bm25s_am_generation_block_header *) block;
    *block_header = header;
    psql_bm25s_am_generation_fill_block_from_index(
        block,
        &header,
        flags,
        index,
        doc_tids
    );

    memset(&generation, 0, sizeof(generation));
    generation.block = block;
    generation.locator = indexRelation->rd_locator;
    generation.meta = *meta;
    generation.shared_preload_attached = true;
    generation.shared_preload_slot = slot;
    psql_bm25s_am_generation_attach_index(&generation, CurrentMemoryContext);
    if (!psql_bm25s_am_generation_validate_postings(&generation))
    {
        psql_bm25s_am_shared_preload_entry_publish(slot, false);
        psql_bm25s_am_shared_preload_release_ref(slot);
        return false;
    }

    psql_bm25s_am_shared_preload_entry_publish(slot, true);
    psql_bm25s_am_shared_preload_release_ref(slot);
    return true;
}

static bool
psql_bm25s_am_generation_block_is_preload(const void *block)
{
    char *arena_base;
    const char *ptr = (const char *) block;

    arena_base = psql_bm25s_am_shared_preload_arena_base();
    if (arena_base == NULL || block == NULL)
    {
        return false;
    }
    return ptr >= arena_base &&
        ptr < arena_base + psql_bm25s_shared_preload->arena_size;
}

static const char *
psql_bm25s_am_cache_entry_tier(const psql_bm25s_am_cache_entry *entry)
{
    if (entry == NULL || entry->mcxt == NULL || entry->generation.block == NULL)
    {
        return "none";
    }
    if (psql_bm25s_am_generation_block_is_preload(entry->generation.block))
    {
        return "shared_preload";
    }
    if (entry->generation.segment != NULL)
    {
        return "dsm";
    }
    return "backend_local";
}

static void
psql_bm25s_am_cache_ensure_capacity(void)
{
    MemoryContext oldcontext;
    size_t new_capacity;
    size_t bytes;

    psql_bm25s_am_cache_init();
    if (psql_bm25s_am_cache.len < psql_bm25s_am_cache.capacity)
    {
        return;
    }

    new_capacity = psql_bm25s_am_cache.capacity == 0 ?
        8 : psql_bm25s_am_cache.capacity * 2;
    bytes = sizeof(*psql_bm25s_am_cache.entries) * new_capacity;
    oldcontext = MemoryContextSwitchTo(psql_bm25s_am_cache.mcxt);
    if (psql_bm25s_am_cache.entries == NULL)
    {
        psql_bm25s_am_cache.entries = palloc0(bytes);
    }
    else
    {
        size_t old_bytes;

        old_bytes = sizeof(*psql_bm25s_am_cache.entries) *
            psql_bm25s_am_cache.capacity;
        psql_bm25s_am_cache.entries = repalloc(
            psql_bm25s_am_cache.entries,
            bytes
        );
        memset(
            ((char *) psql_bm25s_am_cache.entries) + old_bytes,
            0,
            bytes - old_bytes
        );
    }
    MemoryContextSwitchTo(oldcontext);
    psql_bm25s_am_cache.capacity = new_capacity;
}

static psql_bm25s_am_cache_entry *
psql_bm25s_am_cache_get_unleased_entry(Oid index_oid)
{
    size_t i;

    psql_bm25s_am_cache_init();
    for (i = 0; i < psql_bm25s_am_cache.len; i++)
    {
        psql_bm25s_am_cache_entry *entry;

        entry = &psql_bm25s_am_cache.entries[i];
        if (entry->index_oid == index_oid && entry->lease_count == 0)
        {
            psql_bm25s_am_cache_entry_reset(entry);
            entry->index_oid = index_oid;
            return entry;
        }
    }

    psql_bm25s_am_cache_ensure_capacity();
    psql_bm25s_am_cache.entries[psql_bm25s_am_cache.len].index_oid = index_oid;
    psql_bm25s_am_cache.len++;
    return &psql_bm25s_am_cache.entries[psql_bm25s_am_cache.len - 1];
}

static psql_bm25s_am_cache_entry *
psql_bm25s_am_cache_get_entry(
    Oid index_oid,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    size_t i;

    psql_bm25s_am_cache_init();
    for (i = 0; i < psql_bm25s_am_cache.len; i++)
    {
        psql_bm25s_am_cache_entry *entry;

        entry = &psql_bm25s_am_cache.entries[i];
        if (entry->index_oid == index_oid &&
            psql_bm25s_am_cache_entry_matches(entry, indexRelation, meta))
        {
            return entry;
        }
    }

    return psql_bm25s_am_cache_get_unleased_entry(index_oid);
}

static psql_bm25s_am_cache_entry *
psql_bm25s_am_cache_find_resident_entry(Oid index_oid, Oid source_type)
{
    size_t i;

    psql_bm25s_am_cache_init();
    for (i = 0; i < psql_bm25s_am_cache.len; i++)
    {
        psql_bm25s_am_cache_entry *entry;

        entry = &psql_bm25s_am_cache.entries[i];
        if (entry->index_oid == index_oid &&
            entry->mcxt != NULL &&
            entry->generation.block != NULL &&
            entry->generation.meta.source_type == source_type)
        {
            return entry;
        }
    }
    return NULL;
}

static bool
psql_bm25s_am_cache_entry_matches(
    const psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    if (entry->mcxt == NULL ||
        !RelFileLocatorEquals(
            entry->generation.locator,
            indexRelation->rd_locator
        ))
    {
        return false;
    }

    if (!psql_bm25s_am_generation_identity_matches(
            &entry->generation.meta,
            meta))
    {
        return false;
    }

    if (!entry->generation.delta_overlay_materialized)
    {
        return true;
    }

    return entry->generation.meta.flags == meta->flags &&
        entry->generation.meta.delta_record_count == meta->delta_record_count &&
        entry->generation.meta.delta_bytes_len == meta->delta_bytes_len &&
        entry->generation.meta.pending_write_tuples ==
            meta->pending_write_tuples &&
        entry->generation.meta.pending_delete_tuples ==
            meta->pending_delete_tuples;
}

static void
psql_bm25s_am_cache_build_sorted_vocab(psql_bm25s_am_cache_entry *entry)
{
    size_t i;

    if (entry->generation.sorted_vocab_ids != NULL)
    {
        return;
    }
    if (entry->generation.index.vocab == NULL || entry->generation.index.vocab_size == 0)
    {
        return;
    }

    entry->generation.sorted_vocab = MemoryContextAlloc(
        entry->mcxt,
        sizeof(*entry->generation.sorted_vocab) * entry->generation.index.vocab_size
    );
    for (i = 0; i < entry->generation.index.vocab_size; i++)
    {
        entry->generation.sorted_vocab[i].token = entry->generation.index.vocab[i];
        entry->generation.sorted_vocab[i].token_id = (uint32_t) i;
    }
    qsort(
        entry->generation.sorted_vocab,
        entry->generation.index.vocab_size,
        sizeof(*entry->generation.sorted_vocab),
        psql_bm25s_am_cmp_vocab_item
    );
}

static void
psql_bm25s_builder_tokens_free(psql_bm25s_doc_tokens_builder *builder)
{
    size_t i;

    if (builder == NULL)
    {
        return;
    }

    for (i = 0; i < builder->len; i++)
    {
        size_t j;

        for (j = 0; j < builder->docs[i].len; j++)
        {
            if (builder->docs[i].tokens[j] != NULL)
            {
                pfree((void *) builder->docs[i].tokens[j]);
            }
        }
        if (builder->docs[i].tokens != NULL)
        {
            pfree((void *) builder->docs[i].tokens);
        }
    }
    if (builder->docs != NULL)
    {
        pfree(builder->docs);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_builder_ids_free(psql_bm25s_doc_ids_builder *builder)
{
    size_t i;

    if (builder == NULL)
    {
        return;
    }

    for (i = 0; i < builder->len; i++)
    {
        if (builder->docs[i].token_ids != NULL)
        {
            pfree(builder->docs[i].token_ids);
        }
    }
    if (builder->docs != NULL)
    {
        pfree(builder->docs);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_tid_builder_free(psql_bm25s_tid_builder *builder)
{
    if (builder == NULL)
    {
        return;
    }

    if (builder->tids != NULL)
    {
        pfree(builder->tids);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_docid_builder_free(psql_bm25s_docid_builder *builder)
{
    if (builder == NULL)
    {
        return;
    }

    if (builder->doc_ids != NULL)
    {
        pfree(builder->doc_ids);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_am_term_entry_builder_free(
    psql_bm25s_am_term_entry_builder *builder
)
{
    if (builder == NULL)
    {
        return;
    }
    if (builder->entries != NULL)
    {
        pfree(builder->entries);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_am_doc_length_builder_free(
    psql_bm25s_am_doc_length_builder *builder
)
{
    if (builder == NULL)
    {
        return;
    }
    if (builder->lengths != NULL)
    {
        pfree(builder->lengths);
    }
    memset(builder, 0, sizeof(*builder));
}

static void
psql_bm25s_am_vocab_map_free(psql_bm25s_am_vocab_map *map)
{
    size_t i;

    if (map == NULL)
    {
        return;
    }
    if (map->slots != NULL)
    {
        for (i = 0; i < map->capacity; i++)
        {
            if (map->slots[i].key != NULL)
            {
                pfree(map->slots[i].key);
            }
        }
        pfree(map->slots);
    }
    memset(map, 0, sizeof(*map));
}

static void
psql_bm25s_ensure_doc_tokens_capacity(psql_bm25s_doc_tokens_builder *builder)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;

        if (builder->docs == NULL)
        {
            builder->docs = palloc(sizeof(*builder->docs) * new_capacity);
        }
        else
        {
            builder->docs = repalloc(
                builder->docs,
                sizeof(*builder->docs) * new_capacity
            );
        }
        builder->capacity = new_capacity;
    }
}

static void
psql_bm25s_ensure_doc_ids_capacity(psql_bm25s_doc_ids_builder *builder)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;

        if (builder->docs == NULL)
        {
            builder->docs = palloc(sizeof(*builder->docs) * new_capacity);
        }
        else
        {
            builder->docs = repalloc(
                builder->docs,
                sizeof(*builder->docs) * new_capacity
            );
        }
        builder->capacity = new_capacity;
    }
}

static void
psql_bm25s_ensure_tid_capacity(psql_bm25s_tid_builder *builder)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;

        if (builder->tids == NULL)
        {
            builder->tids = palloc(sizeof(*builder->tids) * new_capacity);
        }
        else
        {
            builder->tids = repalloc(
                builder->tids,
                sizeof(*builder->tids) * new_capacity
            );
        }
        builder->capacity = new_capacity;
    }
}

static void
psql_bm25s_ensure_docid_capacity(psql_bm25s_docid_builder *builder)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;

        if (builder->doc_ids == NULL)
        {
            builder->doc_ids = palloc(sizeof(*builder->doc_ids) * new_capacity);
        }
        else
        {
            builder->doc_ids = repalloc(
                builder->doc_ids,
                sizeof(*builder->doc_ids) * new_capacity
            );
        }
        builder->capacity = new_capacity;
    }
}

static void
psql_bm25s_am_ensure_term_entry_capacity(
    psql_bm25s_am_term_entry_builder *builder
)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity =
            builder->capacity == 0 ? 1024 : builder->capacity * 2;
        size_t bytes;

        if (new_capacity < builder->capacity ||
            !psql_bm25s_am_checked_mul_size(
                new_capacity,
                sizeof(*builder->entries),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s compact build is too large")));
        }
        if (builder->entries == NULL)
        {
            builder->entries = palloc(bytes);
        }
        else
        {
            builder->entries = repalloc(builder->entries, bytes);
        }
        builder->capacity = new_capacity;
    }
}

static void
psql_bm25s_am_ensure_doc_length_capacity(
    psql_bm25s_am_doc_length_builder *builder
)
{
    if (builder->len == builder->capacity)
    {
        size_t new_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;
        size_t bytes;

        if (new_capacity < builder->capacity ||
            !psql_bm25s_am_checked_mul_size(
                new_capacity,
                sizeof(*builder->lengths),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s compact build is too large")));
        }
        if (builder->lengths == NULL)
        {
            builder->lengths = palloc(bytes);
        }
        else
        {
            builder->lengths = repalloc(builder->lengths, bytes);
        }
        builder->capacity = new_capacity;
    }
}

static uint64
psql_bm25s_am_hash_bytes(const char *s)
{
    uint64 hash = UINT64CONST(1469598103934665603);
    const unsigned char *ptr = (const unsigned char *) s;

    while (*ptr != '\0')
    {
        hash ^= (uint64) *ptr;
        hash *= UINT64CONST(1099511628211);
        ptr++;
    }
    return hash;
}

static void
psql_bm25s_am_vocab_map_init(psql_bm25s_am_vocab_map *map)
{
    memset(map, 0, sizeof(*map));
    map->capacity = 8;
    map->slots = palloc0(sizeof(*map->slots) * map->capacity);
}

static void
psql_bm25s_am_vocab_map_rehash(
    psql_bm25s_am_vocab_map *map,
    size_t new_capacity
)
{
    psql_bm25s_am_vocab_map_slot *old_slots = map->slots;
    size_t old_capacity = map->capacity;
    size_t i;

    map->slots = palloc0(sizeof(*map->slots) * new_capacity);
    map->capacity = new_capacity;
    map->size = 0;
    for (i = 0; i < old_capacity; i++)
    {
        psql_bm25s_am_vocab_map_slot slot = old_slots[i];
        size_t idx;
        size_t mask;

        if (!slot.used)
        {
            continue;
        }

        mask = map->capacity - 1;
        idx = (size_t) psql_bm25s_am_hash_bytes(slot.key) & mask;
        while (map->slots[idx].used)
        {
            idx = (idx + 1) & mask;
        }
        map->slots[idx] = slot;
        map->slots[idx].used = true;
        map->size++;
    }
    pfree(old_slots);
}

static uint32_t
psql_bm25s_am_vocab_map_get_or_add(
    psql_bm25s_am_vocab_map *map,
    const char *token
)
{
    size_t idx;
    size_t mask;

    if (map->slots == NULL)
    {
        psql_bm25s_am_vocab_map_init(map);
    }
    if ((map->size + 1) * 10 >= map->capacity * 7)
    {
        if (map->capacity > SIZE_MAX / 2)
        {
            ereport(ERROR, (errmsg("psql_bm25s vocabulary is too large")));
        }
        psql_bm25s_am_vocab_map_rehash(map, map->capacity * 2);
    }

    mask = map->capacity - 1;
    idx = (size_t) psql_bm25s_am_hash_bytes(token) & mask;
    while (map->slots[idx].used)
    {
        if (strcmp(map->slots[idx].key, token) == 0)
        {
            return map->slots[idx].value;
        }
        idx = (idx + 1) & mask;
    }

    if (map->size > UINT32_MAX)
    {
        ereport(ERROR, (errmsg("psql_bm25s vocabulary is too large")));
    }
    map->slots[idx].key = pstrdup(token);
    map->slots[idx].value = (uint32_t) map->size;
    map->slots[idx].used = true;
    map->size++;
    return map->slots[idx].value;
}

static const char **
psql_bm25s_am_vocab_map_to_array(const psql_bm25s_am_vocab_map *map)
{
    const char **vocab;
    size_t i;

    if (map->size == 0)
    {
        return NULL;
    }
    if (map->size > UINT32_MAX)
    {
        ereport(ERROR, (errmsg("psql_bm25s vocabulary is too large")));
    }
    vocab = palloc0(sizeof(*vocab) * map->size);
    for (i = 0; i < map->capacity; i++)
    {
        if (map->slots[i].used)
        {
            vocab[map->slots[i].value] = map->slots[i].key;
        }
    }
    return vocab;
}

static psql_bm25s_status
psql_bm25s_am_spill_term_entry_read(
    void *ctx,
    psql_bm25s_term_entry *entry_out
)
{
    BufFile *file = ctx;
    size_t read_bytes;

    read_bytes = BufFileRead(file, entry_out, sizeof(*entry_out));
    if (read_bytes != sizeof(*entry_out))
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_spill_term_entry_rewind(void *ctx)
{
    BufFile *file = ctx;

    if (BufFileSeek(file, 0, 0, SEEK_SET) != 0)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    return PSQL_BM25S_OK;
}

static void
psql_bm25s_am_payload_free(psql_bm25s_am_payload *payload)
{
    bool trim_malloc = false;

    if (payload == NULL)
    {
        return;
    }

    if (payload->doc_tids != NULL)
    {
        if (payload->meta.tid_bytes_len >=
            PSQL_BM25S_AM_MALLOC_TRIM_THRESHOLD)
        {
            trim_malloc = true;
        }
        free(payload->doc_tids);
    }
    if (payload->index_bytes != NULL)
    {
        if (payload->index_bytes_len >=
            PSQL_BM25S_AM_MALLOC_TRIM_THRESHOLD)
        {
            trim_malloc = true;
        }
        free(payload->index_bytes);
    }
    memset(payload, 0, sizeof(*payload));
    /*
     * Large cold/preload payload reads are intentionally transient, but glibc
     * may otherwise retain the freed arenas in a pooled backend. Return those
     * pages after multi-GB payloads without charging every small query path.
     */
    if (trim_malloc)
    {
        psql_bm25s_am_release_unused_malloc();
    }
}

static void
psql_bm25s_am_replacement_free(psql_bm25s_am_replacement *replacement)
{
    if (replacement == NULL)
    {
        return;
    }

    if (replacement->doc_tids != NULL)
    {
        pfree(replacement->doc_tids);
    }
    if (replacement->index_bytes != NULL)
    {
        free(replacement->index_bytes);
    }
    if (replacement->index_valid)
    {
        psql_bm25s_index_free(&replacement->index);
    }
    memset(replacement, 0, sizeof(*replacement));
}

static void
psql_bm25s_am_delta_tail_free(psql_bm25s_am_delta_tail *tail)
{
    size_t i;

    if (tail == NULL)
    {
        return;
    }

    for (i = 0; i < tail->len; i++)
    {
        if (tail->records[i].value_bytes != NULL)
        {
            pfree(tail->records[i].value_bytes);
        }
    }
    if (tail->records != NULL)
    {
        pfree(tail->records);
    }
    memset(tail, 0, sizeof(*tail));
}

static void
psql_bm25s_am_delta_tail_ensure_capacity(psql_bm25s_am_delta_tail *tail)
{
    if (tail->len == tail->capacity)
    {
        size_t new_capacity = tail->capacity == 0 ? 8 : tail->capacity * 2;

        if (tail->records == NULL)
        {
            tail->records = palloc(
                sizeof(*tail->records) * new_capacity
            );
        }
        else
        {
            tail->records = repalloc(
                tail->records,
                sizeof(*tail->records) * new_capacity
            );
        }
        tail->capacity = new_capacity;
    }
}

static void
psql_bm25s_am_delta_tail_append(
    psql_bm25s_am_delta_tail *tail,
    const ItemPointerData *heap_tid,
    const void *value_bytes,
    uint32 value_bytes_len
)
{
    psql_bm25s_am_delta_tail_record *record;

    psql_bm25s_am_delta_tail_ensure_capacity(tail);
    record = &tail->records[tail->len++];
    record->heap_tid = *heap_tid;
    record->value_bytes_len = value_bytes_len;
    record->value_bytes = palloc(value_bytes_len);
    memcpy(record->value_bytes, value_bytes, value_bytes_len);
}

static uint32
psql_bm25s_am_saturating_add_u32(uint32 lhs, uint32 rhs)
{
    if (UINT32_MAX - lhs < rhs)
    {
        return UINT32_MAX;
    }

    return lhs + rhs;
}

static bool
psql_bm25s_am_should_batch_refresh(void)
{
    return IsTransactionState() && GetCurrentTransactionNestLevel() == 1;
}

static bool
psql_bm25s_am_has_pending_refresh(Oid index_oid)
{
    return list_member_oid(psql_bm25s_am_pending_refresh_oids, index_oid);
}

static bool
psql_bm25s_am_has_pinned_maintenance(Oid index_oid)
{
    return list_member_oid(psql_bm25s_am_pinned_maintenance_oids, index_oid);
}

static void
psql_bm25s_am_unschedule_refresh(Oid index_oid)
{
    if (psql_bm25s_am_pending_refresh_oids == NIL)
    {
        return;
    }

    psql_bm25s_am_pending_refresh_oids = list_delete_oid(
        psql_bm25s_am_pending_refresh_oids,
        index_oid
    );
}

static void
psql_bm25s_am_clear_pending_refreshes(void)
{
    psql_bm25s_am_pending_refresh_oids = NIL;
}

static void
psql_bm25s_am_clear_pinned_maintenances(void)
{
    psql_bm25s_am_pinned_maintenance_oids = NIL;
}

static void
psql_bm25s_am_clear_pending_background_maintenance(void)
{
    psql_bm25s_am_pending_background_maintenance = false;
}

static void
psql_bm25s_am_note_pending_background_maintenance(void)
{
    psql_bm25s_am_pending_background_maintenance = true;
}

static void
psql_bm25s_am_clear_pending_maintenance_activity(void)
{
    psql_bm25s_am_pending_maintenance_activity = NIL;
}

static psql_bm25s_am_pending_activity *
psql_bm25s_am_find_pending_maintenance_activity(Oid index_oid)
{
    ListCell *cell;

    foreach (cell, psql_bm25s_am_pending_maintenance_activity)
    {
        psql_bm25s_am_pending_activity *entry = lfirst(cell);

        if (entry->index_oid == index_oid)
        {
            return entry;
        }
    }
    return NULL;
}

static void
psql_bm25s_am_note_pending_maintenance_activity(
    Relation indexRelation,
    uint32 pending_write_add,
    uint32 pending_delete_add
)
{
    MemoryContext oldcontext;
    psql_bm25s_am_pending_activity *entry;
    Oid index_oid;

    if (!IsTransactionState())
    {
        psql_bm25s_am_note_maintenance_activity(
            indexRelation,
            pending_write_add,
            pending_delete_add
        );
        psql_bm25s_am_schedule_background_maintenance(indexRelation);
        return;
    }

    index_oid = RelationGetRelid(indexRelation);
    entry = psql_bm25s_am_find_pending_maintenance_activity(index_oid);
    if (entry == NULL)
    {
        oldcontext = MemoryContextSwitchTo(TopTransactionContext);
        entry = palloc0(sizeof(*entry));
        entry->index_oid = index_oid;
        psql_bm25s_am_pending_maintenance_activity = lappend(
            psql_bm25s_am_pending_maintenance_activity,
            entry
        );
        MemoryContextSwitchTo(oldcontext);
        psql_bm25s_am_schedule_background_maintenance(indexRelation);
    }

    entry->pending_writes = psql_bm25s_am_saturating_add_u32(
        entry->pending_writes,
        pending_write_add
    );
    entry->pending_deletes = psql_bm25s_am_saturating_add_u32(
        entry->pending_deletes,
        pending_delete_add
    );
}

static void
psql_bm25s_am_flush_pending_maintenance_activity(void)
{
    List *pending;
    ListCell *cell;

    pending = psql_bm25s_am_pending_maintenance_activity;
    psql_bm25s_am_pending_maintenance_activity = NIL;

    foreach (cell, pending)
    {
        psql_bm25s_am_pending_activity *entry = lfirst(cell);
        Relation indexRelation;

        if (entry == NULL ||
            (entry->pending_writes == 0 && entry->pending_deletes == 0) ||
            !SearchSysCacheExists1(RELOID, ObjectIdGetDatum(entry->index_oid)))
        {
            continue;
        }

        indexRelation = index_open(entry->index_oid, AccessShareLock);
        PG_TRY();
        {
            if (indexRelation->rd_rel->relkind == RELKIND_INDEX &&
                indexRelation->rd_rel->relam ==
                    get_am_oid("psql_bm25s", false))
            {
                psql_bm25s_am_note_maintenance_activity(
                    indexRelation,
                    entry->pending_writes,
                    entry->pending_deletes
                );
            }
        }
        PG_CATCH();
        {
            index_close(indexRelation, AccessShareLock);
            list_free(pending);
            PG_RE_THROW();
        }
        PG_END_TRY();

        index_close(indexRelation, AccessShareLock);
    }

    list_free(pending);
}

static void
psql_bm25s_am_schedule_refresh(Relation indexRelation)
{
    MemoryContext oldcontext;
    Oid index_oid;

    if (!psql_bm25s_am_should_batch_refresh())
    {
        return;
    }

    index_oid = RelationGetRelid(indexRelation);
    if (psql_bm25s_am_has_pending_refresh(index_oid))
    {
        return;
    }

    oldcontext = MemoryContextSwitchTo(TopTransactionContext);
    psql_bm25s_am_pending_refresh_oids = lappend_oid(
        psql_bm25s_am_pending_refresh_oids,
        index_oid
    );
    MemoryContextSwitchTo(oldcontext);
}

static void
psql_bm25s_am_pin_maintenance_xact(Relation indexRelation)
{
    MemoryContext oldcontext;
    Oid index_oid;

    if (!IsTransactionState())
    {
        return;
    }

    index_oid = RelationGetRelid(indexRelation);
    if (psql_bm25s_am_has_pinned_maintenance(index_oid))
    {
        return;
    }

    /*
     * Eventual indexes append delta records before heap transaction commit.
     * Online maintenance must not start from a base metapage that already
     * includes an uncommitted delta, otherwise a replacement built from an MVCC
     * snapshot could miss that heap tuple and clear the delta debt. Use a
     * transaction-long RowExclusive marker for eventual writers: it is
     * compatible with other writers, but lets maintenance take a brief ShareLock
     * gate before reading start_meta. Realtime/manual paths keep the historical
     * ShareUpdateExclusive pin because their foreground rebuild semantics rely
     * on the stronger relation-level exclusion.
     */
    if (psql_bm25s_am_eventual_policy_enabled(indexRelation) &&
        !psql_bm25s_am_foreground_maintenance_enabled(indexRelation))
    {
        LockRelation(indexRelation, RowExclusiveLock);
    }
    else
    {
        LockRelation(indexRelation, ShareUpdateExclusiveLock);
    }

    oldcontext = MemoryContextSwitchTo(TopTransactionContext);
    psql_bm25s_am_pinned_maintenance_oids = lappend_oid(
        psql_bm25s_am_pinned_maintenance_oids,
        index_oid
    );
    MemoryContextSwitchTo(oldcontext);
}

static bool
psql_bm25s_am_wait_for_pending_maintenance(
    Relation indexRelation,
    bool block
)
{
    Oid index_oid;

    if (!IsTransactionState())
    {
        return true;
    }
    if (RecoveryInProgress())
    {
        return true;
    }

    index_oid = RelationGetRelid(indexRelation);
    if (psql_bm25s_am_has_pinned_maintenance(index_oid))
    {
        return true;
    }

    if (block)
    {
        LockRelation(indexRelation, ShareUpdateExclusiveLock);
        UnlockRelation(indexRelation, ShareUpdateExclusiveLock);
        return true;
    }

    /*
     * Non-blocking callers are eventual query paths. Match the online
     * maintenance start gate: if another transaction has uncommitted delta
     * records, skip the optional overlay instead of letting invisible rows
     * consume top-k space.
     */
    if (!ConditionalLockRelation(indexRelation, ShareLock))
    {
        return false;
    }
    UnlockRelation(indexRelation, ShareLock);
    return true;
}

static bool
psql_bm25s_am_relation_has_explicit_reloptions(Relation relation)
{
    HeapTuple tuple;
    Datum datum;
    bool isnull = true;
    bool has_options = false;

    tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(RelationGetRelid(relation)));
    if (!HeapTupleIsValid(tuple))
    {
        return false;
    }

    datum = SysCacheGetAttr(RELOID, tuple, Anum_pg_class_reloptions, &isnull);
    if (!isnull)
    {
        ArrayType *options = DatumGetArrayTypeP(datum);

        has_options = ARR_NDIM(options) > 0 && ArrayGetNItems(
            ARR_NDIM(options),
            ARR_DIMS(options)
        ) > 0;
    }

    ReleaseSysCache(tuple);
    return has_options;
}

static uint64
psql_bm25s_am_next_rebuild_count(Relation indexRelation)
{
    psql_bm25s_am_meta_page meta;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        return 1;
    }

    psql_bm25s_am_read_meta(indexRelation, &meta);
    if (meta.rebuild_count == UINT64_MAX)
    {
        return UINT64_MAX;
    }

    return meta.rebuild_count + 1;
}

static int
psql_bm25s_am_rebuild_threshold(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return 0;
    }
    if (options->consistency == PSQL_BM25S_AM_CONSISTENCY_EVENTUAL &&
        !options->auto_rebuild_threshold_is_set)
    {
        return PSQL_BM25S_AM_DEFAULT_EVENTUAL_REBUILD_THRESHOLD;
    }

    return options->auto_rebuild_threshold;
}

static int
psql_bm25s_am_get_consistency(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return PSQL_BM25S_AM_CONSISTENCY_REALTIME;
    }

    return options->consistency;
}

static bool
psql_bm25s_am_maintenance_tracking_enabled(Relation indexRelation)
{
    return psql_bm25s_am_get_consistency(indexRelation) !=
        PSQL_BM25S_AM_CONSISTENCY_MANUAL;
}

static int
psql_bm25s_am_rebuild_delta_bytes_threshold(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return 0;
    }

    return options->auto_rebuild_delta_bytes;
}

static int
psql_bm25s_am_query_overlay_max_records(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return 50000;
    }

    return options->query_overlay_max_records;
}

static int
psql_bm25s_am_query_overlay_max_bytes(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return 16777216;
    }

    return options->query_overlay_max_bytes;
}

static double
psql_bm25s_am_rebuild_churn_ratio_threshold(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return 0.0;
    }

    return options->auto_rebuild_churn_ratio;
}

static bool
psql_bm25s_am_eventual_consistency_enabled(Relation indexRelation)
{
    return psql_bm25s_am_get_consistency(indexRelation) ==
        PSQL_BM25S_AM_CONSISTENCY_EVENTUAL;
}

static bool
psql_bm25s_am_eventual_policy_enabled(Relation indexRelation)
{
    return psql_bm25s_am_eventual_consistency_enabled(indexRelation);
}

static int
psql_bm25s_am_auto_preload_priority(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL)
    {
        return 0;
    }

    return options->auto_preload;
}

static bool
psql_bm25s_am_foreground_maintenance_enabled(Relation indexRelation)
{
    return psql_bm25s_am_get_consistency(indexRelation) ==
        PSQL_BM25S_AM_CONSISTENCY_REALTIME;
}

static int
psql_bm25s_am_effective_maintenance_worker_limit(void)
{
    const char *max_worker_processes;
    char *endptr = NULL;
    long max_workers = 1;
    long configured = psql_bm25s_maintenance_worker_limit;

    if (configured < 1)
    {
        configured = 1;
    }

    max_worker_processes = GetConfigOption("max_worker_processes", true, false);
    if (max_worker_processes != NULL)
    {
        errno = 0;
        max_workers = strtol(max_worker_processes, &endptr, 10);
        if (errno != 0 || endptr == max_worker_processes || max_workers < 1)
        {
            max_workers = 1;
        }
    }

    if (configured > max_workers)
    {
        configured = max_workers;
    }
    if (configured > INT_MAX)
    {
        configured = INT_MAX;
    }
    return (int) configured;
}

static int
psql_bm25s_am_maintenance_timer_interval(void)
{
    /*
     * Treat zero and sub-second values as "use the minimum interval" instead
     * of "launch immediately". This keeps the no-preload touch fallback from
     * starting a dynamic worker on every foreground read/write.
     */
    return Max(
        PSQL_BM25S_AM_MIN_MAINTENANCE_TIMER_INTERVAL_MS,
        psql_bm25s_maintenance_timer_interval_ms
    );
}

static int
psql_bm25s_am_preload_timer_interval(void)
{
    return Max(
        PSQL_BM25S_AM_MIN_PRELOAD_TIMER_INTERVAL_MS,
        psql_bm25s_preload_timer_interval_ms
    );
}

static int
psql_bm25s_am_supervisor_timer_interval(void)
{
    return Min(
        psql_bm25s_am_preload_timer_interval(),
        psql_bm25s_am_maintenance_timer_interval()
    );
}

static bool
psql_bm25s_am_claim_maintenance_cycle(void)
{
    TimestampTz now;
    TimestampTz last;

    if (!psql_bm25s_am_shared_preload_available())
    {
        return true;
    }

    now = GetCurrentTimestamp();
    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    last = psql_bm25s_shared_preload->last_maintenance_cycle;
    if (last == 0 ||
        TimestampDifferenceExceeds(
            last,
            now,
            psql_bm25s_am_maintenance_timer_interval()))
    {
        psql_bm25s_shared_preload->last_maintenance_cycle = now;
        SpinLockRelease(&psql_bm25s_shared_preload->mutex);
        return true;
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    return false;
}

static bool
psql_bm25s_am_background_policy_active(Relation indexRelation)
{
    return psql_bm25s_am_eventual_policy_enabled(indexRelation);
}

static void
psql_bm25s_am_prune_background_launch_stamps(
    TimestampTz now,
    int cooldown_ms
)
{
    ListCell *cell;

    foreach (cell, psql_bm25s_am_background_launch_stamps)
    {
        psql_bm25s_am_background_launch_stamp *stamp = lfirst(cell);

        if (TimestampDifferenceExceeds(
                stamp->last_launch,
                now,
                cooldown_ms))
        {
            pfree(stamp);
            psql_bm25s_am_background_launch_stamps =
                foreach_delete_current(
                    psql_bm25s_am_background_launch_stamps,
                    cell
                );
        }
    }

    /*
     * The list lives in TopMemoryContext because touch wakeups are throttled
     * across transactions in long-lived backends. Bound it defensively for
     * deployments that touch many distinct indexes before old stamps expire.
     */
    while (list_length(psql_bm25s_am_background_launch_stamps) >=
           PSQL_BM25S_AM_BACKGROUND_LAUNCH_STAMP_LIMIT)
    {
        int index = 0;
        int oldest_index = -1;
        TimestampTz oldest_launch = 0;
        psql_bm25s_am_background_launch_stamp *oldest_stamp;

        foreach (cell, psql_bm25s_am_background_launch_stamps)
        {
            psql_bm25s_am_background_launch_stamp *stamp = lfirst(cell);

            if (oldest_index < 0 || stamp->last_launch < oldest_launch)
            {
                oldest_index = index;
                oldest_launch = stamp->last_launch;
            }
            index++;
        }
        if (oldest_index < 0)
        {
            break;
        }

        oldest_stamp = list_nth(
            psql_bm25s_am_background_launch_stamps,
            oldest_index
        );
        pfree(oldest_stamp);
        psql_bm25s_am_background_launch_stamps = list_delete_nth_cell(
            psql_bm25s_am_background_launch_stamps,
            oldest_index
        );
    }
}

static bool
psql_bm25s_am_background_launch_allowed(
    Oid target_index_oid,
    int cooldown_ms
)
{
    ListCell *cell;
    TimestampTz now;
    MemoryContext oldcontext;
    psql_bm25s_am_background_launch_stamp *stamp;

    cooldown_ms = Max(
        PSQL_BM25S_AM_MIN_MAINTENANCE_TIMER_INTERVAL_MS,
        cooldown_ms
    );
    now = GetCurrentTimestamp();
    psql_bm25s_am_prune_background_launch_stamps(now, cooldown_ms);
    foreach (cell, psql_bm25s_am_background_launch_stamps)
    {
        stamp = lfirst(cell);
        if (stamp->target_index_oid != target_index_oid)
        {
            continue;
        }
        if (!TimestampDifferenceExceeds(
                stamp->last_launch,
                now,
                cooldown_ms))
        {
            return false;
        }
        stamp->last_launch = now;
        return true;
    }

    oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    stamp = palloc0(sizeof(*stamp));
    stamp->target_index_oid = target_index_oid;
    stamp->last_launch = now;
    psql_bm25s_am_background_launch_stamps = lappend(
        psql_bm25s_am_background_launch_stamps,
        stamp
    );
    MemoryContextSwitchTo(oldcontext);
    return true;
}

static bool
psql_bm25s_am_launch_background_maintenance(
    Oid db_oid,
    Oid user_oid,
    bool allow_maintenance
)
{
    BackgroundWorker worker;
    psql_bm25s_am_bgworker_args args;

    if (!OidIsValid(db_oid) || !OidIsValid(user_oid))
    {
        return false;
    }

    memset(&worker, 0, sizeof(worker));
    memset(&args, 0, sizeof(args));
    args.db_oid = db_oid;
    args.user_oid = user_oid;
    args.allow_maintenance = allow_maintenance;

    if (allow_maintenance)
    {
        snprintf(
            worker.bgw_name,
            BGW_MAXLEN,
            "psql_bm25s background %u",
            db_oid
        );
        snprintf(worker.bgw_type, BGW_MAXLEN, "psql_bm25s background");
    }
    else
    {
        snprintf(
            worker.bgw_name,
            BGW_MAXLEN,
            "psql_bm25s preload %u",
            db_oid
        );
        snprintf(worker.bgw_type, BGW_MAXLEN, "psql_bm25s preload");
    }
    worker.bgw_flags =
        BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_ConsistentState;
    worker.bgw_restart_time = BGW_NEVER_RESTART;
    snprintf(worker.bgw_library_name, MAXPGPATH, "psql_bm25s");
    snprintf(
        worker.bgw_function_name,
        BGW_MAXLEN,
        "psql_bm25s_maintenance_worker_main"
    );
    memcpy(worker.bgw_extra, &args, sizeof(args));
    worker.bgw_notify_pid = 0;

    if (!RegisterDynamicBackgroundWorker(&worker, NULL))
    {
        ereport(
            DEBUG1,
            (errmsg("psql_bm25s could not launch maintenance worker"))
        );
        return false;
    }

    return true;
}

static void
psql_bm25s_am_touch_background_maintenance(void)
{
    if (psql_bm25s_am_shared_preload_available() || RecoveryInProgress())
    {
        return;
    }

    if (!psql_bm25s_am_background_launch_allowed(
            InvalidOid,
            psql_bm25s_am_maintenance_timer_interval()))
    {
        return;
    }

    /*
     * No shared-preload supervisor exists in this deployment, so the frontend
     * only performs a throttled wakeup. The dynamic worker scans due indexes,
     * picks by the same global priority as the supervisor path, and exits
     * after maintaining one index.
     */
    (void) psql_bm25s_am_launch_background_maintenance(
        MyDatabaseId,
        GetUserId(),
        true
    );
}

static void
psql_bm25s_am_schedule_background_maintenance(Relation indexRelation)
{
    if (!psql_bm25s_am_background_policy_active(indexRelation) ||
        psql_bm25s_am_shared_preload_available())
    {
        return;
    }

    if (IsTransactionState())
    {
        psql_bm25s_am_note_pending_background_maintenance();
        return;
    }

    psql_bm25s_am_touch_background_maintenance();
}

static bool
psql_bm25s_am_wait_for_auto_preload(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    bool launched_worker = false;

    if (entry == NULL ||
        meta == NULL ||
        psql_bm25s_am_preload_publisher_active ||
        !psql_bm25s_am_shared_preload_cache_available() ||
        psql_bm25s_am_auto_preload_priority(indexRelation) <= 0)
    {
        return false;
    }

    for (;;)
    {
        bool resident = false;
        bool loading = false;

        CHECK_FOR_INTERRUPTS();
        if (psql_bm25s_am_generation_attach_preload(
                entry,
                indexRelation,
                meta))
        {
            return true;
        }

        psql_bm25s_am_shared_preload_index_state(
            indexRelation,
            meta,
            &resident,
            &loading
        );
        if (!resident && !loading)
        {
            if (!psql_bm25s_am_shared_preload_should_load(
                    indexRelation,
                    meta))
            {
                return false;
            }
            if (!launched_worker &&
                psql_bm25s_am_background_launch_allowed(
                    RelationGetRelid(indexRelation),
                    psql_bm25s_am_preload_timer_interval()))
            {
                /*
                 * auto_preload means proactive residency. A foreground query
                 * that arrives before warmup finishes should wake a
                 * preload-only worker and wait for shared residency, not build
                 * a private copy or immediately steal the work from the
                 * scheduler. If the worker cannot be launched, the supervisor
                 * timer is still expected to make progress.
                 */
                (void) psql_bm25s_am_launch_background_maintenance(
                    MyDatabaseId,
                    GetUserId(),
                    false
                );
                launched_worker = true;
            }
        }

        WaitLatch(
            &MyProc->procLatch,
            WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
            100,
            0
        );
        ResetLatch(&MyProc->procLatch);
    }
}

static bool
psql_bm25s_am_query_overlay_within_budget(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    int max_records;
    int max_bytes;

    if (meta == NULL ||
        !psql_bm25s_am_eventual_policy_enabled(indexRelation))
    {
        return true;
    }
    if (meta->delta_record_count > 0 &&
        !psql_bm25s_am_meta_uses_append_only(meta))
    {
        return false;
    }

    max_records = psql_bm25s_am_query_overlay_max_records(indexRelation);
    if (max_records == 0 ||
        meta->delta_record_count > (uint32) max_records)
    {
        return false;
    }

    max_bytes = psql_bm25s_am_query_overlay_max_bytes(indexRelation);
    if (max_bytes == 0 ||
        meta->delta_bytes_len > (uint64) max_bytes)
    {
        return false;
    }

    return true;
}

static bool
psql_bm25s_am_is_eager_refresh_policy(Relation indexRelation)
{
    return psql_bm25s_am_maintenance_tracking_enabled(indexRelation) &&
        psql_bm25s_am_rebuild_threshold(indexRelation) <= 0 &&
        psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation) <= 0 &&
        psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation) <= 0.0;
}

static psql_bm25s_am_insert_state *
psql_bm25s_am_get_insert_state(IndexInfo *indexInfo)
{
    MemoryContext oldcontext;
    psql_bm25s_am_insert_state *state;

    if (indexInfo == NULL || indexInfo->ii_Context == NULL)
    {
        return NULL;
    }

    state = (psql_bm25s_am_insert_state *) indexInfo->ii_AmCache;
    if (state != NULL)
    {
        return state;
    }

    oldcontext = MemoryContextSwitchTo(indexInfo->ii_Context);
    state = palloc0(sizeof(*state));
    MemoryContextSwitchTo(oldcontext);
    indexInfo->ii_AmCache = state;
    return state;
}

static psql_bm25s_am_vacuum_stats *
psql_bm25s_am_get_vacuum_stats(IndexBulkDeleteResult *stats)
{
    if (stats == NULL)
    {
        return palloc0(sizeof(psql_bm25s_am_vacuum_stats));
    }

    return (psql_bm25s_am_vacuum_stats *) stats;
}

static void
psql_bm25s_init_reloptions(void)
{
    if (psql_bm25s_relopts_initialized)
    {
        return;
    }

    psql_bm25s_relopt_kind = add_reloption_kind();
    add_enum_reloption(
        psql_bm25s_relopt_kind,
        "method",
        "BM25 variant used when building the index",
        (relopt_enum_elt_def *) psql_bm25s_method_members,
        PSQL_BM25S_METHOD_LUCENE,
        NULL,
        AccessExclusiveLock
    );
    add_enum_reloption(
        psql_bm25s_relopt_kind,
        "idf_method",
        "BM25 IDF variant used when building the index",
        (relopt_enum_elt_def *) psql_bm25s_method_members,
        PSQL_BM25S_METHOD_LUCENE,
        NULL,
        AccessExclusiveLock
    );
    add_enum_reloption(
        psql_bm25s_relopt_kind,
        "consistency",
        "Query consistency model for pending index maintenance",
        (relopt_enum_elt_def *) psql_bm25s_consistency_members,
        PSQL_BM25S_AM_CONSISTENCY_REALTIME,
        NULL,
        AccessExclusiveLock
    );
    add_int_reloption(
        psql_bm25s_relopt_kind,
        "auto_rebuild_threshold",
        "Pending write threshold before commit-time rebuild is forced",
        0,
        0,
        INT_MAX,
        AccessExclusiveLock
    );
    add_int_reloption(
        psql_bm25s_relopt_kind,
        "auto_rebuild_delta_bytes",
        "Pending delta bytes before commit-time rebuild is forced",
        0,
        0,
        INT_MAX,
        AccessExclusiveLock
    );
    add_int_reloption(
        psql_bm25s_relopt_kind,
        "query_overlay_max_records",
        "Maximum pending delta records a query may merge inline",
        50000,
        0,
        INT_MAX,
        AccessExclusiveLock
    );
    add_int_reloption(
        psql_bm25s_relopt_kind,
        "query_overlay_max_bytes",
        "Maximum pending delta bytes a query may merge inline",
        16777216,
        0,
        INT_MAX,
        AccessExclusiveLock
    );
    add_int_reloption(
        psql_bm25s_relopt_kind,
        "auto_preload",
        "Shared-preload auto warmup priority; zero disables auto preload",
        0,
        0,
        INT_MAX,
        AccessExclusiveLock
    );
    add_real_reloption(
        psql_bm25s_relopt_kind,
        "auto_rebuild_churn_ratio",
        "Pending churn ratio before commit-time rebuild is forced",
        0.0,
        0.0,
        DBL_MAX,
        AccessExclusiveLock
    );
    add_real_reloption(
        psql_bm25s_relopt_kind,
        "k1",
        "BM25 k1 parameter",
        1.5,
        0.0,
        DBL_MAX,
        AccessExclusiveLock
    );
    add_real_reloption(
        psql_bm25s_relopt_kind,
        "b",
        "BM25 b parameter",
        0.75,
        0.0,
        DBL_MAX,
        AccessExclusiveLock
    );
    add_real_reloption(
        psql_bm25s_relopt_kind,
        "delta",
        "BM25 delta parameter",
        0.5,
        0.0,
        DBL_MAX,
        AccessExclusiveLock
    );
    add_bool_reloption(
        psql_bm25s_relopt_kind,
        "create_empty_token",
        "Create an empty token for integer-token indexes",
        true,
        AccessExclusiveLock
    );
    add_bool_reloption(
        psql_bm25s_relopt_kind,
        "text_lowercase",
        "Lowercase raw text/varchar inputs before token indexing",
        true,
        AccessExclusiveLock
    );
    add_bool_reloption(
        psql_bm25s_relopt_kind,
        "text_stem_english",
        "Apply English Porter stemming to raw text/varchar inputs",
        false,
        AccessExclusiveLock
    );
    add_bool_reloption(
        psql_bm25s_relopt_kind,
        "text_fold_diacritics",
        "Fold diacritics in raw text/varchar inputs before indexing",
        false,
        AccessExclusiveLock
    );
    add_bool_reloption(
        psql_bm25s_relopt_kind,
        "field_aware",
        "Preserve indexed column identity inside multicolumn text-like indexes",
        false,
        AccessExclusiveLock
    );
    add_string_reloption(
        psql_bm25s_relopt_kind,
        "text_stopwords",
        "Comma-separated stopwords for raw text/varchar inputs",
        NULL,
        NULL,
        AccessExclusiveLock
    );

    psql_bm25s_relopts_initialized = true;
}

static Size
psql_bm25s_am_shared_preload_arena_size(void)
{
    uint64 bytes;

    if (psql_bm25s_shared_generation_cache_size_mb <= 0)
    {
        return 0;
    }

    bytes = (uint64) psql_bm25s_shared_generation_cache_size_mb * 1024ULL *
        1024ULL;
    if (bytes > (uint64) SIZE_MAX)
    {
        ereport(ERROR, (errmsg("psql_bm25s shared generation cache is too large")));
    }
    return (Size) bytes;
}

static uint32
psql_bm25s_am_shared_preload_entry_capacity_for_arena(Size arena_size)
{
    uint64 capacity = PSQL_BM25S_AM_SHARED_PRELOAD_MIN_ENTRIES;
    uint64 arena_capacity;

    /*
     * Entry slots are metadata, not payload memory. Size them automatically so
     * startup preload can drain every marked index without the old fixed 128
     * resident-generation ceiling. The hard cap only bounds shared-memory
     * control growth for pathological schemas.
     */
    arena_capacity = (uint64) arena_size /
        PSQL_BM25S_AM_SHARED_PRELOAD_ENTRY_TARGET_BYTES;
    if (arena_capacity > capacity)
    {
        capacity = arena_capacity;
    }
    if (capacity > PSQL_BM25S_AM_SHARED_PRELOAD_MAX_ENTRIES)
    {
        capacity = PSQL_BM25S_AM_SHARED_PRELOAD_MAX_ENTRIES;
    }
    return (uint32) capacity;
}

static uint32
psql_bm25s_am_shared_preload_entry_capacity(void)
{
    if (psql_bm25s_shared_preload == NULL ||
        psql_bm25s_shared_preload->entry_capacity == 0)
    {
        return 0;
    }
    return psql_bm25s_shared_preload->entry_capacity;
}

static Size
psql_bm25s_am_shared_preload_control_size(uint32 entry_capacity)
{
    Size entries_size;
    Size control_size;

    if (!psql_bm25s_am_checked_mul_size(
            entry_capacity,
            sizeof(psql_bm25s_am_shared_preload_entry),
            &entries_size))
    {
        ereport(ERROR, (errmsg("psql_bm25s shared preload registry is too large")));
    }
    if (!psql_bm25s_am_checked_add_size(
            offsetof(psql_bm25s_am_shared_preload_control, entries),
            entries_size,
            &control_size))
    {
        ereport(ERROR, (errmsg("psql_bm25s shared preload registry is too large")));
    }
    return MAXALIGN(control_size);
}

static Size
psql_bm25s_am_shared_preload_shmem_size(void)
{
    Size arena_size;
    uint32 entry_capacity;

    arena_size = psql_bm25s_am_shared_preload_arena_size();
    entry_capacity =
        psql_bm25s_am_shared_preload_entry_capacity_for_arena(arena_size);
    return psql_bm25s_am_shared_preload_control_size(entry_capacity) +
        arena_size;
}

static bool
psql_bm25s_am_shared_preload_available(void)
{
    return psql_bm25s_shared_preload != NULL &&
        psql_bm25s_shared_preload->magic ==
            PSQL_BM25S_AM_SHARED_PRELOAD_MAGIC &&
        psql_bm25s_shared_preload->version ==
            PSQL_BM25S_AM_SHARED_PRELOAD_VERSION;
}

static bool
psql_bm25s_am_shared_preload_cache_available(void)
{
    return psql_bm25s_am_shared_preload_available() &&
        psql_bm25s_shared_preload->arena_size > 0;
}

static char *
psql_bm25s_am_shared_preload_arena_base(void)
{
    if (!psql_bm25s_am_shared_preload_available())
    {
        return NULL;
    }
    return ((char *) psql_bm25s_shared_preload) +
        psql_bm25s_am_shared_preload_control_size(
            psql_bm25s_shared_preload->entry_capacity
        );
}

static void
psql_bm25s_am_shared_preload_advise_hugepage(void)
{
#ifdef MADV_HUGEPAGE
    char *arena_base;
    long page_size;
    uintptr_t addr;
    uintptr_t aligned_addr;
    Size adjust;
    Size advised_size;

    if (!psql_bm25s_am_shared_preload_cache_available())
    {
        return;
    }

    arena_base = psql_bm25s_am_shared_preload_arena_base();
    if (arena_base == NULL)
    {
        return;
    }

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
    {
        return;
    }

    addr = (uintptr_t) arena_base;
    aligned_addr = addr - (addr % (uintptr_t) page_size);
    adjust = (Size) (addr - aligned_addr);
    if (!psql_bm25s_am_checked_add_size(
            psql_bm25s_shared_preload->arena_size,
            adjust,
            &advised_size))
    {
        return;
    }

    /*
     * A shared-preload entry being resident only means the payload bytes live
     * in the PostgreSQL shared-memory arena. Every newly forked backend still
     * has to establish page table entries the first time it scans a large
     * generation. On Linux, shmem THP plus MADV_HUGEPAGE lets those faults
     * happen at huge-page granularity when the deployment enables
     * /sys/kernel/mm/transparent_hugepage/shmem_enabled=advise. This is a
     * best-effort latency hint; failure must not change correctness.
     */
    if (madvise((void *) aligned_addr, advised_size, MADV_HUGEPAGE) != 0)
    {
        ereport(
            DEBUG1,
            (errmsg("psql_bm25s shared preload hugepage advice failed: %m"))
        );
    }
#endif
}

static void
psql_bm25s_am_shmem_request(void)
{
    Size shmem_size;

    if (psql_bm25s_prev_shmem_request_hook != NULL)
    {
        psql_bm25s_prev_shmem_request_hook();
    }

    shmem_size = psql_bm25s_am_shared_preload_shmem_size();
    if (shmem_size > 0)
    {
        RequestAddinShmemSpace(shmem_size);
    }
}

static void
psql_bm25s_am_shmem_startup(void)
{
    Size shmem_size;
    Size arena_size;
    bool found;

    if (psql_bm25s_prev_shmem_startup_hook != NULL)
    {
        psql_bm25s_prev_shmem_startup_hook();
    }

    shmem_size = psql_bm25s_am_shared_preload_shmem_size();
    if (shmem_size == 0)
    {
        return;
    }

    LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
    psql_bm25s_shared_preload = ShmemInitStruct(
        "psql_bm25s shared generation cache",
        shmem_size,
        &found
    );
    if (!found)
    {
        arena_size = psql_bm25s_am_shared_preload_arena_size();
        memset(psql_bm25s_shared_preload, 0, shmem_size);
        psql_bm25s_shared_preload->magic =
            PSQL_BM25S_AM_SHARED_PRELOAD_MAGIC;
        psql_bm25s_shared_preload->version =
            PSQL_BM25S_AM_SHARED_PRELOAD_VERSION;
        SpinLockInit(&psql_bm25s_shared_preload->mutex);
        psql_bm25s_shared_preload->arena_size = arena_size;
        psql_bm25s_shared_preload->entry_capacity =
            psql_bm25s_am_shared_preload_entry_capacity_for_arena(arena_size);
        psql_bm25s_shared_preload->used = 0;
    }
    psql_bm25s_am_shared_preload_advise_hugepage();
    LWLockRelease(AddinShmemInitLock);
}

static void
psql_bm25s_am_init_gucs(void)
{
    if (!psql_bm25s_am_workspace_gucs_initialized)
    {
        DefineCustomIntVariable(
            "psql_bm25s.workspace_cache_bytes",
            "Sets the retained per-backend query workspace budget.",
            "Mutable score/candidate workspaces larger than this budget are "
            "released after a query finishes. Set to 0 to release all query "
            "workspace after each query or -1 to retain it without a size "
            "limit in the backend.",
            &psql_bm25s_workspace_cache_bytes,
            PSQL_BM25S_AM_DEFAULT_WORKSPACE_CACHE_BYTES,
            -1,
            PSQL_BM25S_AM_MAX_WORKSPACE_CACHE_BYTES,
            PGC_USERSET,
            GUC_UNIT_BYTE,
            NULL,
            NULL,
            NULL
        );
        DefineCustomIntVariable(
            "psql_bm25s.workspace_idle_timeout",
            "Sets the idle timeout for retained per-backend query workspace.",
            "Retained mutable query workspace is released lazily when the "
            "backend next touches a psql_bm25s index after this timeout. "
            "Set to -1 to disable idle-time workspace release.",
            &psql_bm25s_workspace_idle_timeout_ms,
            PSQL_BM25S_AM_DEFAULT_WORKSPACE_IDLE_TIMEOUT_MS,
            -1,
            PSQL_BM25S_AM_MAX_WORKSPACE_IDLE_TIMEOUT_MS,
            PGC_USERSET,
            GUC_UNIT_MS,
            NULL,
            NULL,
            NULL
        );
        psql_bm25s_am_workspace_gucs_initialized = true;
    }

    if (!psql_bm25s_am_maintenance_gucs_initialized)
    {
        DefineCustomIntVariable(
            "psql_bm25s.maintenance_worker_limit",
            "Sets the psql_bm25s background maintenance worker limit.",
            "Background maintenance treats this as its own upper bound and "
            "then clamps it to PostgreSQL max_worker_processes. Each worker "
            "maintains at most one due index and exits.",
            &psql_bm25s_maintenance_worker_limit,
            PSQL_BM25S_AM_DEFAULT_MAINTENANCE_WORKER_LIMIT,
            1,
            INT_MAX,
            PGC_SIGHUP,
            0,
            NULL,
            NULL,
            NULL
        );
        DefineCustomIntVariable(
            "psql_bm25s.maintenance_timer_interval_ms",
            "Sets the psql_bm25s background maintenance wakeup interval.",
            "The shared-preload supervisor uses this only for rebuild/catch-up "
            "cycles. Deployments without shared_preload_libraries use the "
            "same value as the lightweight touch cooldown before launching a "
            "dynamic generic catch-up worker. Values below one second are "
            "clamped to one second.",
            &psql_bm25s_maintenance_timer_interval_ms,
            PSQL_BM25S_AM_DEFAULT_MAINTENANCE_TIMER_INTERVAL_MS,
            0,
            INT_MAX,
            PGC_SIGHUP,
            GUC_UNIT_MS,
            NULL,
            NULL,
            NULL
        );
        DefineCustomIntVariable(
            "psql_bm25s.preload_timer_interval_ms",
            "Sets the psql_bm25s shared-preload warmup interval.",
            "This timer is independent from maintenance throttling so startup "
            "warmup can finish quickly even when rebuild catch-up is delayed. "
            "Values below one second are clamped to one second.",
            &psql_bm25s_preload_timer_interval_ms,
            PSQL_BM25S_AM_DEFAULT_PRELOAD_TIMER_INTERVAL_MS,
            0,
            INT_MAX,
            PGC_SIGHUP,
            GUC_UNIT_MS,
            NULL,
            NULL,
            NULL
        );
        DefineCustomIntVariable(
            "psql_bm25s.maintenance_rebuild_memory_budget",
            "Sets the automatic rebuild memory admission budget.",
            "Background online maintenance estimates the current in-memory "
            "builder peak before starting. If the estimate exceeds this budget "
            "the worker skips the rebuild instead of forcing the host into "
            "swap. Set to 0 to disable the admission guard.",
            &psql_bm25s_maintenance_rebuild_memory_budget_mb,
            PSQL_BM25S_AM_DEFAULT_REBUILD_MEMORY_BUDGET_MB,
            0,
            INT_MAX,
            PGC_SIGHUP,
            GUC_UNIT_MB,
            NULL,
            NULL,
            NULL
        );
        psql_bm25s_am_maintenance_gucs_initialized = true;
    }

    if (!process_shared_preload_libraries_in_progress ||
        psql_bm25s_am_shared_preload_gucs_initialized)
    {
        return;
    }

    DefineCustomIntVariable(
        "psql_bm25s.shared_generation_cache_size",
        "Sets the optional shared-preload generation cache size.",
        "When psql_bm25s is loaded through shared_preload_libraries, this "
        "reserves a main shared-memory arena for immutable BM25 generations. "
        "A value of 0 disables the shared-preload arena and keeps the "
        "zero-configuration DSM path plus the small-index backend-local path.",
        &psql_bm25s_shared_generation_cache_size_mb,
        0,
        0,
        PSQL_BM25S_AM_SHARED_PRELOAD_MAX_MB,
        PGC_POSTMASTER,
        GUC_UNIT_MB,
        NULL,
        NULL,
        NULL
    );
    psql_bm25s_am_shared_preload_gucs_initialized = true;
}

void
_PG_init(void)
{
    psql_bm25s_am_init_gucs();
    psql_bm25s_init_reloptions();
    if (process_shared_preload_libraries_in_progress)
    {
        BackgroundWorker worker;

        psql_bm25s_prev_shmem_request_hook = shmem_request_hook;
        shmem_request_hook = psql_bm25s_am_shmem_request;
        psql_bm25s_prev_shmem_startup_hook = shmem_startup_hook;
        shmem_startup_hook = psql_bm25s_am_shmem_startup;

        memset(&worker, 0, sizeof(worker));
        snprintf(worker.bgw_name, BGW_MAXLEN, "psql_bm25s background supervisor");
        snprintf(worker.bgw_type, BGW_MAXLEN, "psql_bm25s background supervisor");
        worker.bgw_flags =
            BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
        worker.bgw_start_time = BgWorkerStart_ConsistentState;
        worker.bgw_restart_time = 60;
        snprintf(worker.bgw_library_name, MAXPGPATH, "psql_bm25s");
        snprintf(
            worker.bgw_function_name,
            BGW_MAXLEN,
            "psql_bm25s_maintenance_supervisor_main"
        );
        worker.bgw_main_arg = (Datum) 0;
        worker.bgw_notify_pid = 0;
        RegisterBackgroundWorker(&worker);
    }
    if (!psql_bm25s_am_xact_callback_registered)
    {
        RegisterXactCallback(psql_bm25s_am_xact_callback, NULL);
        psql_bm25s_am_xact_callback_registered = true;
    }
}

static const char *
psql_bm25s_am_consistency_label(int consistency)
{
    switch (consistency)
    {
        case PSQL_BM25S_AM_CONSISTENCY_REALTIME:
            return "realtime";
        case PSQL_BM25S_AM_CONSISTENCY_EVENTUAL:
            return "eventual";
        case PSQL_BM25S_AM_CONSISTENCY_MANUAL:
            return "manual";
    }
    return "realtime";
}

static bool
psql_bm25s_am_reloptions_have_name(List *reloption_defs, const char *name)
{
    ListCell *cell;

    foreach (cell, reloption_defs)
    {
        DefElem *defel = (DefElem *) lfirst(cell);

        if (defel != NULL &&
            defel->defname != NULL &&
            strcmp(defel->defname, name) == 0)
        {
            return true;
        }
    }
    return false;
}

static void
psql_bm25s_am_reject_policy_option(
    List *reloption_defs,
    const char *option_name,
    const char *consistency,
    const char *valid_policies
)
{
    if (!psql_bm25s_am_reloptions_have_name(reloption_defs, option_name))
    {
        return;
    }

    ereport(
        ERROR,
        (
            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg(
                "psql_bm25s reloption \"%s\" is not valid with "
                "consistency = '%s'",
                option_name,
                consistency
            ),
            errhint(
                "Use \"%s\" only with consistency = %s.",
                option_name,
                valid_policies
            )
        )
    );
}

static void
psql_bm25s_am_validate_policy_reloptions(
    Datum reloptions,
    const psql_bm25s_am_options *options
)
{
    List *reloption_defs;
    const char *consistency;

    if (DatumGetPointer(reloptions) == NULL || options == NULL)
    {
        return;
    }

    reloption_defs = untransformRelOptions(reloptions);
    consistency = psql_bm25s_am_consistency_label(options->consistency);

    switch (options->consistency)
    {
        case PSQL_BM25S_AM_CONSISTENCY_REALTIME:
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "query_overlay_max_records",
                consistency,
                "'eventual'"
            );
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "query_overlay_max_bytes",
                consistency,
                "'eventual'"
            );
            break;
        case PSQL_BM25S_AM_CONSISTENCY_EVENTUAL:
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "auto_rebuild_churn_ratio",
                consistency,
                "'realtime'"
            );
            break;
        case PSQL_BM25S_AM_CONSISTENCY_MANUAL:
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "auto_rebuild_threshold",
                consistency,
                "'realtime' or 'eventual'"
            );
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "auto_rebuild_delta_bytes",
                consistency,
                "'realtime' or 'eventual'"
            );
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "auto_rebuild_churn_ratio",
                consistency,
                "'realtime'"
            );
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "query_overlay_max_records",
                consistency,
                "'eventual'"
            );
            psql_bm25s_am_reject_policy_option(
                reloption_defs,
                "query_overlay_max_bytes",
                consistency,
                "'eventual'"
            );
            break;
    }

    list_free_deep(reloption_defs);
}

static void
psql_bm25s_am_set_explicit_reloption_flags(
    Datum reloptions,
    psql_bm25s_am_options *options
)
{
    List *reloption_defs;

    if (options == NULL)
    {
        return;
    }

    options->auto_rebuild_threshold_is_set = false;
    if (DatumGetPointer(reloptions) == NULL)
    {
        return;
    }

    reloption_defs = untransformRelOptions(reloptions);
    options->auto_rebuild_threshold_is_set =
        psql_bm25s_am_reloptions_have_name(
            reloption_defs,
            "auto_rebuild_threshold"
        );
    list_free_deep(reloption_defs);
}

static bytea *
psql_bm25s_amoptions(Datum reloptions, bool validate)
{
    psql_bm25s_am_options *options;

    psql_bm25s_init_reloptions();
    options = (psql_bm25s_am_options *) build_reloptions(
        reloptions,
        validate,
        psql_bm25s_relopt_kind,
        sizeof(psql_bm25s_am_options),
        psql_bm25s_relopt_elems,
        lengthof(psql_bm25s_relopt_elems)
    );
    psql_bm25s_am_set_explicit_reloption_flags(reloptions, options);
    if (validate && options != NULL)
    {
        psql_bm25s_am_validate_policy_reloptions(reloptions, options);
    }
    return (bytea *) options;
}

static void
psql_bm25s_am_read_params(
    Relation indexRelation,
    psql_bm25s_params *params_out,
    bool *create_empty_token_out
)
{
    psql_bm25s_am_options *options;

    Assert(params_out != NULL);
    Assert(create_empty_token_out != NULL);

    params_out->method = PSQL_BM25S_METHOD_LUCENE;
    params_out->idf_method = PSQL_BM25S_METHOD_LUCENE;
    params_out->k1 = 1.5f;
    params_out->b = 0.75f;
    params_out->delta = 0.5f;
    *create_empty_token_out = true;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL ||
        !psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        return;
    }

    params_out->method = (psql_bm25s_method) options->method;
    params_out->idf_method = (psql_bm25s_method) options->idf_method;
    params_out->k1 = (float) options->k1;
    params_out->b = (float) options->b;
    params_out->delta = (float) options->delta;
    *create_empty_token_out = options->create_empty_token;
}

static int
psql_bm25s_am_index_natts(Relation indexRelation)
{
    return indexRelation->rd_att->natts;
}

static bool
psql_bm25s_am_field_aware_enabled(Relation indexRelation)
{
    psql_bm25s_am_options *options;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options == NULL)
    {
        return false;
    }

    return options->field_aware;
}

static bool
psql_bm25s_am_is_multicol_index(Relation indexRelation)
{
    return psql_bm25s_am_index_natts(indexRelation) > 1;
}

static bool
psql_bm25s_am_all_index_values_null(const bool *isnull, int natts)
{
    int i;

    for (i = 0; i < natts; i++)
    {
        if (!isnull[i])
        {
            return false;
        }
    }

    return true;
}

static Oid
psql_bm25s_am_source_type(Relation indexRelation)
{
    int natts;
    Oid source_type;
    int i;

    natts = psql_bm25s_am_index_natts(indexRelation);
    if (natts <= 0)
    {
        ereport(ERROR, (errmsg("psql_bm25s requires at least one indexed column")));
    }

    source_type = TupleDescAttr(indexRelation->rd_att, 0)->atttypid;
    for (i = 1; i < natts; i++)
    {
        Oid column_type = TupleDescAttr(indexRelation->rd_att, i)->atttypid;

        if (column_type != source_type)
        {
            ereport(
                ERROR,
                (
                    errmsg("psql_bm25s multicolumn indexes require matching column types")
                )
            );
        }
    }

    return source_type;
}

static void
psql_bm25s_am_validate_source_type(Oid source_type)
{
    if (source_type != INT4ARRAYOID &&
        source_type != TEXTARRAYOID &&
        source_type != VARCHARARRAYOID &&
        source_type != TEXTOID &&
        source_type != VARCHAROID)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s indexes require int4[], text[], varchar[], text, or varchar input"
                )
            )
        );
    }
}

static bool
psql_bm25s_am_source_type_is_text_array(Oid source_type)
{
    return source_type == TEXTARRAYOID || source_type == VARCHARARRAYOID;
}

static bool
psql_bm25s_am_source_type_is_scalar_text(Oid source_type)
{
    return source_type == TEXTOID || source_type == VARCHAROID;
}

static bool
psql_bm25s_am_source_type_is_textlike(Oid source_type)
{
    return psql_bm25s_am_source_type_is_text_array(source_type) ||
        psql_bm25s_am_source_type_is_scalar_text(source_type);
}

static psql_bm25s_am_build_mode
psql_bm25s_am_build_mode_from_source_type(Oid source_type, int natts)
{
    if (source_type == INT4ARRAYOID)
    {
        if (natts != 1)
        {
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s multicolumn fusion does not support int4[] columns"
                    )
                )
            );
        }
        return PSQL_BM25S_AM_BUILD_MODE_IDS;
    }

    if (psql_bm25s_am_source_type_is_text_array(source_type))
    {
        if (natts == 1)
        {
            return PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_SINGLE;
        }
        return PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_MULTI;
    }

    if (natts == 1)
    {
        return PSQL_BM25S_AM_BUILD_MODE_SCALAR_SINGLE;
    }

    return PSQL_BM25S_AM_BUILD_MODE_SCALAR_MULTI;
}

static Oid
psql_bm25s_am_text_array_element_type(
    ArrayType *array,
    const char *context
)
{
    Oid element_type = ARR_ELEMTYPE(array);

    if (element_type != TEXTOID && element_type != VARCHAROID)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "%s must use text[] or varchar[] values",
                    context
                )
            )
        );
    }

    return element_type;
}

static Oid
psql_bm25s_am_require_textlike_source_type(
    Relation indexRelation,
    const char *context
)
{
    Oid source_type = psql_bm25s_am_source_type(indexRelation);

    if (!psql_bm25s_am_source_type_is_textlike(source_type))
    {
        ereport(ERROR, (errmsg("%s", context)));
    }

    return source_type;
}

static void
psql_bm25s_am_append_tid(
    psql_bm25s_tid_builder *builder,
    const ItemPointerData *tid
)
{
    psql_bm25s_ensure_tid_capacity(builder);
    builder->tids[builder->len++] = *tid;
}

static void
psql_bm25s_am_append_docid(
    psql_bm25s_docid_builder *builder,
    uint32_t doc_id
)
{
    psql_bm25s_ensure_docid_capacity(builder);
    builder->doc_ids[builder->len++] = doc_id;
}

static void
psql_bm25s_am_append_owned_id_doc(
    psql_bm25s_doc_ids_builder *builder,
    psql_bm25s_doc_ids *doc
)
{
    psql_bm25s_ensure_doc_ids_capacity(builder);
    builder->docs[builder->len++] = *doc;
    memset(doc, 0, sizeof(*doc));
}

static void
psql_bm25s_am_append_owned_token_doc(
    psql_bm25s_doc_tokens_builder *builder,
    psql_bm25s_doc_tokens *doc
)
{
    psql_bm25s_ensure_doc_tokens_capacity(builder);
    builder->docs[builder->len++] = *doc;
    memset(doc, 0, sizeof(*doc));
}

static void
psql_bm25s_am_append_id_doc(
    psql_bm25s_doc_ids_builder *builder,
    ArrayType *array
)
{
    Datum *datums;
    bool *nulls;
    int nelems;
    int i;
    psql_bm25s_doc_ids doc = {0};

    deconstruct_array(
        array,
        INT4OID,
        4,
        true,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );

    if (nelems > 0)
    {
        doc.token_ids = palloc(sizeof(*doc.token_ids) * (size_t) nelems);
    }
    doc.len = (size_t) nelems;

    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("indexed int4[] documents cannot contain NULLs")));
        }
        doc.token_ids[i] = (uint32_t) DatumGetInt32(datums[i]);
    }

    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }

    psql_bm25s_ensure_doc_ids_capacity(builder);
    builder->docs[builder->len++] = doc;
}

static void
psql_bm25s_am_free_id_doc(psql_bm25s_doc_ids *doc)
{
    if (doc == NULL)
    {
        return;
    }

    if (doc->token_ids != NULL)
    {
        pfree(doc->token_ids);
    }
    memset(doc, 0, sizeof(*doc));
}

static void
psql_bm25s_am_free_token_doc(psql_bm25s_doc_tokens *doc)
{
    size_t i;

    if (doc == NULL)
    {
        return;
    }

    for (i = 0; i < doc->len; i++)
    {
        if (doc->tokens != NULL && doc->tokens[i] != NULL)
        {
            pfree((void *) doc->tokens[i]);
        }
    }
    if (doc->tokens != NULL)
    {
        pfree((void *) doc->tokens);
    }
    memset(doc, 0, sizeof(*doc));
}

static void
psql_bm25s_am_append_token_doc(
    psql_bm25s_doc_tokens_builder *builder,
    ArrayType *array
)
{
    psql_bm25s_doc_tokens doc = {0};

    doc.tokens = (const char **) psql_bm25s_array_read_doc_tokens(
        array,
        &doc.len
    );

    psql_bm25s_ensure_doc_tokens_capacity(builder);
    builder->docs[builder->len++] = doc;
}

static void
psql_bm25s_am_append_compact_doc_from_ids(
    psql_bm25s_am_build_state *build_state,
    ItemPointer tid,
    const uint32_t *token_ids,
    size_t token_len
)
{
    uint32_t *sorted_ids = NULL;
    uint32_t doc_id;
    size_t i;
    size_t bytes;

    if (build_state->doc_lengths.len > UINT32_MAX ||
        token_len > UINT32_MAX)
    {
        ereport(ERROR, (errmsg("psql_bm25s compact build is too large")));
    }
    doc_id = (uint32_t) build_state->doc_lengths.len;
    psql_bm25s_am_append_tid(&build_state->tids, tid);
    psql_bm25s_am_ensure_doc_length_capacity(&build_state->doc_lengths);
    build_state->doc_lengths.lengths[build_state->doc_lengths.len++] =
        (uint32_t) token_len;

    if (token_len == 0)
    {
        build_state->index_tuples += 1.0;
        return;
    }
    if (!psql_bm25s_am_checked_mul_size(token_len, sizeof(*sorted_ids), &bytes))
    {
        ereport(ERROR, (errmsg("psql_bm25s compact build is too large")));
    }
    sorted_ids = palloc(bytes);
    memcpy(sorted_ids, token_ids, bytes);
    qsort(
        sorted_ids,
        token_len,
        sizeof(*sorted_ids),
        psql_bm25s_am_cmp_uint32_asc
    );

    for (i = 0; i < token_len; i++)
    {
        uint32_t token_id = sorted_ids[i];
        uint32_t tf = 1;

        if (!build_state->has_terms || token_id > build_state->max_token_id)
        {
            build_state->max_token_id = token_id;
        }
        if (token_id == 0)
        {
            build_state->zero_present = true;
        }
        build_state->has_terms = true;

        while (i + 1 < token_len && sorted_ids[i + 1] == token_id)
        {
            tf++;
            i++;
        }
        if (build_state->rebuild_builder ==
            PSQL_BM25S_AM_REBUILD_BUILDER_SPILL)
        {
            psql_bm25s_term_entry entry;

            if (build_state->spill_entries == NULL)
            {
                build_state->spill_entries = BufFileCreateTemp(false);
            }
            entry.token_id = token_id;
            entry.doc_id = doc_id;
            entry.tf = tf;
            BufFileWrite(
                build_state->spill_entries,
                &entry,
                sizeof(entry)
            );
            build_state->spill_entry_count++;
        }
        else
        {
            psql_bm25s_am_ensure_term_entry_capacity(
                &build_state->term_entries
            );
            build_state->term_entries.entries[
                build_state->term_entries.len
            ].token_id = token_id;
            build_state->term_entries.entries[
                build_state->term_entries.len
            ].doc_id = doc_id;
            build_state->term_entries.entries[
                build_state->term_entries.len
            ].tf = tf;
            build_state->term_entries.len++;
        }
    }

    pfree(sorted_ids);
    build_state->index_tuples += 1.0;
}

static void
psql_bm25s_am_append_compact_doc_from_tokens(
    psql_bm25s_am_build_state *build_state,
    ItemPointer tid,
    const psql_bm25s_doc_tokens *doc
)
{
    uint32_t *token_ids = NULL;
    size_t i;

    if (doc->len > 0)
    {
        size_t bytes;

        if (!psql_bm25s_am_checked_mul_size(
                doc->len,
                sizeof(*token_ids),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s compact build is too large")));
        }
        token_ids = palloc(bytes);
        for (i = 0; i < doc->len; i++)
        {
            token_ids[i] = psql_bm25s_am_vocab_map_get_or_add(
                &build_state->vocab_map,
                doc->tokens[i]
            );
        }
    }

    psql_bm25s_am_append_compact_doc_from_ids(
        build_state,
        tid,
        token_ids,
        doc->len
    );
    if (token_ids != NULL)
    {
        pfree(token_ids);
    }
}

static void
psql_bm25s_am_append_compact_doc_from_int_array(
    psql_bm25s_am_build_state *build_state,
    ItemPointer tid,
    ArrayType *array
)
{
    Datum *datums;
    bool *nulls;
    int nelems;
    uint32_t *token_ids = NULL;
    int i;

    deconstruct_array(
        array,
        INT4OID,
        4,
        true,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );
    if (nelems > 0)
    {
        token_ids = palloc(sizeof(*token_ids) * (size_t) nelems);
    }
    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("indexed int4[] documents cannot contain NULLs")));
        }
        token_ids[i] = (uint32_t) DatumGetInt32(datums[i]);
    }

    psql_bm25s_am_append_compact_doc_from_ids(
        build_state,
        tid,
        token_ids,
        (size_t) nelems
    );
    if (token_ids != NULL)
    {
        pfree(token_ids);
    }
    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }
}

static void
psql_bm25s_am_tokenize_scalar_datum(
    Datum value,
    const psql_bm25s_text_options *options,
    psql_bm25s_doc_tokens *doc_out
)
{
    char *raw_text;
    char **tokens = NULL;
    size_t token_len = 0;
    size_t i;
    psql_bm25s_status status;

    memset(doc_out, 0, sizeof(*doc_out));
    raw_text = TextDatumGetCString(value);
    status = psql_bm25s_tokenize_text(
        raw_text,
        options,
        &tokens,
        &token_len
    );
    pfree(raw_text);
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_text_tokens_free(tokens, token_len);
        ereport(
            ERROR,
            (
                errmsg(
                    "failed to tokenize indexed text value: %s",
                    psql_bm25s_strerror(status)
                )
            )
        );
    }

    if (token_len > 0)
    {
        doc_out->tokens = palloc(sizeof(*doc_out->tokens) * token_len);
        for (i = 0; i < token_len; i++)
        {
            doc_out->tokens[i] = pstrdup(tokens[i]);
        }
    }
    doc_out->len = token_len;
    psql_bm25s_text_tokens_free(tokens, token_len);
}

static void
psql_bm25s_am_collect_token_doc_from_values(
    Datum *values,
    bool *isnull,
    int natts,
    Oid source_type,
    const psql_bm25s_text_options *text_options,
    psql_bm25s_doc_tokens *doc_out
)
{
    int i;

    memset(doc_out, 0, sizeof(*doc_out));
    for (i = 0; i < natts; i++)
    {
        char **tokens = NULL;
        size_t token_len = 0;
        size_t new_len;
        size_t bytes;
        psql_bm25s_doc_tokens scalar_doc = {0};

        if (isnull[i])
        {
            continue;
        }

        if (psql_bm25s_am_source_type_is_text_array(source_type))
        {
            ArrayType *array = DatumGetArrayTypeP(values[i]);

            tokens = psql_bm25s_array_read_doc_tokens(array, &token_len);
        }
        else if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            const psql_bm25s_text_options *effective_options = text_options;
            psql_bm25s_text_options default_options;

            if (effective_options == NULL)
            {
                psql_bm25s_text_options_init(&default_options);
                effective_options = &default_options;
            }
            psql_bm25s_am_tokenize_scalar_datum(
                values[i],
                effective_options,
                &scalar_doc
            );
            tokens = (char **) scalar_doc.tokens;
            token_len = scalar_doc.len;
        }
        else
        {
            psql_bm25s_am_free_token_doc(doc_out);
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s multicolumn fusion requires text[], varchar[], text, or varchar columns"
                    )
                )
            );
        }
        if (token_len == 0)
        {
            if (scalar_doc.tokens != NULL)
            {
                psql_bm25s_am_free_token_doc(&scalar_doc);
            }
            else if (tokens != NULL)
            {
                pfree(tokens);
            }
            continue;
        }

        if (doc_out->len > SIZE_MAX - token_len)
        {
            if (scalar_doc.tokens != NULL)
            {
                psql_bm25s_am_free_token_doc(&scalar_doc);
            }
            else if (tokens != NULL)
            {
                pfree(tokens);
            }
            psql_bm25s_am_free_token_doc(doc_out);
            ereport(ERROR, (errmsg("indexed document is too large")));
        }
        new_len = doc_out->len + token_len;
        if (!psql_bm25s_am_checked_mul_size(
                new_len,
                sizeof(*doc_out->tokens),
                &bytes))
        {
            if (scalar_doc.tokens != NULL)
            {
                psql_bm25s_am_free_token_doc(&scalar_doc);
            }
            else if (tokens != NULL)
            {
                pfree(tokens);
            }
            psql_bm25s_am_free_token_doc(doc_out);
            ereport(ERROR, (errmsg("indexed document is too large")));
        }

        if (doc_out->tokens == NULL)
        {
            doc_out->tokens = palloc(bytes);
        }
        else
        {
            doc_out->tokens = repalloc((void *) doc_out->tokens, bytes);
        }
        memcpy(
            (void *) (doc_out->tokens + doc_out->len),
            tokens,
            sizeof(*tokens) * token_len
        );
        doc_out->len = new_len;
        if (scalar_doc.tokens != NULL)
        {
            pfree((void *) scalar_doc.tokens);
            scalar_doc.tokens = NULL;
            scalar_doc.len = 0;
        }
        else
        {
            pfree(tokens);
        }
    }
}

static char *
psql_bm25s_am_make_field_token(int field_index, const char *token)
{
    int written;
    size_t len;
    char *field_token;

    if (token == NULL)
    {
        ereport(ERROR, (errmsg("field-aware indexed tokens cannot be NULL")));
    }
    written = snprintf(NULL, 0, "\x1F" "f%d" "\x1F" "%s", field_index, token);
    if (written < 0)
    {
        ereport(ERROR, (errmsg("failed to build field-aware token")));
    }
    len = (size_t) written + 1;
    field_token = palloc(len);
    written = snprintf(
        field_token,
        len,
        "\x1F" "f%d" "\x1F" "%s",
        field_index,
        token
    );
    if (written < 0 || (size_t) written >= len)
    {
        pfree(field_token);
        ereport(ERROR, (errmsg("failed to build field-aware token")));
    }
    return field_token;
}

static void
psql_bm25s_am_append_field_tokens(
    psql_bm25s_doc_tokens *doc_out,
    int field_index,
    char **tokens,
    size_t token_len
)
{
    size_t new_len;
    size_t bytes;
    size_t i;

    if (token_len == 0)
    {
        return;
    }
    if (doc_out->len > SIZE_MAX - token_len)
    {
        psql_bm25s_am_free_token_doc(doc_out);
        ereport(ERROR, (errmsg("indexed document is too large")));
    }
    new_len = doc_out->len + token_len;
    if (!psql_bm25s_am_checked_mul_size(
            new_len,
            sizeof(*doc_out->tokens),
            &bytes))
    {
        psql_bm25s_am_free_token_doc(doc_out);
        ereport(ERROR, (errmsg("indexed document is too large")));
    }

    if (doc_out->tokens == NULL)
    {
        doc_out->tokens = palloc(bytes);
    }
    else
    {
        doc_out->tokens = repalloc((void *) doc_out->tokens, bytes);
    }

    for (i = 0; i < token_len; i++)
    {
        doc_out->tokens[doc_out->len + i] =
            psql_bm25s_am_make_field_token(field_index, tokens[i]);
    }
    doc_out->len = new_len;
}

static void
psql_bm25s_am_collect_field_token_doc_from_values(
    Datum *values,
    bool *isnull,
    int natts,
    Oid source_type,
    const psql_bm25s_text_options *text_options,
    psql_bm25s_doc_tokens *doc_out
)
{
    int i;

    memset(doc_out, 0, sizeof(*doc_out));
    for (i = 0; i < natts; i++)
    {
        char **tokens = NULL;
        size_t token_len = 0;
        psql_bm25s_doc_tokens scalar_doc = {0};

        if (isnull[i])
        {
            continue;
        }

        if (psql_bm25s_am_source_type_is_text_array(source_type))
        {
            ArrayType *array = DatumGetArrayTypeP(values[i]);

            tokens = psql_bm25s_array_read_doc_tokens(array, &token_len);
        }
        else if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            const psql_bm25s_text_options *effective_options = text_options;
            psql_bm25s_text_options default_options;

            if (effective_options == NULL)
            {
                psql_bm25s_text_options_init(&default_options);
                effective_options = &default_options;
            }
            psql_bm25s_am_tokenize_scalar_datum(
                values[i],
                effective_options,
                &scalar_doc
            );
            tokens = (char **) scalar_doc.tokens;
            token_len = scalar_doc.len;
        }
        else
        {
            psql_bm25s_am_free_token_doc(doc_out);
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s field-aware indexes require text[], "
                        "varchar[], text, or varchar columns"
                    )
                )
            );
        }

        psql_bm25s_am_append_field_tokens(doc_out, i, tokens, token_len);
        if (scalar_doc.tokens != NULL)
        {
            psql_bm25s_am_free_token_doc(&scalar_doc);
        }
        else if (tokens != NULL)
        {
            psql_bm25s_am_free_query_tokens(tokens, token_len);
        }
    }
}

static ArrayType *
psql_bm25s_am_build_fused_text_array(
    Datum *values,
    bool *isnull,
    int natts,
    Oid source_type,
    const psql_bm25s_text_options *text_options
)
{
    psql_bm25s_doc_tokens doc = {0};
    ArrayType *array;

    psql_bm25s_am_collect_token_doc_from_values(
        values,
        isnull,
        natts,
        source_type,
        text_options,
        &doc
    );
    array = psql_bm25s_am_text_array_from_cstrings((char **) doc.tokens, doc.len);
    psql_bm25s_am_free_token_doc(&doc);
    return array;
}

static ArrayType *
psql_bm25s_am_build_field_aware_text_array(
    Datum *values,
    bool *isnull,
    int natts,
    Oid source_type,
    const psql_bm25s_text_options *text_options
)
{
    psql_bm25s_doc_tokens doc = {0};
    ArrayType *array;

    psql_bm25s_am_collect_field_token_doc_from_values(
        values,
        isnull,
        natts,
        source_type,
        text_options,
        &doc
    );
    array = psql_bm25s_am_text_array_from_cstrings((char **) doc.tokens, doc.len);
    psql_bm25s_am_free_token_doc(&doc);
    return array;
}

static void
psql_bm25s_am_build_callback(
    Relation indexRelation,
    ItemPointer tid,
    Datum *values,
    bool *isnull,
    bool tupleIsAlive,
    void *state
)
{
    psql_bm25s_am_build_state *build_state = state;
    ArrayType *array;
    psql_bm25s_doc_tokens doc = {0};

    (void) indexRelation;

    if (!tupleIsAlive)
    {
        return;
    }

    if (build_state->rebuild_builder ==
            PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT ||
        build_state->rebuild_builder ==
            PSQL_BM25S_AM_REBUILD_BUILDER_SPILL)
    {
        switch (build_state->build_mode)
        {
            case PSQL_BM25S_AM_BUILD_MODE_IDS:
                if (isnull[0])
                {
                    return;
                }
                array = DatumGetArrayTypeP(values[0]);
                psql_bm25s_am_append_compact_doc_from_int_array(
                    build_state,
                    tid,
                    array
                );
                break;
            case PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_SINGLE:
                if (isnull[0])
                {
                    return;
                }
                array = DatumGetArrayTypeP(values[0]);
                doc.tokens = (const char **) psql_bm25s_array_read_doc_tokens(
                    array,
                    &doc.len
                );
                psql_bm25s_am_append_compact_doc_from_tokens(
                    build_state,
                    tid,
                    &doc
                );
                psql_bm25s_am_free_token_doc(&doc);
                break;
            case PSQL_BM25S_AM_BUILD_MODE_SCALAR_SINGLE:
                if (isnull[0])
                {
                    return;
                }
                psql_bm25s_am_tokenize_scalar_datum(
                    values[0],
                    &build_state->text_options,
                    &doc
                );
                psql_bm25s_am_append_compact_doc_from_tokens(
                    build_state,
                    tid,
                    &doc
                );
                psql_bm25s_am_free_token_doc(&doc);
                break;
            case PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_MULTI:
            case PSQL_BM25S_AM_BUILD_MODE_SCALAR_MULTI:
                if (psql_bm25s_am_all_index_values_null(
                        isnull,
                        build_state->natts))
                {
                    return;
                }
                if (psql_bm25s_am_field_aware_enabled(indexRelation))
                {
                    psql_bm25s_am_collect_field_token_doc_from_values(
                        values,
                        isnull,
                        build_state->natts,
                        build_state->source_type,
                        &build_state->text_options,
                        &doc
                    );
                }
                else
                {
                    psql_bm25s_am_collect_token_doc_from_values(
                        values,
                        isnull,
                        build_state->natts,
                        build_state->source_type,
                        &build_state->text_options,
                        &doc
                    );
                }
                psql_bm25s_am_append_compact_doc_from_tokens(
                    build_state,
                    tid,
                    &doc
                );
                psql_bm25s_am_free_token_doc(&doc);
                break;
        }
        return;
    }

    switch (build_state->build_mode)
    {
        case PSQL_BM25S_AM_BUILD_MODE_IDS:
            if (isnull[0])
            {
                return;
            }
            array = DatumGetArrayTypeP(values[0]);
            psql_bm25s_am_append_tid(&build_state->tids, tid);
            psql_bm25s_am_append_id_doc(&build_state->id_docs, array);
            break;
        case PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_SINGLE:
            if (isnull[0])
            {
                return;
            }
            array = DatumGetArrayTypeP(values[0]);
            psql_bm25s_am_append_tid(&build_state->tids, tid);
            psql_bm25s_am_append_token_doc(&build_state->token_docs, array);
            break;
        case PSQL_BM25S_AM_BUILD_MODE_SCALAR_SINGLE:
            if (isnull[0])
            {
                return;
            }
            psql_bm25s_am_tokenize_scalar_datum(
                values[0],
                &build_state->text_options,
                &doc
            );
            psql_bm25s_am_append_tid(&build_state->tids, tid);
            psql_bm25s_am_append_owned_token_doc(&build_state->token_docs, &doc);
            break;
        case PSQL_BM25S_AM_BUILD_MODE_TEXT_ARRAY_MULTI:
        case PSQL_BM25S_AM_BUILD_MODE_SCALAR_MULTI:
            if (psql_bm25s_am_all_index_values_null(isnull, build_state->natts))
            {
                return;
            }
            if (psql_bm25s_am_field_aware_enabled(indexRelation))
            {
                psql_bm25s_am_collect_field_token_doc_from_values(
                    values,
                    isnull,
                    build_state->natts,
                    build_state->source_type,
                    &build_state->text_options,
                    &doc
                );
            }
            else
            {
                psql_bm25s_am_collect_token_doc_from_values(
                    values,
                    isnull,
                    build_state->natts,
                    build_state->source_type,
                    &build_state->text_options,
                    &doc
                );
            }
            psql_bm25s_am_append_tid(&build_state->tids, tid);
            psql_bm25s_am_append_owned_token_doc(&build_state->token_docs, &doc);
            break;
    }

    build_state->index_tuples += 1.0;
}

static void
psql_bm25s_am_write_new_page(Relation indexRelation, const void *contents, Size len)
{
    Buffer buffer;
    Page page;

    buffer = ReadBufferExtended(indexRelation, MAIN_FORKNUM, P_NEW, RBM_NORMAL, NULL);
    LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
    page = BufferGetPage(buffer);
    PageInit(page, BLCKSZ, 0);
    memcpy(PageGetContents(page), contents, len);
    MarkBufferDirty(buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        /*
         * These pages store raw custom payload in PageGetContents() and do
         * not maintain the standard pd_lower/pd_upper item layout, so WAL
         * must treat them as non-standard full-page images.
         */
        recptr = log_newpage_buffer(buffer, false);
        PageSetLSN(page, recptr);
    }
    UnlockReleaseBuffer(buffer);
}

static void
psql_bm25s_am_lock_maintenance(Relation indexRelation)
{
    /*
     * Serialize custom maintenance writers and rebuilds without blocking
     * ordinary read scans. We rely on this to keep metapage and delta
     * updates from racing with vacuum-driven full rewrites.
     */
    LockRelation(indexRelation, ShareUpdateExclusiveLock);
}

static void
psql_bm25s_am_unlock_maintenance(Relation indexRelation)
{
    UnlockRelation(indexRelation, ShareUpdateExclusiveLock);
}

static bool
psql_bm25s_am_delta_record_value_fits(uint32 value_bytes_len)
{
    const Size header_size = MAXALIGN(sizeof(psql_bm25s_am_data_page));
    const Size record_header_size =
        MAXALIGN(sizeof(psql_bm25s_am_delta_record_header));
    const Size max_bytes = BLCKSZ - SizeOfPageHeaderData - header_size;

    return record_header_size + MAXALIGN((Size) value_bytes_len) <= max_bytes;
}

static void
psql_bm25s_am_note_delta_record(
    Relation indexRelation,
    const ItemPointerData *heap_tid,
    const void *value_bytes,
    uint32 value_bytes_len
)
{
    Buffer meta_buffer;
    Page meta_page_raw;
    psql_bm25s_am_meta_page *meta;
    BlockNumber blkno;
    const Size header_size = MAXALIGN(sizeof(psql_bm25s_am_data_page));
    const Size record_header_size =
        MAXALIGN(sizeof(psql_bm25s_am_delta_record_header));
    const Size required_bytes = record_header_size + MAXALIGN(value_bytes_len);
    char page_contents[BLCKSZ - SizeOfPageHeaderData];
    psql_bm25s_am_data_page page_header;
    bool appended = false;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        return;
    }

    psql_bm25s_am_pin_maintenance_xact(indexRelation);
    psql_bm25s_am_lock_maintenance(indexRelation);
    meta_buffer = ReadBufferExtended(
        indexRelation,
        MAIN_FORKNUM,
        0,
        RBM_NORMAL,
        NULL
    );
    LockBuffer(meta_buffer, BUFFER_LOCK_EXCLUSIVE);
    meta_page_raw = BufferGetPage(meta_buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(meta_page_raw);

    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(meta_buffer);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    if (!psql_bm25s_am_delta_record_value_fits(value_bytes_len))
    {
        UnlockReleaseBuffer(meta_buffer);
        ereport(ERROR, (errmsg("delta record too large for psql_bm25s page")));
    }

    for (blkno = RelationGetNumberOfBlocks(indexRelation); blkno > 1; blkno--)
    {
        Buffer buffer;
        Page page;
        psql_bm25s_am_data_page *existing;
        char *page_data;

        buffer = ReadBufferExtended(
            indexRelation,
            MAIN_FORKNUM,
            blkno - 1,
            RBM_NORMAL,
            NULL
        );
        LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
        page = BufferGetPage(buffer);
        existing = (psql_bm25s_am_data_page *) PageGetContents(page);

        if (existing->magic != PSQL_BM25S_AM_MAGIC ||
            existing->version != PSQL_BM25S_AM_VERSION)
        {
            UnlockReleaseBuffer(buffer);
            UnlockReleaseBuffer(meta_buffer);
            ereport(ERROR, (errmsg("invalid psql_bm25s index page")));
        }

        if (existing->page_kind != PSQL_BM25S_AM_PAGE_DELTA)
        {
            UnlockReleaseBuffer(buffer);
            break;
        }

        if ((Size) existing->used_bytes + required_bytes <=
            BLCKSZ - SizeOfPageHeaderData - header_size)
        {
            psql_bm25s_am_delta_record_header record_header;
            Size offset = header_size + (Size) existing->used_bytes;

            record_header.heap_tid = *heap_tid;
            record_header.value_bytes_len = value_bytes_len;
            page_data = (char *) PageGetContents(page);
            memcpy(page_data + offset, &record_header, sizeof(record_header));
            if (record_header_size > sizeof(record_header))
            {
                memset(
                    page_data + offset + sizeof(record_header),
                    0,
                    record_header_size - sizeof(record_header)
                );
            }
            memcpy(page_data + offset + record_header_size, value_bytes, value_bytes_len);
            if (MAXALIGN(value_bytes_len) > value_bytes_len)
            {
                memset(
                    page_data + offset + record_header_size + value_bytes_len,
                    0,
                    MAXALIGN(value_bytes_len) - value_bytes_len
                );
            }
            existing->used_bytes += (uint32) required_bytes;
            MarkBufferDirty(buffer);
            if (RelationNeedsWAL(indexRelation))
            {
                log_newpage_buffer(buffer, false);
            }
            UnlockReleaseBuffer(buffer);
            appended = true;
            break;
        }

        UnlockReleaseBuffer(buffer);
    }

    if (!appended)
    {
        psql_bm25s_am_delta_record_header record_header;

        memset(page_contents, 0, sizeof(page_contents));
        memset(&page_header, 0, sizeof(page_header));
        page_header.magic = PSQL_BM25S_AM_MAGIC;
        page_header.version = PSQL_BM25S_AM_VERSION;
        page_header.page_kind = PSQL_BM25S_AM_PAGE_DELTA;
        page_header.used_bytes = (uint32) required_bytes;
        memcpy(page_contents, &page_header, sizeof(page_header));
        record_header.heap_tid = *heap_tid;
        record_header.value_bytes_len = value_bytes_len;
        memcpy(page_contents + header_size, &record_header, sizeof(record_header));
        if (record_header_size > sizeof(record_header))
        {
            memset(
                page_contents + header_size + sizeof(record_header),
                0,
                record_header_size - sizeof(record_header)
            );
        }
        memcpy(
            page_contents + header_size + record_header_size,
            value_bytes,
            value_bytes_len
        );
        if (MAXALIGN(value_bytes_len) > value_bytes_len)
        {
            memset(
                page_contents + header_size + record_header_size + value_bytes_len,
                0,
                MAXALIGN(value_bytes_len) - value_bytes_len
            );
        }
        psql_bm25s_am_write_new_page(
            indexRelation,
            page_contents,
            header_size + required_bytes
        );
    }

    meta->delta_record_count = psql_bm25s_am_saturating_add_u32(
        meta->delta_record_count,
        1
    );
    if (UINT64_MAX - meta->delta_bytes_len < (uint64) value_bytes_len)
    {
        meta->delta_bytes_len = UINT64_MAX;
    }
    else
    {
        meta->delta_bytes_len += (uint64) value_bytes_len;
    }
    MarkBufferDirty(meta_buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(meta_buffer, false);
        PageSetLSN(meta_page_raw, recptr);
    }
    UnlockReleaseBuffer(meta_buffer);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static void
psql_bm25s_am_note_tombstone_record(
    Relation indexRelation,
    uint32_t doc_id
)
{
    ItemPointerData invalid_tid;

    ItemPointerSetInvalid(&invalid_tid);
    psql_bm25s_am_note_delta_record(
        indexRelation,
        &invalid_tid,
        &doc_id,
        (uint32) sizeof(doc_id)
    );
}

static void
psql_bm25s_am_write_relation(
    Relation indexRelation,
    Oid source_type,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const uint8_t *index_bytes,
    size_t index_bytes_len,
    uint16 cache_epoch,
    uint16 flags
)
{
    psql_bm25s_am_meta_page meta;
    size_t tid_bytes_len;
    uint64 rebuild_count;
    uint32 generation;
    BlockNumber start_blkno;
    BlockNumber delta_start_blkno;
    uint32 data_pages;
    Buffer meta_buffer;
    Page meta_page_raw;
    psql_bm25s_am_meta_page *meta_page;

    tid_bytes_len = sizeof(*doc_tids) * num_docs;
    rebuild_count = psql_bm25s_am_next_rebuild_count(indexRelation);
    generation = rebuild_count > UINT32_MAX
        ? UINT32_MAX
        : (uint32) rebuild_count;

    memset(&meta, 0, sizeof(meta));
    meta.magic = PSQL_BM25S_AM_MAGIC;
    meta.version = PSQL_BM25S_AM_VERSION;
    meta.page_kind = PSQL_BM25S_AM_PAGE_META;

    psql_bm25s_am_lock_maintenance(indexRelation);
    RelationTruncate(indexRelation, 0);
    psql_bm25s_am_write_new_page(indexRelation, &meta, sizeof(meta));
    psql_bm25s_am_write_generation_pages(
        indexRelation,
        doc_tids,
        num_docs,
        index_bytes,
        index_bytes_len,
        generation,
        &start_blkno,
        &data_pages
    );
    delta_start_blkno = RelationGetNumberOfBlocks(indexRelation);

    meta_buffer = ReadBufferExtended(
        indexRelation,
        MAIN_FORKNUM,
        0,
        RBM_NORMAL,
        NULL
    );
    LockBuffer(meta_buffer, BUFFER_LOCK_EXCLUSIVE);
    meta_page_raw = BufferGetPage(meta_buffer);
    meta_page = (psql_bm25s_am_meta_page *) PageGetContents(meta_page_raw);
    if (meta_page->magic != PSQL_BM25S_AM_MAGIC ||
        meta_page->version != PSQL_BM25S_AM_VERSION ||
        meta_page->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(meta_buffer);
        psql_bm25s_am_unlock_maintenance(indexRelation);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta_page->flags = flags;
    meta_page->cache_epoch = cache_epoch;
    meta_page->source_type = source_type;
    meta_page->num_docs = (uint32) num_docs;
    meta_page->tid_bytes_len = tid_bytes_len;
    meta_page->index_bytes_len = index_bytes_len;
    meta_page->delta_record_count = 0;
    meta_page->delta_bytes_len = 0;
    meta_page->pending_write_tuples = 0;
    meta_page->pending_delete_tuples = 0;
    meta_page->rebuild_count = rebuild_count;
    meta_page->storage_version = PSQL_BM25S_AM_STORAGE_APPEND_ONLY;
    meta_page->active_generation = generation;
    meta_page->active_start_blkno = start_blkno;
    meta_page->active_data_pages = data_pages;
    meta_page->delta_start_blkno = delta_start_blkno;

    MarkBufferDirty(meta_buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(meta_buffer, false);
        PageSetLSN(meta_page_raw, recptr);
    }
    UnlockReleaseBuffer(meta_buffer);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static void
psql_bm25s_am_write_generation_pages(
    Relation indexRelation,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const uint8_t *index_bytes,
    size_t index_bytes_len,
    uint32 generation,
    BlockNumber *start_blkno_out,
    uint32 *data_pages_out
)
{
    size_t tid_bytes_len = sizeof(*doc_tids) * num_docs;
    size_t tid_bytes_offset = 0;
    size_t index_bytes_offset = 0;
    uint32 ordinal = 0;
    const Size header_size =
        MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));
    const Size max_payload = BLCKSZ - SizeOfPageHeaderData - header_size;

    *start_blkno_out = RelationGetNumberOfBlocks(indexRelation);
    *data_pages_out = 0;
    while (tid_bytes_offset < tid_bytes_len ||
           index_bytes_offset < index_bytes_len)
    {
        psql_bm25s_am_generation_data_page page_header;
        char page_contents[BLCKSZ - SizeOfPageHeaderData];
        size_t chunk_len = 0;

        memset(page_contents, 0, sizeof(page_contents));
        memset(&page_header, 0, sizeof(page_header));
        page_header.magic = PSQL_BM25S_AM_MAGIC;
        page_header.version = PSQL_BM25S_AM_VERSION;
        page_header.page_kind = PSQL_BM25S_AM_PAGE_GENERATION_DATA;
        page_header.generation = generation;
        page_header.ordinal = ordinal++;

        if (tid_bytes_offset < tid_bytes_len)
        {
            size_t available = (size_t) max_payload - chunk_len;
            size_t copy_len = Min(available, tid_bytes_len - tid_bytes_offset);

            memcpy(
                page_contents + header_size + chunk_len,
                ((const char *) doc_tids) + tid_bytes_offset,
                copy_len
            );
            chunk_len += copy_len;
            tid_bytes_offset += copy_len;
        }

        if (chunk_len < (size_t) max_payload &&
            index_bytes_offset < index_bytes_len)
        {
            size_t available = (size_t) max_payload - chunk_len;
            size_t copy_len = Min(available, index_bytes_len - index_bytes_offset);

            memcpy(
                page_contents + header_size + chunk_len,
                index_bytes + index_bytes_offset,
                copy_len
            );
            chunk_len += copy_len;
            index_bytes_offset += copy_len;
        }

        page_header.used_bytes = (uint32) chunk_len;
        memcpy(page_contents, &page_header, sizeof(page_header));
        psql_bm25s_am_write_new_page(
            indexRelation,
            page_contents,
            header_size + chunk_len
        );
        (*data_pages_out)++;
    }
}

typedef struct psql_bm25s_am_generation_stream_writer
{
    Relation indexRelation;
    uint32 generation;
    uint32 ordinal;
    BlockNumber start_blkno;
    uint32 data_pages;
    size_t chunk_len;
    char page_contents[BLCKSZ - SizeOfPageHeaderData];
} psql_bm25s_am_generation_stream_writer;

static void
psql_bm25s_am_generation_stream_init(
    psql_bm25s_am_generation_stream_writer *writer,
    Relation indexRelation,
    uint32 generation
)
{
    memset(writer, 0, sizeof(*writer));
    writer->indexRelation = indexRelation;
    writer->generation = generation;
    writer->start_blkno = RelationGetNumberOfBlocks(indexRelation);
}

static void
psql_bm25s_am_generation_stream_flush(
    psql_bm25s_am_generation_stream_writer *writer
)
{
    psql_bm25s_am_generation_data_page page_header;
    const Size header_size =
        MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));

    if (writer->chunk_len == 0)
    {
        return;
    }

    memset(&page_header, 0, sizeof(page_header));
    page_header.magic = PSQL_BM25S_AM_MAGIC;
    page_header.version = PSQL_BM25S_AM_VERSION;
    page_header.page_kind = PSQL_BM25S_AM_PAGE_GENERATION_DATA;
    page_header.used_bytes = (uint32) writer->chunk_len;
    page_header.generation = writer->generation;
    page_header.ordinal = writer->ordinal++;

    memcpy(writer->page_contents, &page_header, sizeof(page_header));
    psql_bm25s_am_write_new_page(
        writer->indexRelation,
        writer->page_contents,
        header_size + writer->chunk_len
    );
    writer->data_pages++;
    writer->chunk_len = 0;
    memset(writer->page_contents, 0, sizeof(writer->page_contents));
}

static psql_bm25s_status
psql_bm25s_am_generation_stream_write(
    void *ctx,
    const uint8_t *bytes,
    size_t len
)
{
    psql_bm25s_am_generation_stream_writer *writer = ctx;
    const Size header_size =
        MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));
    const Size max_payload = BLCKSZ - SizeOfPageHeaderData - header_size;
    size_t offset = 0;

    while (offset < len)
    {
        size_t available = (size_t) max_payload - writer->chunk_len;
        size_t copy_len = Min(available, len - offset);

        memcpy(
            writer->page_contents + header_size + writer->chunk_len,
            bytes + offset,
            copy_len
        );
        writer->chunk_len += copy_len;
        offset += copy_len;
        if (writer->chunk_len == (size_t) max_payload)
        {
            psql_bm25s_am_generation_stream_flush(writer);
        }
    }
    return PSQL_BM25S_OK;
}

static void
psql_bm25s_am_write_relation_from_index(
    Relation indexRelation,
    Oid source_type,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const psql_bm25s_index *index,
    size_t index_bytes_len,
    uint16 cache_epoch,
    uint16 flags
)
{
    psql_bm25s_am_meta_page meta;
    size_t tid_bytes_len;
    uint64 rebuild_count;
    uint32 generation;
    BlockNumber delta_start_blkno;
    Buffer meta_buffer;
    Page meta_page_raw;
    psql_bm25s_am_meta_page *meta_page;
    psql_bm25s_am_generation_stream_writer writer;
    psql_bm25s_status status;

    tid_bytes_len = sizeof(*doc_tids) * num_docs;
    rebuild_count = psql_bm25s_am_next_rebuild_count(indexRelation);
    generation = rebuild_count > UINT32_MAX
        ? UINT32_MAX
        : (uint32) rebuild_count;

    memset(&meta, 0, sizeof(meta));
    meta.magic = PSQL_BM25S_AM_MAGIC;
    meta.version = PSQL_BM25S_AM_VERSION;
    meta.page_kind = PSQL_BM25S_AM_PAGE_META;

    psql_bm25s_am_lock_maintenance(indexRelation);
    RelationTruncate(indexRelation, 0);
    psql_bm25s_am_write_new_page(indexRelation, &meta, sizeof(meta));

    psql_bm25s_am_generation_stream_init(
        &writer,
        indexRelation,
        generation
    );
    if (tid_bytes_len > 0)
    {
        status = psql_bm25s_am_generation_stream_write(
            &writer,
            (const uint8_t *) doc_tids,
            tid_bytes_len
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_unlock_maintenance(indexRelation);
            ereport(ERROR, (errmsg("failed to stream bm25 tids")));
        }
    }
    status = psql_bm25s_serialize_index_stream(
        index,
        psql_bm25s_am_generation_stream_write,
        &writer,
        &index_bytes_len
    );
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_am_unlock_maintenance(indexRelation);
        ereport(ERROR, (errmsg("failed to stream bm25 index: %s",
                               psql_bm25s_strerror(status))));
    }
    psql_bm25s_am_generation_stream_flush(&writer);
    delta_start_blkno = RelationGetNumberOfBlocks(indexRelation);

    meta_buffer = ReadBufferExtended(
        indexRelation,
        MAIN_FORKNUM,
        0,
        RBM_NORMAL,
        NULL
    );
    LockBuffer(meta_buffer, BUFFER_LOCK_EXCLUSIVE);
    meta_page_raw = BufferGetPage(meta_buffer);
    meta_page = (psql_bm25s_am_meta_page *) PageGetContents(meta_page_raw);
    if (meta_page->magic != PSQL_BM25S_AM_MAGIC ||
        meta_page->version != PSQL_BM25S_AM_VERSION ||
        meta_page->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(meta_buffer);
        psql_bm25s_am_unlock_maintenance(indexRelation);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta_page->flags = flags;
    meta_page->cache_epoch = cache_epoch;
    meta_page->source_type = source_type;
    meta_page->num_docs = (uint32) num_docs;
    meta_page->tid_bytes_len = tid_bytes_len;
    meta_page->index_bytes_len = index_bytes_len;
    meta_page->delta_record_count = 0;
    meta_page->delta_bytes_len = 0;
    meta_page->pending_write_tuples = 0;
    meta_page->pending_delete_tuples = 0;
    meta_page->rebuild_count = rebuild_count;
    meta_page->storage_version = PSQL_BM25S_AM_STORAGE_APPEND_ONLY;
    meta_page->active_generation = generation;
    meta_page->active_start_blkno = writer.start_blkno;
    meta_page->active_data_pages = writer.data_pages;
    meta_page->delta_start_blkno = delta_start_blkno;

    MarkBufferDirty(meta_buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(meta_buffer, false);
        PageSetLSN(meta_page_raw, recptr);
    }
    UnlockReleaseBuffer(meta_buffer);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static void
psql_bm25s_am_publish_append_only_generation(
    Relation indexRelation,
    Oid source_type,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const uint8_t *index_bytes,
    size_t index_bytes_len,
    uint16 cache_epoch,
    uint16 flags,
    uint32 pending_writes,
    uint32 pending_deletes
)
{
    Buffer meta_buffer;
    Page meta_page_raw;
    psql_bm25s_am_meta_page *meta;
    BlockNumber start_blkno;
    BlockNumber delta_start_blkno;
    uint32 data_pages;
    uint64 rebuild_count;
    uint32 generation;
    size_t tid_bytes_len;

    tid_bytes_len = sizeof(*doc_tids) * num_docs;
    rebuild_count = psql_bm25s_am_next_rebuild_count(indexRelation);
    generation = rebuild_count > UINT32_MAX
        ? UINT32_MAX
        : (uint32) rebuild_count;

    /*
     * Caller holds ShareUpdateExclusiveLock. Keep publish itself to append-only
     * page writes plus a small metapage switch; do not take AccessExclusiveLock
     * or truncate the relation on the online path.
     */
    psql_bm25s_am_write_generation_pages(
        indexRelation,
        doc_tids,
        num_docs,
        index_bytes,
        index_bytes_len,
        generation,
        &start_blkno,
        &data_pages
    );
    delta_start_blkno = RelationGetNumberOfBlocks(indexRelation);

    meta_buffer = ReadBufferExtended(
        indexRelation,
        MAIN_FORKNUM,
        0,
        RBM_NORMAL,
        NULL
    );
    LockBuffer(meta_buffer, BUFFER_LOCK_EXCLUSIVE);
    meta_page_raw = BufferGetPage(meta_buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(meta_page_raw);
    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(meta_buffer);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta->flags = flags;
    meta->cache_epoch = cache_epoch;
    meta->source_type = source_type;
    meta->num_docs = (uint32) num_docs;
    meta->tid_bytes_len = tid_bytes_len;
    meta->index_bytes_len = index_bytes_len;
    meta->delta_record_count = 0;
    meta->delta_bytes_len = 0;
    meta->pending_write_tuples = pending_writes;
    meta->pending_delete_tuples = pending_deletes;
    meta->rebuild_count = rebuild_count;
    meta->storage_version = PSQL_BM25S_AM_STORAGE_APPEND_ONLY;
    meta->active_generation = generation;
    meta->active_start_blkno = start_blkno;
    meta->active_data_pages = data_pages;
    meta->delta_start_blkno = delta_start_blkno;

    MarkBufferDirty(meta_buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(meta_buffer, false);
        PageSetLSN(meta_page_raw, recptr);
    }
    UnlockReleaseBuffer(meta_buffer);
}

static void
psql_bm25s_am_publish_append_only_generation_from_index(
    Relation indexRelation,
    Oid source_type,
    const ItemPointerData *doc_tids,
    size_t num_docs,
    const psql_bm25s_index *index,
    size_t index_bytes_len,
    uint16 cache_epoch,
    uint16 flags,
    uint32 pending_writes,
    uint32 pending_deletes
)
{
    Buffer meta_buffer;
    Page meta_page_raw;
    psql_bm25s_am_meta_page *meta;
    BlockNumber delta_start_blkno;
    uint64 rebuild_count;
    uint32 generation;
    size_t tid_bytes_len;
    psql_bm25s_am_generation_stream_writer writer;
    psql_bm25s_status status;

    tid_bytes_len = sizeof(*doc_tids) * num_docs;
    rebuild_count = psql_bm25s_am_next_rebuild_count(indexRelation);
    generation = rebuild_count > UINT32_MAX
        ? UINT32_MAX
        : (uint32) rebuild_count;

    psql_bm25s_am_generation_stream_init(&writer, indexRelation, generation);
    if (tid_bytes_len > 0)
    {
        status = psql_bm25s_am_generation_stream_write(
            &writer,
            (const uint8_t *) doc_tids,
            tid_bytes_len
        );
        if (status != PSQL_BM25S_OK)
        {
            ereport(ERROR, (errmsg("failed to stream bm25 tids")));
        }
    }
    status = psql_bm25s_serialize_index_stream(
        index,
        psql_bm25s_am_generation_stream_write,
        &writer,
        &index_bytes_len
    );
    if (status != PSQL_BM25S_OK)
    {
        ereport(ERROR, (errmsg("failed to stream bm25 index: %s",
                               psql_bm25s_strerror(status))));
    }
    psql_bm25s_am_generation_stream_flush(&writer);
    delta_start_blkno = RelationGetNumberOfBlocks(indexRelation);

    meta_buffer = ReadBufferExtended(
        indexRelation,
        MAIN_FORKNUM,
        0,
        RBM_NORMAL,
        NULL
    );
    LockBuffer(meta_buffer, BUFFER_LOCK_EXCLUSIVE);
    meta_page_raw = BufferGetPage(meta_buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(meta_page_raw);
    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(meta_buffer);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta->flags = flags;
    meta->cache_epoch = cache_epoch;
    meta->source_type = source_type;
    meta->num_docs = (uint32) num_docs;
    meta->tid_bytes_len = tid_bytes_len;
    meta->index_bytes_len = index_bytes_len;
    meta->delta_record_count = 0;
    meta->delta_bytes_len = 0;
    meta->pending_write_tuples = pending_writes;
    meta->pending_delete_tuples = pending_deletes;
    meta->rebuild_count = rebuild_count;
    meta->storage_version = PSQL_BM25S_AM_STORAGE_APPEND_ONLY;
    meta->active_generation = generation;
    meta->active_start_blkno = writer.start_blkno;
    meta->active_data_pages = writer.data_pages;
    meta->delta_start_blkno = delta_start_blkno;

    MarkBufferDirty(meta_buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(meta_buffer, false);
        PageSetLSN(meta_page_raw, recptr);
    }
    UnlockReleaseBuffer(meta_buffer);
}

static uint16
psql_bm25s_am_next_cache_epoch(Relation indexRelation)
{
    psql_bm25s_am_meta_page meta;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        return 1;
    }

    psql_bm25s_am_read_meta(indexRelation, &meta);
    if (meta.cache_epoch == UINT16_MAX)
    {
        return 1;
    }

    return (uint16) (meta.cache_epoch + 1);
}

static void
psql_bm25s_am_reindex_relation(Relation indexRelation, IndexInfo *indexInfo)
{
    Relation heapRelation;
    IndexBuildResult *result;

    psql_bm25s_am_lock_maintenance(indexRelation);
    heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
    result = psql_bm25s_am_build_common(heapRelation, indexRelation, indexInfo);
    if (result != NULL)
    {
        pfree(result);
    }
    table_close(heapRelation, AccessShareLock);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static void
psql_bm25s_am_rebuild_index_oid(Oid index_oid)
{
    Relation indexRelation;
    IndexInfo *indexInfo;
    bool pushed_snapshot = false;

    if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(index_oid)))
    {
        return;
    }

    indexRelation = index_open(index_oid, AccessExclusiveLock);
    if (indexRelation->rd_rel->relkind != RELKIND_INDEX)
    {
        index_close(indexRelation, AccessExclusiveLock);
        return;
    }
    if (indexRelation->rd_rel->relam != get_am_oid("psql_bm25s", false))
    {
        index_close(indexRelation, AccessExclusiveLock);
        return;
    }

    indexInfo = BuildIndexInfo(indexRelation);
    CommandCounterIncrement();
    if (!ActiveSnapshotSet())
    {
        PushActiveSnapshot(GetTransactionSnapshot());
        pushed_snapshot = true;
    }
    PG_TRY();
    {
        psql_bm25s_am_reindex_relation(indexRelation, indexInfo);
    }
    PG_CATCH();
    {
        if (pushed_snapshot)
        {
            PopActiveSnapshot();
        }
        PG_RE_THROW();
    }
    PG_END_TRY();
    if (pushed_snapshot)
    {
        PopActiveSnapshot();
    }
    if (indexInfo != NULL)
    {
        pfree(indexInfo);
    }
    index_close(indexRelation, AccessExclusiveLock);
}

static bool
psql_bm25s_am_meta_equal_for_online_swap(
    const psql_bm25s_am_meta_page *start,
    const psql_bm25s_am_meta_page *current
)
{
    return start->magic == current->magic &&
        start->version == current->version &&
        start->page_kind == current->page_kind &&
        start->flags == current->flags &&
        start->cache_epoch == current->cache_epoch &&
        start->source_type == current->source_type &&
        start->num_docs == current->num_docs &&
        start->tid_bytes_len == current->tid_bytes_len &&
        start->index_bytes_len == current->index_bytes_len &&
        start->delta_record_count == current->delta_record_count &&
        start->delta_bytes_len == current->delta_bytes_len &&
        start->pending_write_tuples == current->pending_write_tuples &&
        start->pending_delete_tuples == current->pending_delete_tuples &&
        start->rebuild_count == current->rebuild_count &&
        start->storage_version == current->storage_version &&
        start->active_generation == current->active_generation &&
        start->active_start_blkno == current->active_start_blkno &&
        start->active_data_pages == current->active_data_pages &&
        start->delta_start_blkno == current->delta_start_blkno;
}

static bool
psql_bm25s_am_meta_has_compatible_delta_tail(
    const psql_bm25s_am_meta_page *start,
    const psql_bm25s_am_meta_page *current
)
{
    /*
     * Concurrent tail carry is only valid for append-only generations. Any
     * unsupported on-disk layout must be rebuilt from the heap and published as
     * a clean append-only generation instead of trying to inspect old pages.
     */
    return start->magic == current->magic &&
        start->version == current->version &&
        start->page_kind == current->page_kind &&
        psql_bm25s_am_meta_uses_append_only(start) &&
        psql_bm25s_am_meta_uses_append_only(current) &&
        start->flags == current->flags &&
        start->cache_epoch == current->cache_epoch &&
        start->source_type == current->source_type &&
        start->num_docs == current->num_docs &&
        start->tid_bytes_len == current->tid_bytes_len &&
        start->index_bytes_len == current->index_bytes_len &&
        start->rebuild_count == current->rebuild_count &&
        start->storage_version == current->storage_version &&
        start->active_generation == current->active_generation &&
        start->active_start_blkno == current->active_start_blkno &&
        start->active_data_pages == current->active_data_pages &&
        start->delta_start_blkno == current->delta_start_blkno &&
        current->delta_record_count >= start->delta_record_count &&
        current->delta_bytes_len >= start->delta_bytes_len &&
        current->pending_write_tuples >= start->pending_write_tuples &&
        current->pending_delete_tuples >= start->pending_delete_tuples;
}

static HTAB *
psql_bm25s_am_build_tid_doc_id_hash(
    const ItemPointerData *tids,
    size_t num_tids
)
{
    HASHCTL hash_ctl;
    HTAB *tid_to_doc_id;
    size_t i;

    memset(&hash_ctl, 0, sizeof(hash_ctl));
    hash_ctl.keysize = sizeof(ItemPointerData);
    hash_ctl.entrysize = sizeof(psql_bm25s_am_tid_doc_map_entry);
    tid_to_doc_id = hash_create(
        "psql_bm25s online replacement tids",
        (long) Max(num_tids, (size_t) 16),
        &hash_ctl,
        HASH_ELEM | HASH_BLOBS
    );

    for (i = 0; i < num_tids; i++)
    {
        psql_bm25s_am_tid_doc_map_entry *entry;

        if (!ItemPointerIsValid(&tids[i]))
        {
            continue;
        }
        entry = hash_search(tid_to_doc_id, &tids[i], HASH_ENTER, NULL);
        entry->doc_id = (uint32_t) i;
    }

    return tid_to_doc_id;
}

static bool
psql_bm25s_am_lookup_doc_id(
    HTAB *tid_to_doc_id,
    const ItemPointerData *tid,
    uint32_t *doc_id_out
)
{
    psql_bm25s_am_tid_doc_map_entry *entry;

    if (tid_to_doc_id == NULL || !ItemPointerIsValid(tid))
    {
        return false;
    }

    entry = hash_search(tid_to_doc_id, tid, HASH_FIND, NULL);
    if (entry == NULL)
    {
        return false;
    }

    *doc_id_out = entry->doc_id;
    return true;
}

static HTAB *
psql_bm25s_am_build_doc_id_hash(size_t expected_size)
{
    HASHCTL hash_ctl;

    memset(&hash_ctl, 0, sizeof(hash_ctl));
    hash_ctl.keysize = sizeof(uint32_t);
    hash_ctl.entrysize = sizeof(psql_bm25s_am_doc_id_map_entry);
    return hash_create(
        "psql_bm25s online delta tail doc ids",
        (long) Max(expected_size, (size_t) 16),
        &hash_ctl,
        HASH_ELEM | HASH_BLOBS
    );
}

static void
psql_bm25s_am_remap_doc_id(
    HTAB *doc_id_map,
    uint32_t old_doc_id,
    uint32_t new_doc_id
)
{
    psql_bm25s_am_doc_id_map_entry *entry;

    entry = hash_search(doc_id_map, &old_doc_id, HASH_ENTER, NULL);
    entry->new_doc_id = new_doc_id;
}

static bool
psql_bm25s_am_lookup_remapped_doc_id(
    HTAB *doc_id_map,
    uint32_t old_doc_id,
    uint32_t *new_doc_id_out
)
{
    psql_bm25s_am_doc_id_map_entry *entry;

    entry = hash_search(doc_id_map, &old_doc_id, HASH_FIND, NULL);
    if (entry == NULL)
    {
        return false;
    }

    *new_doc_id_out = entry->new_doc_id;
    return true;
}

static void
psql_bm25s_am_read_active_generation_bytes(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    uint64 offset,
    void *dest,
    Size len
)
{
    const Size generation_header_size =
        MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));
    const Size header_size = generation_header_size;
    const Size max_payload = BLCKSZ - SizeOfPageHeaderData - header_size;
    char *dest_bytes = dest;
    uint64 remaining = (uint64) len;
    uint64 total_payload_len;

    if (meta == NULL || dest == NULL || len == 0)
    {
        return;
    }
    if (!psql_bm25s_am_meta_uses_append_only(meta))
    {
        ereport(
            ERROR,
            (
                errmsg("unsupported psql_bm25s index storage layout"),
                errhint("Rebuild the index to use append-only generations.")
            )
        );
    }
    if (UINT64_MAX - meta->tid_bytes_len < meta->index_bytes_len)
    {
        ereport(ERROR, (errmsg("psql_bm25s generation length overflow")));
    }
    total_payload_len = meta->tid_bytes_len + meta->index_bytes_len;
    if (max_payload == 0 ||
        (uint64) len > total_payload_len ||
        offset > total_payload_len - (uint64) len)
    {
        ereport(ERROR, (errmsg("psql_bm25s generation read out of bounds")));
    }

    while (remaining > 0)
    {
        uint64 payload_page = offset / (uint64) max_payload;
        Size page_offset = (Size) (offset % (uint64) max_payload);
        Size copy_len = (Size) Min(
            remaining,
            (uint64) max_payload - (uint64) page_offset
        );
        BlockNumber blkno =
            meta->active_start_blkno + (BlockNumber) payload_page;
        Buffer buffer;
        Page page;
        const char *page_data;
        uint32 used_bytes;

        if (payload_page >= (uint64) meta->active_data_pages)
        {
            ereport(ERROR, (errmsg("psql_bm25s generation read out of bounds")));
        }

        buffer = ReadBufferExtended(
            indexRelation,
            MAIN_FORKNUM,
            blkno,
            RBM_NORMAL,
            NULL
        );
        LockBuffer(buffer, BUFFER_LOCK_SHARE);
        page = BufferGetPage(buffer);
        page_data = (const char *) PageGetContents(page);

        {
            psql_bm25s_am_generation_data_page *generation_header =
                (psql_bm25s_am_generation_data_page *) page_data;

            if (generation_header->magic != PSQL_BM25S_AM_MAGIC ||
                generation_header->version != PSQL_BM25S_AM_VERSION ||
                generation_header->page_kind !=
                    PSQL_BM25S_AM_PAGE_GENERATION_DATA ||
                generation_header->generation != meta->active_generation ||
                generation_header->ordinal != (uint32) payload_page)
            {
                UnlockReleaseBuffer(buffer);
                ereport(ERROR, (errmsg("invalid psql_bm25s generation page")));
            }
            used_bytes = generation_header->used_bytes;
        }
        if (page_offset + copy_len > (Size) used_bytes)
        {
            UnlockReleaseBuffer(buffer);
            ereport(ERROR, (errmsg("truncated psql_bm25s generation payload")));
        }

        memcpy(dest_bytes, page_data + header_size + page_offset, copy_len);
        UnlockReleaseBuffer(buffer);

        dest_bytes += copy_len;
        offset += (uint64) copy_len;
        remaining -= (uint64) copy_len;
    }
}

static bool
psql_bm25s_am_active_generation_doc_id_to_tid(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *old_meta,
    const psql_bm25s_tid_builder *delta_tids,
    uint32_t doc_id,
    ItemPointerData *tid_out
)
{
    if (doc_id < old_meta->num_docs)
    {
        uint64 offset = (uint64) doc_id * sizeof(ItemPointerData);

        psql_bm25s_am_read_active_generation_bytes(
            indexRelation,
            old_meta,
            offset,
            tid_out,
            sizeof(*tid_out)
        );
        return ItemPointerIsValid(tid_out);
    }

    doc_id -= old_meta->num_docs;
    if ((size_t) doc_id >= delta_tids->len)
    {
        return false;
    }

    *tid_out = delta_tids->tids[doc_id];
    return ItemPointerIsValid(tid_out);
}

static void
psql_bm25s_am_collect_delta_tail(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *start_meta,
    const psql_bm25s_am_meta_page *current_meta,
    const psql_bm25s_am_replacement *replacement,
    psql_bm25s_am_delta_tail *tail_out
)
{
    psql_bm25s_tid_builder all_delta_tids = {0};
    HTAB *replacement_tid_to_doc_id;
    HTAB *tail_doc_id_map;
    BlockNumber blkno;
    BlockNumber nblocks;
    BlockNumber delta_start_blkno;
    uint32 tail_pending_writes;
    uint32 tail_pending_deletes;
    uint32 seen_tail_writes = 0;
    uint32 seen_tail_deletes = 0;
    uint32 carried_writes = 0;
    uint32 carried_deletes = 0;
    uint32 record_index = 0;
    uint32 delta_write_index = 0;
    uint32 carried_write_index = 0;
    const Size header_size = MAXALIGN(sizeof(psql_bm25s_am_data_page));
    const Size record_header_size =
        MAXALIGN(sizeof(psql_bm25s_am_delta_record_header));

    memset(tail_out, 0, sizeof(*tail_out));
    tail_pending_writes = current_meta->pending_write_tuples -
        start_meta->pending_write_tuples;
    tail_pending_deletes = current_meta->pending_delete_tuples -
        start_meta->pending_delete_tuples;

    if (current_meta->delta_record_count == start_meta->delta_record_count)
    {
        tail_out->pending_writes = tail_pending_writes;
        tail_out->pending_deletes = tail_pending_deletes;
        return;
    }

    psql_bm25s_am_load_delta_state(
        indexRelation,
        current_meta,
        current_meta->source_type,
        current_meta->delta_record_count,
        NULL,
        NULL,
        &all_delta_tids,
        NULL
    );
    replacement_tid_to_doc_id = psql_bm25s_am_build_tid_doc_id_hash(
        replacement->doc_tids,
        replacement->num_docs
    );
    tail_doc_id_map = psql_bm25s_am_build_doc_id_hash(
        current_meta->delta_record_count - start_meta->delta_record_count
    );

    delta_start_blkno = current_meta->delta_start_blkno;
    nblocks = psql_bm25s_am_relation_nblocks(indexRelation);
    PG_TRY();
    {
        for (blkno = delta_start_blkno; blkno < nblocks; blkno++)
        {
            Buffer buffer;
            Page page;
            psql_bm25s_am_data_page *page_header;
            const char *page_data;
            Size offset;
            Size limit;

            buffer = ReadBufferExtended(
                indexRelation,
                MAIN_FORKNUM,
                blkno,
                RBM_NORMAL,
                NULL
            );
            LockBuffer(buffer, BUFFER_LOCK_SHARE);
            page = BufferGetPage(buffer);
            page_header = (psql_bm25s_am_data_page *) PageGetContents(page);

            if (page_header->magic != PSQL_BM25S_AM_MAGIC ||
                page_header->version != PSQL_BM25S_AM_VERSION)
            {
                UnlockReleaseBuffer(buffer);
                ereport(ERROR, (errmsg("invalid psql_bm25s index data page")));
            }
            if (page_header->page_kind != PSQL_BM25S_AM_PAGE_DELTA)
            {
                UnlockReleaseBuffer(buffer);
                continue;
            }
            if (page_header->used_bytes >
                (uint32) (BLCKSZ - SizeOfPageHeaderData - header_size))
            {
                UnlockReleaseBuffer(buffer);
                ereport(ERROR, (errmsg("corrupt psql_bm25s delta payload")));
            }

            page_data = (const char *) PageGetContents(page);
            offset = header_size;
            limit = header_size + (Size) page_header->used_bytes;
            while (offset < limit)
            {
                psql_bm25s_am_delta_record_header record_header;
                const char *value_ptr;
                Size aligned_value_len;

                if (limit - offset < record_header_size)
                {
                    UnlockReleaseBuffer(buffer);
                    ereport(
                        ERROR,
                        (errmsg("truncated psql_bm25s delta record"))
                    );
                }
                memcpy(
                    &record_header,
                    page_data + offset,
                    sizeof(record_header)
                );
                aligned_value_len = MAXALIGN(
                    (Size) record_header.value_bytes_len
                );
                if (record_header.value_bytes_len == 0 ||
                    limit - offset < record_header_size + aligned_value_len)
                {
                    UnlockReleaseBuffer(buffer);
                    ereport(ERROR, (errmsg("corrupt psql_bm25s delta record")));
                }

                value_ptr = page_data + offset + record_header_size;
                if (record_index >= start_meta->delta_record_count)
                {
                    if (ItemPointerIsValid(&record_header.heap_tid))
                    {
                        uint64 old_doc_id64;
                        uint64 new_doc_id64;
                        uint32_t old_doc_id;
                        uint32_t new_doc_id;

                        old_doc_id64 = (uint64) start_meta->num_docs +
                            (uint64) delta_write_index;
                        if (old_doc_id64 > UINT32_MAX)
                        {
                            UnlockReleaseBuffer(buffer);
                            ereport(
                                ERROR,
                                (errmsg("psql_bm25s delta doc id overflow"))
                            );
                        }
                        old_doc_id = (uint32_t) old_doc_id64;
                        seen_tail_writes++;
                        if (psql_bm25s_am_lookup_doc_id(
                                replacement_tid_to_doc_id,
                                &record_header.heap_tid,
                                &new_doc_id))
                        {
                            psql_bm25s_am_remap_doc_id(
                                tail_doc_id_map,
                                old_doc_id,
                                new_doc_id
                            );
                        }
                        else
                        {
                            new_doc_id64 = (uint64) replacement->num_docs +
                                (uint64) carried_write_index;
                            if (new_doc_id64 > UINT32_MAX)
                            {
                                UnlockReleaseBuffer(buffer);
                                ereport(
                                    ERROR,
                                    (errmsg("psql_bm25s delta doc id overflow"))
                                );
                            }
                            psql_bm25s_am_delta_tail_append(
                                tail_out,
                                &record_header.heap_tid,
                                value_ptr,
                                record_header.value_bytes_len
                            );
                            psql_bm25s_am_remap_doc_id(
                                tail_doc_id_map,
                                old_doc_id,
                                (uint32_t) new_doc_id64
                            );
                            carried_write_index++;
                            carried_writes++;
                        }
                    }
                    else
                    {
                        uint32_t old_doc_id;
                        ItemPointerData old_tid;
                        uint32_t new_doc_id;
                        ItemPointerData invalid_tid;

                        seen_tail_deletes++;
                        if (record_header.value_bytes_len !=
                            sizeof(old_doc_id))
                        {
                            UnlockReleaseBuffer(buffer);
                            ereport(
                                ERROR,
                                (errmsg("corrupt psql_bm25s tombstone record"))
                            );
                        }
                        memcpy(&old_doc_id, value_ptr, sizeof(old_doc_id));
                        if (psql_bm25s_am_lookup_remapped_doc_id(
                                tail_doc_id_map,
                                old_doc_id,
                                &new_doc_id) ||
                            (psql_bm25s_am_active_generation_doc_id_to_tid(
                                indexRelation,
                                start_meta,
                                &all_delta_tids,
                                old_doc_id,
                                &old_tid) &&
                            psql_bm25s_am_lookup_doc_id(
                                replacement_tid_to_doc_id,
                                &old_tid,
                                &new_doc_id)))
                        {
                            ItemPointerSetInvalid(&invalid_tid);
                            psql_bm25s_am_delta_tail_append(
                                tail_out,
                                &invalid_tid,
                                &new_doc_id,
                                (uint32) sizeof(new_doc_id)
                            );
                            carried_deletes++;
                        }
                    }
                }

                if (ItemPointerIsValid(&record_header.heap_tid))
                {
                    delta_write_index++;
                }
                record_index++;
                offset += record_header_size + aligned_value_len;
            }
            UnlockReleaseBuffer(buffer);
        }
    }
    PG_CATCH();
    {
        psql_bm25s_am_delta_tail_free(tail_out);
        hash_destroy(tail_doc_id_map);
        hash_destroy(replacement_tid_to_doc_id);
        psql_bm25s_tid_builder_free(&all_delta_tids);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if (record_index != current_meta->delta_record_count)
    {
        psql_bm25s_am_delta_tail_free(tail_out);
        hash_destroy(tail_doc_id_map);
        hash_destroy(replacement_tid_to_doc_id);
        psql_bm25s_tid_builder_free(&all_delta_tids);
        ereport(ERROR, (errmsg("psql_bm25s delta record count mismatch")));
    }

    tail_out->pending_writes = carried_writes;
    if (tail_pending_writes > seen_tail_writes)
    {
        tail_out->pending_writes = psql_bm25s_am_saturating_add_u32(
            tail_out->pending_writes,
            tail_pending_writes - seen_tail_writes
        );
    }
    tail_out->pending_deletes = carried_deletes;
    if (tail_pending_deletes > seen_tail_deletes)
    {
        tail_out->pending_deletes = psql_bm25s_am_saturating_add_u32(
            tail_out->pending_deletes,
            tail_pending_deletes - seen_tail_deletes
        );
    }

    hash_destroy(tail_doc_id_map);
    hash_destroy(replacement_tid_to_doc_id);
    psql_bm25s_tid_builder_free(&all_delta_tids);
}

static void
psql_bm25s_am_append_delta_tail(
    Relation indexRelation,
    const psql_bm25s_am_delta_tail *tail
)
{
    size_t i;

    if (tail == NULL)
    {
        return;
    }

    for (i = 0; i < tail->len; i++)
    {
        psql_bm25s_am_note_delta_record(
            indexRelation,
            &tail->records[i].heap_tid,
            tail->records[i].value_bytes,
            tail->records[i].value_bytes_len
        );
    }
    if (tail->pending_writes > 0 || tail->pending_deletes > 0)
    {
        psql_bm25s_am_note_maintenance_activity(
            indexRelation,
            tail->pending_writes,
            tail->pending_deletes
        );
    }
}

static text *
psql_bm25s_am_online_maintain_relation(
    Relation indexRelation,
    IndexInfo *indexInfo
)
{
    psql_bm25s_am_meta_page start_meta;
    psql_bm25s_am_meta_page current_meta;
    psql_bm25s_am_payload_health_state start_health;
    psql_bm25s_am_replacement replacement;
    psql_bm25s_am_delta_tail tail;
    Relation heapRelation = NULL;
    Oid heap_oid;
    bool pushed_snapshot = false;
    bool swap_locked = false;
    bool maintenance_locked = false;
    bool swap_replacement = true;
    bool meta_unchanged = false;
    bool tail_carried = false;
    bool publish_tail_as_stale = false;
    bool shared_preload_published = false;
    uint32 tail_pending_writes = 0;
    uint32 tail_pending_deletes = 0;
    psql_bm25s_am_rebuild_builder builder =
        PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD;
    uint16 cache_epoch;
    text *result;

    memset(&replacement, 0, sizeof(replacement));
    memset(&tail, 0, sizeof(tail));
    /*
     * Writers mark transactions that may have uncommitted delta records with a
     * RowExclusive relation lock. Take a short ShareLock before reading
     * start_meta so online maintenance only folds deltas that are visible to the
     * heap snapshot it is about to build from. The lock is released before the
     * heap scan; concurrent writes after start_meta are handled by tail carry.
     */
    if (!ConditionalLockRelation(indexRelation, ShareLock))
    {
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=online)"
        );
    }

    psql_bm25s_am_read_meta(indexRelation, &start_meta);
    psql_bm25s_am_payload_health(indexRelation, &start_meta, &start_health);
    if (!psql_bm25s_am_meta_has_pending_maintenance(&start_meta) &&
        (start_meta.flags & PSQL_BM25S_AM_FLAG_STALE) == 0 &&
        !start_health.rebuild_required)
    {
        UnlockRelation(indexRelation, ShareLock);
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=no_pending, mode=online)"
        );
    }
    if (!psql_bm25s_am_eventual_background_maintenance_due(
            indexRelation,
            &start_meta,
            &start_health))
    {
        UnlockRelation(indexRelation, ShareLock);
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=not_due, mode=online)"
        );
    }
    {
        uint64 standard_estimated_bytes;
        uint64 compact_estimated_bytes;
        uint64 spill_estimated_bytes;
        uint64 budget_bytes;

        if (!psql_bm25s_am_rebuild_memory_budget_choose(
                &start_meta,
                &builder,
                &standard_estimated_bytes,
                &compact_estimated_bytes,
                &spill_estimated_bytes,
                &budget_bytes))
        {
            UnlockRelation(indexRelation, ShareLock);
            return cstring_to_text(psprintf(
                "psql_bm25s_maintenance_result(maintained=false, "
                "reason=memory_budget, mode=online, "
                "standard_estimated_bytes=%llu, "
                "compact_estimated_bytes=%llu, "
                "spill_estimated_bytes=%llu, budget_bytes=%llu)",
                (unsigned long long) standard_estimated_bytes,
                (unsigned long long) compact_estimated_bytes,
                (unsigned long long) spill_estimated_bytes,
                (unsigned long long) budget_bytes
            ));
        }
    }

    PushActiveSnapshot(GetLatestSnapshot());
    pushed_snapshot = true;
    UnlockRelation(indexRelation, ShareLock);

    heap_oid = indexRelation->rd_index->indrelid;
    if (!ConditionalLockRelationOid(heap_oid, AccessShareLock))
    {
        PopActiveSnapshot();
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=online)"
        );
    }
    heapRelation = table_open(heap_oid, NoLock);

    PG_TRY();
    {
        psql_bm25s_am_build_replacement(
            heapRelation,
            indexRelation,
            indexInfo,
            builder,
            true,
            &replacement
        );
    }
    PG_CATCH();
    {
        if (pushed_snapshot)
        {
            PopActiveSnapshot();
        }
        if (heapRelation != NULL)
        {
            table_close(heapRelation, AccessShareLock);
        }
        psql_bm25s_am_replacement_free(&replacement);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if (pushed_snapshot)
    {
        PopActiveSnapshot();
    }
    table_close(heapRelation, AccessShareLock);
    heapRelation = NULL;

    if (!ConditionalLockRelation(indexRelation, ShareUpdateExclusiveLock))
    {
        psql_bm25s_am_replacement_free(&replacement);
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=online)"
        );
    }
    swap_locked = true;

    PG_TRY();
    {
        /*
         * Relation ShareUpdateExclusiveLock does not block concurrent writers.
         * Delta writers serialize through this advisory maintenance lock, so
         * the append-only generation publish and tail carry-forward must use
         * the same lock to avoid appending deltas against an obsolete metapage.
         */
        psql_bm25s_am_lock_maintenance(indexRelation);
        maintenance_locked = true;

        psql_bm25s_am_read_meta(indexRelation, &current_meta);
        meta_unchanged = psql_bm25s_am_meta_equal_for_online_swap(
            &start_meta,
            &current_meta
        );
        if (meta_unchanged)
        {
            /* No tail to preserve. */
            current_meta.pending_write_tuples = 0;
            current_meta.pending_delete_tuples = 0;
        }
        else if (!start_health.corrupt &&
                 psql_bm25s_am_meta_has_compatible_delta_tail(
                     &start_meta,
                     &current_meta))
        {
            psql_bm25s_am_collect_delta_tail(
                indexRelation,
                &start_meta,
                &current_meta,
                &replacement,
                &tail
            );
            /*
             * The replacement was built from a full heap snapshot, so the old
             * pending debt observed at build start has already been absorbed
             * into the new base. The remaining concurrent tail can be mixed:
             * concrete delta records from changed indexed values plus
             * counter-only debt from indexUnchanged updates or oversized
             * eventual values. Preserve concrete records and carry the extra
             * counters as debt. Carrying current_meta counters would requeue
             * the entire historical debt and cause repeated full-table
             * rebuilds under write traffic.
             */
            tail_carried = tail.len > 0 ||
                tail.pending_writes > 0 ||
                tail.pending_deletes > 0;
        }
        else
        {
            /*
             * Concurrent writes observed after the build snapshot may not be
             * safely remapped if the old generation is corrupt. Publish a
             * complete but stale base and let the next worker round catch up.
             */
            publish_tail_as_stale = true;
            if (current_meta.pending_write_tuples >=
                start_meta.pending_write_tuples)
            {
                tail_pending_writes =
                    current_meta.pending_write_tuples -
                    start_meta.pending_write_tuples;
            }
            else
            {
                tail_pending_writes = current_meta.pending_write_tuples;
            }
            if (current_meta.pending_delete_tuples >=
                start_meta.pending_delete_tuples)
            {
                tail_pending_deletes =
                    current_meta.pending_delete_tuples -
                    start_meta.pending_delete_tuples;
            }
            else
            {
                tail_pending_deletes = current_meta.pending_delete_tuples;
            }
        }

        if (swap_replacement)
        {
            uint16 publish_flags = 0;
            uint32 pending_writes = 0;
            uint32 pending_deletes = 0;

            if (!meta_unchanged && !tail_carried)
            {
                if (publish_tail_as_stale)
                {
                    publish_flags = PSQL_BM25S_AM_FLAG_STALE;
                }
                pending_writes = tail_pending_writes;
                pending_deletes = tail_pending_deletes;
            }
            cache_epoch = psql_bm25s_am_next_cache_epoch(indexRelation);
            if (replacement.index_valid)
            {
                psql_bm25s_am_publish_append_only_generation_from_index(
                    indexRelation,
                    replacement.source_type,
                    replacement.doc_tids,
                    replacement.num_docs,
                    &replacement.index,
                    replacement.index_bytes_len,
                    cache_epoch,
                    publish_flags,
                    pending_writes,
                    pending_deletes
                );
            }
            else
            {
                psql_bm25s_am_publish_append_only_generation(
                    indexRelation,
                    replacement.source_type,
                    replacement.doc_tids,
                    replacement.num_docs,
                    replacement.index_bytes,
                    replacement.index_bytes_len,
                    cache_epoch,
                    publish_flags,
                    pending_writes,
                    pending_deletes
                );
            }
            psql_bm25s_am_unschedule_refresh(RelationGetRelid(indexRelation));
            if (tail_carried)
            {
                psql_bm25s_am_append_delta_tail(indexRelation, &tail);
            }
            psql_bm25s_am_read_meta(indexRelation, &current_meta);
            pending_writes = current_meta.pending_write_tuples;
            pending_deletes = current_meta.pending_delete_tuples;
            psql_bm25s_am_unlock_maintenance(indexRelation);
            maintenance_locked = false;
            psql_bm25s_am_shared_preload_retire_obsolete(
                indexRelation,
                &current_meta
            );
            /*
             * The online builder already has the finished immutable index in
             * memory. Publish that same generation into shared-preload now when
             * possible, instead of making the first required shared-cache query
             * cold-read the just-written payload and publish it again.
             */
            if (replacement.index_valid)
            {
                shared_preload_published =
                    psql_bm25s_am_generation_build_preload_from_index(
                        indexRelation,
                        &current_meta,
                        &replacement.index,
                        replacement.doc_tids,
                        replacement.num_docs
                    );
            }
            result = cstring_to_text(psprintf(
                "psql_bm25s_maintenance_result(maintained=true, "
                "reason=rebuilt, mode=online, builder=%s, docs=%zu, "
                "stale=%s, pending_writes=%u, pending_deletes=%u, "
                "tail_carried=%s, tail_records=%zu, "
                "shared_preload_published=%s)",
                builder == PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT
                    ? "compact"
                    : builder == PSQL_BM25S_AM_REBUILD_BUILDER_SPILL
                    ? "spill"
                    : "standard",
                replacement.num_docs,
                (publish_flags & PSQL_BM25S_AM_FLAG_STALE) != 0
                    ? "true"
                    : "false",
                pending_writes,
                pending_deletes,
                tail_carried ? "true" : "false",
                tail.len,
                shared_preload_published ? "true" : "false"
            ));
        }
        if (maintenance_locked)
        {
            psql_bm25s_am_unlock_maintenance(indexRelation);
            maintenance_locked = false;
        }
    }
    PG_CATCH();
    {
        if (maintenance_locked)
        {
            psql_bm25s_am_unlock_maintenance(indexRelation);
        }
        if (swap_locked)
        {
            UnlockRelation(indexRelation, ShareUpdateExclusiveLock);
        }
        psql_bm25s_am_delta_tail_free(&tail);
        psql_bm25s_am_replacement_free(&replacement);
        PG_RE_THROW();
    }
    PG_END_TRY();

    UnlockRelation(indexRelation, ShareUpdateExclusiveLock);
    psql_bm25s_am_delta_tail_free(&tail);
    psql_bm25s_am_replacement_free(&replacement);
    return result;
}

static text *
psql_bm25s_am_try_blocking_maintain_oid(Oid index_oid)
{
    Relation indexRelation;
    IndexInfo *indexInfo = NULL;
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    text *result;

    if (!ConditionalLockRelationOid(index_oid, AccessExclusiveLock))
    {
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=blocking)"
        );
    }

    indexRelation = index_open(index_oid, NoLock);
    PG_TRY();
    {
        psql_bm25s_am_require_index_relation(indexRelation);
        psql_bm25s_am_read_meta(indexRelation, &meta);
        psql_bm25s_am_payload_health(indexRelation, &meta, &health);
        if (!psql_bm25s_am_meta_has_pending_maintenance(&meta) &&
            (meta.flags & PSQL_BM25S_AM_FLAG_STALE) == 0 &&
            !health.rebuild_required)
        {
            result = cstring_to_text(
                "psql_bm25s_maintenance_result(maintained=false, "
                "reason=no_pending, mode=blocking)"
            );
        }
        else
        {
            char *result_str;

            indexInfo = BuildIndexInfo(indexRelation);
            CommandCounterIncrement();
            psql_bm25s_am_reindex_relation(indexRelation, indexInfo);
            psql_bm25s_am_unschedule_refresh(index_oid);
            psql_bm25s_am_read_meta(indexRelation, &meta);
            psql_bm25s_am_shared_preload_retire_obsolete(
                indexRelation,
                &meta
            );
            result_str = psprintf(
                "psql_bm25s_maintenance_result(maintained=true, "
                "reason=rebuilt, mode=blocking, docs=%u)",
                meta.num_docs
            );
            result = cstring_to_text(result_str);
        }
    }
    PG_CATCH();
    {
        if (indexInfo != NULL)
        {
            pfree(indexInfo);
        }
        index_close(indexRelation, AccessExclusiveLock);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if (indexInfo != NULL)
    {
        pfree(indexInfo);
    }
    index_close(indexRelation, AccessExclusiveLock);
    return result;
}

static void
psql_bm25s_am_flush_pending_refresh_oid(Oid index_oid)
{
    if (!psql_bm25s_am_has_pending_refresh(index_oid) ||
        psql_bm25s_am_flush_running)
    {
        return;
    }
    if (psql_bm25s_am_should_defer_foreground_refresh_oid(index_oid))
    {
        psql_bm25s_am_unschedule_refresh(index_oid);
        return;
    }

    psql_bm25s_am_flush_running = true;
    PG_TRY();
    {
        psql_bm25s_am_rebuild_index_oid(index_oid);
        psql_bm25s_am_unschedule_refresh(index_oid);
        psql_bm25s_am_flush_running = false;
    }
    PG_CATCH();
    {
        psql_bm25s_am_flush_running = false;
        PG_RE_THROW();
    }
    PG_END_TRY();
}

static bool
psql_bm25s_am_should_defer_foreground_refresh_oid(Oid index_oid)
{
    Relation indexRelation;
    bool defer_refresh;

    if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(index_oid)))
    {
        return false;
    }

    indexRelation = index_open(index_oid, AccessShareLock);
    if (indexRelation->rd_rel->relkind != RELKIND_INDEX ||
        indexRelation->rd_rel->relam != get_am_oid("psql_bm25s", false))
    {
        index_close(indexRelation, AccessShareLock);
        return false;
    }

    defer_refresh =
        psql_bm25s_am_eventual_policy_enabled(indexRelation) &&
        !psql_bm25s_am_foreground_maintenance_enabled(indexRelation);
    index_close(indexRelation, AccessShareLock);
    return defer_refresh;
}

static bool
psql_bm25s_am_should_flush_scheduled_oid(Oid index_oid)
{
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    int threshold;
    int delta_bytes_threshold;
    double churn_ratio_threshold;
    uint64 pending_total;
    double churn_ratio;

    if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(index_oid)))
    {
        return false;
    }
    if (psql_bm25s_am_should_defer_foreground_refresh_oid(index_oid))
    {
        return false;
    }

    indexRelation = index_open(index_oid, AccessShareLock);
    if (indexRelation->rd_rel->relkind != RELKIND_INDEX ||
        indexRelation->rd_rel->relam != get_am_oid("psql_bm25s", false))
    {
        index_close(indexRelation, AccessShareLock);
        return false;
    }

    threshold = psql_bm25s_am_rebuild_threshold(indexRelation);
    delta_bytes_threshold =
        psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation);
    churn_ratio_threshold =
        psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation);
    psql_bm25s_am_read_meta(indexRelation, &meta);
    index_close(indexRelation, AccessShareLock);

    if ((meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0)
    {
        return true;
    }

    if (threshold <= 0 && delta_bytes_threshold <= 0 &&
        churn_ratio_threshold <= 0.0)
    {
        return true;
    }

    pending_total = (uint64) meta.pending_write_tuples +
        (uint64) meta.pending_delete_tuples;
    if (threshold > 0 && pending_total >= (uint64) threshold)
    {
        return true;
    }
    if (delta_bytes_threshold > 0 &&
        meta.delta_bytes_len >= (uint64) delta_bytes_threshold)
    {
        return true;
    }
    if (churn_ratio_threshold > 0.0)
    {
        if (meta.num_docs == 0)
        {
            return true;
        }
        churn_ratio = (double) pending_total / (double) meta.num_docs;
        if (churn_ratio >= churn_ratio_threshold)
        {
            return true;
        }
    }
    return false;
}

static void
psql_bm25s_am_load_delta_state(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *snapshot_meta,
    Oid source_type,
    uint32 expected_delta_records,
    psql_bm25s_doc_ids_builder *id_docs_out,
    psql_bm25s_doc_tokens_builder *token_docs_out,
    psql_bm25s_tid_builder *tids_out,
    psql_bm25s_docid_builder *deleted_doc_ids_out
)
{
    BlockNumber blkno;
    BlockNumber nblocks;
    BlockNumber delta_start_blkno;
    bool delta_uses_fused_array;
    const Size header_size = MAXALIGN(sizeof(psql_bm25s_am_data_page));
    const Size record_header_size =
        MAXALIGN(sizeof(psql_bm25s_am_delta_record_header));
    psql_bm25s_text_options text_options;
    char **text_stopwords = NULL;
    size_t text_stopword_len = 0;
    psql_bm25s_am_meta_page meta;
    uint32 record_index = 0;
    uint32 record_limit;

    delta_uses_fused_array =
        psql_bm25s_am_is_multicol_index(indexRelation) &&
        psql_bm25s_am_source_type_is_scalar_text(source_type);
    psql_bm25s_text_options_init(&text_options);
    if (token_docs_out != NULL &&
        psql_bm25s_am_source_type_is_scalar_text(source_type))
    {
        psql_bm25s_am_read_index_text_options(
            indexRelation,
            &text_options,
            &text_stopwords,
            &text_stopword_len
        );
    }

    if (snapshot_meta != NULL)
    {
        meta = *snapshot_meta;
    }
    else
    {
        psql_bm25s_am_read_meta(indexRelation, &meta);
    }
    if (!psql_bm25s_am_meta_uses_append_only(&meta))
    {
        ereport(
            ERROR,
            (
                errmsg("unsupported psql_bm25s index storage layout"),
                errhint("Rebuild the index to use append-only generations.")
            )
        );
    }
    record_limit = expected_delta_records;
    if (record_limit == 0)
    {
        psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
        return;
    }
    if (record_limit != meta.delta_record_count)
    {
        psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s delta record count mismatch: expected %u "
                    "records, metapage has %u records",
                    record_limit,
                    meta.delta_record_count
                )
            )
        );
    }
    delta_start_blkno = meta.delta_start_blkno;
    nblocks = psql_bm25s_am_relation_nblocks(indexRelation);
    if (!BlockNumberIsValid(delta_start_blkno) ||
        delta_start_blkno >= nblocks)
    {
        psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s delta record count mismatch: expected %u "
                    "records, found 0 records (blocks=%u)",
                    record_limit,
                    (unsigned int) nblocks
                )
            )
        );
    }
    for (blkno = delta_start_blkno; blkno < nblocks; blkno++)
    {
        Buffer buffer;
        Page page;
        psql_bm25s_am_data_page *page_header;
        const char *page_data;
        Size offset;
        Size limit;

        if ((blkno & 0x7f) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }

        buffer = ReadBufferExtended(
            indexRelation,
            MAIN_FORKNUM,
            blkno,
            RBM_NORMAL,
            NULL
        );
        LockBuffer(buffer, BUFFER_LOCK_SHARE);
        page = BufferGetPage(buffer);
        page_header = (psql_bm25s_am_data_page *) PageGetContents(page);

        if (page_header->magic != PSQL_BM25S_AM_MAGIC ||
            page_header->version != PSQL_BM25S_AM_VERSION)
        {
            UnlockReleaseBuffer(buffer);
            ereport(ERROR, (errmsg("invalid psql_bm25s index data page")));
        }
        if (page_header->page_kind != PSQL_BM25S_AM_PAGE_DELTA)
        {
            UnlockReleaseBuffer(buffer);
            continue;
        }

        if (page_header->used_bytes >
            (uint32) (BLCKSZ - SizeOfPageHeaderData - header_size))
        {
            UnlockReleaseBuffer(buffer);
            ereport(ERROR, (errmsg("corrupt psql_bm25s delta payload")));
        }

        page_data = (const char *) PageGetContents(page);
        offset = header_size;
        limit = header_size + (Size) page_header->used_bytes;
        while (offset < limit && record_index < record_limit)
        {
            psql_bm25s_am_delta_record_header record_header;
            const char *value_ptr;
            Size aligned_value_len;
            ArrayType *array;

            if (limit - offset < record_header_size)
            {
                UnlockReleaseBuffer(buffer);
                ereport(ERROR, (errmsg("truncated psql_bm25s delta record")));
            }

            memcpy(
                &record_header,
                page_data + offset,
                sizeof(record_header)
            );
            aligned_value_len = MAXALIGN((Size) record_header.value_bytes_len);
            if (record_header.value_bytes_len == 0 ||
                limit - offset < record_header_size + aligned_value_len)
            {
                UnlockReleaseBuffer(buffer);
                ereport(ERROR, (errmsg("corrupt psql_bm25s delta record")));
            }

            value_ptr = page_data + offset + record_header_size;
            if (ItemPointerIsValid(&record_header.heap_tid))
            {
                if (source_type == INT4ARRAYOID ||
                    psql_bm25s_am_source_type_is_text_array(source_type) ||
                    delta_uses_fused_array)
                {
                    array = (ArrayType *) value_ptr;
                    if ((uint32) VARSIZE_ANY(array) !=
                        record_header.value_bytes_len)
                    {
                        UnlockReleaseBuffer(buffer);
                        ereport(
                            ERROR,
                            (errmsg("corrupt psql_bm25s delta array"))
                        );
                    }

                    if (tids_out != NULL)
                    {
                        psql_bm25s_am_append_tid(
                            tids_out,
                            &record_header.heap_tid
                        );
                    }
                    if (source_type == INT4ARRAYOID)
                    {
                        if (id_docs_out != NULL)
                        {
                            psql_bm25s_am_append_id_doc(id_docs_out, array);
                        }
                    }
                    else if (token_docs_out != NULL)
                    {
                        psql_bm25s_am_append_token_doc(token_docs_out, array);
                    }
                }
                else
                {
                    psql_bm25s_doc_tokens doc = {0};

                    if (tids_out != NULL)
                    {
                        psql_bm25s_am_append_tid(
                            tids_out,
                            &record_header.heap_tid
                        );
                    }
                    if (token_docs_out != NULL)
                    {
                        psql_bm25s_am_tokenize_scalar_datum(
                            PointerGetDatum((struct varlena *) value_ptr),
                            &text_options,
                            &doc
                        );
                        psql_bm25s_am_append_owned_token_doc(
                            token_docs_out,
                            &doc
                        );
                    }
                }
            }
            else
            {
                uint32_t deleted_doc_id;

                if (record_header.value_bytes_len != sizeof(deleted_doc_id))
                {
                    UnlockReleaseBuffer(buffer);
                    ereport(ERROR, (errmsg("corrupt psql_bm25s tombstone record")));
                }
                memcpy(&deleted_doc_id, value_ptr, sizeof(deleted_doc_id));
                if (deleted_doc_ids_out != NULL)
                {
                    psql_bm25s_am_append_docid(
                        deleted_doc_ids_out,
                        deleted_doc_id
                    );
                }
            }

            offset += record_header_size + aligned_value_len;
            record_index++;
        }
        UnlockReleaseBuffer(buffer);
        if (record_index >= record_limit)
        {
            break;
        }
    }
    if (record_index != record_limit)
    {
        psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s delta record count mismatch: expected %u "
                    "records, found %u records (delta_start_block=%u, "
                    "blocks=%u)",
                    record_limit,
                    record_index,
                    (unsigned int) delta_start_blkno,
                    (unsigned int) nblocks
                )
            )
        );
    }

    psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
}

static void
psql_bm25s_am_reconstruct_ids_from_index(
    const psql_bm25s_index *index,
    psql_bm25s_doc_ids_builder *docs_out
)
{
    size_t *offsets;
    size_t token_id;
    size_t i;

    docs_out->len = index->num_docs;
    docs_out->capacity = index->num_docs;
    if (index->num_docs > 0)
    {
        docs_out->docs = palloc0(sizeof(*docs_out->docs) * index->num_docs);
    }
    offsets = palloc0(sizeof(*offsets) * Max((size_t) index->num_docs, (size_t) 1));

    for (i = 0; i < index->num_docs; i++)
    {
        size_t doc_len = (size_t) index->doc_lengths[i];

        docs_out->docs[i].len = doc_len;
        if (doc_len > 0)
        {
            docs_out->docs[i].token_ids = palloc(sizeof(uint32_t) * doc_len);
        }
    }

    for (token_id = 0; token_id < index->vocab_size; token_id++)
    {
        uint64_t start = index->indptr[token_id];
        uint64_t end = index->indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = index->indices[j];
            uint32_t tf = index->term_frequencies[j];
            uint32_t repeat;

            for (repeat = 0; repeat < tf; repeat++)
            {
                docs_out->docs[doc_id].token_ids[offsets[doc_id]++] =
                    (uint32_t) token_id;
            }
        }
    }

    pfree(offsets);
}

static void
psql_bm25s_am_reconstruct_tokens_from_index(
    const psql_bm25s_index *index,
    psql_bm25s_doc_tokens_builder *docs_out
)
{
    size_t *offsets;
    size_t token_id;
    size_t i;

    docs_out->len = index->num_docs;
    docs_out->capacity = index->num_docs;
    if (index->num_docs > 0)
    {
        docs_out->docs = palloc0(sizeof(*docs_out->docs) * index->num_docs);
    }
    offsets = palloc0(sizeof(*offsets) * Max((size_t) index->num_docs, (size_t) 1));

    for (i = 0; i < index->num_docs; i++)
    {
        size_t doc_len = (size_t) index->doc_lengths[i];

        docs_out->docs[i].len = doc_len;
        if (doc_len > 0)
        {
            docs_out->docs[i].tokens =
                palloc(sizeof(*docs_out->docs[i].tokens) * doc_len);
        }
    }

    for (token_id = 0; token_id < index->vocab_size; token_id++)
    {
        uint64_t start = index->indptr[token_id];
        uint64_t end = index->indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = index->indices[j];
            uint32_t tf = index->term_frequencies[j];
            uint32_t repeat;

            for (repeat = 0; repeat < tf; repeat++)
            {
                docs_out->docs[doc_id].tokens[offsets[doc_id]++] = pstrdup(
                    index->vocab[token_id]
                );
            }
        }
    }

    pfree(offsets);
}

static bool
psql_bm25s_am_can_use_delta_overlay(const psql_bm25s_am_meta_page *meta)
{
    uint64 pending_total;

    if (meta == NULL)
    {
        return false;
    }

    pending_total = (uint64) meta->pending_write_tuples +
        (uint64) meta->pending_delete_tuples;
    return pending_total > 0 &&
        psql_bm25s_am_meta_uses_append_only(meta) &&
        meta->delta_record_count > 0 &&
        (uint64) meta->delta_record_count <= pending_total &&
        (meta->flags & PSQL_BM25S_AM_FLAG_STALE) == 0;
}

static bool
psql_bm25s_am_delta_overlay_entry_matches(
    const psql_bm25s_am_cache_entry *delta_entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    const psql_bm25s_am_meta_page *cached_meta;

    if (delta_entry == NULL || delta_entry->mcxt == NULL || meta == NULL)
    {
        return false;
    }
    if (!RelFileLocatorEquals(
            delta_entry->generation.locator,
            indexRelation->rd_locator))
    {
        return false;
    }

    cached_meta = &delta_entry->generation.meta;
    return cached_meta->source_type == meta->source_type &&
        cached_meta->cache_epoch == meta->cache_epoch &&
        cached_meta->rebuild_count == meta->rebuild_count &&
        cached_meta->flags == meta->flags &&
        cached_meta->delta_record_count == meta->delta_record_count &&
        cached_meta->delta_bytes_len == meta->delta_bytes_len &&
        cached_meta->pending_write_tuples == meta->pending_write_tuples &&
        cached_meta->pending_delete_tuples == meta->pending_delete_tuples;
}

static psql_bm25s_am_cache_entry *
psql_bm25s_am_get_delta_overlay_entry(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    psql_bm25s_doc_tokens_builder delta_docs = {0};
    psql_bm25s_tid_builder delta_tids = {0};
    psql_bm25s_docid_builder deleted_doc_ids = {0};
    psql_bm25s_am_cache_entry *delta_entry;
    psql_bm25s_status status;
    MemoryContext oldcontext;

    if (entry == NULL || meta == NULL ||
        !psql_bm25s_am_source_type_is_textlike(meta->source_type))
    {
        return NULL;
    }
    if (psql_bm25s_am_delta_overlay_entry_matches(
            entry->delta_overlay,
            indexRelation,
            meta))
    {
        return entry->delta_overlay;
    }

    if (entry->delta_overlay != NULL)
    {
        psql_bm25s_am_cache_entry_reset(entry->delta_overlay);
    }
    else
    {
        oldcontext = MemoryContextSwitchTo(entry->mcxt);
        entry->delta_overlay = palloc0(sizeof(*entry->delta_overlay));
        MemoryContextSwitchTo(oldcontext);
    }

    psql_bm25s_am_load_delta_state(
        indexRelation,
        meta,
        meta->source_type,
        meta->delta_record_count,
        NULL,
        &delta_docs,
        &delta_tids,
        &deleted_doc_ids
    );
    if (delta_tids.len + deleted_doc_ids.len != meta->delta_record_count)
    {
        psql_bm25s_tid_builder_free(&delta_tids);
        psql_bm25s_docid_builder_free(&deleted_doc_ids);
        psql_bm25s_builder_tokens_free(&delta_docs);
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s delta record count mismatch: expected %u "
                    "records, loaded %zu records",
                    meta->delta_record_count,
                    delta_tids.len + deleted_doc_ids.len
                )
            )
        );
    }

    delta_entry = entry->delta_overlay;
    memset(delta_entry, 0, sizeof(*delta_entry));
    delta_entry->index_oid = RelationGetRelid(indexRelation);
    delta_entry->mcxt = AllocSetContextCreate(
        entry->mcxt,
        "psql_bm25s cached delta overlay",
        ALLOCSET_START_SMALL_SIZES
    );
    delta_entry->generation.locator = indexRelation->rd_locator;
    delta_entry->generation.meta = *meta;
    delta_entry->generation.delta_overlay_materialized = true;

    oldcontext = MemoryContextSwitchTo(delta_entry->mcxt);
    if (delta_tids.len > 0)
    {
        delta_entry->generation.doc_tids = palloc(
            sizeof(*delta_entry->generation.doc_tids) * delta_tids.len
        );
        memcpy(
            delta_entry->generation.doc_tids,
            delta_tids.tids,
            sizeof(*delta_entry->generation.doc_tids) * delta_tids.len
        );
    }
    MemoryContextSwitchTo(oldcontext);

    psql_bm25s_index_init(&delta_entry->generation.index);
    if (delta_docs.len == 0)
    {
        psql_bm25s_tid_builder_free(&delta_tids);
        psql_bm25s_docid_builder_free(&deleted_doc_ids);
        psql_bm25s_builder_tokens_free(&delta_docs);
        return delta_entry;
    }

    status = psql_bm25s_build_index_from_tokens(
        delta_docs.docs,
        delta_docs.len,
        &entry->generation.index.params,
        &delta_entry->generation.index
    );
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_index_free(&delta_entry->generation.index);
        MemoryContextDelete(delta_entry->mcxt);
        delta_entry->mcxt = NULL;
        psql_bm25s_tid_builder_free(&delta_tids);
        psql_bm25s_docid_builder_free(&deleted_doc_ids);
        psql_bm25s_builder_tokens_free(&delta_docs);
        ereport(ERROR, (errmsg("failed to build delta bm25 overlay: %s",
                               psql_bm25s_strerror(status))));
    }

    delta_entry->generation.index_owns_allocations = true;
    psql_bm25s_am_cache_build_sorted_vocab(delta_entry);

    psql_bm25s_tid_builder_free(&delta_tids);
    psql_bm25s_docid_builder_free(&deleted_doc_ids);
    psql_bm25s_builder_tokens_free(&delta_docs);
    return delta_entry;
}

static void
psql_bm25s_am_cache_load_overlay(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    bool create_empty_token = true;
    psql_bm25s_am_payload payload = {0};
    psql_bm25s_index base_index;
    psql_bm25s_index overlay_index;
    psql_bm25s_status status;
    psql_bm25s_doc_ids_builder base_id_docs = {0};
    psql_bm25s_doc_ids_builder delta_id_docs = {0};
    psql_bm25s_doc_tokens_builder base_token_docs = {0};
    psql_bm25s_doc_tokens_builder delta_token_docs = {0};
    psql_bm25s_doc_tokens_builder overlay_token_docs = {0};
    psql_bm25s_doc_ids_builder overlay_id_docs = {0};
    psql_bm25s_tid_builder delta_tids = {0};
    psql_bm25s_tid_builder overlay_tids = {0};
    psql_bm25s_docid_builder deleted_doc_ids = {0};
    bool *deleted_docs = NULL;
    MemoryContext oldcontext;
    size_t i;

    psql_bm25s_index_init(&base_index);
    psql_bm25s_index_init(&overlay_index);

    psql_bm25s_am_read_params(
        indexRelation,
        &base_index.params,
        &create_empty_token
    );

    psql_bm25s_am_load_payload(indexRelation, false, &payload);
    status = psql_bm25s_deserialize_index(
        payload.index_bytes,
        payload.index_bytes_len,
        &base_index
    );
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_am_payload_free(&payload);
        ereport(ERROR, (errmsg("failed to deserialize bm25 index: %s",
                               psql_bm25s_strerror(status))));
    }

    psql_bm25s_am_load_delta_state(
        indexRelation,
        meta,
        meta->source_type,
        meta->delta_record_count,
        &delta_id_docs,
        &delta_token_docs,
        &delta_tids,
        &deleted_doc_ids
    );
    if (delta_tids.len + deleted_doc_ids.len != meta->delta_record_count)
    {
        psql_bm25s_index_free(&base_index);
        psql_bm25s_tid_builder_free(&delta_tids);
        psql_bm25s_docid_builder_free(&deleted_doc_ids);
        psql_bm25s_builder_ids_free(&delta_id_docs);
        psql_bm25s_builder_tokens_free(&delta_token_docs);
        psql_bm25s_am_payload_free(&payload);
        ereport(ERROR, (errmsg("psql_bm25s delta record count mismatch")));
    }

    if (payload.meta.num_docs + delta_tids.len > 0)
    {
        deleted_docs = palloc0(
            sizeof(*deleted_docs) * (payload.meta.num_docs + delta_tids.len)
        );
    }
    for (i = 0; i < deleted_doc_ids.len; i++)
    {
        if ((size_t) deleted_doc_ids.doc_ids[i] >= payload.meta.num_docs + delta_tids.len)
        {
            psql_bm25s_index_free(&base_index);
            psql_bm25s_tid_builder_free(&delta_tids);
            psql_bm25s_tid_builder_free(&overlay_tids);
            psql_bm25s_docid_builder_free(&deleted_doc_ids);
            psql_bm25s_builder_ids_free(&base_id_docs);
            psql_bm25s_builder_ids_free(&delta_id_docs);
            psql_bm25s_builder_ids_free(&overlay_id_docs);
            psql_bm25s_builder_tokens_free(&base_token_docs);
            psql_bm25s_builder_tokens_free(&delta_token_docs);
            psql_bm25s_builder_tokens_free(&overlay_token_docs);
            psql_bm25s_am_payload_free(&payload);
            if (deleted_docs != NULL)
            {
                pfree(deleted_docs);
            }
            ereport(ERROR, (errmsg("psql_bm25s tombstone doc id out of range")));
        }
        deleted_docs[deleted_doc_ids.doc_ids[i]] = true;
    }

    if (deleted_docs != NULL)
    {
        psql_bm25s_am_mark_replaced_docs(
            indexRelation,
            &payload,
            &delta_tids,
            deleted_docs,
            payload.meta.num_docs + delta_tids.len
        );
    }

    if (meta->source_type == INT4ARRAYOID)
    {
        psql_bm25s_am_reconstruct_ids_from_index(&base_index, &base_id_docs);
        for (i = 0; i < base_id_docs.len; i++)
        {
            if (deleted_docs == NULL || !deleted_docs[i])
            {
                psql_bm25s_am_append_owned_id_doc(
                    &overlay_id_docs,
                    &base_id_docs.docs[i]
                );
            }
            else
            {
                psql_bm25s_am_free_id_doc(&base_id_docs.docs[i]);
            }
        }
        for (i = 0; i < delta_id_docs.len; i++)
        {
            size_t doc_id = payload.meta.num_docs + i;

            if (deleted_docs == NULL || !deleted_docs[doc_id])
            {
                psql_bm25s_am_append_owned_id_doc(
                    &overlay_id_docs,
                    &delta_id_docs.docs[i]
                );
            }
            else
            {
                psql_bm25s_am_free_id_doc(&delta_id_docs.docs[i]);
            }
        }
        for (i = 0; i < payload.meta.num_docs; i++)
        {
            if (deleted_docs == NULL || !deleted_docs[i])
            {
                psql_bm25s_am_append_tid(&overlay_tids, &payload.doc_tids[i]);
            }
        }
        for (i = 0; i < delta_tids.len; i++)
        {
            size_t doc_id = payload.meta.num_docs + i;

            if (deleted_docs == NULL || !deleted_docs[doc_id])
            {
                psql_bm25s_am_append_tid(&overlay_tids, &delta_tids.tids[i]);
            }
        }
        status = psql_bm25s_build_index_from_ids(
            overlay_id_docs.docs,
            overlay_id_docs.len,
            &base_index.params,
            create_empty_token,
            &overlay_index
        );
    }
    else
    {
        psql_bm25s_am_reconstruct_tokens_from_index(
            &base_index,
            &base_token_docs
        );
        for (i = 0; i < base_token_docs.len; i++)
        {
            if (deleted_docs == NULL || !deleted_docs[i])
            {
                psql_bm25s_am_append_owned_token_doc(
                    &overlay_token_docs,
                    &base_token_docs.docs[i]
                );
            }
            else
            {
                psql_bm25s_am_free_token_doc(&base_token_docs.docs[i]);
            }
        }
        for (i = 0; i < delta_token_docs.len; i++)
        {
            size_t doc_id = payload.meta.num_docs + i;

            if (deleted_docs == NULL || !deleted_docs[doc_id])
            {
                psql_bm25s_am_append_owned_token_doc(
                    &overlay_token_docs,
                    &delta_token_docs.docs[i]
                );
            }
            else
            {
                psql_bm25s_am_free_token_doc(&delta_token_docs.docs[i]);
            }
        }
        for (i = 0; i < payload.meta.num_docs; i++)
        {
            if (deleted_docs == NULL || !deleted_docs[i])
            {
                psql_bm25s_am_append_tid(&overlay_tids, &payload.doc_tids[i]);
            }
        }
        for (i = 0; i < delta_tids.len; i++)
        {
            size_t doc_id = payload.meta.num_docs + i;

            if (deleted_docs == NULL || !deleted_docs[doc_id])
            {
                psql_bm25s_am_append_tid(&overlay_tids, &delta_tids.tids[i]);
            }
        }
        status = psql_bm25s_build_index_from_tokens(
            overlay_token_docs.docs,
            overlay_token_docs.len,
            &base_index.params,
            &overlay_index
        );
    }

    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_index_free(&base_index);
        psql_bm25s_index_free(&overlay_index);
        psql_bm25s_tid_builder_free(&delta_tids);
        psql_bm25s_tid_builder_free(&overlay_tids);
        psql_bm25s_docid_builder_free(&deleted_doc_ids);
        psql_bm25s_builder_ids_free(&base_id_docs);
        psql_bm25s_builder_ids_free(&delta_id_docs);
        psql_bm25s_builder_ids_free(&overlay_id_docs);
        psql_bm25s_builder_tokens_free(&base_token_docs);
        psql_bm25s_builder_tokens_free(&delta_token_docs);
        psql_bm25s_builder_tokens_free(&overlay_token_docs);
        psql_bm25s_am_payload_free(&payload);
        if (deleted_docs != NULL)
        {
            pfree(deleted_docs);
        }
        ereport(ERROR, (errmsg("failed to build bm25 overlay: %s",
                               psql_bm25s_strerror(status))));
    }

    entry->mcxt = AllocSetContextCreate(
        psql_bm25s_am_cache.mcxt,
        "psql_bm25s cache entry",
        ALLOCSET_START_SMALL_SIZES
    );
    oldcontext = MemoryContextSwitchTo(entry->mcxt);
    if (overlay_tids.len > 0)
    {
        entry->generation.doc_tids = palloc(sizeof(*entry->generation.doc_tids) * overlay_tids.len);
        memcpy(
            entry->generation.doc_tids,
            overlay_tids.tids,
            sizeof(*entry->generation.doc_tids) * overlay_tids.len
        );
    }
    MemoryContextSwitchTo(oldcontext);

    entry->generation.locator = indexRelation->rd_locator;
    entry->generation.meta = *meta;
    entry->generation.index = overlay_index;
    entry->generation.delta_overlay_materialized = true;
    entry->generation.index_owns_allocations = true;
    psql_bm25s_am_cache_build_sorted_vocab(entry);

    psql_bm25s_index_free(&base_index);
    psql_bm25s_tid_builder_free(&delta_tids);
    psql_bm25s_tid_builder_free(&overlay_tids);
    psql_bm25s_docid_builder_free(&deleted_doc_ids);
    psql_bm25s_builder_ids_free(&base_id_docs);
    psql_bm25s_builder_ids_free(&delta_id_docs);
    psql_bm25s_builder_ids_free(&overlay_id_docs);
    psql_bm25s_builder_tokens_free(&base_token_docs);
    psql_bm25s_builder_tokens_free(&delta_token_docs);
    psql_bm25s_builder_tokens_free(&overlay_token_docs);
    psql_bm25s_am_payload_free(&payload);
    if (deleted_docs != NULL)
    {
        pfree(deleted_docs);
    }
}

static void
psql_bm25s_am_mark_replaced_docs(
    Relation indexRelation,
    const psql_bm25s_am_payload *payload,
    const psql_bm25s_tid_builder *delta_tids,
    bool *deleted_docs,
    size_t total_docs
)
{
    HASHCTL hash_ctl;
    HTAB *tid_to_doc_id;
    Relation heap_relation;
    TableScanDesc scan = NULL;
    Oid heap_oid;
    size_t i;

    if (payload == NULL || deleted_docs == NULL || total_docs == 0)
    {
        return;
    }

    memset(&hash_ctl, 0, sizeof(hash_ctl));
    hash_ctl.keysize = sizeof(ItemPointerData);
    hash_ctl.entrysize = sizeof(psql_bm25s_am_tid_doc_map_entry);
    tid_to_doc_id = hash_create(
        "psql_bm25s delta overlay tids",
        (long) Max(total_docs, (size_t) 16),
        &hash_ctl,
        HASH_ELEM | HASH_BLOBS
    );

    for (i = 0; i < payload->meta.num_docs; i++)
    {
        psql_bm25s_am_tid_doc_map_entry *entry;

        if (!ItemPointerIsValid(&payload->doc_tids[i]))
        {
            continue;
        }

        entry = hash_search(
            tid_to_doc_id,
            &payload->doc_tids[i],
            HASH_ENTER,
            NULL
        );
        entry->doc_id = (uint32_t) i;
    }
    for (i = 0; i < delta_tids->len; i++)
    {
        psql_bm25s_am_tid_doc_map_entry *entry;
        size_t doc_id = payload->meta.num_docs + i;

        if (!ItemPointerIsValid(&delta_tids->tids[i]))
        {
            continue;
        }

        entry = hash_search(
            tid_to_doc_id,
            &delta_tids->tids[i],
            HASH_ENTER,
            NULL
        );
        entry->doc_id = (uint32_t) doc_id;
    }

    heap_oid = IndexGetRelation(RelationGetRelid(indexRelation), false);
    heap_relation = table_open(heap_oid, AccessShareLock);

    PG_TRY();
    {
        scan = table_beginscan(heap_relation, SnapshotAny, 0, NULL);

        for (i = 0; i < total_docs; i++)
        {
            ItemPointerData current_tid;
            ItemPointerData latest_tid;
            psql_bm25s_am_tid_doc_map_entry *target;

            if (deleted_docs[i])
            {
                continue;
            }

            if (i < payload->meta.num_docs)
            {
                current_tid = payload->doc_tids[i];
            }
            else
            {
                current_tid = delta_tids->tids[i - payload->meta.num_docs];
            }

            if (!ItemPointerIsValid(&current_tid) ||
                !table_tuple_tid_valid(scan, &current_tid))
            {
                continue;
            }

            latest_tid = current_tid;
            table_tuple_get_latest_tid(scan, &latest_tid);
            if (ItemPointerEquals(&latest_tid, &current_tid))
            {
                continue;
            }

            target = hash_search(tid_to_doc_id, &latest_tid, HASH_FIND, NULL);
            if (target != NULL && target->doc_id != (uint32_t) i)
            {
                deleted_docs[i] = true;
            }
        }

        table_endscan(scan);
        scan = NULL;
        table_close(heap_relation, AccessShareLock);
    }
    PG_CATCH();
    {
        if (scan != NULL)
        {
            table_endscan(scan);
        }
        table_close(heap_relation, AccessShareLock);
        hash_destroy(tid_to_doc_id);
        PG_RE_THROW();
    }
    PG_END_TRY();

    hash_destroy(tid_to_doc_id);
}

static void
psql_bm25s_am_xact_callback(XactEvent event, void *arg)
{
    (void) arg;

    switch (event)
    {
        case XACT_EVENT_PRE_COMMIT:
        case XACT_EVENT_PRE_PREPARE:
        {
            List *pending_oids;
            ListCell *cell;

            psql_bm25s_am_flush_pending_maintenance_activity();
            pending_oids = list_copy(psql_bm25s_am_pending_refresh_oids);
            foreach (cell, pending_oids)
            {
                Oid index_oid = lfirst_oid(cell);

                if (psql_bm25s_am_should_flush_scheduled_oid(index_oid))
                {
                    psql_bm25s_am_flush_pending_refresh_oid(index_oid);
                }
                else
                {
                    psql_bm25s_am_unschedule_refresh(index_oid);
                }
            }
            list_free(pending_oids);
            break;
        }
        case XACT_EVENT_COMMIT:
        case XACT_EVENT_PARALLEL_COMMIT:
            psql_bm25s_am_cache_release_shared_preload_leases();
            if (psql_bm25s_am_pending_background_maintenance)
            {
                psql_bm25s_am_touch_background_maintenance();
            }
            psql_bm25s_am_clear_pending_background_maintenance();
            psql_bm25s_am_clear_pending_maintenance_activity();
            psql_bm25s_am_clear_pending_refreshes();
            psql_bm25s_am_clear_pinned_maintenances();
            break;
        case XACT_EVENT_ABORT:
        case XACT_EVENT_PARALLEL_ABORT:
        case XACT_EVENT_PREPARE:
            psql_bm25s_am_cache_release_shared_preload_leases();
            psql_bm25s_am_clear_pending_background_maintenance();
            psql_bm25s_am_clear_pending_maintenance_activity();
            psql_bm25s_am_clear_pending_refreshes();
            psql_bm25s_am_clear_pinned_maintenances();
            break;
        default:
            break;
    }
}

static bool
psql_bm25s_am_meta_uses_append_only(
    const psql_bm25s_am_meta_page *meta
)
{
    return meta != NULL &&
        meta->storage_version == PSQL_BM25S_AM_STORAGE_APPEND_ONLY &&
        meta->active_start_blkno > 0;
}

static void
psql_bm25s_am_normalize_meta_storage(
    psql_bm25s_am_meta_page *meta,
    BlockNumber nblocks
)
{
    uint64 end_blkno;

    if (meta == NULL)
    {
        return;
    }
    if (meta->storage_version != PSQL_BM25S_AM_STORAGE_APPEND_ONLY)
    {
        meta->flags |= PSQL_BM25S_AM_FLAG_REBUILD_REQUIRED;
        return;
    }
    end_blkno = (uint64) meta->active_start_blkno +
        (uint64) meta->active_data_pages;
    if (meta->active_start_blkno == 0 ||
        end_blkno > (uint64) nblocks ||
        meta->delta_start_blkno == 0 ||
        meta->delta_start_blkno > nblocks)
    {
        meta->flags |= PSQL_BM25S_AM_FLAG_CORRUPT |
            PSQL_BM25S_AM_FLAG_REBUILD_REQUIRED;
    }
}

static void
psql_bm25s_am_read_meta(
    Relation indexRelation,
    psql_bm25s_am_meta_page *meta_out
)
{
    Buffer buffer;
    Page page;
    psql_bm25s_am_meta_page *meta;

    if (psql_bm25s_am_relation_nblocks(indexRelation) == 0)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s index relation %u (%s) is empty",
                    RelationGetRelid(indexRelation),
                    RelationGetRelationName(indexRelation)
                )
            )
        );
    }

    buffer = ReadBufferExtended(indexRelation, MAIN_FORKNUM, 0, RBM_NORMAL, NULL);
    LockBuffer(buffer, BUFFER_LOCK_SHARE);
    page = BufferGetPage(buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(page);

    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        uint32 magic = meta->magic;
        uint32 version = meta->version;
        uint32 page_kind = meta->page_kind;
        Oid relid = RelationGetRelid(indexRelation);
        const char *relname = RelationGetRelationName(indexRelation);

        UnlockReleaseBuffer(buffer);
        ereport(
            ERROR,
            (
                errmsg(
                    "invalid psql_bm25s index metapage for relation %u (%s): "
                    "magic=%u version=%u page_kind=%u",
                    relid,
                    relname != NULL ? relname : "<unknown>",
                    magic,
                    version,
                    page_kind
                )
            )
        );
    }

    *meta_out = *meta;
    psql_bm25s_am_normalize_meta_storage(
        meta_out,
        psql_bm25s_am_relation_nblocks(indexRelation)
    );
    UnlockReleaseBuffer(buffer);
}

static BlockNumber
psql_bm25s_am_relation_nblocks(Relation indexRelation)
{
    BlockNumber nblocks;

    nblocks = RelationGetNumberOfBlocks(indexRelation);
    if (nblocks != 0)
    {
        return nblocks;
    }

    /*
     * Full rewrites truncate then extend the relation. The smgr nblocks cache
     * can transiently retain the post-truncate zero size after new pages are
     * visible. Close/reopen only on this rare zero-size path, so normal query
     * scans keep the cheap cached nblocks check.
     */
    RelationCloseSmgr(indexRelation);
    return RelationGetNumberOfBlocks(indexRelation);
}

static void
psql_bm25s_am_prewarm_delta_pages(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta
)
{
    BlockNumber nblocks;
    BlockNumber blkno;

    if (indexRelation == NULL ||
        meta == NULL ||
        meta->delta_record_count == 0 ||
        meta->delta_start_blkno == InvalidBlockNumber)
    {
        return;
    }
    if (!psql_bm25s_am_meta_uses_append_only(meta))
    {
        return;
    }
    if (!psql_bm25s_am_query_overlay_within_budget(indexRelation, meta))
    {
        return;
    }

    nblocks = psql_bm25s_am_relation_nblocks(indexRelation);
    if (meta->delta_start_blkno >= nblocks)
    {
        return;
    }

    /*
     * Shared-preload keeps the immutable base generation out of backend-local
     * memory, but eventual-consistency queries may still merge a bounded delta
     * tail from the PostgreSQL index relation. Warm those delta pages in the
     * background so the first foreground query after restart or ingest does not
     * pay random DataFileRead latency.
     */
    for (blkno = meta->delta_start_blkno; blkno < nblocks; blkno++)
    {
        Buffer buffer;

        CHECK_FOR_INTERRUPTS();
        buffer = ReadBufferExtended(
            indexRelation,
            MAIN_FORKNUM,
            blkno,
            RBM_NORMAL,
            NULL
        );
        ReleaseBuffer(buffer);
    }
}

static void
psql_bm25s_am_payload_health(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    psql_bm25s_am_payload_health_state *health_out
)
{
    uint64 expected_bytes;
    uint64 capacity_bytes = 0;
    BlockNumber nblocks;
    Size header_size;
    Size max_payload;

    memset(health_out, 0, sizeof(*health_out));
    health_out->status = "ok";
    health_out->reason = "ok";
    if (meta == NULL)
    {
        health_out->corrupt = true;
        health_out->rebuild_required = true;
        health_out->status = "corrupt";
        health_out->reason = "missing_meta";
        return;
    }

    if (meta->tid_bytes_len !=
        (uint64) meta->num_docs * sizeof(ItemPointerData))
    {
        health_out->corrupt = true;
        health_out->rebuild_required = true;
        health_out->status = "corrupt";
        health_out->reason = "invalid_tid_map_length";
        return;
    }

    expected_bytes = meta->tid_bytes_len + meta->index_bytes_len;
    health_out->expected_bytes = expected_bytes;
    nblocks = psql_bm25s_am_relation_nblocks(indexRelation);

    if (!psql_bm25s_am_meta_uses_append_only(meta))
    {
        health_out->corrupt = true;
        health_out->rebuild_required = true;
        health_out->status = "corrupt";
        health_out->reason = "unsupported_storage_layout";
        return;
    }
    else
    {
        uint64 end_blkno = (uint64) meta->active_start_blkno +
            (uint64) meta->active_data_pages;

        header_size = MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));
        max_payload = BLCKSZ - SizeOfPageHeaderData - header_size;
        if (end_blkno > (uint64) nblocks ||
            meta->delta_start_blkno > nblocks)
        {
            health_out->corrupt = true;
            health_out->rebuild_required = true;
            health_out->status = "corrupt";
            health_out->reason = "active_generation_out_of_bounds";
            return;
        }
        capacity_bytes = (uint64) meta->active_data_pages *
            (uint64) max_payload;
    }

    health_out->capacity_bytes = capacity_bytes;
    if (expected_bytes > capacity_bytes)
    {
        health_out->corrupt = true;
        health_out->rebuild_required = true;
        health_out->status = "corrupt";
        health_out->reason = "payload_capacity_shortfall";
        return;
    }
    if ((meta->flags & PSQL_BM25S_AM_FLAG_CORRUPT) != 0)
    {
        health_out->corrupt = true;
        health_out->rebuild_required = true;
        health_out->status = "corrupt";
        health_out->reason = "marked_corrupt";
        return;
    }
    if ((meta->flags & PSQL_BM25S_AM_FLAG_REBUILD_REQUIRED) != 0)
    {
        health_out->rebuild_required = true;
        health_out->status = "rebuild_required";
        health_out->reason = "marked_rebuild_required";
        return;
    }
    if ((meta->flags & PSQL_BM25S_AM_FLAG_STALE) != 0)
    {
        health_out->rebuild_required = true;
        health_out->status = "stale";
        health_out->reason = "stale";
    }
}

static uint64
psql_bm25s_am_rebuild_estimate(
    uint64 payload_bytes,
    uint64 multiplier
)
{
    if (multiplier == 0)
    {
        return 0;
    }
    if (payload_bytes > UINT64_MAX / multiplier)
    {
        return UINT64_MAX;
    }
    return payload_bytes * multiplier;
}

static uint64
psql_bm25s_am_budget_headroom_limit(
    uint64 budget_bytes,
    uint64 numerator,
    uint64 denominator
)
{
    if (denominator == 0)
    {
        return 0;
    }
    if (budget_bytes > UINT64_MAX / numerator)
    {
        return budget_bytes;
    }
    return (budget_bytes * numerator) / denominator;
}

static bool
psql_bm25s_am_rebuild_memory_budget_choose(
    const psql_bm25s_am_meta_page *meta,
    psql_bm25s_am_rebuild_builder *builder_out,
    uint64 *standard_estimated_bytes_out,
    uint64 *compact_estimated_bytes_out,
    uint64 *spill_estimated_bytes_out,
    uint64 *budget_bytes_out
)
{
    uint64 payload_bytes;
    uint64 standard_estimated_bytes;
    uint64 compact_estimated_bytes;
    uint64 spill_estimated_bytes;
    uint64 budget_bytes;
    uint64 standard_budget_limit;
    uint64 compact_budget_limit;
    bool standard_safe;
    bool compact_safe;

    if (builder_out != NULL)
    {
        *builder_out = PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD;
    }
    if (standard_estimated_bytes_out != NULL)
    {
        *standard_estimated_bytes_out = 0;
    }
    if (compact_estimated_bytes_out != NULL)
    {
        *compact_estimated_bytes_out = 0;
    }
    if (spill_estimated_bytes_out != NULL)
    {
        *spill_estimated_bytes_out = 0;
    }
    if (budget_bytes_out != NULL)
    {
        *budget_bytes_out = 0;
    }
    if (psql_bm25s_maintenance_rebuild_memory_budget_mb <= 0 ||
        meta == NULL)
    {
        return true;
    }

    payload_bytes = meta->tid_bytes_len + meta->index_bytes_len;
    /*
     * The standard builder keeps raw docs, term stats, COO arrays, CSC arrays,
     * and serialized bytes live at overlapping points. The compact builder
     * removes the doc_terms/COO overlap. The spill builder moves term entries
     * to PostgreSQL temp files and streams publish, leaving final arrays as the
     * main memory consumer.
     */
    standard_estimated_bytes = psql_bm25s_am_rebuild_estimate(
        payload_bytes,
        PSQL_BM25S_AM_REBUILD_MEMORY_ESTIMATE_MULTIPLIER
    );
    compact_estimated_bytes = psql_bm25s_am_rebuild_estimate(
        payload_bytes,
        PSQL_BM25S_AM_COMPACT_REBUILD_MEMORY_ESTIMATE_MULTIPLIER
    );
    spill_estimated_bytes = psql_bm25s_am_rebuild_estimate(
        payload_bytes,
        PSQL_BM25S_AM_SPILL_REBUILD_MEMORY_ESTIMATE_MULTIPLIER
    );
    standard_safe =
        payload_bytes <= PSQL_BM25S_AM_STANDARD_REBUILD_MAX_PAYLOAD_BYTES;
    compact_safe =
        payload_bytes <= PSQL_BM25S_AM_COMPACT_REBUILD_MAX_PAYLOAD_BYTES;
    budget_bytes = (uint64) psql_bm25s_maintenance_rebuild_memory_budget_mb *
        1024ULL * 1024ULL;
    standard_budget_limit = psql_bm25s_am_budget_headroom_limit(
        budget_bytes,
        PSQL_BM25S_AM_STANDARD_REBUILD_BUDGET_HEADROOM_NUM,
        PSQL_BM25S_AM_STANDARD_REBUILD_BUDGET_HEADROOM_DEN
    );
    compact_budget_limit = psql_bm25s_am_budget_headroom_limit(
        budget_bytes,
        PSQL_BM25S_AM_COMPACT_REBUILD_BUDGET_HEADROOM_NUM,
        PSQL_BM25S_AM_COMPACT_REBUILD_BUDGET_HEADROOM_DEN
    );
    if (standard_estimated_bytes_out != NULL)
    {
        *standard_estimated_bytes_out = standard_estimated_bytes;
    }
    if (compact_estimated_bytes_out != NULL)
    {
        *compact_estimated_bytes_out = compact_estimated_bytes;
    }
    if (spill_estimated_bytes_out != NULL)
    {
        *spill_estimated_bytes_out = spill_estimated_bytes;
    }
    if (budget_bytes_out != NULL)
    {
        *budget_bytes_out = budget_bytes;
    }
    /*
     * Automatic rebuilds optimize for steady query service, not fastest build
     * time. The standard builder has the widest live-memory overlap, so it
     * needs substantial headroom below the configured budget. Compact is
     * safer but still keeps final arrays plus term entries live. Spill is the
     * only path allowed to use the full budget envelope.
     */
    if (standard_safe && standard_estimated_bytes <= standard_budget_limit)
    {
        return true;
    }
    /*
     * Compact still keeps its term-entry array in backend memory. Large
     * payloads can cross PostgreSQL's single-allocation cap even when the
     * coarse total-byte estimate appears to fit, so large rebuilds must go to
     * the spill builder.
     */
    if (compact_safe && compact_estimated_bytes <= compact_budget_limit)
    {
        if (builder_out != NULL)
        {
            *builder_out = PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT;
        }
        return true;
    }
    if (spill_estimated_bytes <= budget_bytes)
    {
        if (builder_out != NULL)
        {
            *builder_out = PSQL_BM25S_AM_REBUILD_BUILDER_SPILL;
        }
        return true;
    }
    return false;
}

static psql_bm25s_am_rebuild_builder
psql_bm25s_am_choose_explicit_build_builder(
    Relation heapRelation,
    Relation indexRelation
)
{
    psql_bm25s_am_rebuild_builder builder =
        PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD;
    psql_bm25s_am_meta_page estimate_meta;
    bool has_estimate = false;
    uint64 standard_estimated_bytes = 0;
    uint64 compact_estimated_bytes = 0;
    uint64 spill_estimated_bytes = 0;
    uint64 budget_bytes = 0;
    bool admitted;

    memset(&estimate_meta, 0, sizeof(estimate_meta));

    /*
     * REINDEX may still expose the old metapage at build entry. Prefer that
     * exact active-payload estimate because it captures append-only bloat:
     * a physically huge relation can still have a much smaller live
     * generation, and choosing spill from live bytes avoids a standard
     * rebuild that would spike memory.
     */
    if (RelationGetNumberOfBlocks(indexRelation) > 0)
    {
        psql_bm25s_am_read_meta(indexRelation, &estimate_meta);
        has_estimate = true;
    }
    else if (heapRelation != NULL)
    {
        uint64 heap_bytes =
            (uint64) RelationGetNumberOfBlocks(heapRelation) * BLCKSZ;

        /*
         * CREATE INDEX has no old metapage. Heap size is an intentionally
         * conservative proxy: if it looks too large for the standard builder,
         * choose the safer compact/spill path rather than risking the first
         * build on a large table.
         */
        estimate_meta.index_bytes_len = heap_bytes;
        has_estimate = heap_bytes > 0;
    }

    if (!has_estimate ||
        psql_bm25s_maintenance_rebuild_memory_budget_mb <= 0)
    {
        return builder;
    }

    admitted = psql_bm25s_am_rebuild_memory_budget_choose(
        &estimate_meta,
        &builder,
        &standard_estimated_bytes,
        &compact_estimated_bytes,
        &spill_estimated_bytes,
        &budget_bytes
    );
    if (admitted)
    {
        return builder;
    }

    /*
     * Automatic maintenance skips when no builder fits the budget because a
     * stale resident generation is still better than forcing the host into
     * swap. Explicit CREATE INDEX / REINDEX has different semantics: the user
     * asked to build now, so take the lowest-memory implementation and make
     * the admission decision visible.
     */
    ereport(
        NOTICE,
        (
            errmsg(
                "psql_bm25s explicit build exceeds rebuild memory budget; "
                "using spill builder"
            ),
            errdetail(
                "standard_estimated_bytes=%llu, "
                "compact_estimated_bytes=%llu, "
                "spill_estimated_bytes=%llu, budget_bytes=%llu",
                (unsigned long long) standard_estimated_bytes,
                (unsigned long long) compact_estimated_bytes,
                (unsigned long long) spill_estimated_bytes,
                (unsigned long long) budget_bytes
            )
        )
    );
    return PSQL_BM25S_AM_REBUILD_BUILDER_SPILL;
}

static void
psql_bm25s_am_load_payload(
    Relation indexRelation,
    bool suppress_stale_warning,
    psql_bm25s_am_payload *payload_out
)
{
    size_t tid_bytes_len;
    size_t tid_bytes_offset = 0;
    size_t index_bytes_offset = 0;
    BlockNumber blkno;
    BlockNumber start_blkno;
    BlockNumber end_blkno;
    Size header_size;
    Size max_payload;
    psql_bm25s_am_payload_health_state health;

    memset(payload_out, 0, sizeof(*payload_out));
    psql_bm25s_am_read_meta(indexRelation, &payload_out->meta);

    if (!suppress_stale_warning &&
        (payload_out->meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0)
    {
        ereport(
            NOTICE,
            (
                errmsg("psql_bm25s index is stale"),
                errhint(
                    "Run psql_bm25s_index_maintain or REINDEX INDEX "
                    "to refresh it."
                )
            )
        );
    }

    psql_bm25s_am_payload_health(indexRelation, &payload_out->meta, &health);
    if (health.corrupt)
    {
        ereport(
            ERROR,
            (
                errmsg("corrupt psql_bm25s index payload"),
                errdetail(
                    "reason=%s expected_bytes=%llu capacity_bytes=%llu",
                    health.reason,
                    (unsigned long long) health.expected_bytes,
                    (unsigned long long) health.capacity_bytes
                ),
                errhint("Rebuild the index with REINDEX or psql_bm25s maintenance.")
            )
        );
    }

    if (!psql_bm25s_am_meta_uses_append_only(&payload_out->meta))
    {
        ereport(
            ERROR,
            (
                errmsg("unsupported psql_bm25s index storage layout"),
                errdetail(
                    "storage_version=%u",
                    payload_out->meta.storage_version
                ),
                errhint("Rebuild the index to use append-only generations.")
            )
        );
    }

    tid_bytes_len = (size_t) payload_out->meta.tid_bytes_len;
    payload_out->index_bytes_len = (size_t) payload_out->meta.index_bytes_len;

    if (tid_bytes_len > 0)
    {
        payload_out->doc_tids = malloc(tid_bytes_len);
        if (payload_out->doc_tids == NULL)
        {
            psql_bm25s_am_oom();
        }
    }
    if (payload_out->index_bytes_len > 0)
    {
        payload_out->index_bytes = malloc(payload_out->index_bytes_len);
        if (payload_out->index_bytes == NULL)
        {
            psql_bm25s_am_payload_free(payload_out);
            psql_bm25s_am_oom();
        }
    }

    start_blkno = payload_out->meta.active_start_blkno;
    end_blkno = start_blkno + payload_out->meta.active_data_pages;
    header_size = MAXALIGN(sizeof(psql_bm25s_am_generation_data_page));
    max_payload = BLCKSZ - SizeOfPageHeaderData - header_size;

    for (blkno = start_blkno; blkno < end_blkno; blkno++)
    {
        Buffer buffer;
        Page page;
        psql_bm25s_am_generation_data_page *generation_header;
        const char *page_data;
        uint32 used_bytes;

        if ((blkno & 0x7F) == 0)
        {
            CHECK_FOR_INTERRUPTS();
        }
        buffer = ReadBufferExtended(
            indexRelation,
            MAIN_FORKNUM,
            blkno,
            RBM_NORMAL,
            NULL
        );
        LockBuffer(buffer, BUFFER_LOCK_SHARE);
        page = BufferGetPage(buffer);
        generation_header =
            (psql_bm25s_am_generation_data_page *) PageGetContents(page);
        if (generation_header->magic != PSQL_BM25S_AM_MAGIC ||
            generation_header->version != PSQL_BM25S_AM_VERSION ||
            generation_header->page_kind != PSQL_BM25S_AM_PAGE_GENERATION_DATA ||
            generation_header->generation !=
                payload_out->meta.active_generation ||
            generation_header->ordinal != (uint32) (blkno - start_blkno))
        {
            UnlockReleaseBuffer(buffer);
            psql_bm25s_am_payload_free(payload_out);
            ereport(ERROR, (errmsg("invalid psql_bm25s generation page")));
        }
        used_bytes = generation_header->used_bytes;

        if (used_bytes > (uint32) max_payload)
        {
            UnlockReleaseBuffer(buffer);
            psql_bm25s_am_payload_free(payload_out);
            ereport(ERROR, (errmsg("corrupt psql_bm25s index payload")));
        }

        page_data = (const char *) PageGetContents(page) + header_size;
        if (tid_bytes_offset < tid_bytes_len)
        {
            size_t copy_len = Min(
                (size_t) used_bytes,
                tid_bytes_len - tid_bytes_offset
            );
            size_t remaining = (size_t) used_bytes - copy_len;

            memcpy(
                ((char *) payload_out->doc_tids) + tid_bytes_offset,
                page_data,
                copy_len
            );
            tid_bytes_offset += copy_len;
            page_data += copy_len;

            if (remaining > 0)
            {
                if (index_bytes_offset + remaining > payload_out->index_bytes_len)
                {
                    UnlockReleaseBuffer(buffer);
                    psql_bm25s_am_payload_free(payload_out);
                    ereport(ERROR, (errmsg("corrupt psql_bm25s index payload")));
                }
                memcpy(
                    payload_out->index_bytes + index_bytes_offset,
                    page_data,
                    remaining
                );
                index_bytes_offset += remaining;
            }
        }
        else
        {
            if (index_bytes_offset + used_bytes >
                payload_out->index_bytes_len)
            {
                UnlockReleaseBuffer(buffer);
                psql_bm25s_am_payload_free(payload_out);
                ereport(ERROR, (errmsg("corrupt psql_bm25s index payload")));
            }
            memcpy(
                payload_out->index_bytes + index_bytes_offset,
                page_data,
                used_bytes
            );
            index_bytes_offset += used_bytes;
        }
        UnlockReleaseBuffer(buffer);
    }

    if (tid_bytes_offset != tid_bytes_len ||
        index_bytes_offset != payload_out->index_bytes_len)
    {
        psql_bm25s_am_payload_free(payload_out);
        ereport(ERROR, (errmsg("truncated psql_bm25s index payload")));
    }
}

static void
psql_bm25s_am_set_meta_flags(
    Relation indexRelation,
    uint16 flags
)
{
    Buffer buffer;
    Page page;
    psql_bm25s_am_meta_page *meta;

    psql_bm25s_am_lock_maintenance(indexRelation);
    buffer = ReadBufferExtended(indexRelation, MAIN_FORKNUM, 0, RBM_NORMAL, NULL);
    LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
    page = BufferGetPage(buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(page);

    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(buffer);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta->flags = flags;
    MarkBufferDirty(buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(buffer, false);
        PageSetLSN(page, recptr);
    }
    UnlockReleaseBuffer(buffer);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static void
psql_bm25s_am_note_maintenance_activity(
    Relation indexRelation,
    uint32 pending_write_add,
    uint32 pending_delete_add
)
{
    Buffer buffer;
    Page page;
    psql_bm25s_am_meta_page *meta;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        return;
    }

    psql_bm25s_am_pin_maintenance_xact(indexRelation);
    psql_bm25s_am_lock_maintenance(indexRelation);
    buffer = ReadBufferExtended(indexRelation, MAIN_FORKNUM, 0, RBM_NORMAL, NULL);
    LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
    page = BufferGetPage(buffer);
    meta = (psql_bm25s_am_meta_page *) PageGetContents(page);

    if (meta->magic != PSQL_BM25S_AM_MAGIC ||
        meta->version != PSQL_BM25S_AM_VERSION ||
        meta->page_kind != PSQL_BM25S_AM_PAGE_META)
    {
        UnlockReleaseBuffer(buffer);
        ereport(ERROR, (errmsg("invalid psql_bm25s index metapage")));
    }

    meta->pending_write_tuples = psql_bm25s_am_saturating_add_u32(
        meta->pending_write_tuples,
        pending_write_add
    );
    meta->pending_delete_tuples = psql_bm25s_am_saturating_add_u32(
        meta->pending_delete_tuples,
        pending_delete_add
    );
    MarkBufferDirty(buffer);
    if (RelationNeedsWAL(indexRelation))
    {
        XLogRecPtr recptr;

        recptr = log_newpage_buffer(buffer, false);
        PageSetLSN(page, recptr);
    }
    UnlockReleaseBuffer(buffer);
    psql_bm25s_am_unlock_maintenance(indexRelation);
}

static bool
psql_bm25s_am_meta_has_pending_maintenance(
    const psql_bm25s_am_meta_page *meta
)
{
    return meta != NULL &&
        (meta->pending_write_tuples > 0 || meta->pending_delete_tuples > 0);
}

static bool
psql_bm25s_am_eventual_background_maintenance_due(
    Relation indexRelation,
    const psql_bm25s_am_meta_page *meta,
    const psql_bm25s_am_payload_health_state *health
)
{
    uint64 pending_total;
    bool stale;
    int threshold;
    int delta_bytes_threshold;

    if (meta == NULL || health == NULL)
    {
        return false;
    }

    pending_total = (uint64) meta->pending_write_tuples +
        (uint64) meta->pending_delete_tuples;
    stale = (meta->flags & PSQL_BM25S_AM_FLAG_STALE) != 0;

    if (stale || health->rebuild_required)
    {
        return true;
    }
    threshold = psql_bm25s_am_rebuild_threshold(indexRelation);
    if (pending_total > 0 && threshold > 0 &&
        pending_total >= (uint64) threshold)
    {
        return true;
    }
    delta_bytes_threshold =
        psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation);
    if (meta->delta_bytes_len > 0 && delta_bytes_threshold > 0 &&
        meta->delta_bytes_len >= (uint64) delta_bytes_threshold)
    {
        return true;
    }
    if (meta->delta_record_count > 0 &&
        !psql_bm25s_am_can_use_delta_overlay(meta))
    {
        return true;
    }

    return false;
}

static psql_bm25s_am_cache_entry *
psql_bm25s_am_get_cached_index(
    Relation indexRelation,
    Oid expected_source_type
)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    psql_bm25s_am_cache_entry *entry;
    psql_bm25s_am_payload payload;
    MemoryContext oldcontext;
    bool use_delta_overlay = false;
    bool materialize_delta_overlay = false;
    bool maintenance_stable = true;
    bool eventual_policy;
    bool dsm_share_eligible;
    bool local_selected;
    bool shared_preload_available;
    bool shared_required;
    volatile int generation_lock_fd = -1;
    volatile bool cache_load_locked = false;

    eventual_policy =
        psql_bm25s_am_eventual_policy_enabled(indexRelation);
    psql_bm25s_am_flush_pending_refresh_oid(RelationGetRelid(indexRelation));
    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_payload_health(indexRelation, &meta, &health);
    if (eventual_policy &&
        (psql_bm25s_am_meta_has_pending_maintenance(&meta) ||
         (meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0 ||
         health.rebuild_required))
    {
        psql_bm25s_am_schedule_background_maintenance(indexRelation);
    }
    if (health.corrupt)
    {
        entry = psql_bm25s_am_cache_find_resident_entry(
            RelationGetRelid(indexRelation),
            expected_source_type
        );
        if (entry != NULL)
        {
            psql_bm25s_am_cache_acquire_lease(entry);
            return entry;
        }
    }
    if (psql_bm25s_am_maintenance_tracking_enabled(indexRelation) &&
        psql_bm25s_am_meta_has_pending_maintenance(&meta))
    {
        maintenance_stable =
            psql_bm25s_am_wait_for_pending_maintenance(
                indexRelation,
                !eventual_policy
            );
        if (maintenance_stable)
        {
            psql_bm25s_am_read_meta(indexRelation, &meta);
        }
        if (psql_bm25s_am_meta_has_pending_maintenance(&meta) &&
            maintenance_stable &&
            !psql_bm25s_am_can_use_delta_overlay(&meta))
        {
            if (!RecoveryInProgress() &&
                (!eventual_policy ||
                 psql_bm25s_am_foreground_maintenance_enabled(indexRelation)))
            {
                psql_bm25s_am_rebuild_index_oid(
                    RelationGetRelid(indexRelation)
                );
                psql_bm25s_am_read_meta(indexRelation, &meta);
            }
        }
        else if (psql_bm25s_am_meta_has_pending_maintenance(&meta) &&
                 maintenance_stable &&
                 psql_bm25s_am_query_overlay_within_budget(
                     indexRelation,
                     &meta
                 ))
        {
            use_delta_overlay = true;
        }
    }
    materialize_delta_overlay = use_delta_overlay &&
        (
            meta.source_type == INT4ARRAYOID ||
            (
                !eventual_policy &&
                psql_bm25s_am_source_type_is_textlike(meta.source_type)
            )
        );
    if (meta.source_type != expected_source_type)
    {
        ereport(ERROR, (errmsg("query function does not match indexed column type")));
    }
    if ((meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0)
    {
        ereport(
            NOTICE,
            (
                errmsg("psql_bm25s index is stale"),
                errhint(
                    "Run psql_bm25s_index_maintain or REINDEX INDEX "
                    "to refresh it."
                )
            )
        );
    }

    entry = psql_bm25s_am_cache_get_entry(
        RelationGetRelid(indexRelation),
        indexRelation,
        &meta
    );
    if (psql_bm25s_am_cache_entry_matches(entry, indexRelation, &meta))
    {
        if (!entry->generation.delta_overlay_materialized)
        {
            entry->generation.meta = meta;
        }
        psql_bm25s_am_cache_maybe_shrink_idle_workspace(entry);
        if (!materialize_delta_overlay || entry->generation.delta_overlay_materialized)
        {
            psql_bm25s_am_cache_acquire_lease(entry);
            return entry;
        }
        if (entry->lease_count > 0)
        {
            entry = psql_bm25s_am_cache_get_unleased_entry(
                RelationGetRelid(indexRelation)
            );
        }
    }

    psql_bm25s_am_cache_entry_reset(entry);
    /*
     * AccessShareLock is enough to keep relation rewrites from replacing the
     * physical index while a cold backend reads payload pages. The previous
     * ShareLock also blocked normal RowExclusiveLock writers, so a slow
     * first-query generation load could stall ingest during eventual
     * maintenance.
     */
    LockRelation(indexRelation, AccessShareLock);
    cache_load_locked = true;
    PG_TRY();
    {
        psql_bm25s_am_read_meta(indexRelation, &meta);
        use_delta_overlay = false;
        if (psql_bm25s_am_maintenance_tracking_enabled(indexRelation) &&
            psql_bm25s_am_meta_has_pending_maintenance(&meta) &&
            psql_bm25s_am_can_use_delta_overlay(&meta) &&
            psql_bm25s_am_query_overlay_within_budget(indexRelation, &meta))
        {
            use_delta_overlay = true;
        }
        materialize_delta_overlay = use_delta_overlay &&
            (
                meta.source_type == INT4ARRAYOID ||
                (
                    !eventual_policy &&
                    psql_bm25s_am_source_type_is_textlike(meta.source_type)
                )
            );
        if (meta.source_type != expected_source_type)
        {
            ereport(
                ERROR,
                (errmsg("query function does not match indexed column type"))
            );
        }
        if (materialize_delta_overlay)
        {
            psql_bm25s_am_cache_load_overlay(entry, indexRelation, &meta);
        }
        else
        {
            dsm_share_eligible =
                psql_bm25s_am_generation_dsm_share_eligible(&meta);
            shared_preload_available =
                psql_bm25s_am_shared_preload_cache_available();
            shared_required =
                psql_bm25s_am_generation_share_required(&meta);
            local_selected =
                psql_bm25s_am_generation_local_selected(&meta);
            if (shared_preload_available &&
                psql_bm25s_am_generation_attach_preload(
                    entry,
                    indexRelation,
                    &meta))
            {
                /* attached below */
            }
            else if (shared_preload_available &&
                     psql_bm25s_am_wait_for_auto_preload(
                         entry,
                         indexRelation,
                         &meta))
            {
                /* attached below */
            }
            else if (dsm_share_eligible &&
                psql_bm25s_am_generation_attach_shared(
                    entry,
                    indexRelation,
                    &meta))
            {
                /* attached below */
            }
            else
            {
                memset(&payload, 0, sizeof(payload));
                psql_bm25s_am_payload_health(indexRelation, &meta, &health);
                if (health.corrupt)
                {
                    ereport(
                        ERROR,
                        (
                            errmsg("corrupt psql_bm25s index payload"),
                            errdetail(
                                "reason=%s expected_bytes=%llu "
                                "capacity_bytes=%llu",
                                health.reason,
                                (unsigned long long) health.expected_bytes,
                                (unsigned long long) health.capacity_bytes
                            ),
                            errhint(
                                "Run psql_bm25s maintenance or REINDEX to "
                                "rebuild the index."
                            )
                        )
                    );
                }
                if (!local_selected)
                {
                    bool lock_acquired = false;

                    generation_lock_fd =
                        psql_bm25s_am_generation_try_publish_lock(
                            indexRelation,
                            &meta
                        );
                    if (generation_lock_fd < 0)
                    {
                        generation_lock_fd =
                            psql_bm25s_am_generation_wait_for_publish_lock(
                                indexRelation,
                                &meta
                            );
                    }
                    if (generation_lock_fd >= 0)
                    {
                        bool shared_built = false;

                        lock_acquired = true;
                        psql_bm25s_am_generation_publish_failure_unlink(
                            indexRelation,
                            &meta
                        );
                        if (shared_preload_available &&
                            psql_bm25s_am_generation_attach_preload(
                                entry,
                                indexRelation,
                                &meta))
                        {
                            shared_built = true;
                        }
                        else if (dsm_share_eligible &&
                                 psql_bm25s_am_generation_attach_shared(
                                     entry,
                                     indexRelation,
                                     &meta))
                        {
                            shared_built = true;
                        }
                        else
                        {
                            psql_bm25s_am_load_payload(
                                indexRelation,
                                true,
                                &payload
                            );
                            if (shared_preload_available)
                            {
                                shared_built =
                                    psql_bm25s_am_generation_build_preload_from_payload(
                                        entry,
                                        indexRelation,
                                        &payload
                                    );
                            }
                            if (!shared_built && dsm_share_eligible)
                            {
                                shared_built =
                                    psql_bm25s_am_generation_build_shared_from_payload(
                                        entry,
                                        indexRelation,
                                        &payload
                                    );
                            }
                            if (!shared_built)
                            {
                                psql_bm25s_am_generation_publish_failure_write(
                                    indexRelation,
                                    &meta
                                );
                                if (shared_required)
                                {
                                    psql_bm25s_am_payload_free(&payload);
                                    psql_bm25s_am_generation_publish_unlock(
                                        indexRelation,
                                        &meta,
                                        (int) generation_lock_fd
                                    );
                                    generation_lock_fd = -1;
                                    ereport(
                                        ERROR,
                                        (
                                            errmsg(
                                                "could not publish psql_bm25s generation to required shared cache"
                                            ),
                                            errhint(
                                                "Clear stale generation cache state, "
                                                "increase shared generation cache "
                                                "capacity, disable the optional "
                                                "shared-preload arena, or retry "
                                                "after shared memory pressure is "
                                                "lower."
                                            )
                                        )
                                    );
                                }
                            }
                        }
                        if (shared_built)
                        {
                            psql_bm25s_am_generation_publish_failure_unlink(
                                indexRelation,
                                &meta
                            );
                        }
                        psql_bm25s_am_generation_publish_unlock(
                            indexRelation,
                            &meta,
                            (int) generation_lock_fd
                        );
                        generation_lock_fd = -1;
                    }
                    if (!lock_acquired)
                    {
                        ereport(
                            ERROR,
                            (
                                errmsg(
                                    "could not acquire psql_bm25s generation "
                                    "publish lock"
                                )
                            )
                        );
                    }
                }
                else
                {
                    psql_bm25s_am_load_payload(indexRelation, true, &payload);
                }

                if (entry->mcxt == NULL)
                {
                    entry->mcxt = AllocSetContextCreate(
                        psql_bm25s_am_cache.mcxt,
                        "psql_bm25s cache entry",
                        ALLOCSET_START_SMALL_SIZES
                    );
                    oldcontext = MemoryContextSwitchTo(entry->mcxt);
                    psql_bm25s_am_generation_build_block_from_payload(
                        &entry->generation,
                        entry->mcxt,
                        &payload
                    );
                    MemoryContextSwitchTo(oldcontext);

                    entry->generation.locator = indexRelation->rd_locator;
                    entry->generation.meta = payload.meta;
                    entry->generation.delta_overlay_materialized = false;
                    psql_bm25s_am_cache_build_sorted_vocab(entry);
                }
                psql_bm25s_am_payload_free(&payload);
            }
        }
        UnlockRelation(indexRelation, AccessShareLock);
        cache_load_locked = false;
    }
    PG_CATCH();
    {
        if (generation_lock_fd >= 0)
        {
            psql_bm25s_am_generation_publish_unlock(
                indexRelation,
                &meta,
                (int) generation_lock_fd
            );
            generation_lock_fd = -1;
        }
        if (cache_load_locked)
        {
            UnlockRelation(indexRelation, AccessShareLock);
            cache_load_locked = false;
        }
        PG_RE_THROW();
    }
    PG_END_TRY();
    psql_bm25s_am_cache_acquire_lease(entry);
    return entry;
}

static psql_bm25s_status
psql_bm25s_am_query_token_ids_cached(
    const psql_bm25s_am_cache_entry *entry,
    char **tokens,
    size_t num_tokens,
    uint32_t **query_ids_out,
    size_t *query_len_out
)
{
    uint32_t *query_ids = NULL;
    size_t bytes;
    size_t i;

    if (entry == NULL || query_ids_out == NULL || query_len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *query_ids_out = NULL;
    *query_len_out = 0;
    if (entry->generation.sorted_vocab_ids == NULL &&
        entry->generation.sorted_vocab == NULL &&
        entry->generation.index.vocab_size > 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (!psql_bm25s_am_checked_mul_size(num_tokens, sizeof(*query_ids), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (num_tokens > 0)
    {
        query_ids = malloc(bytes);
        if (query_ids == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    for (i = 0; i < num_tokens; i++)
    {
        size_t pos;

        if (tokens[i] == NULL)
        {
            free(query_ids);
            return PSQL_BM25S_ERR_INVALID;
        }

        pos = psql_bm25s_am_vocab_lower_bound(entry, tokens[i]);
        if (pos < entry->generation.index.vocab_size &&
            strcmp(psql_bm25s_am_sorted_vocab_token(entry, pos), tokens[i]) == 0)
        {
            query_ids[*query_len_out] =
                psql_bm25s_am_sorted_vocab_token_id(entry, pos);
            (*query_len_out)++;
        }
    }

    *query_ids_out = query_ids;
    return PSQL_BM25S_OK;
}

static void
psql_bm25s_am_require_index_relation(Relation indexRelation)
{
    Oid am_oid;

    am_oid = get_am_oid("psql_bm25s", false);
    if (indexRelation->rd_rel->relkind != RELKIND_INDEX ||
        indexRelation->rd_rel->relam != am_oid)
    {
        ereport(ERROR, (errmsg("relation is not a psql_bm25s index")));
    }
}

static void
psql_bm25s_am_require_index_owner(Relation indexRelation)
{
    if (!object_ownercheck(
            RelationRelationId,
            RelationGetRelid(indexRelation),
            GetUserId()
        ))
    {
        aclcheck_error(
            ACLCHECK_NOT_OWNER,
            OBJECT_INDEX,
            RelationGetRelationName(indexRelation)
        );
    }
}

static text *
psql_bm25s_am_describe_relation(Relation indexRelation)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    BlockNumber pages;
    char *result;

    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_payload_health(indexRelation, &meta, &health);
    pages = RelationGetNumberOfBlocks(indexRelation);
    result = psprintf(
        "psql_bm25s_access_index(type=%s, docs=%u, bytes=%llu, pages=%u, "
        "stale=%s, payload_health=%s, rebuild_required=%s)",
        format_type_be(meta.source_type),
        meta.num_docs,
        (unsigned long long) meta.index_bytes_len,
        pages,
        (meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0 ? "true" : "false",
        health.status,
        health.rebuild_required ? "true" : "false"
    );

    return cstring_to_text(result);
}

static text *
psql_bm25s_am_describe_maintenance(Relation indexRelation)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    char *result;

    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_payload_health(indexRelation, &meta, &health);
    result = psprintf(
        "psql_bm25s_maintenance_state(rebuilds=%llu, pending_writes=%u, "
        "pending_deletes=%u, delta_records=%u, delta_bytes=%llu, stale=%s, "
        "payload_health=%s, rebuild_required=%s)",
        (unsigned long long) meta.rebuild_count,
        meta.pending_write_tuples,
        meta.pending_delete_tuples,
        meta.delta_record_count,
        (unsigned long long) meta.delta_bytes_len,
        (meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0 ? "true" : "false",
        health.status,
        health.rebuild_required ? "true" : "false"
    );
    return cstring_to_text(result);
}

static text *
psql_bm25s_am_describe_maintenance_policy(Relation indexRelation)
{
    char *result;
    int consistency;

    consistency = psql_bm25s_am_get_consistency(indexRelation);
    if (consistency == PSQL_BM25S_AM_CONSISTENCY_MANUAL)
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=manual)"
        );
    }
    else if (consistency == PSQL_BM25S_AM_CONSISTENCY_EVENTUAL)
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=eventual, "
            "auto_rebuild_threshold=%d, auto_rebuild_delta_bytes=%d)",
            psql_bm25s_am_rebuild_threshold(indexRelation),
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation)
        );
    }
    else
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=realtime, "
            "auto_rebuild_threshold=%d, auto_rebuild_delta_bytes=%d, "
            "auto_rebuild_churn_ratio=%.6g)",
            psql_bm25s_am_rebuild_threshold(indexRelation),
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation),
            psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation)
        );
    }
    return cstring_to_text(result);
}

static text *
psql_bm25s_am_describe_maintenance_policy_details(Relation indexRelation)
{
    char *result;
    int consistency;

    consistency = psql_bm25s_am_get_consistency(indexRelation);
    if (consistency == PSQL_BM25S_AM_CONSISTENCY_MANUAL)
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=manual)"
        );
    }
    else if (consistency == PSQL_BM25S_AM_CONSISTENCY_EVENTUAL)
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=eventual, "
            "auto_rebuild_threshold=%d, auto_rebuild_delta_bytes=%d, "
            "query_overlay_max_records=%d, query_overlay_max_bytes=%d)",
            psql_bm25s_am_rebuild_threshold(indexRelation),
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation),
            psql_bm25s_am_query_overlay_max_records(indexRelation),
            psql_bm25s_am_query_overlay_max_bytes(indexRelation)
        );
    }
    else
    {
        result = psprintf(
            "psql_bm25s_maintenance_policy(consistency=realtime, "
            "auto_rebuild_threshold=%d, auto_rebuild_delta_bytes=%d, "
            "auto_rebuild_churn_ratio=%.6g)",
            psql_bm25s_am_rebuild_threshold(indexRelation),
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation),
            psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation)
        );
    }
    return cstring_to_text(result);
}

static const char *
psql_bm25s_am_consistency_name(int consistency)
{
    switch (consistency)
    {
        case PSQL_BM25S_AM_CONSISTENCY_REALTIME:
            return "realtime";
        case PSQL_BM25S_AM_CONSISTENCY_EVENTUAL:
            return "eventual";
        case PSQL_BM25S_AM_CONSISTENCY_MANUAL:
            return "manual";
        default:
            return "unknown";
    }
}

static bool
psql_bm25s_am_double_eq(double left, double right)
{
    double diff = left - right;

    if (diff < 0.0)
    {
        diff = -diff;
    }

    return diff < 1e-9;
}

static void
psql_bm25s_am_get_policy_recommendation(
    Relation indexRelation,
    const char *profile,
    psql_bm25s_am_policy_recommendation *recommendation
)
{
    psql_bm25s_am_meta_page meta;
    int threshold;
    int delta_bytes_threshold;
    double churn_ratio_threshold;

    memset(recommendation, 0, sizeof(*recommendation));
    psql_bm25s_am_read_meta(indexRelation, &meta);
    threshold = psql_bm25s_am_rebuild_threshold(indexRelation);
    delta_bytes_threshold =
        psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation);
    churn_ratio_threshold =
        psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation);
    recommendation->docs = meta.num_docs;
    recommendation->pending_total = (uint64) meta.pending_write_tuples +
        (uint64) meta.pending_delete_tuples;
    recommendation->refresh_now =
        (meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0;

    if (profile == NULL || *profile == '\0' ||
        pg_strcasecmp(profile, "balanced") == 0)
    {
        recommendation->profile = "balanced";
        recommendation->recommended_options =
            "WITH (consistency = 'realtime', auto_rebuild_threshold = 1000, auto_rebuild_delta_bytes = 50000, auto_rebuild_churn_ratio = 0.05)";
        recommendation->recommended_consistency = "realtime";
        recommendation->recommended_auto_rebuild_threshold = 1000;
        recommendation->recommended_auto_rebuild_delta_bytes = 50000;
        recommendation->recommended_auto_rebuild_churn_ratio = 0.05;
        recommendation->has_recommended_auto_rebuild_threshold = true;
        recommendation->has_recommended_auto_rebuild_delta_bytes = true;
        recommendation->has_recommended_auto_rebuild_churn_ratio = true;
        recommendation->reason =
            "the broader local policy matrix currently favors the tuned deferred preset as the default benchmark-backed choice, but repeat runs show real workload sensitivity";
        recommendation->confidence = "medium";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_REALTIME &&
            threshold > 0 &&
            delta_bytes_threshold > 0 &&
            churn_ratio_threshold > 0.0;
    }
    else if (pg_strcasecmp(profile, "small_mixed_churn") == 0)
    {
        recommendation->profile = "small_mixed_churn";
        recommendation->recommended_options =
            "WITH (consistency = 'realtime', auto_rebuild_threshold = 1000, auto_rebuild_delta_bytes = 50000, auto_rebuild_churn_ratio = 0.05)";
        recommendation->recommended_consistency = "realtime";
        recommendation->recommended_auto_rebuild_threshold = 1000;
        recommendation->recommended_auto_rebuild_delta_bytes = 50000;
        recommendation->recommended_auto_rebuild_churn_ratio = 0.05;
        recommendation->has_recommended_auto_rebuild_threshold = true;
        recommendation->has_recommended_auto_rebuild_delta_bytes = true;
        recommendation->has_recommended_auto_rebuild_churn_ratio = true;
        recommendation->reason =
            "smaller local mixed-churn benchmarks favored a bytes-plus-churn deferred policy among deferred presets";
        recommendation->confidence = "medium";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_REALTIME &&
            threshold > 0 &&
            delta_bytes_threshold > 0 &&
            churn_ratio_threshold > 0.0;
    }
    else if (pg_strcasecmp(profile, "heavy_mixed_churn") == 0)
    {
        recommendation->profile = "heavy_mixed_churn";
        recommendation->recommended_options =
            "WITH (consistency = 'realtime', auto_rebuild_threshold = 1000, auto_rebuild_delta_bytes = 50000, auto_rebuild_churn_ratio = 0.05)";
        recommendation->recommended_consistency = "realtime";
        recommendation->recommended_auto_rebuild_threshold = 1000;
        recommendation->recommended_auto_rebuild_delta_bytes = 50000;
        recommendation->recommended_auto_rebuild_churn_ratio = 0.05;
        recommendation->has_recommended_auto_rebuild_threshold = true;
        recommendation->has_recommended_auto_rebuild_delta_bytes = true;
        recommendation->has_recommended_auto_rebuild_churn_ratio = true;
        recommendation->reason =
            "the broader local policy matrix favored the tuned deferred preset for heavy mixed churn, but repeat runs showed no single deferred policy winning consistently";
        recommendation->confidence = "low";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_REALTIME &&
            threshold > 0 &&
            delta_bytes_threshold > 0 &&
            churn_ratio_threshold > 0.0;
    }
    else if (pg_strcasecmp(profile, "heavy_insert_skew") == 0)
    {
        recommendation->profile = "heavy_insert_skew";
        recommendation->recommended_options =
            "WITH (consistency = 'realtime', auto_rebuild_threshold = 1000, auto_rebuild_delta_bytes = 40000)";
        recommendation->recommended_consistency = "realtime";
        recommendation->recommended_auto_rebuild_threshold = 1000;
        recommendation->recommended_auto_rebuild_delta_bytes = 40000;
        recommendation->has_recommended_auto_rebuild_threshold = true;
        recommendation->has_recommended_auto_rebuild_delta_bytes = true;
        recommendation->reason =
            "repeatable heavy insert-skew benchmarks favored the simpler bytes-bounded deferred policy in all deferred repeats";
        recommendation->confidence = "high";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_REALTIME &&
            threshold > 0 &&
            delta_bytes_threshold == 40000 &&
            psql_bm25s_am_double_eq(churn_ratio_threshold, 0.0);
    }
    else if (pg_strcasecmp(profile, "longrun_mixed_churn") == 0)
    {
        recommendation->profile = "longrun_mixed_churn";
        recommendation->recommended_options =
            "WITH (consistency = 'realtime', auto_rebuild_threshold = 1000, auto_rebuild_delta_bytes = 50000, auto_rebuild_churn_ratio = 0.09)";
        recommendation->recommended_consistency = "realtime";
        recommendation->recommended_auto_rebuild_threshold = 1000;
        recommendation->recommended_auto_rebuild_delta_bytes = 50000;
        recommendation->recommended_auto_rebuild_churn_ratio = 0.09;
        recommendation->has_recommended_auto_rebuild_threshold = true;
        recommendation->has_recommended_auto_rebuild_delta_bytes = true;
        recommendation->has_recommended_auto_rebuild_churn_ratio = true;
        recommendation->reason =
            "the heavier long-run mixed-churn benchmark favored a slightly more relaxed deferred preset in two of three repeats";
        recommendation->confidence = "medium";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_REALTIME &&
            threshold > 0 &&
            delta_bytes_threshold > 0 &&
            churn_ratio_threshold > 0.0 &&
            churn_ratio_threshold >= 0.09;
    }
    else if (pg_strcasecmp(profile, "query_first") == 0)
    {
        recommendation->profile = "query_first";
        recommendation->recommended_options =
            "WITH (consistency = 'eventual')";
        recommendation->recommended_consistency = "eventual";
        recommendation->reason =
            "query-first guidance favors bounded stale reads with automatic "
            "background convergence";
        recommendation->confidence = "high";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_EVENTUAL &&
            threshold == PSQL_BM25S_AM_DEFAULT_EVENTUAL_REBUILD_THRESHOLD;
    }
    else if (pg_strcasecmp(profile, "write_tolerant_query_first") == 0 ||
             pg_strcasecmp(profile, "write_first") == 0)
    {
        recommendation->profile = "write_tolerant_query_first";
        recommendation->recommended_options =
            "WITH (consistency = 'eventual')";
        recommendation->recommended_consistency = "eventual";
        recommendation->reason =
            "eventual consistency minimizes foreground write and query "
            "maintenance while converging automatically";
        recommendation->confidence = "medium";
        recommendation->matches_current =
            psql_bm25s_am_get_consistency(indexRelation) ==
            PSQL_BM25S_AM_CONSISTENCY_EVENTUAL;
    }
    else
    {
        ereport(
            ERROR,
            (errmsg("unsupported maintenance recommendation profile: %s",
                    profile))
        );
        pg_unreachable();
    }
}

static text *
psql_bm25s_am_recommend_maintenance_policy(
    Relation indexRelation,
    const char *profile
)
{
    psql_bm25s_am_policy_recommendation recommendation;
    char *result;

    psql_bm25s_am_get_policy_recommendation(
        indexRelation,
        profile,
        &recommendation
    );

    result = psprintf(
        "psql_bm25s_maintenance_recommendation(profile=%s, confidence=%s, recommended='%s', matches_current=%s, refresh_now=%s, docs=%u, pending_total=%llu, reason='%s')",
        recommendation.profile,
        recommendation.confidence,
        recommendation.recommended_options,
        recommendation.matches_current ? "true" : "false",
        recommendation.refresh_now ? "true" : "false",
        recommendation.docs,
        (unsigned long long) recommendation.pending_total,
        recommendation.reason
    );
    return cstring_to_text(result);
}

static void
psql_bm25s_am_mark_stale(Relation indexRelation)
{
    psql_bm25s_am_meta_page meta;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        return;
    }

    psql_bm25s_am_read_meta(indexRelation, &meta);
    if ((meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0)
    {
        return;
    }

    psql_bm25s_am_set_meta_flags(
        indexRelation,
        meta.flags | PSQL_BM25S_AM_FLAG_STALE
    );
}

static void
psql_bm25s_am_get_stats(
    Relation indexRelation,
    IndexBulkDeleteResult *stats
)
{
    psql_bm25s_am_meta_page meta;

    if (RelationGetNumberOfBlocks(indexRelation) == 0)
    {
        memset(stats, 0, sizeof(*stats));
        stats->num_pages = 0;
        return;
    }

    psql_bm25s_am_read_meta(indexRelation, &meta);
    stats->num_pages = RelationGetNumberOfBlocks(indexRelation);
    stats->num_index_tuples = meta.num_docs;
}

static void
psql_bm25s_am_build_replacement(
    Relation heapRelation,
    Relation indexRelation,
    IndexInfo *indexInfo,
    psql_bm25s_am_rebuild_builder builder,
    bool keep_index_for_streaming,
    psql_bm25s_am_replacement *replacement_out
)
{
    Oid source_type;
    psql_bm25s_am_build_state build_state;
    psql_bm25s_params params;
    bool create_empty_token;
    psql_bm25s_index index;
    psql_bm25s_status status;
    uint8_t *index_bytes = NULL;
    size_t index_bytes_len = 0;
    double heap_tuples;
    ItemPointerData *replacement_tids = NULL;
    MemoryContext caller_context;
    MemoryContext build_context;

    psql_bm25s_init_reloptions();
    memset(replacement_out, 0, sizeof(*replacement_out));
    memset(&build_state, 0, sizeof(build_state));
    caller_context = CurrentMemoryContext;
    build_context = AllocSetContextCreate(
        caller_context,
        "psql_bm25s build replacement",
        ALLOCSET_DEFAULT_SIZES
    );
    MemoryContextSwitchTo(build_context);
    source_type = psql_bm25s_am_source_type(indexRelation);
    psql_bm25s_am_validate_source_type(source_type);
    build_state.source_type = source_type;
    build_state.rebuild_builder = builder;
    build_state.natts = psql_bm25s_am_index_natts(indexRelation);
    build_state.build_mode = psql_bm25s_am_build_mode_from_source_type(
        source_type,
        build_state.natts
    );
    if (psql_bm25s_am_field_aware_enabled(indexRelation))
    {
        if (build_state.natts <= 1)
        {
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s field_aware indexes require multiple indexed columns"
                    )
                )
            );
        }
        if (!psql_bm25s_am_source_type_is_textlike(source_type))
        {
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s field_aware indexes require text[], "
                        "varchar[], text, or varchar columns"
                    )
                )
            );
        }
    }
    if (psql_bm25s_am_source_type_is_scalar_text(source_type))
    {
        psql_bm25s_am_read_index_text_options(
            indexRelation,
            &build_state.text_options,
            &build_state.text_stopwords,
            &build_state.text_stopword_len
        );
    }
    psql_bm25s_am_read_params(
        indexRelation,
        &params,
        &create_empty_token
    );
    psql_bm25s_index_init(&index);

    heap_tuples = table_index_build_scan(
        heapRelation,
        indexRelation,
        indexInfo,
        true,
        true,
        psql_bm25s_am_build_callback,
        &build_state,
        NULL
    );

    if (builder == PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT ||
        builder == PSQL_BM25S_AM_REBUILD_BUILDER_SPILL)
    {
        uint32_t vocab_size = 0;
        uint32_t empty_token_id = 0;
        bool has_empty_token = false;
        const char **vocab = NULL;

        if (source_type == INT4ARRAYOID)
        {
            if (build_state.has_terms)
            {
                vocab_size = build_state.max_token_id + 1;
            }
            if (create_empty_token)
            {
                has_empty_token = true;
                if (!build_state.zero_present)
                {
                    empty_token_id = 0;
                }
                else if (!build_state.has_terms)
                {
                    empty_token_id = 0;
                }
                else if (build_state.max_token_id == UINT32_MAX)
                {
                    status = PSQL_BM25S_ERR_RANGE;
                    goto compact_built;
                }
                else
                {
                    empty_token_id = build_state.max_token_id + 1;
                }
                if (empty_token_id + 1 > vocab_size)
                {
                    vocab_size = empty_token_id + 1;
                }
            }
            if (!build_state.has_terms && !create_empty_token)
            {
                status = PSQL_BM25S_ERR_INVALID;
                goto compact_built;
            }
        }
        else
        {
            if (build_state.vocab_map.size > UINT32_MAX)
            {
                status = PSQL_BM25S_ERR_RANGE;
                goto compact_built;
            }
            vocab_size = (uint32_t) build_state.vocab_map.size;
            vocab = psql_bm25s_am_vocab_map_to_array(&build_state.vocab_map);
        }

        if (builder == PSQL_BM25S_AM_REBUILD_BUILDER_SPILL)
        {
            if (build_state.spill_entries == NULL)
            {
                build_state.spill_entries = BufFileCreateTemp(false);
            }
            status = psql_bm25s_am_spill_term_entry_rewind(
                build_state.spill_entries
            );
            if (status != PSQL_BM25S_OK)
            {
                goto compact_built;
            }
            status = psql_bm25s_build_index_from_term_entry_reader(
                build_state.spill_entry_count,
                psql_bm25s_am_spill_term_entry_read,
                psql_bm25s_am_spill_term_entry_rewind,
                build_state.spill_entries,
                build_state.doc_lengths.lengths,
                (uint32_t) build_state.doc_lengths.len,
                vocab_size,
                &params,
                create_empty_token,
                has_empty_token,
                empty_token_id,
                vocab,
                &index
            );
        }
        else
        {
            status = psql_bm25s_build_index_from_term_entries(
                build_state.term_entries.entries,
                build_state.term_entries.len,
                build_state.doc_lengths.lengths,
                (uint32_t) build_state.doc_lengths.len,
                vocab_size,
                &params,
                create_empty_token,
                has_empty_token,
                empty_token_id,
                vocab,
                &index
            );
        }
compact_built:
        if (vocab != NULL)
        {
            pfree(vocab);
        }
    }
    else if (source_type == INT4ARRAYOID)
    {
        status = psql_bm25s_build_index_from_ids(
            build_state.id_docs.docs,
            build_state.id_docs.len,
            &params,
            create_empty_token,
            &index
        );
    }
    else
    {
        status = psql_bm25s_build_index_from_tokens(
            build_state.token_docs.docs,
            build_state.token_docs.len,
            &params,
            &index
        );
    }

    psql_bm25s_builder_ids_free(&build_state.id_docs);
    psql_bm25s_builder_tokens_free(&build_state.token_docs);
    psql_bm25s_am_term_entry_builder_free(&build_state.term_entries);
    psql_bm25s_am_doc_length_builder_free(&build_state.doc_lengths);
    psql_bm25s_am_vocab_map_free(&build_state.vocab_map);
    if (build_state.spill_entries != NULL)
    {
        BufFileClose(build_state.spill_entries);
        build_state.spill_entries = NULL;
    }
    psql_bm25s_am_free_query_tokens(
        build_state.text_stopwords,
        build_state.text_stopword_len
    );
    psql_bm25s_am_release_unused_malloc();
    if (status != PSQL_BM25S_OK)
    {
        MemoryContextSwitchTo(caller_context);
        MemoryContextDelete(build_context);
        psql_bm25s_am_release_unused_malloc();
        ereport(ERROR, (errmsg("failed to build bm25 index: %s",
                               psql_bm25s_strerror(status))));
    }

    if (keep_index_for_streaming)
    {
        status = psql_bm25s_serialized_index_size(&index, &index_bytes_len);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_index_free(&index);
            MemoryContextSwitchTo(caller_context);
            MemoryContextDelete(build_context);
            psql_bm25s_am_release_unused_malloc();
            ereport(ERROR, (errmsg("failed to size bm25 index: %s",
                                   psql_bm25s_strerror(status))));
        }
    }
    else
    {
        status = psql_bm25s_serialize_index(
            &index,
            &index_bytes,
            &index_bytes_len
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_index_free(&index);
            MemoryContextSwitchTo(caller_context);
            MemoryContextDelete(build_context);
            psql_bm25s_am_release_unused_malloc();
            ereport(ERROR, (errmsg("failed to serialize bm25 index: %s",
                                   psql_bm25s_strerror(status))));
        }
    }
    if (build_state.tids.len > 0)
    {
        size_t tids_bytes;

        if (!psql_bm25s_am_checked_mul_size(
                build_state.tids.len,
                sizeof(*replacement_tids),
                &tids_bytes))
        {
            psql_bm25s_index_free(&index);
            free(index_bytes);
            MemoryContextSwitchTo(caller_context);
            MemoryContextDelete(build_context);
            psql_bm25s_am_release_unused_malloc();
            ereport(ERROR, (errmsg("psql_bm25s replacement tid map is too large")));
        }
        MemoryContextSwitchTo(caller_context);
        replacement_tids = palloc(tids_bytes);
        memcpy(replacement_tids, build_state.tids.tids, tids_bytes);
        MemoryContextSwitchTo(build_context);
    }

    replacement_out->source_type = source_type;
    replacement_out->doc_tids = replacement_tids;
    replacement_out->num_docs = build_state.tids.len;
    replacement_out->index_bytes = index_bytes;
    replacement_out->index_bytes_len = index_bytes_len;
    if (keep_index_for_streaming)
    {
        replacement_out->index = index;
        replacement_out->index_valid = true;
        psql_bm25s_index_init(&index);
    }
    replacement_out->heap_tuples = heap_tuples;
    replacement_out->index_tuples = build_state.index_tuples;
    build_state.tids.tids = NULL;
    build_state.tids.len = 0;
    build_state.tids.capacity = 0;
    index_bytes = NULL;

    psql_bm25s_index_free(&index);
    psql_bm25s_tid_builder_free(&build_state.tids);
    MemoryContextSwitchTo(caller_context);
    MemoryContextDelete(build_context);
    psql_bm25s_am_release_unused_malloc();
}

static IndexBuildResult *
psql_bm25s_am_build_common(
    Relation heapRelation,
    Relation indexRelation,
    IndexInfo *indexInfo
)
{
    psql_bm25s_am_replacement replacement;
    psql_bm25s_am_meta_page current_meta;
    psql_bm25s_am_rebuild_builder builder;
    IndexBuildResult *result;
    uint16 cache_epoch;
    bool stream_build;

    builder = psql_bm25s_am_choose_explicit_build_builder(
        heapRelation,
        indexRelation
    );
    stream_build = builder != PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD;
    psql_bm25s_am_build_replacement(
        heapRelation,
        indexRelation,
        indexInfo,
        builder,
        stream_build,
        &replacement
    );
    cache_epoch = psql_bm25s_am_next_cache_epoch(indexRelation);
    /*
     * Explicit CREATE INDEX / REINDEX must use the same low-memory publish
     * path as online maintenance once the builder has been forced to compact
     * or spill. Otherwise a large controlled rebuild still allocates a full
     * serialized index copy at the final write stage.
     */
    if (replacement.index_valid)
    {
        psql_bm25s_am_write_relation_from_index(
            indexRelation,
            replacement.source_type,
            replacement.doc_tids,
            replacement.num_docs,
            &replacement.index,
            replacement.index_bytes_len,
            cache_epoch,
            0
        );
    }
    else
    {
        psql_bm25s_am_write_relation(
            indexRelation,
            replacement.source_type,
            replacement.doc_tids,
            replacement.num_docs,
            replacement.index_bytes,
            replacement.index_bytes_len,
            cache_epoch,
            0
        );
    }
    psql_bm25s_am_read_meta(indexRelation, &current_meta);
    psql_bm25s_am_shared_preload_retire_obsolete(
        indexRelation,
        &current_meta
    );

    result = palloc0(sizeof(*result));
    result->heap_tuples = replacement.heap_tuples;
    result->index_tuples = replacement.index_tuples;
    psql_bm25s_am_replacement_free(&replacement);
    return result;
}

static IndexBuildResult *
psql_bm25s_ambuild(
    Relation heapRelation,
    Relation indexRelation,
    IndexInfo *indexInfo
)
{
    return psql_bm25s_am_build_common(heapRelation, indexRelation, indexInfo);
}

static void
psql_bm25s_ambuildempty(Relation indexRelation)
{
    psql_bm25s_init_reloptions();
    psql_bm25s_am_write_relation(
        indexRelation,
        psql_bm25s_am_source_type(indexRelation),
        NULL,
        0,
        NULL,
        0,
        1,
        PSQL_BM25S_AM_FLAG_STALE
    );
}

static bool
psql_bm25s_aminsert(
    Relation indexRelation,
    Datum *values,
    bool *isnull,
    ItemPointer heap_tid,
    Relation heapRelation,
    IndexUniqueCheck checkUnique,
    bool indexUnchanged,
    IndexInfo *indexInfo
)
{
    (void) heapRelation;
    (void) checkUnique;

    psql_bm25s_init_reloptions();
    if (psql_bm25s_am_maintenance_tracking_enabled(indexRelation))
    {
        psql_bm25s_am_insert_state *state;
        int rebuild_threshold;
        int delta_bytes_threshold;
        int natts;
        bool has_index_values;
        bool skipped_delta_payload = false;

        rebuild_threshold = psql_bm25s_am_rebuild_threshold(indexRelation);
        delta_bytes_threshold =
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation);
        natts = psql_bm25s_am_index_natts(indexRelation);
        has_index_values = !psql_bm25s_am_all_index_values_null(isnull, natts);

        if (indexUnchanged &&
            has_index_values &&
            psql_bm25s_am_eventual_policy_enabled(indexRelation))
        {
            psql_bm25s_am_note_pending_maintenance_activity(
                indexRelation,
                1,
                0
            );
            state = psql_bm25s_am_get_insert_state(indexInfo);
            if (state != NULL)
            {
                state->needs_refresh = true;
            }
            return false;
        }

        /*
         * For eager policies, unchanged-key updates can stay transaction-local
         * until the scheduled refresh runs. Other backends should continue to
         * see the pre-update base index instead of blocking on in-flight
         * maintenance for rows whose indexed text did not change.
         */
        if (indexUnchanged &&
            has_index_values &&
            psql_bm25s_am_is_eager_refresh_policy(indexRelation))
        {
            state = psql_bm25s_am_get_insert_state(indexInfo);
            if (state != NULL)
            {
                state->needs_refresh = true;
                return false;
            }
        }

        if ((rebuild_threshold > 0 || delta_bytes_threshold > 0) &&
            has_index_values)
        {
            if (psql_bm25s_am_is_multicol_index(indexRelation))
            {
                ArrayType *fused_array;
                Oid source_type;
                psql_bm25s_text_options text_options;
                char **text_stopwords = NULL;
                size_t text_stopword_len = 0;

                source_type = psql_bm25s_am_source_type(indexRelation);
                psql_bm25s_text_options_init(&text_options);
                if (psql_bm25s_am_source_type_is_scalar_text(source_type))
                {
                    psql_bm25s_am_read_index_text_options(
                        indexRelation,
                        &text_options,
                        &text_stopwords,
                        &text_stopword_len
                    );
                }
                if (psql_bm25s_am_field_aware_enabled(indexRelation))
                {
                    fused_array = psql_bm25s_am_build_field_aware_text_array(
                        values,
                        isnull,
                        natts,
                        source_type,
                        &text_options
                    );
                }
                else
                {
                    fused_array = psql_bm25s_am_build_fused_text_array(
                        values,
                        isnull,
                        natts,
                        source_type,
                        &text_options
                    );
                }
                if (psql_bm25s_am_delta_record_value_fits(
                        (uint32) VARSIZE_ANY(fused_array)))
                {
                    psql_bm25s_am_note_delta_record(
                        indexRelation,
                        heap_tid,
                        fused_array,
                        (uint32) VARSIZE_ANY(fused_array)
                    );
                }
                else
                {
                    skipped_delta_payload = true;
                }
                pfree(fused_array);
                psql_bm25s_am_free_query_tokens(
                    text_stopwords,
                    text_stopword_len
                );
            }
            else
            {
                struct varlena *value_copy;

                value_copy = PG_DETOAST_DATUM_COPY(values[0]);
                if (psql_bm25s_am_delta_record_value_fits(
                        (uint32) VARSIZE_ANY(value_copy)))
                {
                    psql_bm25s_am_note_delta_record(
                        indexRelation,
                        heap_tid,
                        value_copy,
                        (uint32) VARSIZE_ANY(value_copy)
                    );
                }
                else
                {
                    skipped_delta_payload = true;
                }
                pfree(value_copy);
            }
        }
        /*
         * Oversized text values cannot fit in the compact delta page format.
         * In eventual count-threshold mode this is freshness debt for the next
         * threshold-driven rebuild. Byte-threshold policies need an immediate
         * due signal because there is no compact payload to add to delta_bytes.
         * Eager/realtime policies also mark stale because they promise a tighter
         * refresh path than eventual consistency.
         */
        if (skipped_delta_payload &&
            (!psql_bm25s_am_eventual_policy_enabled(indexRelation) ||
             delta_bytes_threshold > 0))
        {
            psql_bm25s_am_mark_stale(indexRelation);
        }
        psql_bm25s_am_note_maintenance_activity(indexRelation, 1, 0);
        psql_bm25s_am_schedule_background_maintenance(indexRelation);
        state = psql_bm25s_am_get_insert_state(indexInfo);
        if (state != NULL)
        {
            state->needs_refresh = true;
            return false;
        }
    }
    psql_bm25s_am_note_maintenance_activity(indexRelation, 1, 0);
    psql_bm25s_am_mark_stale(indexRelation);
    return false;
}

static void
psql_bm25s_aminsertcleanup(Relation indexRelation, IndexInfo *indexInfo)
{
    psql_bm25s_am_insert_state *state;

    psql_bm25s_init_reloptions();
    if (!psql_bm25s_am_maintenance_tracking_enabled(indexRelation))
    {
        return;
    }

    state = psql_bm25s_am_get_insert_state(indexInfo);
    if (state == NULL || !state->needs_refresh || state->refresh_running)
    {
        return;
    }

    state->needs_refresh = false;
    state->refresh_running = true;
    if (psql_bm25s_am_eventual_policy_enabled(indexRelation) &&
        !psql_bm25s_am_foreground_maintenance_enabled(indexRelation))
    {
        psql_bm25s_am_unschedule_refresh(RelationGetRelid(indexRelation));
        psql_bm25s_am_schedule_background_maintenance(indexRelation);
        state->refresh_running = false;
        return;
    }
    if (psql_bm25s_am_should_batch_refresh())
    {
        psql_bm25s_am_schedule_refresh(indexRelation);
    }
    else
    {
        CommandCounterIncrement();
        psql_bm25s_am_reindex_relation(indexRelation, indexInfo);
        psql_bm25s_am_unschedule_refresh(RelationGetRelid(indexRelation));
    }
    state->refresh_running = false;
}

static uint32
psql_bm25s_am_record_exact_deletes(
    Relation indexRelation,
    IndexBulkDeleteCallback callback,
    void *callback_state
)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload payload = {0};
    psql_bm25s_tid_builder delta_tids = {0};
    psql_bm25s_docid_builder deleted_doc_ids = {0};
    bool *deleted_flags = NULL;
    uint32 recorded = 0;
    size_t total_docs;
    size_t i;

    if (callback == NULL)
    {
        return 0;
    }

    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_load_payload(indexRelation, true, &payload);
    psql_bm25s_am_load_delta_state(
        indexRelation,
        &meta,
        meta.source_type,
        meta.delta_record_count,
        NULL,
        NULL,
        &delta_tids,
        &deleted_doc_ids
    );

    total_docs = payload.meta.num_docs + delta_tids.len;
    if (total_docs > 0)
    {
        deleted_flags = palloc0(sizeof(*deleted_flags) * total_docs);
    }
    for (i = 0; i < deleted_doc_ids.len; i++)
    {
        if ((size_t) deleted_doc_ids.doc_ids[i] >= total_docs)
        {
            psql_bm25s_tid_builder_free(&delta_tids);
            psql_bm25s_docid_builder_free(&deleted_doc_ids);
            psql_bm25s_am_payload_free(&payload);
            if (deleted_flags != NULL)
            {
                pfree(deleted_flags);
            }
            ereport(ERROR, (errmsg("psql_bm25s tombstone doc id out of range")));
        }
        deleted_flags[deleted_doc_ids.doc_ids[i]] = true;
    }

    for (i = 0; i < payload.meta.num_docs; i++)
    {
        if (!deleted_flags[i] && callback(&payload.doc_tids[i], callback_state))
        {
            psql_bm25s_am_note_tombstone_record(indexRelation, (uint32_t) i);
            deleted_flags[i] = true;
            recorded++;
        }
    }
    for (i = 0; i < delta_tids.len; i++)
    {
        size_t doc_id = payload.meta.num_docs + i;

        if (!deleted_flags[doc_id] &&
            callback(&delta_tids.tids[i], callback_state))
        {
            if (doc_id > UINT32_MAX)
            {
                psql_bm25s_tid_builder_free(&delta_tids);
                psql_bm25s_docid_builder_free(&deleted_doc_ids);
                psql_bm25s_am_payload_free(&payload);
                if (deleted_flags != NULL)
                {
                    pfree(deleted_flags);
                }
                ereport(ERROR, (errmsg("psql_bm25s tombstone doc id overflow")));
            }
            psql_bm25s_am_note_tombstone_record(
                indexRelation,
                (uint32_t) doc_id
            );
            deleted_flags[doc_id] = true;
            recorded++;
        }
    }

    psql_bm25s_tid_builder_free(&delta_tids);
    psql_bm25s_docid_builder_free(&deleted_doc_ids);
    psql_bm25s_am_payload_free(&payload);
    if (deleted_flags != NULL)
    {
        pfree(deleted_flags);
    }
    return recorded;
}

static IndexBulkDeleteResult *
psql_bm25s_ambulkdelete(
    IndexVacuumInfo *info,
    IndexBulkDeleteResult *stats,
    IndexBulkDeleteCallback callback,
    void *callback_state
)
{
    psql_bm25s_am_vacuum_stats *vacuum_stats;

    psql_bm25s_init_reloptions();
    vacuum_stats = psql_bm25s_am_get_vacuum_stats(stats);
    stats = &vacuum_stats->base;

    if (!info->analyze_only)
    {
        if (!vacuum_stats->delete_recorded)
        {
            psql_bm25s_am_meta_page meta;
            uint32 pending_delete_tuples = 0;

            psql_bm25s_am_read_meta(info->index, &meta);
            if (psql_bm25s_am_maintenance_tracking_enabled(info->index) &&
                (psql_bm25s_am_rebuild_threshold(info->index) > 0 ||
                 psql_bm25s_am_rebuild_delta_bytes_threshold(info->index) > 0) &&
                callback != NULL)
            {
                pending_delete_tuples = psql_bm25s_am_record_exact_deletes(
                    info->index,
                    callback,
                    callback_state
                );
            }
            else if (info->num_heap_tuples >= 0.0 &&
                     (double) meta.num_docs > info->num_heap_tuples)
            {
                double removed = (double) meta.num_docs - info->num_heap_tuples;

                if (removed >= (double) UINT32_MAX)
                {
                    pending_delete_tuples = UINT32_MAX;
                }
                else if (removed > 0.0)
                {
                    pending_delete_tuples = (uint32) removed;
                }
            }

            vacuum_stats->pending_delete_tuples = pending_delete_tuples;
            vacuum_stats->delete_recorded = true;
            if (pending_delete_tuples > 0)
            {
                psql_bm25s_am_note_maintenance_activity(
                    info->index,
                    0,
                    pending_delete_tuples
                );
            }
        }

        if (psql_bm25s_am_maintenance_tracking_enabled(info->index))
        {
            if (psql_bm25s_am_rebuild_threshold(info->index) <= 0)
            {
                vacuum_stats->needs_refresh = true;
            }
            else
            {
                vacuum_stats->needs_refresh =
                    psql_bm25s_am_should_flush_scheduled_oid(
                        RelationGetRelid(info->index)
                    );
            }
        }
        else
        {
            psql_bm25s_am_mark_stale(info->index);
        }
    }

    psql_bm25s_am_get_stats(info->index, stats);
    stats->estimated_count = info->estimated_count;
    return stats;
}

static IndexBulkDeleteResult *
psql_bm25s_amvacuumcleanup(
    IndexVacuumInfo *info,
    IndexBulkDeleteResult *stats
)
{
    psql_bm25s_am_vacuum_stats *vacuum_stats;

    psql_bm25s_init_reloptions();
    vacuum_stats = psql_bm25s_am_get_vacuum_stats(stats);
    stats = &vacuum_stats->base;

    if (!info->analyze_only &&
        psql_bm25s_am_maintenance_tracking_enabled(info->index) &&
        psql_bm25s_am_eventual_policy_enabled(info->index) &&
        !psql_bm25s_am_foreground_maintenance_enabled(info->index))
    {
        if (vacuum_stats->needs_refresh ||
            vacuum_stats->pending_delete_tuples > 0)
        {
            psql_bm25s_am_schedule_background_maintenance(info->index);
        }
        psql_bm25s_am_get_stats(info->index, stats);
        stats->estimated_count = info->estimated_count;
        return stats;
    }

    if (!info->analyze_only &&
        psql_bm25s_am_maintenance_tracking_enabled(info->index) &&
        (psql_bm25s_am_rebuild_threshold(info->index) <= 0 ||
         vacuum_stats->needs_refresh))
    {
        IndexInfo *indexInfo;

        indexInfo = BuildIndexInfo(info->index);
        psql_bm25s_am_reindex_relation(info->index, indexInfo);
        if (indexInfo != NULL)
        {
            pfree(indexInfo);
        }
        vacuum_stats->needs_refresh = false;
    }

    psql_bm25s_am_get_stats(info->index, stats);
    stats->estimated_count = info->estimated_count;
    return stats;
}

static bool
psql_bm25s_amcanreturn(Relation indexRelation, int attno)
{
    (void) indexRelation;
    (void) attno;
    return false;
}

static void
psql_bm25s_amcostestimate(
    PlannerInfo *root,
    IndexPath *path,
    double loop_count,
    Cost *indexStartupCost,
    Cost *indexTotalCost,
    Selectivity *indexSelectivity,
    double *indexCorrelation,
    double *indexPages
)
{
    GenericCosts costs = {0};

    genericcostestimate(root, path, loop_count, &costs);
    *indexStartupCost = costs.indexStartupCost;
    *indexTotalCost = costs.indexTotalCost;
    *indexSelectivity = costs.indexSelectivity;
    *indexCorrelation = costs.indexCorrelation;
    *indexPages = costs.numIndexPages;
}

static bool
psql_bm25s_amvalidate(Oid opclassoid)
{
    (void) opclassoid;
    return true;
}

static IndexScanDesc
psql_bm25s_ambeginscan(Relation indexRelation, int nkeys, int norderbys)
{
    IndexScanDesc scan = RelationGetIndexScan(indexRelation, nkeys, norderbys);

    scan->opaque = palloc0(sizeof(psql_bm25s_am_scan_opaque));
    if (norderbys > 0)
    {
        scan->xs_orderbyvals = palloc0(sizeof(Datum) * (size_t) norderbys);
        scan->xs_orderbynulls = palloc0(sizeof(bool) * (size_t) norderbys);
    }
    return scan;
}

static void
psql_bm25s_amrescan(
    IndexScanDesc scan,
    ScanKey keys,
    int nkeys,
    ScanKey orderbys,
    int norderbys
)
{
    psql_bm25s_am_scan_opaque *opaque = scan->opaque;

    if (opaque != NULL)
    {
        psql_bm25s_am_scan_opaque_reset(opaque);
    }
    scan->numberOfKeys = nkeys;
    scan->numberOfOrderBys = norderbys;
    scan->keyData = keys;
    scan->orderByData = orderbys;
}

static bool
psql_bm25s_amgettuple(IndexScanDesc scan, ScanDirection direction)
{
    psql_bm25s_am_scan_opaque *opaque = scan->opaque;
    ItemPointerData tid;
    uint32_t doc_id;
    float score;

    if (!ScanDirectionIsForward(direction))
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s ordered scans only support forward direction")
            )
        );
    }
    if (opaque == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s scan state is missing")));
    }
    if (!opaque->initialized)
    {
        psql_bm25s_am_scan_init(scan, opaque);
    }
    if (!psql_bm25s_am_scan_next_hit(opaque, &tid, &doc_id, &score))
    {
        return false;
    }

    ItemPointerCopy(&tid, &scan->xs_heaptid);
    scan->xs_recheck = false;
    scan->xs_recheckorderby = false;
    if (scan->numberOfOrderBys > 0 && scan->xs_orderbyvals != NULL &&
        scan->xs_orderbynulls != NULL)
    {
        scan->xs_orderbyvals[0] = Float8GetDatum(-(double) score);
        scan->xs_orderbynulls[0] = false;
    }

    (void) doc_id;
    return true;
}

static int64
psql_bm25s_amgetbitmap(IndexScanDesc scan, TIDBitmap *tbm)
{
    psql_bm25s_am_scan_opaque *opaque = scan->opaque;
    ItemPointerData tid;
    uint32_t doc_id;
    float score;
    int64 ntids = 0;

    if (tbm == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s bitmap scan requires a TID bitmap")));
    }
    if (opaque == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s scan state is missing")));
    }
    if (!opaque->initialized)
    {
        psql_bm25s_am_scan_init(scan, opaque);
    }

    while (psql_bm25s_am_scan_next_hit(opaque, &tid, &doc_id, &score))
    {
        tbm_add_tuples(tbm, &tid, 1, false);
        ntids++;
    }

    (void) doc_id;
    (void) score;
    return ntids;
}

static void
psql_bm25s_amendscan(IndexScanDesc scan)
{
    psql_bm25s_am_scan_opaque *opaque = scan->opaque;

    if (opaque != NULL)
    {
        psql_bm25s_am_scan_opaque_reset(opaque);
        pfree(opaque);
        scan->opaque = NULL;
    }
}

static inline size_t
psql_bm25s_am_query_term_token_len(
    const psql_bm25s_query_term *term,
    size_t index
)
{
    if (term->token_lens != NULL)
    {
        return term->token_lens[index];
    }

    return strlen(term->tokens[index]);
}

static void psql_bm25s_am_visibility_begin_with_snapshot(
    Relation heapRelation,
    Snapshot snapshot,
    psql_bm25s_am_visibility_ctx *visibility_out
);

static bool
psql_bm25s_am_tid_visible(
    psql_bm25s_am_visibility_ctx *visibility,
    const ItemPointerData *tid
)
{
    ItemPointerData heap_tid = *tid;
    bool call_again = false;
    bool all_dead = false;

    table_index_fetch_reset(visibility->fetch);
    do
    {
        if (table_index_fetch_tuple(
                visibility->fetch,
                &heap_tid,
                visibility->snapshot,
                visibility->slot,
                &call_again,
                &all_dead))
        {
            ExecClearTuple(visibility->slot);
            return true;
        }
        ExecClearTuple(visibility->slot);
    } while (call_again);

    return false;
}

static void
psql_bm25s_am_visibility_begin(
    Relation heapRelation,
    psql_bm25s_am_visibility_ctx *visibility_out
)
{
    psql_bm25s_am_visibility_begin_with_snapshot(
        heapRelation,
        GetActiveSnapshot(),
        visibility_out
    );
}

static void
psql_bm25s_am_visibility_begin_with_snapshot(
    Relation heapRelation,
    Snapshot snapshot,
    psql_bm25s_am_visibility_ctx *visibility_out
)
{
    memset(visibility_out, 0, sizeof(*visibility_out));
    visibility_out->fetch = table_index_fetch_begin(heapRelation);
    visibility_out->snapshot = snapshot;
    visibility_out->slot = MakeSingleTupleTableSlot(
        RelationGetDescr(heapRelation),
        &TTSOpsBufferHeapTuple
    );
}

static void
psql_bm25s_am_visibility_end(psql_bm25s_am_visibility_ctx *visibility)
{
    if (visibility == NULL)
    {
        return;
    }

    if (visibility->slot != NULL)
    {
        ExecDropSingleTupleTableSlot(visibility->slot);
        visibility->slot = NULL;
    }
    if (visibility->fetch != NULL)
    {
        table_index_fetch_end(visibility->fetch);
        visibility->fetch = NULL;
    }
    visibility->snapshot = NULL;
}

static void
psql_bm25s_am_collect_visible_hits(
    const ItemPointerData *doc_tids,
    const psql_bm25s_topk_result *ranked,
    size_t requested_k,
    psql_bm25s_am_visibility_ctx *visibility,
    psql_bm25s_search_state *state_out
)
{
    size_t i;
    size_t visible = 0;
    size_t alloc_len = Min(requested_k, ranked->len);

    memset(state_out, 0, sizeof(*state_out));
    if (alloc_len == 0)
    {
        return;
    }

    state_out->tids = palloc(sizeof(*state_out->tids) * alloc_len);
    state_out->topk.doc_ids = malloc(sizeof(*state_out->topk.doc_ids) * alloc_len);
    state_out->topk.scores = malloc(sizeof(*state_out->topk.scores) * alloc_len);
    if (state_out->topk.doc_ids == NULL || state_out->topk.scores == NULL)
    {
        if (state_out->topk.doc_ids != NULL)
        {
            free(state_out->topk.doc_ids);
        }
        if (state_out->topk.scores != NULL)
        {
            free(state_out->topk.scores);
        }
        if (state_out->tids != NULL)
        {
            pfree(state_out->tids);
        }
        ereport(ERROR, (errmsg("out of memory while building visible hit list")));
    }

    for (i = 0; i < ranked->len && visible < requested_k; i++)
    {
        uint32_t doc_id = ranked->doc_ids[i];

        if (!psql_bm25s_am_tid_visible(visibility, &doc_tids[doc_id]))
        {
            continue;
        }

        state_out->tids[visible] = doc_tids[doc_id];
        state_out->topk.doc_ids[visible] = doc_id;
        state_out->topk.scores[visible] = ranked->scores[i];
        visible++;
    }

    state_out->topk.len = visible;
}

static void
psql_bm25s_am_cache_reset_sparse_workspace(psql_bm25s_am_cache_entry *entry)
{
    size_t i;

    if (entry == NULL || entry->workspace.score_workspace == NULL)
    {
        return;
    }

    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        entry->workspace.score_workspace[entry->workspace.touched_doc_ids[i]] = 0.0f;
    }
    entry->workspace.touched_doc_len = 0;
}

static void
psql_bm25s_am_cache_ensure_sparse_workspace(
    psql_bm25s_am_cache_entry *entry
)
{
    MemoryContext oldcontext;
    size_t score_bytes;
    size_t touched_bytes;

    if (entry == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s cache entry is missing")));
    }

    if (entry->workspace.score_workspace == NULL)
    {
        score_bytes = sizeof(*entry->workspace.score_workspace) * entry->generation.index.num_docs;
        oldcontext = MemoryContextSwitchTo(entry->mcxt);
        entry->workspace.score_workspace = palloc0(score_bytes);
        MemoryContextSwitchTo(oldcontext);
        entry->workspace.last_used = GetCurrentTimestamp();
    }

    if (entry->workspace.touched_doc_capacity < entry->generation.index.num_docs)
    {
        touched_bytes = sizeof(*entry->workspace.touched_doc_ids) * entry->generation.index.num_docs;
        oldcontext = MemoryContextSwitchTo(entry->mcxt);
        if (entry->workspace.touched_doc_ids == NULL)
        {
            entry->workspace.touched_doc_ids = palloc(touched_bytes);
        }
        else
        {
            entry->workspace.touched_doc_ids = repalloc(entry->workspace.touched_doc_ids, touched_bytes);
        }
        MemoryContextSwitchTo(oldcontext);
        entry->workspace.touched_doc_capacity = entry->generation.index.num_docs;
        entry->workspace.last_used = GetCurrentTimestamp();
    }
}

static void
psql_bm25s_am_cache_ensure_candidate_workspace(
    psql_bm25s_am_cache_entry *entry
)
{
    MemoryContext oldcontext;
    size_t bytes;

    if (entry == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s cache entry is missing")));
    }

    if (entry->workspace.candidate_workspace != NULL)
    {
        return;
    }

    bytes = sizeof(*entry->workspace.candidate_workspace) * entry->generation.index.num_docs;
    oldcontext = MemoryContextSwitchTo(entry->mcxt);
    entry->workspace.candidate_workspace = palloc0(bytes);
    MemoryContextSwitchTo(oldcontext);
    entry->workspace.last_used = GetCurrentTimestamp();
}

static void
psql_bm25s_am_init_search_result(
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    memset(state_out, 0, sizeof(*state_out));
    if (requested_k == 0)
    {
        return;
    }

    state_out->tids = palloc(sizeof(*state_out->tids) * requested_k);
    state_out->topk.doc_ids = malloc(sizeof(*state_out->topk.doc_ids) * requested_k);
    state_out->topk.scores = malloc(sizeof(*state_out->topk.scores) * requested_k);
    if (state_out->topk.doc_ids == NULL || state_out->topk.scores == NULL)
    {
        psql_bm25s_am_search_state_reset(state_out);
        ereport(ERROR, (errmsg("out of memory while building search result")));
    }
}

static void
psql_bm25s_am_append_search_hit(
    psql_bm25s_search_state *state,
    size_t pos,
    const ItemPointerData *tid,
    uint32_t doc_id,
    float score
)
{
    state->tids[pos] = *tid;
    state->topk.doc_ids[pos] = doc_id;
    state->topk.scores[pos] = score;
}

static size_t
psql_bm25s_am_next_visibility_rank_limit(
    size_t current,
    size_t wanted,
    size_t max_rank
)
{
    size_t bump = Max(wanted, (size_t) 64);
    size_t bumped;
    size_t doubled;
    size_t next;

    if (current >= max_rank)
    {
        return max_rank;
    }

    doubled = current > max_rank / 2 ? max_rank : current * 2;
    bumped = max_rank - current < bump ? max_rank : current + bump;
    next = Max(doubled, bumped);
    return Min(next, max_rank);
}

static size_t
psql_bm25s_am_append_visible_ranked_hits(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_topk_result *ranked,
    size_t wanted_k,
    psql_bm25s_am_visibility_ctx *visibility,
    psql_bm25s_search_state *state_out
)
{
    size_t visible = 0;
    size_t i;

    for (i = 0; i < ranked->len && visible < wanted_k; i++)
    {
        uint32_t doc_id = ranked->doc_ids[i];

        if (!psql_bm25s_am_tid_visible(
                visibility,
                &entry->generation.doc_tids[doc_id]))
        {
            continue;
        }

        psql_bm25s_am_append_search_hit(
            state_out,
            visible,
            &entry->generation.doc_tids[doc_id],
            doc_id,
            ranked->scores[i]
        );
        visible++;
    }

    state_out->topk.len = visible;
    return visible;
}

static bool
psql_bm25s_am_sparse_weight_mask_supported(
    const float *weight_mask,
    size_t num_docs
)
{
    size_t i;

    if (weight_mask == NULL)
    {
        return true;
    }

    for (i = 0; i < num_docs; i++)
    {
        if (!isfinite(weight_mask[i]))
        {
            return false;
        }
    }

    return true;
}

static bool
psql_bm25s_am_sparse_weight_mask_nonnegative(
    const float *weight_mask,
    size_t num_docs
)
{
    size_t i;

    if (weight_mask == NULL)
    {
        return true;
    }

    for (i = 0; i < num_docs; i++)
    {
        if (weight_mask[i] < 0.0f)
        {
            return false;
        }
    }

    return true;
}

static bool
psql_bm25s_am_field_weights_supported(
    const float *field_weights,
    size_t num_fields
)
{
    size_t i;

    if (field_weights == NULL || num_fields == 0)
    {
        return false;
    }

    for (i = 0; i < num_fields; i++)
    {
        if (!isfinite(field_weights[i]))
        {
            return false;
        }
    }

    return true;
}

static bool
psql_bm25s_am_field_weights_nonnegative(
    const float *field_weights,
    size_t num_fields
)
{
    size_t i;

    if (field_weights == NULL || num_fields == 0)
    {
        return false;
    }

    for (i = 0; i < num_fields; i++)
    {
        if (field_weights[i] < 0.0f)
        {
            return false;
        }
    }

    return true;
}

static bool
psql_bm25s_am_sparse_posting_scores_strictly_positive(
    const psql_bm25s_am_cache_entry *entry
)
{
    /*
     * The unsigned fast path uses score_workspace == 0 as its touched sentinel.
     * That is only safe when every real posting contribution is strictly
     * positive. Lucene idf is positive for every indexed term; Robertson and
     * ATIRE can produce zero idf for very common terms.
     */
    return entry->generation.index.params.idf_method ==
        PSQL_BM25S_METHOD_LUCENE;
}

static bool
psql_bm25s_am_can_use_unsigned_sparse_path(
    const psql_bm25s_am_cache_entry *entry,
    const float *weight_mask
)
{
    return entry->generation.index.nonoccurrence == NULL &&
        psql_bm25s_am_sparse_posting_scores_strictly_positive(entry) &&
        psql_bm25s_am_sparse_weight_mask_nonnegative(
            weight_mask,
            entry->generation.index.num_docs
        );
}

static inline void
psql_bm25s_am_sparse_add_score(
    psql_bm25s_am_cache_entry *entry,
    uint32_t doc_id,
    float score
)
{
    if ((entry->workspace.candidate_workspace[doc_id] &
         PSQL_BM25S_WORKSPACE_TOUCHED) == 0)
    {
        entry->workspace.candidate_workspace[doc_id] |=
            PSQL_BM25S_WORKSPACE_TOUCHED;
        entry->workspace.touched_doc_ids[
            entry->workspace.touched_doc_len++
        ] = doc_id;
    }
    entry->workspace.score_workspace[doc_id] += score;
}

static inline void
psql_bm25s_am_unsigned_sparse_add_score(
    psql_bm25s_am_cache_entry *entry,
    uint32_t doc_id,
    float score
)
{
    if (entry->workspace.score_workspace[doc_id] == 0.0f)
    {
        entry->workspace.touched_doc_ids[
            entry->workspace.touched_doc_len++
        ] = doc_id;
    }
    entry->workspace.score_workspace[doc_id] += score;
}

static void
psql_bm25s_am_clear_sparse_candidate_flags(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len
)
{
    size_t i;

    if (entry == NULL || entry->workspace.candidate_workspace == NULL)
    {
        return;
    }

    for (i = 0; i < candidate_len; i++)
    {
        entry->workspace.candidate_workspace[candidate_ids[i]] &=
            (uint8_t) ~PSQL_BM25S_WORKSPACE_CANDIDATE;
    }
}

static void
psql_bm25s_am_apply_sparse_weight_mask(
    psql_bm25s_am_cache_entry *entry,
    const float *weight_mask
)
{
    size_t i;

    if (entry == NULL || weight_mask == NULL)
    {
        return;
    }

    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        uint32_t doc_id = entry->workspace.touched_doc_ids[i];

        entry->workspace.score_workspace[doc_id] *= weight_mask[doc_id];
    }
}

static void
psql_bm25s_am_cache_reset_signed_sparse_workspace(
    psql_bm25s_am_cache_entry *entry
)
{
    size_t i;

    if (entry == NULL || entry->workspace.score_workspace == NULL)
    {
        return;
    }

    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        uint32_t doc_id = entry->workspace.touched_doc_ids[i];

        entry->workspace.score_workspace[doc_id] = 0.0f;
        if (entry->workspace.candidate_workspace != NULL)
        {
            entry->workspace.candidate_workspace[doc_id] &=
                (uint8_t) ~PSQL_BM25S_WORKSPACE_TOUCHED;
        }
    }
    entry->workspace.touched_doc_len = 0;
}

static inline uint32_t *
psql_bm25s_am_filter_query_ids_for_sparse(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    size_t *filtered_len_out
)
{
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t i;
    size_t bytes;

    *filtered_len_out = 0;
    if (query_len > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                query_len,
                sizeof(*filtered_ids),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s query is too large")));
        }
        filtered_ids = malloc(bytes);
        if (filtered_ids == NULL)
        {
            ereport(ERROR, (errmsg("out of memory while filtering query ids")));
        }
    }

    for (i = 0; i < query_len; i++)
    {
        if (query_ids[i] < index->vocab_size)
        {
            filtered_ids[filtered_len++] = query_ids[i];
        }
    }

    if (filtered_len == 0 && index->has_empty_token)
    {
        filtered_ids = realloc(filtered_ids, sizeof(*filtered_ids));
        if (filtered_ids == NULL)
        {
            ereport(ERROR, (errmsg("out of memory while filtering query ids")));
        }
        filtered_ids[0] = index->empty_token_id;
        filtered_len = 1;
    }

    *filtered_len_out = filtered_len;
    return filtered_ids;
}

static float
psql_bm25s_am_sparse_nonoccurrence_sum(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len
)
{
    double sum = 0.0;
    size_t i;

    if (index->nonoccurrence == NULL)
    {
        return 0.0f;
    }

    for (i = 0; i < query_len; i++)
    {
        sum += (double) index->nonoccurrence[query_ids[i]];
    }

    return (float) sum;
}

static void
psql_bm25s_am_rank_signed_sparse_scores(
    psql_bm25s_am_cache_entry *entry,
    float base_score,
    bool finalize_scores,
    size_t requested_k,
    psql_bm25s_topk_result *ranked_out
)
{
    psql_bm25s_topk_result above_ranked = {0};
    psql_bm25s_topk_result below_ranked = {0};
    uint32_t *above_ids = NULL;
    uint32_t *below_ids = NULL;
    size_t above_len = 0;
    size_t below_len = 0;
    size_t wanted_k;
    size_t out_len = 0;
    size_t i;
    psql_bm25s_status status;

    wanted_k = Min(requested_k, (size_t) entry->generation.index.num_docs);
    memset(ranked_out, 0, sizeof(*ranked_out));
    if (wanted_k == 0)
    {
        return;
    }

    if (entry->workspace.touched_doc_len > 0)
    {
        size_t bytes;

        if (!psql_bm25s_am_checked_mul_size(
                entry->workspace.touched_doc_len,
                sizeof(*above_ids),
                &bytes))
        {
            psql_bm25s_am_oom();
        }
        above_ids = malloc(bytes);
        below_ids = malloc(bytes);
        if (above_ids == NULL || below_ids == NULL)
        {
            free(above_ids);
            free(below_ids);
            psql_bm25s_am_oom();
        }
    }

    /*
     * Dense scoring adds the BM25+ non-occurrence constant after doc masks.
     * Apply the same float operation only to touched docs; untouched docs all
     * share base_score and are merged by doc_id below.
     */
    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        uint32_t doc_id = entry->workspace.touched_doc_ids[i];
        float final_score = entry->workspace.score_workspace[doc_id];

        if (finalize_scores)
        {
            final_score += base_score;
            entry->workspace.score_workspace[doc_id] = final_score;
        }
        if (final_score > base_score)
        {
            above_ids[above_len++] = doc_id;
        }
        else if (final_score < base_score)
        {
            below_ids[below_len++] = doc_id;
        }
    }

    psql_bm25s_am_init_ranked_storage(wanted_k, ranked_out);
    if (above_len > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            above_ids,
            above_len,
            wanted_k,
            true,
            false,
            &above_ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            free(above_ids);
            free(below_ids);
            psql_bm25s_topk_result_free(ranked_out);
            ereport(ERROR, (errmsg("failed to compute sparse top-k: %s",
                                   psql_bm25s_strerror(status))));
        }

        for (i = 0; i < above_ranked.len && out_len < wanted_k; i++)
        {
            ranked_out->doc_ids[out_len] = above_ranked.doc_ids[i];
            ranked_out->scores[out_len] = above_ranked.scores[i];
            out_len++;
        }
    }

    for (i = 0;
         i < entry->generation.index.num_docs && out_len < wanted_k;
         i++)
    {
        if ((entry->workspace.candidate_workspace[i] &
             PSQL_BM25S_WORKSPACE_TOUCHED) != 0 &&
            entry->workspace.score_workspace[i] != base_score)
        {
            continue;
        }
        ranked_out->doc_ids[out_len] = (uint32_t) i;
        ranked_out->scores[out_len] = base_score;
        out_len++;
    }

    if (out_len < wanted_k && below_len > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            below_ids,
            below_len,
            wanted_k - out_len,
            true,
            false,
            &below_ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            free(above_ids);
            free(below_ids);
            psql_bm25s_topk_result_free(&above_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            ereport(ERROR, (errmsg("failed to compute sparse top-k: %s",
                                   psql_bm25s_strerror(status))));
        }

        for (i = 0; i < below_ranked.len && out_len < wanted_k; i++)
        {
            ranked_out->doc_ids[out_len] = below_ranked.doc_ids[i];
            ranked_out->scores[out_len] = below_ranked.scores[i];
            out_len++;
        }
    }

    ranked_out->len = out_len;
    psql_bm25s_topk_result_free(&above_ranked);
    psql_bm25s_topk_result_free(&below_ranked);
    free(above_ids);
    free(below_ids);
}

static void
psql_bm25s_am_rank_unsigned_sparse_candidates(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    size_t limit,
    bool positive_only,
    psql_bm25s_topk_result *ranked_out
)
{
    psql_bm25s_topk_result positive_ranked = {0};
    const uint32_t *scan_ids = NULL;
    uint32_t *filtered_ids = NULL;
    size_t scan_len = 0;
    uint32_t *zero_ids = NULL;
    size_t filtered_len = 0;
    size_t zero_len = 0;
    size_t out_len = 0;
    size_t wanted;
    size_t bytes;
    size_t i;
    bool rank_all_candidates;
    bool merge_all_candidates;
    psql_bm25s_status status;

    wanted = Min(limit, candidate_len);
    rank_all_candidates = !positive_only && wanted == candidate_len;
    /*
     * Keep the filtered-ordered hot path for the common Lucene/no-mask
     * case: rank positive matches, then append zero-score candidate docs by id.
     * Sorting every candidate is measurably slower for the small/medium
     * candidate slices produced by @@ filters.
     */
    merge_all_candidates = rank_all_candidates && weight_mask == NULL &&
        !entry->generation.index.has_empty_token;
    memset(ranked_out, 0, sizeof(*ranked_out));
    if (wanted == 0)
    {
        return;
    }

    /*
     * Fast path for the common Lucene-idf case: all posting scores are
     * strictly positive and untouched docs have the same zero score. Keep the
     * old hot path free from signed base-score bookkeeping; signed sparse
     * remains responsible for zero-idf methods, BM25+ non-occurrence, and
     * negative masks/weights.
     */
    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    if (!merge_all_candidates)
    {
        filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
            &entry->generation.index,
            query_ids,
            query_len,
            &filtered_len
        );
        scan_ids = filtered_ids;
        scan_len = filtered_len;
    }
    else
    {
        scan_ids = query_ids;
        scan_len = query_len;
    }

    if (merge_all_candidates || !rank_all_candidates)
    {
        psql_bm25s_am_init_ranked_storage(wanted, ranked_out);
    }

    for (i = 0; i < candidate_len; i++)
    {
        entry->workspace.candidate_workspace[candidate_ids[i]] |=
            PSQL_BM25S_WORKSPACE_CANDIDATE;
    }

    for (i = 0; i < scan_len; i++)
    {
        uint32_t token_id = scan_ids[i];
        uint64_t start;
        uint64_t end;
        uint64_t j;

        if (token_id >= entry->generation.index.vocab_size)
        {
            continue;
        }

        start = entry->generation.index.indptr[token_id];
        end = entry->generation.index.indptr[token_id + 1];
        for (j = start; j < end; j++)
        {
            uint32_t doc_id = entry->generation.index.indices[j];

            if ((entry->workspace.candidate_workspace[doc_id] &
                 PSQL_BM25S_WORKSPACE_CANDIDATE) == 0)
            {
                continue;
            }
            psql_bm25s_am_unsigned_sparse_add_score(
                entry,
                doc_id,
                entry->generation.index.data[j]
            );
        }
    }

    if (merge_all_candidates)
    {
        if (entry->workspace.touched_doc_len > 0)
        {
            size_t positive_wanted =
                Min(wanted, entry->workspace.touched_doc_len);

            status = psql_bm25s_topk_subset(
                entry->workspace.score_workspace,
                entry->workspace.touched_doc_ids,
                entry->workspace.touched_doc_len,
                positive_wanted,
                true,
                false,
                &positive_ranked
            );
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_topk_result_free(ranked_out);
                psql_bm25s_am_clear_sparse_candidate_flags(
                    entry,
                    candidate_ids,
                    candidate_len
                );
                psql_bm25s_am_cache_reset_sparse_workspace(entry);
                ereport(ERROR, (errmsg("failed to rank sparse candidates: %s",
                                       psql_bm25s_strerror(status))));
            }

            /*
             * This path is deliberately kept as the filtered-ordered hot path:
             * positive candidates first, then zero-score candidates by doc id.
             * The explicit bounds are a safety net for any unexpected
             * workspace contamination or duplicate touched ids; without them a
             * stale candidate bit could overrun the candidate-sized output
             * arrays and corrupt later backend state.
             */
            for (i = 0; i < positive_ranked.len && out_len < wanted; i++)
            {
                ranked_out->doc_ids[out_len] = positive_ranked.doc_ids[i];
                ranked_out->scores[out_len] = positive_ranked.scores[i];
                out_len++;
            }
        }

        for (i = 0; i < candidate_len && out_len < wanted; i++)
        {
            uint32_t doc_id = candidate_ids[i];

            if (entry->workspace.score_workspace[doc_id] == 0.0f)
            {
                ranked_out->doc_ids[out_len++] = doc_id;
            }
        }
        if (out_len > positive_ranked.len)
        {
            qsort(
                ranked_out->doc_ids + positive_ranked.len,
                out_len - positive_ranked.len,
                sizeof(*ranked_out->doc_ids),
                psql_bm25s_am_cmp_uint32_asc
            );
        }

        ranked_out->len = out_len;
        psql_bm25s_topk_result_free(&positive_ranked);
        psql_bm25s_am_clear_sparse_candidate_flags(
            entry,
            candidate_ids,
            candidate_len
        );
        psql_bm25s_am_cache_reset_sparse_workspace(entry);
        return;
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);

    if (rank_all_candidates)
    {
        /*
         * Filtered ordered scans usually ask for every candidate. Ranking the
         * candidate slice directly avoids the positive/zero merge overhead
         * while preserving zero-score docs and doc-id tie ordering.
         */
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            candidate_ids,
            candidate_len,
            wanted,
            true,
            false,
            ranked_out
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            free(filtered_ids);
            ereport(ERROR, (errmsg("failed to rank sparse candidates: %s",
                                   psql_bm25s_strerror(status))));
        }

        psql_bm25s_am_clear_sparse_candidate_flags(
            entry,
            candidate_ids,
            candidate_len
        );
        psql_bm25s_am_cache_reset_sparse_workspace(entry);
        free(filtered_ids);
        return;
    }

    if (entry->workspace.touched_doc_len > 0)
    {
        size_t positive_wanted = positive_only
            ? wanted
            : Min(wanted, entry->workspace.touched_doc_len);

        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            entry->workspace.touched_doc_ids,
            entry->workspace.touched_doc_len,
            positive_wanted,
            true,
            true,
            &positive_ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            free(filtered_ids);
            free(zero_ids);
            ereport(ERROR, (errmsg("failed to rank sparse candidates: %s",
                                   psql_bm25s_strerror(status))));
        }

        for (i = 0; i < positive_ranked.len && out_len < wanted; i++)
        {
            ranked_out->doc_ids[out_len] = positive_ranked.doc_ids[i];
            ranked_out->scores[out_len] = positive_ranked.scores[i];
            out_len++;
        }
    }

    if (!positive_only && out_len < wanted)
    {
        if (!psql_bm25s_am_checked_mul_size(
                candidate_len,
                sizeof(*zero_ids),
                &bytes))
        {
            psql_bm25s_topk_result_free(&positive_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            free(filtered_ids);
            psql_bm25s_am_oom();
        }
        zero_ids = malloc(bytes);
        if (zero_ids == NULL)
        {
            psql_bm25s_topk_result_free(&positive_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            free(filtered_ids);
            psql_bm25s_am_oom();
        }

        for (i = 0; i < candidate_len; i++)
        {
            uint32_t doc_id = candidate_ids[i];

            if (entry->workspace.score_workspace[doc_id] > 0.0f)
            {
                continue;
            }
            zero_ids[zero_len++] = doc_id;
        }
        qsort(
            zero_ids,
            zero_len,
            sizeof(*zero_ids),
            psql_bm25s_am_cmp_uint32_asc
        );

        for (i = 0; i < zero_len && out_len < wanted; i++)
        {
            ranked_out->doc_ids[out_len] = zero_ids[i];
            ranked_out->scores[out_len] = 0.0f;
            out_len++;
        }
    }

    ranked_out->len = out_len;
    psql_bm25s_topk_result_free(&positive_ranked);
    psql_bm25s_am_clear_sparse_candidate_flags(
        entry,
        candidate_ids,
        candidate_len
    );
    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    free(filtered_ids);
    free(zero_ids);
}

static void
psql_bm25s_am_rank_signed_sparse_candidates(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    size_t limit,
    bool positive_only,
    psql_bm25s_topk_result *ranked_out
)
{
    psql_bm25s_topk_result above_ranked = {0};
    psql_bm25s_topk_result below_ranked = {0};
    uint32_t *filtered_ids = NULL;
    uint32_t *above_ids = NULL;
    uint32_t *base_ids = NULL;
    uint32_t *below_ids = NULL;
    size_t filtered_len = 0;
    size_t above_len = 0;
    size_t base_len = 0;
    size_t below_len = 0;
    size_t out_len = 0;
    size_t wanted;
    size_t bytes;
    size_t i;
    float base_score;
    psql_bm25s_status status;

    wanted = Min(limit, candidate_len);
    memset(ranked_out, 0, sizeof(*ranked_out));
    if (wanted == 0)
    {
        return;
    }

    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
        &entry->generation.index,
        query_ids,
        query_len,
        &filtered_len
    );

    psql_bm25s_am_init_ranked_storage(wanted, ranked_out);
    for (i = 0; i < candidate_len; i++)
    {
        entry->workspace.candidate_workspace[candidate_ids[i]] |=
            PSQL_BM25S_WORKSPACE_CANDIDATE;
    }

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint64_t start = entry->generation.index.indptr[token_id];
        uint64_t end = entry->generation.index.indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = entry->generation.index.indices[j];

            if ((entry->workspace.candidate_workspace[doc_id] &
                 PSQL_BM25S_WORKSPACE_CANDIDATE) == 0)
            {
                continue;
            }
            psql_bm25s_am_sparse_add_score(
                entry,
                doc_id,
                entry->generation.index.data[j]
            );
        }
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);
    base_score = psql_bm25s_am_sparse_nonoccurrence_sum(
        &entry->generation.index,
        filtered_ids,
        filtered_len
    );

    if (entry->workspace.touched_doc_len > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                entry->workspace.touched_doc_len,
                sizeof(*above_ids),
                &bytes))
        {
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            psql_bm25s_topk_result_free(ranked_out);
            free(filtered_ids);
            psql_bm25s_am_oom();
        }
        above_ids = malloc(bytes);
        below_ids = malloc(bytes);
        if (above_ids == NULL || below_ids == NULL)
        {
            free(above_ids);
            free(below_ids);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            psql_bm25s_topk_result_free(ranked_out);
            free(filtered_ids);
            psql_bm25s_am_oom();
        }
    }

    /*
     * Candidate-restricted ranking must still account for untouched docs:
     * with BM25+ they carry base_score, and with negative weights/masks they
     * can outrank touched docs that were pushed below the base.
     */
    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        uint32_t doc_id = entry->workspace.touched_doc_ids[i];
        float final_score = entry->workspace.score_workspace[doc_id] +
            base_score;

        entry->workspace.score_workspace[doc_id] = final_score;
        if (positive_only && final_score <= 0.0f)
        {
            continue;
        }
        if (final_score > base_score)
        {
            above_ids[above_len++] = doc_id;
        }
        else if (final_score < base_score)
        {
            below_ids[below_len++] = doc_id;
        }
    }

    if (above_len > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            above_ids,
            above_len,
            wanted,
            true,
            false,
            &above_ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_topk_result_free(&above_ranked);
            psql_bm25s_topk_result_free(&below_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            free(filtered_ids);
            free(above_ids);
            free(base_ids);
            free(below_ids);
            ereport(ERROR, (errmsg("failed to rank sparse candidates: %s",
                                   psql_bm25s_strerror(status))));
        }

        for (i = 0; i < above_ranked.len && out_len < wanted; i++)
        {
            ranked_out->doc_ids[out_len] = above_ranked.doc_ids[i];
            ranked_out->scores[out_len] = above_ranked.scores[i];
            out_len++;
        }
    }

    if (out_len < wanted && (!positive_only || base_score > 0.0f))
    {
        if (!psql_bm25s_am_checked_mul_size(
                candidate_len,
                sizeof(*base_ids),
                &bytes))
        {
            psql_bm25s_topk_result_free(&above_ranked);
            psql_bm25s_topk_result_free(&below_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            free(filtered_ids);
            free(above_ids);
            free(below_ids);
            psql_bm25s_am_oom();
        }
        base_ids = malloc(bytes);
        if (base_ids == NULL)
        {
            psql_bm25s_topk_result_free(&above_ranked);
            psql_bm25s_topk_result_free(&below_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            free(filtered_ids);
            free(above_ids);
            free(below_ids);
            psql_bm25s_am_oom();
        }

        for (i = 0; i < candidate_len; i++)
        {
            uint32_t doc_id = candidate_ids[i];

            if ((entry->workspace.candidate_workspace[doc_id] &
                 PSQL_BM25S_WORKSPACE_TOUCHED) != 0 &&
                entry->workspace.score_workspace[doc_id] != base_score)
            {
                continue;
            }
            base_ids[base_len++] = doc_id;
        }
        qsort(
            base_ids,
            base_len,
            sizeof(*base_ids),
            psql_bm25s_am_cmp_uint32_asc
        );

        for (i = 0; i < base_len && out_len < wanted; i++)
        {
            ranked_out->doc_ids[out_len] = base_ids[i];
            ranked_out->scores[out_len] = base_score;
            out_len++;
        }
    }

    if (out_len < wanted && below_len > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            below_ids,
            below_len,
            wanted - out_len,
            true,
            false,
            &below_ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_topk_result_free(&above_ranked);
            psql_bm25s_topk_result_free(&below_ranked);
            psql_bm25s_topk_result_free(ranked_out);
            psql_bm25s_am_clear_sparse_candidate_flags(
                entry,
                candidate_ids,
                candidate_len
            );
            psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
            free(filtered_ids);
            free(above_ids);
            free(base_ids);
            free(below_ids);
            ereport(ERROR, (errmsg("failed to rank sparse candidates: %s",
                                   psql_bm25s_strerror(status))));
        }

        for (i = 0; i < below_ranked.len && out_len < wanted; i++)
        {
            ranked_out->doc_ids[out_len] = below_ranked.doc_ids[i];
            ranked_out->scores[out_len] = below_ranked.scores[i];
            out_len++;
        }
    }

    ranked_out->len = out_len;
    psql_bm25s_topk_result_free(&above_ranked);
    psql_bm25s_topk_result_free(&below_ranked);
    psql_bm25s_am_clear_sparse_candidate_flags(
        entry,
        candidate_ids,
        candidate_len
    );
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    free(filtered_ids);
    free(above_ids);
    free(base_ids);
    free(below_ids);
}

static void
psql_bm25s_am_rank_all_signed_sparse(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *query_ids,
    size_t query_len,
    psql_bm25s_topk_result *ranked_out
)
{
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t i;
    float base_score;

    memset(ranked_out, 0, sizeof(*ranked_out));
    if (entry->generation.index.num_docs == 0)
    {
        return;
    }

    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
        &entry->generation.index,
        query_ids,
        query_len,
        &filtered_len
    );

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint64_t start = entry->generation.index.indptr[token_id];
        uint64_t end = entry->generation.index.indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = entry->generation.index.indices[j];

            psql_bm25s_am_sparse_add_score(
                entry,
                doc_id,
                entry->generation.index.data[j]
            );
        }
    }

    base_score = psql_bm25s_am_sparse_nonoccurrence_sum(
        &entry->generation.index,
        filtered_ids,
        filtered_len
    );
    psql_bm25s_am_rank_signed_sparse_scores(
        entry,
        base_score,
        true,
        entry->generation.index.num_docs,
        ranked_out
    );
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    free(filtered_ids);
}

static void
psql_bm25s_am_prepare_search_state_unsigned_sparse(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t visible = 0;
    size_t rank_limit;
    size_t rank_max;
    size_t wanted_k;
    size_t i;
    psql_bm25s_status status;

    wanted_k = Min(requested_k, (size_t) entry->generation.index.num_docs);
    memset(state_out, 0, sizeof(*state_out));
    if (wanted_k == 0)
    {
        return;
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
        &entry->generation.index,
        query_ids,
        query_len,
        &filtered_len
    );

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint64_t start = entry->generation.index.indptr[token_id];
        uint64_t end = entry->generation.index.indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = entry->generation.index.indices[j];

            psql_bm25s_am_unsigned_sparse_add_score(
                entry,
                doc_id,
                entry->generation.index.data[j]
            );
        }
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);
    psql_bm25s_am_init_search_result(wanted_k, state_out);
    rank_max = entry->workspace.touched_doc_len;
    rank_limit = Min(wanted_k, rank_max);
    while (rank_limit > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            entry->workspace.touched_doc_ids,
            entry->workspace.touched_doc_len,
            rank_limit,
            true,
            true,
            &ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_visibility_end(&visibility);
            psql_bm25s_am_search_state_reset(state_out);
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            free(filtered_ids);
            ereport(ERROR, (errmsg("failed to compute sparse top-k: %s",
                                   psql_bm25s_strerror(status))));
        }

        visible = psql_bm25s_am_append_visible_ranked_hits(
            entry,
            &ranked,
            wanted_k,
            &visibility,
            state_out
        );
        if (visible >= wanted_k || rank_limit >= rank_max)
        {
            break;
        }

        /*
         * MVCC churn can hide some of the top-ranked TIDs. Expand gradually
         * instead of jumping from requested k to every positive-scoring doc.
         */
        psql_bm25s_topk_result_free(&ranked);
        psql_bm25s_am_search_state_reset(state_out);
        psql_bm25s_am_init_search_result(wanted_k, state_out);
        rank_limit = psql_bm25s_am_next_visibility_rank_limit(
            rank_limit,
            wanted_k,
            rank_max
        );
    }

    /*
     * Only zero-score docs can be appended after all positive candidates have
     * been considered. This preserves BM25 ordering under MVCC churn without
     * charging the normal low-churn path for a full positive ranking.
     */
    if (visible < wanted_k)
    {
        uint32_t doc_id;

        for (doc_id = 0;
             doc_id < entry->generation.index.num_docs && visible < wanted_k;
             doc_id++)
        {
            if (entry->workspace.score_workspace[doc_id] > 0.0f)
            {
                continue;
            }
            if (!psql_bm25s_am_tid_visible(
                    &visibility,
                    &entry->generation.doc_tids[doc_id]))
            {
                continue;
            }

            psql_bm25s_am_append_search_hit(
                state_out,
                visible,
                &entry->generation.doc_tids[doc_id],
                doc_id,
                0.0f
            );
            visible++;
        }
    }

    state_out->topk.len = visible;
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    psql_bm25s_topk_result_free(&ranked);
    free(filtered_ids);
}

static void
psql_bm25s_am_prepare_search_state_sparse(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t visible = 0;
    size_t wanted_k;
    size_t i;
    size_t rank_limit;
    size_t rank_max;
    float base_score;

    wanted_k = Min(requested_k, (size_t) entry->generation.index.num_docs);
    memset(state_out, 0, sizeof(*state_out));
    if (wanted_k == 0)
    {
        return;
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
        &entry->generation.index,
        query_ids,
        query_len,
        &filtered_len
    );

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint64_t start = entry->generation.index.indptr[token_id];
        uint64_t end = entry->generation.index.indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            uint32_t doc_id = entry->generation.index.indices[j];

            psql_bm25s_am_sparse_add_score(
                entry,
                doc_id,
                entry->generation.index.data[j]
            );
        }
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);
    base_score = psql_bm25s_am_sparse_nonoccurrence_sum(
        &entry->generation.index,
        filtered_ids,
        filtered_len
    );
    psql_bm25s_am_init_search_result(wanted_k, state_out);
    rank_max = entry->generation.index.num_docs;
    rank_limit = Min(wanted_k, rank_max);
    while (rank_limit > 0)
    {
        psql_bm25s_am_rank_signed_sparse_scores(
            entry,
            base_score,
            rank_limit == wanted_k,
            rank_limit,
            &ranked
        );
        visible = psql_bm25s_am_append_visible_ranked_hits(
            entry,
            &ranked,
            wanted_k,
            &visibility,
            state_out
        );
        if (visible >= wanted_k || rank_limit >= rank_max)
        {
            break;
        }

        psql_bm25s_topk_result_free(&ranked);
        psql_bm25s_am_search_state_reset(state_out);
        psql_bm25s_am_init_search_result(wanted_k, state_out);
        rank_limit = psql_bm25s_am_next_visibility_rank_limit(
            rank_limit,
            wanted_k,
            rank_max
        );
    }

    state_out->topk.len = visible;
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    psql_bm25s_topk_result_free(&ranked);
    free(filtered_ids);
}

static void
psql_bm25s_am_prepare_field_tokens_search_state_unsigned_sparse(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    const float *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    size_t wanted_k;
    size_t visible = 0;
    size_t rank_limit;
    size_t rank_max;
    size_t field_idx;
    size_t i;
    psql_bm25s_status status;

    wanted_k = Min(requested_k, (size_t) entry->generation.index.num_docs);
    memset(state_out, 0, sizeof(*state_out));
    if (wanted_k == 0)
    {
        return;
    }

    /*
     * Field-aware scoring has the same non-negative hot path when all field
     * weights are >= 0 and there is no BM25+ base score. Negative boosts must
     * stay on the signed path so zero-score docs can outrank penalized docs.
     */
    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);

    for (field_idx = 0; field_idx < num_fields; field_idx++)
    {
        char **field_tokens = NULL;
        uint32_t *query_ids = NULL;
        size_t query_id_len = 0;
        uint32_t *filtered_ids = NULL;
        size_t filtered_len = 0;
        size_t bytes;
        float field_weight = field_weights[field_idx];

        if (field_weight == 0.0f)
        {
            continue;
        }

        if (!psql_bm25s_am_checked_mul_size(
                query_len,
                sizeof(*field_tokens),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s query is too large")));
        }
        field_tokens = palloc0(bytes);
        for (i = 0; i < query_len; i++)
        {
            field_tokens[i] = psql_bm25s_am_make_field_token(
                field_indexes[field_idx],
                query_tokens[i]
            );
        }

        status = psql_bm25s_am_query_token_ids_cached(
            entry,
            field_tokens,
            query_len,
            &query_ids,
            &query_id_len
        );
        for (i = 0; i < query_len; i++)
        {
            pfree(field_tokens[i]);
        }
        pfree(field_tokens);
        if (status != PSQL_BM25S_OK)
        {
            ereport(ERROR, (errmsg("failed to resolve field query tokens: %s",
                                   psql_bm25s_strerror(status))));
        }

        filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
            &entry->generation.index,
            query_ids,
            query_id_len,
            &filtered_len
        );
        free(query_ids);

        for (i = 0; i < filtered_len; i++)
        {
            uint32_t token_id = filtered_ids[i];
            uint64_t start = entry->generation.index.indptr[token_id];
            uint64_t end = entry->generation.index.indptr[token_id + 1];
            uint64_t j;

            for (j = start; j < end; j++)
            {
                uint32_t doc_id = entry->generation.index.indices[j];
                float score = entry->generation.index.data[j] * field_weight;

                psql_bm25s_am_unsigned_sparse_add_score(
                    entry,
                    doc_id,
                    score
                );
            }
        }
        free(filtered_ids);
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);
    psql_bm25s_am_init_search_result(wanted_k, state_out);
    rank_max = entry->workspace.touched_doc_len;
    rank_limit = Min(wanted_k, rank_max);
    while (rank_limit > 0)
    {
        status = psql_bm25s_topk_subset(
            entry->workspace.score_workspace,
            entry->workspace.touched_doc_ids,
            entry->workspace.touched_doc_len,
            rank_limit,
            true,
            true,
            &ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_visibility_end(&visibility);
            psql_bm25s_am_search_state_reset(state_out);
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            ereport(ERROR, (errmsg("failed to compute field-aware top-k: %s",
                                   psql_bm25s_strerror(status))));
        }

        visible = psql_bm25s_am_append_visible_ranked_hits(
            entry,
            &ranked,
            wanted_k,
            &visibility,
            state_out
        );
        if (visible >= wanted_k || rank_limit >= rank_max)
        {
            break;
        }

        /*
         * Keep MVCC catch-up bounded. Full positive reranking is only reached
         * after several failed expansions, not for the first invisible top hit.
         */
        psql_bm25s_topk_result_free(&ranked);
        psql_bm25s_am_search_state_reset(state_out);
        psql_bm25s_am_init_search_result(wanted_k, state_out);
        rank_limit = psql_bm25s_am_next_visibility_rank_limit(
            rank_limit,
            wanted_k,
            rank_max
        );
    }

    /*
     * Field-aware unsigned scoring follows the same ordering rule as the
     * single-field path: all positive visible docs must be considered before
     * zero-score fallback docs are emitted.
     */
    if (visible < wanted_k)
    {
        uint32_t doc_id;

        for (doc_id = 0;
             doc_id < entry->generation.index.num_docs && visible < wanted_k;
             doc_id++)
        {
            if (entry->workspace.score_workspace[doc_id] > 0.0f)
            {
                continue;
            }
            if (!psql_bm25s_am_tid_visible(
                    &visibility,
                    &entry->generation.doc_tids[doc_id]))
            {
                continue;
            }

            psql_bm25s_am_append_search_hit(
                state_out,
                visible,
                &entry->generation.doc_tids[doc_id],
                doc_id,
                0.0f
            );
            visible++;
        }
    }

    state_out->topk.len = visible;
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    psql_bm25s_topk_result_free(&ranked);
}

static void
psql_bm25s_am_prepare_field_tokens_search_state_sparse(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    const float *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    size_t wanted_k;
    size_t visible = 0;
    size_t field_idx;
    size_t i;
    float base_score = 0.0f;
    size_t rank_limit;
    size_t rank_max;
    psql_bm25s_status status;

    wanted_k = Min(requested_k, (size_t) entry->generation.index.num_docs);
    memset(state_out, 0, sizeof(*state_out));
    if (wanted_k == 0)
    {
        return;
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);

    for (field_idx = 0; field_idx < num_fields; field_idx++)
    {
        char **field_tokens = NULL;
        uint32_t *query_ids = NULL;
        size_t query_id_len = 0;
        uint32_t *filtered_ids = NULL;
        size_t filtered_len = 0;
        size_t bytes;
        float field_weight = field_weights[field_idx];

        if (field_weight == 0.0f)
        {
            continue;
        }

        if (!psql_bm25s_am_checked_mul_size(
                query_len,
                sizeof(*field_tokens),
                &bytes))
        {
            ereport(ERROR, (errmsg("psql_bm25s query is too large")));
        }
        field_tokens = palloc0(bytes);
        for (i = 0; i < query_len; i++)
        {
            field_tokens[i] = psql_bm25s_am_make_field_token(
                field_indexes[field_idx],
                query_tokens[i]
            );
        }

        status = psql_bm25s_am_query_token_ids_cached(
            entry,
            field_tokens,
            query_len,
            &query_ids,
            &query_id_len
        );
        for (i = 0; i < query_len; i++)
        {
            pfree(field_tokens[i]);
        }
        pfree(field_tokens);
        if (status != PSQL_BM25S_OK)
        {
            ereport(ERROR, (errmsg("failed to resolve field query tokens: %s",
                                   psql_bm25s_strerror(status))));
        }

        filtered_ids = psql_bm25s_am_filter_query_ids_for_sparse(
            &entry->generation.index,
            query_ids,
            query_id_len,
            &filtered_len
        );
        free(query_ids);
        base_score += psql_bm25s_am_sparse_nonoccurrence_sum(
            &entry->generation.index,
            filtered_ids,
            filtered_len
        ) * field_weight;

        for (i = 0; i < filtered_len; i++)
        {
            uint32_t token_id = filtered_ids[i];
            uint64_t start;
            uint64_t end;
            uint64_t j;

            start = entry->generation.index.indptr[token_id];
            end = entry->generation.index.indptr[token_id + 1];
            for (j = start; j < end; j++)
            {
                uint32_t doc_id = entry->generation.index.indices[j];
                float score = entry->generation.index.data[j] * field_weight;

                psql_bm25s_am_sparse_add_score(entry, doc_id, score);
            }
        }
        free(filtered_ids);
    }

    psql_bm25s_am_apply_sparse_weight_mask(entry, weight_mask);
    psql_bm25s_am_init_search_result(wanted_k, state_out);
    rank_max = entry->generation.index.num_docs;
    rank_limit = Min(wanted_k, rank_max);
    while (rank_limit > 0)
    {
        psql_bm25s_am_rank_signed_sparse_scores(
            entry,
            base_score,
            rank_limit == wanted_k,
            rank_limit,
            &ranked
        );

        visible = psql_bm25s_am_append_visible_ranked_hits(
            entry,
            &ranked,
            wanted_k,
            &visibility,
            state_out
        );
        if (visible >= wanted_k || rank_limit >= rank_max)
        {
            break;
        }

        psql_bm25s_topk_result_free(&ranked);
        psql_bm25s_am_search_state_reset(state_out);
        psql_bm25s_am_init_search_result(wanted_k, state_out);
        rank_limit = psql_bm25s_am_next_visibility_rank_limit(
            rank_limit,
            wanted_k,
            rank_max
        );
    }

    state_out->topk.len = visible;
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_am_cache_reset_signed_sparse_workspace(entry);
    psql_bm25s_topk_result_free(&ranked);
}

static void
psql_bm25s_am_scan_opaque_reset(psql_bm25s_am_scan_opaque *opaque)
{
    if (opaque == NULL)
    {
        return;
    }

    psql_bm25s_query_free(&opaque->verify_query);
    psql_bm25s_am_free_query_tokens(
        opaque->verify_stopwords,
        opaque->verify_stopword_len
    );
    psql_bm25s_am_visibility_end(&opaque->visibility);
    psql_bm25s_topk_result_free(&opaque->ranked);
    free(opaque->deferred_candidate_ids);
    free(opaque->deferred_query_ids);
    if (opaque->heap_relation_owned && opaque->heap_relation != NULL)
    {
        table_close(opaque->heap_relation, AccessShareLock);
    }
    if (opaque->use_sparse && opaque->entry != NULL)
    {
        psql_bm25s_am_cache_reset_sparse_workspace(opaque->entry);
    }
    if (opaque->entry != NULL)
    {
        psql_bm25s_am_cache_maybe_shrink_workspace(opaque->entry);
        psql_bm25s_am_cache_release_lease(opaque->entry);
    }

    memset(opaque, 0, sizeof(*opaque));
}

static Datum
psql_bm25s_am_score_overlap_int4(ArrayType *doc_array, ArrayType *query_array)
{
    Datum *doc_datums = NULL;
    Datum *query_datums = NULL;
    bool *doc_nulls = NULL;
    bool *query_nulls = NULL;
    int doc_nelems = 0;
    int query_nelems = 0;
    int i;
    int overlap = 0;

    if (ARR_NDIM(doc_array) != 1 || ARR_NDIM(query_array) != 1)
    {
        ereport(
            ERROR,
            (
                errmsg("bm25 distance operator requires one-dimensional int4[] values")
            )
        );
    }

    deconstruct_array(
        doc_array,
        INT4OID,
        4,
        true,
        TYPALIGN_INT,
        &doc_datums,
        &doc_nulls,
        &doc_nelems
    );
    deconstruct_array(
        query_array,
        INT4OID,
        4,
        true,
        TYPALIGN_INT,
        &query_datums,
        &query_nulls,
        &query_nelems
    );

    for (i = 0; i < query_nelems; i++)
    {
        int j;

        if (query_nulls[i])
        {
            continue;
        }

        for (j = 0; j < doc_nelems; j++)
        {
            if (doc_nulls[j])
            {
                continue;
            }
            if (DatumGetInt32(doc_datums[j]) == DatumGetInt32(query_datums[i]))
            {
                overlap++;
                break;
            }
        }
    }

    if (doc_datums != NULL)
    {
        pfree(doc_datums);
    }
    if (doc_nulls != NULL)
    {
        pfree(doc_nulls);
    }
    if (query_datums != NULL)
    {
        pfree(query_datums);
    }
    if (query_nulls != NULL)
    {
        pfree(query_nulls);
    }

    return Float8GetDatum(-(double) overlap);
}

static Datum
psql_bm25s_am_score_overlap_text(ArrayType *doc_array, ArrayType *query_array)
{
    Datum *doc_datums = NULL;
    Datum *query_datums = NULL;
    bool *doc_nulls = NULL;
    bool *query_nulls = NULL;
    int doc_nelems = 0;
    int query_nelems = 0;
    int i;
    int overlap = 0;

    if (ARR_NDIM(doc_array) != 1 || ARR_NDIM(query_array) != 1)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "bm25 distance operator requires one-dimensional text[] or varchar[] values"
                )
            )
        );
    }

    (void) psql_bm25s_am_text_array_element_type(
        doc_array,
        "bm25 distance operator"
    );
    (void) psql_bm25s_am_text_array_element_type(
        query_array,
        "bm25 distance operator"
    );
    deconstruct_array(
        doc_array,
        ARR_ELEMTYPE(doc_array),
        -1,
        false,
        TYPALIGN_INT,
        &doc_datums,
        &doc_nulls,
        &doc_nelems
    );
    deconstruct_array(
        query_array,
        ARR_ELEMTYPE(query_array),
        -1,
        false,
        TYPALIGN_INT,
        &query_datums,
        &query_nulls,
        &query_nelems
    );

    for (i = 0; i < query_nelems; i++)
    {
        int j;
        text *query_text;

        if (query_nulls[i])
        {
            continue;
        }

        query_text = DatumGetTextPP(query_datums[i]);
        for (j = 0; j < doc_nelems; j++)
        {
            text *doc_text;

            if (doc_nulls[j])
            {
                continue;
            }

            doc_text = DatumGetTextPP(doc_datums[j]);
            if (VARSIZE_ANY_EXHDR(doc_text) == VARSIZE_ANY_EXHDR(query_text) &&
                memcmp(
                    VARDATA_ANY(doc_text),
                    VARDATA_ANY(query_text),
                    VARSIZE_ANY_EXHDR(doc_text)
                ) == 0)
            {
                overlap++;
                break;
            }
        }
    }

    if (doc_datums != NULL)
    {
        pfree(doc_datums);
    }
    if (doc_nulls != NULL)
    {
        pfree(doc_nulls);
    }
    if (query_datums != NULL)
    {
        pfree(query_datums);
    }
    if (query_nulls != NULL)
    {
        pfree(query_nulls);
    }

    return Float8GetDatum(-(double) overlap);
}

static void
psql_bm25s_am_scan_init(IndexScanDesc scan, psql_bm25s_am_scan_opaque *opaque)
{
    Oid source_type;
    ArrayType *query_array;
    size_t query_len = 0;
    size_t token_query_len = 0;
    uint32_t *query_ids = NULL;
    char **query_tokens = NULL;
    uint32_t *mapped_query_ids = NULL;
    size_t mapped_query_len = 0;
    size_t i;
    psql_bm25s_status status;

    if (psql_bm25s_am_is_multicol_index(scan->indexRelation))
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s operator scans do not support multicolumn fusion indexes"),
                errhint(
                    "Use direct regclass retrieval helpers; for field_aware "
                    "indexes use psql_bm25s_field_aware_query_tokens() or "
                    "psql_bm25s_field_aware_query()."
                )
            )
        );
    }

    if (scan->numberOfKeys > 0)
    {
        if (scan->numberOfOrderBys > 0)
        {
            psql_bm25s_am_scan_init_query_match_ordered(scan, opaque);
        }
        else
        {
            psql_bm25s_am_scan_init_query_match(scan, opaque);
        }
        opaque->initialized = true;
        return;
    }

    if (scan->numberOfOrderBys != 1 || scan->orderByData == NULL)
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s ordered scans require exactly one ORDER BY clause")
            )
        );
    }
    if ((scan->orderByData[0].sk_flags & SK_ISNULL) != 0)
    {
        ereport(ERROR, (errmsg("psql_bm25s ORDER BY query cannot be NULL")));
    }

    source_type = psql_bm25s_am_source_type(scan->indexRelation);
    psql_bm25s_am_validate_source_type(source_type);
    opaque->entry = psql_bm25s_am_get_cached_index(scan->indexRelation, source_type);
    opaque->heap_relation = scan->heapRelation;
    opaque->heap_relation_owned = false;
    if (opaque->heap_relation == NULL)
    {
        opaque->heap_relation = table_open(
            scan->indexRelation->rd_index->indrelid,
            AccessShareLock
        );
        opaque->heap_relation_owned = true;
    }
    psql_bm25s_am_visibility_begin_with_snapshot(
        opaque->heap_relation,
        scan->xs_snapshot != NULL ? scan->xs_snapshot : GetActiveSnapshot(),
        &opaque->visibility
    );

    query_array = DatumGetArrayTypeP(scan->orderByData[0].sk_argument);
    if (source_type == INT4ARRAYOID)
    {
        query_ids = psql_bm25s_array_read_query_ids(query_array, &query_len);
    }
    else
    {
        query_tokens = psql_bm25s_array_read_query_tokens(
            query_array,
            &query_len
        );
        token_query_len = query_len;
        status = psql_bm25s_am_query_token_ids_cached(
            opaque->entry,
            query_tokens,
            query_len,
            &mapped_query_ids,
            &mapped_query_len
        );
        if (status != PSQL_BM25S_OK)
        {
            ereport(ERROR, (errmsg("failed to map query tokens: %s",
                                   psql_bm25s_strerror(status))));
        }
        query_ids = mapped_query_ids;
        query_len = mapped_query_len;
    }

    opaque->use_sparse = psql_bm25s_am_can_use_unsigned_sparse_path(
        opaque->entry,
        NULL
    );
    if (opaque->use_sparse)
    {
        uint32_t *filtered_ids = NULL;
        size_t filtered_len = 0;

        psql_bm25s_am_cache_ensure_sparse_workspace(opaque->entry);
        psql_bm25s_am_cache_reset_sparse_workspace(opaque->entry);
        /*
         * This ordered-scan path is latency-sensitive enough that even a
         * non-inlined helper call is measurable. Keep the tiny filter loop
         * local and leave the shared helper for candidate/direct ranking paths
         * where correctness cleanup is more important than sub-millisecond
         * call overhead.
         */
        if (query_len > 0)
        {
            filtered_ids = malloc(sizeof(*filtered_ids) * query_len);
            if (filtered_ids == NULL)
            {
                ereport(ERROR, (errmsg("out of memory while filtering query ids")));
            }
        }

        for (i = 0; i < query_len; i++)
        {
            if (query_ids[i] < opaque->entry->generation.index.vocab_size)
            {
                filtered_ids[filtered_len++] = query_ids[i];
            }
        }

        for (i = 0; i < filtered_len; i++)
        {
            uint32_t token_id = filtered_ids[i];
            uint64_t start = opaque->entry->generation.index.indptr[token_id];
            uint64_t end = opaque->entry->generation.index.indptr[token_id + 1];
            uint64_t j;

            for (j = start; j < end; j++)
            {
                uint32_t doc_id = opaque->entry->generation.index.indices[j];

                if (opaque->entry->workspace.score_workspace[doc_id] == 0.0f)
                {
                    opaque->entry->workspace.touched_doc_ids[
                        opaque->entry->workspace.touched_doc_len++
                    ] = doc_id;
                }
                opaque->entry->workspace.score_workspace[doc_id] +=
                    opaque->entry->generation.index.data[j];
            }
        }

        status = psql_bm25s_topk_subset(
            opaque->entry->workspace.score_workspace,
            opaque->entry->workspace.touched_doc_ids,
            opaque->entry->workspace.touched_doc_len,
            opaque->entry->workspace.touched_doc_len,
            true,
            true,
            &opaque->ranked
        );
        free(filtered_ids);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_cache_reset_sparse_workspace(opaque->entry);
            ereport(ERROR, (errmsg("failed to compute ordered sparse scan: %s",
                                   psql_bm25s_strerror(status))));
        }
    }
    else
    {
        /*
         * BM25+ non-occurrence gives every untouched document a shared base
         * score. Build a complete signed sparse ranking and disable the old
         * zero-tail iterator so the scan does not duplicate base-score docs.
         */
        psql_bm25s_am_rank_all_signed_sparse(
            opaque->entry,
            query_ids,
            query_len,
            &opaque->ranked
        );
    }

    if (query_tokens != NULL)
    {
        for (i = 0; i < token_query_len; i++)
        {
            if (query_tokens[i] != NULL)
            {
                pfree(query_tokens[i]);
            }
        }
        pfree(query_tokens);
    }
    if (mapped_query_ids != NULL)
    {
        free(mapped_query_ids);
    }
    else if (query_ids != NULL)
    {
        pfree(query_ids);
    }

    opaque->initialized = true;
}

static bool
psql_bm25s_am_scan_next_hit(
    psql_bm25s_am_scan_opaque *opaque,
    ItemPointerData *tid_out,
    uint32_t *doc_id_out,
    float *score_out
)
{
    for (;;)
    {
        while (opaque->ranked_pos < opaque->ranked.len)
        {
            size_t pos = opaque->ranked_pos++;
            uint32_t doc_id = opaque->ranked.doc_ids[pos];

            if (!psql_bm25s_am_tid_visible(&opaque->visibility,
                                           &opaque->entry->generation.doc_tids[doc_id]))
            {
                continue;
            }
            if (opaque->verify_query_active &&
                !psql_bm25s_am_scan_doc_matches_verify_query(
                    opaque,
                    &opaque->entry->generation.doc_tids[doc_id]))
            {
                continue;
            }

            *tid_out = opaque->entry->generation.doc_tids[doc_id];
            *doc_id_out = doc_id;
            *score_out = opaque->ranked.scores[pos];
            return true;
        }

        if (psql_bm25s_am_scan_expand_deferred_ordered_ranking(opaque))
        {
            continue;
        }

        if (!opaque->use_sparse)
        {
            return false;
        }

        while (opaque->next_zero_doc_id < opaque->entry->generation.index.num_docs)
        {
            uint32_t doc_id = opaque->next_zero_doc_id++;

            if (opaque->entry->workspace.score_workspace[doc_id] > 0.0f)
            {
                continue;
            }
            if (!psql_bm25s_am_tid_visible(&opaque->visibility,
                                           &opaque->entry->generation.doc_tids[doc_id]))
            {
                continue;
            }
            if (opaque->verify_query_active &&
                !psql_bm25s_am_scan_doc_matches_verify_query(
                    opaque,
                    &opaque->entry->generation.doc_tids[doc_id]))
            {
                continue;
            }

            *tid_out = opaque->entry->generation.doc_tids[doc_id];
            *doc_id_out = doc_id;
            *score_out = 0.0f;
            return true;
        }

        return false;
    }
}

static ArrayType *
psql_bm25s_am_float4_array_from_values(const float4 *items, size_t len)
{
    Datum *values;
    size_t i;
    ArrayType *array;

    if (len == 0)
    {
        return construct_empty_array(FLOAT4OID);
    }

    values = palloc(sizeof(*values) * len);
    for (i = 0; i < len; i++)
    {
        values[i] = Float4GetDatum(items[i]);
    }
    array = construct_array(
        values,
        (int) len,
        FLOAT4OID,
        4,
        true,
        TYPALIGN_INT
    );
    pfree(values);
    return array;
}

static ArrayType *
psql_bm25s_am_int4_array_from_values(const int32 *items, size_t len)
{
    Datum *values;
    size_t i;
    ArrayType *array;

    if (len == 0)
    {
        return construct_empty_array(INT4OID);
    }

    values = palloc(sizeof(*values) * len);
    for (i = 0; i < len; i++)
    {
        values[i] = Int32GetDatum(items[i]);
    }
    array = construct_array(
        values,
        (int) len,
        INT4OID,
        4,
        true,
        TYPALIGN_INT
    );
    pfree(values);
    return array;
}

static ArrayType *
psql_bm25s_am_text_array_from_cstrings(char **tokens, size_t len)
{
    Datum *values;
    size_t i;
    ArrayType *array;

    if (len == 0)
    {
        return construct_empty_array(TEXTOID);
    }

    values = palloc(sizeof(*values) * len);
    for (i = 0; i < len; i++)
    {
        values[i] = CStringGetTextDatum(tokens[i]);
    }
    array = construct_array(
        values,
        (int) len,
        TEXTOID,
        -1,
        false,
        TYPALIGN_INT
    );
    pfree(values);
    return array;
}

static void
psql_bm25s_am_finalize_text_options(
    bool lowercase,
    bool stem_english,
    bool fold_diacritics,
    char **stopwords,
    size_t stopword_len,
    psql_bm25s_text_options *options_out,
    char ***stopwords_out,
    size_t *num_stopwords_out
)
{
    psql_bm25s_text_options_init(options_out);
    options_out->lowercase = lowercase;
    options_out->stem_english = stem_english;
    options_out->fold_diacritics = fold_diacritics;
    *stopwords_out = stopwords;
    *num_stopwords_out = stopword_len;

    if (stopwords == NULL)
    {
        return;
    }

    if (lowercase || stem_english || fold_diacritics)
    {
        size_t i;
        psql_bm25s_text_options stopword_options;

        psql_bm25s_text_options_init(&stopword_options);
        stopword_options.lowercase = lowercase;
        stopword_options.stem_english = stem_english;
        stopword_options.fold_diacritics = fold_diacritics;

        for (i = 0; i < stopword_len; i++)
        {
            char *normalized = NULL;
            char *pg_normalized;
            bool keep = false;
            psql_bm25s_status status;

            status = psql_bm25s_normalize_token(
                stopwords[i],
                &stopword_options,
                &normalized,
                &keep
            );
            if (status != PSQL_BM25S_OK || !keep || normalized == NULL)
            {
                psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
                ereport(
                    ERROR,
                    (
                        errmsg(
                            "failed to normalize stopwords: %s",
                            psql_bm25s_strerror(status)
                        )
                    )
                );
            }

            pg_normalized = pstrdup(normalized);
            free(normalized);
            pfree(stopwords[i]);
            stopwords[i] = pg_normalized;
        }
    }

    options_out->stopwords = (const char * const *) stopwords;
    options_out->num_stopwords = stopword_len;
}

static char **
psql_bm25s_am_parse_stopwords_string(
    const char *raw_stopwords,
    size_t *stopword_len_out
)
{
    char **stopwords = NULL;
    size_t len = 0;
    size_t capacity = 0;
    const char *cursor;

    if (stopword_len_out == NULL)
    {
        ereport(ERROR, (errmsg("stopword length output is missing")));
    }
    *stopword_len_out = 0;
    if (raw_stopwords == NULL || raw_stopwords[0] == '\0')
    {
        return NULL;
    }

    cursor = raw_stopwords;
    while (*cursor != '\0')
    {
        const char *start = cursor;
        const char *end;
        size_t token_len;
        char *token;

        while (*start != '\0' && isspace((unsigned char) *start))
        {
            start++;
        }
        end = start;
        while (*end != '\0' && *end != ',')
        {
            end++;
        }
        while (end > start && isspace((unsigned char) end[-1]))
        {
            end--;
        }

        token_len = (size_t) (end - start);
        if (token_len > 0)
        {
            if (len == capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                stopwords = repalloc(
                    stopwords,
                    sizeof(*stopwords) * capacity
                );
            }

            token = palloc(token_len + 1);
            memcpy(token, start, token_len);
            token[token_len] = '\0';
            stopwords[len++] = token;
        }

        cursor = *end == ',' ? end + 1 : end;
    }

    *stopword_len_out = len;
    return stopwords;
}

static void
psql_bm25s_am_read_index_text_options(
    Relation indexRelation,
    psql_bm25s_text_options *options_out,
    char ***stopwords_out,
    size_t *num_stopwords_out
)
{
    psql_bm25s_am_options *options;
    char *raw_stopwords = NULL;
    char **stopwords = NULL;
    size_t stopword_len = 0;

    options = (psql_bm25s_am_options *) indexRelation->rd_options;
    if (options != NULL &&
        psql_bm25s_am_relation_has_explicit_reloptions(indexRelation))
    {
        raw_stopwords = GET_STRING_RELOPTION(options, text_stopwords);
        if (raw_stopwords != NULL)
        {
            stopwords = psql_bm25s_am_parse_stopwords_string(
                raw_stopwords,
                &stopword_len
            );
        }
        psql_bm25s_am_finalize_text_options(
            options->text_lowercase,
            options->text_stem_english,
            options->text_fold_diacritics,
            stopwords,
            stopword_len,
            options_out,
            stopwords_out,
            num_stopwords_out
        );
        return;
    }

    psql_bm25s_am_finalize_text_options(
        true,
        false,
        false,
        NULL,
        0,
        options_out,
        stopwords_out,
        num_stopwords_out
    );
}

static void
psql_bm25s_am_read_text_options(
    bool lowercase,
    bool stem_english,
    bool fold_diacritics,
    ArrayType *stopwords_array,
    psql_bm25s_text_options *options_out,
    char ***stopwords_out,
    size_t *num_stopwords_out
)
{
    if (stopwords_array == NULL)
    {
        psql_bm25s_am_finalize_text_options(
            lowercase,
            stem_english,
            fold_diacritics,
            NULL,
            0,
            options_out,
            stopwords_out,
            num_stopwords_out
        );
        return;
    }

    *stopwords_out = psql_bm25s_array_read_query_tokens(
        stopwords_array,
        num_stopwords_out
    );
    psql_bm25s_am_finalize_text_options(
        lowercase,
        stem_english,
        fold_diacritics,
        *stopwords_out,
        *num_stopwords_out,
        options_out,
        stopwords_out,
        num_stopwords_out
    );
}

static char *
psql_bm25s_hybrid_tid_text(const ItemPointerData *tid)
{
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "(%u,%u)",
        BlockIdGetBlockNumber(&tid->ip_blkid),
        ItemPointerGetOffsetNumber(tid)
    );
    return pstrdup(buffer);
}

static int
psql_bm25s_hybrid_strcmp(const char *left, const char *right)
{
    if (left == NULL && right == NULL)
    {
        return 0;
    }
    if (left == NULL)
    {
        return -1;
    }
    if (right == NULL)
    {
        return 1;
    }
    return strcmp(left, right);
}

static int
psql_bm25s_hybrid_cmp_double_asc(double left, double right)
{
    if (left < right)
    {
        return -1;
    }
    if (left > right)
    {
        return 1;
    }
    return 0;
}

static int
psql_bm25s_hybrid_cmp_double_desc(double left, double right)
{
    return psql_bm25s_hybrid_cmp_double_asc(right, left);
}

static int
psql_bm25s_hybrid_parse_fusion(const char *value)
{
    const char *effective = value == NULL ? "rrf" : value;

    if (pg_strcasecmp(effective, "rrf") == 0)
    {
        return PSQL_BM25S_HYBRID_FUSION_RRF;
    }
    if (pg_strcasecmp(effective, "score") == 0)
    {
        return PSQL_BM25S_HYBRID_FUSION_SCORE;
    }
    ereport(
        ERROR,
        (
            errmsg("unsupported psql_bm25s hybrid fusion method: %s",
                   effective),
            errhint("Use rrf or score.")
        )
    );
    return PSQL_BM25S_HYBRID_FUSION_RRF;
}

static int
psql_bm25s_hybrid_parse_normalizer(const char *value)
{
    const char *effective = value == NULL ? "identity" : value;

    if (pg_strcasecmp(effective, "identity") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_IDENTITY;
    }
    if (pg_strcasecmp(effective, "negative_distance") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_NEGATIVE_DISTANCE;
    }
    if (pg_strcasecmp(effective, "inverse_distance") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_INVERSE_DISTANCE;
    }
    if (pg_strcasecmp(effective, "minmax") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_MINMAX;
    }
    if (pg_strcasecmp(effective, "zscore") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_ZSCORE;
    }
    if (pg_strcasecmp(effective, "rank") == 0)
    {
        return PSQL_BM25S_HYBRID_NORMALIZER_RANK;
    }
    ereport(
        ERROR,
        (
            errmsg("unsupported psql_bm25s hybrid normalizer: %s",
                   effective),
            errhint("Use identity, negative_distance, inverse_distance, "
                    "minmax, zscore, or rank.")
        )
    );
    return PSQL_BM25S_HYBRID_NORMALIZER_IDENTITY;
}

static int
psql_bm25s_hybrid_parse_direction(const char *value)
{
    const char *effective = value == NULL ? "higher_is_better" : value;

    if (pg_strcasecmp(effective, "higher_is_better") == 0)
    {
        return PSQL_BM25S_HYBRID_DIRECTION_HIGHER;
    }
    if (pg_strcasecmp(effective, "lower_is_better") == 0)
    {
        return PSQL_BM25S_HYBRID_DIRECTION_LOWER;
    }
    ereport(
        ERROR,
        (
            errmsg("unsupported psql_bm25s hybrid direction: %s",
                   effective),
            errhint("Use higher_is_better or lower_is_better.")
        )
    );
    return PSQL_BM25S_HYBRID_DIRECTION_HIGHER;
}

static int
psql_bm25s_hybrid_candidate_rank_cmp(const void *left, const void *right)
{
    const psql_bm25s_hybrid_candidate_state *a = left;
    const psql_bm25s_hybrid_candidate_state *b = right;
    int cmp;

    cmp = psql_bm25s_hybrid_strcmp(a->source_name, b->source_name);
    if (cmp != 0)
    {
        return cmp;
    }
    if (a->direction != b->direction)
    {
        return a->direction == PSQL_BM25S_HYBRID_DIRECTION_LOWER ? -1 : 1;
    }
    if (a->direction == PSQL_BM25S_HYBRID_DIRECTION_LOWER)
    {
        cmp = psql_bm25s_hybrid_cmp_double_asc(a->raw_value, b->raw_value);
    }
    else
    {
        cmp = psql_bm25s_hybrid_cmp_double_desc(a->raw_value, b->raw_value);
    }
    if (cmp != 0)
    {
        return cmp;
    }
    return psql_bm25s_hybrid_strcmp(a->tid_text, b->tid_text);
}

static int
psql_bm25s_hybrid_candidate_dedup_cmp(const void *left, const void *right)
{
    const psql_bm25s_hybrid_candidate_state *a = left;
    const psql_bm25s_hybrid_candidate_state *b = right;
    int cmp;

    cmp = psql_bm25s_hybrid_strcmp(a->source_name, b->source_name);
    if (cmp != 0)
    {
        return cmp;
    }
    cmp = psql_bm25s_hybrid_strcmp(a->tid_text, b->tid_text);
    if (cmp != 0)
    {
        return cmp;
    }
    cmp = psql_bm25s_hybrid_cmp_double_desc(
        a->weighted_score,
        b->weighted_score
    );
    if (cmp != 0)
    {
        return cmp;
    }
    if (a->source_rank < b->source_rank)
    {
        return -1;
    }
    if (a->source_rank > b->source_rank)
    {
        return 1;
    }
    if (a->direction != b->direction)
    {
        return a->direction == PSQL_BM25S_HYBRID_DIRECTION_LOWER ? -1 : 1;
    }
    if (a->direction == PSQL_BM25S_HYBRID_DIRECTION_LOWER)
    {
        return psql_bm25s_hybrid_cmp_double_asc(
            a->raw_value,
            b->raw_value
        );
    }
    return psql_bm25s_hybrid_cmp_double_desc(a->raw_value, b->raw_value);
}

static int
psql_bm25s_hybrid_candidate_tid_source_cmp(
    const void *left,
    const void *right
)
{
    const psql_bm25s_hybrid_candidate_state *a = left;
    const psql_bm25s_hybrid_candidate_state *b = right;
    int cmp;

    cmp = psql_bm25s_hybrid_strcmp(a->tid_text, b->tid_text);
    if (cmp != 0)
    {
        return cmp;
    }
    return psql_bm25s_hybrid_strcmp(a->source_name, b->source_name);
}

static int
psql_bm25s_hybrid_hit_cmp(const void *left, const void *right)
{
    const psql_bm25s_hybrid_hit_state *a = left;
    const psql_bm25s_hybrid_hit_state *b = right;
    int cmp;

    cmp = psql_bm25s_hybrid_cmp_double_desc(a->score, b->score);
    if (cmp != 0)
    {
        return cmp;
    }
    return psql_bm25s_hybrid_strcmp(a->tid_text, b->tid_text);
}

static size_t
psql_bm25s_hybrid_find_or_add_source(
    psql_bm25s_hybrid_source_state *sources,
    size_t *source_len,
    const char *source_name
)
{
    size_t i;

    for (i = 0; i < *source_len; i++)
    {
        if (strcmp(sources[i].source_name, source_name) == 0)
        {
            return i;
        }
    }

    sources[*source_len].source_name = pstrdup(source_name);
    sources[*source_len].min_value = DBL_MAX;
    sources[*source_len].max_value = -DBL_MAX;
    sources[*source_len].sum_value = 0.0;
    sources[*source_len].sumsq_value = 0.0;
    sources[*source_len].count = 0;
    (*source_len)++;
    return *source_len - 1;
}

static void
psql_bm25s_hybrid_deform_candidate(
    Datum candidate_datum,
    psql_bm25s_hybrid_candidate_state *candidate_out,
    psql_bm25s_hybrid_source_state *sources,
    size_t *source_len
)
{
    HeapTupleHeader header = DatumGetHeapTupleHeader(candidate_datum);
    Oid tuple_type = HeapTupleHeaderGetTypeId(header);
    int32 tuple_typmod = HeapTupleHeaderGetTypMod(header);
    TupleDesc tupdesc = lookup_rowtype_tupdesc(tuple_type, tuple_typmod);
    HeapTupleData tuple;
    Datum values[7];
    bool nulls[7];
    char *source_name = NULL;
    char *normalizer = NULL;
    char *direction = NULL;

    memset(candidate_out, 0, sizeof(*candidate_out));
    if (tupdesc->natts != 7)
    {
        ReleaseTupleDesc(tupdesc);
        ereport(ERROR, (errmsg("invalid psql_bm25s hybrid candidate type")));
    }

    tuple.t_len = HeapTupleHeaderGetDatumLength(header);
    ItemPointerSetInvalid(&tuple.t_self);
    tuple.t_tableOid = InvalidOid;
    tuple.t_data = header;
    heap_deform_tuple(&tuple, tupdesc, values, nulls);

    if (nulls[1] || nulls[2])
    {
        candidate_out->scored = false;
        ReleaseTupleDesc(tupdesc);
        return;
    }

    if (!nulls[0])
    {
        source_name = text_to_cstring(DatumGetTextPP(values[0]));
    }
    if (source_name == NULL || source_name[0] == '\0')
    {
        source_name = pstrdup("source");
    }

    candidate_out->source_name = source_name;
    candidate_out->tid = *DatumGetItemPointer(values[1]);
    candidate_out->tid_text = psql_bm25s_hybrid_tid_text(&candidate_out->tid);
    candidate_out->raw_value = (double) DatumGetFloat4(values[2]);
    if (!isfinite(candidate_out->raw_value))
    {
        candidate_out->scored = false;
        ReleaseTupleDesc(tupdesc);
        return;
    }

    candidate_out->source_rank_is_null = nulls[3];
    candidate_out->source_rank_input = nulls[3] ? 0 : DatumGetInt32(values[3]);
    candidate_out->weight = nulls[4] ? 1.0 : (double) DatumGetFloat4(values[4]);

    if (!nulls[6])
    {
        direction = text_to_cstring(DatumGetTextPP(values[6]));
    }
    candidate_out->direction = psql_bm25s_hybrid_parse_direction(direction);

    if (!nulls[5])
    {
        normalizer = text_to_cstring(DatumGetTextPP(values[5]));
    }
    if (normalizer == NULL)
    {
        normalizer = candidate_out->direction ==
            PSQL_BM25S_HYBRID_DIRECTION_LOWER
            ? "negative_distance"
            : "identity";
    }
    candidate_out->normalizer =
        psql_bm25s_hybrid_parse_normalizer(normalizer);
    candidate_out->source_id = psql_bm25s_hybrid_find_or_add_source(
        sources,
        source_len,
        candidate_out->source_name
    );

    candidate_out->scored = true;
    ReleaseTupleDesc(tupdesc);
}

static void
psql_bm25s_hybrid_prepare_candidates(
    psql_bm25s_hybrid_candidate_state *candidates,
    size_t candidate_len,
    psql_bm25s_hybrid_source_state *sources,
    psql_bm25s_hybrid_fusion_method fusion_method,
    double rrf_k,
    double epsilon
)
{
    size_t i;
    size_t current_source = (size_t) -1;
    int32 rank_position = 0;

    if (candidate_len == 0)
    {
        return;
    }

    qsort(
        candidates,
        candidate_len,
        sizeof(*candidates),
        psql_bm25s_hybrid_candidate_rank_cmp
    );

    for (i = 0; i < candidate_len; i++)
    {
        psql_bm25s_hybrid_source_state *source;

        if (i == 0 || candidates[i].source_id != current_source)
        {
            current_source = candidates[i].source_id;
            rank_position = 1;
        }
        else
        {
            rank_position++;
        }

        if (candidates[i].source_rank_is_null ||
            candidates[i].source_rank_input == 0)
        {
            candidates[i].source_rank = rank_position;
        }
        else
        {
            candidates[i].source_rank = candidates[i].source_rank_input;
        }

        source = &sources[candidates[i].source_id];
        source->min_value = Min(source->min_value, candidates[i].raw_value);
        source->max_value = Max(source->max_value, candidates[i].raw_value);
        source->sum_value += candidates[i].raw_value;
        source->sumsq_value +=
            candidates[i].raw_value * candidates[i].raw_value;
        source->count++;
    }

    for (i = 0; i < candidate_len; i++)
    {
        psql_bm25s_hybrid_source_state *source;
        double normalized_score = 0.0;
        double avg_value;
        double variance;
        double stddev_value;

        if (candidates[i].source_rank <= 0)
        {
            candidates[i].scored = false;
            continue;
        }

        source = &sources[candidates[i].source_id];
        avg_value = source->sum_value / (double) source->count;
        variance = source->sumsq_value / (double) source->count
            - avg_value * avg_value;
        if (variance < 0.0 && variance > -1e-12)
        {
            variance = 0.0;
        }
        stddev_value = variance <= 0.0 ? 0.0 : sqrt(variance);

        if (fusion_method == PSQL_BM25S_HYBRID_FUSION_RRF)
        {
            normalized_score = 1.0 /
                (rrf_k + (double) candidates[i].source_rank);
        }
        else if (candidates[i].normalizer ==
                 PSQL_BM25S_HYBRID_NORMALIZER_IDENTITY)
        {
            normalized_score = candidates[i].direction ==
                PSQL_BM25S_HYBRID_DIRECTION_LOWER
                ? -candidates[i].raw_value
                : candidates[i].raw_value;
        }
        else if (candidates[i].normalizer ==
                 PSQL_BM25S_HYBRID_NORMALIZER_NEGATIVE_DISTANCE)
        {
            normalized_score = -candidates[i].raw_value;
        }
        else if (candidates[i].normalizer ==
                 PSQL_BM25S_HYBRID_NORMALIZER_INVERSE_DISTANCE)
        {
            normalized_score = 1.0 /
                Max(epsilon, candidates[i].raw_value + epsilon);
        }
        else if (candidates[i].normalizer ==
                 PSQL_BM25S_HYBRID_NORMALIZER_MINMAX)
        {
            if (source->max_value == source->min_value)
            {
                normalized_score = 1.0;
            }
            else if (candidates[i].direction ==
                     PSQL_BM25S_HYBRID_DIRECTION_LOWER)
            {
                normalized_score = (source->max_value - candidates[i].raw_value)
                    / (source->max_value - source->min_value);
            }
            else
            {
                normalized_score = (candidates[i].raw_value - source->min_value)
                    / (source->max_value - source->min_value);
            }
        }
        else if (candidates[i].normalizer ==
                 PSQL_BM25S_HYBRID_NORMALIZER_ZSCORE)
        {
            if (stddev_value == 0.0)
            {
                normalized_score = 0.0;
            }
            else if (candidates[i].direction ==
                     PSQL_BM25S_HYBRID_DIRECTION_LOWER)
            {
                normalized_score = (avg_value - candidates[i].raw_value)
                    / stddev_value;
            }
            else
            {
                normalized_score = (candidates[i].raw_value - avg_value)
                    / stddev_value;
            }
        }
        else
        {
            normalized_score = 1.0 /
                Max((double) candidates[i].source_rank, 1.0);
        }

        candidates[i].normalized_score = normalized_score;
        candidates[i].weighted_score = normalized_score * candidates[i].weight;
        candidates[i].scored = true;
    }
}

static psql_bm25s_hybrid_candidate_state *
psql_bm25s_hybrid_deduplicate_candidates(
    psql_bm25s_hybrid_candidate_state *candidates,
    size_t candidate_len,
    size_t *deduped_len_out
)
{
    psql_bm25s_hybrid_candidate_state *scored;
    psql_bm25s_hybrid_candidate_state *deduped;
    size_t scored_len = 0;
    size_t i;

    *deduped_len_out = 0;
    scored = palloc(sizeof(*scored) * Max(candidate_len, (size_t) 1));
    deduped = palloc(sizeof(*deduped) * Max(candidate_len, (size_t) 1));

    for (i = 0; i < candidate_len; i++)
    {
        if (candidates[i].scored)
        {
            scored[scored_len++] = candidates[i];
        }
    }
    if (scored_len == 0)
    {
        *deduped_len_out = 0;
        return deduped;
    }

    qsort(
        scored,
        scored_len,
        sizeof(*scored),
        psql_bm25s_hybrid_candidate_dedup_cmp
    );

    for (i = 0; i < scored_len; i++)
    {
        if (i > 0 &&
            strcmp(scored[i].source_name, scored[i - 1].source_name) == 0 &&
            strcmp(scored[i].tid_text, scored[i - 1].tid_text) == 0)
        {
            continue;
        }
        deduped[*deduped_len_out] = scored[i];
        (*deduped_len_out)++;
    }

    return deduped;
}

static psql_bm25s_hybrid_hit_state *
psql_bm25s_hybrid_build_hits(
    psql_bm25s_hybrid_candidate_state *deduped,
    size_t deduped_len,
    size_t *hit_len_out
)
{
    psql_bm25s_hybrid_hit_state *hits;
    size_t i;

    *hit_len_out = 0;
    hits = palloc0(sizeof(*hits) * Max(deduped_len, (size_t) 1));
    if (deduped_len == 0)
    {
        return hits;
    }

    qsort(
        deduped,
        deduped_len,
        sizeof(*deduped),
        psql_bm25s_hybrid_candidate_tid_source_cmp
    );

    i = 0;
    while (i < deduped_len)
    {
        size_t start = i;
        size_t end;
        size_t count;
        size_t j;
        psql_bm25s_hybrid_hit_state *hit;
        double score = 0.0;

        while (i < deduped_len &&
               strcmp(deduped[i].tid_text, deduped[start].tid_text) == 0)
        {
            i++;
        }
        end = i;
        count = end - start;
        hit = &hits[*hit_len_out];
        hit->tid = deduped[start].tid;
        hit->tid_text = deduped[start].tid_text;
        hit->source_count = (int32) count;
        hit->source_names = palloc(sizeof(*hit->source_names) * count);
        hit->raw_values = palloc(sizeof(*hit->raw_values) * count);
        hit->normalized_scores =
            palloc(sizeof(*hit->normalized_scores) * count);
        hit->weighted_scores = palloc(sizeof(*hit->weighted_scores) * count);
        hit->ranks = palloc(sizeof(*hit->ranks) * count);

        for (j = 0; j < count; j++)
        {
            const psql_bm25s_hybrid_candidate_state *candidate =
                &deduped[start + j];

            score += candidate->weighted_score;
            hit->source_names[j] = candidate->source_name;
            hit->raw_values[j] = (float4) candidate->raw_value;
            hit->normalized_scores[j] =
                (float4) candidate->normalized_score;
            hit->weighted_scores[j] = (float4) candidate->weighted_score;
            hit->ranks[j] = candidate->source_rank;
        }
        hit->score = (float4) score;
        (*hit_len_out)++;
    }

    qsort(
        hits,
        *hit_len_out,
        sizeof(*hits),
        psql_bm25s_hybrid_hit_cmp
    );
    return hits;
}

static void
psql_bm25s_hybrid_prepare_search_state(
    ArrayType *candidate_array,
    int32 requested_k,
    psql_bm25s_hybrid_fusion_method fusion_method,
    double rrf_k,
    double epsilon,
    psql_bm25s_hybrid_search_state *state_out
)
{
    Datum *items = NULL;
    bool *nulls = NULL;
    int item_count = 0;
    Oid element_type;
    int16 element_len;
    bool element_byval;
    char element_align;
    psql_bm25s_hybrid_candidate_state *candidates;
    psql_bm25s_hybrid_source_state *sources;
    psql_bm25s_hybrid_candidate_state *deduped;
    size_t candidate_len = 0;
    size_t source_len = 0;
    size_t deduped_len = 0;
    size_t hit_len = 0;
    int i;

    memset(state_out, 0, sizeof(*state_out));
    if (candidate_array == NULL || ArrayGetNItems(
            ARR_NDIM(candidate_array),
            ARR_DIMS(candidate_array)
        ) == 0)
    {
        return;
    }

    element_type = ARR_ELEMTYPE(candidate_array);
    get_typlenbyvalalign(
        element_type,
        &element_len,
        &element_byval,
        &element_align
    );
    deconstruct_array(
        candidate_array,
        element_type,
        element_len,
        element_byval,
        element_align,
        &items,
        &nulls,
        &item_count
    );

    candidates = palloc0(sizeof(*candidates) * Max(item_count, 1));
    sources = palloc0(sizeof(*sources) * Max(item_count, 1));
    for (i = 0; i < item_count; i++)
    {
        psql_bm25s_hybrid_candidate_state candidate;

        if (nulls[i])
        {
            continue;
        }
        psql_bm25s_hybrid_deform_candidate(
            items[i],
            &candidate,
            sources,
            &source_len
        );
        if (!candidate.scored)
        {
            continue;
        }
        candidates[candidate_len++] = candidate;
    }

    psql_bm25s_hybrid_prepare_candidates(
        candidates,
        candidate_len,
        sources,
        fusion_method,
        rrf_k,
        epsilon
    );
    deduped = psql_bm25s_hybrid_deduplicate_candidates(
        candidates,
        candidate_len,
        &deduped_len
    );
    state_out->hits = psql_bm25s_hybrid_build_hits(
        deduped,
        deduped_len,
        &hit_len
    );
    state_out->len = Min((size_t) Max(requested_k, 0), hit_len);
}

static void
psql_bm25s_am_prepare_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    const uint32_t *query_ids,
    size_t query_len,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    float *scores = NULL;
    float *weight_mask = NULL;
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    psql_bm25s_status status;
    size_t rank_k;

    memset(state_out, 0, sizeof(*state_out));
    if (entry == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s cache entry is missing")));
    }

    weight_mask = psql_bm25s_array_read_weight_mask(
        weight_mask_array,
        entry->generation.index.num_docs
    );
    if (psql_bm25s_am_sparse_weight_mask_supported(
            weight_mask,
            entry->generation.index.num_docs))
    {
        if (psql_bm25s_am_can_use_unsigned_sparse_path(entry, weight_mask))
        {
            psql_bm25s_am_prepare_search_state_unsigned_sparse(
                entry,
                heapRelation,
                query_ids,
                query_len,
                weight_mask,
                requested_k,
                state_out
            );
        }
        else
        {
            psql_bm25s_am_prepare_search_state_sparse(
                entry,
                heapRelation,
                query_ids,
                query_len,
                weight_mask,
                requested_k,
                state_out
            );
        }
        if (weight_mask != NULL)
        {
            pfree(weight_mask);
        }
        return;
    }

    status = psql_bm25s_scores_from_ids(
        &entry->generation.index,
        query_ids,
        query_len,
        weight_mask,
        &scores
    );
    if (status != PSQL_BM25S_OK)
    {
        if (weight_mask != NULL)
        {
            pfree(weight_mask);
        }
        ereport(ERROR, (errmsg("failed to score query: %s",
                               psql_bm25s_strerror(status))));
    }

    rank_k = Min((size_t) entry->generation.index.num_docs, Max(requested_k, (size_t) 1));
    status = psql_bm25s_topk(
        scores,
        entry->generation.index.num_docs,
        rank_k,
        true,
        &ranked
    );
    if (status != PSQL_BM25S_OK)
    {
        if (weight_mask != NULL)
        {
            pfree(weight_mask);
        }
        free(scores);
        ereport(ERROR, (errmsg("failed to compute top-k: %s",
                               psql_bm25s_strerror(status))));
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_collect_visible_hits(
        entry->generation.doc_tids,
        &ranked,
        requested_k,
        &visibility,
        state_out
    );

    if (state_out->topk.len < requested_k && rank_k < entry->generation.index.num_docs)
    {
        psql_bm25s_topk_result_free(&ranked);
        memset(&ranked, 0, sizeof(ranked));
        status = psql_bm25s_topk(
            scores,
            entry->generation.index.num_docs,
            entry->generation.index.num_docs,
            true,
            &ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            free(scores);
            ereport(ERROR, (errmsg("failed to expand top-k result: %s",
                                   psql_bm25s_strerror(status))));
        }

        if (state_out->tids != NULL)
        {
            pfree(state_out->tids);
        }
        psql_bm25s_topk_result_free(&state_out->topk);
        memset(state_out, 0, sizeof(*state_out));
        psql_bm25s_am_collect_visible_hits(
            entry->generation.doc_tids,
            &ranked,
            requested_k,
            &visibility,
            state_out
        );
    }

    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_topk_result_free(&ranked);
    if (weight_mask != NULL)
    {
        pfree(weight_mask);
    }
    free(scores);
}

static void
psql_bm25s_am_prepare_field_tokens_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    ArrayType *weight_mask_array,
    size_t requested_k,
    bool include_delta,
    psql_bm25s_search_state *state_out
)
{
    float *scores = NULL;
    float *weight_mask = NULL;
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    psql_bm25s_status status;
    size_t rank_k;
    size_t bytes;
    size_t field_idx;

    memset(state_out, 0, sizeof(*state_out));
    if (entry == NULL)
    {
        ereport(ERROR, (errmsg("psql_bm25s cache entry is missing")));
    }
    if (!psql_bm25s_am_field_aware_enabled(indexRelation))
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s field-aware search requires field_aware=true")
            )
        );
    }
    weight_mask = psql_bm25s_array_read_weight_mask(
        weight_mask_array,
        entry->generation.index.num_docs
    );
    if (psql_bm25s_am_sparse_weight_mask_supported(
            weight_mask,
            entry->generation.index.num_docs) &&
        psql_bm25s_am_field_weights_supported(field_weights, num_fields))
    {
        if (psql_bm25s_am_can_use_unsigned_sparse_path(entry, weight_mask) &&
            psql_bm25s_am_field_weights_nonnegative(
                field_weights,
                num_fields))
        {
            psql_bm25s_am_prepare_field_tokens_search_state_unsigned_sparse(
                entry,
                heapRelation,
                query_tokens,
                query_len,
                field_indexes,
                field_weights,
                num_fields,
                weight_mask,
                requested_k,
                state_out
            );
        }
        else
        {
            psql_bm25s_am_prepare_field_tokens_search_state_sparse(
                entry,
                heapRelation,
                query_tokens,
                query_len,
                field_indexes,
                field_weights,
                num_fields,
                weight_mask,
                requested_k,
                state_out
            );
        }
        if (include_delta)
        {
            psql_bm25s_am_merge_field_delta_tokens(
                entry,
                indexRelation,
                heapRelation,
                query_tokens,
                query_len,
                field_indexes,
                field_weights,
                num_fields,
                weight_mask_array,
                requested_k,
                state_out
            );
        }
        if (weight_mask != NULL)
        {
            pfree(weight_mask);
        }
        return;
    }
    if (!psql_bm25s_am_checked_mul_size(
            entry->generation.index.num_docs,
            sizeof(*scores),
            &bytes))
    {
        ereport(ERROR, (errmsg("psql_bm25s score workspace is too large")));
    }
    scores = palloc0(bytes);

    for (field_idx = 0; field_idx < num_fields; field_idx++)
    {
        char **field_tokens = NULL;
        uint32_t *query_ids = NULL;
        size_t query_id_len = 0;
        float *field_scores = NULL;
        size_t i;

        if (!psql_bm25s_am_checked_mul_size(
                query_len,
                sizeof(*field_tokens),
                &bytes))
        {
            pfree(scores);
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            ereport(ERROR, (errmsg("psql_bm25s query is too large")));
        }
        field_tokens = palloc0(bytes);
        for (i = 0; i < query_len; i++)
        {
            field_tokens[i] = psql_bm25s_am_make_field_token(
                field_indexes[field_idx],
                query_tokens[i]
            );
        }

        status = psql_bm25s_am_query_token_ids_cached(
            entry,
            field_tokens,
            query_len,
            &query_ids,
            &query_id_len
        );
        for (i = 0; i < query_len; i++)
        {
            pfree(field_tokens[i]);
        }
        pfree(field_tokens);
        if (status != PSQL_BM25S_OK)
        {
            pfree(scores);
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            ereport(ERROR, (errmsg("failed to resolve field query tokens: %s",
                                   psql_bm25s_strerror(status))));
        }

        status = psql_bm25s_scores_from_ids(
            &entry->generation.index,
            query_ids,
            query_id_len,
            weight_mask,
            &field_scores
        );
        free(query_ids);
        if (status != PSQL_BM25S_OK)
        {
            pfree(scores);
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            ereport(ERROR, (errmsg("failed to score field query: %s",
                                   psql_bm25s_strerror(status))));
        }
        for (i = 0; i < entry->generation.index.num_docs; i++)
        {
            scores[i] += field_scores[i] * field_weights[field_idx];
        }
        free(field_scores);
    }

    rank_k = Min((size_t) entry->generation.index.num_docs, Max(requested_k, (size_t) 1));
    status = psql_bm25s_topk(
        scores,
        entry->generation.index.num_docs,
        rank_k,
        true,
        &ranked
    );
    if (status != PSQL_BM25S_OK)
    {
        pfree(scores);
        if (weight_mask != NULL)
        {
            pfree(weight_mask);
        }
        ereport(ERROR, (errmsg("failed to compute field-aware top-k: %s",
                               psql_bm25s_strerror(status))));
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_collect_visible_hits(
        entry->generation.doc_tids,
        &ranked,
        requested_k,
        &visibility,
        state_out
    );
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_topk_result_free(&ranked);
    if (weight_mask != NULL)
    {
        pfree(weight_mask);
    }
    pfree(scores);

    if (include_delta)
    {
        psql_bm25s_am_merge_field_delta_tokens(
            entry,
            indexRelation,
            heapRelation,
            query_tokens,
            query_len,
            field_indexes,
            field_weights,
            num_fields,
            weight_mask_array,
            requested_k,
            state_out
        );
    }
}

static bool
psql_bm25s_am_search_state_has_tid(
    const psql_bm25s_search_state *state,
    const ItemPointerData *tid,
    size_t upto
)
{
    size_t i;

    if (state == NULL || tid == NULL)
    {
        return false;
    }

    for (i = 0; i < upto; i++)
    {
        if (ItemPointerEquals(&state->tids[i], (ItemPointer) tid))
        {
            return true;
        }
    }
    return false;
}

static void
psql_bm25s_am_merge_search_states(
    psql_bm25s_search_state *base_state,
    psql_bm25s_search_state *delta_state,
    size_t requested_k
)
{
    psql_bm25s_search_state merged;
    size_t left = 0;
    size_t right = 0;
    size_t accepted = 0;

    if (base_state == NULL || delta_state == NULL ||
        requested_k == 0 || delta_state->topk.len == 0)
    {
        return;
    }

    memset(&merged, 0, sizeof(merged));
    psql_bm25s_am_init_search_result(requested_k, &merged);
    while (accepted < requested_k &&
           (left < base_state->topk.len || right < delta_state->topk.len))
    {
        bool take_delta;
        const ItemPointerData *tid;
        uint32_t doc_id;
        float score;

        if (right >= delta_state->topk.len)
        {
            take_delta = false;
        }
        else if (left >= base_state->topk.len)
        {
            take_delta = true;
        }
        else
        {
            take_delta =
                delta_state->topk.scores[right] > base_state->topk.scores[left];
        }

        if (take_delta)
        {
            tid = &delta_state->tids[right];
            doc_id = delta_state->topk.doc_ids[right];
            score = delta_state->topk.scores[right];
            right++;
        }
        else
        {
            tid = &base_state->tids[left];
            doc_id = base_state->topk.doc_ids[left];
            score = base_state->topk.scores[left];
            left++;
        }

        if (psql_bm25s_am_search_state_has_tid(&merged, tid, accepted))
        {
            continue;
        }

        psql_bm25s_am_append_search_hit(
            &merged,
            accepted,
            tid,
            doc_id,
            score
        );
        accepted++;
    }

    merged.topk.len = accepted;
    psql_bm25s_am_search_state_reset(base_state);
    *base_state = merged;
}

static void
psql_bm25s_am_merge_textlike_delta_tokens(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_cache_entry *delta_entry;
    psql_bm25s_search_state delta_state = {0};
    bool eventual_policy;
    size_t i;

    eventual_policy =
        psql_bm25s_am_eventual_policy_enabled(indexRelation);
    if (entry == NULL || requested_k == 0 || weight_mask != NULL ||
        entry->generation.delta_overlay_materialized)
    {
        return;
    }
    /*
     * Overlay the delta snapshot that belongs to the resident base generation.
     * Current metapage counters can advance or switch generations while this
     * query is running; using them here can either drop fresh-enough delta
     * records or merge the wrong tail into an older leased base.
     */
    meta = entry->generation.meta;
    if (!psql_bm25s_am_source_type_is_textlike(meta.source_type) ||
        meta.pending_write_tuples == 0 ||
        meta.delta_record_count == 0)
    {
        return;
    }
    if (!psql_bm25s_am_can_use_delta_overlay(&meta) ||
        !psql_bm25s_am_query_overlay_within_budget(indexRelation, &meta))
    {
        return;
    }
    if (!psql_bm25s_am_wait_for_pending_maintenance(
            indexRelation,
            !eventual_policy
        ))
    {
        return;
    }

    delta_entry = psql_bm25s_am_get_delta_overlay_entry(
        entry,
        indexRelation,
        &meta
    );
    if (delta_entry == NULL)
    {
        return;
    }
    if (delta_entry->generation.index.num_docs == 0)
    {
        return;
    }

    psql_bm25s_am_prepare_tokens_search_state(
        delta_entry,
        indexRelation,
        heapRelation,
        query_tokens,
        query_len,
        NULL,
        requested_k,
        &delta_state
    );
    for (i = 0; i < delta_state.topk.len; i++)
    {
        delta_state.topk.doc_ids[i] += entry->generation.index.num_docs;
    }

    psql_bm25s_am_merge_search_states(state_out, &delta_state, requested_k);

    psql_bm25s_am_search_state_reset(&delta_state);
}

static void
psql_bm25s_am_merge_field_delta_tokens(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    int *field_indexes,
    float *field_weights,
    size_t num_fields,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_cache_entry *delta_entry;
    psql_bm25s_search_state delta_state = {0};
    bool eventual_policy;
    size_t i;

    eventual_policy = psql_bm25s_am_eventual_policy_enabled(indexRelation);
    if (entry == NULL || requested_k == 0 || weight_mask != NULL ||
        entry->generation.delta_overlay_materialized)
    {
        return;
    }
    /*
     * Keep the field-aware overlay tied to the leased base generation. If a
     * background worker publishes a new generation concurrently, the new base
     * owns its own delta region; this query must keep using the old bounded
     * delta snapshot rather than rereading current metapage counters.
     */
    meta = entry->generation.meta;
    if (!psql_bm25s_am_source_type_is_textlike(meta.source_type) ||
        meta.pending_write_tuples == 0 ||
        meta.delta_record_count == 0)
    {
        return;
    }
    if (!psql_bm25s_am_can_use_delta_overlay(&meta) ||
        !psql_bm25s_am_query_overlay_within_budget(indexRelation, &meta))
    {
        return;
    }
    if (!psql_bm25s_am_wait_for_pending_maintenance(
            indexRelation,
            !eventual_policy
        ))
    {
        return;
    }

    delta_entry = psql_bm25s_am_get_delta_overlay_entry(
        entry,
        indexRelation,
        &meta
    );
    if (delta_entry == NULL)
    {
        return;
    }
    if (delta_entry->generation.index.num_docs == 0)
    {
        return;
    }

    psql_bm25s_am_prepare_field_tokens_search_state(
        delta_entry,
        indexRelation,
        heapRelation,
        query_tokens,
        query_len,
        field_indexes,
        field_weights,
        num_fields,
        NULL,
        requested_k,
        false,
        &delta_state
    );
    for (i = 0; i < delta_state.topk.len; i++)
    {
        delta_state.topk.doc_ids[i] += entry->generation.index.num_docs;
    }

    psql_bm25s_am_merge_search_states(state_out, &delta_state, requested_k);

    psql_bm25s_am_search_state_reset(&delta_state);
}

static void
psql_bm25s_am_free_query_tokens(char **query_tokens, size_t query_len)
{
    size_t i;

    if (query_tokens == NULL)
    {
        return;
    }

    for (i = 0; i < query_len; i++)
    {
        if (query_tokens[i] != NULL)
        {
            pfree(query_tokens[i]);
        }
    }

    pfree(query_tokens);
}

static void
psql_bm25s_am_prepare_tokens_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    char **query_tokens,
    size_t query_len,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    uint32_t *query_ids = NULL;
    size_t query_id_len = 0;
    psql_bm25s_status status;

    if (psql_bm25s_am_field_aware_enabled(indexRelation))
    {
        int *field_indexes = NULL;
        float *field_weights = NULL;
        size_t num_fields = 0;

        psql_bm25s_am_read_all_field_specs(
            indexRelation,
            &field_indexes,
            &field_weights,
            &num_fields
        );
        psql_bm25s_am_prepare_field_tokens_search_state(
            entry,
            indexRelation,
            heapRelation,
            query_tokens,
            query_len,
            field_indexes,
            field_weights,
            num_fields,
            weight_mask,
            requested_k,
            true,
            state_out
        );
        pfree(field_indexes);
        pfree(field_weights);
        return;
    }

    status = psql_bm25s_am_query_token_ids_cached(
        entry,
        query_tokens,
        query_len,
        &query_ids,
        &query_id_len
    );
    if (status != PSQL_BM25S_OK)
    {
        ereport(ERROR, (errmsg("failed to map query tokens: %s",
                               psql_bm25s_strerror(status))));
    }

    psql_bm25s_am_prepare_search_state(
        entry,
        heapRelation,
        query_ids,
        query_id_len,
        weight_mask,
        requested_k,
        state_out
    );
    psql_bm25s_am_merge_textlike_delta_tokens(
        entry,
        indexRelation,
        heapRelation,
        query_tokens,
        query_len,
        weight_mask,
        requested_k,
        state_out
    );
    free(query_ids);
}

static void
psql_bm25s_am_parse_raw_query_with_options(
    text *query_text,
    const psql_bm25s_text_options *options,
    psql_bm25s_query *query_out
)
{
    char *raw_query;
    psql_bm25s_status status;

    if (query_text == NULL || query_out == NULL)
    {
        ereport(ERROR, (errmsg("raw query arguments are missing")));
    }

    psql_bm25s_query_init(query_out);
    raw_query = text_to_cstring(query_text);

    status = psql_bm25s_parse_query_string(raw_query, query_out);
    if (status != PSQL_BM25S_OK)
    {
        pfree(raw_query);
        ereport(ERROR, (errmsg("failed to parse raw query: %s",
                               psql_bm25s_strerror(status))));
    }

    status = psql_bm25s_normalize_query(query_out, options);
    pfree(raw_query);
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_query_free(query_out);
        ereport(ERROR, (errmsg("failed to normalize raw query: %s",
                               psql_bm25s_strerror(status))));
    }
}

static void
psql_bm25s_am_parse_raw_query(
    text *query_text,
    psql_bm25s_query *query_out
)
{
    psql_bm25s_text_options options;

    psql_bm25s_text_options_init(&options);
    options.lowercase = false;
    psql_bm25s_am_parse_raw_query_with_options(
        query_text,
        &options,
        query_out
    );
}

static void
psql_bm25s_am_parse_raw_query_pg_options(
    text *query_text,
    bool lowercase,
    bool stem_english,
    bool fold_diacritics,
    ArrayType *stopwords_array,
    psql_bm25s_query *query_out
)
{
    psql_bm25s_text_options options;
    char **stopwords = NULL;
    size_t stopword_len = 0;

    psql_bm25s_am_read_text_options(
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        &options,
        &stopwords,
        &stopword_len
    );
    psql_bm25s_am_parse_raw_query_with_options(
        query_text,
        &options,
        query_out
    );
    psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
}

static void
psql_bm25s_am_query_plan_init(psql_bm25s_am_query_plan *plan)
{
    memset(plan, 0, sizeof(*plan));
}

static void
psql_bm25s_am_query_plan_free(psql_bm25s_am_query_plan *plan)
{
    size_t i;

    if (plan == NULL)
    {
        return;
    }

    if (plan->clauses != NULL)
    {
        for (i = 0; i < plan->len; i++)
        {
            free(plan->clauses[i].token_ids);
        }
        pfree(plan->clauses);
    }
    if (!plan->positive_ids_aliases_clause)
    {
        free(plan->positive_ids);
    }
    memset(plan, 0, sizeof(*plan));
}

static psql_bm25s_status
psql_bm25s_am_query_plan_append_positive(
    psql_bm25s_am_query_plan *plan,
    const uint32_t *token_ids,
    size_t len
)
{
    size_t new_len;
    size_t bytes;
    uint32_t *resized;

    if (plan == NULL || (len > 0 && token_ids == NULL))
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (len == 0)
    {
        return PSQL_BM25S_OK;
    }

    new_len = plan->positive_len + len;
    if (new_len > plan->positive_capacity)
    {
        size_t new_capacity = plan->positive_capacity == 0 ?
            len : plan->positive_capacity * 2;

        while (new_capacity < new_len)
        {
            new_capacity *= 2;
        }

        if (!psql_bm25s_am_checked_mul_size(
                new_capacity,
                sizeof(*plan->positive_ids),
                &bytes))
        {
            return PSQL_BM25S_ERR_RANGE;
        }

        resized = realloc(plan->positive_ids, bytes);
        if (resized == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        plan->positive_ids = resized;
        plan->positive_capacity = new_capacity;
    }

    memcpy(
        &plan->positive_ids[plan->positive_len],
        token_ids,
        sizeof(*token_ids) * len
    );
    plan->positive_len = new_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_expand_exact_query_term_ids(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query_term *term,
    uint32_t **token_ids_out,
    size_t *len_out
)
{
    uint32_t *token_ids = NULL;
    size_t capacity;
    size_t len = 0;
    size_t i;
    size_t bytes;

    *token_ids_out = NULL;
    *len_out = 0;
    if (entry == NULL || term == NULL || term->tokens == NULL || term->len == 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (term->kind == PSQL_BM25S_QUERY_PREFIX)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    capacity = term->kind == PSQL_BM25S_QUERY_PHRASE ? term->len : 1;
    if (!psql_bm25s_am_checked_mul_size(
            capacity,
            sizeof(*token_ids),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    token_ids = malloc(bytes);
    if (token_ids == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (term->kind == PSQL_BM25S_QUERY_TERM)
    {
        size_t pos = psql_bm25s_am_vocab_lower_bound(entry, term->tokens[0]);

        if (pos < entry->generation.index.vocab_size &&
            strcmp(
                psql_bm25s_am_sorted_vocab_token(entry, pos),
                term->tokens[0]) == 0)
        {
            token_ids[len++] =
                psql_bm25s_am_sorted_vocab_token_id(entry, pos);
        }
    }
    else
    {
        for (i = 0; i < term->len; i++)
        {
            size_t pos = psql_bm25s_am_vocab_lower_bound(entry, term->tokens[i]);

            if (pos >= entry->generation.index.vocab_size ||
                strcmp(
                    psql_bm25s_am_sorted_vocab_token(entry, pos),
                    term->tokens[i]) != 0)
            {
                continue;
            }

            token_ids[len++] =
                psql_bm25s_am_sorted_vocab_token_id(entry, pos);
        }
    }

    if (len == 0)
    {
        free(token_ids);
        return PSQL_BM25S_OK;
    }

    *token_ids_out = token_ids;
    *len_out = len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_expand_query_term_ids(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query_term *term,
    uint32_t **token_ids_out,
    size_t *len_out
)
{
    size_t lower;
    size_t i;
    uint32_t *token_ids = NULL;
    size_t len = 0;
    size_t capacity = 0;

    *token_ids_out = NULL;
    *len_out = 0;
    if (entry == NULL || term == NULL || term->tokens == NULL || term->len == 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (term->kind == PSQL_BM25S_QUERY_PHRASE)
    {
        for (i = 0; i < term->len; i++)
        {
            size_t pos = psql_bm25s_am_vocab_lower_bound(entry, term->tokens[i]);

            if (pos >= entry->generation.index.vocab_size ||
                strcmp(
                    psql_bm25s_am_sorted_vocab_token(entry, pos),
                    term->tokens[i]) != 0)
            {
                continue;
            }

            if (len == capacity)
            {
                size_t bytes;
                uint32_t *resized;

                capacity = capacity == 0 ? term->len : capacity * 2;
                if (!psql_bm25s_am_checked_mul_size(
                        capacity,
                        sizeof(*token_ids),
                        &bytes))
                {
                    free(token_ids);
                    return PSQL_BM25S_ERR_RANGE;
                }
                resized = realloc(token_ids, bytes);
                if (resized == NULL)
                {
                    free(token_ids);
                    return PSQL_BM25S_ERR_NOMEM;
                }
                token_ids = resized;
            }

            token_ids[len++] =
                psql_bm25s_am_sorted_vocab_token_id(entry, pos);
        }

        *token_ids_out = token_ids;
        *len_out = len;
        return PSQL_BM25S_OK;
    }

    lower = psql_bm25s_am_vocab_lower_bound(entry, term->tokens[0]);
    if (term->kind == PSQL_BM25S_QUERY_TERM)
    {
        if (lower < entry->generation.index.vocab_size &&
            strcmp(
                psql_bm25s_am_sorted_vocab_token(entry, lower),
                term->tokens[0]) == 0)
        {
            token_ids = malloc(sizeof(*token_ids));
            if (token_ids == NULL)
            {
                return PSQL_BM25S_ERR_NOMEM;
            }
            token_ids[0] =
                psql_bm25s_am_sorted_vocab_token_id(entry, lower);
            len = 1;
        }
    }
    else if (term->kind == PSQL_BM25S_QUERY_PREFIX)
    {
        size_t prefix_len = psql_bm25s_am_query_term_token_len(term, 0);

        for (i = lower; i < entry->generation.index.vocab_size; i++)
        {
            size_t bytes;
            uint32_t *resized;

            if (strncmp(
                    psql_bm25s_am_sorted_vocab_token(entry, i),
                    term->tokens[0],
                    prefix_len) != 0)
            {
                break;
            }

            if (len == capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                if (!psql_bm25s_am_checked_mul_size(
                        capacity,
                        sizeof(*token_ids),
                        &bytes))
                {
                    free(token_ids);
                    return PSQL_BM25S_ERR_RANGE;
                }
                resized = realloc(token_ids, bytes);
                if (resized == NULL)
                {
                    free(token_ids);
                    return PSQL_BM25S_ERR_NOMEM;
                }
                token_ids = resized;
            }

            token_ids[len++] =
                psql_bm25s_am_sorted_vocab_token_id(entry, i);
        }
    }

    *token_ids_out = token_ids;
    *len_out = len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_append_query_ids(
    uint32_t **query_ids,
    size_t *query_len,
    size_t *query_capacity,
    const uint32_t *term_ids,
    size_t term_len
)
{
    uint32_t *resized;
    size_t new_len;
    size_t bytes;
    size_t new_capacity;

    if (term_len == 0)
    {
        return PSQL_BM25S_OK;
    }

    new_len = *query_len + term_len;
    if (new_len > *query_capacity)
    {
        new_capacity = *query_capacity == 0 ? term_len : *query_capacity * 2;
        while (new_capacity < new_len)
        {
            new_capacity *= 2;
        }

        if (!psql_bm25s_am_checked_mul_size(
                new_capacity,
                sizeof(**query_ids),
                &bytes))
        {
            return PSQL_BM25S_ERR_RANGE;
        }

        resized = realloc(*query_ids, bytes);
        if (resized == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }

        *query_ids = resized;
        *query_capacity = new_capacity;
    }

    memcpy(
        &(*query_ids)[*query_len],
        term_ids,
        sizeof(*term_ids) * term_len
    );
    *query_len = new_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_collect_positive_query_ids_ast(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node,
    bool negated,
    uint32_t **query_ids,
    size_t *query_len,
    size_t *query_capacity
)
{
    psql_bm25s_status status;

    if (node == NULL)
    {
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        uint32_t *term_ids = NULL;
        size_t term_len = 0;

        if (negated || node->term_index >= query->len)
        {
            return PSQL_BM25S_OK;
        }

        status = psql_bm25s_am_expand_query_term_ids(
            entry,
            &query->terms[node->term_index],
            &term_ids,
            &term_len
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }

        status = psql_bm25s_am_append_query_ids(
            query_ids,
            query_len,
            query_capacity,
            term_ids,
            term_len
        );
        free(term_ids);
        return status;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_NOT)
    {
        return psql_bm25s_am_collect_positive_query_ids_ast(
            entry,
            query,
            node->left,
            !negated,
            query_ids,
            query_len,
            query_capacity
        );
    }

    status = psql_bm25s_am_collect_positive_query_ids_ast(
        entry,
        query,
        node->left,
        negated,
        query_ids,
        query_len,
        query_capacity
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    return psql_bm25s_am_collect_positive_query_ids_ast(
        entry,
        query,
        node->right,
        negated,
        query_ids,
        query_len,
        query_capacity
    );
}

static psql_bm25s_status
psql_bm25s_am_collect_positive_query_ids(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    uint32_t **query_ids_out,
    size_t *query_len_out
)
{
    uint32_t *query_ids = NULL;
    size_t query_len = 0;
    size_t query_capacity = 0;
    size_t i;
    psql_bm25s_status status;

    *query_ids_out = NULL;
    *query_len_out = 0;
    if (entry == NULL || query == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (psql_bm25s_query_uses_boolean_ast(query))
    {
        status = psql_bm25s_am_collect_positive_query_ids_ast(
            entry,
            query,
            query->root,
            false,
            &query_ids,
            &query_len,
            &query_capacity
        );
        if (status != PSQL_BM25S_OK)
        {
            free(query_ids);
            return status;
        }
    }
    else
    {
        for (i = 0; i < query->len; i++)
        {
            uint32_t *term_ids = NULL;
            size_t term_len = 0;

            if (query->terms[i].occur == PSQL_BM25S_QUERY_MUST_NOT)
            {
                continue;
            }

            status = psql_bm25s_am_expand_query_term_ids(
                entry,
                &query->terms[i],
                &term_ids,
                &term_len
            );
            if (status != PSQL_BM25S_OK)
            {
                free(query_ids);
                return status;
            }

            status = psql_bm25s_am_append_query_ids(
                &query_ids,
                &query_len,
                &query_capacity,
                term_ids,
                term_len
            );
            free(term_ids);
            if (status != PSQL_BM25S_OK)
            {
                free(query_ids);
                return status;
            }
        }
    }

    *query_ids_out = query_ids;
    *query_len_out = query_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_build_query_plan(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    psql_bm25s_am_query_plan *plan_out
)
{
    size_t i;
    psql_bm25s_am_query_plan plan;

    if (entry == NULL || query == NULL || plan_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    psql_bm25s_am_query_plan_init(&plan);
    if (!psql_bm25s_query_uses_boolean_ast(query) &&
        query->len == 1 &&
        query->terms[0].kind != PSQL_BM25S_QUERY_PREFIX)
    {
        const psql_bm25s_query_term *term = &query->terms[0];
        psql_bm25s_status status;

        plan.clauses = palloc0(sizeof(*plan.clauses));
        plan.len = 1;
        plan.clauses[0].occur = term->occur;
        status = psql_bm25s_am_expand_exact_query_term_ids(
            entry,
            term,
            &plan.clauses[0].token_ids,
            &plan.clauses[0].len
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_query_plan_free(&plan);
            return status;
        }

        if (term->occur == PSQL_BM25S_QUERY_MUST_NOT)
        {
            plan.has_must_not = true;
        }
        else
        {
            plan.positive_clause_count = 1;
            if (term->occur == PSQL_BM25S_QUERY_MUST)
            {
                plan.must_clause_count = 1;
            }
            plan.positive_ids = plan.clauses[0].token_ids;
            plan.positive_len = plan.clauses[0].len;
            plan.positive_capacity = plan.clauses[0].len;
            plan.positive_ids_aliases_clause = true;
        }

        *plan_out = plan;
        return PSQL_BM25S_OK;
    }

    if (query->len > 0)
    {
        plan.clauses = palloc0(sizeof(*plan.clauses) * query->len);
    }
    plan.len = query->len;

    for (i = 0; i < query->len; i++)
    {
        const psql_bm25s_query_term *term = &query->terms[i];
        psql_bm25s_status status;

        plan.clauses[i].occur = term->occur;
        status = psql_bm25s_am_expand_query_term_ids(
            entry,
            term,
            &plan.clauses[i].token_ids,
            &plan.clauses[i].len
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_query_plan_free(&plan);
            return status;
        }

        if (term->occur == PSQL_BM25S_QUERY_MUST_NOT)
        {
            plan.has_must_not = true;
            continue;
        }

        plan.positive_clause_count++;
        if (term->occur == PSQL_BM25S_QUERY_MUST)
        {
            plan.must_clause_count++;
        }

        status = psql_bm25s_am_query_plan_append_positive(
            &plan,
            plan.clauses[i].token_ids,
            plan.clauses[i].len
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_query_plan_free(&plan);
            return status;
        }
    }

    *plan_out = plan;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_append_query_plan_clause(
    psql_bm25s_am_query_plan *plan,
    size_t clause_index,
    psql_bm25s_query_occur occur,
    uint32_t *token_ids,
    size_t token_len
)
{
    psql_bm25s_status status;

    plan->clauses[clause_index].occur = occur;
    plan->clauses[clause_index].token_ids = token_ids;
    plan->clauses[clause_index].len = token_len;

    if (occur == PSQL_BM25S_QUERY_MUST_NOT)
    {
        plan->has_must_not = true;
        return PSQL_BM25S_OK;
    }

    plan->positive_clause_count++;
    if (occur == PSQL_BM25S_QUERY_MUST)
    {
        plan->must_clause_count++;
    }

    status = psql_bm25s_am_query_plan_append_positive(
        plan,
        token_ids,
        token_len
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_append_clause_token_ids(
    uint32_t **token_ids_io,
    size_t *token_len_io,
    size_t *token_capacity_io,
    const uint32_t *append_ids,
    size_t append_len
)
{
    uint32_t *resized;
    size_t new_len;
    size_t bytes;

    if (token_ids_io == NULL ||
        token_len_io == NULL ||
        token_capacity_io == NULL ||
        (append_len > 0 && append_ids == NULL))
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (append_len == 0)
    {
        return PSQL_BM25S_OK;
    }

    new_len = *token_len_io + append_len;
    if (new_len > *token_capacity_io)
    {
        size_t new_capacity = *token_capacity_io == 0 ?
            append_len : *token_capacity_io * 2;

        while (new_capacity < new_len)
        {
            new_capacity *= 2;
        }

        if (!psql_bm25s_am_checked_mul_size(
                new_capacity,
                sizeof(**token_ids_io),
                &bytes))
        {
            return PSQL_BM25S_ERR_RANGE;
        }

        resized = realloc(*token_ids_io, bytes);
        if (resized == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        *token_ids_io = resized;
        *token_capacity_io = new_capacity;
    }

    memcpy(
        *token_ids_io + *token_len_io,
        append_ids,
        append_len * sizeof(**token_ids_io)
    );
    *token_len_io = new_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_am_collect_boolean_ast_disjunction_token_ids(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node,
    uint32_t **token_ids_io,
    size_t *token_len_io,
    size_t *token_capacity_io,
    bool *supported_out
)
{
    psql_bm25s_status status;

    if (node == NULL || !*supported_out)
    {
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        uint32_t *term_ids = NULL;
        size_t term_len = 0;

        if (node->term_index >= query->len)
        {
            return PSQL_BM25S_ERR_INVALID;
        }

        status = psql_bm25s_am_expand_query_term_ids(
            entry,
            &query->terms[node->term_index],
            &term_ids,
            &term_len
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }

        status = psql_bm25s_am_append_clause_token_ids(
            token_ids_io,
            token_len_io,
            token_capacity_io,
            term_ids,
            term_len
        );
        free(term_ids);
        return status;
    }

    if (node->kind != PSQL_BM25S_QUERY_NODE_OR)
    {
        *supported_out = false;
        return PSQL_BM25S_OK;
    }

    status = psql_bm25s_am_collect_boolean_ast_disjunction_token_ids(
        entry,
        query,
        node->left,
        token_ids_io,
        token_len_io,
        token_capacity_io,
        supported_out
    );
    if (status != PSQL_BM25S_OK || !*supported_out)
    {
        return status;
    }

    return psql_bm25s_am_collect_boolean_ast_disjunction_token_ids(
        entry,
        query,
        node->right,
        token_ids_io,
        token_len_io,
        token_capacity_io,
        supported_out
    );
}

static psql_bm25s_status
psql_bm25s_am_build_boolean_ast_conjunction_plan_node(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node,
    bool negated,
    psql_bm25s_am_query_plan *plan,
    size_t *clause_index_io,
    bool *supported_out
)
{
    psql_bm25s_status status;

    if (node == NULL || !*supported_out)
    {
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        uint32_t *token_ids = NULL;
        size_t token_len = 0;
        psql_bm25s_query_occur occur;

        if (node->term_index >= query->len)
        {
            return PSQL_BM25S_ERR_INVALID;
        }

        status = psql_bm25s_am_expand_query_term_ids(
            entry,
            &query->terms[node->term_index],
            &token_ids,
            &token_len
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }

        occur = negated ?
            PSQL_BM25S_QUERY_MUST_NOT :
            PSQL_BM25S_QUERY_MUST;
        status = psql_bm25s_am_append_query_plan_clause(
            plan,
            *clause_index_io,
            occur,
            token_ids,
            token_len
        );
        if (status != PSQL_BM25S_OK)
        {
            free(token_ids);
            return status;
        }

        (*clause_index_io)++;
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_NOT)
    {
        if (node->left != NULL &&
            node->left->kind == PSQL_BM25S_QUERY_NODE_AND)
        {
            *supported_out = false;
            return PSQL_BM25S_OK;
        }

        return psql_bm25s_am_build_boolean_ast_conjunction_plan_node(
            entry,
            query,
            node->left,
            !negated,
            plan,
            clause_index_io,
            supported_out
        );
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_OR)
    {
        uint32_t *token_ids = NULL;
        size_t token_len = 0;
        size_t token_capacity = 0;
        psql_bm25s_query_occur occur = negated ?
            PSQL_BM25S_QUERY_MUST_NOT :
            PSQL_BM25S_QUERY_MUST;

        status = psql_bm25s_am_collect_boolean_ast_disjunction_token_ids(
            entry,
            query,
            node,
            &token_ids,
            &token_len,
            &token_capacity,
            supported_out
        );
        if (status != PSQL_BM25S_OK || !*supported_out)
        {
            free(token_ids);
            return status;
        }

        status = psql_bm25s_am_append_query_plan_clause(
            plan,
            *clause_index_io,
            occur,
            token_ids,
            token_len
        );
        if (status != PSQL_BM25S_OK)
        {
            free(token_ids);
            return status;
        }

        (*clause_index_io)++;
        return PSQL_BM25S_OK;
    }

    if (node->kind != PSQL_BM25S_QUERY_NODE_AND || negated)
    {
        *supported_out = false;
        return PSQL_BM25S_OK;
    }

    status = psql_bm25s_am_build_boolean_ast_conjunction_plan_node(
        entry,
        query,
        node->left,
        negated,
        plan,
        clause_index_io,
        supported_out
    );
    if (status != PSQL_BM25S_OK || !*supported_out)
    {
        return status;
    }

    return psql_bm25s_am_build_boolean_ast_conjunction_plan_node(
        entry,
        query,
        node->right,
        negated,
        plan,
        clause_index_io,
        supported_out
    );
}

static psql_bm25s_status
psql_bm25s_am_build_boolean_ast_conjunction_plan(
    const psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    psql_bm25s_am_query_plan *plan_out,
    bool *supported_out
)
{
    psql_bm25s_am_query_plan plan;
    size_t clause_index = 0;
    psql_bm25s_status status;

    *supported_out = false;
    if (!psql_bm25s_query_uses_boolean_ast(query))
    {
        return PSQL_BM25S_OK;
    }

    psql_bm25s_am_query_plan_init(&plan);
    if (query->len > 0)
    {
        plan.clauses = palloc0(sizeof(*plan.clauses) * query->len);
    }
    plan.len = query->len;
    *supported_out = true;

    status = psql_bm25s_am_build_boolean_ast_conjunction_plan_node(
        entry,
        query,
        query->root,
        false,
        &plan,
        &clause_index,
        supported_out
    );
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_am_query_plan_free(&plan);
        return status;
    }

    if (!*supported_out)
    {
        psql_bm25s_am_query_plan_free(&plan);
        return PSQL_BM25S_OK;
    }

    *plan_out = plan;
    return PSQL_BM25S_OK;
}

static void
psql_bm25s_am_union_candidate_id_sets(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *left_ids,
    size_t left_len,
    const uint32_t *right_ids,
    size_t right_len,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    size_t capacity = left_len + right_len;
    size_t bytes = 0;
    size_t i;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (capacity == 0)
    {
        return;
    }

    if (!psql_bm25s_am_checked_mul_size(
            capacity,
            sizeof(*candidate_ids),
            &bytes))
    {
        psql_bm25s_am_oom();
    }
    candidate_ids = malloc(bytes);
    if (candidate_ids == NULL)
    {
        psql_bm25s_am_oom();
    }

    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    for (i = 0; i < left_len; i++)
    {
        uint32_t doc_id = left_ids[i];

        if (entry->workspace.candidate_workspace[doc_id] == 0)
        {
            entry->workspace.candidate_workspace[doc_id] = 1;
            candidate_ids[candidate_len++] = doc_id;
        }
    }
    for (i = 0; i < right_len; i++)
    {
        uint32_t doc_id = right_ids[i];

        if (entry->workspace.candidate_workspace[doc_id] == 0)
        {
            entry->workspace.candidate_workspace[doc_id] = 1;
            candidate_ids[candidate_len++] = doc_id;
        }
    }
    for (i = 0; i < candidate_len; i++)
    {
        entry->workspace.candidate_workspace[candidate_ids[i]] = 0;
    }

    *candidate_ids_out = candidate_ids;
    *candidate_len_out = candidate_len;
}

static void
psql_bm25s_am_intersect_candidate_id_sets(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *left_ids,
    size_t left_len,
    const uint32_t *right_ids,
    size_t right_len,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    const uint32_t *seed_ids;
    const uint32_t *probe_ids;
    size_t seed_len;
    size_t probe_len;
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    size_t bytes = 0;
    size_t i;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (left_len == 0 || right_len == 0)
    {
        return;
    }

    seed_ids = left_ids;
    seed_len = left_len;
    probe_ids = right_ids;
    probe_len = right_len;
    if (right_len < left_len)
    {
        seed_ids = right_ids;
        seed_len = right_len;
        probe_ids = left_ids;
        probe_len = left_len;
    }

    if (!psql_bm25s_am_checked_mul_size(
            seed_len,
            sizeof(*candidate_ids),
            &bytes))
    {
        psql_bm25s_am_oom();
    }
    candidate_ids = malloc(bytes);
    if (candidate_ids == NULL)
    {
        psql_bm25s_am_oom();
    }

    psql_bm25s_am_cache_ensure_candidate_workspace(entry);
    for (i = 0; i < probe_len; i++)
    {
        entry->workspace.candidate_workspace[probe_ids[i]] = 1;
    }
    for (i = 0; i < seed_len; i++)
    {
        uint32_t doc_id = seed_ids[i];

        if (entry->workspace.candidate_workspace[doc_id] == 1)
        {
            candidate_ids[candidate_len++] = doc_id;
        }
    }
    for (i = 0; i < probe_len; i++)
    {
        entry->workspace.candidate_workspace[probe_ids[i]] = 0;
    }

    *candidate_ids_out = candidate_ids;
    *candidate_len_out = candidate_len;
}

static psql_bm25s_status
psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out,
    bool *supported_out
)
{
    psql_bm25s_status status;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (node == NULL)
    {
        *supported_out = false;
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_NOT)
    {
        *supported_out = false;
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        if (node->term_index >= query->len)
        {
            return PSQL_BM25S_ERR_INVALID;
        }

        psql_bm25s_am_collect_phrase_term_candidate_ids(
            entry,
            &query->terms[node->term_index],
            candidate_ids_out,
            candidate_len_out
        );
        *supported_out = true;
        return PSQL_BM25S_OK;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_AND ||
        node->kind == PSQL_BM25S_QUERY_NODE_OR)
    {
        uint32_t *left_ids = NULL;
        uint32_t *right_ids = NULL;
        size_t left_len = 0;
        size_t right_len = 0;
        bool left_supported = false;
        bool right_supported = false;

        status = psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
            entry,
            query,
            node->left,
            &left_ids,
            &left_len,
            &left_supported
        );
        if (status != PSQL_BM25S_OK || !left_supported)
        {
            free(left_ids);
            *supported_out = false;
            return status;
        }

        status = psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
            entry,
            query,
            node->right,
            &right_ids,
            &right_len,
            &right_supported
        );
        if (status != PSQL_BM25S_OK || !right_supported)
        {
            free(left_ids);
            free(right_ids);
            *supported_out = false;
            return status;
        }

        if (node->kind == PSQL_BM25S_QUERY_NODE_AND)
        {
            psql_bm25s_am_intersect_candidate_id_sets(
                entry,
                left_ids,
                left_len,
                right_ids,
                right_len,
                candidate_ids_out,
                candidate_len_out
            );
        }
        else
        {
            psql_bm25s_am_union_candidate_id_sets(
                entry,
                left_ids,
                left_len,
                right_ids,
                right_len,
                candidate_ids_out,
                candidate_len_out
            );
        }

        free(left_ids);
        free(right_ids);
        *supported_out = true;
        return PSQL_BM25S_OK;
    }

    *supported_out = false;
    return PSQL_BM25S_OK;
}

static void
psql_bm25s_am_collect_phrase_term_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query_term *term,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    uint32_t *token_ids = NULL;
    size_t token_len = 0;
    psql_bm25s_status status;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (entry == NULL || term == NULL)
    {
        return;
    }

    status = psql_bm25s_am_expand_query_term_ids(
        entry,
        term,
        &token_ids,
        &token_len
    );
    if (status != PSQL_BM25S_OK)
    {
        ereport(ERROR, (errmsg("failed to expand phrase term ids: %s",
                               psql_bm25s_strerror(status))));
    }

    if (token_len == 0)
    {
        free(token_ids);
        return;
    }

    if (term->kind != PSQL_BM25S_QUERY_PHRASE || token_len == 1)
    {
        psql_bm25s_am_collect_candidate_ids_from_query_ids(
            entry,
            token_ids,
            token_len,
            false,
            candidate_ids_out,
            candidate_len_out
        );
        free(token_ids);
        return;
    }

    {
        uint32_t *ordered_token_ids = NULL;
        size_t i;
        uint32_t *candidate_ids = NULL;
        size_t candidate_len = 0;
        size_t bytes = 0;

        if (!psql_bm25s_am_checked_mul_size(
                token_len,
                sizeof(*ordered_token_ids),
                &bytes))
        {
            free(token_ids);
            psql_bm25s_am_oom();
        }
        ordered_token_ids = malloc(bytes);
        if (ordered_token_ids == NULL)
        {
            free(token_ids);
            psql_bm25s_am_oom();
        }
        memcpy(ordered_token_ids, token_ids, bytes);

        for (i = 1; i < token_len; i++)
        {
            uint32_t token_id = ordered_token_ids[i];
            uint64_t token_span = token_id < entry->generation.index.vocab_size ?
                (entry->generation.index.indptr[token_id + 1] -
                 entry->generation.index.indptr[token_id]) :
                UINT64_MAX;
            size_t j = i;

            while (j > 0)
            {
                uint32_t prev_id = ordered_token_ids[j - 1];
                uint64_t prev_span = prev_id < entry->generation.index.vocab_size ?
                    (entry->generation.index.indptr[prev_id + 1] -
                     entry->generation.index.indptr[prev_id]) :
                    UINT64_MAX;

                if (prev_span <= token_span)
                {
                    break;
                }
                ordered_token_ids[j] = ordered_token_ids[j - 1];
                j--;
            }
            ordered_token_ids[j] = token_id;
        }

        psql_bm25s_am_collect_candidate_ids_from_query_ids(
            entry,
            &ordered_token_ids[0],
            1,
            false,
            &candidate_ids,
            &candidate_len
        );

        for (i = 1; i < token_len && candidate_len > 0; i++)
        {
            uint32_t token_id = ordered_token_ids[i];
            uint64_t start;
            uint64_t end;
            uint64_t j;
            size_t write_len = 0;

            if (token_id >= entry->generation.index.vocab_size)
            {
                candidate_len = 0;
                break;
            }

            start = entry->generation.index.indptr[token_id];
            end = entry->generation.index.indptr[token_id + 1];
            if (start == end)
            {
                candidate_len = 0;
                break;
            }

            psql_bm25s_am_cache_ensure_candidate_workspace(entry);
            for (j = start; j < end; j++)
            {
                entry->workspace.candidate_workspace[entry->generation.index.indices[j]] = 1;
            }
            for (j = 0; j < candidate_len; j++)
            {
                uint32_t doc_id = candidate_ids[j];

                if (entry->workspace.candidate_workspace[doc_id] == 0)
                {
                    continue;
                }
                candidate_ids[write_len++] = doc_id;
            }
            for (j = start; j < end; j++)
            {
                entry->workspace.candidate_workspace[entry->generation.index.indices[j]] = 0;
            }
            candidate_len = write_len;
        }

        free(ordered_token_ids);
        free(token_ids);
        *candidate_ids_out = candidate_ids;
        *candidate_len_out = candidate_len;
    }
}

static void
psql_bm25s_am_collect_phrase_query_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_query *query,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    uint32_t *must_ids = NULL;
    size_t must_len = 0;
    uint32_t *should_ids = NULL;
    size_t should_len = 0;
    size_t i;
    size_t must_clause_count = 0;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (entry == NULL || query == NULL)
    {
        return;
    }

    for (i = 0; i < query->len; i++)
    {
        const psql_bm25s_query_term *term = &query->terms[i];
        uint32_t *term_ids = NULL;
        size_t term_len = 0;

        if (term->occur == PSQL_BM25S_QUERY_MUST_NOT)
        {
            continue;
        }

        psql_bm25s_am_collect_phrase_term_candidate_ids(
            entry,
            term,
            &term_ids,
            &term_len
        );

        if (term->occur == PSQL_BM25S_QUERY_MUST)
        {
            uint32_t *merged_ids = NULL;
            size_t merged_len = 0;

            must_clause_count++;
            if (must_ids == NULL)
            {
                must_ids = term_ids;
                must_len = term_len;
                continue;
            }

            psql_bm25s_am_intersect_candidate_id_sets(
                entry,
                must_ids,
                must_len,
                term_ids,
                term_len,
                &merged_ids,
                &merged_len
            );
            free(must_ids);
            free(term_ids);
            must_ids = merged_ids;
            must_len = merged_len;
            continue;
        }

        if (should_ids == NULL)
        {
            should_ids = term_ids;
            should_len = term_len;
            continue;
        }

        {
            uint32_t *merged_ids = NULL;
            size_t merged_len = 0;

            psql_bm25s_am_union_candidate_id_sets(
                entry,
                should_ids,
                should_len,
                term_ids,
                term_len,
                &merged_ids,
                &merged_len
            );
            free(should_ids);
            free(term_ids);
            should_ids = merged_ids;
            should_len = merged_len;
        }
    }

    if (must_clause_count > 0)
    {
        free(should_ids);
        *candidate_ids_out = must_ids;
        *candidate_len_out = must_len;
        return;
    }

    *candidate_ids_out = should_ids;
    *candidate_len_out = should_len;
}

static void
psql_bm25s_am_prepare_filtered_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation heapRelation,
    const psql_bm25s_am_query_plan *plan,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    uint32_t *candidate_ids = NULL;
    float *scores = NULL;
    float *weight_mask = NULL;
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_am_visibility_ctx visibility = {0};
    size_t candidate_len = 0;
    size_t rank_k;
    bool use_sparse_scores;
    psql_bm25s_status status;

    memset(state_out, 0, sizeof(*state_out));
    if (requested_k == 0)
    {
        return;
    }

    psql_bm25s_am_collect_query_candidate_ids(
        entry,
        plan,
        &candidate_ids,
        &candidate_len
    );
    if (candidate_len == 0)
    {
        return;
    }

    weight_mask = psql_bm25s_array_read_weight_mask(
        weight_mask_array,
        entry->generation.index.num_docs
    );
    rank_k = Min(candidate_len, Max(requested_k, (size_t) 1));
    use_sparse_scores = psql_bm25s_am_sparse_weight_mask_supported(
        weight_mask,
        entry->generation.index.num_docs
    );
    if (use_sparse_scores)
    {
        psql_bm25s_am_rank_signed_sparse_candidates(
            entry,
            candidate_ids,
            candidate_len,
            plan->positive_ids,
            plan->positive_len,
            weight_mask,
            rank_k,
            true,
            &ranked
        );
    }
    else
    {
        status = psql_bm25s_scores_from_ids(
            &entry->generation.index,
            plan->positive_ids,
            plan->positive_len,
            weight_mask,
            &scores
        );
        if (status != PSQL_BM25S_OK)
        {
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            free(candidate_ids);
            ereport(ERROR, (errmsg("failed to score query: %s",
                                   psql_bm25s_strerror(status))));
        }

        status = psql_bm25s_topk_subset(
            scores,
            candidate_ids,
            candidate_len,
            rank_k,
            true,
            true,
            &ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            free(scores);
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            free(candidate_ids);
            ereport(ERROR, (errmsg("failed to compute filtered top-k: %s",
                                   psql_bm25s_strerror(status))));
        }
    }

    psql_bm25s_am_visibility_begin(heapRelation, &visibility);
    psql_bm25s_am_collect_visible_hits(
        entry->generation.doc_tids,
        &ranked,
        requested_k,
        &visibility,
        state_out
    );
    if (state_out->topk.len < requested_k && rank_k < candidate_len)
    {
        psql_bm25s_topk_result_free(&ranked);
        memset(&ranked, 0, sizeof(ranked));
        if (use_sparse_scores)
        {
            psql_bm25s_am_rank_signed_sparse_candidates(
                entry,
                candidate_ids,
                candidate_len,
                plan->positive_ids,
                plan->positive_len,
                weight_mask,
                candidate_len,
                true,
                &ranked
            );
        }
        else
        {
            status = psql_bm25s_topk_subset(
                scores,
                candidate_ids,
                candidate_len,
                candidate_len,
                true,
                true,
                &ranked
            );
            if (status != PSQL_BM25S_OK)
            {
                free(scores);
                if (weight_mask != NULL)
                {
                    pfree(weight_mask);
                }
                free(candidate_ids);
                ereport(ERROR, (errmsg("failed to expand filtered top-k: %s",
                                       psql_bm25s_strerror(status))));
            }
        }

        if (state_out->tids != NULL)
        {
            pfree(state_out->tids);
        }
        psql_bm25s_topk_result_free(&state_out->topk);
        memset(state_out, 0, sizeof(*state_out));
        psql_bm25s_am_collect_visible_hits(
            entry->generation.doc_tids,
            &ranked,
            requested_k,
            &visibility,
            state_out
        );
    }
    psql_bm25s_am_visibility_end(&visibility);
    psql_bm25s_topk_result_free(&ranked);
    free(candidate_ids);
    if (scores != NULL)
    {
        free(scores);
    }
    if (weight_mask != NULL)
    {
        pfree(weight_mask);
    }
}

static void
psql_bm25s_am_prepare_raw_query_search_state_with_options(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    text *query_text,
    const psql_bm25s_text_options *options,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_query query;
    psql_bm25s_am_query_plan plan;
    char **query_tokens = NULL;
    uint32_t *query_ids = NULL;
    size_t query_id_len = 0;
    psql_bm25s_status status;
    size_t i;

    psql_bm25s_am_query_plan_init(&plan);
    psql_bm25s_am_parse_raw_query_with_options(query_text, options, &query);

    if (query.len == 0)
    {
        psql_bm25s_query_free(&query);
        ereport(ERROR, (errmsg("query produced no searchable tokens")));
    }
    if (psql_bm25s_am_field_aware_enabled(indexRelation) &&
        !psql_bm25s_query_is_simple_term_query(&query))
    {
        psql_bm25s_query_free(&query);
        ereport(
            ERROR,
            (
                errmsg(
                    "field-aware generic raw queries support simple terms only"
                ),
                errhint(
                    "Use psql_bm25s_field_aware_query_tokens() with explicit "
                    "tokens for field-aware weighted retrieval."
                )
            )
        );
    }

    if (psql_bm25s_query_is_simple_term_query(&query))
    {
        query_tokens = palloc(sizeof(*query_tokens) * query.len);
        for (i = 0; i < query.len; i++)
        {
            query_tokens[i] = query.terms[i].tokens[0];
        }

        psql_bm25s_am_prepare_tokens_search_state(
            entry,
            indexRelation,
            heapRelation,
            query_tokens,
            query.len,
            weight_mask,
            requested_k,
            state_out
        );

        pfree(query_tokens);
    }
    else
    {
        if (psql_bm25s_query_uses_boolean_ast(&query))
        {
            bool used_conjunction_plan = false;
            bool used_positive_ast_plan = false;

            status = psql_bm25s_am_collect_positive_query_ids(
                entry,
                &query,
                &query_ids,
                &query_id_len
            );
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_query_free(&query);
                ereport(ERROR, (errmsg("failed to plan boolean raw query: %s",
                                       psql_bm25s_strerror(status))));
            }
            if (query_id_len == 0)
            {
                free(query_ids);
                psql_bm25s_query_free(&query);
                ereport(
                    ERROR,
                    (
                        errmsg("raw query API requires at least one positive term")
                    )
                );
            }

            status = psql_bm25s_am_build_boolean_ast_conjunction_plan(
                entry,
                &query,
                &plan,
                &used_conjunction_plan
            );
            if (status != PSQL_BM25S_OK)
            {
                free(query_ids);
                psql_bm25s_query_free(&query);
                ereport(ERROR, (errmsg("failed to plan boolean raw query: %s",
                                       psql_bm25s_strerror(status))));
            }

            if (used_conjunction_plan && plan.positive_clause_count > 0)
            {
                uint32_t *candidate_ids = NULL;
                size_t candidate_len = 0;

                psql_bm25s_am_collect_query_candidate_ids(
                    entry,
                    &plan,
                    &candidate_ids,
                    &candidate_len
                );
                psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
                    entry,
                    indexRelation,
                    heapRelation,
                    &query,
                    candidate_ids,
                    candidate_len,
                    query_ids,
                    query_id_len,
                    weight_mask,
                    requested_k,
                    state_out
                );
                free(candidate_ids);
            }
            else
            {
                uint32_t *candidate_ids = NULL;
                size_t candidate_len = 0;

                status = psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
                    entry,
                    &query,
                    query.root,
                    &candidate_ids,
                    &candidate_len,
                    &used_positive_ast_plan
                );
                if (status != PSQL_BM25S_OK)
                {
                    free(query_ids);
                    psql_bm25s_query_free(&query);
                    ereport(ERROR, (errmsg("failed to plan boolean raw query: %s",
                                           psql_bm25s_strerror(status))));
                }

                if (used_positive_ast_plan)
                {
                    psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
                        entry,
                        indexRelation,
                        heapRelation,
                        &query,
                        candidate_ids,
                        candidate_len,
                        query_ids,
                        query_id_len,
                        weight_mask,
                        requested_k,
                        state_out
                    );
                    free(candidate_ids);
                }
                else
                {
                    psql_bm25s_am_prepare_verified_query_search_state(
                        entry,
                        indexRelation,
                        heapRelation,
                        &query,
                        query_ids,
                        query_id_len,
                        weight_mask,
                        requested_k,
                        state_out
                    );
                }
            }
        }
        else
        {
            status = psql_bm25s_am_build_query_plan(entry, &query, &plan);
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_query_free(&query);
                ereport(ERROR, (errmsg("failed to plan raw query: %s",
                                       psql_bm25s_strerror(status))));
            }

            if (plan.positive_clause_count == 0)
            {
                psql_bm25s_am_query_plan_free(&plan);
                psql_bm25s_query_free(&query);
                ereport(
                    ERROR,
                    (
                        errmsg("raw query API requires at least one positive term")
                    )
                );
            }

            if (psql_bm25s_query_has_phrase(&query))
            {
                psql_bm25s_am_prepare_phrase_query_search_state(
                    entry,
                    indexRelation,
                    heapRelation,
                    &query,
                    &plan,
                    weight_mask,
                    requested_k,
                    state_out
                );
            }
            else if (plan.must_clause_count == 0 && !plan.has_must_not)
            {
                psql_bm25s_am_prepare_search_state(
                    entry,
                    heapRelation,
                    plan.positive_ids,
                    plan.positive_len,
                    weight_mask,
                    requested_k,
                    state_out
                );
            }
            else
            {
                psql_bm25s_am_prepare_filtered_query_search_state(
                    entry,
                    heapRelation,
                    &plan,
                    weight_mask,
                    requested_k,
                    state_out
                );
            }
        }
    }

    free(query_ids);
    psql_bm25s_am_query_plan_free(&plan);
    psql_bm25s_query_free(&query);
}

static void
psql_bm25s_am_prepare_raw_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    text *query_text,
    ArrayType *weight_mask,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    psql_bm25s_text_options options;
    char **stopwords = NULL;
    size_t stopword_len = 0;

    if (psql_bm25s_am_source_type_is_scalar_text(psql_bm25s_am_source_type(
            indexRelation)))
    {
        psql_bm25s_am_read_index_text_options(
            indexRelation,
            &options,
            &stopwords,
            &stopword_len
        );
    }
    else
    {
        psql_bm25s_text_options_init(&options);
        options.lowercase = false;
    }
    psql_bm25s_am_prepare_raw_query_search_state_with_options(
        entry,
        indexRelation,
        heapRelation,
        query_text,
        &options,
        weight_mask,
        requested_k,
        state_out
    );
    psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
}

static bool
psql_bm25s_am_mark_query_term_matches(
    char **doc_tokens,
    size_t doc_len,
    const psql_bm25s_query_term *term,
    bool *marks
)
{
    bool matched = false;
    size_t i;

    if (term == NULL)
    {
        return false;
    }

    if (term->kind == PSQL_BM25S_QUERY_TERM)
    {
        for (i = 0; i < doc_len; i++)
        {
            if (strcmp(doc_tokens[i], term->tokens[0]) == 0)
            {
                matched = true;
                if (marks != NULL)
                {
                    marks[i] = true;
                }
            }
        }

        return matched;
    }

    if (term->kind == PSQL_BM25S_QUERY_PREFIX)
    {
        size_t prefix_len = psql_bm25s_am_query_term_token_len(term, 0);

        for (i = 0; i < doc_len; i++)
        {
            if (strncmp(doc_tokens[i], term->tokens[0], prefix_len) == 0)
            {
                matched = true;
                if (marks != NULL)
                {
                    marks[i] = true;
                }
            }
        }

        return matched;
    }

    if (term->kind == PSQL_BM25S_QUERY_PHRASE)
    {
        if (term->len == 0 || term->len > doc_len)
        {
            return false;
        }

        for (i = 0; i + term->len <= doc_len; i++)
        {
            size_t j;
            bool phrase_match = true;

            for (j = 0; j < term->len; j++)
            {
                if (strcmp(doc_tokens[i + j], term->tokens[j]) != 0)
                {
                    phrase_match = false;
                    break;
                }
            }

            if (!phrase_match)
            {
                continue;
            }

            matched = true;
            if (marks != NULL)
            {
                for (j = 0; j < term->len; j++)
                {
                    marks[i + j] = true;
                }
            }
        }
    }

    return matched;
}

static bool
psql_bm25s_am_doc_matches_query_node(
    char **doc_tokens,
    size_t doc_len,
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node
)
{
    if (query == NULL || node == NULL)
    {
        return false;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        if (node->term_index >= query->len)
        {
            return false;
        }

        return psql_bm25s_am_mark_query_term_matches(
            doc_tokens,
            doc_len,
            &query->terms[node->term_index],
            NULL
        );
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_NOT)
    {
        return !psql_bm25s_am_doc_matches_query_node(
            doc_tokens,
            doc_len,
            query,
            node->left
        );
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_AND)
    {
        return psql_bm25s_am_doc_matches_query_node(
                   doc_tokens,
                   doc_len,
                   query,
                   node->left
               ) &&
               psql_bm25s_am_doc_matches_query_node(
                   doc_tokens,
                   doc_len,
                   query,
                   node->right
               );
    }

    return psql_bm25s_am_doc_matches_query_node(
               doc_tokens,
               doc_len,
               query,
               node->left
           ) ||
           psql_bm25s_am_doc_matches_query_node(
               doc_tokens,
               doc_len,
               query,
               node->right
           );
}

static bool
psql_bm25s_am_doc_matches_query(
    char **doc_tokens,
    size_t doc_len,
    const psql_bm25s_query *query,
    bool *marks
)
{
    if (query == NULL || query->root == NULL)
    {
        return false;
    }

    if (marks != NULL && psql_bm25s_query_uses_boolean_ast(query))
    {
        size_t i;

        for (i = 0; i < query->len; i++)
        {
            const psql_bm25s_query_term *term = &query->terms[i];

            (void) psql_bm25s_am_mark_query_term_matches(
                doc_tokens,
                doc_len,
                term,
                marks
            );
        }
    }
    else if (marks != NULL)
    {
        size_t i;

        for (i = 0; i < query->len; i++)
        {
            const psql_bm25s_query_term *term = &query->terms[i];

            if (term->occur == PSQL_BM25S_QUERY_MUST_NOT)
            {
                continue;
            }

            (void) psql_bm25s_am_mark_query_term_matches(
                doc_tokens,
                doc_len,
                term,
                marks
            );
        }
    }

    return psql_bm25s_am_doc_matches_query_node(
        doc_tokens,
        doc_len,
        query,
        query->root
    );
}

static void
psql_bm25s_am_mark_query_matches(
    char **doc_tokens,
    size_t doc_len,
    const psql_bm25s_query *query,
    bool *marks
)
{
    (void) psql_bm25s_am_doc_matches_query(doc_tokens, doc_len, query, marks);
}

static void
psql_bm25s_am_choose_snippet_window(
    const bool *marks,
    size_t doc_len,
    size_t max_tokens,
    size_t *start_out,
    size_t *end_out
)
{
    size_t start = 0;
    size_t end = doc_len;
    size_t first_mark = doc_len;
    size_t i;

    if (max_tokens == 0 || doc_len <= max_tokens)
    {
        *start_out = 0;
        *end_out = doc_len;
        return;
    }

    for (i = 0; i < doc_len; i++)
    {
        if (marks[i])
        {
            first_mark = i;
            break;
        }
    }

    if (first_mark == doc_len)
    {
        *start_out = 0;
        *end_out = max_tokens;
        return;
    }

    start = first_mark > (max_tokens / 2) ?
        first_mark - (max_tokens / 2) : 0;
    if (start + max_tokens > doc_len)
    {
        start = doc_len - max_tokens;
    }
    end = start + max_tokens;

    *start_out = start;
    *end_out = end;
}

static text *
psql_bm25s_am_render_highlighted_tokens(
    char **doc_tokens,
    size_t doc_len,
    const bool *marks,
    size_t start,
    size_t end,
    const char *start_tag,
    const char *end_tag
)
{
    StringInfoData buffer;
    size_t i;
    text *result;

    initStringInfo(&buffer);
    if (start > 0)
    {
        appendStringInfoString(&buffer, "... ");
    }

    for (i = start; i < end; i++)
    {
        if (i > start)
        {
            appendStringInfoChar(&buffer, ' ');
        }

        if (marks != NULL && marks[i])
        {
            appendStringInfoString(&buffer, start_tag);
            appendStringInfoString(&buffer, doc_tokens[i]);
            appendStringInfoString(&buffer, end_tag);
        }
        else
        {
            appendStringInfoString(&buffer, doc_tokens[i]);
        }
    }

    if (end < doc_len)
    {
        appendStringInfoString(&buffer, " ...");
    }

    result = cstring_to_text_with_len(buffer.data, buffer.len);
    pfree(buffer.data);
    return result;
}

static bool
psql_bm25s_am_fetch_doc_tokens_for_tid(
    Relation heapRelation,
    TupleTableSlot *slot,
    const AttrNumber *attnums,
    int natts,
    Oid source_type,
    const psql_bm25s_text_options *text_options,
    const ItemPointerData *tid,
    Snapshot snapshot,
    psql_bm25s_doc_tokens *doc_out
)
{
    bool found;
    Datum values[INDEX_MAX_KEYS];
    bool isnull[INDEX_MAX_KEYS];
    ItemPointerData fetch_tid;
    int i;

    memset(doc_out, 0, sizeof(*doc_out));
    if (natts <= 0 || natts > INDEX_MAX_KEYS)
    {
        ereport(ERROR, (errmsg("invalid psql_bm25s index definition")));
    }

    ExecClearTuple(slot);
    fetch_tid = *tid;
    found = table_tuple_fetch_row_version(
        heapRelation,
        &fetch_tid,
        snapshot,
        slot
    );
    if (!found)
    {
        return false;
    }

    for (i = 0; i < natts; i++)
    {
        values[i] = slot_getattr(slot, attnums[i], &isnull[i]);
    }
    if (psql_bm25s_am_all_index_values_null(isnull, natts))
    {
        ereport(
            ERROR,
            (
                errmsg("indexed row unexpectedly returned all NULL indexed columns")
            )
        );
    }

    if (natts > 1)
    {
        psql_bm25s_am_collect_token_doc_from_values(
            values,
            isnull,
            natts,
            source_type,
            text_options,
            doc_out
        );
        return true;
    }

    if (psql_bm25s_am_source_type_is_text_array(source_type))
    {
        ArrayType *array = DatumGetArrayTypeP(values[0]);

        doc_out->tokens = (const char **) psql_bm25s_array_read_doc_tokens(
            array,
            &doc_out->len
        );
        return true;
    }

    if (isnull[0])
    {
        ereport(
            ERROR,
            (
                errmsg("indexed text or varchar column unexpectedly returned NULL")
            )
        );
    }

    psql_bm25s_am_tokenize_scalar_datum(values[0], text_options, doc_out);
    return true;
}

static void
psql_bm25s_am_collect_simple_index_attnums(
    Relation indexRelation,
    AttrNumber *attnums_out,
    int natts
)
{
    int i;

    for (i = 0; i < natts; i++)
    {
        AttrNumber attnum = indexRelation->rd_index->indkey.values[i];

        if (attnum <= 0)
        {
            ereport(
                ERROR,
                (
                    errmsg(
                        "phrase/verified retrieval requires simple indexed text[], varchar[], text, or varchar columns"
                    )
                )
            );
        }
        attnums_out[i] = attnum;
    }
}

static bool
psql_bm25s_am_scan_doc_matches_verify_query(
    psql_bm25s_am_scan_opaque *opaque,
    const ItemPointerData *tid
)
{
    psql_bm25s_doc_tokens doc = {0};
    bool matched;

    if (opaque == NULL || !opaque->verify_query_active)
    {
        return true;
    }

    if (!psql_bm25s_am_fetch_doc_tokens_for_tid(
            opaque->heap_relation,
            opaque->visibility.slot,
            &opaque->verify_attnum,
            1,
            opaque->verify_source_type,
            &opaque->verify_text_options,
            tid,
            opaque->visibility.snapshot,
            &doc))
    {
        return false;
    }

    matched = psql_bm25s_am_doc_matches_query(
        (char **) doc.tokens,
        doc.len,
        &opaque->verify_query,
        NULL
    );
    psql_bm25s_am_free_token_doc(&doc);
    return matched;
}

static void
psql_bm25s_am_collect_verified_hits(
    const psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const psql_bm25s_topk_result *ranked,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    size_t i;
    size_t accepted = 0;
    TupleTableSlot *slot;
    AttrNumber attnums[INDEX_MAX_KEYS];
    int natts = psql_bm25s_am_index_natts(indexRelation);
    bool is_multicol = psql_bm25s_am_is_multicol_index(indexRelation);
    Oid source_type = psql_bm25s_am_source_type(indexRelation);
    psql_bm25s_text_options text_options;
    char **text_stopwords = NULL;
    size_t text_stopword_len = 0;

    if (requested_k == 0)
    {
        return;
    }

    if (!is_multicol)
    {
        psql_bm25s_am_collect_simple_index_attnums(indexRelation, attnums, 1);
    }
    else
    {
        psql_bm25s_am_collect_simple_index_attnums(indexRelation, attnums, natts);
    }
    if (psql_bm25s_am_source_type_is_scalar_text(source_type))
    {
        psql_bm25s_am_read_index_text_options(
            indexRelation,
            &text_options,
            &text_stopwords,
            &text_stopword_len
        );
    }
    else
    {
        psql_bm25s_text_options_init(&text_options);
    }

    psql_bm25s_am_init_search_result(requested_k, state_out);
    slot = table_slot_create(heapRelation, NULL);
    for (i = 0; i < ranked->len && accepted < requested_k; i++)
    {
        uint32_t doc_id = ranked->doc_ids[i];
        psql_bm25s_doc_tokens doc = {0};

        if (!psql_bm25s_am_fetch_doc_tokens_for_tid(
                heapRelation,
                slot,
                attnums,
                natts,
                source_type,
                &text_options,
                &entry->generation.doc_tids[doc_id],
                GetActiveSnapshot(),
                &doc))
        {
            continue;
        }

        if (psql_bm25s_am_doc_matches_query(
                (char **) doc.tokens,
                doc.len,
                query,
                NULL))
        {
            psql_bm25s_am_append_search_hit(
                state_out,
                accepted,
                &entry->generation.doc_tids[doc_id],
                doc_id,
                ranked->scores[i]
            );
            accepted++;
        }
        psql_bm25s_am_free_token_doc(&doc);
    }

    ExecDropSingleTupleTableSlot(slot);
    psql_bm25s_am_free_query_tokens(text_stopwords, text_stopword_len);
    state_out->topk.len = accepted;
}

static void
psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    float *scores = NULL;
    float *weight_mask = NULL;
    psql_bm25s_topk_result ranked = {0};
    psql_bm25s_status status;
    size_t rank_k;
    bool use_sparse_scores;

    memset(state_out, 0, sizeof(*state_out));
    if (requested_k == 0)
    {
        return;
    }

    if (candidate_len == 0)
    {
        return;
    }

    weight_mask = psql_bm25s_array_read_weight_mask(
        weight_mask_array,
        entry->generation.index.num_docs
    );
    rank_k = Min(candidate_len, Max(requested_k, (size_t) 1));
    use_sparse_scores = psql_bm25s_am_sparse_weight_mask_supported(
        weight_mask,
        entry->generation.index.num_docs
    );
    if (use_sparse_scores)
    {
        if (psql_bm25s_am_can_use_unsigned_sparse_path(entry, weight_mask))
        {
            psql_bm25s_am_rank_unsigned_sparse_candidates(
                entry,
                candidate_ids,
                candidate_len,
                query_ids,
                query_len,
                weight_mask,
                rank_k,
                true,
                &ranked
            );
        }
        else
        {
            psql_bm25s_am_rank_signed_sparse_candidates(
                entry,
                candidate_ids,
                candidate_len,
                query_ids,
                query_len,
                weight_mask,
                rank_k,
                true,
                &ranked
            );
        }
    }
    else
    {
        status = psql_bm25s_scores_from_ids(
            &entry->generation.index,
            query_ids,
            query_len,
            weight_mask,
            &scores
        );
        if (status != PSQL_BM25S_OK)
        {
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            ereport(ERROR, (errmsg("failed to score verified query: %s",
                                   psql_bm25s_strerror(status))));
        }

        status = psql_bm25s_topk_subset(
            scores,
            candidate_ids,
            candidate_len,
            rank_k,
            true,
            true,
            &ranked
        );
        if (status != PSQL_BM25S_OK)
        {
            if (weight_mask != NULL)
            {
                pfree(weight_mask);
            }
            free(scores);
            ereport(ERROR, (errmsg("failed to rank verified query candidates: %s",
                                   psql_bm25s_strerror(status))));
        }
    }

    psql_bm25s_am_collect_verified_hits(
        entry,
        indexRelation,
        heapRelation,
        query,
        &ranked,
        requested_k,
        state_out
    );

    if (state_out->topk.len < requested_k && rank_k < candidate_len)
    {
        psql_bm25s_topk_result_free(&ranked);
        memset(&ranked, 0, sizeof(ranked));
        if (use_sparse_scores)
        {
            if (psql_bm25s_am_can_use_unsigned_sparse_path(
                    entry,
                    weight_mask))
            {
                psql_bm25s_am_rank_unsigned_sparse_candidates(
                    entry,
                    candidate_ids,
                    candidate_len,
                    query_ids,
                    query_len,
                    weight_mask,
                    candidate_len,
                    true,
                    &ranked
                );
            }
            else
            {
                psql_bm25s_am_rank_signed_sparse_candidates(
                    entry,
                    candidate_ids,
                    candidate_len,
                    query_ids,
                    query_len,
                    weight_mask,
                    candidate_len,
                    true,
                    &ranked
                );
            }
        }
        else
        {
            status = psql_bm25s_topk_subset(
                scores,
                candidate_ids,
                candidate_len,
                candidate_len,
                true,
                true,
                &ranked
            );
            if (status != PSQL_BM25S_OK)
            {
                if (weight_mask != NULL)
                {
                    pfree(weight_mask);
                }
                free(scores);
                ereport(ERROR, (errmsg("failed to expand verified query candidates: %s",
                                       psql_bm25s_strerror(status))));
            }
        }

        if (state_out->tids != NULL)
        {
            pfree(state_out->tids);
        }
        psql_bm25s_topk_result_free(&state_out->topk);
        memset(state_out, 0, sizeof(*state_out));
        psql_bm25s_am_collect_verified_hits(
            entry,
            indexRelation,
            heapRelation,
            query,
            &ranked,
            requested_k,
            state_out
        );
    }

    psql_bm25s_topk_result_free(&ranked);
    if (weight_mask != NULL)
    {
        pfree(weight_mask);
    }
    free(scores);
}

static void
psql_bm25s_am_prepare_verified_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const uint32_t *query_ids,
    size_t query_len,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    uint32_t *all_doc_ids = NULL;
    size_t bytes;
    size_t i;

    if (query_len == 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                entry->generation.index.num_docs,
                sizeof(*all_doc_ids),
                &bytes))
        {
            psql_bm25s_am_oom();
        }
        all_doc_ids = malloc(bytes);
        if (all_doc_ids == NULL)
        {
            psql_bm25s_am_oom();
        }
        for (i = 0; i < entry->generation.index.num_docs; i++)
        {
            all_doc_ids[i] = (uint32_t) i;
        }

        psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
            entry,
            indexRelation,
            heapRelation,
            query,
            all_doc_ids,
            entry->generation.index.num_docs,
            query_ids,
            query_len,
            weight_mask_array,
            requested_k,
            state_out
        );
        free(all_doc_ids);
        return;
    }

    psql_bm25s_am_collect_candidate_ids_from_query_ids(
        entry,
        query_ids,
        query_len,
        false,
        &candidate_ids,
        &candidate_len
    );
    psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
        entry,
        indexRelation,
        heapRelation,
        query,
        candidate_ids,
        candidate_len,
        query_ids,
        query_len,
        weight_mask_array,
        requested_k,
        state_out
    );
    free(candidate_ids);
}

static void
psql_bm25s_am_prepare_phrase_query_search_state(
    psql_bm25s_am_cache_entry *entry,
    Relation indexRelation,
    Relation heapRelation,
    const psql_bm25s_query *query,
    const psql_bm25s_am_query_plan *plan,
    ArrayType *weight_mask_array,
    size_t requested_k,
    psql_bm25s_search_state *state_out
)
{
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;

    if (psql_bm25s_query_uses_boolean_ast(query))
    {
        psql_bm25s_am_prepare_verified_query_search_state(
            entry,
            indexRelation,
            heapRelation,
            query,
            plan->positive_ids,
            plan->positive_len,
            weight_mask_array,
            requested_k,
            state_out
        );
        return;
    }

    psql_bm25s_am_collect_phrase_query_candidate_ids(
        entry,
        query,
        &candidate_ids,
        &candidate_len
    );
    if (candidate_ids != NULL || candidate_len == 0)
    {
        psql_bm25s_am_prepare_verified_query_search_state_with_candidates(
            entry,
            indexRelation,
            heapRelation,
            query,
            candidate_ids,
            candidate_len,
            plan->positive_ids,
            plan->positive_len,
            weight_mask_array,
            requested_k,
            state_out
        );
        free(candidate_ids);
        return;
    }

    psql_bm25s_am_prepare_verified_query_search_state(
        entry,
        indexRelation,
        heapRelation,
        query,
        plan->positive_ids,
        plan->positive_len,
        weight_mask_array,
        requested_k,
        state_out
    );
}

static void
psql_bm25s_am_init_ranked_from_doc_ids(
    const uint32_t *doc_ids,
    size_t doc_len,
    psql_bm25s_topk_result *ranked_out
)
{
    size_t bytes;

    memset(ranked_out, 0, sizeof(*ranked_out));
    if (doc_len == 0)
    {
        return;
    }

    if (!psql_bm25s_am_checked_mul_size(doc_len, sizeof(*doc_ids), &bytes))
    {
        psql_bm25s_am_oom();
    }
    ranked_out->doc_ids = malloc(bytes);
    if (ranked_out->doc_ids == NULL)
    {
        psql_bm25s_am_oom();
    }
    memcpy(ranked_out->doc_ids, doc_ids, bytes);

    if (!psql_bm25s_am_checked_mul_size(
            doc_len,
            sizeof(*ranked_out->scores),
            &bytes))
    {
        psql_bm25s_topk_result_free(ranked_out);
        psql_bm25s_am_oom();
    }
    ranked_out->scores = calloc(doc_len, sizeof(*ranked_out->scores));
    if (ranked_out->scores == NULL)
    {
        psql_bm25s_topk_result_free(ranked_out);
        psql_bm25s_am_oom();
    }

    ranked_out->len = doc_len;
}

static void
psql_bm25s_am_collect_candidate_ids_from_query_ids(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *query_ids,
    size_t query_len,
    bool all_docs_if_empty,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    size_t bytes = 0;
    size_t i;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (entry == NULL)
    {
        return;
    }

    if (query_len == 0)
    {
        if (!all_docs_if_empty || entry->generation.index.num_docs == 0)
        {
            return;
        }

        if (!psql_bm25s_am_checked_mul_size(
                entry->generation.index.num_docs,
                sizeof(*candidate_ids),
                &bytes))
        {
            psql_bm25s_am_oom();
        }
        candidate_ids = malloc(bytes);
        if (candidate_ids == NULL)
        {
            psql_bm25s_am_oom();
        }
        for (i = 0; i < entry->generation.index.num_docs; i++)
        {
            candidate_ids[i] = (uint32_t) i;
        }

        *candidate_ids_out = candidate_ids;
        *candidate_len_out = entry->generation.index.num_docs;
        return;
    }

    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);

    for (i = 0; i < query_len; i++)
    {
        uint32_t token_id = query_ids[i];
        uint64_t start;
        uint64_t end;
        uint64_t pos;

        if (token_id >= entry->generation.index.vocab_size)
        {
            continue;
        }

        start = entry->generation.index.indptr[token_id];
        end = entry->generation.index.indptr[token_id + 1];
        for (pos = start; pos < end; pos++)
        {
            uint32_t doc_id = entry->generation.index.indices[pos];

            if (entry->workspace.score_workspace[doc_id] != 0.0f)
            {
                continue;
            }

            entry->workspace.score_workspace[doc_id] = 1.0f;
            entry->workspace.touched_doc_ids[entry->workspace.touched_doc_len++] = doc_id;
        }
    }

    if (entry->workspace.touched_doc_len > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                entry->workspace.touched_doc_len,
                sizeof(*candidate_ids),
                &bytes))
        {
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            psql_bm25s_am_oom();
        }
        candidate_ids = malloc(bytes);
        if (candidate_ids == NULL)
        {
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            psql_bm25s_am_oom();
        }

        for (i = 0; i < entry->workspace.touched_doc_len; i++)
        {
            candidate_ids[candidate_len++] = entry->workspace.touched_doc_ids[i];
        }
    }

    psql_bm25s_am_cache_reset_sparse_workspace(entry);
    *candidate_ids_out = candidate_ids;
    *candidate_len_out = candidate_len;
}

static void
psql_bm25s_am_collect_query_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const psql_bm25s_am_query_plan *plan,
    uint32_t **candidate_ids_out,
    size_t *candidate_len_out
)
{
    uint32_t *must_counts = NULL;
    uint32_t *last_must_clause = NULL;
    bool *excluded = NULL;
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    size_t bytes = 0;
    size_t i;

    *candidate_ids_out = NULL;
    *candidate_len_out = 0;
    if (entry == NULL || plan == NULL)
    {
        return;
    }
    if (plan->positive_clause_count == 0)
    {
        return;
    }

    /*
     * Common filtered-ranked shape: exactly one MUST clause plus optional
     * SHOULD clauses, with no MUST_NOT terms. In this case the exact
     * candidate set is just the docs reachable from the MUST clause itself;
     * collecting the union of every positive clause and then filtering it
     * back down only adds work.
     */
    if (plan->must_clause_count == 1 && !plan->has_must_not)
    {
        for (i = 0; i < plan->len; i++)
        {
            const psql_bm25s_am_query_clause *clause = &plan->clauses[i];

            if (clause->occur != PSQL_BM25S_QUERY_MUST)
            {
                continue;
            }
            psql_bm25s_am_collect_candidate_ids_from_query_ids(
                entry,
                clause->token_ids,
                clause->len,
                false,
                candidate_ids_out,
                candidate_len_out
            );
            return;
        }
    }

    /*
     * Another common filtered-ranked shape is one MUST clause combined with
     * one or more MUST_NOT clauses. The exact candidate set is "docs from the
     * MUST clause minus docs excluded by MUST_NOT". We can stay inside the
     * current candidate set and avoid allocating an index-sized exclusion
     * bitmap for the whole corpus.
     */
    if (plan->must_clause_count == 1 && plan->has_must_not)
    {
        const psql_bm25s_am_query_clause *must_clause = NULL;

        for (i = 0; i < plan->len; i++)
        {
            const psql_bm25s_am_query_clause *clause = &plan->clauses[i];

            if (clause->occur == PSQL_BM25S_QUERY_MUST)
            {
                must_clause = clause;
                break;
            }
        }

        if (must_clause != NULL)
        {
            uint32_t *must_candidate_ids = NULL;
            size_t must_candidate_len = 0;
            size_t kept_len = 0;
            size_t clause_index;

            psql_bm25s_am_collect_candidate_ids_from_query_ids(
                entry,
                must_clause->token_ids,
                must_clause->len,
                false,
                &must_candidate_ids,
                &must_candidate_len
            );
            if (must_candidate_len == 0)
            {
                *candidate_ids_out = NULL;
                *candidate_len_out = 0;
                return;
            }

            psql_bm25s_am_cache_ensure_candidate_workspace(entry);
            for (i = 0; i < must_candidate_len; i++)
            {
                entry->workspace.candidate_workspace[must_candidate_ids[i]] = 1;
            }

            for (clause_index = 0; clause_index < plan->len; clause_index++)
            {
                const psql_bm25s_am_query_clause *clause =
                    &plan->clauses[clause_index];
                size_t j;

                if (clause->occur != PSQL_BM25S_QUERY_MUST_NOT)
                {
                    continue;
                }

                for (j = 0; j < clause->len; j++)
                {
                    uint32_t token_id = clause->token_ids[j];
                    uint64_t start = entry->generation.index.indptr[token_id];
                    uint64_t end = entry->generation.index.indptr[token_id + 1];
                    uint64_t pos;

                    for (pos = start; pos < end; pos++)
                    {
                        uint32_t doc_id = entry->generation.index.indices[pos];

                        if (entry->workspace.candidate_workspace[doc_id] == 1)
                        {
                            entry->workspace.candidate_workspace[doc_id] = 2;
                        }
                    }
                }
            }

            for (i = 0; i < must_candidate_len; i++)
            {
                uint32_t doc_id = must_candidate_ids[i];

                if (entry->workspace.candidate_workspace[doc_id] == 1)
                {
                    must_candidate_ids[kept_len++] = doc_id;
                }
                entry->workspace.candidate_workspace[doc_id] = 0;
            }

            *candidate_ids_out = must_candidate_ids;
            *candidate_len_out = kept_len;
            return;
        }
    }

    /*
     * Narrow but common filtered-ranked shape: exactly two MUST clauses and
     * no MUST_NOT terms. The exact candidate set is the intersection of the
     * two clause-level candidate sets. Intersecting directly avoids the
     * generic must-count bookkeeping over the union of all touched docs.
     */
    if (plan->must_clause_count == 2 && !plan->has_must_not)
    {
        const psql_bm25s_am_query_clause *first_must = NULL;
        const psql_bm25s_am_query_clause *second_must = NULL;
        size_t first_span = 0;
        size_t second_span = 0;

        for (i = 0; i < plan->len; i++)
        {
            const psql_bm25s_am_query_clause *clause = &plan->clauses[i];
            size_t clause_span = 0;
            size_t j;

            if (clause->occur != PSQL_BM25S_QUERY_MUST)
            {
                continue;
            }

            for (j = 0; j < clause->len; j++)
            {
                uint32_t token_id = clause->token_ids[j];

                if (token_id >= entry->generation.index.vocab_size)
                {
                    continue;
                }
                clause_span += (size_t)
                    (entry->generation.index.indptr[token_id + 1] -
                     entry->generation.index.indptr[token_id]);
            }

            if (first_must == NULL)
            {
                first_must = clause;
                first_span = clause_span;
                continue;
            }

            second_must = clause;
            second_span = clause_span;
            break;
        }

        if (first_must != NULL && second_must != NULL)
        {
            const psql_bm25s_am_query_clause *seed_clause = first_must;
            const psql_bm25s_am_query_clause *probe_clause = second_must;
            uint32_t *seed_candidate_ids = NULL;
            size_t seed_candidate_len = 0;
            size_t kept_len = 0;
            size_t j;

            if (second_span < first_span)
            {
                seed_clause = second_must;
                probe_clause = first_must;
            }

            psql_bm25s_am_collect_candidate_ids_from_query_ids(
                entry,
                seed_clause->token_ids,
                seed_clause->len,
                false,
                &seed_candidate_ids,
                &seed_candidate_len
            );
            if (seed_candidate_len == 0)
            {
                *candidate_ids_out = NULL;
                *candidate_len_out = 0;
                return;
            }

            psql_bm25s_am_cache_ensure_candidate_workspace(entry);
            for (i = 0; i < seed_candidate_len; i++)
            {
                entry->workspace.candidate_workspace[seed_candidate_ids[i]] = 1;
            }

            for (j = 0; j < probe_clause->len; j++)
            {
                uint32_t token_id = probe_clause->token_ids[j];
                uint64_t start;
                uint64_t end;
                uint64_t pos;

                if (token_id >= entry->generation.index.vocab_size)
                {
                    continue;
                }

                start = entry->generation.index.indptr[token_id];
                end = entry->generation.index.indptr[token_id + 1];
                for (pos = start; pos < end; pos++)
                {
                    uint32_t doc_id = entry->generation.index.indices[pos];

                    if (entry->workspace.candidate_workspace[doc_id] == 1)
                    {
                        entry->workspace.candidate_workspace[doc_id] = 2;
                    }
                }
            }

            for (i = 0; i < seed_candidate_len; i++)
            {
                uint32_t doc_id = seed_candidate_ids[i];

                if (entry->workspace.candidate_workspace[doc_id] == 2)
                {
                    seed_candidate_ids[kept_len++] = doc_id;
                }
                entry->workspace.candidate_workspace[doc_id] = 0;
            }

            *candidate_ids_out = seed_candidate_ids;
            *candidate_len_out = kept_len;
            return;
        }
    }

    /*
     * Generalized filtered-ranked shape: three or more MUST clauses and no
     * MUST_NOT terms. Build from the narrowest MUST clause, then intersect
     * candidate ids in place with each remaining MUST clause in ascending
     * posting-span order.
     */
    if (plan->must_clause_count >= 3 && !plan->has_must_not)
    {
        const psql_bm25s_am_query_clause **must_clauses = NULL;
        size_t *must_spans = NULL;
        size_t must_index = 0;
        size_t alloc_bytes = 0;

        if (!psql_bm25s_am_checked_mul_size(
                plan->must_clause_count,
                sizeof(*must_clauses),
                &alloc_bytes))
        {
            psql_bm25s_am_oom();
        }
        must_clauses = malloc(alloc_bytes);
        if (must_clauses == NULL)
        {
            psql_bm25s_am_oom();
        }

        if (!psql_bm25s_am_checked_mul_size(
                plan->must_clause_count,
                sizeof(*must_spans),
                &alloc_bytes))
        {
            free(must_clauses);
            psql_bm25s_am_oom();
        }
        must_spans = malloc(alloc_bytes);
        if (must_spans == NULL)
        {
            free(must_clauses);
            psql_bm25s_am_oom();
        }

        for (i = 0; i < plan->len; i++)
        {
            const psql_bm25s_am_query_clause *clause = &plan->clauses[i];
            size_t clause_span = 0;
            size_t j;

            if (clause->occur != PSQL_BM25S_QUERY_MUST)
            {
                continue;
            }

            for (j = 0; j < clause->len; j++)
            {
                uint32_t token_id = clause->token_ids[j];

                if (token_id >= entry->generation.index.vocab_size)
                {
                    continue;
                }
                clause_span += (size_t)
                    (entry->generation.index.indptr[token_id + 1] -
                     entry->generation.index.indptr[token_id]);
            }

            must_clauses[must_index] = clause;
            must_spans[must_index] = clause_span;
            must_index++;
        }

        if (must_index == plan->must_clause_count)
        {
            size_t left;

            for (left = 1; left < must_index; left++)
            {
                const psql_bm25s_am_query_clause *clause =
                    must_clauses[left];
                size_t span = must_spans[left];
                size_t right = left;

                while (right > 0 && span < must_spans[right - 1])
                {
                    must_clauses[right] = must_clauses[right - 1];
                    must_spans[right] = must_spans[right - 1];
                    right--;
                }
                must_clauses[right] = clause;
                must_spans[right] = span;
            }

            {
                uint32_t *intersection_candidate_ids = NULL;
                size_t intersection_candidate_len = 0;
                size_t clause_index;

                psql_bm25s_am_collect_candidate_ids_from_query_ids(
                    entry,
                    must_clauses[0]->token_ids,
                    must_clauses[0]->len,
                    false,
                    &intersection_candidate_ids,
                    &intersection_candidate_len
                );

                if (intersection_candidate_len == 0)
                {
                    free(must_clauses);
                    free(must_spans);
                    *candidate_ids_out = NULL;
                    *candidate_len_out = 0;
                    return;
                }

                psql_bm25s_am_cache_ensure_candidate_workspace(entry);
                for (i = 0; i < intersection_candidate_len; i++)
                {
                    entry->workspace.candidate_workspace[
                        intersection_candidate_ids[i]
                    ] = 1;
                }

                for (clause_index = 1;
                     clause_index < must_index &&
                     intersection_candidate_len > 0;
                     clause_index++)
                {
                    const psql_bm25s_am_query_clause *clause =
                        must_clauses[clause_index];
                    size_t kept_len = 0;
                    size_t j;

                    for (j = 0; j < clause->len; j++)
                    {
                        uint32_t token_id = clause->token_ids[j];
                        uint64_t start;
                        uint64_t end;
                        uint64_t pos;

                        if (token_id >= entry->generation.index.vocab_size)
                        {
                            continue;
                        }

                        start = entry->generation.index.indptr[token_id];
                        end = entry->generation.index.indptr[token_id + 1];
                        for (pos = start; pos < end; pos++)
                        {
                            uint32_t doc_id = entry->generation.index.indices[pos];

                            if (entry->workspace.candidate_workspace[doc_id] == 1)
                            {
                                entry->workspace.candidate_workspace[doc_id] = 2;
                            }
                        }
                    }

                    for (i = 0; i < intersection_candidate_len; i++)
                    {
                        uint32_t doc_id = intersection_candidate_ids[i];

                        if (entry->workspace.candidate_workspace[doc_id] == 2)
                        {
                            intersection_candidate_ids[kept_len++] = doc_id;
                            entry->workspace.candidate_workspace[doc_id] = 1;
                        }
                        else
                        {
                            entry->workspace.candidate_workspace[doc_id] = 0;
                        }
                    }
                    intersection_candidate_len = kept_len;
                }

                for (i = 0; i < intersection_candidate_len; i++)
                {
                    entry->workspace.candidate_workspace[
                        intersection_candidate_ids[i]
                    ] = 0;
                }

                free(must_clauses);
                free(must_spans);
                *candidate_ids_out = intersection_candidate_ids;
                *candidate_len_out = intersection_candidate_len;
                return;
            }
        }

        free(must_clauses);
        free(must_spans);
    }

    psql_bm25s_am_cache_ensure_sparse_workspace(entry);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);

    if (plan->must_clause_count > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                entry->generation.index.num_docs,
                sizeof(*must_counts),
                &bytes))
        {
            psql_bm25s_am_oom();
        }
        must_counts = calloc(entry->generation.index.num_docs, sizeof(*must_counts));
        last_must_clause = malloc(bytes);
        if (must_counts == NULL || last_must_clause == NULL)
        {
            free(must_counts);
            free(last_must_clause);
            psql_bm25s_am_oom();
        }
        memset(last_must_clause, 0xFF, bytes);
    }

    if (plan->has_must_not)
    {
        excluded = calloc(entry->generation.index.num_docs, sizeof(*excluded));
        if (excluded == NULL)
        {
            free(must_counts);
            free(last_must_clause);
            psql_bm25s_am_oom();
        }
    }

    for (i = 0; i < plan->len; i++)
    {
        const psql_bm25s_am_query_clause *clause = &plan->clauses[i];
        size_t j;

        if (clause->occur == PSQL_BM25S_QUERY_MUST && clause->len == 0)
        {
            free(must_counts);
            free(last_must_clause);
            free(excluded);
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            return;
        }

        for (j = 0; j < clause->len; j++)
        {
            uint32_t token_id = clause->token_ids[j];
            uint64_t start = entry->generation.index.indptr[token_id];
            uint64_t end = entry->generation.index.indptr[token_id + 1];
            uint64_t pos;

            for (pos = start; pos < end; pos++)
            {
                uint32_t doc_id = entry->generation.index.indices[pos];

                if (clause->occur != PSQL_BM25S_QUERY_MUST_NOT &&
                    entry->workspace.score_workspace[doc_id] == 0.0f)
                {
                    entry->workspace.score_workspace[doc_id] = 1.0f;
                    entry->workspace.touched_doc_ids[entry->workspace.touched_doc_len++] = doc_id;
                }

                if (clause->occur == PSQL_BM25S_QUERY_MUST_NOT)
                {
                    excluded[doc_id] = true;
                }
                else if (clause->occur == PSQL_BM25S_QUERY_MUST &&
                         last_must_clause[doc_id] != (uint32_t) i)
                {
                    last_must_clause[doc_id] = (uint32_t) i;
                    must_counts[doc_id]++;
                }
            }
        }
    }

    if (entry->workspace.touched_doc_len > 0)
    {
        if (!psql_bm25s_am_checked_mul_size(
                entry->workspace.touched_doc_len,
                sizeof(*candidate_ids),
                &bytes))
        {
            free(must_counts);
            free(last_must_clause);
            free(excluded);
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            psql_bm25s_am_oom();
        }
        candidate_ids = malloc(bytes);
        if (candidate_ids == NULL)
        {
            free(must_counts);
            free(last_must_clause);
            free(excluded);
            psql_bm25s_am_cache_reset_sparse_workspace(entry);
            psql_bm25s_am_oom();
        }
    }

    for (i = 0; i < entry->workspace.touched_doc_len; i++)
    {
        uint32_t doc_id = entry->workspace.touched_doc_ids[i];

        if (excluded != NULL && excluded[doc_id])
        {
            continue;
        }
        if (must_counts != NULL && must_counts[doc_id] != plan->must_clause_count)
        {
            continue;
        }

        candidate_ids[candidate_len++] = doc_id;
    }

    free(must_counts);
    free(last_must_clause);
    free(excluded);
    psql_bm25s_am_cache_reset_sparse_workspace(entry);

    *candidate_ids_out = candidate_ids;
    *candidate_len_out = candidate_len;
}

static void
psql_bm25s_am_parse_prepared_query_datum(
    Datum prepared_datum,
    Oid expected_index_oid,
    psql_bm25s_query *query_out
)
{
    bool is_null = false;
    HeapTupleHeader prepared;
    Datum index_name_datum;
    Datum query_text_datum;
    Datum lowercase_datum;
    Datum stopwords_datum;
    Datum stem_english_datum;
    Datum fold_diacritics_datum;
    Oid index_oid;
    text *query_text;
    bool lowercase;
    ArrayType *stopwords_array;
    bool stem_english;
    bool fold_diacritics;

    prepared = DatumGetHeapTupleHeader(prepared_datum);

    index_name_datum = GetAttributeByName(prepared, "index_name", &is_null);
    if (is_null)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query index_name cannot be NULL")
            )
        );
    }
    index_oid = DatumGetObjectId(index_name_datum);
    if (OidIsValid(expected_index_oid) && index_oid != expected_index_oid)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query is bound to a different index")
            )
        );
    }

    query_text_datum = GetAttributeByName(
        prepared,
        "query_text",
        &is_null
    );
    if (is_null)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query query_text cannot be NULL")
            )
        );
    }
    query_text = DatumGetTextPP(query_text_datum);

    lowercase_datum = GetAttributeByName(
        prepared,
        "lowercase",
        &is_null
    );
    if (is_null)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query lowercase cannot be NULL")
            )
        );
    }
    lowercase = DatumGetBool(lowercase_datum);

    stopwords_datum = GetAttributeByName(
        prepared,
        "stopwords",
        &is_null
    );
    stopwords_array = is_null ? NULL : DatumGetArrayTypeP(stopwords_datum);

    stem_english_datum = GetAttributeByName(
        prepared,
        "stem_english",
        &is_null
    );
    if (is_null)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query stem_english cannot be NULL")
            )
        );
    }
    stem_english = DatumGetBool(stem_english_datum);

    fold_diacritics_datum = GetAttributeByName(
        prepared,
        "fold_diacritics",
        &is_null
    );
    if (is_null)
    {
        ereport(
            ERROR,
            (
                errmsg("prepared query fold_diacritics cannot be NULL")
            )
        );
    }
    fold_diacritics = DatumGetBool(fold_diacritics_datum);

    psql_bm25s_am_parse_raw_query_pg_options(
        query_text,
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        query_out
    );
}

static Datum
psql_bm25s_am_make_prepared_query_record(
    FunctionCallInfo fcinfo,
    bool index_isnull,
    Oid index_oid,
    bool query_isnull,
    text *query_text,
    bool lowercase,
    bool stopwords_isnull,
    ArrayType *stopwords_array,
    bool stem_english,
    bool fold_diacritics
)
{
    TupleDesc tupdesc;
    HeapTuple tuple;
    Datum values[6];
    bool nulls[6] = {false, false, false, false, false, false};

    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
    {
        ereport(ERROR, (errmsg("could not resolve result record type")));
    }

    BlessTupleDesc(tupdesc);
    if (index_isnull)
    {
        nulls[0] = true;
    }
    else
    {
        values[0] = ObjectIdGetDatum(index_oid);
    }

    if (query_isnull)
    {
        nulls[1] = true;
    }
    else
    {
        values[1] = PointerGetDatum(query_text);
    }

    values[2] = BoolGetDatum(lowercase);
    if (stopwords_isnull)
    {
        nulls[3] = true;
    }
    else
    {
        values[3] = PointerGetDatum(stopwords_array);
    }
    values[4] = BoolGetDatum(stem_english);
    values[5] = BoolGetDatum(fold_diacritics);

    tuple = heap_form_tuple(tupdesc, values, nulls);
    return HeapTupleGetDatum(tuple);
}

PG_FUNCTION_INFO_V1(psql_bm25s_prepare_query_resolved);
Datum
psql_bm25s_prepare_query_resolved(PG_FUNCTION_ARGS)
{
    bool index_isnull = PG_ARGISNULL(0);
    Oid index_oid = InvalidOid;
    bool query_isnull = PG_ARGISNULL(1);
    text *query_text = NULL;
    bool lowercase = false;
    bool lowercase_isnull = PG_ARGISNULL(2);
    ArrayType *stopwords_array = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
    bool stopwords_isnull = PG_ARGISNULL(3);
    bool stem_english = false;
    bool stem_english_isnull = PG_ARGISNULL(4);
    bool fold_diacritics = false;
    bool fold_diacritics_isnull = PG_ARGISNULL(5);
    ArrayType *resolved_stopwords_array = stopwords_array;
    bool resolved_stopwords_isnull = stopwords_isnull;
    bool owns_stopwords_array = false;

    if (!index_isnull)
    {
        Relation indexRelation;
        Oid source_type;

        index_oid = PG_GETARG_OID(0);
        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        source_type = psql_bm25s_am_require_textlike_source_type(
            indexRelation,
            "psql_bm25s raw queries require a text[], varchar[], text, or varchar index"
        );

        if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            psql_bm25s_text_options options;
            char **stopwords = NULL;
            size_t stopword_len = 0;

            psql_bm25s_am_read_index_text_options(
                indexRelation,
                &options,
                &stopwords,
                &stopword_len
            );
            if (lowercase_isnull)
            {
                lowercase = options.lowercase;
            }
            else
            {
                lowercase = PG_GETARG_BOOL(2);
            }
            if (stem_english_isnull)
            {
                stem_english = options.stem_english;
            }
            else
            {
                stem_english = PG_GETARG_BOOL(4);
            }
            if (fold_diacritics_isnull)
            {
                fold_diacritics = options.fold_diacritics;
            }
            else
            {
                fold_diacritics = PG_GETARG_BOOL(5);
            }

            if (stopwords_isnull && stopword_len > 0)
            {
                resolved_stopwords_array = psql_bm25s_am_text_array_from_cstrings(
                    stopwords,
                    stopword_len
                );
                resolved_stopwords_isnull = false;
                owns_stopwords_array = true;
            }
            psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
        }
        else
        {
            lowercase = lowercase_isnull ? false : PG_GETARG_BOOL(2);
            stem_english = stem_english_isnull ? false : PG_GETARG_BOOL(4);
            fold_diacritics = fold_diacritics_isnull ? false : PG_GETARG_BOOL(5);
        }

        index_close(indexRelation, AccessShareLock);
    }
    else
    {
        lowercase = lowercase_isnull ? false : PG_GETARG_BOOL(2);
        stem_english = stem_english_isnull ? false : PG_GETARG_BOOL(4);
        fold_diacritics = fold_diacritics_isnull ? false : PG_GETARG_BOOL(5);
    }

    if (!query_isnull)
    {
        query_text = PG_GETARG_TEXT_PP(1);
    }

    {
        Datum record = psql_bm25s_am_make_prepared_query_record(
            fcinfo,
            index_isnull,
            index_oid,
            query_isnull,
            query_text,
            lowercase,
            resolved_stopwords_isnull,
            resolved_stopwords_array,
            stem_english,
            fold_diacritics
        );

        if (owns_stopwords_array)
        {
            pfree(resolved_stopwords_array);
        }

        PG_RETURN_DATUM(record);
    }
}

static void
psql_bm25s_am_parse_scan_query_key(
    Relation indexRelation,
    ScanKey key,
    psql_bm25s_query *query_out
)
{
    Oid source_type;
    bool is_prepared;

    source_type = psql_bm25s_am_source_type(indexRelation);
    is_prepared =
        OidIsValid(key->sk_subtype) && key->sk_subtype != TEXTOID;
    if (!is_prepared)
    {
        text *query_text = DatumGetTextPP(key->sk_argument);

        if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            psql_bm25s_text_options options;
            char **stopwords = NULL;
            size_t stopword_len = 0;

            psql_bm25s_am_read_index_text_options(
                indexRelation,
                &options,
                &stopwords,
                &stopword_len
            );
            psql_bm25s_am_parse_raw_query_with_options(
                query_text,
                &options,
                query_out
            );
            psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
            return;
        }

        psql_bm25s_am_parse_raw_query(query_text, query_out);
        return;
    }

    psql_bm25s_am_parse_prepared_query_datum(
        key->sk_argument,
        RelationGetRelid(indexRelation),
        query_out
    );
}

static void
psql_bm25s_am_scan_init_query_match(
    IndexScanDesc scan,
    psql_bm25s_am_scan_opaque *opaque
)
{
    Oid source_type;
    Snapshot snapshot;
    psql_bm25s_query query;
    psql_bm25s_am_query_plan plan;
    AttrNumber attnum = InvalidAttrNumber;
    uint32_t *filter_query_ids = NULL;
    size_t filter_query_len = 0;
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    bool retain_query = false;

    if (scan->numberOfKeys != 1 || scan->keyData == NULL)
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s @@ scans require exactly one indexable clause")
            )
        );
    }
    if (scan->numberOfOrderBys != 0)
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s does not yet support combining @@ with ordered scans")
            )
        );
    }
    if ((scan->keyData[0].sk_flags & SK_ISNULL) != 0)
    {
        return;
    }

    source_type = psql_bm25s_am_source_type(scan->indexRelation);
    if (!psql_bm25s_am_source_type_is_textlike(source_type))
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s @@ scans require a text[], varchar[], text, or varchar index")
            )
        );
    }

    opaque->entry = psql_bm25s_am_get_cached_index(scan->indexRelation, source_type);
    opaque->heap_relation = scan->heapRelation;
    opaque->heap_relation_owned = false;
    if (opaque->heap_relation == NULL)
    {
        opaque->heap_relation = table_open(
            scan->indexRelation->rd_index->indrelid,
            AccessShareLock
        );
        opaque->heap_relation_owned = true;
    }

    snapshot = scan->xs_snapshot != NULL ? scan->xs_snapshot : GetActiveSnapshot();
    psql_bm25s_am_visibility_begin_with_snapshot(
        opaque->heap_relation,
        snapshot,
        &opaque->visibility
    );

    psql_bm25s_am_query_plan_init(&plan);
    psql_bm25s_am_parse_scan_query_key(
        scan->indexRelation,
        &scan->keyData[0],
        &query
    );
    if (query.len == 0)
    {
        psql_bm25s_query_free(&query);
        return;
    }

    if (psql_bm25s_query_uses_boolean_ast(&query))
    {
        bool used_positive_ast_plan = false;
        psql_bm25s_status status;

        status = psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
            opaque->entry,
            &query,
            query.root,
            &candidate_ids,
            &candidate_len,
            &used_positive_ast_plan
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(ERROR, (errmsg("failed to plan boolean @@ query: %s",
                                   psql_bm25s_strerror(status))));
        }

        if (!used_positive_ast_plan)
        {
            if (psql_bm25s_am_collect_positive_query_ids(
                    opaque->entry,
                    &query,
                    &filter_query_ids,
                    &filter_query_len) != PSQL_BM25S_OK)
            {
                psql_bm25s_query_free(&query);
                psql_bm25s_am_query_plan_free(&plan);
                ereport(ERROR, (errmsg("failed to plan boolean @@ query")));
            }
            psql_bm25s_am_collect_candidate_ids_from_query_ids(
                opaque->entry,
                filter_query_ids,
                filter_query_len,
                true,
                &candidate_ids,
                &candidate_len
            );
        }
    }
    else
    {
        if (psql_bm25s_am_build_query_plan(opaque->entry, &query, &plan) !=
            PSQL_BM25S_OK)
        {
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(ERROR, (errmsg("failed to plan @@ query")));
        }

        if (psql_bm25s_query_has_phrase(&query))
        {
            psql_bm25s_am_collect_phrase_query_candidate_ids(
                opaque->entry,
                &query,
                &candidate_ids,
                &candidate_len
            );
        }
        else
        {
            psql_bm25s_am_collect_query_candidate_ids(
                opaque->entry,
                &plan,
                &candidate_ids,
                &candidate_len
            );
        }
    }

    if (candidate_len > 0 &&
        (psql_bm25s_query_uses_boolean_ast(&query) ||
         psql_bm25s_query_has_phrase(&query)))
    {
        attnum = scan->indexRelation->rd_index->indkey.values[0];
        if (attnum <= 0)
        {
            free(filter_query_ids);
            free(candidate_ids);
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(
                ERROR,
                (
                    errmsg(
                        "grouped and phrase @@ scans require a simple indexed text[], varchar[], text, or varchar column"
                    )
                )
            );
        }

    }

    psql_bm25s_am_init_ranked_from_doc_ids(
        candidate_ids,
        candidate_len,
        &opaque->ranked
    );
    if (candidate_len > 0 && attnum > 0)
    {
        opaque->verify_query_active = true;
        opaque->verify_attnum = attnum;
        opaque->verify_source_type = source_type;
        if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            psql_bm25s_am_read_index_text_options(
                scan->indexRelation,
                &opaque->verify_text_options,
                &opaque->verify_stopwords,
                &opaque->verify_stopword_len
            );
        }
        else
        {
            psql_bm25s_text_options_init(&opaque->verify_text_options);
        }
        opaque->verify_query = query;
        retain_query = true;
    }

    free(filter_query_ids);
    free(candidate_ids);
    if (!retain_query)
    {
        psql_bm25s_query_free(&query);
    }
    psql_bm25s_am_query_plan_free(&plan);
}

static int
psql_bm25s_am_cmp_uint32_asc(const void *lhs, const void *rhs)
{
    const uint32_t *left = lhs;
    const uint32_t *right = rhs;

    if (*left < *right)
    {
        return -1;
    }
    if (*left > *right)
    {
        return 1;
    }
    return 0;
}

static void
psql_bm25s_am_init_ranked_storage(
    size_t candidate_len,
    psql_bm25s_topk_result *ranked_out
)
{
    size_t bytes;

    memset(ranked_out, 0, sizeof(*ranked_out));
    if (candidate_len == 0)
    {
        return;
    }

    if (!psql_bm25s_am_checked_mul_size(
            candidate_len,
            sizeof(*ranked_out->doc_ids),
            &bytes))
    {
        psql_bm25s_am_oom();
    }
    ranked_out->doc_ids = malloc(bytes);
    if (ranked_out->doc_ids == NULL)
    {
        psql_bm25s_am_oom();
    }

    if (!psql_bm25s_am_checked_mul_size(
            candidate_len,
            sizeof(*ranked_out->scores),
            &bytes))
    {
        psql_bm25s_topk_result_free(ranked_out);
        psql_bm25s_am_oom();
    }
    ranked_out->scores = calloc(candidate_len, sizeof(*ranked_out->scores));
    if (ranked_out->scores == NULL)
    {
        psql_bm25s_topk_result_free(ranked_out);
        psql_bm25s_am_oom();
    }

    ranked_out->len = candidate_len;
}

static void
psql_bm25s_am_init_zero_score_ranked(
    const uint32_t *candidate_ids,
    size_t candidate_len,
    psql_bm25s_topk_result *ranked_out
)
{
    size_t bytes;

    psql_bm25s_am_init_ranked_storage(candidate_len, ranked_out);
    if (candidate_len == 0)
    {
        return;
    }

    bytes = sizeof(*ranked_out->doc_ids) * candidate_len;
    memcpy(ranked_out->doc_ids, candidate_ids, bytes);
    qsort(
        ranked_out->doc_ids,
        candidate_len,
        sizeof(*ranked_out->doc_ids),
        psql_bm25s_am_cmp_uint32_asc
    );
}

static void
psql_bm25s_am_rank_candidate_ids(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    psql_bm25s_topk_result *ranked_out
)
{
    if (candidate_len == 0)
    {
        memset(ranked_out, 0, sizeof(*ranked_out));
        return;
    }

    if (query_len == 0)
    {
        psql_bm25s_am_init_zero_score_ranked(
            candidate_ids,
            candidate_len,
            ranked_out
        );
        return;
    }

    if (psql_bm25s_am_can_use_unsigned_sparse_path(entry, NULL))
    {
        psql_bm25s_am_rank_unsigned_sparse_candidates(
            entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            NULL,
            candidate_len,
            false,
            ranked_out
        );
    }
    else
    {
        psql_bm25s_am_rank_signed_sparse_candidates(
            entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            NULL,
            candidate_len,
            false,
            ranked_out
        );
    }
}

static void
psql_bm25s_am_rank_candidate_ids_prefix(
    psql_bm25s_am_cache_entry *entry,
    const uint32_t *candidate_ids,
    size_t candidate_len,
    const uint32_t *query_ids,
    size_t query_len,
    size_t limit,
    psql_bm25s_topk_result *ranked_out
)
{
    size_t wanted = Min(limit, candidate_len);

    if (wanted == 0)
    {
        memset(ranked_out, 0, sizeof(*ranked_out));
        return;
    }
    if (query_len == 0)
    {
        psql_bm25s_am_init_ranked_from_doc_ids(candidate_ids, wanted, ranked_out);
        return;
    }

    if (psql_bm25s_am_can_use_unsigned_sparse_path(entry, NULL))
    {
        psql_bm25s_am_rank_unsigned_sparse_candidates(
            entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            NULL,
            wanted,
            false,
            ranked_out
        );
    }
    else
    {
        psql_bm25s_am_rank_signed_sparse_candidates(
            entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            NULL,
            wanted,
            false,
            ranked_out
        );
    }
}

static bool
psql_bm25s_am_scan_expand_deferred_ordered_ranking(
    psql_bm25s_am_scan_opaque *opaque
)
{
    psql_bm25s_topk_result expanded = {0};
    size_t prev_len;
    size_t next_limit;

    if (opaque == NULL || !opaque->deferred_rank_active)
    {
        return false;
    }
    if (opaque->deferred_rank_limit >= opaque->deferred_candidate_len)
    {
        return false;
    }

    prev_len = opaque->ranked.len;
    next_limit = opaque->deferred_rank_limit + opaque->deferred_rank_step;
    if (next_limit < opaque->deferred_rank_limit ||
        next_limit > opaque->deferred_candidate_len)
    {
        next_limit = opaque->deferred_candidate_len;
    }

    psql_bm25s_am_rank_candidate_ids_prefix(
        opaque->entry,
        opaque->deferred_candidate_ids,
        opaque->deferred_candidate_len,
        opaque->deferred_query_ids,
        opaque->deferred_query_len,
        next_limit,
        &expanded
    );

    psql_bm25s_topk_result_free(&opaque->ranked);
    opaque->ranked = expanded;
    opaque->deferred_rank_limit = next_limit;
    opaque->ranked_pos = Min(prev_len, opaque->ranked.len);
    return opaque->ranked.len > prev_len;
}

static void
psql_bm25s_am_scan_init_query_match_ordered(
    IndexScanDesc scan,
    psql_bm25s_am_scan_opaque *opaque
)
{
    const size_t deferred_rank_min_candidates = 4096;
    const size_t deferred_rank_step = 256;
    Oid source_type;
    Snapshot snapshot;
    ArrayType *query_array;
    psql_bm25s_query query;
    psql_bm25s_am_query_plan plan;
    uint32_t *filter_query_ids = NULL;
    size_t filter_query_len = 0;
    uint32_t *candidate_ids = NULL;
    size_t candidate_len = 0;
    char **query_tokens = NULL;
    uint32_t *query_ids = NULL;
    size_t query_len = 0;
    size_t token_query_len = 0;
    AttrNumber attnum = InvalidAttrNumber;
    size_t i;
    psql_bm25s_status status;
    bool retain_query = false;

    if (scan->numberOfKeys != 1 || scan->keyData == NULL)
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s filtered ordered scans require exactly one @@ clause")
            )
        );
    }
    if (scan->numberOfOrderBys != 1 || scan->orderByData == NULL)
    {
        ereport(
            ERROR,
            (
                errmsg("psql_bm25s filtered ordered scans require exactly one ORDER BY clause")
            )
        );
    }
    if ((scan->orderByData[0].sk_flags & SK_ISNULL) != 0)
    {
        ereport(ERROR, (errmsg("psql_bm25s ORDER BY query cannot be NULL")));
    }
    if ((scan->keyData[0].sk_flags & SK_ISNULL) != 0)
    {
        return;
    }

    source_type = psql_bm25s_am_source_type(scan->indexRelation);
    if (!psql_bm25s_am_source_type_is_textlike(source_type))
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "psql_bm25s filtered ordered scans require a text[], varchar[], text, or varchar index"
                )
            )
        );
    }

    opaque->entry = psql_bm25s_am_get_cached_index(scan->indexRelation, source_type);
    opaque->heap_relation = scan->heapRelation;
    opaque->heap_relation_owned = false;
    if (opaque->heap_relation == NULL)
    {
        opaque->heap_relation = table_open(
            scan->indexRelation->rd_index->indrelid,
            AccessShareLock
        );
        opaque->heap_relation_owned = true;
    }

    snapshot = scan->xs_snapshot != NULL ? scan->xs_snapshot : GetActiveSnapshot();
    psql_bm25s_am_visibility_begin_with_snapshot(
        opaque->heap_relation,
        snapshot,
        &opaque->visibility
    );

    psql_bm25s_am_query_plan_init(&plan);
    psql_bm25s_am_parse_scan_query_key(
        scan->indexRelation,
        &scan->keyData[0],
        &query
    );
    if (query.len == 0)
    {
        psql_bm25s_query_free(&query);
        return;
    }

    if (psql_bm25s_query_uses_boolean_ast(&query))
    {
        bool used_positive_ast_plan = false;

        status = psql_bm25s_am_collect_positive_boolean_ast_candidate_ids(
            opaque->entry,
            &query,
            query.root,
            &candidate_ids,
            &candidate_len,
            &used_positive_ast_plan
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(ERROR, (errmsg("failed to plan grouped ordered scan: %s",
                                   psql_bm25s_strerror(status))));
        }

        if (!used_positive_ast_plan)
        {
            status = psql_bm25s_am_collect_positive_query_ids(
                opaque->entry,
                &query,
                &filter_query_ids,
                &filter_query_len
            );
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_query_free(&query);
                psql_bm25s_am_query_plan_free(&plan);
                ereport(
                    ERROR,
                    (errmsg("failed to plan grouped ordered scan: %s",
                            psql_bm25s_strerror(status)))
                );
            }
            psql_bm25s_am_collect_candidate_ids_from_query_ids(
                opaque->entry,
                filter_query_ids,
                filter_query_len,
                true,
                &candidate_ids,
                &candidate_len
            );
        }
    }
    else
    {
        status = psql_bm25s_am_build_query_plan(opaque->entry, &query, &plan);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(ERROR, (errmsg("failed to plan filtered ordered scan: %s",
                                   psql_bm25s_strerror(status))));
        }

        if (psql_bm25s_query_has_phrase(&query))
        {
            psql_bm25s_am_collect_phrase_query_candidate_ids(
                opaque->entry,
                &query,
                &candidate_ids,
                &candidate_len
            );
        }
        else
        {
            psql_bm25s_am_collect_query_candidate_ids(
                opaque->entry,
                &plan,
                &candidate_ids,
                &candidate_len
            );
        }
    }

    if (candidate_len > 0 &&
        (psql_bm25s_query_uses_boolean_ast(&query) ||
         psql_bm25s_query_has_phrase(&query)))
    {
        attnum = scan->indexRelation->rd_index->indkey.values[0];
        if (attnum <= 0)
        {
            free(filter_query_ids);
            free(candidate_ids);
            psql_bm25s_query_free(&query);
            psql_bm25s_am_query_plan_free(&plan);
            ereport(
                ERROR,
                (
                    errmsg(
                        "grouped and phrase @@ scans require a simple indexed text[], varchar[], text, or varchar column"
                    )
                )
            );
        }

    }

    query_array = DatumGetArrayTypeP(scan->orderByData[0].sk_argument);
    query_tokens = psql_bm25s_array_read_query_tokens(
        query_array,
        &query_len
    );
    token_query_len = query_len;
    status = psql_bm25s_am_query_token_ids_cached(
        opaque->entry,
        query_tokens,
        query_len,
        &query_ids,
        &query_len
    );
    if (status != PSQL_BM25S_OK)
    {
        free(candidate_ids);
        psql_bm25s_query_free(&query);
        psql_bm25s_am_query_plan_free(&plan);
        ereport(ERROR, (errmsg("failed to map ordered query tokens: %s",
                               psql_bm25s_strerror(status))));
    }

    if (attnum > 0 &&
        candidate_len >= deferred_rank_min_candidates &&
        candidate_len > deferred_rank_step)
    {
        psql_bm25s_am_rank_candidate_ids_prefix(
            opaque->entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            deferred_rank_step,
            &opaque->ranked
        );
        opaque->deferred_candidate_ids = candidate_ids;
        candidate_ids = NULL;
        opaque->deferred_query_ids = query_ids;
        query_ids = NULL;
        opaque->deferred_candidate_len = candidate_len;
        opaque->deferred_query_len = query_len;
        opaque->deferred_rank_limit = opaque->ranked.len;
        opaque->deferred_rank_step = deferred_rank_step;
        opaque->deferred_rank_active = true;
    }
    else
    {
        psql_bm25s_am_rank_candidate_ids(
            opaque->entry,
            candidate_ids,
            candidate_len,
            query_ids,
            query_len,
            &opaque->ranked
        );
    }
    if (candidate_len > 0 && attnum > 0)
    {
        opaque->verify_query_active = true;
        opaque->verify_attnum = attnum;
        opaque->verify_source_type = source_type;
        if (psql_bm25s_am_source_type_is_scalar_text(source_type))
        {
            psql_bm25s_am_read_index_text_options(
                scan->indexRelation,
                &opaque->verify_text_options,
                &opaque->verify_stopwords,
                &opaque->verify_stopword_len
            );
        }
        else
        {
            psql_bm25s_text_options_init(&opaque->verify_text_options);
        }
        opaque->verify_query = query;
        retain_query = true;
    }

    for (i = 0; i < token_query_len; i++)
    {
        if (query_tokens[i] != NULL)
        {
            pfree(query_tokens[i]);
        }
    }
    pfree(query_tokens);
    free(filter_query_ids);
    free(query_ids);
    free(candidate_ids);
    if (!retain_query)
    {
        psql_bm25s_query_free(&query);
    }
    psql_bm25s_am_query_plan_free(&plan);
}

PG_FUNCTION_INFO_V1(psql_bm25s_score_ids_op);
Datum
psql_bm25s_score_ids_op(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *query_array = PG_GETARG_ARRAYTYPE_P(1);

    PG_RETURN_DATUM(psql_bm25s_am_score_overlap_int4(doc_array, query_array));
}

PG_FUNCTION_INFO_V1(psql_bm25s_score_tokens_op);
Datum
psql_bm25s_score_tokens_op(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *query_array = PG_GETARG_ARRAYTYPE_P(1);

    PG_RETURN_DATUM(psql_bm25s_am_score_overlap_text(doc_array, query_array));
}

PG_FUNCTION_INFO_V1(psql_bm25s_match_query_tokens_op);
Datum
psql_bm25s_match_query_tokens_op(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    text *query_text = PG_GETARG_TEXT_PP(1);
    char **doc_tokens;
    size_t doc_len = 0;
    psql_bm25s_query query;
    bool matched;

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    psql_bm25s_am_parse_raw_query(query_text, &query);
    matched = psql_bm25s_am_doc_matches_query(
        doc_tokens,
        doc_len,
        &query,
        NULL
    );

    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_BOOL(matched);
}

PG_FUNCTION_INFO_V1(psql_bm25s_match_prepared_query_op);
Datum
psql_bm25s_match_prepared_query_op(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    Datum prepared_datum = PG_GETARG_DATUM(1);
    char **doc_tokens;
    size_t doc_len = 0;
    psql_bm25s_query query;
    bool matched;

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    psql_bm25s_am_parse_prepared_query_datum(
        prepared_datum,
        InvalidOid,
        &query
    );
    matched = psql_bm25s_am_doc_matches_query(
        doc_tokens,
        doc_len,
        &query,
        NULL
    );

    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_BOOL(matched);
}

PG_FUNCTION_INFO_V1(psql_bm25s_highlight_tokens);
Datum
psql_bm25s_highlight_tokens(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    text *query_text = PG_GETARG_TEXT_PP(1);
    text *start_tag_text = PG_GETARG_TEXT_PP(2);
    text *end_tag_text = PG_GETARG_TEXT_PP(3);
    char **doc_tokens;
    size_t doc_len = 0;
    bool *marks;
    char *start_tag;
    char *end_tag;
    psql_bm25s_query query;
    text *result;

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    marks = palloc0(sizeof(*marks) * Max(doc_len, 1));
    start_tag = text_to_cstring(start_tag_text);
    end_tag = text_to_cstring(end_tag_text);
    psql_bm25s_am_parse_raw_query(query_text, &query);
    psql_bm25s_am_mark_query_matches(
        doc_tokens,
        doc_len,
        &query,
        marks
    );
    result = psql_bm25s_am_render_highlighted_tokens(
        doc_tokens,
        doc_len,
        marks,
        0,
        doc_len,
        start_tag,
        end_tag
    );

    pfree(start_tag);
    pfree(end_tag);
    pfree(marks);
    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_highlight_tokens_cfg);
Datum
psql_bm25s_highlight_tokens_cfg(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array;
    text *query_text;
    text *start_tag_text;
    text *end_tag_text;
    bool lowercase;
    bool stem_english;
    bool fold_diacritics;
    ArrayType *stopwords_array;
    char **doc_tokens;
    size_t doc_len = 0;
    bool *marks;
    char *start_tag;
    char *end_tag;
    psql_bm25s_query query;
    text *result;

    if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) ||
        PG_ARGISNULL(3) || PG_ARGISNULL(4))
    {
        PG_RETURN_NULL();
    }

    doc_array = PG_GETARG_ARRAYTYPE_P(0);
    query_text = PG_GETARG_TEXT_PP(1);
    start_tag_text = PG_GETARG_TEXT_PP(2);
    end_tag_text = PG_GETARG_TEXT_PP(3);
    lowercase = PG_GETARG_BOOL(4);
    stopwords_array = PG_ARGISNULL(5) ? NULL : PG_GETARG_ARRAYTYPE_P(5);
    stem_english = PG_ARGISNULL(6) ? false : PG_GETARG_BOOL(6);
    fold_diacritics = PG_ARGISNULL(7) ? false : PG_GETARG_BOOL(7);

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    marks = palloc0(sizeof(*marks) * Max(doc_len, 1));
    start_tag = text_to_cstring(start_tag_text);
    end_tag = text_to_cstring(end_tag_text);
    psql_bm25s_am_parse_raw_query_pg_options(
        query_text,
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        &query
    );
    psql_bm25s_am_mark_query_matches(doc_tokens, doc_len, &query, marks);
    result = psql_bm25s_am_render_highlighted_tokens(
        doc_tokens,
        doc_len,
        marks,
        0,
        doc_len,
        start_tag,
        end_tag
    );

    pfree(start_tag);
    pfree(end_tag);
    pfree(marks);
    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_snippet_tokens);
Datum
psql_bm25s_snippet_tokens(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array = PG_GETARG_ARRAYTYPE_P(0);
    text *query_text = PG_GETARG_TEXT_PP(1);
    int32 max_tokens = PG_GETARG_INT32(2);
    text *start_tag_text = PG_GETARG_TEXT_PP(3);
    text *end_tag_text = PG_GETARG_TEXT_PP(4);
    char **doc_tokens;
    size_t doc_len = 0;
    bool *marks;
    char *start_tag;
    char *end_tag;
    psql_bm25s_query query;
    size_t start = 0;
    size_t end = 0;
    text *result;

    if (max_tokens <= 0)
    {
        ereport(ERROR, (errmsg("max_tokens must be positive")));
    }

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    marks = palloc0(sizeof(*marks) * Max(doc_len, 1));
    start_tag = text_to_cstring(start_tag_text);
    end_tag = text_to_cstring(end_tag_text);
    psql_bm25s_am_parse_raw_query(query_text, &query);
    psql_bm25s_am_mark_query_matches(
        doc_tokens,
        doc_len,
        &query,
        marks
    );
    psql_bm25s_am_choose_snippet_window(
        marks,
        doc_len,
        (size_t) max_tokens,
        &start,
        &end
    );
    result = psql_bm25s_am_render_highlighted_tokens(
        doc_tokens,
        doc_len,
        marks,
        start,
        end,
        start_tag,
        end_tag
    );

    pfree(start_tag);
    pfree(end_tag);
    pfree(marks);
    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_snippet_tokens_cfg);
Datum
psql_bm25s_snippet_tokens_cfg(PG_FUNCTION_ARGS)
{
    ArrayType *doc_array;
    text *query_text;
    int32 max_tokens;
    text *start_tag_text;
    text *end_tag_text;
    bool lowercase;
    bool stem_english;
    bool fold_diacritics;
    ArrayType *stopwords_array;
    char **doc_tokens;
    size_t doc_len = 0;
    bool *marks;
    char *start_tag;
    char *end_tag;
    psql_bm25s_query query;
    size_t start = 0;
    size_t end = 0;
    text *result;

    if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) ||
        PG_ARGISNULL(3) || PG_ARGISNULL(4) || PG_ARGISNULL(5))
    {
        PG_RETURN_NULL();
    }

    doc_array = PG_GETARG_ARRAYTYPE_P(0);
    query_text = PG_GETARG_TEXT_PP(1);
    max_tokens = PG_GETARG_INT32(2);
    start_tag_text = PG_GETARG_TEXT_PP(3);
    end_tag_text = PG_GETARG_TEXT_PP(4);
    lowercase = PG_GETARG_BOOL(5);
    stopwords_array = PG_ARGISNULL(6) ? NULL : PG_GETARG_ARRAYTYPE_P(6);
    stem_english = PG_ARGISNULL(7) ? false : PG_GETARG_BOOL(7);
    fold_diacritics = PG_ARGISNULL(8) ? false : PG_GETARG_BOOL(8);

    if (max_tokens <= 0)
    {
        ereport(ERROR, (errmsg("max_tokens must be positive")));
    }

    doc_tokens = psql_bm25s_array_read_doc_tokens(doc_array, &doc_len);
    marks = palloc0(sizeof(*marks) * Max(doc_len, 1));
    start_tag = text_to_cstring(start_tag_text);
    end_tag = text_to_cstring(end_tag_text);
    psql_bm25s_am_parse_raw_query_pg_options(
        query_text,
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        &query
    );
    psql_bm25s_am_mark_query_matches(doc_tokens, doc_len, &query, marks);
    psql_bm25s_am_choose_snippet_window(
        marks,
        doc_len,
        (size_t) max_tokens,
        &start,
        &end
    );
    result = psql_bm25s_am_render_highlighted_tokens(
        doc_tokens,
        doc_len,
        marks,
        start,
        end,
        start_tag,
        end_tag
    );

    pfree(start_tag);
    pfree(end_tag);
    pfree(marks);
    psql_bm25s_am_free_query_tokens(doc_tokens, doc_len);
    psql_bm25s_query_free(&query);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_normalize_tokens_sql);
Datum
psql_bm25s_normalize_tokens_sql(PG_FUNCTION_ARGS)
{
    ArrayType *token_array;
    bool lowercase;
    ArrayType *stopwords_array;
    bool stem_english;
    bool fold_diacritics;
    char **tokens;
    char **stopwords = NULL;
    char **normalized_tokens;
    size_t token_len = 0;
    size_t stopword_len = 0;
    size_t normalized_len = 0;
    size_t i;
    psql_bm25s_text_options options;
    ArrayType *result;

    if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
    {
        PG_RETURN_NULL();
    }

    token_array = PG_GETARG_ARRAYTYPE_P(0);
    lowercase = PG_GETARG_BOOL(1);
    stopwords_array = PG_ARGISNULL(2) ? NULL : PG_GETARG_ARRAYTYPE_P(2);
    stem_english = PG_ARGISNULL(3) ? false : PG_GETARG_BOOL(3);
    fold_diacritics = PG_ARGISNULL(4) ? false : PG_GETARG_BOOL(4);

    tokens = psql_bm25s_array_read_doc_tokens(token_array, &token_len);
    psql_bm25s_am_read_text_options(
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        &options,
        &stopwords,
        &stopword_len
    );

    normalized_tokens = palloc(sizeof(*normalized_tokens) * Max(token_len, 1));
    for (i = 0; i < token_len; i++)
    {
        char *normalized = NULL;
        bool keep = false;
        psql_bm25s_status status;

        status = psql_bm25s_normalize_token(
            tokens[i],
            &options,
            &normalized,
            &keep
        );
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_am_free_query_tokens(tokens, token_len);
            psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
            pfree(normalized_tokens);
            ereport(ERROR, (errmsg("failed to normalize tokens: %s",
                                   psql_bm25s_strerror(status))));
        }
        if (keep)
        {
            normalized_tokens[normalized_len++] = normalized;
        }
    }

    result = psql_bm25s_am_text_array_from_cstrings(
        normalized_tokens,
        normalized_len
    );

    for (i = 0; i < normalized_len; i++)
    {
        free(normalized_tokens[i]);
    }
    pfree(normalized_tokens);
    psql_bm25s_am_free_query_tokens(tokens, token_len);
    psql_bm25s_am_free_query_tokens(stopwords, stopword_len);

    PG_RETURN_ARRAYTYPE_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_tokenize_text_sql);
Datum
psql_bm25s_tokenize_text_sql(PG_FUNCTION_ARGS)
{
    text *input;
    bool lowercase;
    ArrayType *stopwords_array;
    bool stem_english;
    bool fold_diacritics;
    char **tokens = NULL;
    char **stopwords = NULL;
    size_t token_len = 0;
    size_t stopword_len = 0;
    psql_bm25s_text_options options;
    psql_bm25s_status status;
    ArrayType *result;
    char *input_cstr;

    if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
    {
        PG_RETURN_NULL();
    }

    input = PG_GETARG_TEXT_PP(0);
    lowercase = PG_GETARG_BOOL(1);
    stopwords_array = PG_ARGISNULL(2) ? NULL : PG_GETARG_ARRAYTYPE_P(2);
    stem_english = PG_ARGISNULL(3) ? false : PG_GETARG_BOOL(3);
    fold_diacritics = PG_ARGISNULL(4) ? false : PG_GETARG_BOOL(4);

    psql_bm25s_am_read_text_options(
        lowercase,
        stem_english,
        fold_diacritics,
        stopwords_array,
        &options,
        &stopwords,
        &stopword_len
    );

    input_cstr = text_to_cstring(input);
    status = psql_bm25s_tokenize_text(
        input_cstr,
        &options,
        &tokens,
        &token_len
    );
    pfree(input_cstr);
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
        ereport(ERROR, (errmsg("failed to tokenize text: %s",
                               psql_bm25s_strerror(status))));
    }

    result = psql_bm25s_am_text_array_from_cstrings(tokens, token_len);
    psql_bm25s_text_tokens_free(tokens, token_len);
    psql_bm25s_am_free_query_tokens(stopwords, stopword_len);

    PG_RETURN_ARRAYTYPE_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_generation_cache_clear);
Datum
psql_bm25s_generation_cache_clear(PG_FUNCTION_ARGS)
{
    char dir_path[MAXPGPATH];
    DIR *dir;
    struct dirent *de;
    int cleared = 0;
    int written;

    psql_bm25s_am_cache_reset_all();
    if (psql_bm25s_am_shared_preload_available())
    {
        uint32 i;

        SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
        for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
        {
            if (psql_bm25s_shared_preload->entries[i].in_use)
            {
                psql_bm25s_am_shared_preload_retire_entry_locked(
                    &psql_bm25s_shared_preload->entries[i]
                );
                cleared++;
            }
        }
        psql_bm25s_am_shared_preload_try_rewind_locked();
        SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    }

    written = snprintf(
        dir_path,
        sizeof(dir_path),
        "%s/psql_bm25s_generation_cache",
        DataDir
    );
    if (written <= 0 || (size_t) written >= sizeof(dir_path))
    {
        ereport(ERROR, (errmsg("psql_bm25s generation cache path is too long")));
    }

    dir = opendir(dir_path);
    if (dir == NULL)
    {
        if (errno == ENOENT)
        {
            PG_RETURN_INT32(cleared);
        }
        ereport(
            ERROR,
            (
                errcode_for_file_access(),
                errmsg("could not open psql_bm25s generation cache directory")
            )
        );
    }

    while ((de = readdir(dir)) != NULL)
    {
        psql_bm25s_am_generation_descriptor descriptor;
        char path[MAXPGPATH];
        size_t name_len = strlen(de->d_name);
        bool descriptor_file;
        bool tmp_descriptor_file;
        bool lock_file;
        bool failure_file;

        descriptor_file =
            name_len >= 6 && strcmp(de->d_name + name_len - 5, ".desc") == 0;
        tmp_descriptor_file =
            name_len >= 10 &&
            strcmp(de->d_name + name_len - 4, ".tmp") == 0 &&
            strstr(de->d_name, ".desc.") != NULL;
        lock_file =
            name_len >= 6 && strcmp(de->d_name + name_len - 5, ".lock") == 0;
        failure_file =
            name_len >= 6 && strcmp(de->d_name + name_len - 5, ".fail") == 0;
        if (!descriptor_file &&
            !tmp_descriptor_file &&
            !lock_file &&
            !failure_file)
        {
            continue;
        }
        written = snprintf(path, sizeof(path), "%s/%s", dir_path, de->d_name);
        if (written <= 0 || (size_t) written >= sizeof(path))
        {
            continue;
        }
        if (tmp_descriptor_file || lock_file || failure_file)
        {
            if (unlink(path) == 0 || errno == ENOENT)
            {
                cleared++;
            }
            continue;
        }
        if (!psql_bm25s_am_generation_descriptor_read_file(
                path,
                &descriptor))
        {
            if (unlink(path) == 0 || errno == ENOENT)
            {
                cleared++;
            }
            continue;
        }
        PG_TRY();
        {
            dsm_unpin_segment(descriptor.handle);
        }
        PG_CATCH();
        {
            FlushErrorState();
        }
        PG_END_TRY();

        if (unlink(path) == 0 || errno == ENOENT)
        {
            cleared++;
        }
    }
    closedir(dir);

    PG_RETURN_INT32(cleared);
}

PG_FUNCTION_INFO_V1(psql_bm25s_generation_cache_state);
Datum
psql_bm25s_generation_cache_state(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    psql_bm25s_am_generation_descriptor descriptor;
    char path[MAXPGPATH];
    struct stat st;
    bool dsm_share_eligible;
    bool share_required;
    bool descriptor_present = false;
    bool descriptor_valid = false;
    bool shared_preload_available;
    Size shared_preload_arena_size = 0;
    Size shared_preload_used = 0;
    int shared_preload_entries = 0;
    int shared_preload_ready_entries = 0;
    int shared_preload_obsolete_entries = 0;
    int shared_preload_reusable_entries = 0;
    int shared_preload_refcounted_entries = 0;
    Size shared_preload_reusable_bytes = 0;
    uint32 active_background_workers = 0;
    uint32 active_preload_workers = 0;
    uint32 active_index_maintenance_workers = 0;
    uint32 active_maintenance_workers = 0;
    int64 maintenance_cycle_age_ms = -1;
    bool shared_preload_resident = false;
    bool shared_preload_loading = false;
    Size mapped_size = 0;
    uint64 standard_estimated_bytes = 0;
    uint64 compact_estimated_bytes = 0;
    uint64 spill_estimated_bytes = 0;
    uint64 rebuild_budget_bytes = 0;
    psql_bm25s_am_rebuild_builder admitted_builder =
        PSQL_BM25S_AM_REBUILD_BUILDER_STANDARD;
    bool rebuild_admitted;
    text *result;
    char *state;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_payload_health(indexRelation, &meta, &health);

    dsm_share_eligible = psql_bm25s_am_generation_dsm_share_eligible(&meta);
    share_required = psql_bm25s_am_generation_share_required(&meta);
    if (psql_bm25s_am_generation_descriptor_path(
            indexRelation,
            &meta,
            path,
            sizeof(path)))
    {
        descriptor_present = stat(path, &st) == 0;
        descriptor_valid = psql_bm25s_am_generation_descriptor_read(
            indexRelation,
            &meta,
            &descriptor
        );
        if (descriptor_valid)
        {
            mapped_size = descriptor.mapped_size;
        }
    }
    shared_preload_available = psql_bm25s_am_shared_preload_available();
    rebuild_admitted = psql_bm25s_am_rebuild_memory_budget_choose(
        &meta,
        &admitted_builder,
        &standard_estimated_bytes,
        &compact_estimated_bytes,
        &spill_estimated_bytes,
        &rebuild_budget_bytes
    );
    if (shared_preload_available)
    {
        int i;

        psql_bm25s_am_shared_preload_index_state(
            indexRelation,
            &meta,
            &shared_preload_resident,
            &shared_preload_loading
        );
        SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
        shared_preload_arena_size = psql_bm25s_shared_preload->arena_size;
        shared_preload_used = psql_bm25s_shared_preload->used;
        active_background_workers =
            psql_bm25s_shared_preload->active_maintenance_workers;
        active_preload_workers =
            psql_bm25s_shared_preload->active_preload_workers;
        active_index_maintenance_workers =
            psql_bm25s_shared_preload->active_index_maintenance_workers;
        active_maintenance_workers = active_background_workers;
        if (psql_bm25s_shared_preload->last_maintenance_cycle != 0)
        {
            long secs;
            int usecs;

            TimestampDifference(
                psql_bm25s_shared_preload->last_maintenance_cycle,
                GetCurrentTimestamp(),
                &secs,
                &usecs
            );
            maintenance_cycle_age_ms =
                ((int64) secs * 1000) + (usecs / 1000);
        }
        for (i = 0; i < psql_bm25s_am_shared_preload_entry_capacity(); i++)
        {
            if (psql_bm25s_shared_preload->entries[i].in_use)
            {
                shared_preload_entries++;
                if (psql_bm25s_shared_preload->entries[i].obsolete)
                {
                    shared_preload_obsolete_entries++;
                }
                if (psql_bm25s_shared_preload->entries[i].refcount > 0)
                {
                    shared_preload_refcounted_entries++;
                }
                if (psql_bm25s_shared_preload->entries[i].ready)
                {
                    shared_preload_ready_entries++;
                }
            }
            else if (psql_bm25s_am_shared_preload_entry_has_block(
                         &psql_bm25s_shared_preload->entries[i]))
            {
                shared_preload_reusable_entries++;
                shared_preload_reusable_bytes +=
                    psql_bm25s_shared_preload->entries[i].mapped_size;
            }
        }
        SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    }

    state = psprintf(
        "psql_bm25s_generation_cache_state("
        "cache_epoch=%llu, "
        "share_eligible=%s, "
        "share_required=%s, "
        "descriptor_present=%s, "
        "descriptor_valid=%s, "
        "mapped_size=%zu, "
        "shared_preload_configured_mb=%d, "
        "shared_preload_available=%s, "
        "shared_preload_arena_size=%zu, "
        "shared_preload_used=%zu, "
        "shared_preload_entries=%d, "
        "shared_preload_ready_entries=%d, "
        "shared_preload_obsolete_entries=%d, "
        "shared_preload_reusable_entries=%d, "
        "shared_preload_reusable_bytes=%zu, "
        "shared_preload_refcounted_entries=%d, "
        "active_background_workers=%u, "
        "active_preload_workers=%u, "
        "active_index_maintenance_workers=%u, "
        "active_maintenance_workers=%u, "
        "maintenance_cycle_age_ms=%lld, "
        "shared_preload_resident=%s, "
        "shared_preload_loading=%s, "
        "pending_writes=%u, "
        "pending_deletes=%u, "
        "delta_records=%u, "
        "delta_bytes=%llu, "
        "flags=0x%x, "
        "active_start_blkno=%u, "
        "active_data_pages=%u, "
        "delta_start_blkno=%u, "
        "relation_blocks=%u, "
        "payload_health=%s, "
        "payload_health_reason=%s, "
        "payload_expected_bytes=%llu, "
        "payload_capacity_bytes=%llu, "
        "rebuild_required=%s, "
        "rebuild_admitted=%s, "
        "rebuild_builder=%s, "
        "standard_estimated_bytes=%llu, "
        "compact_estimated_bytes=%llu, "
        "spill_estimated_bytes=%llu, "
        "rebuild_budget_bytes=%llu, "
        "index_bytes=%llu, "
        "docs=%u, "
        "locator=%u/%u/%u)",
        (unsigned long long) meta.cache_epoch,
        dsm_share_eligible ? "true" : "false",
        share_required ? "true" : "false",
        descriptor_present ? "true" : "false",
        descriptor_valid ? "true" : "false",
        (size_t) mapped_size,
        psql_bm25s_shared_generation_cache_size_mb,
        shared_preload_available ? "true" : "false",
        (size_t) shared_preload_arena_size,
        (size_t) shared_preload_used,
        shared_preload_entries,
        shared_preload_ready_entries,
        shared_preload_obsolete_entries,
        shared_preload_reusable_entries,
        (size_t) shared_preload_reusable_bytes,
        shared_preload_refcounted_entries,
        active_background_workers,
        active_preload_workers,
        active_index_maintenance_workers,
        active_maintenance_workers,
        (long long) maintenance_cycle_age_ms,
        shared_preload_resident ? "true" : "false",
        shared_preload_loading ? "true" : "false",
        meta.pending_write_tuples,
        meta.pending_delete_tuples,
        meta.delta_record_count,
        (unsigned long long) meta.delta_bytes_len,
        meta.flags,
        meta.active_start_blkno,
        meta.active_data_pages,
        meta.delta_start_blkno,
        psql_bm25s_am_relation_nblocks(indexRelation),
        health.status,
        health.reason,
        (unsigned long long) health.expected_bytes,
        (unsigned long long) health.capacity_bytes,
        health.rebuild_required ? "true" : "false",
        rebuild_admitted ? "true" : "false",
        admitted_builder == PSQL_BM25S_AM_REBUILD_BUILDER_COMPACT
            ? "compact"
            : admitted_builder == PSQL_BM25S_AM_REBUILD_BUILDER_SPILL
            ? "spill"
            : "standard",
        (unsigned long long) standard_estimated_bytes,
        (unsigned long long) compact_estimated_bytes,
        (unsigned long long) spill_estimated_bytes,
        (unsigned long long) rebuild_budget_bytes,
        (unsigned long long) meta.index_bytes_len,
        meta.num_docs,
        indexRelation->rd_locator.spcOid,
        indexRelation->rd_locator.dbOid,
        indexRelation->rd_locator.relNumber
    );
    result = cstring_to_text(state);
    pfree(state);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_generation_cache_preload);
Datum
psql_bm25s_generation_cache_preload(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_cache_entry *entry;
    const char *tier;
    text *result;
    char *state;
    bool old_preload_publisher_active;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_read_meta(indexRelation, &meta);
    old_preload_publisher_active =
        psql_bm25s_am_preload_publisher_active;
    PG_TRY();
    {
        psql_bm25s_am_preload_publisher_active = true;
        entry = psql_bm25s_am_get_cached_index(indexRelation, meta.source_type);
        psql_bm25s_am_preload_publisher_active =
            old_preload_publisher_active;
    }
    PG_CATCH();
    {
        psql_bm25s_am_preload_publisher_active =
            old_preload_publisher_active;
        index_close(indexRelation, AccessShareLock);
        PG_RE_THROW();
    }
    PG_END_TRY();
    tier = psql_bm25s_am_cache_entry_tier(entry);
    state = psprintf(
        "psql_bm25s_generation_cache_preload("
        "index_oid=%u, "
        "cache_epoch=%llu, "
        "tier=%s, "
        "docs=%u, "
        "index_bytes=%llu)",
        index_oid,
        (unsigned long long) entry->generation.meta.cache_epoch,
        tier,
        entry->generation.meta.num_docs,
        (unsigned long long) entry->generation.meta.index_bytes_len
    );
    result = cstring_to_text(state);
    pfree(state);
    psql_bm25s_am_cache_release_lease(entry);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_handler);
Datum
psql_bm25s_handler(PG_FUNCTION_ARGS)
{
    IndexAmRoutine *amroutine = makeNode(IndexAmRoutine);

    psql_bm25s_init_reloptions();
    amroutine->amstrategies = 1;
    amroutine->amsupport = 0;
    amroutine->amoptsprocnum = 0;
    amroutine->amcanorder = false;
    amroutine->amcanorderbyop = true;
    amroutine->amcanbackward = false;
    amroutine->amcanunique = false;
    amroutine->amcanmulticol = true;
    amroutine->amoptionalkey = true;
    amroutine->amsearcharray = false;
    amroutine->amsearchnulls = false;
    amroutine->amstorage = true;
    amroutine->amclusterable = false;
    amroutine->ampredlocks = false;
    amroutine->amcanparallel = false;
    amroutine->amcanbuildparallel = false;
    amroutine->amcaninclude = false;
    amroutine->amusemaintenanceworkmem = true;
    amroutine->amsummarizing = false;
    amroutine->amparallelvacuumoptions = 0;
    amroutine->amkeytype = InvalidOid;

    amroutine->ambuild = psql_bm25s_ambuild;
    amroutine->ambuildempty = psql_bm25s_ambuildempty;
    amroutine->aminsert = psql_bm25s_aminsert;
    amroutine->aminsertcleanup = psql_bm25s_aminsertcleanup;
    amroutine->ambulkdelete = psql_bm25s_ambulkdelete;
    amroutine->amvacuumcleanup = psql_bm25s_amvacuumcleanup;
    amroutine->amcanreturn = psql_bm25s_amcanreturn;
    amroutine->amcostestimate = psql_bm25s_amcostestimate;
    amroutine->amoptions = psql_bm25s_amoptions;
    amroutine->amproperty = NULL;
    amroutine->ambuildphasename = NULL;
    amroutine->amvalidate = psql_bm25s_amvalidate;
    amroutine->amadjustmembers = NULL;
    amroutine->ambeginscan = psql_bm25s_ambeginscan;
    amroutine->amrescan = psql_bm25s_amrescan;
    amroutine->amgettuple = psql_bm25s_amgettuple;
    amroutine->amgetbitmap = psql_bm25s_amgetbitmap;
    amroutine->amendscan = psql_bm25s_amendscan;
    amroutine->ammarkpos = NULL;
    amroutine->amrestrpos = NULL;
    amroutine->amestimateparallelscan = NULL;
    amroutine->aminitparallelscan = NULL;
    amroutine->amparallelrescan = NULL;

    PG_RETURN_POINTER(amroutine);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_describe);
Datum
psql_bm25s_index_describe(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    result = psql_bm25s_am_describe_relation(indexRelation);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_maintenance_state);
Datum
psql_bm25s_index_maintenance_state(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    result = psql_bm25s_am_describe_maintenance(indexRelation);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_maintenance_policy);
Datum
psql_bm25s_index_maintenance_policy(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    result = psql_bm25s_am_describe_maintenance_policy(indexRelation);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_maintenance_policy_details);
Datum
psql_bm25s_index_maintenance_policy_details(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    result = psql_bm25s_am_describe_maintenance_policy_details(indexRelation);
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_details);
Datum
psql_bm25s_index_details(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    TupleDesc tupdesc;
    HeapTuple tuple;
    Datum values[17];
    bool nulls[17] = {false};
    BlockNumber pages;
    int consistency;

    psql_bm25s_init_reloptions();
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
    {
        ereport(ERROR, (errmsg("could not resolve result record type")));
    }
    BlessTupleDesc(tupdesc);

    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_read_meta(indexRelation, &meta);
    pages = RelationGetNumberOfBlocks(indexRelation);
    consistency = psql_bm25s_am_get_consistency(indexRelation);

    values[0] = ObjectIdGetDatum(index_oid);
    values[1] = PointerGetDatum(cstring_to_text(format_type_be(meta.source_type)));
    values[2] = Int64GetDatum((int64) meta.num_docs);
    values[3] = Int64GetDatum((int64) meta.index_bytes_len);
    values[4] = Int64GetDatum((int64) pages);
    values[5] = BoolGetDatum((meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0);
    values[6] = PointerGetDatum(
        cstring_to_text(psql_bm25s_am_consistency_name(consistency))
    );
    values[7] = Int64GetDatum((int64) meta.rebuild_count);
    values[8] = Int64GetDatum((int64) meta.pending_write_tuples);
    values[9] = Int64GetDatum((int64) meta.pending_delete_tuples);
    values[10] = Int64GetDatum((int64) meta.delta_record_count);
    values[11] = Int64GetDatum((int64) meta.delta_bytes_len);

    if (consistency == PSQL_BM25S_AM_CONSISTENCY_MANUAL)
    {
        nulls[12] = true;
    }
    else
    {
        values[12] = Int32GetDatum(psql_bm25s_am_rebuild_threshold(indexRelation));
    }

    if (consistency == PSQL_BM25S_AM_CONSISTENCY_REALTIME)
    {
        values[13] = Int32GetDatum(
            psql_bm25s_am_rebuild_delta_bytes_threshold(indexRelation)
        );
        values[14] = Float8GetDatum(
            psql_bm25s_am_rebuild_churn_ratio_threshold(indexRelation)
        );
    }
    else
    {
        nulls[13] = true;
        nulls[14] = true;
    }

    if (consistency == PSQL_BM25S_AM_CONSISTENCY_EVENTUAL)
    {
        values[15] = Int32GetDatum(
            psql_bm25s_am_query_overlay_max_records(indexRelation)
        );
        values[16] = Int32GetDatum(
            psql_bm25s_am_query_overlay_max_bytes(indexRelation)
        );
    }
    else
    {
        nulls[15] = true;
        nulls[16] = true;
    }

    index_close(indexRelation, AccessShareLock);
    tuple = heap_form_tuple(tupdesc, values, nulls);
    PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}

PG_FUNCTION_INFO_V1(psql_bm25s_recommend_maintenance_policy);
Datum
psql_bm25s_recommend_maintenance_policy(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    text *profile_text;
    char *profile;
    Relation indexRelation;
    text *result;

    psql_bm25s_init_reloptions();
    profile_text = PG_ARGISNULL(1) ? cstring_to_text("balanced") :
        PG_GETARG_TEXT_PP(1);
    profile = text_to_cstring(profile_text);
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    result = psql_bm25s_am_recommend_maintenance_policy(
        indexRelation,
        profile
    );
    index_close(indexRelation, AccessShareLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_index_policy_recommend);
Datum
psql_bm25s_index_policy_recommend(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    text *profile_text;
    char *profile;
    Relation indexRelation;
    psql_bm25s_am_policy_recommendation recommendation;
    TupleDesc tupdesc;
    HeapTuple tuple;
    Datum values[13];
    bool nulls[13] = {false};

    psql_bm25s_init_reloptions();
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
    {
        ereport(ERROR, (errmsg("could not resolve result record type")));
    }
    BlessTupleDesc(tupdesc);

    profile_text = PG_ARGISNULL(1) ? cstring_to_text("balanced") :
        PG_GETARG_TEXT_PP(1);
    profile = text_to_cstring(profile_text);
    indexRelation = index_open(index_oid, AccessShareLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_get_policy_recommendation(
        indexRelation,
        profile,
        &recommendation
    );
    index_close(indexRelation, AccessShareLock);

    values[0] = ObjectIdGetDatum(index_oid);
    values[1] = PointerGetDatum(cstring_to_text(recommendation.profile));
    values[2] = PointerGetDatum(cstring_to_text(recommendation.confidence));
    values[3] = PointerGetDatum(
        cstring_to_text(recommendation.recommended_options)
    );
    values[4] = PointerGetDatum(
        cstring_to_text(recommendation.recommended_consistency)
    );
    if (recommendation.has_recommended_auto_rebuild_threshold)
    {
        values[5] = Int32GetDatum(
            recommendation.recommended_auto_rebuild_threshold
        );
    }
    else
    {
        nulls[5] = true;
    }
    if (recommendation.has_recommended_auto_rebuild_delta_bytes)
    {
        values[6] = Int32GetDatum(
            recommendation.recommended_auto_rebuild_delta_bytes
        );
    }
    else
    {
        nulls[6] = true;
    }
    if (recommendation.has_recommended_auto_rebuild_churn_ratio)
    {
        values[7] = Float8GetDatum(
            recommendation.recommended_auto_rebuild_churn_ratio
        );
    }
    else
    {
        nulls[7] = true;
    }
    values[8] = BoolGetDatum(recommendation.matches_current);
    values[9] = BoolGetDatum(recommendation.refresh_now);
    values[10] = Int64GetDatum((int64) recommendation.docs);
    values[11] = Int64GetDatum((int64) recommendation.pending_total);
    values[12] = PointerGetDatum(cstring_to_text(recommendation.reason));

    tuple = heap_form_tuple(tupdesc, values, nulls);
    PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}

PG_FUNCTION_INFO_V1(psql_bm25s_refresh_index);
Datum
psql_bm25s_refresh_index(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    IndexInfo *indexInfo;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessExclusiveLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_require_index_owner(indexRelation);
    indexInfo = BuildIndexInfo(indexRelation);
    CommandCounterIncrement();
    psql_bm25s_am_reindex_relation(indexRelation, indexInfo);
    psql_bm25s_am_unschedule_refresh(index_oid);
    result = psql_bm25s_am_describe_relation(indexRelation);
    if (indexInfo != NULL)
    {
        pfree(indexInfo);
    }
    index_close(indexRelation, AccessExclusiveLock);

    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_maintain_index);
Datum
psql_bm25s_maintain_index(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    Relation indexRelation;
    IndexInfo *indexInfo = NULL;
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    text *result;

    psql_bm25s_init_reloptions();
    indexRelation = index_open(index_oid, AccessExclusiveLock);
    psql_bm25s_am_require_index_relation(indexRelation);
    psql_bm25s_am_require_index_owner(indexRelation);
    psql_bm25s_am_read_meta(indexRelation, &meta);
    psql_bm25s_am_payload_health(indexRelation, &meta, &health);
    if (!psql_bm25s_am_meta_has_pending_maintenance(&meta) &&
        (meta.flags & PSQL_BM25S_AM_FLAG_STALE) == 0 &&
        !health.rebuild_required)
    {
        result = cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=no_pending)"
        );
        index_close(indexRelation, AccessExclusiveLock);
        PG_RETURN_TEXT_P(result);
    }

    indexInfo = BuildIndexInfo(indexRelation);
    CommandCounterIncrement();
    psql_bm25s_am_reindex_relation(indexRelation, indexInfo);
    psql_bm25s_am_unschedule_refresh(index_oid);
    result = psql_bm25s_am_describe_relation(indexRelation);
    if (indexInfo != NULL)
    {
        pfree(indexInfo);
    }
    index_close(indexRelation, AccessExclusiveLock);

    PG_RETURN_TEXT_P(result);
}

static text *
psql_bm25s_am_try_maintain_index_oid(Oid index_oid)
{
    Relation indexRelation;
    IndexInfo *indexInfo = NULL;
    text *result;
    bool use_online;

    psql_bm25s_init_reloptions();
    /*
     * Online maintenance releases heavyweight relation locks while building
     * the replacement index so normal queries and RowExclusive writes keep
     * moving. This per-index advisory lock only deduplicates psql_bm25s
     * maintenance workers, preventing parallel rebuilds of the same index.
     */
    if (!psql_bm25s_am_try_maintenance_lock(index_oid))
    {
        return cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=maintenance)"
        );
    }

    if (!ConditionalLockRelationOid(index_oid, AccessShareLock))
    {
        result = cstring_to_text(
            "psql_bm25s_maintenance_result(maintained=false, "
            "reason=lock_busy, mode=try)"
        );
        psql_bm25s_am_maintenance_unlock(index_oid);
        return result;
    }

    indexRelation = index_open(index_oid, NoLock);
    PG_TRY();
    {
        psql_bm25s_am_require_index_relation(indexRelation);
        psql_bm25s_am_require_index_owner(indexRelation);
        use_online =
            psql_bm25s_am_eventual_policy_enabled(indexRelation) &&
            !psql_bm25s_am_foreground_maintenance_enabled(indexRelation);
        if (use_online)
        {
            indexInfo = BuildIndexInfo(indexRelation);
            result = psql_bm25s_am_online_maintain_relation(
                indexRelation,
                indexInfo
            );
        }
        else
        {
            result = NULL;
        }
    }
    PG_CATCH();
    {
        if (indexInfo != NULL)
        {
            pfree(indexInfo);
        }
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_maintenance_unlock(index_oid);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if (indexInfo != NULL)
    {
        pfree(indexInfo);
    }
    index_close(indexRelation, AccessShareLock);

    if (result != NULL)
    {
        psql_bm25s_am_maintenance_unlock(index_oid);
        return result;
    }

    PG_TRY();
    {
        result = psql_bm25s_am_try_blocking_maintain_oid(index_oid);
    }
    PG_CATCH();
    {
        psql_bm25s_am_maintenance_unlock(index_oid);
        PG_RE_THROW();
    }
    PG_END_TRY();

    psql_bm25s_am_maintenance_unlock(index_oid);
    return result;
}

PG_FUNCTION_INFO_V1(psql_bm25s_try_maintain_index);
Datum
psql_bm25s_try_maintain_index(PG_FUNCTION_ARGS)
{
    Oid index_oid = PG_GETARG_OID(0);
    text *result;

    result = psql_bm25s_am_try_maintain_index_oid(index_oid);
    PG_RETURN_TEXT_P(result);
}

static bool
psql_bm25s_am_maintenance_result_counts(text *result)
{
    char *result_str;
    bool counts;

    if (result == NULL)
    {
        return false;
    }

    result_str = text_to_cstring(result);
    counts = strstr(result_str, "reason=no_pending") == NULL &&
        strstr(result_str, "reason=not_due") == NULL &&
        strstr(result_str, "reason=lock_busy") == NULL &&
        strstr(result_str, "reason=concurrent_change") == NULL &&
        strstr(result_str, "reason=memory_budget") == NULL;
    pfree(result_str);
    return counts;
}

static bool
psql_bm25s_am_get_background_due_candidate(
    Oid index_oid,
    psql_bm25s_am_maintenance_candidate *candidate_out
)
{
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    Oid am_oid;
    bool due = false;

    memset(candidate_out, 0, sizeof(*candidate_out));
    if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(index_oid)))
    {
        return false;
    }
    am_oid = get_am_oid("psql_bm25s", true);
    if (!OidIsValid(am_oid))
    {
        return false;
    }
    if (!ConditionalLockRelationOid(index_oid, AccessShareLock))
    {
        return false;
    }

    indexRelation = index_open(index_oid, NoLock);
    PG_TRY();
    {
        if (indexRelation->rd_rel->relkind == RELKIND_INDEX &&
            indexRelation->rd_rel->relam == am_oid &&
            psql_bm25s_am_background_policy_active(indexRelation))
        {
            uint64 pending_total;
            bool stale;
            psql_bm25s_am_payload_health_state health;

            psql_bm25s_am_read_meta(indexRelation, &meta);
            psql_bm25s_am_payload_health(indexRelation, &meta, &health);
            pending_total = (uint64) meta.pending_write_tuples +
                (uint64) meta.pending_delete_tuples;
            stale = (meta.flags & PSQL_BM25S_AM_FLAG_STALE) != 0;
            /*
             * query_overlay_max_* is a foreground query budget, not a rebuild
             * trigger. A large PubMed-like index can collect a 20MB delta that
             * is too expensive to merge inline, but rebuilding a 12GB active
             * generation for that small tail is worse for query stability.
             * Use auto_rebuild_threshold / auto_rebuild_delta_bytes for
             * background convergence pressure instead.
             */
            due = psql_bm25s_am_eventual_background_maintenance_due(
                indexRelation,
                &meta,
                &health
            );
            if (due)
            {
                candidate_out->index_oid = index_oid;
                candidate_out->debt_records = Max(
                    pending_total,
                    (uint64) meta.delta_record_count
                );
                candidate_out->debt_bytes = meta.delta_bytes_len;
                /*
                 * "Hot" means the current generation is resident, not merely
                 * that an older generation for the same index still occupies
                 * shared memory. Otherwise stale arena entries can bias
                 * maintenance priority after a rebuild.
                 */
                psql_bm25s_am_shared_preload_index_state(
                    indexRelation,
                    &meta,
                    &candidate_out->hot,
                    NULL
                );
                candidate_out->stale = stale;
                candidate_out->urgent = health.corrupt ||
                    health.rebuild_required;
            }
        }
    }
    PG_CATCH();
    {
        index_close(indexRelation, AccessShareLock);
        PG_RE_THROW();
    }
    PG_END_TRY();

    index_close(indexRelation, AccessShareLock);
    return due;
}

static int
psql_bm25s_am_cmp_maintenance_candidate(const void *left, const void *right)
{
    const psql_bm25s_am_maintenance_candidate *a = left;
    const psql_bm25s_am_maintenance_candidate *b = right;

    if (a->stale != b->stale)
    {
        return a->stale ? -1 : 1;
    }
    if (a->hot != b->hot)
    {
        return a->hot ? -1 : 1;
    }
    if (a->urgent != b->urgent)
    {
        return a->urgent ? -1 : 1;
    }
    if (a->debt_records != b->debt_records)
    {
        return a->debt_records > b->debt_records ? -1 : 1;
    }
    if (a->debt_bytes != b->debt_bytes)
    {
        return a->debt_bytes > b->debt_bytes ? -1 : 1;
    }
    if (a->index_oid == b->index_oid)
    {
        return 0;
    }
    return a->index_oid < b->index_oid ? -1 : 1;
}

static psql_bm25s_am_auto_preload_attempt
psql_bm25s_am_try_auto_preload_index_oid(Oid index_oid)
{
    Relation indexRelation;
    psql_bm25s_am_meta_page meta;
    psql_bm25s_am_payload_health_state health;
    psql_bm25s_am_cache_entry *entry = NULL;
    bool preloaded = false;
    bool should_preload = false;
    bool old_preload_publisher_active =
        psql_bm25s_am_preload_publisher_active;

    if (!psql_bm25s_am_try_maintenance_lock(index_oid))
    {
        return PSQL_BM25S_AM_AUTO_PRELOAD_BUSY;
    }
    if (!ConditionalLockRelationOid(index_oid, AccessShareLock))
    {
        psql_bm25s_am_maintenance_unlock(index_oid);
        return PSQL_BM25S_AM_AUTO_PRELOAD_BUSY;
    }

    indexRelation = index_open(index_oid, NoLock);
    PG_TRY();
    {
        psql_bm25s_am_require_index_relation(indexRelation);
        psql_bm25s_am_read_meta(indexRelation, &meta);
        psql_bm25s_am_payload_health(indexRelation, &meta, &health);
        if (psql_bm25s_am_auto_preload_priority(indexRelation) > 0 &&
            !health.corrupt)
        {
            psql_bm25s_am_prewarm_delta_pages(indexRelation, &meta);
        }
        should_preload =
            psql_bm25s_am_auto_preload_priority(indexRelation) > 0 &&
            !health.corrupt &&
            psql_bm25s_am_shared_preload_should_load(indexRelation, &meta);
        if (should_preload)
        {
            psql_bm25s_am_preload_publisher_active = true;
            entry = psql_bm25s_am_get_cached_index(
                indexRelation,
                meta.source_type
            );
            psql_bm25s_am_preload_publisher_active =
                old_preload_publisher_active;
            preloaded = entry != NULL &&
                psql_bm25s_am_generation_block_is_preload(
                    entry->generation.block
                );
            psql_bm25s_am_cache_release_lease(entry);
            entry = NULL;
        }
    }
    PG_CATCH();
    {
        psql_bm25s_am_preload_publisher_active =
            old_preload_publisher_active;
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_maintenance_unlock(index_oid);
        PG_RE_THROW();
    }
    PG_END_TRY();

    index_close(indexRelation, AccessShareLock);
    psql_bm25s_am_maintenance_unlock(index_oid);
    if (preloaded)
    {
        return PSQL_BM25S_AM_AUTO_PRELOAD_DONE;
    }
    if (should_preload)
    {
        return PSQL_BM25S_AM_AUTO_PRELOAD_BUSY;
    }
    return PSQL_BM25S_AM_AUTO_PRELOAD_NONE;
}

static int
psql_bm25s_am_auto_preload_due_indexes(void)
{
    int ret;
    uint64 i;
    int preloaded = 0;
    bool busy = false;
    List *candidate_oids = NIL;
    ListCell *cell;
    bool spi_connected = false;

    if (!psql_bm25s_am_shared_preload_cache_available())
    {
        return 0;
    }

    PG_TRY();
    {
        ret = SPI_connect();
        if (ret != SPI_OK_CONNECT)
        {
            ereport(ERROR, (errmsg("SPI_connect failed: %d", ret)));
        }
        spi_connected = true;

        ret = SPI_execute(
            "SELECT c.oid "
            "FROM pg_catalog.pg_class c "
            "JOIN pg_catalog.pg_am am ON am.oid = c.relam "
            "JOIN pg_catalog.pg_index i ON i.indexrelid = c.oid "
            "CROSS JOIN LATERAL ( "
            "    SELECT substring(opt FROM '^auto_preload=([0-9]+)$')::int4 "
            "        AS priority "
            "    FROM pg_catalog.unnest(c.reloptions) AS opt "
            "    WHERE opt LIKE 'auto_preload=%' "
            "    LIMIT 1 "
            ") preload "
            "WHERE c.relkind = 'i' "
            "AND am.amname = 'psql_bm25s' "
            "AND i.indisvalid "
            "AND i.indisready "
            "AND preload.priority > 0 "
            "AND pg_catalog.pg_has_role(c.relowner, 'USAGE') "
            /*
             * Same-priority preload candidates should warm the large resident
             * generations first. OID order can leave PubMed/arXiv cold for
             * several startup cycles, so the first foreground query has to wait
             * for a multi-GB load. Relation size is a cheap catalog-visible
             * proxy for the immutable generation payload and is good enough for
             * scheduling; the preload path still validates the exact payload
             * size before publishing.
             */
            "ORDER BY preload.priority DESC, "
            "pg_catalog.pg_relation_size(c.oid) DESC, c.oid",
            true,
            0
        );
        if (ret != SPI_OK_SELECT)
        {
            ereport(ERROR, (errmsg("SPI_execute failed: %d", ret)));
        }

        for (i = 0; i < SPI_processed; i++)
        {
            bool isnull;
            Datum datum;
            MemoryContext oldcontext;

            datum = SPI_getbinval(
                SPI_tuptable->vals[i],
                SPI_tuptable->tupdesc,
                1,
                &isnull
            );
            if (isnull)
            {
                continue;
            }

            oldcontext = MemoryContextSwitchTo(TopTransactionContext);
            candidate_oids = lappend_oid(
                candidate_oids,
                DatumGetObjectId(datum)
            );
            MemoryContextSwitchTo(oldcontext);
        }

        SPI_finish();
        spi_connected = false;

        foreach (cell, candidate_oids)
        {
            Oid index_oid = lfirst_oid(cell);
            psql_bm25s_am_auto_preload_attempt attempt;

            attempt = psql_bm25s_am_try_auto_preload_index_oid(index_oid);
            if (attempt == PSQL_BM25S_AM_AUTO_PRELOAD_DONE)
            {
                preloaded++;
            }
            if (attempt == PSQL_BM25S_AM_AUTO_PRELOAD_BUSY)
            {
                busy = true;
            }
        }
    }
    PG_CATCH();
    {
        if (spi_connected)
        {
            SPI_finish();
        }
        list_free(candidate_oids);
        PG_RE_THROW();
    }
    PG_END_TRY();

    list_free(candidate_oids);
    if (preloaded == 0 && busy)
    {
        return -1;
    }
    return preloaded;
}

static int
psql_bm25s_am_maintenance_worker_due_indexes(void)
{
    int ret;
    uint64 i;
    int maintained = 0;
    psql_bm25s_am_maintenance_candidate *candidates = NULL;
    size_t candidate_len = 0;
    size_t candidate_capacity = 0;
    List *candidate_oids = NIL;
    ListCell *cell;

    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
    {
        ereport(ERROR, (errmsg("SPI_connect failed: %d", ret)));
    }

    ret = SPI_execute(
        "SELECT c.oid "
        "FROM pg_catalog.pg_class c "
        "JOIN pg_catalog.pg_am am ON am.oid = c.relam "
        "JOIN pg_catalog.pg_index i ON i.indexrelid = c.oid "
        "WHERE c.relkind = 'i' "
        "AND am.amname = 'psql_bm25s' "
        "AND i.indisvalid "
        "AND i.indisready "
        "AND pg_catalog.pg_has_role(c.relowner, 'USAGE') "
        "AND COALESCE(c.reloptions, ARRAY[]::text[]) "
        "@> ARRAY['consistency=eventual']::text[] "
        "ORDER BY c.oid",
        true,
        0
    );
    if (ret != SPI_OK_SELECT)
    {
        SPI_finish();
        ereport(ERROR, (errmsg("SPI_execute failed: %d", ret)));
    }

    for (i = 0; i < SPI_processed; i++)
    {
        bool isnull;
        Datum datum;
        Oid index_oid;
        MemoryContext oldcontext;

        datum = SPI_getbinval(
            SPI_tuptable->vals[i],
            SPI_tuptable->tupdesc,
            1,
            &isnull
        );
        if (isnull)
        {
            continue;
        }

        index_oid = DatumGetObjectId(datum);
        oldcontext = MemoryContextSwitchTo(TopTransactionContext);
        candidate_oids = lappend_oid(candidate_oids, index_oid);
        MemoryContextSwitchTo(oldcontext);
    }

    SPI_finish();

    foreach (cell, candidate_oids)
    {
        Oid index_oid = lfirst_oid(cell);
        psql_bm25s_am_maintenance_candidate candidate;

        if (!psql_bm25s_am_get_background_due_candidate(
                index_oid,
                &candidate))
        {
            continue;
        }
        if (candidate_len == candidate_capacity)
        {
            size_t new_capacity = candidate_capacity == 0
                ? 16
                : candidate_capacity * 2;
            size_t bytes;

            if (!psql_bm25s_am_checked_mul_size(
                    new_capacity,
                    sizeof(*candidates),
                    &bytes))
            {
                list_free(candidate_oids);
                if (candidates != NULL)
                {
                    pfree(candidates);
                }
                psql_bm25s_am_oom();
            }
            if (candidates == NULL)
            {
                candidates = palloc(bytes);
            }
            else
            {
                candidates = repalloc(candidates, bytes);
            }
            candidate_capacity = new_capacity;
        }
        candidates[candidate_len++] = candidate;
    }

    list_free(candidate_oids);

    if (candidate_len > 1)
    {
        qsort(
            candidates,
            candidate_len,
            sizeof(*candidates),
            psql_bm25s_am_cmp_maintenance_candidate
        );
    }
    if (candidate_len == 0 || !psql_bm25s_am_claim_maintenance_cycle())
    {
        if (candidates != NULL)
        {
            pfree(candidates);
        }
        return 0;
    }

    for (i = 0; i < candidate_len; i++)
    {
        Oid index_oid = candidates[i].index_oid;
        text *result;

        result = psql_bm25s_am_try_maintain_index_oid(index_oid);
        if (psql_bm25s_am_maintenance_result_counts(result))
        {
            maintained++;
        }
        if (result != NULL)
        {
            pfree(result);
        }
        if (maintained > 0)
        {
            break;
        }
    }

    if (candidates != NULL)
    {
        pfree(candidates);
    }
    return maintained;
}

static void
psql_bm25s_am_report_background_worker_phase(
    psql_bm25s_am_background_worker_phase phase
)
{
    const char *appname;
    const char *activity;
    BackendState state;

    switch (phase)
    {
        case PSQL_BM25S_AM_WORKER_PHASE_PRELOAD:
            appname = "psql_bm25s preload";
            activity = "auto-preload";
            state = STATE_RUNNING;
            break;
        case PSQL_BM25S_AM_WORKER_PHASE_INDEX_MAINTENANCE:
            appname = "psql_bm25s maintenance";
            activity = "index maintenance";
            state = STATE_RUNNING;
            break;
        case PSQL_BM25S_AM_WORKER_PHASE_IDLE:
        default:
            appname = "psql_bm25s background";
            activity = "idle";
            state = STATE_IDLE;
            break;
    }

    pgstat_report_appname(appname);
    pgstat_report_activity(state, activity);
}

static void
psql_bm25s_am_adjust_background_worker_phase_locked(
    psql_bm25s_am_background_worker_phase phase,
    int delta
)
{
    uint32 *counter = NULL;

    switch (phase)
    {
        case PSQL_BM25S_AM_WORKER_PHASE_PRELOAD:
            counter = &psql_bm25s_shared_preload->active_preload_workers;
            break;
        case PSQL_BM25S_AM_WORKER_PHASE_INDEX_MAINTENANCE:
            counter =
                &psql_bm25s_shared_preload->active_index_maintenance_workers;
            break;
        case PSQL_BM25S_AM_WORKER_PHASE_IDLE:
        default:
            return;
    }

    if (delta > 0)
    {
        if (*counter < UINT32_MAX)
        {
            (*counter)++;
        }
    }
    else if (delta < 0)
    {
        if (*counter > 0)
        {
            (*counter)--;
        }
    }
}

static void
psql_bm25s_am_note_background_worker_phase(
    psql_bm25s_am_background_worker_phase phase
)
{
    if (psql_bm25s_am_current_worker_phase == phase)
    {
        psql_bm25s_am_report_background_worker_phase(phase);
        return;
    }

    if (psql_bm25s_am_shared_preload_available())
    {
        SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
        psql_bm25s_am_adjust_background_worker_phase_locked(
            psql_bm25s_am_current_worker_phase,
            -1
        );
        psql_bm25s_am_adjust_background_worker_phase_locked(phase, 1);
        SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    }

    psql_bm25s_am_current_worker_phase = phase;
    psql_bm25s_am_report_background_worker_phase(phase);
}

static void
psql_bm25s_am_note_maintenance_worker_started(void)
{
    if (!psql_bm25s_am_shared_preload_available() ||
        psql_bm25s_am_maintenance_worker_counted_active)
    {
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    if (psql_bm25s_shared_preload->active_maintenance_workers < UINT32_MAX)
    {
        psql_bm25s_shared_preload->active_maintenance_workers++;
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_am_maintenance_worker_counted_active = true;
}

static void
psql_bm25s_am_note_maintenance_worker_finished(void)
{
    psql_bm25s_am_note_background_worker_phase(
        PSQL_BM25S_AM_WORKER_PHASE_IDLE
    );

    if (!psql_bm25s_am_maintenance_worker_counted_active)
    {
        return;
    }
    if (!psql_bm25s_am_shared_preload_available())
    {
        psql_bm25s_am_maintenance_worker_counted_active = false;
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    if (psql_bm25s_shared_preload->active_maintenance_workers > 0)
    {
        psql_bm25s_shared_preload->active_maintenance_workers--;
    }
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_am_maintenance_worker_counted_active = false;
}

static void
psql_bm25s_am_maintenance_worker_shmem_exit(int code, Datum arg)
{
    (void) code;
    (void) arg;

    /*
     * Heavyweight locks are process-owned and PostgreSQL releases them during
     * backend exit, but the supervisor's active-worker counter is extension
     * shared memory. Keep it balanced even if a worker exits outside the normal
     * success or ERROR cleanup paths.
     */
    psql_bm25s_am_note_maintenance_worker_finished();
}

static void
psql_bm25s_am_register_maintenance_worker_exit(void)
{
    if (psql_bm25s_am_maintenance_worker_exit_registered)
    {
        return;
    }

    before_shmem_exit(psql_bm25s_am_maintenance_worker_shmem_exit, 0);
    psql_bm25s_am_maintenance_worker_exit_registered = true;
}

static uint32
psql_bm25s_am_active_maintenance_workers(void)
{
    uint32 active = 0;

    if (!psql_bm25s_am_shared_preload_available())
    {
        return 0;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    active = psql_bm25s_shared_preload->active_maintenance_workers;
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    return active;
}

static Oid
psql_bm25s_am_last_maintenance_db_oid(void)
{
    Oid db_oid = InvalidOid;

    if (!psql_bm25s_am_shared_preload_available())
    {
        return InvalidOid;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    db_oid = psql_bm25s_shared_preload->last_maintenance_db_oid;
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
    return db_oid;
}

static void
psql_bm25s_am_set_last_maintenance_db_oid(Oid db_oid)
{
    if (!psql_bm25s_am_shared_preload_available())
    {
        return;
    }

    SpinLockAcquire(&psql_bm25s_shared_preload->mutex);
    psql_bm25s_shared_preload->last_maintenance_db_oid = db_oid;
    SpinLockRelease(&psql_bm25s_shared_preload->mutex);
}

PGDLLEXPORT void
psql_bm25s_maintenance_worker_main(Datum main_arg)
{
    psql_bm25s_am_bgworker_args args;
    bool pushed_snapshot = false;
    bool transaction_started = false;
    bool slot_acquired = false;
    bool counted_active = false;
    int worker_slot = -1;

    (void) main_arg;

    memset(&args, 0, sizeof(args));
    if (MyBgworkerEntry != NULL)
    {
        memcpy(&args, MyBgworkerEntry->bgw_extra, sizeof(args));
    }

    BackgroundWorkerUnblockSignals();
    if (!OidIsValid(args.db_oid))
    {
        return;
    }

    BackgroundWorkerInitializeConnectionByOid(
        args.db_oid,
        args.user_oid,
        0
    );
    psql_bm25s_am_note_background_worker_phase(
        PSQL_BM25S_AM_WORKER_PHASE_IDLE
    );
    psql_bm25s_am_register_maintenance_worker_exit();

    PG_TRY();
    {
        StartTransactionCommand();
        transaction_started = true;
        if (!psql_bm25s_am_try_maintenance_worker_slot(&worker_slot))
        {
            CommitTransactionCommand();
            transaction_started = false;
        }
        else
        {
            slot_acquired = true;
            psql_bm25s_am_note_maintenance_worker_started();
            counted_active = true;
            PushActiveSnapshot(GetTransactionSnapshot());
            pushed_snapshot = true;
            psql_bm25s_init_reloptions();
            /*
             * Auto-preload uses the same bounded worker pool as maintenance
             * but drains every due marked index in priority order. Per-index
             * maintenance locks let concurrent workers skip an already claimed
             * preload candidate and warm a different marked index instead. If
             * all preload work is already claimed, the worker waits for the
             * next cycle instead of starting maintenance, keeping startup
             * warmup ahead of rebuild catch-up.
             */
            {
                int preload_result;

                psql_bm25s_am_note_background_worker_phase(
                    PSQL_BM25S_AM_WORKER_PHASE_PRELOAD
                );
                preload_result = psql_bm25s_am_auto_preload_due_indexes();
                if (preload_result == 0 &&
                    args.allow_maintenance &&
                    !RecoveryInProgress())
                {
                    psql_bm25s_am_note_background_worker_phase(
                        PSQL_BM25S_AM_WORKER_PHASE_INDEX_MAINTENANCE
                    );
                    (void) psql_bm25s_am_maintenance_worker_due_indexes();
                }
            }
            PopActiveSnapshot();
            pushed_snapshot = false;
            psql_bm25s_am_note_maintenance_worker_finished();
            counted_active = false;
            psql_bm25s_am_release_maintenance_worker_slot(worker_slot);
            slot_acquired = false;
            CommitTransactionCommand();
            transaction_started = false;
        }
    }
    PG_CATCH();
    {
        EmitErrorReport();
        FlushErrorState();
        if (pushed_snapshot)
        {
            PopActiveSnapshot();
        }
        if (counted_active)
        {
            psql_bm25s_am_note_maintenance_worker_finished();
        }
        if (slot_acquired)
        {
            psql_bm25s_am_release_maintenance_worker_slot(worker_slot);
        }
        if (transaction_started)
        {
            AbortCurrentTransaction();
        }
    }
PG_END_TRY();
}

PGDLLEXPORT void
psql_bm25s_maintenance_supervisor_main(Datum main_arg)
{
    (void) main_arg;

    BackgroundWorkerUnblockSignals();
    BackgroundWorkerInitializeConnectionByOid(
        Template1DbOid,
        BOOTSTRAP_SUPERUSERID,
        BGWORKER_BYPASS_ALLOWCONN | BGWORKER_BYPASS_ROLELOGINCHECK
    );
    pgstat_report_appname("psql_bm25s background supervisor");
    pgstat_report_activity(STATE_IDLE, "idle");

    for (;;)
    {
        bool pushed_snapshot = false;
        bool transaction_started = false;
        int interval_ms;

        CHECK_FOR_INTERRUPTS();
        PG_TRY();
        {
            int limit;
            uint32 active;
            int available;
            int ret;
            uint64 i;
            uint64 db_count;
            Oid *db_oids = NULL;
            Oid last_db_oid;
            int start = 0;
            bool allow_maintenance = true;

            limit = psql_bm25s_am_effective_maintenance_worker_limit();
            active = psql_bm25s_am_active_maintenance_workers();
            available = limit - (int) Min(active, (uint32) limit);
            if (available > 0)
            {
                StartTransactionCommand();
                transaction_started = true;
                PushActiveSnapshot(GetTransactionSnapshot());
                pushed_snapshot = true;

                ret = SPI_connect();
                if (ret != SPI_OK_CONNECT)
                {
                    ereport(ERROR, (errmsg("SPI_connect failed: %d", ret)));
                }
                ret = SPI_execute(
                    "SELECT oid "
                    "FROM pg_catalog.pg_database "
                    "WHERE datallowconn AND NOT datistemplate "
                    "ORDER BY oid",
                    true,
                    0
                );
                if (ret != SPI_OK_SELECT)
                {
                    SPI_finish();
                    ereport(ERROR, (errmsg("SPI_execute failed: %d", ret)));
                }

                db_count = SPI_processed;
                if (db_count > 0)
                {
                    db_oids = palloc(sizeof(*db_oids) * db_count);
                    for (i = 0; i < db_count; i++)
                    {
                        bool isnull;
                        Datum datum;

                        datum = SPI_getbinval(
                            SPI_tuptable->vals[i],
                            SPI_tuptable->tupdesc,
                            1,
                            &isnull
                        );
                        db_oids[i] = isnull ? InvalidOid : DatumGetObjectId(datum);
                    }
                }
                SPI_finish();

                last_db_oid = psql_bm25s_am_last_maintenance_db_oid();
                for (i = 0; i < db_count; i++)
                {
                    if (db_oids[i] > last_db_oid)
                    {
                        start = (int) i;
                        break;
                    }
                }

                for (i = 0; i < (uint64) available && db_count > 0; i++)
                {
                    Oid db_oid = db_oids[(start + (int) i) % (int) db_count];

                    if (!OidIsValid(db_oid))
                    {
                        continue;
                    }
                    if (psql_bm25s_am_launch_background_maintenance(
                            db_oid,
                            BOOTSTRAP_SUPERUSERID,
                            allow_maintenance))
                    {
                        psql_bm25s_am_set_last_maintenance_db_oid(db_oid);
                    }
                }

                if (db_oids != NULL)
                {
                    pfree(db_oids);
                }
                PopActiveSnapshot();
                pushed_snapshot = false;
                CommitTransactionCommand();
                transaction_started = false;
            }
        }
        PG_CATCH();
        {
            EmitErrorReport();
            FlushErrorState();
            if (pushed_snapshot)
            {
                PopActiveSnapshot();
            }
            if (transaction_started)
            {
                AbortCurrentTransaction();
            }
        }
        PG_END_TRY();

        interval_ms = psql_bm25s_am_supervisor_timer_interval();
        if (interval_ms < 1000)
        {
            interval_ms = 1000;
        }
        WaitLatch(
            &MyProc->procLatch,
            WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
            interval_ms,
            0
        );
        ResetLatch(&MyProc->procLatch);
    }
}

static int
psql_bm25s_am_find_index_field(Relation indexRelation, const char *field_name)
{
    int i;

    if (field_name == NULL || field_name[0] == '\0')
    {
        ereport(ERROR, (errmsg("field names cannot be empty")));
    }
    for (i = 0; i < psql_bm25s_am_index_natts(indexRelation); i++)
    {
        Form_pg_attribute attr = TupleDescAttr(indexRelation->rd_att, i);

        if (!attr->attisdropped &&
            strcmp(NameStr(attr->attname), field_name) == 0)
        {
            return i;
        }
    }
    ereport(
        ERROR,
        (
            errmsg("field \"%s\" is not part of the psql_bm25s index", field_name)
        )
    );
    return -1;
}

static void
psql_bm25s_am_read_field_specs(
    Relation indexRelation,
    ArrayType *field_names_array,
    ArrayType *weights_array,
    int **field_indexes_out,
    float **field_weights_out,
    size_t *num_fields_out
)
{
    Datum *field_datums = NULL;
    bool *field_nulls = NULL;
    int field_count = 0;
    Datum *weight_datums = NULL;
    bool *weight_nulls = NULL;
    int weight_count = 0;
    int *field_indexes;
    float *field_weights;
    int i;

    *field_indexes_out = NULL;
    *field_weights_out = NULL;
    *num_fields_out = 0;
    deconstruct_array(
        field_names_array,
        TEXTOID,
        -1,
        false,
        TYPALIGN_INT,
        &field_datums,
        &field_nulls,
        &field_count
    );
    if (field_count <= 0)
    {
        ereport(ERROR, (errmsg("at least one field name is required")));
    }
    if (weights_array != NULL)
    {
        deconstruct_array(
            weights_array,
            FLOAT4OID,
            4,
            true,
            TYPALIGN_INT,
            &weight_datums,
            &weight_nulls,
            &weight_count
        );
        if (weight_count != field_count)
        {
            ereport(
                ERROR,
                (
                    errmsg("field_names and weights must have the same length")
                )
            );
        }
    }

    field_indexes = palloc(sizeof(*field_indexes) * (size_t) field_count);
    field_weights = palloc(sizeof(*field_weights) * (size_t) field_count);
    for (i = 0; i < field_count; i++)
    {
        char *field_name;

        if (field_nulls[i])
        {
            ereport(ERROR, (errmsg("field_names cannot contain NULLs")));
        }
        field_name = TextDatumGetCString(field_datums[i]);
        field_indexes[i] = psql_bm25s_am_find_index_field(
            indexRelation,
            field_name
        );
        pfree(field_name);
        if (weights_array == NULL)
        {
            field_weights[i] = 1.0f;
        }
        else if (weight_nulls[i])
        {
            ereport(ERROR, (errmsg("weights cannot contain NULLs")));
        }
        else
        {
            field_weights[i] = DatumGetFloat4(weight_datums[i]);
        }
    }

    if (field_datums != NULL)
    {
        pfree(field_datums);
    }
    if (field_nulls != NULL)
    {
        pfree(field_nulls);
    }
    if (weight_datums != NULL)
    {
        pfree(weight_datums);
    }
    if (weight_nulls != NULL)
    {
        pfree(weight_nulls);
    }

    *field_indexes_out = field_indexes;
    *field_weights_out = field_weights;
    *num_fields_out = (size_t) field_count;
}

static void
psql_bm25s_am_read_all_field_specs(
    Relation indexRelation,
    int **field_indexes_out,
    float **field_weights_out,
    size_t *num_fields_out
)
{
    int natts = psql_bm25s_am_index_natts(indexRelation);
    int *field_indexes;
    float *field_weights;
    int i;

    if (natts <= 0)
    {
        ereport(ERROR, (errmsg("psql_bm25s index has no fields")));
    }

    field_indexes = palloc(sizeof(*field_indexes) * (size_t) natts);
    field_weights = palloc(sizeof(*field_weights) * (size_t) natts);
    for (i = 0; i < natts; i++)
    {
        field_indexes[i] = i;
        field_weights[i] = 1.0f;
    }

    *field_indexes_out = field_indexes;
    *field_weights_out = field_weights;
    *num_fields_out = (size_t) natts;
}

PG_FUNCTION_INFO_V1(psql_bm25s_field_aware_query_tokens);
Datum
psql_bm25s_field_aware_query_tokens(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        Oid index_oid = PG_GETARG_OID(0);
        ArrayType *query_array = PG_GETARG_ARRAYTYPE_P(1);
        ArrayType *field_names = PG_GETARG_ARRAYTYPE_P(2);
        ArrayType *weights = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
        int32 k = PG_GETARG_INT32(4);
        Relation indexRelation;
        Relation heapRelation;
        psql_bm25s_am_cache_entry *entry;
        Oid source_type;
        char **query_tokens;
        size_t query_len;
        int *field_indexes;
        float *field_weights;
        size_t num_fields;

        psql_bm25s_init_reloptions();
        if (k < 0)
        {
            ereport(ERROR, (errmsg("k must be non-negative")));
        }

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));

        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        if (!psql_bm25s_am_is_multicol_index(indexRelation))
        {
            ereport(
                ERROR,
                (
                    errmsg(
                        "psql_bm25s field-aware search requires a multicolumn index"
                    )
                )
            );
        }
        source_type = psql_bm25s_am_require_textlike_source_type(
            indexRelation,
            "psql_bm25s field-aware searches require a text-like index"
        );
        entry = psql_bm25s_am_get_cached_index(indexRelation, source_type);
        heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
        query_tokens = psql_bm25s_array_read_query_tokens(
            query_array,
            &query_len
        );
        psql_bm25s_am_read_field_specs(
            indexRelation,
            field_names,
            weights,
            &field_indexes,
            &field_weights,
            &num_fields
        );
        psql_bm25s_am_prepare_field_tokens_search_state(
            entry,
            indexRelation,
            heapRelation,
            query_tokens,
            query_len,
            field_indexes,
            field_weights,
            num_fields,
            NULL,
            (size_t) k,
            true,
            state
        );

        pfree(field_indexes);
        pfree(field_weights);
        psql_bm25s_am_free_query_tokens(query_tokens, query_len);
        table_close(heapRelation, AccessShareLock);
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_cache_maybe_shrink_workspace(entry);
        psql_bm25s_am_cache_release_lease(entry);

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->topk.len)
    {
        Datum values[3];
        bool nulls[3] = {false, false, false};
        HeapTuple tuple;
        size_t pos = state->pos++;

        values[0] = ItemPointerGetDatum(&state->tids[pos]);
        values[1] = Int32GetDatum((int32) state->topk.doc_ids[pos]);
        values[2] = Float4GetDatum(state->topk.scores[pos]);
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    psql_bm25s_am_search_state_reset(state);
    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(psql_bm25s_search_ids);
Datum
psql_bm25s_search_ids(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        Oid index_oid = PG_GETARG_OID(0);
        ArrayType *query_array = PG_GETARG_ARRAYTYPE_P(1);
        int32 k = PG_GETARG_INT32(2);
        ArrayType *weight_mask = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
        Relation indexRelation;
        Relation heapRelation;
        psql_bm25s_am_cache_entry *entry;
        size_t query_len;
        uint32_t *query_ids;

        psql_bm25s_init_reloptions();
        if (k < 0)
        {
            ereport(ERROR, (errmsg("k must be non-negative")));
        }

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));

        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        entry = psql_bm25s_am_get_cached_index(indexRelation, INT4ARRAYOID);
        heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
        query_ids = psql_bm25s_array_read_query_ids(query_array, &query_len);
        psql_bm25s_am_prepare_search_state(
            entry,
            heapRelation,
            query_ids,
            query_len,
            weight_mask,
            (size_t) k,
            state
        );
        pfree(query_ids);
        table_close(heapRelation, AccessShareLock);
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_cache_maybe_shrink_workspace(entry);
        psql_bm25s_am_cache_release_lease(entry);

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->topk.len)
    {
        Datum values[3];
        bool nulls[3] = {false, false, false};
        HeapTuple tuple;
        size_t pos = state->pos++;

        values[0] = ItemPointerGetDatum(&state->tids[pos]);
        values[1] = Int32GetDatum((int32) state->topk.doc_ids[pos]);
        values[2] = Float4GetDatum(state->topk.scores[pos]);
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    psql_bm25s_am_search_state_reset(state);
    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(psql_bm25s_search_tokens);
Datum
psql_bm25s_search_tokens(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        Oid index_oid = PG_GETARG_OID(0);
        ArrayType *query_array = PG_GETARG_ARRAYTYPE_P(1);
        int32 k = PG_GETARG_INT32(2);
        ArrayType *weight_mask = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
        Relation indexRelation;
        Relation heapRelation;
        psql_bm25s_am_cache_entry *entry;
        Oid source_type;
        char **query_tokens;
        size_t query_len;

        psql_bm25s_init_reloptions();
        if (k < 0)
        {
            ereport(ERROR, (errmsg("k must be non-negative")));
        }

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));

        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        source_type = psql_bm25s_am_require_textlike_source_type(
            indexRelation,
            "psql_bm25s token searches require a text[], varchar[], text, or varchar index"
        );
        entry = psql_bm25s_am_get_cached_index(indexRelation, source_type);
        heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
        query_tokens = psql_bm25s_array_read_query_tokens(
            query_array,
            &query_len
        );
        psql_bm25s_am_prepare_tokens_search_state(
            entry,
            indexRelation,
            heapRelation,
            query_tokens,
            query_len,
            weight_mask,
            (size_t) k,
            state
        );

        psql_bm25s_am_free_query_tokens(query_tokens, query_len);
        table_close(heapRelation, AccessShareLock);
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_cache_maybe_shrink_workspace(entry);
        psql_bm25s_am_cache_release_lease(entry);

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->topk.len)
    {
        Datum values[3];
        bool nulls[3] = {false, false, false};
        HeapTuple tuple;
        size_t pos = state->pos++;

        values[0] = ItemPointerGetDatum(&state->tids[pos]);
        values[1] = Int32GetDatum((int32) state->topk.doc_ids[pos]);
        values[2] = Float4GetDatum(state->topk.scores[pos]);
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    psql_bm25s_am_search_state_reset(state);
    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(psql_bm25s_search_query);
Datum
psql_bm25s_search_query(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        Oid index_oid = PG_GETARG_OID(0);
        text *query_text = PG_GETARG_TEXT_PP(1);
        int32 k = PG_GETARG_INT32(2);
        ArrayType *weight_mask = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
        Relation indexRelation;
        Relation heapRelation;
        psql_bm25s_am_cache_entry *entry;
        Oid source_type;

        psql_bm25s_init_reloptions();
        if (k < 0)
        {
            ereport(ERROR, (errmsg("k must be non-negative")));
        }

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));

        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        source_type = psql_bm25s_am_require_textlike_source_type(
            indexRelation,
            "psql_bm25s raw queries require a text[], varchar[], text, or varchar index"
        );
        entry = psql_bm25s_am_get_cached_index(indexRelation, source_type);
        heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
        psql_bm25s_am_prepare_raw_query_search_state(
            entry,
            indexRelation,
            heapRelation,
            query_text,
            weight_mask,
            (size_t) k,
            state
        );
        table_close(heapRelation, AccessShareLock);
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_cache_maybe_shrink_workspace(entry);
        psql_bm25s_am_cache_release_lease(entry);

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->topk.len)
    {
        Datum values[3];
        bool nulls[3] = {false, false, false};
        HeapTuple tuple;
        size_t pos = state->pos++;

        values[0] = ItemPointerGetDatum(&state->tids[pos]);
        values[1] = Int32GetDatum((int32) state->topk.doc_ids[pos]);
        values[2] = Float4GetDatum(state->topk.scores[pos]);
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    psql_bm25s_am_search_state_reset(state);
    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(psql_bm25s_search_query_cfg);
Datum
psql_bm25s_search_query_cfg(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        Oid index_oid = PG_GETARG_OID(0);
        text *query_text = PG_GETARG_TEXT_PP(1);
        int32 k = PG_GETARG_INT32(2);
        ArrayType *weight_mask = PG_ARGISNULL(3) ? NULL : PG_GETARG_ARRAYTYPE_P(3);
        bool lowercase_isnull = PG_ARGISNULL(4);
        bool lowercase = lowercase_isnull ? false : PG_GETARG_BOOL(4);
        bool stopwords_isnull = PG_ARGISNULL(5);
        ArrayType *stopwords_array = PG_ARGISNULL(5) ? NULL : PG_GETARG_ARRAYTYPE_P(5);
        bool stem_english_isnull = PG_ARGISNULL(6);
        bool stem_english = stem_english_isnull ? false : PG_GETARG_BOOL(6);
        bool fold_diacritics_isnull = PG_ARGISNULL(7);
        bool fold_diacritics = fold_diacritics_isnull ? false : PG_GETARG_BOOL(7);
        Relation indexRelation;
        Relation heapRelation;
        psql_bm25s_am_cache_entry *entry;
        Oid source_type;
        psql_bm25s_text_options options;
        char **stopwords = NULL;
        size_t stopword_len = 0;

        psql_bm25s_init_reloptions();
        if (k < 0)
        {
            ereport(ERROR, (errmsg("k must be non-negative")));
        }

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));

        indexRelation = index_open(index_oid, AccessShareLock);
        psql_bm25s_am_require_index_relation(indexRelation);
        source_type = psql_bm25s_am_require_textlike_source_type(
            indexRelation,
            "psql_bm25s raw queries require a text[], varchar[], text, or varchar index"
        );

        if (psql_bm25s_am_source_type_is_scalar_text(source_type) &&
            (lowercase_isnull ||
             stopwords_isnull ||
             stem_english_isnull ||
             fold_diacritics_isnull))
        {
            psql_bm25s_text_options index_options;
            char **index_stopwords = NULL;
            size_t index_stopword_len = 0;

            psql_bm25s_am_read_index_text_options(
                indexRelation,
                &index_options,
                &index_stopwords,
                &index_stopword_len
            );
            if (lowercase_isnull)
            {
                lowercase = index_options.lowercase;
            }
            if (stem_english_isnull)
            {
                stem_english = index_options.stem_english;
            }
            if (fold_diacritics_isnull)
            {
                fold_diacritics = index_options.fold_diacritics;
            }

            if (stopwords_isnull)
            {
                psql_bm25s_am_finalize_text_options(
                    lowercase,
                    stem_english,
                    fold_diacritics,
                    index_stopwords,
                    index_stopword_len,
                    &options,
                    &stopwords,
                    &stopword_len
                );
            }
            else
            {
                psql_bm25s_am_free_query_tokens(
                    index_stopwords,
                    index_stopword_len
                );
                psql_bm25s_am_read_text_options(
                    lowercase,
                    stem_english,
                    fold_diacritics,
                    stopwords_array,
                    &options,
                    &stopwords,
                    &stopword_len
                );
            }
        }
        else
        {
            psql_bm25s_am_read_text_options(
                lowercase,
                stem_english,
                fold_diacritics,
                stopwords_array,
                &options,
                &stopwords,
                &stopword_len
            );
        }

        entry = psql_bm25s_am_get_cached_index(indexRelation, source_type);
        heapRelation = table_open(indexRelation->rd_index->indrelid, AccessShareLock);
        psql_bm25s_am_prepare_raw_query_search_state_with_options(
            entry,
            indexRelation,
            heapRelation,
            query_text,
            &options,
            weight_mask,
            (size_t) k,
            state
        );
        psql_bm25s_am_free_query_tokens(stopwords, stopword_len);
        table_close(heapRelation, AccessShareLock);
        index_close(indexRelation, AccessShareLock);
        psql_bm25s_am_cache_maybe_shrink_workspace(entry);
        psql_bm25s_am_cache_release_lease(entry);

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->topk.len)
    {
        Datum values[3];
        bool nulls[3] = {false, false, false};
        HeapTuple tuple;
        size_t pos = state->pos++;

        values[0] = ItemPointerGetDatum(&state->tids[pos]);
        values[1] = Int32GetDatum((int32) state->topk.doc_ids[pos]);
        values[2] = Float4GetDatum(state->topk.scores[pos]);
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    psql_bm25s_am_search_state_reset(state);
    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(psql_bm25s_hybrid_fuse_candidates);
Datum
psql_bm25s_hybrid_fuse_candidates(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    psql_bm25s_hybrid_search_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        ArrayType *candidate_array = PG_ARGISNULL(0)
            ? NULL
            : PG_GETARG_ARRAYTYPE_P(0);
        int32 k = PG_ARGISNULL(1) ? 10 : PG_GETARG_INT32(1);
        char *fusion_text = PG_ARGISNULL(2)
            ? NULL
            : text_to_cstring(PG_GETARG_TEXT_PP(2));
        float4 rrf_arg = PG_ARGISNULL(3) ? 60.0f : PG_GETARG_FLOAT4(3);
        float4 epsilon_arg = PG_ARGISNULL(4)
            ? 0.000001f
            : PG_GETARG_FLOAT4(4);
        psql_bm25s_hybrid_fusion_method fusion_method;
        double rrf_k;
        double epsilon;

        k = Max(k, 0);
        fusion_method = psql_bm25s_hybrid_parse_fusion(fusion_text);
        rrf_k = Max((double) rrf_arg, 0.000001);
        epsilon = Max((double) epsilon_arg, 0.000000000001);

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        state = palloc0(sizeof(*state));
        psql_bm25s_hybrid_prepare_search_state(
            candidate_array,
            k,
            fusion_method,
            rrf_k,
            epsilon,
            state
        );

        funcctx->user_fctx = state;
        if (get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc) !=
            TYPEFUNC_COMPOSITE)
        {
            ereport(ERROR, (errmsg("could not resolve result record type")));
        }
        BlessTupleDesc(funcctx->tuple_desc);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    state = funcctx->user_fctx;

    if (state->pos < state->len)
    {
        Datum values[8];
        bool nulls[8] = {
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false
        };
        HeapTuple tuple;
        psql_bm25s_hybrid_hit_state *hit = &state->hits[state->pos++];

        values[0] = ItemPointerGetDatum(&hit->tid);
        values[1] = Float4GetDatum(hit->score);
        values[2] = Int32GetDatum(hit->source_count);
        values[3] = PointerGetDatum(
            psql_bm25s_am_text_array_from_cstrings(
                hit->source_names,
                hit->source_count
            )
        );
        values[4] = PointerGetDatum(
            psql_bm25s_am_float4_array_from_values(
                hit->raw_values,
                hit->source_count
            )
        );
        values[5] = PointerGetDatum(
            psql_bm25s_am_float4_array_from_values(
                hit->normalized_scores,
                hit->source_count
            )
        );
        values[6] = PointerGetDatum(
            psql_bm25s_am_float4_array_from_values(
                hit->weighted_scores,
                hit->source_count
            )
        );
        values[7] = PointerGetDatum(
            psql_bm25s_am_int4_array_from_values(
                hit->ranks,
                hit->source_count
            )
        );

        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    pfree(state);
    funcctx->user_fctx = NULL;
    SRF_RETURN_DONE(funcctx);
}
