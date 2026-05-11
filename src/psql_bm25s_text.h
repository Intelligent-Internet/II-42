#ifndef PSQL_BM25S_TEXT_H
#define PSQL_BM25S_TEXT_H

#include <stdbool.h>
#include <stddef.h>

#include "psql_bm25s_core.h"
#include "psql_bm25s_query.h"

typedef struct psql_bm25s_text_options
{
    bool lowercase;
    bool stem_english;
    bool fold_diacritics;
    const char * const *stopwords;
    size_t num_stopwords;
} psql_bm25s_text_options;

void psql_bm25s_text_options_init(psql_bm25s_text_options *options);

psql_bm25s_status psql_bm25s_normalize_token(
    const char *input,
    const psql_bm25s_text_options *options,
    char **token_out,
    bool *keep_out
);

psql_bm25s_status psql_bm25s_normalize_query(
    psql_bm25s_query *query,
    const psql_bm25s_text_options *options
);

psql_bm25s_status psql_bm25s_tokenize_text(
    const char *input,
    const psql_bm25s_text_options *options,
    char ***tokens_out,
    size_t *len_out
);

void psql_bm25s_text_tokens_free(char **tokens, size_t len);

#endif
