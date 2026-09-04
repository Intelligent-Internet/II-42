#ifndef II42_CORE_H
#define II42_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ii42_status
{
    II42_OK = 0,
    II42_ERR_NOMEM = 1,
    II42_ERR_INVALID = 2,
    II42_ERR_RANGE = 3,
    II42_ERR_FORMAT = 4
} ii42_status;

uint32_t ii42_u32_saturating_add(uint32_t left, uint32_t right);
uint64_t ii42_u64_saturating_add(uint64_t left, uint64_t right);
uint64_t ii42_u64_saturating_mul(uint64_t left, uint64_t right);

typedef enum ii42_method
{
    II42_METHOD_ROBERTSON = 0,
    II42_METHOD_LUCENE = 1,
    II42_METHOD_ATIRE = 2,
    II42_METHOD_BM25L = 3,
    II42_METHOD_BM25PLUS = 4
} ii42_method;

typedef struct ii42_doc_ids
{
    uint32_t *token_ids;
    size_t len;
} ii42_doc_ids;

typedef struct ii42_doc_tokens
{
    const char **tokens;
    size_t len;
} ii42_doc_tokens;

typedef struct ii42_term_entry
{
    uint32_t token_id;
    uint32_t doc_id;
    uint32_t tf;
} ii42_term_entry;

typedef ii42_status (*ii42_term_entry_reader_cb)(
    void *ctx,
    ii42_term_entry *entry_out
);

typedef ii42_status (*ii42_term_entry_rewind_cb)(void *ctx);

typedef struct ii42_params
{
    float k1;
    float b;
    float delta;
    ii42_method method;
    ii42_method idf_method;
} ii42_params;

typedef struct ii42_index
{
    ii42_params params;
    uint32_t num_docs;
    uint32_t vocab_size;
    uint64_t data_len;
    bool has_empty_token;
    uint32_t empty_token_id;
    float *data;
    uint32_t *indices;
    uint64_t *indptr;
    uint32_t *term_frequencies;
    uint32_t *doc_lengths;
    uint32_t *doc_frequencies;
    float *nonoccurrence;
    char **vocab;
} ii42_index;

typedef struct ii42_topk_result
{
    uint32_t *doc_ids;
    float *scores;
    size_t len;
} ii42_topk_result;

typedef struct ii42_topk_item
{
    float score;
    uint32_t doc_id;
    uint64_t tie_break_key;
} ii42_topk_item;

/* One-shot streaming top-k state whose allocation is proportional to k. */
typedef struct ii42_topk_accumulator
{
    ii42_topk_item *heap;
    size_t len;
    size_t capacity;
    bool finalized;
} ii42_topk_accumulator;

typedef enum ii42_posting_extent_kind
{
    II42_POSTING_EXTENT_LEXICAL_NEUTRAL = 1,
    II42_POSTING_EXTENT_SEMANTIC_IMPACT = 2,
    II42_POSTING_EXTENT_LEXICAL_IMPACT = 3
} ii42_posting_extent_kind;

typedef union ii42_posting_value
{
    float impact;
    uint32_t term_frequency;
} ii42_posting_value;

#define II42_DEFAULT_POSTING_BLOCK_SHIFT 7U

/*
 * Persistent, query-independent metadata for one posting slice inside a
 * stable global document-slot block. Document-length extrema are deliberately
 * not duplicated per term: the query runtime derives them once from the
 * global document-block directory.
 */
typedef struct ii42_posting_block_record
{
    uint64_t posting_offset;
    uint32_t posting_count;
    uint32_t block_id;
    uint32_t first_document_id;
    uint32_t last_document_id;
    uint32_t min_term_frequency;
    uint32_t max_term_frequency;
    float min_impact;
    float max_impact;
    ii42_posting_extent_kind kind;
} ii42_posting_block_record;

typedef struct ii42_posting_extent
{
    const float *data;
    const uint32_t *indices;
    const uint32_t *term_frequencies;
    const ii42_posting_value *values;
    const uint32_t *document_id_map;
    const ii42_posting_block_record *blocks;
    uint64_t len;
    uint32_t document_id_base;
    uint32_t local_document_count;
    uint32_t block_count;
    uint32_t block_shift;
    ii42_posting_extent_kind kind;
} ii42_posting_extent;

typedef struct ii42_term_extent_list
{
    const ii42_posting_extent *extents;
    size_t len;
} ii42_term_extent_list;

