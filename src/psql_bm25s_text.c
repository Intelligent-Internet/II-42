#include "psql_bm25s_text.h"
#include "psql_bm25s_stem.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>

static bool
psql_bm25s_text_checked_mul_size(
    size_t left,
    size_t right,
    size_t *result_out
)
{
    if (result_out == NULL)
    {
        return false;
    }

    if (left == 0 || right == 0)
    {
        *result_out = 0;
        return true;
    }

    if (left > SIZE_MAX / right)
    {
        return false;
    }

    *result_out = left * right;
    return true;
}

static psql_bm25s_status
psql_bm25s_text_alloc_uchar_buffer(
    int32_t len,
    UChar **buffer_out
)
{
    size_t bytes;
    UChar *buffer;

    if (buffer_out == NULL || len < 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (!psql_bm25s_text_checked_mul_size(
            (size_t) len + 1,
            sizeof(*buffer),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }

    buffer = malloc(bytes);
    if (buffer == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }
    memset(buffer, 0, bytes);
    *buffer_out = buffer;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_text_icu_status_to_result(UErrorCode status)
{
    if (U_SUCCESS(status))
    {
        return PSQL_BM25S_OK;
    }

    switch (status)
    {
        case U_BUFFER_OVERFLOW_ERROR:
        case U_INDEX_OUTOFBOUNDS_ERROR:
        case U_MEMORY_ALLOCATION_ERROR:
            return PSQL_BM25S_ERR_RANGE;
        case U_INVALID_CHAR_FOUND:
        case U_TRUNCATED_CHAR_FOUND:
        case U_ILLEGAL_CHAR_FOUND:
            return PSQL_BM25S_ERR_FORMAT;
        default:
            return PSQL_BM25S_ERR_INVALID;
    }
}

static psql_bm25s_status
psql_bm25s_text_utf8_to_utf16(
    const char *input,
    UChar **utf16_out,
    int32_t *len_out
)
{
    UErrorCode status;
    UChar *utf16;
    int32_t len;
    psql_bm25s_status result;

    if (input == NULL || utf16_out == NULL || len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = U_ZERO_ERROR;
    u_strFromUTF8(NULL, 0, &len, input, -1, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_alloc_uchar_buffer(len, &utf16);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    u_strFromUTF8(utf16, len + 1, &len, input, -1, &status);
    if (U_FAILURE(status))
    {
        free(utf16);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    *utf16_out = utf16;
    *len_out = len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_text_utf16_to_utf8(
    const UChar *input,
    int32_t len,
    char **utf8_out
)
{
    UErrorCode status;
    char *utf8;
    int32_t utf8_len;
    size_t bytes;

    if (input == NULL || utf8_out == NULL || len < 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = U_ZERO_ERROR;
    u_strToUTF8(NULL, 0, &utf8_len, input, len, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        return psql_bm25s_text_icu_status_to_result(status);
    }

    if (!psql_bm25s_text_checked_mul_size(
            (size_t) utf8_len + 1,
            sizeof(*utf8),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }

    utf8 = malloc(bytes);
    if (utf8 == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    status = U_ZERO_ERROR;
    u_strToUTF8(utf8, utf8_len, &utf8_len, input, len, &status);
    if (U_FAILURE(status))
    {
        free(utf8);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    utf8[utf8_len] = '\0';
    *utf8_out = utf8;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_text_normalize_utf16(
    const UNormalizer2 *normalizer,
    const UChar *input,
    int32_t input_len,
    UChar **output_out,
    int32_t *output_len_out
)
{
    UErrorCode status;
    UChar *output;
    int32_t output_len;
    psql_bm25s_status result;

    if (normalizer == NULL ||
        input == NULL ||
        output_out == NULL ||
        output_len_out == NULL ||
        input_len < 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = U_ZERO_ERROR;
    output_len = unorm2_normalize(
        normalizer,
        input,
        input_len,
        NULL,
        0,
        &status
    );
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_alloc_uchar_buffer(output_len, &output);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    output_len = unorm2_normalize(
        normalizer,
        input,
        input_len,
        output,
        output_len + 1,
        &status
    );
    if (U_FAILURE(status))
    {
        free(output);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    *output_out = output;
    *output_len_out = output_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_text_case_fold_utf16(
    const UChar *input,
    int32_t input_len,
    UChar **output_out,
    int32_t *output_len_out
)
{
    UErrorCode status;
    UChar *output;
    int32_t output_len;
    psql_bm25s_status result;

    if (input == NULL ||
        output_out == NULL ||
        output_len_out == NULL ||
        input_len < 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = U_ZERO_ERROR;
    output_len = u_strFoldCase(
        NULL,
        0,
        input,
        input_len,
        U_FOLD_CASE_DEFAULT,
        &status
    );
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
    {
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_alloc_uchar_buffer(output_len, &output);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    output_len = u_strFoldCase(
        output,
        output_len + 1,
        input,
        input_len,
        U_FOLD_CASE_DEFAULT,
        &status
    );
    if (U_FAILURE(status))
    {
        free(output);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    *output_out = output;
    *output_len_out = output_len;
    return PSQL_BM25S_OK;
}

static bool
psql_bm25s_text_is_diacritic(UChar32 codepoint)
{
    int8_t category = u_charType(codepoint);

    return category == U_NON_SPACING_MARK ||
           category == U_COMBINING_SPACING_MARK ||
           category == U_ENCLOSING_MARK;
}

static psql_bm25s_status
psql_bm25s_text_strip_diacritics_utf16(
    const UChar *input,
    int32_t input_len,
    UChar **output_out,
    int32_t *output_len_out
)
{
    const UNormalizer2 *nfd;
    const UNormalizer2 *nfc;
    UErrorCode status;
    UChar *nfd_text;
    int32_t nfd_len;
    UChar *stripped;
    int32_t stripped_len;
    int32_t cursor;
    psql_bm25s_status result;

    if (input == NULL ||
        output_out == NULL ||
        output_len_out == NULL ||
        input_len < 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = U_ZERO_ERROR;
    nfd = unorm2_getNFDInstance(&status);
    if (U_FAILURE(status))
    {
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_normalize_utf16(
        nfd,
        input,
        input_len,
        &nfd_text,
        &nfd_len
    );
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    result = psql_bm25s_text_alloc_uchar_buffer(nfd_len, &stripped);
    if (result != PSQL_BM25S_OK)
    {
        free(nfd_text);
        return result;
    }

    stripped_len = 0;
    cursor = 0;
    while (cursor < nfd_len)
    {
        UChar32 codepoint;
        int32_t next;
        int32_t code_units;

        next = cursor;
        U16_NEXT(nfd_text, next, nfd_len, codepoint);
        code_units = next - cursor;
        if (!psql_bm25s_text_is_diacritic(codepoint))
        {
            memcpy(
                stripped + stripped_len,
                nfd_text + cursor,
                (size_t) code_units * sizeof(*stripped)
            );
            stripped_len += code_units;
        }
        cursor = next;
    }
    stripped[stripped_len] = 0;
    free(nfd_text);

    status = U_ZERO_ERROR;
    nfc = unorm2_getNFCInstance(&status);
    if (U_FAILURE(status))
    {
        free(stripped);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_normalize_utf16(
        nfc,
        stripped,
        stripped_len,
        output_out,
        output_len_out
    );
    free(stripped);
    return result;
}

static bool
psql_bm25s_is_stopword(
    const char *token,
    const psql_bm25s_text_options *options
)
{
    size_t i;

    if (token == NULL || options == NULL || options->stopwords == NULL)
    {
        return false;
    }

    for (i = 0; i < options->num_stopwords; i++)
    {
        if (options->stopwords[i] != NULL &&
            strcmp(token, options->stopwords[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

static psql_bm25s_status
psql_bm25s_text_append_token(
    char ***tokens,
    size_t *token_len,
    char *token
)
{
    char **resized;

    if (tokens == NULL || token_len == NULL || token == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    resized = realloc(*tokens, sizeof(**tokens) * (*token_len + 1));
    if (resized == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    *tokens = resized;
    (*tokens)[*token_len] = token;
    *token_len += 1;
    return PSQL_BM25S_OK;
}

void
psql_bm25s_text_options_init(psql_bm25s_text_options *options)
{
    if (options == NULL)
    {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->lowercase = true;
}

psql_bm25s_status
psql_bm25s_normalize_token(
    const char *input,
    const psql_bm25s_text_options *options,
    char **token_out,
    bool *keep_out
)
{
    const psql_bm25s_text_options *effective_options;
    psql_bm25s_text_options defaults;
    const UNormalizer2 *nfc;
    UErrorCode status;
    UChar *utf16 = NULL;
    UChar *normalized = NULL;
    UChar *folded = NULL;
    UChar *stripped = NULL;
    UChar *current;
    int32_t utf16_len;
    int32_t normalized_len;
    int32_t folded_len;
    int32_t stripped_len;
    bool lowercase_output;
    char *token = NULL;
    psql_bm25s_status result;

    if (input == NULL || token_out == NULL || keep_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *token_out = NULL;
    *keep_out = false;

    effective_options = options;
    if (effective_options == NULL)
    {
        psql_bm25s_text_options_init(&defaults);
        effective_options = &defaults;
    }

    result = psql_bm25s_text_utf8_to_utf16(input, &utf16, &utf16_len);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    nfc = unorm2_getNFCInstance(&status);
    if (U_FAILURE(status))
    {
        free(utf16);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_normalize_utf16(
        nfc,
        utf16,
        utf16_len,
        &normalized,
        &normalized_len
    );
    free(utf16);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    current = normalized;
    normalized = NULL;
    lowercase_output = effective_options->lowercase ||
        effective_options->stem_english;
    if (lowercase_output)
    {
        result = psql_bm25s_text_case_fold_utf16(
            current,
            normalized_len,
            &folded,
            &folded_len
        );
        free(current);
        if (result != PSQL_BM25S_OK)
        {
            return result;
        }
        current = folded;
        normalized_len = folded_len;
        folded = NULL;
    }

    if (effective_options->fold_diacritics)
    {
        result = psql_bm25s_text_strip_diacritics_utf16(
            current,
            normalized_len,
            &stripped,
            &stripped_len
        );
        free(current);
        if (result != PSQL_BM25S_OK)
        {
            return result;
        }
        current = stripped;
        normalized_len = stripped_len;
        stripped = NULL;
    }

    result = psql_bm25s_text_utf16_to_utf8(
        current,
        normalized_len,
        &token
    );
    free(current);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    if (psql_bm25s_is_stopword(token, effective_options))
    {
        free(token);
        return PSQL_BM25S_OK;
    }

    if (effective_options->stem_english)
    {
        psql_bm25s_stem_english_porter(token);
    }

    *token_out = token;
    *keep_out = true;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_tokenize_text(
    const char *input,
    const psql_bm25s_text_options *options,
    char ***tokens_out,
    size_t *len_out
)
{
    const UNormalizer2 *nfc;
    UErrorCode status;
    UChar *utf16 = NULL;
    UChar *normalized = NULL;
    int32_t utf16_len;
    int32_t normalized_len;
    UBreakIterator *iterator = NULL;
    int32_t start;
    int32_t end;
    char **tokens = NULL;
    size_t token_len = 0;
    psql_bm25s_status result;

    if (input == NULL || tokens_out == NULL || len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *tokens_out = NULL;
    *len_out = 0;

    result = psql_bm25s_text_utf8_to_utf16(input, &utf16, &utf16_len);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    nfc = unorm2_getNFCInstance(&status);
    if (U_FAILURE(status))
    {
        free(utf16);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    result = psql_bm25s_text_normalize_utf16(
        nfc,
        utf16,
        utf16_len,
        &normalized,
        &normalized_len
    );
    free(utf16);
    if (result != PSQL_BM25S_OK)
    {
        return result;
    }

    status = U_ZERO_ERROR;
    iterator = ubrk_open(
        UBRK_WORD,
        NULL,
        normalized,
        normalized_len,
        &status
    );
    if (U_FAILURE(status))
    {
        free(normalized);
        return psql_bm25s_text_icu_status_to_result(status);
    }

    start = ubrk_first(iterator);
    end = ubrk_next(iterator);
    while (end != UBRK_DONE)
    {
        int32_t rule_status;

        rule_status = ubrk_getRuleStatus(iterator);
        if (rule_status >= UBRK_WORD_NONE_LIMIT)
        {
            char *raw = NULL;
            char *token = NULL;
            bool keep = false;

            result = psql_bm25s_text_utf16_to_utf8(
                normalized + start,
                end - start,
                &raw
            );
            if (result != PSQL_BM25S_OK)
            {
                ubrk_close(iterator);
                free(normalized);
                psql_bm25s_text_tokens_free(tokens, token_len);
                return result;
            }

            result = psql_bm25s_normalize_token(
                raw,
                options,
                &token,
                &keep
            );
            free(raw);
            if (result != PSQL_BM25S_OK)
            {
                ubrk_close(iterator);
                free(normalized);
                psql_bm25s_text_tokens_free(tokens, token_len);
                return result;
            }

            if (keep)
            {
                result = psql_bm25s_text_append_token(
                    &tokens,
                    &token_len,
                    token
                );
                if (result != PSQL_BM25S_OK)
                {
                    free(token);
                    ubrk_close(iterator);
                    free(normalized);
                    psql_bm25s_text_tokens_free(tokens, token_len);
                    return result;
                }
            }
        }

        start = end;
        end = ubrk_next(iterator);
    }

    ubrk_close(iterator);
    free(normalized);
    *tokens_out = tokens;
    *len_out = token_len;
    return PSQL_BM25S_OK;
}

void
psql_bm25s_text_tokens_free(char **tokens, size_t len)
{
    size_t i;

    if (tokens == NULL)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        free(tokens[i]);
    }
    free(tokens);
}

psql_bm25s_status
psql_bm25s_normalize_query(
    psql_bm25s_query *query,
    const psql_bm25s_text_options *options
)
{
    psql_bm25s_query_term *terms;
    size_t *old_to_new;
    size_t out_len;
    size_t i;

    if (query == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    terms = query->terms;
    old_to_new = malloc(
        sizeof(*old_to_new) * (query->len > 0 ? query->len : (size_t) 1)
    );
    if (old_to_new == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    out_len = 0;
    for (i = 0; i < query->len; i++)
    {
        psql_bm25s_query_term *term;
        size_t out_tokens;
        size_t j;

        term = &terms[i];
        out_tokens = 0;
        for (j = 0; j < term->len; j++)
        {
            char *normalized;
            bool keep;
            psql_bm25s_status result;

            normalized = NULL;
            keep = false;
            result = psql_bm25s_normalize_token(
                term->tokens[j],
                options,
                &normalized,
                &keep
            );
            if (result != PSQL_BM25S_OK)
            {
                free(old_to_new);
                return result;
            }

            free(term->tokens[j]);
            term->tokens[j] = normalized;
            if (keep)
            {
                term->tokens[out_tokens++] = term->tokens[j];
                if (term->token_lens != NULL)
                {
                    term->token_lens[out_tokens - 1] = strlen(normalized);
                }
            }
        }

        term->len = out_tokens;
        if (term->len > 0)
        {
            old_to_new[i] = out_len;
            terms[out_len++] = *term;
        }
        else
        {
            old_to_new[i] = SIZE_MAX;
            free(term->tokens);
            free(term->token_lens);
        }
    }

    query->len = out_len;
    if (out_len == 0)
    {
        free(query->terms);
        query->terms = NULL;
    }
    else
    {
        psql_bm25s_query_term *shrunk;

        shrunk = realloc(query->terms, sizeof(*shrunk) * out_len);
        if (shrunk != NULL)
        {
            query->terms = shrunk;
        }
    }

    psql_bm25s_query_rewrite_terms(query, old_to_new, i);
    free(old_to_new);

    return PSQL_BM25S_OK;
}
