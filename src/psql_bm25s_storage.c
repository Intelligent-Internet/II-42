#include "psql_bm25s_storage.h"

#include <stdlib.h>
#include <string.h>

#define PSQL_BM25S_MAGIC 0x424D3235U
#define PSQL_BM25S_STORAGE_VERSION 2U
#define PSQL_BM25S_FLAG_HAS_NONOCCURRENCE 0x0001U
#define PSQL_BM25S_FLAG_HAS_VOCAB 0x0002U
#define PSQL_BM25S_FLAG_HAS_EMPTY_TOKEN 0x0004U
#define PSQL_BM25S_FLAG_HAS_EXACT_STATS 0x0008U
#define PSQL_BM25S_FLAG_KNOWN_MASK                                           \
    (PSQL_BM25S_FLAG_HAS_NONOCCURRENCE | PSQL_BM25S_FLAG_HAS_VOCAB |         \
     PSQL_BM25S_FLAG_HAS_EMPTY_TOKEN | PSQL_BM25S_FLAG_HAS_EXACT_STATS)
#define PSQL_BM25S_STREAM_SCRATCH_BYTES 65536U

static void
psql_bm25s_write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t) (value & 0xFFU);
    dst[1] = (uint8_t) ((value >> 8) & 0xFFU);
}

static void
psql_bm25s_write_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t) (value & 0xFFU);
    dst[1] = (uint8_t) ((value >> 8) & 0xFFU);
    dst[2] = (uint8_t) ((value >> 16) & 0xFFU);
    dst[3] = (uint8_t) ((value >> 24) & 0xFFU);
}

static void
psql_bm25s_write_u64_le(uint8_t *dst, uint64_t value)
{
    size_t i;

    for (i = 0; i < 8; i++)
    {
        dst[i] = (uint8_t) ((value >> (i * 8)) & 0xFFU);
    }
}

static void
psql_bm25s_write_f32_le(uint8_t *dst, float value)
{
    union
    {
        float f;
        uint32_t u;
    } conv;

    conv.f = value;
    psql_bm25s_write_u32_le(dst, conv.u);
}

static uint16_t
psql_bm25s_read_u16_le(const uint8_t *src)
{
    return (uint16_t) src[0] |
           (uint16_t) ((uint16_t) src[1] << 8);
}

static uint32_t
psql_bm25s_read_u32_le(const uint8_t *src)
{
    return (uint32_t) src[0] |
           ((uint32_t) src[1] << 8) |
           ((uint32_t) src[2] << 16) |
           ((uint32_t) src[3] << 24);
}

static uint64_t
psql_bm25s_read_u64_le(const uint8_t *src)
{
    uint64_t value = 0;
    size_t i;

    for (i = 0; i < 8; i++)
    {
        value |= ((uint64_t) src[i]) << (i * 8);
    }

    return value;
}

static float
psql_bm25s_read_f32_le(const uint8_t *src)
{
    union
    {
        float f;
        uint32_t u;
    } conv;

    conv.u = psql_bm25s_read_u32_le(src);
    return conv.f;
}

