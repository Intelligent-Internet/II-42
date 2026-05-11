#include "psql_bm25s_core.h"
#include "psql_bm25s_query.h"
#include "psql_bm25s_storage.h"
#include "psql_bm25s_text.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(expr)                                                       \
    do                                                                          \
    {                                                                           \
        if (!(expr))                                                            \
        {                                                                       \
            fprintf(stderr, "assertion failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #expr);                                 \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define ASSERT_STATUS_OK(expr)                                                  \
    do                                                                          \
    {                                                                           \
        psql_bm25s_status _status = (expr);                                     \
        if (_status != PSQL_BM25S_OK)                                           \
        {                                                                       \
            fprintf(stderr, "unexpected status at %s:%d: %s (%s)\n",            \
                    __FILE__, __LINE__, #expr, psql_bm25s_strerror(_status));   \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

static void
assert_float_close(float actual, float expected)
{
    float diff = fabsf(actual - expected);
    if (diff > 1e-5f)
    {
        fprintf(stderr,
                "float mismatch: expected %.8f, got %.8f\n",
                expected,
                actual);
        exit(1);
    }
}

static void
assert_uint32_array(
    const uint32_t *actual,
    const uint32_t *expected,
    size_t len
)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (actual[i] != expected[i])
        {
            fprintf(stderr,
                    "uint32 mismatch at %zu: expected %u, got %u\n",
                    i,
                    expected[i],
                    actual[i]);
            exit(1);
        }
    }
}

static void
assert_uint64_array(
    const uint64_t *actual,
    const uint64_t *expected,
    size_t len
)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (actual[i] != expected[i])
        {
            fprintf(stderr,
                    "uint64 mismatch at %zu: expected %llu, got %llu\n",
                    i,
                    (unsigned long long) expected[i],
                    (unsigned long long) actual[i]);
            exit(1);
        }
    }
}

static void
assert_float_array(
    const float *actual,
    const float *expected,
    size_t len
)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        assert_float_close(actual[i], expected[i]);
    }
}

static void
assert_uint32_array_present(
    const uint32_t *actual,
    const uint32_t *expected,
    size_t len
)
{
    ASSERT_TRUE(actual != NULL);
    assert_uint32_array(actual, expected, len);
}

static psql_bm25s_doc_ids
make_doc(uint32_t *ids, size_t len)
{
    psql_bm25s_doc_ids doc;

    doc.token_ids = ids;
    doc.len = len;
    return doc;
}

static void
assert_index_layout_equal(
    const psql_bm25s_index *actual,
    const psql_bm25s_index *expected
)
{
    uint32_t i;

    ASSERT_TRUE(actual->num_docs == expected->num_docs);
    ASSERT_TRUE(actual->vocab_size == expected->vocab_size);
    ASSERT_TRUE(actual->data_len == expected->data_len);
    ASSERT_TRUE(actual->has_empty_token == expected->has_empty_token);
    ASSERT_TRUE(actual->empty_token_id == expected->empty_token_id);
    assert_float_array(actual->data, expected->data, expected->data_len);
    assert_uint32_array(actual->indices, expected->indices, expected->data_len);
    assert_uint64_array(
        actual->indptr,
        expected->indptr,
        (size_t) expected->vocab_size + 1
    );
    assert_uint32_array_present(
        actual->term_frequencies,
        expected->term_frequencies,
        expected->data_len
    );
    assert_uint32_array_present(
        actual->doc_lengths,
        expected->doc_lengths,
        expected->num_docs
    );
    assert_uint32_array_present(
        actual->doc_frequencies,
        expected->doc_frequencies,
        expected->vocab_size
    );
    if (expected->nonoccurrence == NULL)
    {
        ASSERT_TRUE(actual->nonoccurrence == NULL);
    }
    else
    {
        assert_float_array(
            actual->nonoccurrence,
            expected->nonoccurrence,
            expected->vocab_size
        );
    }
    if (expected->vocab == NULL)
    {
        ASSERT_TRUE(actual->vocab == NULL);
    }
    else
    {
        ASSERT_TRUE(actual->vocab != NULL);
        for (i = 0; i < expected->vocab_size; i++)
        {
            ASSERT_TRUE(strcmp(actual->vocab[i], expected->vocab[i]) == 0);
        }
    }
}

static void
write_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t) (value & 0xFFU);
    dst[1] = (uint8_t) ((value >> 8) & 0xFFU);
    dst[2] = (uint8_t) ((value >> 16) & 0xFFU);
    dst[3] = (uint8_t) ((value >> 24) & 0xFFU);
}

static void
write_u64_le(uint8_t *dst, uint64_t value)
{
    size_t i;

    for (i = 0; i < 8; i++)
    {
        dst[i] = (uint8_t) ((value >> (i * 8)) & 0xFFU);
    }
}

