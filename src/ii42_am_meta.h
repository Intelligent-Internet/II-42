#ifndef II42_AM_META_H
#define II42_AM_META_H

#include "postgres.h"

#include "access/genam.h"
#include "storage/buf.h"
#include "utils/rel.h"

#include "ii42_core.h"
#include "ii42_semantic.h"
#include "ii42_segments.h"

#define II42_AM_MAGIC UINT32_C(0x53323542)
#define II42_AM_VERSION 3
#define II42_AM_PAGE_META 1

#define II42_AM_FLAG_STALE UINT16_C(0x0001)
#define II42_AM_FLAG_CORRUPT UINT16_C(0x0002)
#define II42_AM_FLAG_REBUILD_REQUIRED UINT16_C(0x0004)
#define II42_AM_FLAG_SEMANTIC_QUARANTINE UINT16_C(0x0008)
#define II42_AM_STORAGE_CONVERGENT_SEGMENTS 3

typedef struct ii42_am_meta_page
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
    uint32 delta_data_pages;
    BlockNumber semantic_start_blkno;
    uint32 semantic_data_pages;
    uint32 semantic_record_count;
    uint64 semantic_bytes_len;
    char semantic_signature[II42_AM_SEMANTIC_SIGNATURE_LEN + 1];
    uint8 segment_read_root_bytes[
        II42_SEGMENT_READ_ROOT_SERIALIZED_SIZE
    ];
} ii42_am_meta_page;

typedef struct ii42_am_payload_health_state
{
    bool corrupt;
    bool rebuild_required;
    const char *status;
    const char *reason;
    uint64 expected_bytes;
    uint64 capacity_bytes;
} ii42_am_payload_health_state;

typedef struct ii42_am_convergent_mutation_debt
{
    uint32 upserts;
    uint32 retirements;
    uint32 records;
    uint64 bytes;
} ii42_am_convergent_mutation_debt;

bool ii42_am_meta_uses_convergent_segment_storage(
    const ii42_am_meta_page *meta
);
bool ii42_am_generation_identity_matches(
    const ii42_am_meta_page *cached,
    const ii42_am_meta_page *current
);
void ii42_am_require_convergent_segment_storage(
    const ii42_am_meta_page *meta
);
ii42_status ii42_am_segment_read_root_from_meta(
    const ii42_am_meta_page *meta,
    ii42_segment_read_root *root_out
);
void ii42_am_read_meta(
    Relation indexRelation,
    ii42_am_meta_page *meta_out
);
bool ii42_am_try_read_current_meta(
    Relation indexRelation,
    ii42_am_meta_page *meta_out
);
void ii42_am_note_maintenance_activity(
    Relation indexRelation,
    uint32 pending_write_add,
    uint32 pending_delete_add
);
void ii42_am_mark_stale(Relation indexRelation);
void ii42_am_get_stats(
    Relation indexRelation,
    IndexBulkDeleteResult *stats
);
void ii42_am_lock_generation_barrier(Relation indexRelation);
void ii42_am_unlock_generation_barrier(Relation indexRelation);
void ii42_am_mark_buffer_dirty_with_wal(
    Relation indexRelation,
    Buffer buffer
);
void ii42_am_write_page_at(
    Relation indexRelation,
    BlockNumber blkno,
    const void *contents,
    Size len
);
void ii42_am_write_init_page_at(
    Relation indexRelation,
    BlockNumber blkno,
    const void *contents,
    Size len
);
void ii42_am_write_new_page(
    Relation indexRelation,
    const void *contents,
    Size len
);
uint64 ii42_am_next_rebuild_count(Relation indexRelation);
uint16 ii42_am_next_cache_epoch(Relation indexRelation);
void ii42_am_publish_rebuild_meta(
    Relation indexRelation,
    ForkNumber fork_number,
    const ii42_segment_read_root *root,
    Oid source_type,
    uint32 num_docs,
    uint16 cache_epoch,
    uint16 flags,
    uint64 rebuild_count
);
BlockNumber ii42_am_relation_nblocks(Relation indexRelation);
void ii42_am_payload_health(
    Relation indexRelation,
    const ii42_am_meta_page *meta,
    ii42_am_payload_health_state *health_out
);
uint64 ii42_am_segment_object_bytes_add(uint64 left, uint64 right);
uint64 ii42_am_convergent_object_bytes(
    Relation indexRelation,
    const ii42_am_meta_page *meta
);
void ii42_am_convergent_mutation_debt_read(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_convergent_mutation_debt *debt_out
);
bool ii42_am_meta_has_pending_maintenance(
    const ii42_am_meta_page *meta
);

#endif
