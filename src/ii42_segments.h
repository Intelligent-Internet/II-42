#ifndef II42_SEGMENTS_H
#define II42_SEGMENTS_H

#include <stddef.h>
#include <stdint.h>

#include "ii42_block_ranges.h"
#include "ii42_core.h"
#include "ii42_semantic_bmp.h"

#define II42_SEGMENT_MANIFEST_VERSION UINT16_C(14)
#define II42_SEGMENT_MANIFEST_LEGACY_VERSION UINT16_C(13)
#define II42_SEGMENT_MANIFEST_MAX_SEGMENTS UINT32_C(1024)
#define II42_SEGMENT_MANIFEST_MAX_RETIRED_RANGES UINT32_C(64)
#define II42_TERM_DIRECTORY_LEGACY_MAX_EXTENTS_PER_TERM UINT32_C(32)
#define II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM UINT32_C(64)
#define II42_SEGMENT_CONTRACT_HASH_BYTES 32U
#define II42_DOCUMENT_FINGERPRINT_BYTES 16U
#define II42_SEGMENT_PAGE_HEADER_SIZE 64U
#define II42_ACTIVE_L0_PAGE_HEADER_SIZE 64U
#define II42_L0_RECORD_HEADER_SIZE 112U
#define II42_L0_FRAME_HEADER_SIZE 40U
#define II42_TERM_FOLD_HEADER_SIZE 128U
#define II42_TERM_FOLD_RUN_SIZE 48U
#define II42_SEGMENT_PAYLOAD_HEADER_SIZE 176U
#define II42_SEGMENT_TERM_RUN_SIZE 40U
#define II42_POSTING_BLOCK_RECORD_SIZE 48U
#define II42_ACTIVE_L0_MAX_PAGES UINT32_C(1024)
#define II42_ACTIVE_L0_MAX_RECORDS UINT32_C(131072)
#define II42_ACTIVE_L0_ROTATION_PAGES \
    (II42_ACTIVE_L0_MAX_PAGES / UINT32_C(2))
#define II42_ACTIVE_L0_ROTATION_RECORDS \
    (II42_ACTIVE_L0_MAX_RECORDS / UINT32_C(2))
#define II42_ACTIVE_L0_NO_NEXT_BLOCK UINT32_MAX
#define II42_SEGMENT_READ_ROOT_SERIALIZED_SIZE 200U
#define II42_SEGMENT_SIZE_CLASS_BASE_BYTES UINT64_C(65536)

#define II42_SEGMENT_MANIFEST_FLAG_SAE UINT32_C(0x0001)
#define II42_SEGMENT_MANIFEST_FLAG_TERM_DIRECTORY UINT32_C(0x0002)
#define II42_SEGMENT_MANIFEST_FLAG_NEUTRAL_FOLD UINT32_C(0x0004)
#define II42_SEGMENT_MANIFEST_FLAG_IMPACT_FOLD UINT32_C(0x0008)
#define II42_SEGMENT_MANIFEST_FLAG_COW_TERM_DIRECTORY UINT32_C(0x0010)
#define II42_SEGMENT_MANIFEST_FLAG_DOCUMENT_DIRECTORY UINT32_C(0x0020)
#define II42_SEGMENT_MANIFEST_FLAG_LEXICON_LOOKUP UINT32_C(0x0040)
#define II42_SEGMENT_MANIFEST_FLAG_PREFIX_LOOKUP UINT32_C(0x0080)
#define II42_SEGMENT_MANIFEST_FLAG_SEMANTIC_ACCELERATOR \
    UINT32_C(0x0100)

#define II42_SEGMENT_FLAG_SEALED UINT32_C(0x0002)
#define II42_SEGMENT_FLAG_LEXICAL UINT32_C(0x0004)
#define II42_SEGMENT_FLAG_SEMANTIC UINT32_C(0x0008)
#define II42_SEGMENT_FLAG_PENDING UINT32_C(0x0010)
#define II42_SEGMENT_FLAG_RETIREMENTS UINT32_C(0x0020)
#define II42_SEGMENT_FLAG_QUARANTINE UINT32_C(0x0040)
#define II42_SEGMENT_FLAG_HISTORY_BARRIER UINT32_C(0x0080)

#define II42_DOCUMENT_VERSION_FLAG_SEMANTIC_PENDING UINT16_C(0x0001)
#define II42_DOCUMENT_VERSION_FLAG_SEMANTIC_COMPLETE UINT16_C(0x0002)
#define II42_DOCUMENT_VERSION_FLAG_SEMANTIC_QUARANTINED UINT16_C(0x0004)
#define II42_DOCUMENT_VERSION_FLAG_FROZEN_XID UINT16_C(0x0008)
#define II42_DOCUMENT_VERSION_FLAG_ABORTED_HOLE UINT16_C(0x0010)
/* COW state is authoritative while the matching version remains in L0. */
#define II42_DOCUMENT_VERSION_FLAG_L0_OWNED UINT16_C(0x0020)

#define II42_DOCUMENT_RETIREMENT_FLAG_FROZEN_XID UINT16_C(0x0001)

#define II42_SEMANTIC_STATE_FLAG_COMPLETE UINT16_C(0x0001)
#define II42_SEMANTIC_STATE_FLAG_QUARANTINED UINT16_C(0x0002)
#define II42_SEMANTIC_STATE_FLAG_FROZEN_XID UINT16_C(0x0004)

#define II42_SEGMENT_PAYLOAD_FLAG_DOCUMENT_MAP UINT32_C(0x0001)
#define II42_QUERY_CONTRACT_FLAG_VOCABULARY UINT32_C(0x0001)
#define II42_QUERY_CONTRACT_FLAG_EMPTY_TOKEN UINT32_C(0x0002)

#define II42_L0_FRAME_FLAG_START UINT16_C(0x0001)
#define II42_L0_FRAME_FLAG_END UINT16_C(0x0002)