static void
test_lucene_index_layout(void)
{
    uint32_t doc0[] = {0, 0, 1};
    uint32_t doc1[] = {1, 2};
    uint32_t doc2[] = {0, 2, 2};
    uint32_t doc3[] = {3};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 3),
        make_doc(doc1, 2),
        make_doc(doc2, 3),
        make_doc(doc3, 1)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index index;
    float expected_data[] = {
        0.35775340f,
        0.24109468f,
        0.24109468f,
        0.29185143f,
        0.29185143f,
        0.35775340f,
        0.64211881f
    };
    uint32_t expected_indices[] = {0, 2, 0, 1, 1, 2, 3};
    uint64_t expected_indptr[] = {0, 2, 4, 6, 7, 7};
    uint32_t expected_doc_lengths[] = {3, 2, 3, 1};
    uint32_t expected_doc_frequencies[] = {2, 2, 2, 1, 0};
    uint32_t expected_term_frequencies[] = {2, 1, 1, 1, 1, 2, 1};

    psql_bm25s_index_init(&index);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        4,
        &params,
        true,
        &index
    ));

    ASSERT_TRUE(index.num_docs == 4);
    ASSERT_TRUE(index.vocab_size == 5);
    ASSERT_TRUE(index.has_empty_token);
    ASSERT_TRUE(index.empty_token_id == 4);
    assert_float_array(index.data, expected_data, 7);
    assert_uint32_array(index.indices, expected_indices, 7);
    assert_uint64_array(index.indptr, expected_indptr, 6);
    assert_uint32_array_present(index.doc_lengths, expected_doc_lengths, 4);
    assert_uint32_array_present(
        index.doc_frequencies,
        expected_doc_frequencies,
        5
    );
    assert_uint32_array_present(
        index.term_frequencies,
        expected_term_frequencies,
        7
    );
    ASSERT_TRUE(index.nonoccurrence == NULL);

    psql_bm25s_index_free(&index);
}

static void
test_compact_id_builder_matches_standard(void)
{
    uint32_t doc0[] = {0, 0, 1};
    uint32_t doc1[] = {1, 2};
    uint32_t doc2[] = {0, 2, 2};
    uint32_t doc3[] = {3};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 3),
        make_doc(doc1, 2),
        make_doc(doc2, 3),
        make_doc(doc3, 1)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_BM25PLUS,
        .idf_method = PSQL_BM25S_METHOD_BM25PLUS
    };
    psql_bm25s_index standard;
    psql_bm25s_index compact;

    psql_bm25s_index_init(&standard);
    psql_bm25s_index_init(&compact);

    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        4,
        &params,
        true,
        &standard
    ));
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids_compact(
        docs,
        4,
        &params,
        true,
        &compact
    ));
    assert_index_layout_equal(&compact, &standard);

    psql_bm25s_index_free(&standard);
    psql_bm25s_index_free(&compact);
}

static void
test_compact_token_builder_matches_standard(void)
{
    const char *doc0[] = {"alpha", "alpha", "beta"};
    const char *doc1[] = {"beta", "gamma"};
    const char *doc2[] = {"alpha", "gamma", "gamma"};
    psql_bm25s_doc_tokens docs[] = {
        {doc0, 3},
        {doc1, 2},
        {doc2, 3}
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index standard;
    psql_bm25s_index compact;

    psql_bm25s_index_init(&standard);
    psql_bm25s_index_init(&compact);

    ASSERT_STATUS_OK(psql_bm25s_build_index_from_tokens(
        docs,
        3,
        &params,
        &standard
    ));
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_tokens_compact(
        docs,
        3,
        &params,
        &compact
    ));
    assert_index_layout_equal(&compact, &standard);

    psql_bm25s_index_free(&standard);
    psql_bm25s_index_free(&compact);
}

typedef struct limited_term_entry_reader
{
    const psql_bm25s_term_entry *entries;
    uint64_t len;
    uint64_t pos;
} limited_term_entry_reader;