static bool
psql_bm25s_checked_add_size(size_t a, size_t b, size_t *out)
{
    if (a > ((size_t) -1) - b)
    {
        return false;
    }

    *out = a + b;
    return true;
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

static bool
psql_bm25s_is_valid_method(psql_bm25s_method method)
{
    switch (method)
    {
        case PSQL_BM25S_METHOD_ROBERTSON:
        case PSQL_BM25S_METHOD_LUCENE:
        case PSQL_BM25S_METHOD_ATIRE:
        case PSQL_BM25S_METHOD_BM25L:
        case PSQL_BM25S_METHOD_BM25PLUS:
            return true;
    }

    return false;
}

static psql_bm25s_status
psql_bm25s_validate_header_values(
    const psql_bm25s_index *index,
    uint16_t flags
)
{
    if ((flags & ~PSQL_BM25S_FLAG_KNOWN_MASK) != 0)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (!psql_bm25s_is_valid_method(index->params.method) ||
        !psql_bm25s_is_valid_method(index->params.idf_method))
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (index->data_len > (uint64_t) UINT32_MAX)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (index->vocab_size == UINT32_MAX)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (index->has_empty_token && index->empty_token_id >= index->vocab_size)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_validate_postings(const psql_bm25s_index *index)
{
    size_t i;
    size_t indptr_len = (size_t) index->vocab_size + 1;
    uint64_t last = 0;

    if (index->indptr[0] != 0)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    for (i = 0; i < indptr_len; i++)
    {
        uint64_t current = index->indptr[i];

        if (current < last || current > index->data_len)
        {
            return PSQL_BM25S_ERR_FORMAT;
        }
        last = current;
    }
    if (last != index->data_len)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    for (i = 0; i < index->data_len; i++)
    {
        if (index->indices[i] >= index->num_docs)
        {
            return PSQL_BM25S_ERR_FORMAT;
        }
    }

    return PSQL_BM25S_OK;
}

static uint16_t
psql_bm25s_serialized_flags(const psql_bm25s_index *index)
{
    uint16_t flags = 0;

    if (index->nonoccurrence != NULL)
    {
        flags |= PSQL_BM25S_FLAG_HAS_NONOCCURRENCE;
    }
    if (index->vocab != NULL)
    {
        flags |= PSQL_BM25S_FLAG_HAS_VOCAB;
    }
    if (index->has_empty_token)
    {
        flags |= PSQL_BM25S_FLAG_HAS_EMPTY_TOKEN;
    }
    if (index->term_frequencies != NULL &&
        index->doc_lengths != NULL &&
        index->doc_frequencies != NULL)
    {
        flags |= PSQL_BM25S_FLAG_HAS_EXACT_STATS;
    }
    return flags;
}

static psql_bm25s_status
psql_bm25s_stream_f32_array(
    psql_bm25s_stream_write_cb write_cb,
    void *write_ctx,
    const float *values,
    uint64_t count
)
{
    uint8_t scratch[PSQL_BM25S_STREAM_SCRATCH_BYTES];
    uint64_t pos = 0;

    while (pos < count)
    {
        size_t i;
        size_t chunk = (size_t) (count - pos);
        psql_bm25s_status status;

        if (chunk > sizeof(scratch) / sizeof(float))
        {
            chunk = sizeof(scratch) / sizeof(float);
        }
        for (i = 0; i < chunk; i++)
        {
            psql_bm25s_write_f32_le(
                scratch + i * sizeof(float),
                values[pos + i]
            );
        }
        status = write_cb(write_ctx, scratch, chunk * sizeof(float));
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        pos += chunk;
    }
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_stream_u32_array(
    psql_bm25s_stream_write_cb write_cb,
    void *write_ctx,
    const uint32_t *values,
    uint64_t count
)
{
    uint8_t scratch[PSQL_BM25S_STREAM_SCRATCH_BYTES];
    uint64_t pos = 0;

    while (pos < count)
    {
        size_t i;
        size_t chunk = (size_t) (count - pos);
        psql_bm25s_status status;

        if (chunk > sizeof(scratch) / sizeof(uint32_t))
        {
            chunk = sizeof(scratch) / sizeof(uint32_t);
        }
        for (i = 0; i < chunk; i++)
        {
            psql_bm25s_write_u32_le(
                scratch + i * sizeof(uint32_t),
                values[pos + i]
            );
        }
        status = write_cb(write_ctx, scratch, chunk * sizeof(uint32_t));
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        pos += chunk;
    }
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_stream_u64_array(
    psql_bm25s_stream_write_cb write_cb,
    void *write_ctx,
    const uint64_t *values,
    uint64_t count
)
{
    uint8_t scratch[PSQL_BM25S_STREAM_SCRATCH_BYTES];
    uint64_t pos = 0;

    while (pos < count)
    {
        size_t i;
        size_t chunk = (size_t) (count - pos);
        psql_bm25s_status status;

        if (chunk > sizeof(scratch) / sizeof(uint64_t))
        {
            chunk = sizeof(scratch) / sizeof(uint64_t);
        }
        for (i = 0; i < chunk; i++)
        {
            psql_bm25s_write_u64_le(
                scratch + i * sizeof(uint64_t),
                values[pos + i]
            );
        }
        status = write_cb(write_ctx, scratch, chunk * sizeof(uint64_t));
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        pos += chunk;
    }
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_serialized_index_size(
    const psql_bm25s_index *index,
    size_t *len_out
)
{
    uint16_t flags;
    size_t len = PSQL_BM25S_HEADER_SIZE;
    size_t chunk = 0;
    uint32_t i;

    if (index == NULL || len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    *len_out = 0;

    flags = psql_bm25s_serialized_flags(index);
    if (psql_bm25s_validate_header_values(index, flags) != PSQL_BM25S_OK)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    if (!psql_bm25s_checked_mul_size(
            (size_t) index->data_len,
            sizeof(*index->data),
            &chunk) ||
        !psql_bm25s_checked_add_size(len, chunk, &len))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (!psql_bm25s_checked_mul_size(
            (size_t) index->data_len,
            sizeof(*index->indices),
            &chunk) ||
        !psql_bm25s_checked_add_size(len, chunk, &len))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (!psql_bm25s_checked_mul_size(
            (size_t) index->vocab_size + 1,
            sizeof(*index->indptr),
            &chunk) ||
        !psql_bm25s_checked_add_size(len, chunk, &len))
    {
        return PSQL_BM25S_ERR_RANGE;
    }
    if (index->nonoccurrence != NULL)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) index->vocab_size,
                sizeof(*index->nonoccurrence),
                &chunk) ||
            !psql_bm25s_checked_add_size(len, chunk, &len))
        {
            return PSQL_BM25S_ERR_RANGE;
        }
    }
    if ((flags & PSQL_BM25S_FLAG_HAS_EXACT_STATS) != 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) index->data_len,
                sizeof(*index->term_frequencies),
                &chunk) ||
            !psql_bm25s_checked_add_size(len, chunk, &len))
        {
            return PSQL_BM25S_ERR_RANGE;
        }
        if (!psql_bm25s_checked_mul_size(
                (size_t) index->num_docs,
                sizeof(*index->doc_lengths),
                &chunk) ||
            !psql_bm25s_checked_add_size(len, chunk, &len))
        {
            return PSQL_BM25S_ERR_RANGE;
        }
        if (!psql_bm25s_checked_mul_size(
                (size_t) index->vocab_size,
                sizeof(*index->doc_frequencies),
                &chunk) ||
            !psql_bm25s_checked_add_size(len, chunk, &len))
        {
            return PSQL_BM25S_ERR_RANGE;
        }
    }
    if (index->vocab != NULL)
    {
        for (i = 0; i < index->vocab_size; i++)
        {
            size_t token_len = strlen(index->vocab[i]);
            if (token_len > UINT32_MAX ||
                !psql_bm25s_checked_add_size(len, sizeof(uint32_t), &len) ||
                !psql_bm25s_checked_add_size(len, token_len, &len))
            {
                return PSQL_BM25S_ERR_RANGE;
            }
        }
    }

    *len_out = len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_parse_header(
    const uint8_t *bytes,
    size_t len,
    uint16_t *flags_out,
    psql_bm25s_index *index_out
)
{
    uint32_t magic;
    uint16_t version;

    if (len < PSQL_BM25S_HEADER_SIZE)
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    magic = psql_bm25s_read_u32_le(bytes);
    version = psql_bm25s_read_u16_le(bytes + 4);
    if (magic != PSQL_BM25S_MAGIC ||
        (version != 1U && version != PSQL_BM25S_STORAGE_VERSION))
    {
        return PSQL_BM25S_ERR_FORMAT;
    }

    *flags_out = psql_bm25s_read_u16_le(bytes + 6);
    index_out->params.method =
        (psql_bm25s_method) psql_bm25s_read_u32_le(bytes + 8);
    index_out->params.idf_method =
        (psql_bm25s_method) psql_bm25s_read_u32_le(bytes + 12);
    index_out->params.k1 = psql_bm25s_read_f32_le(bytes + 16);
    index_out->params.b = psql_bm25s_read_f32_le(bytes + 20);
    index_out->params.delta = psql_bm25s_read_f32_le(bytes + 24);
    index_out->num_docs = psql_bm25s_read_u32_le(bytes + 28);
    index_out->vocab_size = psql_bm25s_read_u32_le(bytes + 32);
    index_out->data_len = psql_bm25s_read_u64_le(bytes + 36);
    index_out->empty_token_id = psql_bm25s_read_u32_le(bytes + 44);
    index_out->has_empty_token =
        ((*flags_out & PSQL_BM25S_FLAG_HAS_EMPTY_TOKEN) != 0);

    return psql_bm25s_validate_header_values(index_out, *flags_out);
}