typedef enum ii42_segment_object_kind
{
    II42_SEGMENT_OBJECT_INVALID = 0,
    II42_SEGMENT_OBJECT_MANIFEST = 1,
    II42_SEGMENT_OBJECT_TERM_DIRECTORY = 2,
    II42_SEGMENT_OBJECT_PAYLOAD = 3,
    II42_SEGMENT_OBJECT_NEUTRAL_FOLD = 4,
    II42_SEGMENT_OBJECT_IMPACT_FOLD = 5,
    II42_SEGMENT_OBJECT_QUERY_CONTRACT = 6,
    II42_SEGMENT_OBJECT_LEXICAL_CATALOG = 7,
    II42_SEGMENT_OBJECT_DOCUMENT_DIRECTORY = 8,
    II42_SEGMENT_OBJECT_LEXICON_LOOKUP = 9,
    II42_SEGMENT_OBJECT_PREFIX_LOOKUP = 10,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_DIRECTORY = 11,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_TERM = 12,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD = 13,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_SCOPE = 14,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_TID_LOOKUP = 15,
    II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD_BOUND = 16
} ii42_segment_object_kind;

typedef enum ii42_l0_record_kind
{
    II42_L0_RECORD_INVALID = 0,
    II42_L0_RECORD_UPSERT = 1,
    II42_L0_RECORD_RETIRE = 2,
    II42_L0_RECORD_SEMANTIC_COMPLETE = 3,
    II42_L0_RECORD_SEMANTIC_QUARANTINE = 4
} ii42_l0_record_kind;

typedef enum ii42_l0_term_encoding
{
    II42_L0_TERM_ENCODING_NONE = 0,
    II42_L0_TERM_ENCODING_NUMERIC = 1,
    II42_L0_TERM_ENCODING_UTF8 = 2
} ii42_l0_term_encoding;

/*
 * Immutable relation objects use one checksummed envelope per PostgreSQL
 * page. All pages repeat the object identity so orphaned or cross-linked
 * chains fail closed before an object codec sees their bytes.
 */
typedef struct ii42_segment_page_header
{
    ii42_segment_object_kind object_kind;
    uint64_t object_id;
    uint64_t owner_manifest_id;
    uint64_t object_bytes;
    uint64_t object_checksum;
    uint32_t ordinal;
    uint32_t page_count;
    uint32_t used_bytes;
    uint32_t payload_checksum;
} ii42_segment_page_header;

typedef struct ii42_segment_object_ref
{
    ii42_segment_object_kind object_kind;
    uint32_t start_block;
    uint32_t page_count;
    uint64_t object_id;
    uint64_t owner_manifest_id;
    uint64_t object_bytes;
    uint64_t object_checksum;
} ii42_segment_object_ref;

typedef struct ii42_active_l0_page_header
{
    uint64_t segment_id;
    uint64_t min_sequence;
    uint64_t max_sequence;
    uint64_t payload_checksum;
    uint32_t ordinal;
    uint32_t next_block;
    uint32_t frame_count;
    uint32_t used_bytes;
} ii42_active_l0_page_header;

typedef struct ii42_active_l0_frontier
{
    uint64_t segment_id;
    uint64_t min_sequence;
    uint64_t max_sequence;
    uint64_t payload_bytes;
    uint32_t head_block;
    uint32_t tail_block;
    uint32_t page_count;
    uint32_t record_count;
} ii42_active_l0_frontier;

bool ii42_segment_object_ref_equal(
    const ii42_segment_object_ref *left,
    const ii42_segment_object_ref *right
);
bool ii42_active_l0_frontier_equal(
    const ii42_active_l0_frontier *left,
    const ii42_active_l0_frontier *right
);

typedef struct ii42_segment_read_root
{
    uint64_t root_id;
    uint64_t next_sequence;
    uint64_t next_document_slot;
    uint64_t next_segment_id;
    uint32_t published_block_high_watermark;
    /* Lowest COW slot not already claimed by either linked L0 frontier. */
    uint32_t reusable_document_slot_cursor;
    ii42_segment_object_ref manifest;
    ii42_active_l0_frontier active_l0;
    ii42_active_l0_frontier pending_l0;
} ii42_segment_read_root;

typedef struct ii42_l0_lexical_atom
{
    uint32_t term_id;
    uint32_t term_frequency;
    const uint8_t *term_bytes;
    uint32_t term_bytes_len;
} ii42_l0_lexical_atom;

typedef struct ii42_l0_semantic_atom
{
    uint32_t term_id;
    float impact;
} ii42_l0_semantic_atom;

typedef struct ii42_segment_semantic_posting
{
    uint32_t term_id;
    uint32_t document_slot;
    float impact;
} ii42_segment_semantic_posting;

typedef ii42_status (*ii42_segment_semantic_posting_reader)(
    void *context,
    ii42_segment_semantic_posting *posting_out
);

typedef ii42_status (*ii42_segment_semantic_posting_rewind)(
    void *context
);

typedef struct ii42_initial_fold_stream ii42_initial_fold_stream;

typedef struct ii42_l0_record
{
    ii42_l0_record_kind kind;
    ii42_l0_term_encoding term_encoding;
    uint16_t flags;
    uint64_t sequence;
    uint64_t document_slot;
    uint32_t record_xid;
    uint32_t heap_block;
    uint16_t heap_offset;
    uint16_t semantic_failure_count;
    uint32_t document_length;
    uint32_t semantic_error_code;
    int64_t semantic_retry_after;
    int64_t semantic_pending_since;
    uint64_t semantic_error_hash;
    const uint8_t *semantic_input_fingerprint;
    const ii42_l0_lexical_atom *atoms;
    const ii42_l0_semantic_atom *semantic_atoms;
    uint32_t atom_count;
} ii42_l0_record;

typedef struct ii42_l0_record_view
{
    ii42_l0_record_kind kind;
    ii42_l0_term_encoding term_encoding;
    uint16_t flags;
    uint64_t sequence;
    uint64_t document_slot;
    uint32_t record_xid;
    uint32_t heap_block;
    uint16_t heap_offset;
    uint16_t semantic_failure_count;
    uint32_t document_length;
    uint32_t semantic_error_code;
    int64_t semantic_retry_after;
    int64_t semantic_pending_since;
    uint64_t semantic_error_hash;
    uint32_t atom_count;
    uint8_t semantic_input_fingerprint[
        II42_DOCUMENT_FINGERPRINT_BYTES
    ];
    const uint8_t *payload;
    size_t payload_size;
} ii42_l0_record_view;

