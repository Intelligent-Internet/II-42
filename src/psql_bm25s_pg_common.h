#ifndef PSQL_BM25S_PG_COMMON_H
#define PSQL_BM25S_PG_COMMON_H

#include "postgres.h"

#include "utils/array.h"

#include "psql_bm25s_core.h"

typedef struct psql_bm25s_doc_tokens_builder
{
    psql_bm25s_doc_tokens *docs;
    size_t len;
    size_t capacity;
} psql_bm25s_doc_tokens_builder;

typedef struct psql_bm25s_doc_ids_builder
{
    psql_bm25s_doc_ids *docs;
    size_t len;
    size_t capacity;
} psql_bm25s_doc_ids_builder;

float *psql_bm25s_array_read_weight_mask(ArrayType *array, uint32_t expected_len);
uint32_t *psql_bm25s_array_read_query_ids(ArrayType *array, size_t *query_len_out);
char **psql_bm25s_array_read_query_tokens(ArrayType *array, size_t *query_len_out);
char **psql_bm25s_array_read_doc_tokens(ArrayType *array, size_t *doc_len_out);

#endif