static psql_bm25s_status
limited_term_entry_read(void *ctx, psql_bm25s_term_entry *entry_out)
{
    limited_term_entry_reader *reader = ctx;

    if (reader == NULL || entry_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (reader->pos >= reader->len)
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    *entry_out = reader->entries[reader->pos++];
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
limited_term_entry_rewind(void *ctx)
{
    limited_term_entry_reader *reader = ctx;

    if (reader == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    reader->pos = 0;
    return PSQL_BM25S_OK;
}

static void
test_compact_builders_reject_invalid_inputs(void)
{
    const char *empty_tokens[] = {NULL};
    psql_bm25s_doc_tokens empty_docs[] = {
        {empty_tokens, 0}
    };
    uint32_t doc_lengths[] = {1};
    psql_bm25s_term_entry entry = {
        .token_id = 0,
        .doc_id = 0,
        .tf = 1
    };
    limited_term_entry_reader short_reader = {
        .entries = &entry,
        .len = 0,
        .pos = 0
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index index;

    psql_bm25s_index_init(&index);
    ASSERT_TRUE(psql_bm25s_build_index_from_tokens_compact(
        NULL,
        1,
        &params,
        &index
    ) == PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_build_index_from_tokens_compact(
        empty_docs,
        0,
        &params,
        &index
    ) == PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_build_index_from_tokens_compact(
        empty_docs,
        1,
        &params,
        &index
    ) == PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_build_index_from_term_entries(
        NULL,
        1,
        doc_lengths,
        1,
        1,
        &params,
        false,
        false,
        0,
        NULL,
        &index
    ) == PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_build_index_from_term_entry_reader(
        1,
        limited_term_entry_read,
        limited_term_entry_rewind,
        &short_reader,
        doc_lengths,
        1,
        1,
        &params,
        false,
        false,
        0,
        NULL,
        &index
    ) == PSQL_BM25S_ERR_RANGE);
}

static void
test_bm25plus_scores_and_weight_mask(void)
{
    uint32_t doc0[] = {0, 0, 1};
    uint32_t doc1[] = {1, 2};
    uint32_t doc2[] = {0, 2, 2};
    uint32_t doc3[] = {3};
    uint32_t query[] = {0, 2};
    float weight_mask[] = {1.0f, 0.0f, 1.0f, 0.0f};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 3),
        make_doc(doc1, 2),
        make_doc(doc2, 3),
        make_doc(doc3, 1)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_BM25PLUS,
        .idf_method = PSQL_BM25S_METHOD_BM25PLUS
    };
    psql_bm25s_index index;
    float *scores = NULL;
    psql_bm25s_topk_result topk;
    float expected_nonocc[] = {
        0.45814538f,
        0.45814538f,
        0.45814538f,
        0.80471897f,
        0.0f
    };
    uint32_t expected_doc_ids[] = {2, 0};
    float expected_scores[] = {2.89537597f, 2.09860134f};

    psql_bm25s_index_init(&index);
    memset(&topk, 0, sizeof(topk));
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        4,
        &params,
        true,
        &index
    ));
    assert_float_array(index.nonoccurrence, expected_nonocc, 5);

    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &index,
        query,
        2,
        weight_mask,
        &scores
    ));
    ASSERT_STATUS_OK(psql_bm25s_topk(scores, index.num_docs, 2, true, &topk));
    assert_uint32_array(topk.doc_ids, expected_doc_ids, 2);
    assert_float_array(topk.scores, expected_scores, 2);

    free(scores);
    psql_bm25s_topk_result_free(&topk);
    psql_bm25s_index_free(&index);
}

static void
test_token_index_and_query_mapping(void)
{
    const char *doc0[] = {"cat", "cat", "feline"};
    const char *doc1[] = {"dog", "friend"};
    const char *doc2[] = {"cat", "bird", "bird"};
    psql_bm25s_doc_tokens docs[] = {
        {.tokens = doc0, .len = 3},
        {.tokens = doc1, .len = 2},
        {.tokens = doc2, .len = 3}
    };
    const char *query_tokens[] = {"bird", "cat", "missing"};
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index index;
    uint32_t *query_ids = NULL;
    size_t query_len = 0;
    float *scores = NULL;
    psql_bm25s_topk_result topk;

    psql_bm25s_index_init(&index);
    memset(&topk, 0, sizeof(topk));
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_tokens(
        docs,
        3,
        &params,
        &index
    ));
    ASSERT_TRUE(index.vocab != NULL);
    ASSERT_TRUE(index.has_empty_token == false);

    ASSERT_STATUS_OK(psql_bm25s_query_token_ids(
        &index,
        query_tokens,
        3,
        &query_ids,
        &query_len
    ));
    ASSERT_TRUE(query_len == 2);
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &index,
        query_ids,
        query_len,
        NULL,
        &scores
    ));
    ASSERT_STATUS_OK(psql_bm25s_topk(scores, index.num_docs, 2, true, &topk));
    ASSERT_TRUE(topk.doc_ids[0] == 2);
    ASSERT_TRUE(topk.doc_ids[1] == 0);

    free(query_ids);
    free(scores);
    psql_bm25s_topk_result_free(&topk);
    psql_bm25s_index_free(&index);
}

static void
test_exact_stats_scoring_matches_dense_ids(void)
{
    uint32_t doc0[] = {0, 0, 1};
    uint32_t doc1[] = {1, 2};
    uint32_t doc2[] = {0, 2, 2};
    uint32_t doc3[] = {3};
    uint32_t query[] = {0, 2};
    float weight_mask[] = {1.0f, 0.5f, 1.0f, 1.0f};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 3),
        make_doc(doc1, 2),
        make_doc(doc2, 3),
        make_doc(doc3, 1)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_BM25L,
        .idf_method = PSQL_BM25S_METHOD_BM25L
    };
    psql_bm25s_index index;
    float *dense_scores = NULL;
    float *exact_scores = NULL;

    psql_bm25s_index_init(&index);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        4,
        &params,
        true,
        &index
    ));
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &index,
        query,
        2,
        weight_mask,
        &dense_scores
    ));
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids_exact_stats(
        &index,
        query,
        2,
        weight_mask,
        &exact_scores
    ));
    assert_float_array(exact_scores, dense_scores, index.num_docs);

    free(dense_scores);
    free(exact_scores);
    psql_bm25s_index_free(&index);
}

