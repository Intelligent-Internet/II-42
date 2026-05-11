#include "psql_bm25s_core.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#if !defined(PSQL_BM25S_FORCE_SCALAR) && defined(__GNUC__) && \
    (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#define PSQL_BM25S_HAVE_X86_AVX2_DISPATCH 1
#endif

typedef struct psql_bm25s_term_freq
{
    uint32_t token_id;
    uint32_t tf;
} psql_bm25s_term_freq;

typedef struct psql_bm25s_term_freqs
{
    psql_bm25s_term_freq *terms;
    size_t len;
} psql_bm25s_term_freqs;

typedef struct psql_bm25s_strmap_slot
{
    char *key;
    uint32_t value;
    bool used;
} psql_bm25s_strmap_slot;

typedef struct psql_bm25s_strmap
{
    psql_bm25s_strmap_slot *slots;
    size_t size;
    size_t capacity;
} psql_bm25s_strmap;

typedef struct psql_bm25s_heap_item
{
    float score;
    uint32_t doc_id;
} psql_bm25s_heap_item;

typedef void (*psql_bm25s_apply_weight_mask_fn)(
    float *scores,
    const float *weight_mask,
    size_t len
);

typedef void (*psql_bm25s_add_constant_fn)(
    float *scores,
    float value,
    size_t len
);

typedef enum psql_bm25s_simd_override
{
    PSQL_BM25S_SIMD_OVERRIDE_AUTO = 0,
    PSQL_BM25S_SIMD_OVERRIDE_SCALAR = 1,
    PSQL_BM25S_SIMD_OVERRIDE_AVX2 = 2
} psql_bm25s_simd_override;

typedef enum psql_bm25s_simd_path
{
    PSQL_BM25S_SIMD_PATH_SCALAR = 0,
    PSQL_BM25S_SIMD_PATH_AVX2 = 1
} psql_bm25s_simd_path;

#ifdef PSQL_BM25S_HAVE_X86_AVX2_DISPATCH
static psql_bm25s_simd_override
psql_bm25s_simd_override_from_env(void)
{
    const char *mode = getenv("PSQL_BM25S_SIMD_MODE");

    if (mode == NULL || mode[0] == '\0' || strcmp(mode, "auto") == 0)
    {
        return PSQL_BM25S_SIMD_OVERRIDE_AUTO;
    }
    if (strcmp(mode, "scalar") == 0 || strcmp(mode, "fallback") == 0)
    {
        return PSQL_BM25S_SIMD_OVERRIDE_SCALAR;
    }
    if (strcmp(mode, "avx2") == 0)
    {
        return PSQL_BM25S_SIMD_OVERRIDE_AVX2;
    }

    return PSQL_BM25S_SIMD_OVERRIDE_AUTO;
}
#endif

static void
psql_bm25s_apply_weight_mask_scalar(
    float *scores,
    const float *weight_mask,
    size_t len
)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        scores[i] *= weight_mask[i];
    }
}

static void
psql_bm25s_add_constant_scalar(float *scores, float value, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        scores[i] += value;
    }
}

#ifdef PSQL_BM25S_HAVE_X86_AVX2_DISPATCH
__attribute__((target("avx2")))
static void
psql_bm25s_apply_weight_mask_avx2(
    float *scores,
    const float *weight_mask,
    size_t len
)
{
    size_t i = 0;
    size_t vec_len = len & ~((size_t) 7);

    for (; i < vec_len; i += 8)
    {
        __m256 scores_vec = _mm256_loadu_ps(scores + i);
        __m256 mask_vec = _mm256_loadu_ps(weight_mask + i);
        scores_vec = _mm256_mul_ps(scores_vec, mask_vec);
        _mm256_storeu_ps(scores + i, scores_vec);
    }

    for (; i < len; i++)
    {
        scores[i] *= weight_mask[i];
    }
}

__attribute__((target("avx2")))
static void
psql_bm25s_add_constant_avx2(float *scores, float value, size_t len)
{
    size_t i = 0;
    size_t vec_len = len & ~((size_t) 7);
    __m256 value_vec = _mm256_set1_ps(value);

    for (; i < vec_len; i += 8)
    {
        __m256 scores_vec = _mm256_loadu_ps(scores + i);
        scores_vec = _mm256_add_ps(scores_vec, value_vec);
        _mm256_storeu_ps(scores + i, scores_vec);
    }

    for (; i < len; i++)
    {
        scores[i] += value;
    }
}

static bool
psql_bm25s_runtime_has_avx2(void)
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
}
#endif

static psql_bm25s_apply_weight_mask_fn psql_bm25s_apply_weight_mask_impl =
    psql_bm25s_apply_weight_mask_scalar;
static psql_bm25s_add_constant_fn psql_bm25s_add_constant_impl =
    psql_bm25s_add_constant_scalar;
static psql_bm25s_simd_path psql_bm25s_simd_active_path =
    PSQL_BM25S_SIMD_PATH_SCALAR;
static bool psql_bm25s_simd_dispatch_initialized = false;

static void
psql_bm25s_simd_dispatch_init(void)
{
#ifdef PSQL_BM25S_HAVE_X86_AVX2_DISPATCH
    psql_bm25s_simd_override override_mode = PSQL_BM25S_SIMD_OVERRIDE_AUTO;
#endif

    if (psql_bm25s_simd_dispatch_initialized)
    {
        return;
    }

#ifdef PSQL_BM25S_HAVE_X86_AVX2_DISPATCH
    override_mode = psql_bm25s_simd_override_from_env();

    if (override_mode != PSQL_BM25S_SIMD_OVERRIDE_SCALAR &&
        psql_bm25s_runtime_has_avx2())
    {
        psql_bm25s_apply_weight_mask_impl = psql_bm25s_apply_weight_mask_avx2;
        psql_bm25s_add_constant_impl = psql_bm25s_add_constant_avx2;
        psql_bm25s_simd_active_path = PSQL_BM25S_SIMD_PATH_AVX2;
    }
#endif

    psql_bm25s_simd_dispatch_initialized = true;
}

static inline void
psql_bm25s_apply_weight_mask_inplace(
    float *scores,
    const float *weight_mask,
    size_t len
)
{
    if (weight_mask == NULL || len == 0)
    {
        return;
    }

    psql_bm25s_simd_dispatch_init();
    psql_bm25s_apply_weight_mask_impl(scores, weight_mask, len);
}

static inline void
psql_bm25s_add_constant_inplace(float *scores, float value, size_t len)
{
    if (value == 0.0f || len == 0)
    {
        return;
    }

    psql_bm25s_simd_dispatch_init();
    psql_bm25s_add_constant_impl(scores, value, len);
}

static bool
psql_bm25s_method_requires_nonoccurrence(psql_bm25s_method method)
{
    return method == PSQL_BM25S_METHOD_BM25L ||
           method == PSQL_BM25S_METHOD_BM25PLUS;
}

const char *
psql_bm25s_strerror(psql_bm25s_status status)
{
    switch (status)
    {
        case PSQL_BM25S_OK:
            return "ok";
        case PSQL_BM25S_ERR_NOMEM:
            return "out of memory";
        case PSQL_BM25S_ERR_INVALID:
            return "invalid input";
        case PSQL_BM25S_ERR_RANGE:
            return "value out of range";
        case PSQL_BM25S_ERR_FORMAT:
            return "invalid serialized format";
    }

    return "unknown error";
}

bool
psql_bm25s_parse_method(const char *name, psql_bm25s_method *method_out)
{
    if (name == NULL || method_out == NULL)
    {
        return false;
    }

    if (strcmp(name, "robertson") == 0)
    {
        *method_out = PSQL_BM25S_METHOD_ROBERTSON;
        return true;
    }
    if (strcmp(name, "lucene") == 0)
    {
        *method_out = PSQL_BM25S_METHOD_LUCENE;
        return true;
    }
    if (strcmp(name, "atire") == 0)
    {
        *method_out = PSQL_BM25S_METHOD_ATIRE;
        return true;
    }
    if (strcmp(name, "bm25l") == 0)
    {
        *method_out = PSQL_BM25S_METHOD_BM25L;
        return true;
    }
    if (strcmp(name, "bm25+") == 0)
    {
        *method_out = PSQL_BM25S_METHOD_BM25PLUS;
        return true;
    }

    return false;
}

const char *
psql_bm25s_method_name(psql_bm25s_method method)
{
    switch (method)
    {
        case PSQL_BM25S_METHOD_ROBERTSON:
            return "robertson";
        case PSQL_BM25S_METHOD_LUCENE:
            return "lucene";
        case PSQL_BM25S_METHOD_ATIRE:
            return "atire";
        case PSQL_BM25S_METHOD_BM25L:
            return "bm25l";
        case PSQL_BM25S_METHOD_BM25PLUS:
            return "bm25+";
    }

    return "unknown";
}

const char *
psql_bm25s_active_simd_path(void)
{
    psql_bm25s_simd_dispatch_init();
    if (psql_bm25s_simd_active_path == PSQL_BM25S_SIMD_PATH_AVX2)
    {
        return "avx2";
    }
    return "scalar";
}

void
psql_bm25s_index_init(psql_bm25s_index *index)
{
    if (index == NULL)
    {
        return;
    }

    memset(index, 0, sizeof(*index));
}

void
psql_bm25s_index_free(psql_bm25s_index *index)
{
    uint32_t i;

    if (index == NULL)
    {
        return;
    }

    free(index->data);
    free(index->indices);
    free(index->indptr);
    free(index->term_frequencies);
    free(index->doc_lengths);
    free(index->doc_frequencies);
    free(index->nonoccurrence);

    if (index->vocab != NULL)
    {
        for (i = 0; i < index->vocab_size; i++)
        {
            free(index->vocab[i]);
        }
    }
    free(index->vocab);

    memset(index, 0, sizeof(*index));
}

