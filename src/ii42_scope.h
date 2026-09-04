#ifndef II42_SCOPE_H
#define II42_SCOPE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ii42_core.h"

#define II42_SCOPE_HEADER_SIZE 80U
#define II42_SCOPE_COLUMN_ENTRY_SIZE 40U
#define II42_SCOPE_VALUE_ENTRY_SIZE 32U
#define II42_SCOPE_GRAM_BLOCK_VALUES 256U
#define II42_SCOPE_GRAM_FILTER_BYTES 1024U
#define II42_SCOPE_CURRENT_VERSION 6U

typedef enum ii42_scope_value_kind
{
    II42_SCOPE_VALUE_SCALAR = 1,
    II42_SCOPE_VALUE_ARRAY_ELEMENT = 2
} ii42_scope_value_kind;

typedef struct ii42_scope_column
{
    uint16_t index_attribute;
    int16_t heap_attribute;
    uint32_t type_oid;
    uint32_t element_type_oid;
    uint32_t collation_oid;
    int32_t type_modifier;
    const uint8_t *name;
    uint32_t name_size;
    uint32_t first_value;
    uint32_t value_count;
} ii42_scope_column;

typedef struct ii42_scope_value
{
    uint32_t column_index;
    uint8_t kind;
    const uint8_t *value;
    uint32_t value_size;
    const uint8_t *gram_value;
    uint32_t gram_value_size;
    const uint32_t *documents;
    uint32_t document_count;
} ii42_scope_value;

typedef struct ii42_scope_header
{
    uint64_t source_authority_checksum;
    uint64_t object_checksum;
    uint64_t total_size;
    uint64_t dictionary_offset;
    uint64_t dictionary_size;
    uint64_t postings_offset;
    uint64_t postings_size;
    uint64_t gram_filter_offset;
    uint64_t gram_filter_size;
    uint32_t document_count;
    uint32_t column_count;
    uint32_t value_count;
    uint16_t version;
    uint16_t gram_block_values;
    uint16_t gram_filter_bytes;
} ii42_scope_header;

typedef struct ii42_scope_column_entry
{
    uint16_t index_attribute;
    int16_t heap_attribute;
    uint32_t type_oid;
    uint32_t element_type_oid;
    uint32_t collation_oid;
    int32_t type_modifier;
    uint32_t name_offset;
    uint32_t name_size;
    uint32_t first_value;
    uint32_t value_count;
} ii42_scope_column_entry;

typedef struct ii42_scope_value_entry
{
    uint32_t column_index;
    uint8_t kind;
    uint32_t value_offset;
    uint32_t value_size;
    uint64_t postings_offset;
    uint32_t document_count;
    uint32_t gram_mask;
} ii42_scope_value_entry;

typedef struct ii42_scope_writer
{
    uint8_t *bytes;
    size_t total_size;
    size_t column_bytes;
    size_t dictionary_offset;
    size_t dictionary_size;
    size_t dictionary_cursor;
    size_t gram_filter_offset;
    size_t postings_offset;
    size_t postings_size;
    size_t postings_cursor;
    uint32_t document_count;
    uint32_t column_count;
    uint32_t value_count;
    uint32_t next_value;
    uint32_t current_document_count;
    uint32_t previous_document;
    bool value_open;
} ii42_scope_writer;

void ii42_scope_writer_init(ii42_scope_writer *writer);

ii42_status ii42_scope_writer_begin(
    ii42_scope_writer *writer,
    uint64_t source_authority_checksum,
    uint32_t document_count,
    const ii42_scope_column *columns,
    uint32_t column_count,
    uint32_t value_count,
    size_t value_dictionary_size,
    uint64_t posting_count,
    size_t maximum_size
);

ii42_status ii42_scope_writer_start_value(
    ii42_scope_writer *writer,
    const ii42_scope_value *value
);

ii42_status ii42_scope_writer_append_document(
    ii42_scope_writer *writer,
    uint32_t document
);

ii42_status ii42_scope_writer_finish(
    ii42_scope_writer *writer,
    uint8_t **bytes_out,
    size_t *size_out
);

void ii42_scope_writer_free(ii42_scope_writer *writer);

ii42_status ii42_scope_serialize(
    uint64_t source_authority_checksum,
    uint32_t document_count,
    const ii42_scope_column *columns,
    uint32_t column_count,
    const ii42_scope_value *values,
    uint32_t value_count,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_scope_header_deserialize(
    const uint8_t *bytes,
    size_t size,
    size_t object_size,
    uint64_t expected_source_authority_checksum,
    ii42_scope_header *header_out
);

ii42_status ii42_scope_column_entry_deserialize(
    const uint8_t *bytes,
    size_t size,
    const ii42_scope_header *header,
    uint32_t column_index,
    ii42_scope_column_entry *entry_out
);

ii42_status ii42_scope_value_entry_deserialize(
    const uint8_t *bytes,
    size_t size,
    const ii42_scope_header *header,
    uint32_t value_index,
    ii42_scope_value_entry *entry_out
);

/* Decode one lazily fetched value entry at its global table position. */
ii42_status ii42_scope_value_entry_deserialize_one(
    const uint8_t *bytes,
    size_t size,
    const ii42_scope_header *header,
    uint32_t value_index,
    ii42_scope_value_entry *entry_out
);

ii42_status ii42_scope_validate(
    const uint8_t *bytes,
    size_t size,
    uint64_t expected_source_authority_checksum
);

bool ii42_scope_header_is_current(const ii42_scope_header *header);

uint32_t ii42_scope_ascii_gram_bit(
    uint8_t first,
    uint8_t second,
    uint8_t third
);

bool ii42_scope_ascii_ilike_contains_pattern(
    const uint8_t *pattern,
    size_t pattern_size,
    const uint8_t **literal_out,
    size_t *literal_size_out
);

bool ii42_scope_ascii_case_insensitive_contains(
    const uint8_t *value,
    size_t value_size,
    const uint8_t *literal,
    size_t literal_size,
    bool *matches_out
);

#endif