typedef struct ii42_l0_frame_header
{
    uint16_t flags;
    uint64_t sequence;
    uint64_t record_checksum;
    uint32_t record_bytes;
    uint32_t fragment_offset;
    uint32_t fragment_bytes;
} ii42_l0_frame_header;

typedef struct ii42_segment_descriptor
{
    uint64_t segment_id;
    uint64_t min_sequence;
    uint64_t max_sequence;
    uint64_t posting_count;
    uint64_t retirement_count;
    uint64_t document_count;
    uint64_t total_document_length;
    uint64_t first_document_slot;
    uint64_t document_slot_count;
    uint64_t payload_checksum;
    uint32_t start_block;
    uint32_t block_count;
    uint32_t size_class;
    uint32_t flags;
    uint64_t payload_bytes;
    uint64_t payload_owner_manifest_id;
    uint64_t semantic_state_count;
} ii42_segment_descriptor;

typedef struct ii42_segment_manifest
{
    uint32_t flags;
    uint64_t manifest_id;
    uint64_t parent_manifest_id;
    uint64_t max_sequence;
    uint64_t statistics_epoch;
    uint64_t visible_document_count;
    uint64_t document_slot_count;
    uint64_t total_document_length;
    uint64_t reclaim_before_sequence;
    uint64_t neutral_fold_coverage;
    uint64_t impact_fold_coverage;
    uint64_t impact_statistics_epoch;
    uint32_t vocab_size;
    ii42_segment_object_ref query_contract;
    ii42_segment_object_ref term_directory;
    ii42_segment_object_ref neutral_fold;
    ii42_segment_object_ref impact_fold;
    ii42_segment_object_ref document_directory;
    ii42_segment_object_ref lexicon_lookup;
    ii42_segment_object_ref prefix_lookup;
    ii42_segment_object_ref semantic_accelerator_directory;
    /*
     * Logical frontier covered by the inherited accelerator. Version 13
     * reserved these final header bytes, so zero remains the legacy encoding
     * for an accelerator built from this manifest's exact authority.
     */
    uint64_t semantic_accelerator_max_sequence;
    uint64_t lexicon_hash_seed;
    uint8_t contract_hash[II42_SEGMENT_CONTRACT_HASH_BYTES];
    ii42_block_range *retired_ranges;
    uint32_t retired_range_count;
    ii42_segment_descriptor *segments;
    uint32_t segment_count;
    uint32_t *doc_frequencies;
} ii42_segment_manifest;

typedef struct ii42_term_extent_descriptor
{
    uint32_t segment_index;
    ii42_posting_extent_kind kind;
    uint64_t posting_offset;
    uint64_t posting_count;
} ii42_term_extent_descriptor;

typedef struct ii42_segment_query_contract
{
    ii42_params params;
    uint32_t vocab_size;
    uint32_t flags;
    uint32_t empty_token_id;
    uint32_t block_shift;
    char **vocab;
} ii42_segment_query_contract;

/*
 * One immutable, contiguous term-id range. Descendant manifests append a new
 * catalog object and bind only newly admitted COW term records to it.
 */
typedef struct ii42_lexical_catalog
{
    uint64_t owner_manifest_id;
    uint32_t first_term_id;
    uint32_t term_count;
    char **terms;
} ii42_lexical_catalog;

typedef struct ii42_term_directory
{
    uint32_t vocab_size;
    uint32_t extent_count;
    uint64_t *term_offsets;
    ii42_term_extent_descriptor *extents;
} ii42_term_directory;

typedef struct ii42_segment_term_run
{
    uint32_t term_id;
    ii42_posting_extent_kind kind;
    uint64_t posting_offset;
    uint64_t posting_count;
    uint64_t block_offset;
    uint32_t block_count;
} ii42_segment_term_run;

/*
 * One persistent, term-local fold run. Coverage is a complete immutable
 * extent boundary; all postings in the run represent the reduced prefix
 * through that sequence.
 */
typedef struct ii42_term_fold_run
{
    uint32_t term_id;
    ii42_posting_extent_kind kind;
    uint64_t coverage_sequence;
    uint64_t posting_offset;
    uint64_t posting_count;
    uint64_t block_offset;
    uint32_t block_count;
} ii42_term_fold_run;

/*
 * Several independently covered term prefixes may share one immutable object
 * to amortize page and WAL overhead. Neutral bundles preserve lexical TF and
 * semantic impacts. Impact bundles contain only epoch-bound lexical impacts.
 */
typedef struct ii42_term_fold_bundle
{
    ii42_segment_object_kind object_kind;
    uint64_t owner_manifest_id;
    uint64_t statistics_epoch;
    uint32_t run_count;
    uint32_t block_shift;
    uint32_t block_count;
    uint64_t posting_count;
    ii42_term_fold_run *runs;
    ii42_posting_block_record *blocks;
    uint32_t *document_slots;
    ii42_posting_value *values;
    ii42_semantic_impact_precision semantic_impact_precision;
} ii42_term_fold_bundle;

/*
 * Cold attach resolves one COW term record into a bounded slice of one
 * validated neutral fold bundle. The read view owns neither the bundle nor
 * this plan.
 */
typedef struct ii42_term_fold_read_plan
{
    uint64_t neutral_coverage;
    uint32_t neutral_bundle_index;
    uint32_t neutral_first_run;
    uint32_t neutral_run_count;
    uint64_t neutral_minor_coverage;
    uint32_t neutral_minor_bundle_index;
    uint32_t neutral_minor_first_run;
    uint32_t neutral_minor_run_count;
    uint64_t impact_coverage;
    uint64_t impact_statistics_epoch;
    uint32_t impact_bundle_index;
    uint32_t impact_first_run;
    uint32_t impact_run_count;
} ii42_term_fold_read_plan;