void
psql_bm25s_topk_result_free(psql_bm25s_topk_result *result)
{
    if (result == NULL)
    {
        return;
    }

    free(result->doc_ids);
    free(result->scores);
    memset(result, 0, sizeof(*result));
}

static bool
psql_bm25s_checked_mul_size(size_t a, size_t b, size_t *out)
{
    if (a == 0 || b == 0)
    {
        *out = 0;
        return true;
    }

    if (a > ((size_t) -1) / b)
    {
        return false;
    }

    *out = a * b;
    return true;
}

static uint64_t
psql_bm25s_hash_bytes(const char *s)
{
    uint64_t hash = 1469598103934665603ULL;
    const unsigned char *ptr = (const unsigned char *) s;

    while (*ptr != '\0')
    {
        hash ^= (uint64_t) *ptr;
        hash *= 1099511628211ULL;
        ptr++;
    }

    return hash;
}

static void
psql_bm25s_strmap_destroy(psql_bm25s_strmap *map)
{
    size_t i;

    if (map == NULL)
    {
        return;
    }

    if (map->slots != NULL)
    {
        for (i = 0; i < map->capacity; i++)
        {
            free(map->slots[i].key);
        }
    }

    free(map->slots);
    memset(map, 0, sizeof(*map));
}

static psql_bm25s_status
psql_bm25s_strmap_init(psql_bm25s_strmap *map, size_t min_capacity)
{
    size_t capacity = 8;

    if (map == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    while (capacity < min_capacity)
    {
        if (capacity > ((size_t) -1) / 2)
        {
            return PSQL_BM25S_ERR_RANGE;
        }
        capacity *= 2;
    }

    map->slots = calloc(capacity, sizeof(*map->slots));
    if (map->slots == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    map->capacity = capacity;
    map->size = 0;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_strmap_rehash(psql_bm25s_strmap *map, size_t new_capacity)
{
    psql_bm25s_strmap_slot *old_slots;
    size_t old_capacity;
    size_t i;

    old_slots = map->slots;
    old_capacity = map->capacity;
    map->slots = calloc(new_capacity, sizeof(*map->slots));
    if (map->slots == NULL)
    {
        map->slots = old_slots;
        return PSQL_BM25S_ERR_NOMEM;
    }

    map->capacity = new_capacity;
    map->size = 0;

    for (i = 0; i < old_capacity; i++)
    {
        psql_bm25s_strmap_slot slot = old_slots[i];

        if (!slot.used)
        {
            continue;
        }

        {
            uint64_t hash = psql_bm25s_hash_bytes(slot.key);
            size_t mask = map->capacity - 1;
            size_t idx = (size_t) hash & mask;

            while (map->slots[idx].used)
            {
                idx = (idx + 1) & mask;
            }

            map->slots[idx] = slot;
            map->slots[idx].used = true;
            map->size++;
        }
    }

    free(old_slots);
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_strmap_ensure_capacity(psql_bm25s_strmap *map)
{
    if ((map->size + 1) * 10 < map->capacity * 7)
    {
        return PSQL_BM25S_OK;
    }

    if (map->capacity > ((size_t) -1) / 2)
    {
        return PSQL_BM25S_ERR_RANGE;
    }

    return psql_bm25s_strmap_rehash(map, map->capacity * 2);
}

static char *
psql_bm25s_strdup(const char *s)
{
    size_t len = strlen(s);
    char *copy = malloc(len + 1);

    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

static psql_bm25s_status
psql_bm25s_strmap_get(
    const psql_bm25s_strmap *map,
    const char *key,
    uint32_t *value_out,
    bool *found_out
)
{
    uint64_t hash;
    size_t mask;
    size_t idx;

    if (map == NULL || key == NULL || found_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    if (map->capacity == 0)
    {
        *found_out = false;
        return PSQL_BM25S_OK;
    }

    hash = psql_bm25s_hash_bytes(key);
    mask = map->capacity - 1;
    idx = (size_t) hash & mask;

    while (map->slots[idx].used)
    {
        if (strcmp(map->slots[idx].key, key) == 0)
        {
            *found_out = true;
            if (value_out != NULL)
            {
                *value_out = map->slots[idx].value;
            }
            return PSQL_BM25S_OK;
        }

        idx = (idx + 1) & mask;
    }

    *found_out = false;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_strmap_put(
    psql_bm25s_strmap *map,
    const char *key,
    uint32_t value
)
{
    uint64_t hash;
    size_t mask;
    size_t idx;
    psql_bm25s_status status;

    status = psql_bm25s_strmap_ensure_capacity(map);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    hash = psql_bm25s_hash_bytes(key);
    mask = map->capacity - 1;
    idx = (size_t) hash & mask;

    while (map->slots[idx].used)
    {
        if (strcmp(map->slots[idx].key, key) == 0)
        {
            map->slots[idx].value = value;
            return PSQL_BM25S_OK;
        }

        idx = (idx + 1) & mask;
    }

    map->slots[idx].key = psql_bm25s_strdup(key);
    if (map->slots[idx].key == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }
    map->slots[idx].value = value;
    map->slots[idx].used = true;
    map->size++;
    return PSQL_BM25S_OK;
}

static int
psql_bm25s_cmp_uint32(const void *lhs, const void *rhs)
{
    uint32_t a = *(const uint32_t *) lhs;
    uint32_t b = *(const uint32_t *) rhs;

    if (a < b)
    {
        return -1;
    }
    if (a > b)
    {
        return 1;
    }
    return 0;
}

static void
psql_bm25s_term_freqs_free(psql_bm25s_term_freqs *term_freqs)
{
    if (term_freqs == NULL)
    {
        return;
    }

    free(term_freqs->terms);
    term_freqs->terms = NULL;
    term_freqs->len = 0;
}

static psql_bm25s_status
psql_bm25s_count_doc_terms(
    const uint32_t *token_ids,
    size_t len,
    psql_bm25s_term_freqs *term_freqs_out
)
{
    uint32_t *sorted_ids = NULL;
    psql_bm25s_term_freq *terms = NULL;
    size_t i;
    size_t unique_count = 0;
    size_t bytes;

    term_freqs_out->terms = NULL;
    term_freqs_out->len = 0;

    if (len == 0)
    {
        return PSQL_BM25S_OK;
    }

    if (!psql_bm25s_checked_mul_size(len, sizeof(*sorted_ids), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    sorted_ids = malloc(bytes);
    if (sorted_ids == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    memcpy(sorted_ids, token_ids, bytes);
    qsort(sorted_ids, len, sizeof(*sorted_ids), psql_bm25s_cmp_uint32);

    if (!psql_bm25s_checked_mul_size(len, sizeof(*terms), &bytes))
    {
        free(sorted_ids);
        return PSQL_BM25S_ERR_RANGE;
    }
    terms = malloc(bytes);
    if (terms == NULL)
    {
        free(sorted_ids);
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < len; i++)
    {
        if (unique_count == 0 ||
            terms[unique_count - 1].token_id != sorted_ids[i])
        {
            terms[unique_count].token_id = sorted_ids[i];
            terms[unique_count].tf = 1;
            unique_count++;
        }
        else
        {
            terms[unique_count - 1].tf++;
        }
    }

    free(sorted_ids);
    term_freqs_out->terms = terms;
    term_freqs_out->len = unique_count;
    return PSQL_BM25S_OK;
}

static double
psql_bm25s_score_tfc(
    psql_bm25s_method method,
    double tf,
    double l_d,
    double l_avg,
    double k1,
    double b,
    double delta
)
{
    switch (method)
    {
        case PSQL_BM25S_METHOD_ROBERTSON:
        case PSQL_BM25S_METHOD_LUCENE:
            return tf / (k1 * ((1.0 - b) + (b * l_d / l_avg)) + tf);
        case PSQL_BM25S_METHOD_ATIRE:
            return (tf * (k1 + 1.0)) /
                   (tf + k1 * (1.0 - b + (b * l_d / l_avg)));
        case PSQL_BM25S_METHOD_BM25L:
        {
            double c = tf / (1.0 - b + (b * l_d / l_avg));
            return ((k1 + 1.0) * (c + delta)) / (k1 + c + delta);
        }
        case PSQL_BM25S_METHOD_BM25PLUS:
        {
            double num = (k1 + 1.0) * tf;
            double den = k1 * (1.0 - b + (b * l_d / l_avg)) + tf;
            return (num / den) + delta;
        }
    }

    return 0.0;
}

static double
psql_bm25s_score_idf(
    psql_bm25s_method method,
    double df,
    double n_docs
)
{
    switch (method)
    {
        case PSQL_BM25S_METHOD_ROBERTSON:
        {
            double inner = (n_docs - df + 0.5) / (df + 0.5);
            if (inner < 1.0)
            {
                inner = 1.0;
            }
            return log(inner);
        }
        case PSQL_BM25S_METHOD_LUCENE:
            return log(1.0 + (n_docs - df + 0.5) / (df + 0.5));
        case PSQL_BM25S_METHOD_ATIRE:
            return log(n_docs / df);
        case PSQL_BM25S_METHOD_BM25L:
            return log((n_docs + 1.0) / (df + 0.5));
        case PSQL_BM25S_METHOD_BM25PLUS:
            return log((n_docs + 1.0) / df);
    }

    return 0.0;
}

static psql_bm25s_status
psql_bm25s_build_csc(
    const float *scores,
    const uint32_t *rows,
    const uint32_t *cols,
    const uint32_t *term_freqs,
    uint64_t data_len,
    uint32_t n_cols,
    float **data_out,
    uint32_t **indices_out,
    uint64_t **indptr_out,
    uint32_t **term_freqs_out
)
{
    uint64_t *col_counts = NULL;
    uint64_t *heads = NULL;
    uint64_t *indptr = NULL;
    float *sorted_data = NULL;
    uint32_t *sorted_indices = NULL;
    uint32_t *sorted_term_freqs = NULL;
    size_t bytes;
    uint64_t i;
    uint64_t acc = 0;

    *data_out = NULL;
    *indices_out = NULL;
    *indptr_out = NULL;
    if (term_freqs_out != NULL)
    {
        *term_freqs_out = NULL;
    }

    if (!psql_bm25s_checked_mul_size(
            n_cols,
            sizeof(*col_counts),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    col_counts = calloc(n_cols, sizeof(*col_counts));
    heads = calloc(n_cols, sizeof(*heads));
    if (col_counts == NULL || heads == NULL)
    {
        free(col_counts);
        free(heads);
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (!psql_bm25s_checked_mul_size(
            n_cols + 1,
            sizeof(*indptr),
            &bytes))
    {
        free(col_counts);
        free(heads);
        return PSQL_BM25S_ERR_RANGE;
    }
    indptr = malloc(bytes);
    if (indptr == NULL)
    {
        free(col_counts);
        free(heads);
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (data_len > 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) data_len,
                sizeof(*sorted_data),
                &bytes))
        {
            free(col_counts);
            free(heads);
            free(indptr);
            return PSQL_BM25S_ERR_RANGE;
        }
        sorted_data = malloc(bytes);
        if (sorted_data == NULL)
        {
            free(col_counts);
            free(heads);
            free(indptr);
            return PSQL_BM25S_ERR_NOMEM;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) data_len,
                sizeof(*sorted_indices),
                &bytes))
        {
            free(col_counts);
            free(heads);
            free(indptr);
            free(sorted_data);
            return PSQL_BM25S_ERR_RANGE;
        }
        sorted_indices = malloc(bytes);
        if (sorted_indices == NULL)
        {
            free(col_counts);
            free(heads);
            free(indptr);
            free(sorted_data);
            return PSQL_BM25S_ERR_NOMEM;
        }

        if (term_freqs_out != NULL)
        {
            if (!psql_bm25s_checked_mul_size(
                    (size_t) data_len,
                    sizeof(*sorted_term_freqs),
                    &bytes))
            {
                free(col_counts);
                free(heads);
                free(indptr);
                free(sorted_data);
                free(sorted_indices);
                return PSQL_BM25S_ERR_RANGE;
            }
            sorted_term_freqs = malloc(bytes);
            if (sorted_term_freqs == NULL)
            {
                free(col_counts);
                free(heads);
                free(indptr);
                free(sorted_data);
                free(sorted_indices);
                return PSQL_BM25S_ERR_NOMEM;
            }
        }
    }

    for (i = 0; i < data_len; i++)
    {
        col_counts[cols[i]]++;
    }

    indptr[0] = 0;
    for (i = 0; i < n_cols; i++)
    {
        heads[i] = acc;
        acc += col_counts[i];
        indptr[i + 1] = acc;
    }

    for (i = 0; i < data_len; i++)
    {
        uint32_t col = cols[i];
        uint64_t pos = heads[col];
        sorted_data[pos] = scores[i];
        sorted_indices[pos] = rows[i];
        if (sorted_term_freqs != NULL && term_freqs != NULL)
        {
            sorted_term_freqs[pos] = term_freqs[i];
        }
        heads[col]++;
    }

    free(col_counts);
    free(heads);

    *data_out = sorted_data;
    *indices_out = sorted_indices;
    *indptr_out = indptr;
    if (term_freqs_out != NULL)
    {
        *term_freqs_out = sorted_term_freqs;
    }
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_build_index_internal(
    const psql_bm25s_doc_ids *docs,
    size_t num_docs,
    uint32_t vocab_size,
    const psql_bm25s_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    char **vocab,
    psql_bm25s_index *index_out
)
{
    psql_bm25s_term_freqs *doc_terms = NULL;
    uint32_t *doc_frequencies = NULL;
    float *idf_array = NULL;
    float *nonoccurrence = NULL;
    float *scores = NULL;
    uint32_t *doc_indices = NULL;
    uint32_t *voc_indices = NULL;
    uint32_t *term_frequencies = NULL;
    uint32_t *doc_lengths = NULL;
    size_t i;
    size_t bytes;
    uint64_t array_size = 0;
    double total_doc_len = 0.0;
    double avg_doc_len;
    uint64_t cursor = 0;
    psql_bm25s_status status = PSQL_BM25S_OK;

    if (num_docs == 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    doc_terms = calloc(num_docs, sizeof(*doc_terms));
    if (doc_terms == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (!psql_bm25s_checked_mul_size(num_docs, sizeof(*doc_lengths), &bytes))
    {
        status = PSQL_BM25S_ERR_RANGE;
        goto cleanup;
    }
    doc_lengths = calloc(num_docs, sizeof(*doc_lengths));
    if (doc_lengths == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }

    if (!psql_bm25s_checked_mul_size(vocab_size, sizeof(*doc_frequencies), &bytes))
    {
        status = PSQL_BM25S_ERR_RANGE;
        goto cleanup;
    }
    doc_frequencies = calloc(vocab_size, sizeof(*doc_frequencies));
    if (doc_frequencies == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }

    for (i = 0; i < num_docs; i++)
    {
        size_t j;

        total_doc_len += (double) docs[i].len;
        doc_lengths[i] = (uint32_t) docs[i].len;
        status = psql_bm25s_count_doc_terms(
            docs[i].token_ids,
            docs[i].len,
            &doc_terms[i]
        );
        if (status != PSQL_BM25S_OK)
        {
            goto cleanup;
        }

        array_size += (uint64_t) doc_terms[i].len;
        for (j = 0; j < doc_terms[i].len; j++)
        {
            uint32_t token_id = doc_terms[i].terms[j].token_id;
            if (token_id >= vocab_size)
            {
                status = PSQL_BM25S_ERR_RANGE;
                goto cleanup;
            }
            doc_frequencies[token_id]++;
        }
    }

    avg_doc_len = total_doc_len / (double) num_docs;

    if (!psql_bm25s_checked_mul_size(vocab_size, sizeof(*idf_array), &bytes))
    {
        status = PSQL_BM25S_ERR_RANGE;
        goto cleanup;
    }
    idf_array = calloc(vocab_size, sizeof(*idf_array));
    if (idf_array == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }

    for (i = 0; i < vocab_size; i++)
    {
        if (doc_frequencies[i] != 0)
        {
            idf_array[i] = (float) psql_bm25s_score_idf(
                params->idf_method,
                (double) doc_frequencies[i],
                (double) num_docs
            );
        }
    }

    if (psql_bm25s_method_requires_nonoccurrence(params->method))
    {
        nonoccurrence = calloc(vocab_size, sizeof(*nonoccurrence));
        if (nonoccurrence == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }

        for (i = 0; i < vocab_size; i++)
        {
            if (doc_frequencies[i] != 0)
            {
                double idf = psql_bm25s_score_idf(
                    params->idf_method,
                    (double) doc_frequencies[i],
                    (double) num_docs
                );
                double tfc = psql_bm25s_score_tfc(
                    params->method,
                    0.0,
                    avg_doc_len,
                    avg_doc_len,
                    params->k1,
                    params->b,
                    params->delta
                );
                nonoccurrence[i] = (float) (idf * tfc);
            }
        }
    }

    if (array_size > 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) array_size,
                sizeof(*scores),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        scores = malloc(bytes);
        if (scores == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) array_size,
                sizeof(*doc_indices),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        doc_indices = malloc(bytes);
        if (doc_indices == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) array_size,
                sizeof(*voc_indices),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        voc_indices = malloc(bytes);
        if (voc_indices == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) array_size,
                sizeof(*term_frequencies),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        term_frequencies = malloc(bytes);
        if (term_frequencies == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
    }

    for (i = 0; i < num_docs; i++)
    {
        size_t j;
        double doc_len = (double) docs[i].len;

        for (j = 0; j < doc_terms[i].len; j++)
        {
            uint32_t token_id = doc_terms[i].terms[j].token_id;
            double tf = (double) doc_terms[i].terms[j].tf;
            double tfc = psql_bm25s_score_tfc(
                params->method,
                tf,
                doc_len,
                avg_doc_len,
                params->k1,
                params->b,
                params->delta
            );
            double score = (double) idf_array[token_id] * tfc;

            if (psql_bm25s_method_requires_nonoccurrence(params->method))
            {
                score -= (double) nonoccurrence[token_id];
            }

            scores[cursor] = (float) score;
            doc_indices[cursor] = (uint32_t) i;
            voc_indices[cursor] = token_id;
            term_frequencies[cursor] = doc_terms[i].terms[j].tf;
            cursor++;
        }
    }

    psql_bm25s_index_init(index_out);
    index_out->params = *params;
    index_out->num_docs = (uint32_t) num_docs;
    index_out->vocab_size = vocab_size;
    index_out->data_len = array_size;
    index_out->has_empty_token = create_empty_token && has_empty_token;
    index_out->empty_token_id = empty_token_id;
    index_out->doc_lengths = doc_lengths;
    index_out->doc_frequencies = doc_frequencies;
    index_out->nonoccurrence = nonoccurrence;
    index_out->vocab = vocab;
    doc_lengths = NULL;
    doc_frequencies = NULL;
    nonoccurrence = NULL;
    vocab = NULL;

    status = psql_bm25s_build_csc(
        scores,
        doc_indices,
        voc_indices,
        term_frequencies,
        array_size,
        vocab_size,
        &index_out->data,
        &index_out->indices,
        &index_out->indptr,
        &index_out->term_frequencies
    );
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_index_free(index_out);
        goto cleanup;
    }

cleanup:
    if (doc_terms != NULL)
    {
        for (i = 0; i < num_docs; i++)
        {
            psql_bm25s_term_freqs_free(&doc_terms[i]);
        }
    }
    free(doc_terms);
    free(doc_frequencies);
    free(idf_array);
    free(nonoccurrence);
    free(scores);
    free(doc_indices);
    free(voc_indices);
    free(term_frequencies);
    free(doc_lengths);
    if (vocab != NULL)
    {
        uint32_t j;
        for (j = 0; j < vocab_size; j++)
        {
            free(vocab[j]);
        }
        free(vocab);
    }
    return status;
}

typedef struct psql_bm25s_term_entry_builder
{
    psql_bm25s_term_entry *entries;
    uint64_t len;
    uint64_t capacity;
} psql_bm25s_term_entry_builder;

static void
psql_bm25s_term_entry_builder_free(psql_bm25s_term_entry_builder *builder)
{
    if (builder == NULL)
    {
        return;
    }

    free(builder->entries);
    memset(builder, 0, sizeof(*builder));
}

static psql_bm25s_status
psql_bm25s_term_entry_builder_append(
    psql_bm25s_term_entry_builder *builder,
    uint32_t token_id,
    uint32_t doc_id,
    uint32_t tf
)
{
    size_t bytes;

    if (builder->len == builder->capacity)
    {
        uint64_t new_capacity = builder->capacity == 0
            ? 1024
            : builder->capacity * 2;
        psql_bm25s_term_entry *new_entries;

        if (new_capacity < builder->capacity ||
            new_capacity > (uint64_t) SIZE_MAX)
        {
            return PSQL_BM25S_ERR_RANGE;
        }
        if (!psql_bm25s_checked_mul_size(
                (size_t) new_capacity,
                sizeof(*builder->entries),
                &bytes))
        {
            return PSQL_BM25S_ERR_RANGE;
        }
        new_entries = realloc(builder->entries, bytes);
        if (new_entries == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        builder->entries = new_entries;
        builder->capacity = new_capacity;
    }

    builder->entries[builder->len].token_id = token_id;
    builder->entries[builder->len].doc_id = doc_id;
    builder->entries[builder->len].tf = tf;
    builder->len++;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_copy_vocab(
    const char *const *vocab,
    uint32_t vocab_size,
    char ***vocab_out
)
{
    char **copy = NULL;
    uint32_t i;
    size_t bytes;

    *vocab_out = NULL;
    if (vocab == NULL || vocab_size == 0)
    {
        return PSQL_BM25S_OK;
    }
    if (!psql_bm25s_checked_mul_size(vocab_size, sizeof(*copy), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    copy = calloc(vocab_size, sizeof(*copy));
    if (copy == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }
    for (i = 0; i < vocab_size; i++)
    {
        if (vocab[i] == NULL)
        {
            uint32_t j;

            for (j = 0; j < i; j++)
            {
                free(copy[j]);
            }
            free(copy);
            return PSQL_BM25S_ERR_INVALID;
        }
        copy[i] = psql_bm25s_strdup(vocab[i]);
        if (copy[i] == NULL)
        {
            uint32_t j;

            for (j = 0; j < i; j++)
            {
                free(copy[j]);
            }
            free(copy);
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    *vocab_out = copy;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_build_index_from_term_entry_reader(
    uint64_t num_entries,
    psql_bm25s_term_entry_reader_cb read_cb,
    psql_bm25s_term_entry_rewind_cb rewind_cb,
    void *reader_ctx,
    const uint32_t *doc_lengths_in,
    uint32_t num_docs,
    uint32_t vocab_size,
    const psql_bm25s_params *params,
    bool create_empty_token,
    bool has_empty_token,
    uint32_t empty_token_id,
    const char *const *vocab,
    psql_bm25s_index *index_out
)
{
    uint32_t *doc_lengths = NULL;
    uint32_t *doc_frequencies = NULL;
    float *idf_array = NULL;
    float *nonoccurrence = NULL;
    float *data = NULL;
    uint32_t *indices = NULL;
    uint64_t *indptr = NULL;
    uint64_t *heads = NULL;
    uint32_t *term_frequencies = NULL;
    char **vocab_copy = NULL;
    uint32_t i;
    uint64_t j;
    size_t bytes;
    double total_doc_len = 0.0;
    double avg_doc_len = 0.0;
    psql_bm25s_status status = PSQL_BM25S_OK;

    if (params == NULL || index_out == NULL ||
        (num_docs > 0 && doc_lengths_in == NULL) ||
        (num_entries > 0 && (read_cb == NULL || rewind_cb == NULL)))
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (num_docs == 0 || num_entries > (uint64_t) SIZE_MAX)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (vocab_size == 0 && num_entries > 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (has_empty_token && empty_token_id >= vocab_size)
    {
        return PSQL_BM25S_ERR_RANGE;
    }

    if (!psql_bm25s_checked_mul_size(num_docs, sizeof(*doc_lengths), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    doc_lengths = malloc(bytes);
    if (doc_lengths == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }
    for (i = 0; i < num_docs; i++)
    {
        doc_lengths[i] = doc_lengths_in[i];
        total_doc_len += (double) doc_lengths[i];
    }
    avg_doc_len = total_doc_len / (double) num_docs;

    if (vocab_size > 0)
    {
        if (!psql_bm25s_checked_mul_size(
                vocab_size,
                sizeof(*doc_frequencies),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        doc_frequencies = calloc(vocab_size, sizeof(*doc_frequencies));
        idf_array = calloc(vocab_size, sizeof(*idf_array));
        if (doc_frequencies == NULL || idf_array == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
    }

    for (j = 0; j < num_entries; j++)
    {
        psql_bm25s_term_entry entry;

        status = read_cb(reader_ctx, &entry);
        if (status != PSQL_BM25S_OK)
        {
            goto cleanup;
        }
        if (entry.doc_id >= num_docs || entry.token_id >= vocab_size)
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        if (entry.tf == 0)
        {
            status = PSQL_BM25S_ERR_INVALID;
            goto cleanup;
        }
        doc_frequencies[entry.token_id]++;
    }

    status = rewind_cb(reader_ctx);
    if (status != PSQL_BM25S_OK)
    {
        goto cleanup;
    }

    for (i = 0; i < vocab_size; i++)
    {
        if (doc_frequencies[i] != 0)
        {
            idf_array[i] = (float) psql_bm25s_score_idf(
                params->idf_method,
                (double) doc_frequencies[i],
                (double) num_docs
            );
        }
    }

    if (psql_bm25s_method_requires_nonoccurrence(params->method) &&
        vocab_size > 0)
    {
        nonoccurrence = calloc(vocab_size, sizeof(*nonoccurrence));
        if (nonoccurrence == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
        for (i = 0; i < vocab_size; i++)
        {
            if (doc_frequencies[i] != 0)
            {
                double idf = psql_bm25s_score_idf(
                    params->idf_method,
                    (double) doc_frequencies[i],
                    (double) num_docs
                );
                double tfc = psql_bm25s_score_tfc(
                    params->method,
                    0.0,
                    avg_doc_len,
                    avg_doc_len,
                    params->k1,
                    params->b,
                    params->delta
                );

                nonoccurrence[i] = (float) (idf * tfc);
            }
        }
    }

    if (!psql_bm25s_checked_mul_size(
            (size_t) vocab_size + 1,
            sizeof(*indptr),
            &bytes))
    {
        status = PSQL_BM25S_ERR_RANGE;
        goto cleanup;
    }
    indptr = calloc((size_t) vocab_size + 1, sizeof(*indptr));
    heads = calloc((size_t) vocab_size + 1, sizeof(*heads));
    if (indptr == NULL || heads == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }

    for (j = 0; j < num_entries; j++)
    {
        psql_bm25s_term_entry entry;

        status = read_cb(reader_ctx, &entry);
        if (status != PSQL_BM25S_OK)
        {
            goto cleanup;
        }
        indptr[(size_t) entry.token_id + 1]++;
    }
    status = rewind_cb(reader_ctx);
    if (status != PSQL_BM25S_OK)
    {
        goto cleanup;
    }
    for (i = 0; i < vocab_size; i++)
    {
        indptr[(size_t) i + 1] += indptr[i];
        heads[i] = indptr[i];
    }

    if (num_entries > 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) num_entries,
                sizeof(*data),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        data = malloc(bytes);
        if (data == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
        if (!psql_bm25s_checked_mul_size(
                (size_t) num_entries,
                sizeof(*indices),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        indices = malloc(bytes);
        if (indices == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
        if (!psql_bm25s_checked_mul_size(
                (size_t) num_entries,
                sizeof(*term_frequencies),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        term_frequencies = malloc(bytes);
        if (term_frequencies == NULL)
        {
            status = PSQL_BM25S_ERR_NOMEM;
            goto cleanup;
        }
    }

    for (j = 0; j < num_entries; j++)
    {
        psql_bm25s_term_entry entry;
        uint32_t token_id;
        uint32_t doc_id;
        uint64_t pos;
        double doc_len;
        double tf;
        double tfc;
        double score;

        status = read_cb(reader_ctx, &entry);
        if (status != PSQL_BM25S_OK)
        {
            goto cleanup;
        }
        token_id = entry.token_id;
        doc_id = entry.doc_id;
        pos = heads[token_id]++;
        doc_len = (double) doc_lengths[doc_id];
        tf = (double) entry.tf;
        tfc = psql_bm25s_score_tfc(
            params->method,
            tf,
            doc_len,
            avg_doc_len,
            params->k1,
            params->b,
            params->delta
        );
        score = (double) idf_array[token_id] * tfc;

        if (psql_bm25s_method_requires_nonoccurrence(params->method))
        {
            score -= (double) nonoccurrence[token_id];
        }

        data[pos] = (float) score;
        indices[pos] = doc_id;
        term_frequencies[pos] = entry.tf;
    }

    status = psql_bm25s_copy_vocab(vocab, vocab_size, &vocab_copy);
    if (status != PSQL_BM25S_OK)
    {
        goto cleanup;
    }

    psql_bm25s_index_init(index_out);
    index_out->params = *params;
    index_out->num_docs = num_docs;
    index_out->vocab_size = vocab_size;
    index_out->data_len = num_entries;
    index_out->has_empty_token = create_empty_token && has_empty_token;
    index_out->empty_token_id = empty_token_id;
    index_out->data = data;
    index_out->indices = indices;
    index_out->indptr = indptr;
    index_out->term_frequencies = term_frequencies;
    index_out->doc_lengths = doc_lengths;
    index_out->doc_frequencies = doc_frequencies;
    index_out->nonoccurrence = nonoccurrence;
    index_out->vocab = vocab_copy;

    data = NULL;
    indices = NULL;
    indptr = NULL;
    term_frequencies = NULL;
    doc_lengths = NULL;
    doc_frequencies = NULL;
    nonoccurrence = NULL;
    vocab_copy = NULL;

cleanup:
    free(heads);
    free(data);
    free(indices);
    free(indptr);
    free(term_frequencies);
    free(doc_lengths);
    free(doc_frequencies);
    free(idf_array);
    free(nonoccurrence);
    if (vocab_copy != NULL)
    {
        for (i = 0; i < vocab_size; i++)
        {
            free(vocab_copy[i]);
        }
        free(vocab_copy);
    }
    return status;
}

typedef struct psql_bm25s_memory_term_entry_reader
{
    const psql_bm25s_term_entry *entries;
    uint64_t len;
    uint64_t pos;
} psql_bm25s_memory_term_entry_reader;

static psql_bm25s_status
psql_bm25s_memory_term_entry_read(
    void *ctx,
    psql_bm25s_term_entry *entry_out
)
{
    psql_bm25s_memory_term_entry_reader *reader = ctx;

    if (reader == NULL || entry_out == NULL || reader->entries == NULL)
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
psql_bm25s_memory_term_entry_rewind(void *ctx)
{
    psql_bm25s_memory_term_entry_reader *reader = ctx;

    if (reader == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    reader->pos = 0;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_build_index_from_term_entries(
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
)
{
    psql_bm25s_memory_term_entry_reader reader;

    if (num_entries > 0 && entries == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    reader.entries = entries;
    reader.len = num_entries;
    reader.pos = 0;
    return psql_bm25s_build_index_from_term_entry_reader(
        num_entries,
        psql_bm25s_memory_term_entry_read,
        psql_bm25s_memory_term_entry_rewind,
        &reader,
        doc_lengths,
        num_docs,
        vocab_size,
        params,
        create_empty_token,
        has_empty_token,
        empty_token_id,
        vocab,
        index_out
    );
}

psql_bm25s_status
psql_bm25s_build_index_from_ids(
    const psql_bm25s_doc_ids *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    bool create_empty_token,
    psql_bm25s_index *index_out
)
{
    uint32_t vocab_size = 0;
    uint32_t max_token_id = 0;
    bool has_terms = false;
    bool zero_present = false;
    uint32_t empty_token_id = 0;
    bool has_empty_token = false;
    size_t i;
    size_t j;

    if (docs == NULL || params == NULL || index_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    for (i = 0; i < num_docs; i++)
    {
        for (j = 0; j < docs[i].len; j++)
        {
            uint32_t token_id = docs[i].token_ids[j];
            if (!has_terms || token_id > max_token_id)
            {
                max_token_id = token_id;
            }
            if (token_id == 0)
            {
                zero_present = true;
            }
            has_terms = true;
        }
    }

    if (has_terms)
    {
        vocab_size = max_token_id + 1;
    }

    if (create_empty_token)
    {
        has_empty_token = true;
        if (!zero_present)
        {
            empty_token_id = 0;
        }
        else
        {
            if (!has_terms)
            {
                empty_token_id = 0;
            }
            else if (max_token_id == UINT32_MAX)
            {
                return PSQL_BM25S_ERR_RANGE;
            }
            else
            {
                empty_token_id = max_token_id + 1;
            }
        }

        if (empty_token_id + 1 > vocab_size)
        {
            vocab_size = empty_token_id + 1;
        }
    }

    if (!has_terms && !create_empty_token)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    return psql_bm25s_build_index_internal(
        docs,
        num_docs,
        vocab_size,
        params,
        create_empty_token,
        has_empty_token,
        empty_token_id,
        NULL,
        index_out
    );
}

psql_bm25s_status
psql_bm25s_build_index_from_ids_compact(
    const psql_bm25s_doc_ids *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    bool create_empty_token,
    psql_bm25s_index *index_out
)
{
    uint32_t *doc_lengths = NULL;
    uint32_t vocab_size = 0;
    uint32_t max_token_id = 0;
    bool has_terms = false;
    bool zero_present = false;
    uint32_t empty_token_id = 0;
    bool has_empty_token = false;
    psql_bm25s_term_entry_builder entries = {0};
    size_t bytes;
    size_t i;
    size_t j;
    psql_bm25s_status status = PSQL_BM25S_OK;

    if (docs == NULL || params == NULL || index_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (num_docs == 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (num_docs > UINT32_MAX)
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (!psql_bm25s_checked_mul_size(num_docs, sizeof(*doc_lengths), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    doc_lengths = malloc(bytes);
    if (doc_lengths == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_docs; i++)
    {
        if (docs[i].len > UINT32_MAX)
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        doc_lengths[i] = (uint32_t) docs[i].len;
        for (j = 0; j < docs[i].len; j++)
        {
            uint32_t token_id = docs[i].token_ids[j];

            if (!has_terms || token_id > max_token_id)
            {
                max_token_id = token_id;
            }
            if (token_id == 0)
            {
                zero_present = true;
            }
            has_terms = true;
        }
    }

    if (has_terms)
    {
        vocab_size = max_token_id + 1;
    }
    if (create_empty_token)
    {
        has_empty_token = true;
        if (!zero_present)
        {
            empty_token_id = 0;
        }
        else if (!has_terms)
        {
            empty_token_id = 0;
        }
        else if (max_token_id == UINT32_MAX)
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        else
        {
            empty_token_id = max_token_id + 1;
        }
        if (empty_token_id + 1 > vocab_size)
        {
            vocab_size = empty_token_id + 1;
        }
    }
    if (!has_terms && !create_empty_token)
    {
        status = PSQL_BM25S_ERR_INVALID;
        goto cleanup;
    }

    for (i = 0; i < num_docs; i++)
    {
        psql_bm25s_term_freqs terms = {0};

        status = psql_bm25s_count_doc_terms(
            docs[i].token_ids,
            docs[i].len,
            &terms
        );
        if (status != PSQL_BM25S_OK)
        {
            goto cleanup;
        }
        for (j = 0; j < terms.len; j++)
        {
            status = psql_bm25s_term_entry_builder_append(
                &entries,
                terms.terms[j].token_id,
                (uint32_t) i,
                terms.terms[j].tf
            );
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_term_freqs_free(&terms);
                goto cleanup;
            }
        }
        psql_bm25s_term_freqs_free(&terms);
    }

    status = psql_bm25s_build_index_from_term_entries(
        entries.entries,
        entries.len,
        doc_lengths,
        (uint32_t) num_docs,
        vocab_size,
        params,
        create_empty_token,
        has_empty_token,
        empty_token_id,
        NULL,
        index_out
    );

cleanup:
    free(doc_lengths);
    psql_bm25s_term_entry_builder_free(&entries);
    return status;
}

psql_bm25s_status
psql_bm25s_build_index_from_tokens(
    const psql_bm25s_doc_tokens *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    psql_bm25s_index *index_out
)
{
    psql_bm25s_doc_ids *docs_as_ids = NULL;
    psql_bm25s_strmap vocab_map;
    char **vocab = NULL;
    size_t i;
    psql_bm25s_status status;

    if (docs == NULL || params == NULL || index_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    memset(&vocab_map, 0, sizeof(vocab_map));
    status = psql_bm25s_strmap_init(&vocab_map, 8);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    docs_as_ids = calloc(num_docs, sizeof(*docs_as_ids));
    if (docs_as_ids == NULL)
    {
        psql_bm25s_strmap_destroy(&vocab_map);
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_docs; i++)
    {
        size_t j;
        size_t bytes;

        docs_as_ids[i].len = docs[i].len;
        if (!psql_bm25s_checked_mul_size(
                docs[i].len,
                sizeof(*docs_as_ids[i].token_ids),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        docs_as_ids[i].token_ids = NULL;
        if (docs[i].len > 0)
        {
            docs_as_ids[i].token_ids = malloc(bytes);
            if (docs_as_ids[i].token_ids == NULL)
            {
                status = PSQL_BM25S_ERR_NOMEM;
                goto cleanup;
            }
        }

        for (j = 0; j < docs[i].len; j++)
        {
            uint32_t token_id = 0;
            bool found = false;

            if (docs[i].tokens[j] == NULL)
            {
                status = PSQL_BM25S_ERR_INVALID;
                goto cleanup;
            }

            status = psql_bm25s_strmap_get(
                &vocab_map,
                docs[i].tokens[j],
                &token_id,
                &found
            );
            if (status != PSQL_BM25S_OK)
            {
                goto cleanup;
            }

            if (!found)
            {
                if (vocab_map.size > UINT32_MAX)
                {
                    status = PSQL_BM25S_ERR_RANGE;
                    goto cleanup;
                }
                token_id = (uint32_t) vocab_map.size;
                status = psql_bm25s_strmap_put(
                    &vocab_map,
                    docs[i].tokens[j],
                    token_id
                );
                if (status != PSQL_BM25S_OK)
                {
                    goto cleanup;
                }
            }

            docs_as_ids[i].token_ids[j] = token_id;
        }
    }

    vocab = calloc(vocab_map.size, sizeof(*vocab));
    if (vocab == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }

    for (i = 0; i < vocab_map.capacity; i++)
    {
        if (vocab_map.slots[i].used)
        {
            vocab[vocab_map.slots[i].value] = vocab_map.slots[i].key;
            vocab_map.slots[i].key = NULL;
        }
    }

    status = psql_bm25s_build_index_internal(
        docs_as_ids,
        num_docs,
        (uint32_t) vocab_map.size,
        params,
        false,
        false,
        0,
        vocab,
        index_out
    );
    if (status == PSQL_BM25S_OK)
    {
        vocab = NULL;
    }

cleanup:
    if (docs_as_ids != NULL)
    {
        for (i = 0; i < num_docs; i++)
        {
            free(docs_as_ids[i].token_ids);
        }
    }
    free(docs_as_ids);
    if (vocab != NULL)
    {
        for (i = 0; i < vocab_map.size; i++)
        {
            free(vocab[i]);
        }
    }
    free(vocab);
    psql_bm25s_strmap_destroy(&vocab_map);
    return status;
}

psql_bm25s_status
psql_bm25s_build_index_from_tokens_compact(
    const psql_bm25s_doc_tokens *docs,
    size_t num_docs,
    const psql_bm25s_params *params,
    psql_bm25s_index *index_out
)
{
    psql_bm25s_doc_ids *docs_as_ids = NULL;
    psql_bm25s_strmap vocab_map;
    char **vocab = NULL;
    size_t i;
    psql_bm25s_status status;

    if (docs == NULL || params == NULL || index_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (num_docs == 0)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    memset(&vocab_map, 0, sizeof(vocab_map));
    status = psql_bm25s_strmap_init(&vocab_map, 8);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    docs_as_ids = calloc(num_docs, sizeof(*docs_as_ids));
    if (docs_as_ids == NULL)
    {
        psql_bm25s_strmap_destroy(&vocab_map);
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_docs; i++)
    {
        size_t j;
        size_t bytes;

        docs_as_ids[i].len = docs[i].len;
        if (!psql_bm25s_checked_mul_size(
                docs[i].len,
                sizeof(*docs_as_ids[i].token_ids),
                &bytes))
        {
            status = PSQL_BM25S_ERR_RANGE;
            goto cleanup;
        }
        if (docs[i].len > 0)
        {
            docs_as_ids[i].token_ids = malloc(bytes);
            if (docs_as_ids[i].token_ids == NULL)
            {
                status = PSQL_BM25S_ERR_NOMEM;
                goto cleanup;
            }
        }

        for (j = 0; j < docs[i].len; j++)
        {
            uint32_t token_id = 0;
            bool found = false;

            if (docs[i].tokens[j] == NULL)
            {
                status = PSQL_BM25S_ERR_INVALID;
                goto cleanup;
            }

            status = psql_bm25s_strmap_get(
                &vocab_map,
                docs[i].tokens[j],
                &token_id,
                &found
            );
            if (status != PSQL_BM25S_OK)
            {
                goto cleanup;
            }

            if (!found)
            {
                if (vocab_map.size > UINT32_MAX)
                {
                    status = PSQL_BM25S_ERR_RANGE;
                    goto cleanup;
                }
                token_id = (uint32_t) vocab_map.size;
                status = psql_bm25s_strmap_put(
                    &vocab_map,
                    docs[i].tokens[j],
                    token_id
                );
                if (status != PSQL_BM25S_OK)
                {
                    goto cleanup;
                }
            }

            docs_as_ids[i].token_ids[j] = token_id;
        }
    }

    if (vocab_map.size == 0)
    {
        status = PSQL_BM25S_ERR_INVALID;
        goto cleanup;
    }

    vocab = calloc(vocab_map.size, sizeof(*vocab));
    if (vocab == NULL)
    {
        status = PSQL_BM25S_ERR_NOMEM;
        goto cleanup;
    }
    for (i = 0; i < vocab_map.capacity; i++)
    {
        if (vocab_map.slots[i].used)
        {
            vocab[vocab_map.slots[i].value] = vocab_map.slots[i].key;
            vocab_map.slots[i].key = NULL;
        }
    }

    status = psql_bm25s_build_index_from_ids_compact(
        docs_as_ids,
        num_docs,
        params,
        false,
        index_out
    );
    if (status == PSQL_BM25S_OK)
    {
        index_out->vocab = vocab;
        vocab = NULL;
    }

cleanup:
    if (docs_as_ids != NULL)
    {
        for (i = 0; i < num_docs; i++)
        {
            free(docs_as_ids[i].token_ids);
        }
    }
    free(docs_as_ids);
    if (vocab != NULL)
    {
        for (i = 0; i < vocab_map.size; i++)
        {
            free(vocab[i]);
        }
    }
    free(vocab);
    psql_bm25s_strmap_destroy(&vocab_map);
    return status;
}

psql_bm25s_status
psql_bm25s_query_token_ids(
    const psql_bm25s_index *index,
    const char **tokens,
    size_t num_tokens,
    uint32_t **query_ids_out,
    size_t *query_len_out
)
{
    psql_bm25s_strmap vocab_map;
    uint32_t *query_ids = NULL;
    size_t bytes;
    size_t i;
    psql_bm25s_status status;

    if (index == NULL || query_ids_out == NULL || query_len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *query_ids_out = NULL;
    *query_len_out = 0;

    if (index->vocab == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    memset(&vocab_map, 0, sizeof(vocab_map));
    status = psql_bm25s_strmap_init(
        &vocab_map,
        index->vocab_size > 8 ? index->vocab_size * 2 : 8
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    for (i = 0; i < index->vocab_size; i++)
    {
        status = psql_bm25s_strmap_put(&vocab_map, index->vocab[i], i);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_strmap_destroy(&vocab_map);
            return status;
        }
    }

    if (!psql_bm25s_checked_mul_size(num_tokens, sizeof(*query_ids), &bytes))
    {
        psql_bm25s_strmap_destroy(&vocab_map);
        return PSQL_BM25S_ERR_RANGE;
    }
    if (num_tokens > 0)
    {
        query_ids = malloc(bytes);
        if (query_ids == NULL)
        {
            psql_bm25s_strmap_destroy(&vocab_map);
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    *query_len_out = 0;
    for (i = 0; i < num_tokens; i++)
    {
        uint32_t token_id = 0;
        bool found = false;

        if (tokens[i] == NULL)
        {
            free(query_ids);
            psql_bm25s_strmap_destroy(&vocab_map);
            return PSQL_BM25S_ERR_INVALID;
        }

        status = psql_bm25s_strmap_get(
            &vocab_map,
            tokens[i],
            &token_id,
            &found
        );
        if (status != PSQL_BM25S_OK)
        {
            free(query_ids);
            psql_bm25s_strmap_destroy(&vocab_map);
            return status;
        }

        if (found)
        {
            query_ids[*query_len_out] = token_id;
            (*query_len_out)++;
        }
    }

    psql_bm25s_strmap_destroy(&vocab_map);
    *query_ids_out = query_ids;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_scores_from_ids(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
)
{
    float *scores = NULL;
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t i;
    size_t bytes;
    double nonoccurrence_sum = 0.0;

    if (index == NULL || scores_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *scores_out = NULL;
    if (!psql_bm25s_checked_mul_size(index->num_docs, sizeof(*scores), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    scores = calloc(index->num_docs, sizeof(*scores));
    if (scores == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (query_len > 0)
    {
        if (!psql_bm25s_checked_mul_size(query_len, sizeof(*filtered_ids), &bytes))
        {
            free(scores);
            return PSQL_BM25S_ERR_RANGE;
        }
        filtered_ids = malloc(bytes);
        if (filtered_ids == NULL)
        {
            free(scores);
            return PSQL_BM25S_ERR_NOMEM;
        }

        for (i = 0; i < query_len; i++)
        {
            if (query_ids[i] < index->vocab_size)
            {
                filtered_ids[filtered_len++] = query_ids[i];
            }
        }
    }

    if (filtered_len == 0 && index->has_empty_token)
    {
        filtered_ids = realloc(filtered_ids, sizeof(*filtered_ids));
        if (filtered_ids == NULL)
        {
            free(scores);
            return PSQL_BM25S_ERR_NOMEM;
        }
        filtered_ids[0] = index->empty_token_id;
        filtered_len = 1;
    }

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint64_t start = index->indptr[token_id];
        uint64_t end = index->indptr[token_id + 1];
        uint64_t j;

        for (j = start; j < end; j++)
        {
            scores[index->indices[j]] += index->data[j];
        }
    }

    psql_bm25s_apply_weight_mask_inplace(scores, weight_mask, index->num_docs);

    if (index->nonoccurrence != NULL)
    {
        for (i = 0; i < filtered_len; i++)
        {
            nonoccurrence_sum += (double) index->nonoccurrence[filtered_ids[i]];
        }
        psql_bm25s_add_constant_inplace(
            scores,
            (float) nonoccurrence_sum,
            index->num_docs
        );
    }

    free(filtered_ids);
    *scores_out = scores;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_scores_from_ids_exact_stats(
    const psql_bm25s_index *index,
    const uint32_t *query_ids,
    size_t query_len,
    const float *weight_mask,
    float **scores_out
)
{
    float *scores = NULL;
    uint32_t *filtered_ids = NULL;
    size_t filtered_len = 0;
    size_t i;
    size_t bytes;
    double total_doc_len = 0.0;
    double avg_doc_len = 0.0;

    if (index == NULL || scores_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (index->term_frequencies == NULL ||
        index->doc_lengths == NULL ||
        index->doc_frequencies == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *scores_out = NULL;
    if (!psql_bm25s_checked_mul_size(index->num_docs, sizeof(*scores), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    scores = calloc(index->num_docs, sizeof(*scores));
    if (scores == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (query_len > 0)
    {
        if (!psql_bm25s_checked_mul_size(
                query_len,
                sizeof(*filtered_ids),
                &bytes))
        {
            free(scores);
            return PSQL_BM25S_ERR_RANGE;
        }
        filtered_ids = malloc(bytes);
        if (filtered_ids == NULL)
        {
            free(scores);
            return PSQL_BM25S_ERR_NOMEM;
        }

        for (i = 0; i < query_len; i++)
        {
            if (query_ids[i] < index->vocab_size)
            {
                filtered_ids[filtered_len++] = query_ids[i];
            }
        }
    }

    if (filtered_len == 0 && index->has_empty_token)
    {
        filtered_ids = realloc(filtered_ids, sizeof(*filtered_ids));
        if (filtered_ids == NULL)
        {
            free(scores);
            return PSQL_BM25S_ERR_NOMEM;
        }
        filtered_ids[0] = index->empty_token_id;
        filtered_len = 1;
    }

    for (i = 0; i < index->num_docs; i++)
    {
        total_doc_len += (double) index->doc_lengths[i];
    }
    if (index->num_docs > 0)
    {
        avg_doc_len = total_doc_len / (double) index->num_docs;
    }

    for (i = 0; i < filtered_len; i++)
    {
        uint32_t token_id = filtered_ids[i];
        uint32_t df = index->doc_frequencies[token_id];
        double idf = 0.0;
        double nonocc = 0.0;
        uint64_t start;
        uint64_t end;
        uint64_t j;

        if (df == 0)
        {
            continue;
        }

        idf = psql_bm25s_score_idf(
            index->params.idf_method,
            (double) df,
            (double) index->num_docs
        );
        if (psql_bm25s_method_requires_nonoccurrence(index->params.method))
        {
            nonocc = idf * psql_bm25s_score_tfc(
                index->params.method,
                0.0,
                0.0,
                avg_doc_len,
                index->params.k1,
                index->params.b,
                index->params.delta
            );
        }

        start = index->indptr[token_id];
        end = index->indptr[token_id + 1];
        for (j = start; j < end; j++)
        {
            uint32_t doc_id = index->indices[j];
            double tf = (double) index->term_frequencies[j];
            double doc_len = (double) index->doc_lengths[doc_id];
            double tfc = psql_bm25s_score_tfc(
                index->params.method,
                tf,
                doc_len,
                avg_doc_len,
                index->params.k1,
                index->params.b,
                index->params.delta
            );
            double score = idf * tfc;

            if (psql_bm25s_method_requires_nonoccurrence(index->params.method))
            {
                score -= nonocc;
            }
            scores[doc_id] += (float) score;
        }
    }

    psql_bm25s_apply_weight_mask_inplace(scores, weight_mask, index->num_docs);

    if (psql_bm25s_method_requires_nonoccurrence(index->params.method))
    {
        double nonoccurrence_sum = 0.0;

        for (i = 0; i < filtered_len; i++)
        {
            uint32_t token_id = filtered_ids[i];
            uint32_t df = index->doc_frequencies[token_id];
            double idf;
            double nonocc;

            if (df == 0)
            {
                continue;
            }

            idf = psql_bm25s_score_idf(
                index->params.idf_method,
                (double) df,
                (double) index->num_docs
            );
            nonocc = idf * psql_bm25s_score_tfc(
                index->params.method,
                0.0,
                0.0,
                avg_doc_len,
                index->params.k1,
                index->params.b,
                index->params.delta
            );
            nonoccurrence_sum += nonocc;
        }

        psql_bm25s_add_constant_inplace(
            scores,
            (float) nonoccurrence_sum,
            index->num_docs
        );
    }

    free(filtered_ids);
    *scores_out = scores;
    return PSQL_BM25S_OK;
}

static bool
psql_bm25s_heap_item_is_worse(
    const psql_bm25s_heap_item *lhs,
    const psql_bm25s_heap_item *rhs
)
{
    if (lhs->score < rhs->score)
    {
        return true;
    }
    if (lhs->score > rhs->score)
    {
        return false;
    }
    return lhs->doc_id > rhs->doc_id;
}

static bool
psql_bm25s_heap_item_is_better(
    const psql_bm25s_heap_item *lhs,
    const psql_bm25s_heap_item *rhs
)
{
    if (lhs->score > rhs->score)
    {
        return true;
    }
    if (lhs->score < rhs->score)
    {
        return false;
    }
    return lhs->doc_id < rhs->doc_id;
}

static void
psql_bm25s_heap_sift_up(
    psql_bm25s_heap_item *heap,
    size_t idx
)
{
    while (idx > 0)
    {
        size_t parent = (idx - 1) / 2;

        if (!psql_bm25s_heap_item_is_worse(&heap[idx], &heap[parent]))
        {
            break;
        }

        {
            psql_bm25s_heap_item tmp = heap[parent];
            heap[parent] = heap[idx];
            heap[idx] = tmp;
        }
        idx = parent;
    }
}

static void
psql_bm25s_heap_sift_down(
    psql_bm25s_heap_item *heap,
    size_t len,
    size_t idx
)
{
    while (true)
    {
        size_t left = idx * 2 + 1;
        size_t right = left + 1;
        size_t smallest = idx;

        if (left < len &&
            psql_bm25s_heap_item_is_worse(&heap[left], &heap[smallest]))
        {
            smallest = left;
        }
        if (right < len &&
            psql_bm25s_heap_item_is_worse(&heap[right], &heap[smallest]))
        {
            smallest = right;
        }
        if (smallest == idx)
        {
            break;
        }

        {
            psql_bm25s_heap_item tmp = heap[idx];
            heap[idx] = heap[smallest];
            heap[smallest] = tmp;
        }
        idx = smallest;
    }
}

static int
psql_bm25s_cmp_topk_desc(const void *lhs, const void *rhs)
{
    const psql_bm25s_heap_item *a = lhs;
    const psql_bm25s_heap_item *b = rhs;

    if (a->score < b->score)
    {
        return 1;
    }
    if (a->score > b->score)
    {
        return -1;
    }
    if (a->doc_id < b->doc_id)
    {
        return -1;
    }
    if (a->doc_id > b->doc_id)
    {
        return 1;
    }
    return 0;
}

static bool
psql_bm25s_use_buffered_topk(
    size_t universe,
    size_t k,
    bool sorted
)
{
    static const size_t buffered_topk_max_k = 4096;

    return sorted && k > 0 && k < universe && k <= buffered_topk_max_k;
}

static psql_bm25s_status
psql_bm25s_topk_result_from_items(
    const psql_bm25s_heap_item *items,
    size_t len,
    psql_bm25s_topk_result *result_out
)
{
    size_t bytes;
    size_t i;

    if (result_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    memset(result_out, 0, sizeof(*result_out));
    if (len == 0)
    {
        return PSQL_BM25S_OK;
    }

    if (!psql_bm25s_checked_mul_size(
            len,
            sizeof(*result_out->doc_ids),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    result_out->doc_ids = malloc(bytes);
    if (result_out->doc_ids == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (!psql_bm25s_checked_mul_size(
            len,
            sizeof(*result_out->scores),
            &bytes))
    {
        free(result_out->doc_ids);
        result_out->doc_ids = NULL;
        return PSQL_BM25S_ERR_RANGE;
    }
    result_out->scores = malloc(bytes);
    if (result_out->scores == NULL)
    {
        free(result_out->doc_ids);
        result_out->doc_ids = NULL;
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < len; i++)
    {
        result_out->doc_ids[i] = items[i].doc_id;
        result_out->scores[i] = items[i].score;
    }
    result_out->len = len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_topk_buffered(
    const float *scores,
    size_t num_scores,
    size_t k,
    psql_bm25s_topk_result *result_out
)
{
    psql_bm25s_heap_item *items = NULL;
    size_t buffer_cap = 0;
    size_t buffer_len = 0;
    size_t bytes;
    size_t i;

    if (!psql_bm25s_checked_mul_size(k, 2, &buffer_cap))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (buffer_cap > num_scores)
    {
        buffer_cap = num_scores;
    }
    if (!psql_bm25s_checked_mul_size(
            buffer_cap,
            sizeof(*items),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    items = malloc(bytes);
    if (items == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < k; i++)
    {
        items[buffer_len].score = scores[i];
        items[buffer_len].doc_id = (uint32_t) i;
        buffer_len++;
    }
    qsort(items, buffer_len, sizeof(*items), psql_bm25s_cmp_topk_desc);

    for (i = k; i < num_scores; i++)
    {
        psql_bm25s_heap_item item;

        item.score = scores[i];
        item.doc_id = (uint32_t) i;
        if (!psql_bm25s_heap_item_is_better(&item, &items[k - 1]))
        {
            continue;
        }

        items[buffer_len] = item;
        buffer_len++;
        if (buffer_len == buffer_cap)
        {
            qsort(
                items,
                buffer_len,
                sizeof(*items),
                psql_bm25s_cmp_topk_desc
            );
            buffer_len = k;
        }
    }

    if (buffer_len > k)
    {
        qsort(items, buffer_len, sizeof(*items), psql_bm25s_cmp_topk_desc);
        buffer_len = k;
    }

    {
        psql_bm25s_status status = psql_bm25s_topk_result_from_items(
            items,
            buffer_len,
            result_out
        );

        free(items);
        return status;
    }
}

static psql_bm25s_status
psql_bm25s_topk_subset_buffered(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    size_t k,
    bool positive_only,
    psql_bm25s_topk_result *result_out
)
{
    psql_bm25s_heap_item *items = NULL;
    size_t heap_cap;
    size_t buffer_cap = 0;
    size_t buffer_len = 0;
    size_t bytes;
    size_t i;

    heap_cap = k < num_candidate_doc_ids ? k : num_candidate_doc_ids;
    if (heap_cap == 0)
    {
        memset(result_out, 0, sizeof(*result_out));
        return PSQL_BM25S_OK;
    }

    if (!psql_bm25s_checked_mul_size(heap_cap, 2, &buffer_cap))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (buffer_cap > num_candidate_doc_ids)
    {
        buffer_cap = num_candidate_doc_ids;
    }
    if (!psql_bm25s_checked_mul_size(
            buffer_cap,
            sizeof(*items),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    items = malloc(bytes);
    if (items == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_candidate_doc_ids; i++)
    {
        psql_bm25s_heap_item item;

        item.doc_id = candidate_doc_ids[i];
        item.score = scores[item.doc_id];
        if (positive_only && item.score <= 0.0f)
        {
            continue;
        }

        items[buffer_len] = item;
        buffer_len++;
        if (buffer_len == heap_cap)
        {
            break;
        }
    }

    if (buffer_len == 0)
    {
        free(items);
        memset(result_out, 0, sizeof(*result_out));
        return PSQL_BM25S_OK;
    }

    qsort(items, buffer_len, sizeof(*items), psql_bm25s_cmp_topk_desc);

    for (i++; i < num_candidate_doc_ids; i++)
    {
        psql_bm25s_heap_item item;

        item.doc_id = candidate_doc_ids[i];
        item.score = scores[item.doc_id];
        if (positive_only && item.score <= 0.0f)
        {
            continue;
        }
        if (!psql_bm25s_heap_item_is_better(
                &item,
                &items[heap_cap - 1]))
        {
            continue;
        }

        items[buffer_len] = item;
        buffer_len++;
        if (buffer_len == buffer_cap)
        {
            qsort(
                items,
                buffer_len,
                sizeof(*items),
                psql_bm25s_cmp_topk_desc
            );
            buffer_len = heap_cap;
        }
    }

    if (buffer_len > heap_cap)
    {
        qsort(items, buffer_len, sizeof(*items), psql_bm25s_cmp_topk_desc);
        buffer_len = heap_cap;
    }

    {
        psql_bm25s_status status = psql_bm25s_topk_result_from_items(
            items,
            buffer_len,
            result_out
        );

        free(items);
        return status;
    }
}

static psql_bm25s_status
psql_bm25s_rank_subset_sorted_all(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    bool positive_only,
    psql_bm25s_topk_result *result_out
)
{
    psql_bm25s_heap_item *items = NULL;
    size_t bytes;
    size_t len = 0;
    size_t i;

    if (!psql_bm25s_checked_mul_size(
            num_candidate_doc_ids,
            sizeof(*items),
            &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    items = malloc(bytes);
    if (items == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_candidate_doc_ids; i++)
    {
        psql_bm25s_heap_item item;

        item.doc_id = candidate_doc_ids[i];
        item.score = scores[item.doc_id];
        if (positive_only && item.score <= 0.0f)
        {
            continue;
        }
        items[len++] = item;
    }

    if (len > 1)
    {
        qsort(items, len, sizeof(*items), psql_bm25s_cmp_topk_desc);
    }

    {
        psql_bm25s_status status = psql_bm25s_topk_result_from_items(
            items,
            len,
            result_out
        );

        free(items);
        return status;
    }
}

psql_bm25s_status
psql_bm25s_topk(
    const float *scores,
    size_t num_scores,
    size_t k,
    bool sorted,
    psql_bm25s_topk_result *result_out
)
{
    psql_bm25s_heap_item *heap = NULL;
    size_t heap_len = 0;
    size_t i;
    size_t bytes;

    if (scores == NULL || result_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    memset(result_out, 0, sizeof(*result_out));

    if (k > num_scores)
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (k == 0)
    {
        return PSQL_BM25S_OK;
    }
    if (psql_bm25s_use_buffered_topk(num_scores, k, sorted))
    {
        return psql_bm25s_topk_buffered(
            scores,
            num_scores,
            k,
            result_out
        );
    }

    if (!psql_bm25s_checked_mul_size(k, sizeof(*heap), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    heap = malloc(bytes);
    if (heap == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_scores; i++)
    {
        psql_bm25s_heap_item item;

        item.score = scores[i];
        item.doc_id = (uint32_t) i;
        if (heap_len < k)
        {
            heap[heap_len] = item;
            psql_bm25s_heap_sift_up(heap, heap_len);
            heap_len++;
        }
        else if (psql_bm25s_heap_item_is_better(&item, &heap[0]))
        {
            heap[0] = item;
            psql_bm25s_heap_sift_down(heap, heap_len, 0);
        }
    }

    if (sorted)
    {
        qsort(heap, heap_len, sizeof(*heap), psql_bm25s_cmp_topk_desc);
    }

    if (!psql_bm25s_checked_mul_size(k, sizeof(*result_out->doc_ids), &bytes))
    {
        free(heap);
        return PSQL_BM25S_ERR_RANGE;
    }
    result_out->doc_ids = malloc(bytes);
    if (result_out->doc_ids == NULL)
    {
        free(heap);
        return PSQL_BM25S_ERR_NOMEM;
    }

    if (!psql_bm25s_checked_mul_size(k, sizeof(*result_out->scores), &bytes))
    {
        free(heap);
        free(result_out->doc_ids);
        result_out->doc_ids = NULL;
        return PSQL_BM25S_ERR_RANGE;
    }
    result_out->scores = malloc(bytes);
    if (result_out->scores == NULL)
    {
        free(heap);
        free(result_out->doc_ids);
        result_out->doc_ids = NULL;
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < heap_len; i++)
    {
        result_out->doc_ids[i] = heap[i].doc_id;
        result_out->scores[i] = heap[i].score;
    }
    result_out->len = heap_len;

    free(heap);
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_topk_subset(
    const float *scores,
    const uint32_t *candidate_doc_ids,
    size_t num_candidate_doc_ids,
    size_t k,
    bool sorted,
    bool positive_only,
    psql_bm25s_topk_result *result_out
)
{
    psql_bm25s_heap_item *heap = NULL;
    size_t heap_len = 0;
    size_t heap_cap;
    size_t i;
    size_t bytes;

    if (scores == NULL || result_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    if (num_candidate_doc_ids > 0 && candidate_doc_ids == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    memset(result_out, 0, sizeof(*result_out));
    if (k == 0 || num_candidate_doc_ids == 0)
    {
        return PSQL_BM25S_OK;
    }
    if (sorted && k == num_candidate_doc_ids)
    {
        return psql_bm25s_rank_subset_sorted_all(
            scores,
            candidate_doc_ids,
            num_candidate_doc_ids,
            positive_only,
            result_out
        );
    }
    if (psql_bm25s_use_buffered_topk(
            num_candidate_doc_ids,
            k < num_candidate_doc_ids ? k : num_candidate_doc_ids,
            sorted))
    {
        return psql_bm25s_topk_subset_buffered(
            scores,
            candidate_doc_ids,
            num_candidate_doc_ids,
            k,
            positive_only,
            result_out
        );
    }

    heap_cap = k < num_candidate_doc_ids ? k : num_candidate_doc_ids;
    if (!psql_bm25s_checked_mul_size(heap_cap, sizeof(*heap), &bytes))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    heap = malloc(bytes);
    if (heap == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < num_candidate_doc_ids; i++)
    {
        psql_bm25s_heap_item item;

        item.doc_id = candidate_doc_ids[i];
        item.score = scores[item.doc_id];
        if (positive_only && item.score <= 0.0f)
        {
            continue;
        }

        if (heap_len < heap_cap)
        {
            heap[heap_len] = item;
            psql_bm25s_heap_sift_up(heap, heap_len);
            heap_len++;
        }
        else if (psql_bm25s_heap_item_is_better(&item, &heap[0]))
        {
            heap[0] = item;
            psql_bm25s_heap_sift_down(heap, heap_len, 0);
        }
    }

    if (sorted)
    {
        qsort(heap, heap_len, sizeof(*heap), psql_bm25s_cmp_topk_desc);
    }

    if (!psql_bm25s_checked_mul_size(heap_len, sizeof(*result_out->doc_ids), &bytes))
    {
        free(heap);
        return PSQL_BM25S_ERR_RANGE;
    }
    if (heap_len > 0)
    {
        result_out->doc_ids = malloc(bytes);
        if (result_out->doc_ids == NULL)
        {
            free(heap);
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    if (!psql_bm25s_checked_mul_size(heap_len, sizeof(*result_out->scores), &bytes))
    {
        free(heap);
        free(result_out->doc_ids);
        result_out->doc_ids = NULL;
        return PSQL_BM25S_ERR_RANGE;
    }
    if (heap_len > 0)
    {
        result_out->scores = malloc(bytes);
        if (result_out->scores == NULL)
        {
            free(heap);
            free(result_out->doc_ids);
            result_out->doc_ids = NULL;
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    for (i = 0; i < heap_len; i++)
    {
        result_out->doc_ids[i] = heap[i].doc_id;
        result_out->scores[i] = heap[i].score;
    }
    result_out->len = heap_len;

    free(heap);
    return PSQL_BM25S_OK;
}
