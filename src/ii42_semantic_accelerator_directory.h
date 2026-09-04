#ifndef II42_SEMANTIC_ACCELERATOR_DIRECTORY_H
#define II42_SEMANTIC_ACCELERATOR_DIRECTORY_H

#include <stddef.h>
#include <stdint.h>

#include "ii42_segments.h"
#include "ii42_semantic_forward_bound.h"

#define II42_SEMANTIC_ACCELERATOR_POLICY_SCOPE_FORWARD_INT8 \
    UINT32_C(7)
#define II42_SEMANTIC_ACCELERATOR_CURRENT_POLICY \
    II42_SEMANTIC_ACCELERATOR_POLICY_SCOPE_FORWARD_INT8

/*
 * The retained surface is a candidate seed, not a posting-list replica.
 * ArXiv scale evidence rejected widening this cap to 128: it increased the
 * derived artifact without repairing cross-term tail misses.
 */
#define II42_SEMANTIC_ACCELERATOR_RETAINED_DOCUMENT_CAP UINT32_C(64)
#define II42_SEMANTIC_ACCELERATOR_DIRECTORY_MIN_HEADER_SIZE 80U
#define II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE 176U

typedef struct ii42_semantic_accelerator_directory_entry
{
    uint32_t term_id;
    ii42_segment_object_ref term_object;
} ii42_semantic_accelerator_directory_entry;

typedef struct ii42_semantic_accelerator_forward_entry
{
    uint32_t first_document;
    uint32_t document_count;
    uint32_t posting_count;
    uint32_t row_data_offset;
    ii42_segment_object_ref forward_object;
} ii42_semantic_accelerator_forward_entry;

typedef struct ii42_semantic_accelerator_directory
{
    uint64_t source_manifest_id;
    uint64_t source_authority_checksum;
    uint64_t owner_manifest_id;
    uint32_t document_count;
    uint32_t vocab_size;
    uint32_t term_count;
    uint32_t forward_chunk_count;
    uint32_t forward_bound_shard_count;
    uint32_t forward_document_shift;
    uint32_t builder_policy_id;
    uint32_t retained_document_cap;
    ii42_segment_object_ref scope_object;
    ii42_segment_object_ref tid_lookup_object;
    ii42_semantic_accelerator_directory_entry *terms;
    ii42_semantic_accelerator_forward_entry *forward_chunks;
    uint64_t *forward_term_work;
    uint64_t *forward_row_data_bytes;
    uint64_t *forward_transpose_fixed_bytes;
    uint32_t *forward_row_offsets;
    uint64_t *forward_term_bytes;
    uint64_t *forward_bound_term_bytes;
    ii42_segment_object_ref *forward_bound_shards;
} ii42_semantic_accelerator_directory;

typedef struct ii42_semantic_accelerator_directory_summary
{
    uint16_t format_version;
    uint16_t header_size;
    uint16_t term_entry_size;
    uint16_t forward_entry_size;
    uint64_t source_manifest_id;
    uint64_t source_authority_checksum;
    uint64_t owner_manifest_id;
    uint64_t total_size;
    uint32_t document_count;
    uint32_t vocab_size;
    uint32_t term_count;
    uint32_t forward_chunk_count;
    uint32_t forward_bound_shard_count;
    uint32_t forward_document_shift;
    uint32_t builder_policy_id;
    uint32_t retained_document_cap;
    uint32_t forward_term_work_count;
    uint32_t forward_chunk_cost_count;
    uint64_t forward_row_offset_count;
    uint32_t forward_term_bytes_count;
    uint32_t forward_bound_term_bytes_count;
    ii42_segment_object_ref scope_object;
    ii42_segment_object_ref tid_lookup_object;
} ii42_semantic_accelerator_directory_summary;

bool ii42_semantic_accelerator_directory_has_scope(
    const ii42_semantic_accelerator_directory *directory
);

bool ii42_semantic_accelerator_directory_summary_has_scope(
    const ii42_semantic_accelerator_directory_summary *summary
);

bool ii42_semantic_accelerator_directory_has_tid_lookup(
    const ii42_semantic_accelerator_directory *directory
);

bool ii42_semantic_accelerator_directory_summary_has_tid_lookup(
    const ii42_semantic_accelerator_directory_summary *summary
);

void ii42_semantic_accelerator_directory_init(
    ii42_semantic_accelerator_directory *directory
);

void ii42_semantic_accelerator_directory_free(
    ii42_semantic_accelerator_directory *directory
);

ii42_status ii42_semantic_accelerator_directory_validate(
    const ii42_semantic_accelerator_directory *directory
);

const ii42_semantic_accelerator_directory_entry *
ii42_semantic_accelerator_directory_find(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t term_id
);

const ii42_semantic_accelerator_forward_entry *
ii42_semantic_accelerator_directory_find_forward(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t document_id
);

bool ii42_semantic_accelerator_directory_has_complete_forward(
    const ii42_semantic_accelerator_directory *directory
);

bool ii42_semantic_accelerator_directory_has_complete_forward_bounds(
    const ii42_semantic_accelerator_directory *directory
);

const ii42_segment_object_ref *
ii42_semantic_accelerator_directory_find_forward_bound(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t term_id
);

bool ii42_semantic_accelerator_directory_is_current(
    const ii42_semantic_accelerator_directory *directory
);

ii42_status ii42_semantic_accelerator_directory_serialize(
    const ii42_semantic_accelerator_directory *directory,
    uint8_t **bytes_out,
    size_t *size_out
);

/* directory_out must be initialized before this call. */
ii42_status ii42_semantic_accelerator_directory_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
);

/* Retirement-only reader for the immediately preceding derived format. */
ii42_status ii42_semantic_accelerator_directory_deserialize_retired(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
);

ii42_status ii42_semantic_accelerator_directory_summary_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory_summary *summary_out
);

bool ii42_semantic_accelerator_directory_summary_is_current(
    const ii42_semantic_accelerator_directory_summary *summary
);

bool ii42_semantic_accelerator_directory_summary_format_is_current(
    const ii42_semantic_accelerator_directory_summary *summary
);

bool ii42_semantic_accelerator_directory_summary_has_complete_forward(
    const ii42_semantic_accelerator_directory_summary *summary
);

bool ii42_semantic_accelerator_directory_summary_has_complete_forward_bounds(
    const ii42_semantic_accelerator_directory_summary *summary
);

ii42_status ii42_semantic_accelerator_directory_forward_slice(
    const uint8_t *header_bytes,
    size_t header_size,
    ii42_semantic_accelerator_directory_summary *summary_out,
    size_t *offset_out,
    size_t *size_out
);

/*
 * directory_out owns forward chunks, term-work data, and bound refs. Term
 * objects remain intentionally lazy.
 */
ii42_status ii42_semantic_accelerator_forward_directory_deserialize(
    const ii42_semantic_accelerator_directory_summary *summary,
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
);

#endif
