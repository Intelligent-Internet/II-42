#ifndef II42_STORAGE_H
#define II42_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "ii42_core.h"

#define II42_HEADER_SIZE 48U
#define II42_STORAGE_CURRENT_VERSION 2U

typedef ii42_status (*ii42_stream_write_cb)(
    void *ctx,
    const uint8_t *bytes,
    size_t len
);

ii42_status ii42_serialized_index_size(
    const ii42_index *index,
    size_t *len_out
);

ii42_status ii42_serialize_index(
    const ii42_index *index,
    uint8_t **bytes_out,
    size_t *len_out
);

ii42_status ii42_serialize_index_stream(
    const ii42_index *index,
    ii42_stream_write_cb write_cb,
    void *write_ctx,
    size_t *len_out
);

ii42_status ii42_deserialize_index(
    const uint8_t *bytes,
    size_t len,
    ii42_index *index_out
);

ii42_status ii42_peek_serialized_meta(
    const uint8_t *bytes,
    size_t len,
    uint32_t *num_docs_out,
    uint32_t *vocab_size_out,
    uint64_t *data_len_out
);

#endif
