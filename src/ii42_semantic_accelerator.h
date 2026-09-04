#ifndef II42_SEMANTIC_ACCELERATOR_H
#define II42_SEMANTIC_ACCELERATOR_H

#include <stddef.h>
#include <stdint.h>

#include "ii42_core.h"

#define II42_SEMANTIC_ACCELERATOR_FORMAT_VERSION 2U
#define II42_SEMANTIC_ACCELERATOR_HEADER_SIZE 88U
#define II42_SEMANTIC_ACCELERATOR_TERM_SIZE 12U
#define II42_SEMANTIC_ACCELERATOR_CLUSTER_SIZE 20U
#define II42_SEMANTIC_ACCELERATOR_SUMMARY_SIZE 8U

/*
 * This is a disposable approximate candidate index. The callback computes
 * exact scores from the packed posting authority, but cluster pruning can omit
 * documents. Do not use it for the public exact route.
 */

typedef struct ii42_semantic_accelerator_summary_input
{
    uint32_t term_id;
    float max_impact;
} ii42_semantic_accelerator_summary_input;

typedef struct ii42_semantic_accelerator_cluster_input
{
    const uint32_t *document_ids;
    uint32_t document_count;
    const ii42_semantic_accelerator_summary_input *summary;
    uint32_t summary_count;
} ii42_semantic_accelerator_cluster_input;

typedef struct ii42_semantic_accelerator_term_input
{
    uint32_t term_id;
    const ii42_semantic_accelerator_cluster_input *clusters;
    uint32_t cluster_count;
} ii42_semantic_accelerator_term_input;

typedef struct ii42_semantic_accelerator_term
{
    uint32_t term_id;
    uint32_t first_cluster;
    uint32_t cluster_count;
} ii42_semantic_accelerator_term;

typedef struct ii42_semantic_accelerator_cluster
{
    uint32_t first_document;
    uint32_t document_count;
    uint32_t first_summary;
    uint16_t summary_count;
    uint16_t reserved;
    float quantum;
} ii42_semantic_accelerator_cluster;

typedef struct ii42_semantic_accelerator_summary
{
    uint32_t term_id;
    uint8_t quantized_impact;
    uint8_t reserved[3];
} ii42_semantic_accelerator_summary;

typedef struct ii42_semantic_accelerator_index
{
    uint64_t source_root_checksum;
    uint32_t document_count;
    uint32_t term_count;
    uint32_t cluster_count;
    uint32_t document_ref_count;
    uint32_t summary_count;
    ii42_semantic_accelerator_term *terms;
    ii42_semantic_accelerator_cluster *clusters;
    uint32_t *document_ids;
    ii42_semantic_accelerator_summary *summaries;
} ii42_semantic_accelerator_index;

/* Initialize every index before passing it to build or deserialize. */

typedef struct ii42_semantic_accelerator_options
{
    uint32_t query_cut;
    uint32_t candidate_multiplier;
    uint32_t document_shift;
    float heap_factor;
} ii42_semantic_accelerator_options;

typedef struct ii42_semantic_accelerator_stats
{
    uint64_t query_scratch_peak_bytes;
    uint64_t summary_entries_examined;
    uint64_t clusters_considered;
    uint64_t clusters_opened;
    uint64_t clusters_skipped;
    uint64_t document_refs_examined;
    uint64_t documents_scored;
    uint64_t duplicate_documents_skipped;
    bool block_major_query;
} ii42_semantic_accelerator_stats;

typedef ii42_status (*ii42_semantic_accelerator_score_cb)(
    void *context,
    uint32_t document_id,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_count,
    float *score_out
);

/*
 * Opaque query scratch ownership for hosts whose score callback can perform a
 * non-local exit. The regular topk APIs retain one-shot ownership semantics;
 * PostgreSQL keeps this handle in its statement cleanup owner instead.
 */
typedef struct ii42_semantic_accelerator_query_scratch
    ii42_semantic_accelerator_query_scratch;

ii42_semantic_accelerator_query_scratch *
ii42_semantic_accelerator_query_scratch_create(void);

void ii42_semantic_accelerator_query_scratch_destroy(
    ii42_semantic_accelerator_query_scratch *scratch
);

void ii42_semantic_accelerator_index_init(
    ii42_semantic_accelerator_index *index
);

void ii42_semantic_accelerator_index_free(
    ii42_semantic_accelerator_index *index
);

ii42_status ii42_semantic_accelerator_index_build(
    uint64_t source_root_checksum,
    uint32_t document_count,
    const ii42_semantic_accelerator_term_input *terms,
    uint32_t term_count,
    ii42_semantic_accelerator_index *index_out
);

ii42_status ii42_semantic_accelerator_index_validate(
    const ii42_semantic_accelerator_index *index
);

bool ii42_semantic_accelerator_root_matches(
    const ii42_semantic_accelerator_index *index,
    uint64_t source_root_checksum
);

ii42_status ii42_semantic_accelerator_serialize(
    const ii42_semantic_accelerator_index *index,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_semantic_accelerator_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_index *index_out
);

/* Bounded immutable-object codec used by page-native per-term publication. */
ii42_status ii42_semantic_accelerator_term_serialize(
    uint64_t source_root_checksum,
    uint32_t document_count,
    const ii42_semantic_accelerator_term_input *term,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_semantic_accelerator_term_deserialize(
    const uint8_t *bytes,
    size_t size,
    uint64_t expected_source_root_checksum,
    uint32_t expected_term_id,
    ii42_semantic_accelerator_index *index_out
);

ii42_status ii42_semantic_accelerator_topk(
    const ii42_semantic_accelerator_index *index,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_count,
    size_t k,
    const ii42_semantic_accelerator_options *options,
    ii42_semantic_accelerator_score_cb score_document,
    void *score_context,
    ii42_topk_result *result_out,
    ii42_semantic_accelerator_stats *stats_out
);

/*
 * Search independently stored per-term objects without reconstructing one
 * corpus-sized accelerator in backend memory. Every index must be bound to
 * the same source authority and document-slot space.
 */
ii42_status ii42_semantic_accelerator_topk_many(
    const ii42_semantic_accelerator_index *const *indexes,
    size_t index_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_count,
    size_t k,
    const ii42_semantic_accelerator_options *options,
    ii42_semantic_accelerator_score_cb score_document,
    void *score_context,
    ii42_topk_result *result_out,
    ii42_semantic_accelerator_stats *stats_out
);

ii42_status ii42_semantic_accelerator_topk_many_owned(
    const ii42_semantic_accelerator_index *const *indexes,
    size_t index_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_count,
    size_t k,
    const ii42_semantic_accelerator_options *options,
    ii42_semantic_accelerator_score_cb score_document,
    void *score_context,
    ii42_topk_result *result_out,
    ii42_semantic_accelerator_stats *stats_out,
    ii42_semantic_accelerator_query_scratch *scratch
);

#endif