static void
test_exact_stats_scoring_matches_dense_tokens(void)
{
    const char *doc0[] = {"cat", "cat", "feline"};
    const char *doc1[] = {"dog", "friend"};
    const char *doc2[] = {"cat", "bird", "bird"};
    const char *query_tokens[] = {"bird", "cat", "missing"};
    psql_bm25s_doc_tokens docs[] = {
        {.tokens = doc0, .len = 3},
        {.tokens = doc1, .len = 2},
        {.tokens = doc2, .len = 3}
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index index;
    uint32_t *query_ids = NULL;
    size_t query_len = 0;
    float *dense_scores = NULL;
    float *exact_scores = NULL;

    psql_bm25s_index_init(&index);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_tokens(
        docs,
        3,
        &params,
        &index
    ));
    ASSERT_STATUS_OK(psql_bm25s_query_token_ids(
        &index,
        query_tokens,
        3,
        &query_ids,
        &query_len
    ));
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &index,
        query_ids,
        query_len,
        NULL,
        &dense_scores
    ));
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids_exact_stats(
        &index,
        query_ids,
        query_len,
        NULL,
        &exact_scores
    ));
    assert_float_array(exact_scores, dense_scores, index.num_docs);

    free(query_ids);
    free(dense_scores);
    free(exact_scores);
    psql_bm25s_index_free(&index);
}

static void
test_serialization_roundtrip(void)
{
    uint32_t doc0[] = {0, 0, 1};
    uint32_t doc1[] = {1, 2};
    uint32_t doc2[] = {0, 2, 2};
    uint32_t doc3[] = {3};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 3),
        make_doc(doc1, 2),
        make_doc(doc2, 3),
        make_doc(doc3, 1)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_BM25L,
        .idf_method = PSQL_BM25S_METHOD_BM25L
    };
    psql_bm25s_index original;
    psql_bm25s_index restored;
    uint8_t *bytes = NULL;
    size_t len = 0;
    uint32_t query[] = {0, 2};
    float *scores_a = NULL;
    float *scores_b = NULL;

    psql_bm25s_index_init(&original);
    psql_bm25s_index_init(&restored);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        4,
        &params,
        true,
        &original
    ));
    ASSERT_STATUS_OK(psql_bm25s_serialize_index(&original, &bytes, &len));
    ASSERT_STATUS_OK(psql_bm25s_deserialize_index(bytes, len, &restored));

    ASSERT_TRUE(restored.num_docs == original.num_docs);
    ASSERT_TRUE(restored.vocab_size == original.vocab_size);
    ASSERT_TRUE(restored.data_len == original.data_len);
    assert_float_array(restored.data, original.data, original.data_len);
    assert_uint32_array(restored.indices, original.indices, original.data_len);
    assert_uint64_array(restored.indptr, original.indptr, original.vocab_size + 1);
    assert_uint32_array_present(
        restored.term_frequencies,
        original.term_frequencies,
        original.data_len
    );
    assert_uint32_array_present(
        restored.doc_lengths,
        original.doc_lengths,
        original.num_docs
    );
    assert_uint32_array_present(
        restored.doc_frequencies,
        original.doc_frequencies,
        original.vocab_size
    );
    assert_float_array(
        restored.nonoccurrence,
        original.nonoccurrence,
        original.vocab_size
    );

    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &original,
        query,
        2,
        NULL,
        &scores_a
    ));
    ASSERT_STATUS_OK(psql_bm25s_scores_from_ids(
        &restored,
        query,
        2,
        NULL,
        &scores_b
    ));
    assert_float_array(scores_a, scores_b, original.num_docs);

    free(bytes);
    free(scores_a);
    free(scores_b);
    psql_bm25s_index_free(&original);
    psql_bm25s_index_free(&restored);
}

typedef struct test_stream_buffer
{
    uint8_t *bytes;
    size_t len;
    size_t capacity;
} test_stream_buffer;