/*
 * A document slot identifies one immutable heap tuple version while that
 * incarnation remains reachable. UPDATE retires the old incarnation; a fully
 * drained retired slot may later be reused with a strictly newer born sequence.
 */
typedef struct ii42_document_version_record
{
    uint64_t document_slot;
    uint64_t born_sequence;
    uint32_t record_xid;
    uint32_t heap_block;
    uint32_t document_length;
    uint16_t heap_offset;
    uint16_t flags;
    uint8_t semantic_input_fingerprint[
        II42_DOCUMENT_FINGERPRINT_BYTES
    ];
} ii42_document_version_record;

typedef struct ii42_document_retirement_record
{
    uint64_t document_slot;
    uint64_t retirement_sequence;
    uint32_t record_xid;
    uint32_t document_length;
    uint16_t flags;
    uint16_t reserved;
    uint32_t reserved2;
} ii42_document_retirement_record;

typedef struct ii42_semantic_state_record
{
    uint64_t document_slot;
    uint64_t transition_sequence;
    uint32_t record_xid;
    uint32_t error_code;
    int64_t retry_after;
    int64_t pending_since;
    uint64_t error_hash;
    uint16_t flags;
    uint16_t failure_count;
    uint32_t reserved;
    uint8_t semantic_input_fingerprint[
        II42_DOCUMENT_FINGERPRINT_BYTES
    ];
} ii42_semantic_state_record;

typedef struct ii42_segment_payload
{
    uint64_t segment_id;
    uint32_t vocab_size;
    uint32_t flags;
    uint32_t document_id_base;
    uint32_t local_document_count;
    uint32_t run_count;
    uint32_t block_shift;
    uint32_t block_count;
    uint64_t posting_count;
    ii42_segment_term_run *runs;
    ii42_posting_block_record *blocks;
    uint32_t *indices;
    ii42_posting_value *values;
    uint32_t *document_id_map;
    ii42_document_version_record *versions;
    uint32_t version_count;
    ii42_document_retirement_record *retirements;
    uint32_t retirement_count;
    ii42_semantic_state_record *semantic_states;
    uint32_t semantic_state_count;
    ii42_semantic_impact_precision semantic_impact_precision;
} ii42_segment_payload;

typedef struct ii42_segment_payload_view
{
    uint64_t segment_id;
    uint64_t posting_count;
    const ii42_segment_term_run *runs;
    uint32_t run_count;
    uint32_t block_shift;
    uint32_t block_count;
    const ii42_posting_block_record *blocks;
    const ii42_posting_value *values;
    const float *data;
    const uint32_t *indices;
    const uint32_t *term_frequencies;
    const uint32_t *document_id_map;
    uint32_t document_id_base;
    uint32_t local_document_count;
} ii42_segment_payload_view;

/*
 * Checked fixed-width metadata used by page-native readers. These structures
 * describe serialized offsets only; they never own posting payload memory.
 */
typedef struct ii42_segment_payload_disk_header
{
    uint16_t payload_version;
    uint16_t semantic_bmp_version;
    uint32_t flags;
    uint32_t vocab_size;
    uint32_t local_document_count;
    uint32_t run_count;
    uint64_t segment_id;
    uint64_t posting_count;
    uint32_t version_count;
    uint32_t retirement_count;
    uint32_t document_id_base;
    uint32_t semantic_state_count;
    uint32_t block_count;
    uint32_t block_shift;
    uint64_t generic_posting_count;
    uint64_t runs_offset;
    uint64_t blocks_offset;
    uint64_t indices_offset;
    uint64_t values_offset;
    uint64_t document_map_offset;
    uint64_t versions_offset;
    uint64_t retirements_offset;
    uint64_t semantic_states_offset;
    uint64_t semantic_bmp_offset;
    uint64_t semantic_bmp_size;
    uint64_t total_size;
    uint64_t internal_checksum;
} ii42_segment_payload_disk_header;

typedef struct ii42_term_fold_disk_header
{
    uint16_t format_version;
    ii42_segment_object_kind object_kind;
    uint32_t run_count;
    uint32_t block_count;
    uint64_t owner_manifest_id;
    uint64_t statistics_epoch;
    uint64_t posting_count;
    uint64_t generic_posting_count;
    uint32_t block_shift;
    uint64_t runs_offset;
    uint64_t blocks_offset;
    uint64_t document_slots_offset;
    uint64_t values_offset;
    uint32_t semantic_bmp_version;
    uint64_t semantic_bmp_offset;
    uint64_t semantic_bmp_size;
    uint64_t total_size;
    uint64_t internal_checksum;
} ii42_term_fold_disk_header;

typedef struct ii42_segment_read_view
{
    ii42_term_extent_list *terms;
    ii42_posting_extent *extents;
    uint32_t vocab_size;
    uint32_t extent_count;
} ii42_segment_read_view;

uint64_t ii42_segment_blob_checksum(const uint8_t *bytes, size_t size);

uint32_t ii42_segment_page_payload_checksum(
    const uint8_t *bytes,
    size_t size
);

ii42_status ii42_segment_page_count_required(
    size_t object_bytes,
    size_t page_content_bytes,
    uint32_t *page_count_out
);

ii42_status ii42_segment_page_header_serialize(
    const ii42_segment_page_header *header,
    size_t page_content_bytes,
    uint8_t *bytes_out,
    size_t size_out
);

ii42_status ii42_segment_page_header_deserialize(
    const uint8_t *bytes,
    size_t size,
    size_t page_content_bytes,
    ii42_segment_page_header *header_out
);

ii42_status ii42_segment_page_payload_validate(
    const ii42_segment_page_header *header,
    const uint8_t *payload,
    size_t payload_size
);

ii42_status ii42_active_l0_page_header_serialize(
    const ii42_active_l0_page_header *header,
    size_t page_content_bytes,
    uint8_t *bytes_out,
    size_t size_out
);

