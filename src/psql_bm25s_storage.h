#ifndef PSQL_BM25S_STORAGE_H
#define PSQL_BM25S_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "psql_bm25s_core.h"

#define PSQL_BM25S_HEADER_SIZE 48U

typedef psql_bm25s_status (*psql_bm25s_stream_write_cb)(
    void *ctx,
    const uint8_t *bytes,
    size_t len
);

psql_bm25s_status psql_bm25s_serialized_index_size(
    const psql_bm25s_index *index,
    size_t *len_out
);

psql_bm25s_status psql_bm25s_serialize_index(
    const psql_bm25s_index *index,
    uint8_t **bytes_out,
    size_t *len_out
);

psql_bm25s_status psql_bm25s_serialize_index_stream(
    const psql_bm25s_index *index,
    psql_bm25s_stream_write_cb write_cb,
    void *write_ctx,
    size_t *len_out
);

psql_bm25s_status psql_bm25s_deserialize_index(
    const uint8_t *bytes,
    size_t len,
    psql_bm25s_index *index_out
);

psql_bm25s_status psql_bm25s_peek_serialized_meta(
    const uint8_t *bytes,
    size_t len,
    uint32_t *num_docs_out,
    uint32_t *vocab_size_out,
    uint64_t *data_len_out
);

#endif