static psql_bm25s_status
test_stream_write(void *ctx, const uint8_t *bytes, size_t len)
{
    test_stream_buffer *buffer = ctx;

    if (buffer->len + len > buffer->capacity)
    {
        size_t new_capacity = buffer->capacity == 0
            ? 128
            : buffer->capacity * 2;
        uint8_t *new_bytes;

        while (buffer->len + len > new_capacity)
        {
            new_capacity *= 2;
        }
        new_bytes = realloc(buffer->bytes, new_capacity);
        if (new_bytes == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        buffer->bytes = new_bytes;
        buffer->capacity = new_capacity;
    }
    memcpy(buffer->bytes + buffer->len, bytes, len);
    buffer->len += len;
    return PSQL_BM25S_OK;
}

static void
test_stream_serialization_matches_buffered(void)
{
    const char *doc0[] = {"alpha", "alpha", "beta"};
    const char *doc1[] = {"beta", "gamma"};
    const char *doc2[] = {"alpha", "delta", "gamma"};
    psql_bm25s_doc_tokens docs[] = {
        {doc0, 3},
        {doc1, 2},
        {doc2, 3}
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index index;
    test_stream_buffer streamed = {0};
    uint8_t *buffered = NULL;
    size_t buffered_len = 0;
    size_t streamed_len = 0;

    psql_bm25s_index_init(&index);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_tokens(
        docs,
        3,
        &params,
        &index
    ));
    ASSERT_STATUS_OK(psql_bm25s_serialize_index(
        &index,
        &buffered,
        &buffered_len
    ));
    ASSERT_STATUS_OK(psql_bm25s_serialize_index_stream(
        &index,
        test_stream_write,
        &streamed,
        &streamed_len
    ));
    ASSERT_TRUE(streamed_len == buffered_len);
    ASSERT_TRUE(streamed.len == buffered_len);
    ASSERT_TRUE(memcmp(streamed.bytes, buffered, buffered_len) == 0);

    free(streamed.bytes);
    free(buffered);
    psql_bm25s_index_free(&index);
}

static void
test_deserialize_rejects_corrupt_postings(void)
{
    uint32_t doc0[] = {0, 1};
    uint32_t doc1[] = {1, 2};
    psql_bm25s_doc_ids docs[] = {
        make_doc(doc0, 2),
        make_doc(doc1, 2)
    };
    psql_bm25s_params params = {
        .k1 = 1.5f,
        .b = 0.75f,
        .delta = 0.5f,
        .method = PSQL_BM25S_METHOD_LUCENE,
        .idf_method = PSQL_BM25S_METHOD_LUCENE
    };
    psql_bm25s_index original;
    psql_bm25s_index restored;
    uint8_t *bytes = NULL;
    size_t len = 0;
    size_t indices_offset;
    size_t indptr_offset;
    size_t final_indptr_offset;
    const size_t header_size = PSQL_BM25S_HEADER_SIZE;

    psql_bm25s_index_init(&original);
    psql_bm25s_index_init(&restored);
    ASSERT_STATUS_OK(psql_bm25s_build_index_from_ids(
        docs,
        2,
        &params,
        false,
        &original
    ));
    ASSERT_STATUS_OK(psql_bm25s_serialize_index(&original, &bytes, &len));

    indices_offset = header_size + (size_t) original.data_len * sizeof(float);
    indptr_offset = indices_offset + (size_t) original.data_len * sizeof(uint32_t);
    final_indptr_offset = indptr_offset + (size_t) original.vocab_size * sizeof(uint64_t);

    ASSERT_TRUE(final_indptr_offset + sizeof(uint64_t) <= len);
    ASSERT_TRUE(indices_offset + sizeof(uint32_t) <= len);

    write_u64_le(bytes + final_indptr_offset, original.data_len + 1);
    ASSERT_TRUE(psql_bm25s_deserialize_index(bytes, len, &restored) ==
                PSQL_BM25S_ERR_FORMAT);

    free(bytes);
    bytes = NULL;
    ASSERT_STATUS_OK(psql_bm25s_serialize_index(&original, &bytes, &len));
    write_u32_le(bytes + indices_offset, original.num_docs);
    ASSERT_TRUE(psql_bm25s_deserialize_index(bytes, len, &restored) ==
                PSQL_BM25S_ERR_FORMAT);

    free(bytes);
    psql_bm25s_index_free(&original);
    psql_bm25s_index_free(&restored);
}

static void
test_topk_numpy_compatible_shape(void)
{
    float scores[] = {1.0f, 5.0f, 3.0f, 2.0f, 4.0f};
    psql_bm25s_topk_result sorted_result;
    psql_bm25s_topk_result unsorted_result;
    uint32_t expected_ids[] = {1, 4, 2};
    float expected_scores[] = {5.0f, 4.0f, 3.0f};
    size_t i;

    memset(&sorted_result, 0, sizeof(sorted_result));
    memset(&unsorted_result, 0, sizeof(unsorted_result));

    ASSERT_STATUS_OK(psql_bm25s_topk(scores, 5, 3, true, &sorted_result));
    assert_uint32_array(sorted_result.doc_ids, expected_ids, 3);
    assert_float_array(sorted_result.scores, expected_scores, 3);

    ASSERT_STATUS_OK(psql_bm25s_topk(scores, 5, 3, false, &unsorted_result));
    ASSERT_TRUE(unsorted_result.len == 3);
    for (i = 0; i < 3; i++)
    {
        bool seen = false;
        size_t j;

        for (j = 0; j < 3; j++)
        {
            if (unsorted_result.doc_ids[i] == expected_ids[j])
            {
                seen = true;
            }
        }
        ASSERT_TRUE(seen);
    }

    psql_bm25s_topk_result_free(&sorted_result);
    psql_bm25s_topk_result_free(&unsorted_result);
}

static void
test_topk_subset_buffered_keeps_stable_threshold(void)
{
    float scores[] = {100.0f, 50.0f, 40.0f, 60.0f, 55.0f, 10.0f};
    uint32_t candidate_doc_ids[] = {0, 1, 2, 3, 4, 5};
    psql_bm25s_topk_result result;
    uint32_t expected_ids[] = {0, 3, 4};
    float expected_scores[] = {100.0f, 60.0f, 55.0f};

    memset(&result, 0, sizeof(result));

    ASSERT_STATUS_OK(psql_bm25s_topk_subset(
        scores,
        candidate_doc_ids,
        6,
        3,
        true,
        false,
        &result
    ));

    ASSERT_TRUE(result.len == 3);
    assert_uint32_array(result.doc_ids, expected_ids, 3);
    assert_float_array(result.scores, expected_scores, 3);

    psql_bm25s_topk_result_free(&result);
}

static void
test_query_parser_basic_terms(void)
{
    psql_bm25s_query query;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("cat bird", &query));

    ASSERT_TRUE(query.len == 2);
    ASSERT_TRUE(query.terms[0].kind == PSQL_BM25S_QUERY_TERM);
    ASSERT_TRUE(query.terms[0].occur == PSQL_BM25S_QUERY_SHOULD);
    ASSERT_TRUE(query.terms[0].len == 1);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "cat") == 0);
    ASSERT_TRUE(query.terms[1].kind == PSQL_BM25S_QUERY_TERM);
    ASSERT_TRUE(query.terms[1].occur == PSQL_BM25S_QUERY_SHOULD);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "bird") == 0);

    psql_bm25s_query_free(&query);
}