ii42_status ii42_active_l0_page_header_deserialize(
    const uint8_t *bytes,
    size_t size,
    size_t page_content_bytes,
    ii42_active_l0_page_header *header_out
);

ii42_status ii42_active_l0_page_payload_validate(
    const ii42_active_l0_page_header *header,
    const uint8_t *payload,
    size_t payload_size
);

ii42_status ii42_active_l0_frontier_validate(
    const ii42_active_l0_frontier *frontier,
    bool allow_absent
);

static inline bool
ii42_document_fingerprint_is_zero(const uint8_t *fingerprint)
{
    size_t index;

    if (fingerprint == NULL)
    {
        return true;
    }
    for (index = 0; index < II42_DOCUMENT_FINGERPRINT_BYTES; index++)
    {
        if (fingerprint[index] != 0)
        {
            return false;
        }
    }
    return true;
}

ii42_status ii42_l0_record_serialized_size(
    const ii42_l0_record *record,
    size_t *size_out
);

ii42_status ii42_l0_record_serialize(
    const ii42_l0_record *record,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_l0_record_view_parse(
    const uint8_t *bytes,
    size_t size,
    ii42_l0_record_view *view_out
);

ii42_status ii42_l0_record_view_atom(
    const ii42_l0_record_view *view,
    uint32_t atom_index,
    ii42_l0_lexical_atom *atom_out
);

ii42_status ii42_l0_record_view_decode_atoms(
    const ii42_l0_record_view *view,
    ii42_l0_lexical_atom *atoms_out,
    size_t atom_capacity
);

ii42_status ii42_l0_record_view_semantic_atom(
    const ii42_l0_record_view *view,
    uint32_t atom_index,
    ii42_l0_semantic_atom *atom_out
);

ii42_status ii42_l0_record_view_decode_semantic_atoms(
    const ii42_l0_record_view *view,
    ii42_l0_semantic_atom *atoms_out,
    size_t atom_capacity
);

ii42_status ii42_l0_frame_header_serialize(
    const ii42_l0_frame_header *header,
    uint8_t *bytes_out,
    size_t size_out
);

ii42_status ii42_l0_frame_header_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_l0_frame_header *header_out
);

ii42_status ii42_segment_object_ref_validate(
    const ii42_segment_object_ref *ref,
    uint32_t published_block_high_watermark
);

ii42_status ii42_segment_read_root_validate(
    const ii42_segment_read_root *root
);

ii42_status ii42_segment_read_root_rotate_l0(
    ii42_segment_read_root *root
);

/*
 * Publish one already-written descendant manifest that seals the complete
 * pending frontier. The active frontier is preserved byte-for-byte.
 */
ii42_status ii42_segment_read_root_seal_pending(
    ii42_segment_read_root *root,
    const ii42_segment_object_ref *manifest_ref,
    uint32_t published_block_high_watermark
);

/*
 * Publish a physical-only descendant manifest while preserving both linked
 * L0 frontiers byte-for-byte.
 */
ii42_status ii42_segment_read_root_replace_manifest(
    ii42_segment_read_root *root,
    const ii42_segment_object_ref *manifest_ref,
    uint32_t published_block_high_watermark
);

ii42_status ii42_segment_read_root_serialize(
    const ii42_segment_read_root *root,
    uint8_t *bytes_out,
    size_t size_out
);

ii42_status ii42_segment_read_root_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_segment_read_root *root_out
);

void ii42_segment_manifest_init(ii42_segment_manifest *manifest);
void ii42_segment_manifest_free(ii42_segment_manifest *manifest);

ii42_status ii42_segment_manifest_build_identity(
    const ii42_segment_read_root *build_root,
    const ii42_segment_manifest *old_manifest,
    ii42_segment_manifest *next_manifest
);

/*
 * Fingerprint the exact query authority while excluding manifest identity and
 * derived accelerators. A derived-only child must retain this value exactly.
 */
ii42_status ii42_segment_manifest_authority_checksum(
    const ii42_segment_manifest *manifest,
    uint64_t *checksum_out
);

/*
 * A published derived accelerator remains query-visible as an immutable
 * baseline. Linked L0 stays authoritative but does not invalidate the baseline
 * merely by being non-empty. Stale-baseline queries revalidate returned rows;
 * low-volume post-baseline additions may wait for the next derived refresh.
 */
bool ii42_segment_manifest_semantic_accelerator_eligible(
    const ii42_segment_read_root *root,
    const ii42_segment_manifest *manifest
);

uint64_t ii42_segment_manifest_semantic_accelerator_baseline_sequence(
    const ii42_segment_manifest *manifest
);

bool ii42_segment_manifest_semantic_accelerator_published(
    const ii42_segment_read_root *root,
    const ii42_segment_manifest *manifest
);

ii42_status ii42_segment_manifest_validate(
    const ii42_segment_manifest *manifest
);

bool ii42_segment_descriptor_equal(
    const ii42_segment_descriptor *left,
    const ii42_segment_descriptor *right
);

ii42_status ii42_segment_descriptor_payload_ref(
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    ii42_segment_object_ref *ref_out
);

ii42_status ii42_segment_manifest_validate_published(
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *manifest_ref,
    uint32_t published_block_high_watermark
);

ii42_status ii42_segment_manifest_serialized_size(
    const ii42_segment_manifest *manifest,
    size_t *size_out
);

ii42_status ii42_segment_manifest_serialize(
    const ii42_segment_manifest *manifest,
    uint8_t **bytes_out,
    size_t *size_out
);

/*
 * manifest_out must be initialized with ii42_segment_manifest_init().
 * A failed decode leaves the existing output unchanged.
 */
ii42_status ii42_segment_manifest_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_segment_manifest *manifest_out
);

void ii42_segment_query_contract_init(
    ii42_segment_query_contract *contract
);
void ii42_segment_query_contract_free(
    ii42_segment_query_contract *contract
);

ii42_status ii42_segment_query_contract_build(
    const ii42_index *index,
    const ii42_segment_manifest *manifest,
    ii42_segment_query_contract *contract_out
);