typedef struct psql_bm25s_buffer_writer
{
    uint8_t *ptr;
} psql_bm25s_buffer_writer;

static psql_bm25s_status
psql_bm25s_write_to_buffer(void *ctx, const uint8_t *bytes, size_t len)
{
    psql_bm25s_buffer_writer *writer = ctx;

    memcpy(writer->ptr, bytes, len);
    writer->ptr += len;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_serialize_index(
    const psql_bm25s_index *index,
    uint8_t **bytes_out,
    size_t *len_out
)
{
    size_t len = 0;
    uint8_t *bytes;
    psql_bm25s_buffer_writer writer;
    psql_bm25s_status status;

    if (index == NULL || bytes_out == NULL || len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    *bytes_out = NULL;
    *len_out = 0;

    status = psql_bm25s_serialized_index_size(index, &len);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }
    bytes = malloc(len);
    if (bytes == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }
    memset(bytes, 0, len);

    writer.ptr = bytes;
    status = psql_bm25s_serialize_index_stream(
        index,
        psql_bm25s_write_to_buffer,
        &writer,
        &len
    );
    if (status != PSQL_BM25S_OK)
    {
        free(bytes);
        return status;
    }

    *bytes_out = bytes;
    *len_out = len;
    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_serialize_index_stream(
    const psql_bm25s_index *index,
    psql_bm25s_stream_write_cb write_cb,
    void *write_ctx,
    size_t *len_out
)
{
    uint16_t flags;
    uint8_t scratch[PSQL_BM25S_HEADER_SIZE];
    uint32_t i;
    psql_bm25s_status status;
    size_t len = 0;

    if (index == NULL || write_cb == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }
    status = psql_bm25s_serialized_index_size(index, &len);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }
    if (len_out != NULL)
    {
        *len_out = len;
    }

    flags = psql_bm25s_serialized_flags(index);
    memset(scratch, 0, sizeof(scratch));
    psql_bm25s_write_u32_le(scratch, PSQL_BM25S_MAGIC);
    psql_bm25s_write_u16_le(scratch + 4, PSQL_BM25S_STORAGE_VERSION);
    psql_bm25s_write_u16_le(scratch + 6, flags);
    psql_bm25s_write_u32_le(scratch + 8, (uint32_t) index->params.method);
    psql_bm25s_write_u32_le(scratch + 12, (uint32_t) index->params.idf_method);
    psql_bm25s_write_f32_le(scratch + 16, index->params.k1);
    psql_bm25s_write_f32_le(scratch + 20, index->params.b);
    psql_bm25s_write_f32_le(scratch + 24, index->params.delta);
    psql_bm25s_write_u32_le(scratch + 28, index->num_docs);
    psql_bm25s_write_u32_le(scratch + 32, index->vocab_size);
    psql_bm25s_write_u64_le(scratch + 36, index->data_len);
    psql_bm25s_write_u32_le(scratch + 44, index->empty_token_id);
    status = write_cb(write_ctx, scratch, PSQL_BM25S_HEADER_SIZE);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    status = psql_bm25s_stream_f32_array(
        write_cb,
        write_ctx,
        index->data,
        index->data_len
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }
    status = psql_bm25s_stream_u32_array(
        write_cb,
        write_ctx,
        index->indices,
        index->data_len
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }
    status = psql_bm25s_stream_u64_array(
        write_cb,
        write_ctx,
        index->indptr,
        (uint64_t) index->vocab_size + 1
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }
    if ((flags & PSQL_BM25S_FLAG_HAS_EXACT_STATS) != 0)
    {
        status = psql_bm25s_stream_u32_array(
            write_cb,
            write_ctx,
            index->term_frequencies,
            index->data_len
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        status = psql_bm25s_stream_u32_array(
            write_cb,
            write_ctx,
            index->doc_lengths,
            index->num_docs
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        status = psql_bm25s_stream_u32_array(
            write_cb,
            write_ctx,
            index->doc_frequencies,
            index->vocab_size
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
    }
    if (index->nonoccurrence != NULL)
    {
        status = psql_bm25s_stream_f32_array(
            write_cb,
            write_ctx,
            index->nonoccurrence,
            index->vocab_size
        );
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
    }
    if (index->vocab != NULL)
    {
        for (i = 0; i < index->vocab_size; i++)
        {
            size_t token_len = strlen(index->vocab[i]);
            psql_bm25s_write_u32_le(scratch, (uint32_t) token_len);
            status = write_cb(write_ctx, scratch, sizeof(uint32_t));
            if (status != PSQL_BM25S_OK)
            {
                return status;
            }
            status = write_cb(
                write_ctx,
                (const uint8_t *) index->vocab[i],
                token_len
            );
            if (status != PSQL_BM25S_OK)
            {
                return status;
            }
        }
    }

    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_deserialize_index(
    const uint8_t *bytes,
    size_t len,
    psql_bm25s_index *index_out
)
{
    uint16_t flags = 0;
    const uint8_t *ptr;
    size_t remaining;
    size_t chunk;
    uint32_t i;
    psql_bm25s_status status;

    if (bytes == NULL || index_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    psql_bm25s_index_init(index_out);
    status = psql_bm25s_parse_header(bytes, len, &flags, index_out);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    ptr = bytes + PSQL_BM25S_HEADER_SIZE;
    remaining = len - PSQL_BM25S_HEADER_SIZE;

    if (!psql_bm25s_checked_mul_size(
            (size_t) index_out->data_len,
            sizeof(*index_out->data),
            &chunk) ||
        chunk > remaining)
    {
        psql_bm25s_index_free(index_out);
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (chunk > 0)
    {
        index_out->data = malloc(chunk);
        if (index_out->data == NULL)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_NOMEM;
        }
        for (i = 0; i < index_out->data_len; i++)
        {
            index_out->data[i] = psql_bm25s_read_f32_le(ptr);
            ptr += sizeof(float);
        }
        remaining -= chunk;
    }

    if (!psql_bm25s_checked_mul_size(
            (size_t) index_out->data_len,
            sizeof(*index_out->indices),
            &chunk) ||
        chunk > remaining)
    {
        psql_bm25s_index_free(index_out);
        return PSQL_BM25S_ERR_FORMAT;
    }
    if (chunk > 0)
    {
        index_out->indices = malloc(chunk);
        if (index_out->indices == NULL)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_NOMEM;
        }
        for (i = 0; i < index_out->data_len; i++)
        {
            index_out->indices[i] = psql_bm25s_read_u32_le(ptr);
            ptr += sizeof(uint32_t);
        }
        remaining -= chunk;
    }

    if (!psql_bm25s_checked_mul_size(
            (size_t) index_out->vocab_size + 1,
            sizeof(*index_out->indptr),
            &chunk) ||
        chunk > remaining)
    {
        psql_bm25s_index_free(index_out);
        return PSQL_BM25S_ERR_FORMAT;
    }
    index_out->indptr = malloc(chunk);
    if (index_out->indptr == NULL)
    {
        psql_bm25s_index_free(index_out);
        return PSQL_BM25S_ERR_NOMEM;
    }
    for (i = 0; i < (size_t) index_out->vocab_size + 1; i++)
    {
        index_out->indptr[i] = psql_bm25s_read_u64_le(ptr);
        ptr += sizeof(uint64_t);
    }
    remaining -= chunk;

    status = psql_bm25s_validate_postings(index_out);
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_index_free(index_out);
        return status;
    }

    if ((flags & PSQL_BM25S_FLAG_HAS_EXACT_STATS) != 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) index_out->data_len,
                sizeof(*index_out->term_frequencies),
                &chunk) ||
            chunk > remaining)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_FORMAT;
        }
        if (chunk > 0)
        {
            index_out->term_frequencies = malloc(chunk);
            if (index_out->term_frequencies == NULL)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_NOMEM;
            }
            for (i = 0; i < index_out->data_len; i++)
            {
                index_out->term_frequencies[i] = psql_bm25s_read_u32_le(ptr);
                ptr += sizeof(uint32_t);
            }
            remaining -= chunk;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) index_out->num_docs,
                sizeof(*index_out->doc_lengths),
                &chunk) ||
            chunk > remaining)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_FORMAT;
        }
        if (chunk > 0)
        {
            index_out->doc_lengths = malloc(chunk);
            if (index_out->doc_lengths == NULL)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_NOMEM;
            }
            for (i = 0; i < index_out->num_docs; i++)
            {
                index_out->doc_lengths[i] = psql_bm25s_read_u32_le(ptr);
                ptr += sizeof(uint32_t);
            }
            remaining -= chunk;
        }

        if (!psql_bm25s_checked_mul_size(
                (size_t) index_out->vocab_size,
                sizeof(*index_out->doc_frequencies),
                &chunk) ||
            chunk > remaining)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_FORMAT;
        }
        if (chunk > 0)
        {
            index_out->doc_frequencies = malloc(chunk);
            if (index_out->doc_frequencies == NULL)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_NOMEM;
            }
            for (i = 0; i < index_out->vocab_size; i++)
            {
                index_out->doc_frequencies[i] = psql_bm25s_read_u32_le(ptr);
                ptr += sizeof(uint32_t);
            }
            remaining -= chunk;
        }
    }

    if ((flags & PSQL_BM25S_FLAG_HAS_NONOCCURRENCE) != 0)
    {
        if (!psql_bm25s_checked_mul_size(
                (size_t) index_out->vocab_size,
                sizeof(*index_out->nonoccurrence),
                &chunk) ||
            chunk > remaining)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_FORMAT;
        }

        if (chunk > 0)
        {
            index_out->nonoccurrence = malloc(chunk);
            if (index_out->nonoccurrence == NULL)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_NOMEM;
            }
            for (i = 0; i < index_out->vocab_size; i++)
            {
                index_out->nonoccurrence[i] = psql_bm25s_read_f32_le(ptr);
                ptr += sizeof(float);
            }
            remaining -= chunk;
        }
    }

    if ((flags & PSQL_BM25S_FLAG_HAS_VOCAB) != 0)
    {
        index_out->vocab = calloc(index_out->vocab_size, sizeof(*index_out->vocab));
        if (index_out->vocab == NULL)
        {
            psql_bm25s_index_free(index_out);
            return PSQL_BM25S_ERR_NOMEM;
        }

        for (i = 0; i < index_out->vocab_size; i++)
        {
            uint32_t token_len;

            if (remaining < sizeof(uint32_t))
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_FORMAT;
            }
            token_len = psql_bm25s_read_u32_le(ptr);
            ptr += sizeof(uint32_t);
            remaining -= sizeof(uint32_t);

            if (token_len > remaining)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_FORMAT;
            }

            index_out->vocab[i] = malloc((size_t) token_len + 1);
            if (index_out->vocab[i] == NULL)
            {
                psql_bm25s_index_free(index_out);
                return PSQL_BM25S_ERR_NOMEM;
            }
            memcpy(index_out->vocab[i], ptr, token_len);
            index_out->vocab[i][token_len] = '\0';
            ptr += token_len;
            remaining -= token_len;
        }
    }

    if (remaining != 0)
    {
        psql_bm25s_index_free(index_out);
        return PSQL_BM25S_ERR_FORMAT;
    }

    return PSQL_BM25S_OK;
}

psql_bm25s_status
psql_bm25s_peek_serialized_meta(
    const uint8_t *bytes,
    size_t len,
    uint32_t *num_docs_out,
    uint32_t *vocab_size_out,
    uint64_t *data_len_out
)
{
    psql_bm25s_index meta;
    uint16_t flags = 0;
    psql_bm25s_status status;

    if (bytes == NULL || num_docs_out == NULL || vocab_size_out == NULL ||
        data_len_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    psql_bm25s_index_init(&meta);
    status = psql_bm25s_parse_header(bytes, len, &flags, &meta);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    *num_docs_out = meta.num_docs;
    *vocab_size_out = meta.vocab_size;
    *data_len_out = meta.data_len;
    return PSQL_BM25S_OK;
}