static void
test_query_parser_occurs_prefix_and_phrase(void)
{
    psql_bm25s_query query;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "+must -omit pref* \"exact phrase here\"",
        &query
    ));

    ASSERT_TRUE(query.len == 4);

    ASSERT_TRUE(query.terms[0].occur == PSQL_BM25S_QUERY_MUST);
    ASSERT_TRUE(query.terms[0].kind == PSQL_BM25S_QUERY_TERM);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "must") == 0);

    ASSERT_TRUE(query.terms[1].occur == PSQL_BM25S_QUERY_MUST_NOT);
    ASSERT_TRUE(query.terms[1].kind == PSQL_BM25S_QUERY_TERM);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "omit") == 0);

    ASSERT_TRUE(query.terms[2].occur == PSQL_BM25S_QUERY_SHOULD);
    ASSERT_TRUE(query.terms[2].kind == PSQL_BM25S_QUERY_PREFIX);
    ASSERT_TRUE(strcmp(query.terms[2].tokens[0], "pref") == 0);

    ASSERT_TRUE(query.terms[3].occur == PSQL_BM25S_QUERY_SHOULD);
    ASSERT_TRUE(query.terms[3].kind == PSQL_BM25S_QUERY_PHRASE);
    ASSERT_TRUE(query.terms[3].len == 3);
    ASSERT_TRUE(strcmp(query.terms[3].tokens[0], "exact") == 0);
    ASSERT_TRUE(strcmp(query.terms[3].tokens[1], "phrase") == 0);
    ASSERT_TRUE(strcmp(query.terms[3].tokens[2], "here") == 0);

    psql_bm25s_query_free(&query);
}

static void
test_query_parser_textual_boolean_aliases(void)
{
    psql_bm25s_query query;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "cat OR bird AND fish",
        &query
    ));

    ASSERT_TRUE(psql_bm25s_query_uses_boolean_ast(&query));
    ASSERT_TRUE(query.len == 3);
    ASSERT_TRUE(query.root != NULL);
    ASSERT_TRUE(query.root->kind == PSQL_BM25S_QUERY_NODE_OR);
    ASSERT_TRUE(query.root->left != NULL);
    ASSERT_TRUE(query.root->left->kind == PSQL_BM25S_QUERY_NODE_TERM);
    ASSERT_TRUE(query.root->right != NULL);
    ASSERT_TRUE(query.root->right->kind == PSQL_BM25S_QUERY_NODE_AND);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "cat") == 0);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "bird") == 0);
    ASSERT_TRUE(strcmp(query.terms[2].tokens[0], "fish") == 0);
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "cat AND (bird OR NOT omit)",
        &query
    ));
    ASSERT_TRUE(psql_bm25s_query_uses_boolean_ast(&query));
    ASSERT_TRUE(query.len == 3);
    ASSERT_TRUE(query.root != NULL);
    ASSERT_TRUE(query.root->kind == PSQL_BM25S_QUERY_NODE_AND);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "cat") == 0);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "bird") == 0);
    ASSERT_TRUE(strcmp(query.terms[2].tokens[0], "omit") == 0);
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("cat and bird", &query));
    ASSERT_TRUE(!psql_bm25s_query_uses_boolean_ast(&query));
    ASSERT_TRUE(query.len == 3);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "and") == 0);
    psql_bm25s_query_free(&query);
}

