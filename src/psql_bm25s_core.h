#ifndef PSQL_BM25S_CORE_H
#define PSQL_BM25S_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum psql_bm25s_status
{
    PSQL_BM25S_OK = 0,
    PSQL_BM25S_ERR_NOMEM = 1,
    PSQL_BM25S_ERR_INVALID = 2,
    PSQL_BM25S_ERR_RANGE = 3,
    PSQL_BM25S_ERR_FORMAT = 4
} psql_bm25s_status;

typedef enum psql_bm25s_method
{
    PSQL_BM25S_METHOD_ROBERTSON = 0,
    PSQL_BM25S_METHOD_LUCENE = 1,
    PSQL_BM25S_METHOD_ATIRE = 2,
    PSQL_BM25S_METHOD_BM25L = 3,
    PSQL_BM25S_METHOD_BM25PLUS = 4
} psql_bm25s_method;

typedef struct psql_bm25s_doc_ids
{
    uint32_t *token_ids;
    size_t len;
} psql_bm25s_doc_ids;

typedef struct psql_bm25s_doc_tokens
{
    const char **tokens;
    size_t len;
} psql_bm25s_doc_tokens;

typedef struct psql_bm25s_term_entry
{
    uint32_t token_id;
    uint32_t doc_id;
    uint32_t tf;
} psql_bm25s_term_entry;

typedef psql_bm25s_status (*psql_bm25s_term_entry_reader_cb)(
    void *ctx,
    psql_bm25s_term_entry *entry_out
);

typedef psql_bm25s_status (*psql_bm25s_term_entry_rewind_cb)(void *ctx);

typedef struct psql_bm25s_params
{
    float k1;
    float b;
    float delta;
    psql_bm25s_method method;
    psql_bm25s_method idf_method;
} psql_bm25s_params;

typedef struct psql_bm25s_index
{
    psql_bm25s_params params;
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
} psql_bm25s_index;

typedef struct psql_bm25s_topk_result
{
    uint32_t *doc_ids;
    float *scores;
    size_t len;
} psql_bm25s_topk_result;

const char *psql_bm25s_strerror(psql_bm25s_status status);
bool psql_bm25s_parse_method(
    const char *name,
    psql_bm25s_method *method_out
);
const char *psql_bm25s_method_name(psql_bm25s_method method);
const char *psql_bm25s_active_simd_path(void);
void psql_bm25s_index_init(psql_bm25s_index *index);
void psql_bm25s_index_free(psql_bm25s_index *index);
void psql_bm25s_topk_result_free(psql_bm25s_topk_result *result);

psql_bm25s_status psql_bm25s_build_index_from_ids(
    const psql_bm25s_doc_ids *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    bool create_empty_token,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_build_index_from_ids_compact(
    const psql_bm25s_doc_ids *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    bool create_empty_token,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_build_index_from_term_entries(
    const psql_bm25s_term_entry *entries,
    uint64_t num_entries,
    const uint32_t *doc_lengths,
    uint32_t num_docs,
    uint32_t vocab_size,
    const psql_bm25s_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    const char *const *vocab,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_build_index_from_term_entry_reader(
    uint64_t num_entries,
    psql_bm25s_term_entry_reader_cb read_cb,
    psql_bm25s_term_entry_rewind_cb rewind_cb,
    void *reader_ctx,
    const uint32_t *doc_lengths,
    uint32_t num_docs,
    uint32_t vocab_size,
    const psql_bm25s_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    const char *const *vocab,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_build_index_from_tokens(
    const psql_bm25s_doc_tokens *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_build_index_from_tokens_compact(
    const psql_bm25s_doc_tokens *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_query_token_ids(
    const psql_bm25s_index *index,
    const char **tokens,
    size_t num_tokens,
    uint32_t **query_ids_out,
    size_t *query_len_out
);

psql_bm25s_status psql_bm25s_scores_from_ids(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

psql_bm25s_status psql_bm25s_scores_from_ids_exact_stats(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
);

psql_bm25s_status psql_bm25s_topk(
    const float *scores,
    size_t num_scores,
    size_t k,
    bool sorted,
    psql_bm25s_topk_result *result_out
);

psql_bm25s_status psql_bm25s_topk_subset(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    size_t k,
    bool sorted,
    bool positive_only,
    psql_bm25s_topk_result *result_out
);

#endif