ii42_status ii42_segment_query_contract_validate(
    const ii42_segment_query_contract *contract,
    const ii42_segment_manifest *manifest
);

ii42_status ii42_segment_query_contract_serialize(
    const ii42_segment_query_contract *contract,
    const ii42_segment_manifest *manifest,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_segment_query_contract_deserialize(
    const uint8_t *bytes,
    size_t size,
    const ii42_segment_manifest *manifest,
    ii42_segment_query_contract *contract_out
);

void ii42_lexical_catalog_init(ii42_lexical_catalog *catalog);
void ii42_lexical_catalog_free(ii42_lexical_catalog *catalog);

ii42_status ii42_lexical_catalog_build(
    uint64_t owner_manifest_id,
    uint32_t first_term_id,
    uint32_t term_count,
    const char *const *terms,
    ii42_lexical_catalog *catalog_out
);

ii42_status ii42_lexical_catalog_validate(
    const ii42_lexical_catalog *catalog
);

ii42_status ii42_lexical_catalog_serialize(
    const ii42_lexical_catalog *catalog,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_lexical_catalog_deserialize(
    const uint8_t *bytes,
    size_t size,
    uint64_t expected_owner_manifest_id,
    ii42_lexical_catalog *catalog_out
);

void ii42_term_directory_init(ii42_term_directory *directory);
void ii42_term_directory_free(ii42_term_directory *directory);

ii42_status ii42_term_directory_validate_logical(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest
);

ii42_status ii42_term_directory_validate(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest
);

/*
 * Sizing and serialization validate logical segment references but permit the
 * directory's own object ref to be absent until native page publication.
 */
ii42_status ii42_term_directory_serialized_size(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    size_t *size_out
);

ii42_status ii42_term_directory_serialize(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    uint8_t **bytes_out,
    size_t *size_out
);

/*
 * directory_out must be initialized with ii42_term_directory_init().
 * A failed decode leaves the existing output unchanged.
 */
ii42_status ii42_term_directory_deserialize(
    const uint8_t *bytes,
    size_t size,
    const ii42_segment_manifest *manifest,
    ii42_term_directory *directory_out
);

ii42_status ii42_document_version_records_validate(
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    const ii42_document_version_record *versions,
    size_t version_count,
    const ii42_document_retirement_record *retirements,
    size_t retirement_count
);

ii42_status ii42_semantic_state_records_validate(
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    const ii42_semantic_state_record *states,
    size_t state_count
);

/*
 * Collect the immutable, globally visible retirement set without reading any
 * posting run. Every retirement must already be frozen at PostgreSQL's safe
 * snapshot horizon. The caller owns the returned malloc allocation.
 */
ii42_status ii42_segment_frozen_retirement_ids_build(
    const ii42_segment_manifest *manifest,
    const ii42_segment_payload *payloads,
    size_t payload_count,
    uint32_t **document_ids_out,
    size_t *document_id_count_out
);

ii42_status ii42_segment_payload_view_validate(
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    const ii42_segment_payload_view *payload,
    const ii42_document_version_record *versions,
    size_t version_count,
    const ii42_document_retirement_record *retirements,
    size_t retirement_count
);

ii42_status ii42_term_directory_build_from_payloads(
    const ii42_segment_manifest *manifest,
    const ii42_segment_payload_view *payloads,
    size_t payload_count,
    ii42_term_directory *directory_out
);

/*
 * Build the next directory from one validated immutable directory plus the
 * newly appended segment. Unchanged payloads are not required or read.
 */
ii42_status ii42_term_directory_append_payload(
    const ii42_term_directory *old_directory,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_payload_view *new_payload,
    ii42_term_directory *directory_out
);

/*
 * Replace one contiguous descriptor range with one merged payload. Unchanged
 * directory extents are remapped without opening their payloads.
 */
ii42_status ii42_term_directory_replace_payloads(
    const ii42_term_directory *old_directory,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    uint32_t first_segment_index,
    uint32_t replaced_segment_count,
    const ii42_segment_payload_view *replacement_payload,
    ii42_term_directory *directory_out
);

void ii42_term_fold_bundle_init(ii42_term_fold_bundle *bundle);
void ii42_term_fold_bundle_free(ii42_term_fold_bundle *bundle);

ii42_status ii42_term_fold_bundle_validate(
    const ii42_term_fold_bundle *bundle
);

/*
 * Advance one exact neutral fold from already-decoded term-local extents.
 * Every extent may use local document ids plus a map/base, but must contain
 * postings for term_id only. This lets page-native maintenance read one term
 * without materializing the complete source segment payload. A prior coverage
 * that compaction absorbed inside a wider segment is accepted only when the
 * caller supplies authority from the validated durable term COW record.
 */
ii42_status ii42_term_fold_bundle_advance_neutral_extents(
    const ii42_segment_manifest *manifest,
    uint32_t term_id,
    const ii42_posting_extent *tail_extents,
    size_t tail_extent_count,
    const ii42_term_fold_bundle *prior_bundle,
    uint64_t prior_coverage_sequence,
    bool prior_coverage_is_fold_boundary,
    uint64_t owner_manifest_id,
    uint64_t coverage_sequence,
    ii42_term_fold_bundle *bundle_out
);

ii42_status ii42_term_fold_bundle_serialize(
    const ii42_term_fold_bundle *bundle,
    uint8_t **bytes_out,
    size_t *size_out,
    uint64_t *checksum_out
);

/*
 * Compile one previously-unfolded term prefix without scanning unrelated
 * terms. coverage_sequence must be a complete immutable segment boundary.
 * The output contains exactly the lexical-neutral and/or semantic-impact
 * postings consumed from directory; lexical-impact input is rejected.
 */
ii42_status ii42_term_fold_bundle_build_neutral_prefix(
    const ii42_segment_manifest *manifest,
    const ii42_term_directory *directory,
    const ii42_segment_payload_view *payloads,
    size_t payload_count,
    uint64_t owner_manifest_id,
    uint32_t term_id,
    uint64_t coverage_sequence,
    ii42_term_fold_bundle *bundle_out
);

/*
 * Advance one term fold from an optional prior bundle by consuming only
 * post-coverage tail extents. When prior_bundle is NULL and
 * prior_coverage_sequence is nonzero, the output is the disjoint interval
 * (prior_coverage_sequence, coverage_sequence]. payload_segment_indices maps
 * each supplied payload view to its immutable manifest segment. The caller
 * may therefore load only segments touched by the selected term instead of
 * reopening the complete corpus. bundle_out must be initialized.
 */
ii42_status ii42_term_fold_bundle_advance_neutral(
    const ii42_segment_manifest *manifest,
    uint32_t term_id,
    const ii42_term_extent_descriptor *tail_extents,
    size_t tail_extent_count,
    const ii42_segment_payload_view *payloads,
    const uint32_t *payload_segment_indices,
    size_t payload_count,
    const ii42_term_fold_bundle *prior_bundle,
    uint64_t prior_coverage_sequence,
    uint64_t owner_manifest_id,
    uint64_t coverage_sequence,
    ii42_term_fold_bundle *bundle_out
);

/*
 * Promote two adjacent exact neutral-fold intervals into one term-local major
 * fold without reopening any source segment payload. Both source bundles may
 * contain runs for other terms; only term_id is copied. bundle_out must be
 * initialized.
 */
ii42_status ii42_term_fold_bundle_merge_neutral(
    const ii42_segment_manifest *manifest,
    uint32_t term_id,
    const ii42_term_fold_bundle *major_bundle,
    const ii42_term_fold_bundle *minor_bundle,
    uint64_t owner_manifest_id,
    ii42_term_fold_bundle *bundle_out
);

/*
 * Compile the lexical-neutral runs of one complete effective neutral prefix
 * into exact impacts for one corpus-statistics epoch. Semantic runs remain in
 * the neutral source bundles and are not copied. live_document_frequency must
 * already include the complete visible tail and retirement correction.
 */
ii42_status ii42_term_fold_bundle_build_impact(
    const ii42_index *global_index,
    const ii42_corpus_stats *stats,
    uint32_t term_id,
    uint32_t live_document_frequency,
    const ii42_term_fold_bundle *major_bundle,
    const ii42_term_fold_bundle *minor_bundle,
    uint64_t owner_manifest_id,
    uint64_t statistics_epoch,
    ii42_term_fold_bundle *bundle_out
);

/*
 * bundle_out must be initialized with ii42_term_fold_bundle_init().
 * A failed decode leaves the existing output unchanged.
 */
ii42_status ii42_term_fold_bundle_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_term_fold_bundle *bundle_out
);

/*
 * Decode fixed-width metadata after the caller has checked the containing
 * immutable page envelope. Full-object deserializers remain responsible for
 * recomputing the end-to-end object checksum during scrub and maintenance.
 */
ii42_status ii42_term_fold_disk_header_decode(
    const uint8_t *bytes,
    size_t size,
    ii42_term_fold_disk_header *header_out
);

ii42_status ii42_term_fold_run_decode(
    const uint8_t *bytes,
    size_t size,
    ii42_term_fold_run *run_out
);

void ii42_segment_payload_init(ii42_segment_payload *payload);
void ii42_segment_payload_free(ii42_segment_payload *payload);

/*
 * Transfer one contiguous rebuild payload into bounded immutable segments.
 * The source must describe a contiguous document-slot range without mutable
 * retirements or semantic-state records. On success payload is empty and the
 * caller owns payloads_out; each output serializes to at most max_bytes.
 */
ii42_status ii42_segment_payload_partition_contiguous(
    ii42_segment_payload *payload,
    uint64_t first_segment_id,
    size_t max_bytes,
    ii42_segment_payload **payloads_out,
    uint32_t *payload_count_out
);

uint32_t ii42_segment_size_class(uint64_t payload_bytes);

/*
 * Convert one contiguous lexical index into a statistics-neutral canonical
 * segment. versions must describe the same contiguous document-slot range and
 * document lengths as index. payload_out must be initialized.
 */
ii42_status ii42_segment_payload_build_lexical(
    const ii42_index *index,
    uint64_t segment_id,
    uint32_t document_id_base,
    const ii42_document_version_record *versions,
    size_t version_count,
    ii42_segment_payload *payload_out
);

/*
 * Build the same canonical lexical payload directly from sparse global term
 * ids. Unlike ii42_segment_payload_build_lexical(), this path allocates only
 * in proportion to the changed postings and touched term runs; it never
 * materializes arrays sized to the complete vocabulary. entries may arrive in
 * any order. payload_out must be initialized.
 */
ii42_status ii42_segment_payload_build_lexical_entries(
    const ii42_term_entry *entries,
    size_t entry_count,
    uint32_t vocab_size,
    uint64_t segment_id,
    uint32_t document_id_base,
    const ii42_document_version_record *versions,
    size_t version_count,
    ii42_segment_payload *payload_out
);

/* Build a canonical lexical payload over sorted root-relative slots. */
ii42_status ii42_segment_payload_build_lexical_mapped_entries(
    const ii42_term_entry *entries,
    size_t entry_count,
    uint32_t vocab_size,
    uint64_t segment_id,
    const uint32_t *document_id_map,
    const ii42_document_version_record *versions,
    size_t version_count,
    ii42_segment_payload *payload_out
);

/*
 * Add semantic postings and state transitions to one lexical payload. Global
 * document slots are compiled into one sorted document map, and lexical and
 * semantic runs remain one canonical (term, kind) stream.
 */
ii42_status ii42_segment_payload_attach_semantic(
    ii42_segment_payload *payload,
    const ii42_segment_semantic_posting *postings,
    size_t posting_count,
    const ii42_semantic_state_record *states,
    size_t state_count
);

/*
 * Attach a term/document-sorted semantic stream without materializing a
 * second sortable posting array. The reader must return exactly posting_count
 * records after every successful rewind.
 */
ii42_status ii42_segment_payload_attach_semantic_sorted_reader(
    ii42_segment_payload *payload,
    size_t posting_count,
    ii42_segment_semantic_posting_reader reader,
    ii42_segment_semantic_posting_rewind rewind,
    void *reader_context,
    const ii42_semantic_state_record *states,
    size_t state_count
);

/*
 * Merge one canonical lexical index and a term/document-sorted semantic
 * reader into bounded, complete term-fold bundles. The stream retains at most
 * one pending term plus one target-sized output bundle; it never materializes
 * the corpus posting set. The caller owns each returned bundle.
 */
ii42_status ii42_initial_fold_stream_create(
    const ii42_index *lexical_index,
    size_t semantic_posting_count,
    ii42_segment_semantic_posting_reader semantic_reader,
    ii42_segment_semantic_posting_rewind semantic_rewind,
    void *semantic_reader_context,
    uint64_t owner_manifest_id,
    uint64_t coverage_sequence,
    size_t target_bytes,
    ii42_initial_fold_stream **stream_out
);

ii42_status ii42_initial_fold_stream_next(
    ii42_initial_fold_stream *stream,
    ii42_term_fold_bundle *bundle_out,
    bool *done_out
);

void ii42_initial_fold_stream_free(ii42_initial_fold_stream *stream);

/*
 * Validate only the posting/run projection of a payload. Incremental
 * metadata publication uses this before immutable version/state records are
 * attached by the page writer.
 */
ii42_status ii42_segment_posting_payload_validate(
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    const ii42_segment_payload_view *payload
);

ii42_status ii42_segment_payload_validate(
    const ii42_segment_payload *payload,
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment
);

/*
 * Merge one contiguous immutable descriptor range without changing posting
 * values, document-version identity, or retirement facts.
 */
ii42_status ii42_segment_payload_merge(
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload *payloads,
    uint32_t payload_count,
    uint64_t merged_segment_id,
    ii42_segment_payload *payload_out
);

/*
 * Merge one bounded descriptor range while omitting document slots whose
 * retirement is already authoritative and snapshot-safe. The exclusion set
 * must be strictly increasing.
 */
ii42_status ii42_segment_payload_merge_excluding(
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload *payloads,
    uint32_t payload_count,
    uint64_t merged_segment_id,
    const uint32_t *excluded_document_ids,
    uint32_t excluded_document_count,
    ii42_segment_payload *payload_out
);

ii42_status ii42_segment_payload_serialized_size(
    const ii42_segment_payload *payload,
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    size_t *size_out
);

/*
 * The target descriptor may have zero payload_bytes/payload_checksum while it
 * is being built. Publish only after filling them from size_out/checksum_out
 * and validating the complete manifest.
 */
ii42_status ii42_segment_payload_serialize(
    const ii42_segment_payload *payload,
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    uint8_t **bytes_out,
    size_t *size_out,
    uint64_t *checksum_out
);

/*
 * payload_out must be initialized with ii42_segment_payload_init().
 * A failed decode leaves the existing output unchanged.
 */
ii42_status ii42_segment_payload_deserialize(
    const uint8_t *bytes,
    size_t size,
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    ii42_segment_payload *payload_out
);

ii42_status ii42_segment_payload_disk_header_decode(
    const uint8_t *bytes,
    size_t size,
    const ii42_segment_manifest *manifest,
    const ii42_segment_descriptor *segment,
    ii42_segment_payload_disk_header *header_out
);

ii42_status ii42_segment_term_run_decode(
    const uint8_t *bytes,
    size_t size,
    ii42_segment_term_run *run_out
);

ii42_status ii42_posting_block_record_decode(
    const uint8_t *bytes,
    size_t size,
    ii42_posting_block_record *record_out
);

void ii42_segment_payload_as_view(
    const ii42_segment_payload *payload,
    ii42_segment_payload_view *view_out
);

/*
 * Reconstruct the scorer and query metadata required by a segmented read
 * view. Posting arrays remain owned by payloads; index_out contains no
 * duplicate posting matrix.
 */
ii42_status ii42_segment_index_metadata_build_base(
    const ii42_segment_query_contract *contract,
    const ii42_segment_manifest *manifest,
    const uint32_t *doc_frequencies,
    ii42_index *index_out
);

/*
 * Validate that every immutable posting references an authoritative document
 * version. available_document_slots uses 1 for a materialized version and any
 * other value for an unavailable or aborted slot.
 */
ii42_status ii42_segment_payload_document_references_validate(
    const ii42_segment_manifest *manifest,
    const ii42_segment_payload *payloads,
    size_t payload_count,
    const uint8_t *available_document_slots,
    size_t document_slot_count
);

ii42_status ii42_segment_index_metadata_build(
    const ii42_segment_query_contract *contract,
    const ii42_segment_manifest *manifest,
    const uint32_t *doc_frequencies,
    const ii42_segment_payload *payloads,
    size_t payload_count,
    ii42_index *index_out
);

void ii42_segment_read_view_init(ii42_segment_read_view *view);
void ii42_segment_read_view_free(ii42_segment_read_view *view);

ii42_status ii42_segment_read_view_build(
    const ii42_index *global_index,
    const ii42_segment_manifest *manifest,
    const ii42_term_directory *directory,
    const ii42_segment_payload_view *payloads,
    size_t payload_count,
    ii42_segment_read_view *view_out
);

/*
 * Attach persistent neutral prefixes before their exact post-coverage tail.
 * Every immutable payload run must be consumed by exactly one fold or tail
 * extent; gaps, overlaps, and straddling coverage boundaries fail closed.
 */
ii42_status ii42_segment_read_view_build_folded(
    const ii42_index *global_index,
    const ii42_segment_manifest *manifest,
    const ii42_term_directory *tail_directory,
    const ii42_segment_payload_view *payloads,
    size_t payload_count,
    const ii42_term_fold_bundle *fold_bundles,
    size_t fold_bundle_count,
    const ii42_term_fold_read_plan *fold_plans,
    size_t fold_plan_count,
    ii42_segment_read_view *view_out
);

#endif
