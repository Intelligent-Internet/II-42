#ifndef II42_PG_COMMON_H
#define II42_PG_COMMON_H

#include "postgres.h"

#include "utils/array.h"

#include "ii42_core.h"

typedef struct ii42_doc_tokens_builder
{
    ii42_doc_tokens *docs;
    size_t len;
    size_t capacity;
} ii42_doc_tokens_builder;

typedef struct ii42_doc_ids_builder
{
    ii42_doc_ids *docs;
    size_t len;
    size_t capacity;
} ii42_doc_ids_builder;

float *ii42_array_read_weight_mask(ArrayType *array, uint32_t expected_len);
uint32_t *ii42_array_read_query_ids(ArrayType *array, size_t *query_len_out);
char **ii42_array_read_query_tokens(ArrayType *array, size_t *query_len_out);
char **ii42_array_read_doc_tokens(ArrayType *array, size_t *doc_len_out);
ArrayType *ii42_array_from_float4_values(const float4 *items, size_t len);
ArrayType *ii42_array_from_int4_values(const int32 *items, size_t len);
ArrayType *ii42_array_from_cstrings(char **tokens, size_t len);

#endif