/*
 * One query-independent upper-bound unit aligned to the stable global
 * document-slot space. Every extent and fold uses the same block identity, so
 * bounds from folded prefixes and exact tails can later enter one global
 * pruning schedule.
 */
typedef struct ii42_posting_block_bound
{
    uint64_t posting_offset;
    uint32_t posting_count;
    uint32_t block_id;
    uint32_t first_document_id;
    uint32_t last_document_id;
    uint32_t min_term_frequency;
    uint32_t max_term_frequency;
    uint32_t min_document_length;
    uint32_t max_document_length;
    float min_impact;
    float max_impact;
    ii42_posting_extent_kind kind;
} ii42_posting_block_bound;

typedef struct ii42_document_block_extrema
{
    uint64_t document_count;
    uint32_t min_document_length;
    uint32_t max_document_length;
} ii42_document_block_extrema;

typedef struct ii42_blockmax_stats
{
    uint64_t blocks_considered;
    uint64_t blocks_scored;
    uint64_t blocks_skipped;
    uint64_t postings_scored;
    uint64_t zero_score_documents_considered;
} ii42_blockmax_stats;

typedef struct ii42_corpus_stats
{
    uint64_t document_count;
    uint64_t total_document_length;
    const uint32_t *doc_frequencies;
    size_t vocab_size;
} ii42_corpus_stats;

/*
 * Validate an extent once when it is attached to a trusted read view. Scoring
 * assumes this validation has succeeded and does not repeat per-posting range
 * checks in the hot loop.
 */
ii42_status ii42_posting_extent_validate_layout(
    const ii42_index *index,
    const ii42_posting_extent *extent
);

ii42_status ii42_posting_extent_build_block_records(
    const ii42_posting_extent *extent,
    uint32_t block_shift,
    ii42_posting_block_record **records_out,
    size_t *record_count_out
);

void ii42_posting_block_records_free(
    ii42_posting_block_record *records
);

ii42_status ii42_posting_extent_validate_block_records(
    const ii42_posting_extent *extent,
    uint32_t block_shift,
    const ii42_posting_block_record *records,
    size_t record_count
);

ii42_status ii42_posting_extent_build_block_bounds(
    const ii42_index *index,
    const ii42_posting_extent *extent,
    uint32_t block_shift,
    ii42_posting_block_bound **bounds_out,
    size_t *bound_count_out
);

void ii42_posting_block_bounds_free(ii42_posting_block_bound *bounds);

ii42_status ii42_posting_block_score_upper_bound(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    uint32_t live_document_frequency,
    const ii42_posting_block_bound *bound,
    float query_weight,
    float *score_upper_bound_out
);

const char *ii42_strerror(ii42_status status);
bool ii42_params_are_valid(const ii42_params *params);
bool ii42_parse_method(
    const char *name,
    ii42_method *method_out
);
const char *ii42_method_name(ii42_method method);
bool ii42_method_requires_nonoccurrence(ii42_method method);
double ii42_score_tfc(
    ii42_method method,
    double tf,
    double doc_length,
    double average_doc_length,
    double k1,
    double b,
    double delta
);
double ii42_score_idf(
    ii42_method method,
    double document_frequency,
    double document_count
);
const char *ii42_active_simd_path(void);
void ii42_index_init(ii42_index *index);
void ii42_index_free(ii42_index *index);
void ii42_topk_result_free(ii42_topk_result *result);

ii42_status ii42_build_index_from_ids(
    const ii42_doc_ids *docs,
    size_t num_docs,
    const ii42_params *params,
    bool create_empty_token,
    ii42_index *index_out
);

ii42_status ii42_build_index_from_ids_compact(
    const ii42_doc_ids *docs,
    size_t num_docs,
    const ii42_params *params,
    bool create_empty_token,
    ii42_index *index_out
);

ii42_status ii42_build_index_from_term_entries(
    const ii42_term_entry *entries,
    uint64_t num_entries,
    const uint32_t *doc_lengths,
    uint32_t num_docs,
    uint32_t vocab_size,
    const ii42_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    const char *const *vocab,
    ii42_index *index_out
);

ii42_status ii42_build_index_from_term_entry_reader(
    uint64_t num_entries,
    ii42_term_entry_reader_cb read_cb,
    ii42_term_entry_rewind_cb rewind_cb,
    void *reader_ctx,
    const uint32_t *doc_lengths,
    uint32_t num_docs,
    uint32_t vocab_size,
    const ii42_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    const char *const *vocab,
    ii42_index *index_out
);