static void
test_query_parser_invalid_inputs(void)
{
    psql_bm25s_query query;

    psql_bm25s_query_init(&query);
    ASSERT_TRUE(psql_bm25s_parse_query_string("+", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("\"unterminated", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("*", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("\"   \"", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("cat AND", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("OR cat", &query) ==
                PSQL_BM25S_ERR_INVALID);
    ASSERT_TRUE(psql_bm25s_parse_query_string("(cat OR bird", &query) ==
                PSQL_BM25S_ERR_INVALID);
    psql_bm25s_query_free(&query);
}

static void
test_query_parser_simple_term_detection(void)
{
    psql_bm25s_query query;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("cat bird", &query));
    ASSERT_TRUE(psql_bm25s_query_is_simple_term_query(&query));
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("+cat", &query));
    ASSERT_TRUE(!psql_bm25s_query_is_simple_term_query(&query));
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("cat*", &query));
    ASSERT_TRUE(!psql_bm25s_query_is_simple_term_query(&query));
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("\"cat bird\"", &query));
    ASSERT_TRUE(!psql_bm25s_query_is_simple_term_query(&query));
    psql_bm25s_query_free(&query);

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string("cat OR bird", &query));
    ASSERT_TRUE(psql_bm25s_query_is_simple_term_query(&query));
    psql_bm25s_query_free(&query);
}

static void
test_text_normalize_token(void)
{
    const char *stopwords[] = {"the", "and"};
    psql_bm25s_text_options options;
    char *token = NULL;
    bool keep = false;

    psql_bm25s_text_options_init(&options);
    options.stopwords = stopwords;
    options.num_stopwords = 2;

    ASSERT_STATUS_OK(psql_bm25s_normalize_token(
        "Cat",
        &options,
        &token,
        &keep
    ));
    ASSERT_TRUE(keep);
    ASSERT_TRUE(strcmp(token, "cat") == 0);
    free(token);

    ASSERT_STATUS_OK(psql_bm25s_normalize_token(
        "The",
        &options,
        &token,
        &keep
    ));
    ASSERT_TRUE(!keep);
    ASSERT_TRUE(token == NULL);
}

static void
test_text_normalize_query(void)
{
    const char *stopwords[] = {"the"};
    psql_bm25s_text_options options;
    psql_bm25s_query query;

    psql_bm25s_text_options_init(&options);
    options.stopwords = stopwords;
    options.num_stopwords = 1;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "+Cat \"The Bird\" -And",
        &query
    ));
    ASSERT_STATUS_OK(psql_bm25s_normalize_query(&query, &options));

    ASSERT_TRUE(query.len == 3);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "cat") == 0);
    ASSERT_TRUE(query.terms[1].kind == PSQL_BM25S_QUERY_PHRASE);
    ASSERT_TRUE(query.terms[1].len == 1);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "bird") == 0);
    ASSERT_TRUE(strcmp(query.terms[2].tokens[0], "and") == 0);

    psql_bm25s_query_free(&query);
}

static void
test_text_normalize_boolean_query(void)
{
    const char *stopwords[] = {"the"};
    psql_bm25s_text_options options;
    psql_bm25s_query query;

    psql_bm25s_text_options_init(&options);
    options.stopwords = stopwords;
    options.num_stopwords = 1;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "Cat AND (The OR Bird)",
        &query
    ));
    ASSERT_TRUE(psql_bm25s_query_uses_boolean_ast(&query));
    ASSERT_STATUS_OK(psql_bm25s_normalize_query(&query, &options));

    ASSERT_TRUE(query.len == 2);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "cat") == 0);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "bird") == 0);
    ASSERT_TRUE(query.root != NULL);
    ASSERT_TRUE(query.root->kind == PSQL_BM25S_QUERY_NODE_AND);

    psql_bm25s_query_free(&query);
}

static void
test_text_tokenize_text(void)
{
    const char *stopwords[] = {"and"};
    psql_bm25s_text_options options;
    char **tokens = NULL;
    size_t len = 0;

    psql_bm25s_text_options_init(&options);
    options.stopwords = stopwords;
    options.num_stopwords = 1;

    ASSERT_STATUS_OK(psql_bm25s_tokenize_text(
        "Cat, and dog! \316\262eta",
        &options,
        &tokens,
        &len
    ));

    ASSERT_TRUE(len == 3);
    ASSERT_TRUE(strcmp(tokens[0], "cat") == 0);
    ASSERT_TRUE(strcmp(tokens[1], "dog") == 0);
    ASSERT_TRUE(strcmp(tokens[2], "\316\262eta") == 0);

    psql_bm25s_text_tokens_free(tokens, len);
}

