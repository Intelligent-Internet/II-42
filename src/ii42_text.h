#ifndef II42_TEXT_H
#define II42_TEXT_H

#include <stdbool.h>
#include <stddef.h>

#include "ii42_core.h"
#include "ii42_query.h"

typedef struct ii42_text_options
{
    bool lowercase;
    bool stem_english;
    bool fold_diacritics;
    const char * const *stopwords;
    size_t num_stopwords;
} ii42_text_options;

void ii42_text_options_init(ii42_text_options *options);

ii42_status ii42_normalize_token(
    const char *input,
    const ii42_text_options *options,
    char **token_out,
    bool *keep_out
);

ii42_status ii42_normalize_query(
    ii42_query *query,
    const ii42_text_options *options
);

ii42_status ii42_tokenize_text(
    const char *input,
    const ii42_text_options *options,
    char ***tokens_out,
    size_t *len_out
);

void ii42_text_tokens_free(char **tokens, size_t len);

#endif