ii42_status ii42_build_index_from_tokens(
    const ii42_doc_tokens *docs,
    size_t num_docs,
    const ii42_params *params,
    ii42_index *index_out
);

ii42_status ii42_build_index_from_tokens_compact(
    const ii42_doc_tokens *docs,
    size_t num_docs,
    const ii42_params *params,
    ii42_index *index_out
);

ii42_status ii42_query_token_ids(
    const ii42_index *index,
    const char **tokens,
    size_t num_tokens,
    uint32_t **query_ids_out,
    size_t *query_len_out
);

ii42_status ii42_scores_from_ids(
    const ii42_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_scores_from_ids_extents(
    const ii42_index *index,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_scores_from_ids_exact_stats(
    const ii42_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_scores_from_ids_neutral(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_scores_from_ids_mixed(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_scores_from_ids_mixed_retired(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

/*
 * Resolve the exact live lexical document frequency for one mixed read-view
 * term. This is the shared authority for query scoring and epoch-bound impact
 * compilation; semantic extents do not contribute to lexical DF.
 */
ii42_status ii42_term_live_document_frequency(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    uint32_t term_id,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    uint32_t *live_document_frequency_out
);

ii42_status ii42_scores_from_weighted_ids_mixed_retired(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

ii42_status ii42_topk_from_weighted_ids_mixed_retired_blockmax(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const ii42_document_block_extrema *document_blocks,
    size_t document_block_count,
    uint32_t block_shift,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_len,
    size_t k,
    ii42_topk_result *result_out,
    ii42_blockmax_stats *blockmax_stats_out
);

ii42_status
ii42_topk_from_weighted_ids_mixed_retired_blockmax_with_tie_breaks(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const ii42_document_block_extrema *document_blocks,
    size_t document_block_count,
    uint32_t block_shift,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    const uint64_t *tie_break_keys,
    const uint32_t *tie_break_order,
    size_t tie_break_order_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_len,
    size_t k,
    ii42_topk_result *result_out,
    ii42_blockmax_stats *blockmax_stats_out
);

ii42_status
ii42_topk_from_weighted_ids_mixed_retired_blockmax_filtered_with_tie_breaks(
    const ii42_index *index,
    const ii42_corpus_stats *stats,
    const ii42_term_extent_list *term_extents,
    size_t num_term_extent_lists,
    const ii42_document_block_extrema *document_blocks,
    size_t document_block_count,
    uint32_t block_shift,
    const uint32_t *retired_document_ids,
    size_t retired_document_count,
    const uint64_t *tie_break_keys,
    const uint32_t *tie_break_order,
    size_t tie_break_order_count,
    const uint8_t *allowed_document_bitmap,
    size_t allowed_document_count,
    const uint32_t *query_ids,
    const float *query_weights,
    size_t query_len,
    size_t k,
    ii42_topk_result *result_out,
    ii42_blockmax_stats *blockmax_stats_out
);

ii42_status ii42_topk(
    const float *scores,
    size_t num_scores,
    size_t k,
    bool sorted,
    ii42_topk_result *result_out
);

ii42_status ii42_topk_with_tie_breaks(
    const float *scores,
    size_t num_scores,
    size_t k,
    bool sorted,
    const uint64_t *tie_break_keys,
    ii42_topk_result *result_out
);

ii42_status ii42_topk_subset(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    size_t k,
    bool sorted,
    bool positive_only,
    ii42_topk_result *result_out
);

ii42_status ii42_topk_subset_with_tie_breaks(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    size_t k,
    bool sorted,
    bool positive_only,
    const uint64_t *tie_break_keys,
    ii42_topk_result *result_out
);

ii42_status ii42_topk_accumulator_init(
    ii42_topk_accumulator *accumulator,
    size_t capacity
);

ii42_status ii42_topk_accumulator_offer(
    ii42_topk_accumulator *accumulator,
    float score,
    uint32_t doc_id,
    uint64_t tie_break_key
);

ii42_status ii42_topk_accumulator_finish(
    ii42_topk_accumulator *accumulator,
    bool sorted,
    ii42_topk_result *result_out
);

void ii42_topk_accumulator_free(
    ii42_topk_accumulator *accumulator
);

#endif