static void
test_text_normalize_token_with_stemming(void)
{
    psql_bm25s_text_options options;
    char *token = NULL;
    bool keep = false;

    psql_bm25s_text_options_init(&options);
    options.lowercase = false;
    options.stem_english = true;

    ASSERT_STATUS_OK(psql_bm25s_normalize_token(
        "Running",
        &options,
        &token,
        &keep
    ));
    ASSERT_TRUE(keep);
    ASSERT_TRUE(strcmp(token, "run") == 0);
    free(token);
}

static void
test_text_normalize_query_with_stemming(void)
{
    psql_bm25s_text_options options;
    psql_bm25s_query query;

    psql_bm25s_text_options_init(&options);
    options.lowercase = false;
    options.stem_english = true;

    psql_bm25s_query_init(&query);
    ASSERT_STATUS_OK(psql_bm25s_parse_query_string(
        "Running \"Cats Running\"",
        &query
    ));
    ASSERT_STATUS_OK(psql_bm25s_normalize_query(&query, &options));

    ASSERT_TRUE(query.len == 2);
    ASSERT_TRUE(strcmp(query.terms[0].tokens[0], "run") == 0);
    ASSERT_TRUE(query.terms[1].kind == PSQL_BM25S_QUERY_PHRASE);
    ASSERT_TRUE(query.terms[1].len == 2);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[0], "cat") == 0);
    ASSERT_TRUE(strcmp(query.terms[1].tokens[1], "run") == 0);

    psql_bm25s_query_free(&query);
}

static void
test_text_tokenize_text_with_stemming(void)
{
    psql_bm25s_text_options options;
    char **tokens = NULL;
    size_t len = 0;

    psql_bm25s_text_options_init(&options);
    options.stem_english = true;

    ASSERT_STATUS_OK(psql_bm25s_tokenize_text(
        "Running runs quickly",
        &options,
        &tokens,
        &len
    ));

    ASSERT_TRUE(len == 3);
    ASSERT_TRUE(strcmp(tokens[0], "run") == 0);
    ASSERT_TRUE(strcmp(tokens[1], "run") == 0);
    ASSERT_TRUE(strcmp(tokens[2], "quickli") == 0);

    psql_bm25s_text_tokens_free(tokens, len);
}

static void
test_text_normalize_token_with_diacritic_folding(void)
{
    psql_bm25s_text_options options;
    char *token = NULL;
    bool keep = false;

    psql_bm25s_text_options_init(&options);
    options.fold_diacritics = true;

    ASSERT_STATUS_OK(psql_bm25s_normalize_token(
        "Stra\303\237e",
        &options,
        &token,
        &keep
    ));
    ASSERT_TRUE(keep);
    ASSERT_TRUE(strcmp(token, "strasse") == 0);
    free(token);
}

static void
test_text_tokenize_text_with_diacritic_folding(void)
{
    psql_bm25s_text_options options;
    char **tokens = NULL;
    size_t len = 0;

    psql_bm25s_text_options_init(&options);
    options.fold_diacritics = true;

    ASSERT_STATUS_OK(psql_bm25s_tokenize_text(
        "\303\234ber caf\303\251 fa\303\247ade",
        &options,
        &tokens,
        &len
    ));

    ASSERT_TRUE(len == 3);
    ASSERT_TRUE(strcmp(tokens[0], "uber") == 0);
    ASSERT_TRUE(strcmp(tokens[1], "cafe") == 0);
    ASSERT_TRUE(strcmp(tokens[2], "facade") == 0);

    psql_bm25s_text_tokens_free(tokens, len);
}

int
main(void)
{
    test_lucene_index_layout();
    test_compact_id_builder_matches_standard();
    test_compact_token_builder_matches_standard();
    test_compact_builders_reject_invalid_inputs();
    test_bm25plus_scores_and_weight_mask();
    test_token_index_and_query_mapping();
    test_exact_stats_scoring_matches_dense_ids();
    test_exact_stats_scoring_matches_dense_tokens();
    test_serialization_roundtrip();
    test_stream_serialization_matches_buffered();
    test_deserialize_rejects_corrupt_postings();
    test_topk_numpy_compatible_shape();
    test_topk_subset_buffered_keeps_stable_threshold();
    test_query_parser_basic_terms();
    test_query_parser_occurs_prefix_and_phrase();
    test_query_parser_textual_boolean_aliases();
    test_query_parser_invalid_inputs();
    test_query_parser_simple_term_detection();
    test_text_normalize_token();
    test_text_normalize_query();
    test_text_normalize_boolean_query();
    test_text_tokenize_text();
    test_text_normalize_token_with_stemming();
    test_text_normalize_query_with_stemming();
    test_text_tokenize_text_with_stemming();
    test_text_normalize_token_with_diacritic_folding();
    test_text_tokenize_text_with_diacritic_folding();
    puts("all unit tests passed");
    return 0;
}
