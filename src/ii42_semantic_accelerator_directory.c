#include "ii42_semantic_accelerator_directory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define II42_ACCELERATOR_DIRECTORY_MAGIC UINT32_C(0x44414932)
#define II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION UINT16_C(5)
#define II42_ACCELERATOR_DIRECTORY_RETIREMENT_MAX_VERSION UINT16_C(9)
#define II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION UINT16_C(9)
#define II42_ACCELERATOR_DIRECTORY_VERSION UINT16_C(10)
#define II42_ACCELERATOR_DIRECTORY_SCOPE_HEADER_SIZE 128U
#define II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE 56U
#define II42_ACCELERATOR_RETIREMENT_V5_FORWARD_ENTRY_SIZE 56U
#define II42_ACCELERATOR_FORWARD_ENTRY_SIZE 64U
#define II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE 48U
#define II42_ACCELERATOR_DIRECTORY_CHECKSUM_OFFSET 64U

static void ii42_accelerator_directory_read_ref(
    const uint8_t *bytes,
    ii42_segment_object_ref *ref
);

static bool ii42_accelerator_directory_ref_is_zero(
    const ii42_segment_object_ref *ref
);

static bool ii42_accelerator_directory_refs_overlap(
    const ii42_segment_object_ref *left,
    const ii42_segment_object_ref *right
);

static uint32_t
ii42_accelerator_directory_expected_forward_bound_shards(
    uint32_t vocab_size
)
{
    return vocab_size == 0
        ? 0
        : (vocab_size - 1U) /
            II42_SEMANTIC_FORWARD_BOUND_TERMS_PER_SHARD + 1U;
}

static uint64_t
ii42_accelerator_directory_expected_forward_row_offsets(
    uint32_t document_count,
    uint32_t forward_chunk_count
)
{
    return (uint64_t) document_count + forward_chunk_count;
}

static bool
ii42_accelerator_directory_policy_is_queryable(uint32_t policy)
{
    return policy == II42_SEMANTIC_ACCELERATOR_CURRENT_POLICY;
}

static void
ii42_accelerator_directory_write_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t) (value & UINT16_C(0xFF));
    bytes[1] = (uint8_t) ((value >> 8) & UINT16_C(0xFF));
}

static void
ii42_accelerator_directory_write_u32(uint8_t *bytes, uint32_t value)
{
    size_t index;

    for (index = 0; index < sizeof(value); index++)
    {
        bytes[index] = (uint8_t) ((value >> (index * 8)) & UINT32_C(0xFF));
    }
}

static void
ii42_accelerator_directory_write_u64(uint8_t *bytes, uint64_t value)
{
    size_t index;

    for (index = 0; index < sizeof(value); index++)
    {
        bytes[index] = (uint8_t) ((value >> (index * 8)) & UINT64_C(0xFF));
    }
}

static uint16_t
ii42_accelerator_directory_read_u16(const uint8_t *bytes)
{
    return (uint16_t) bytes[0] |
        (uint16_t) ((uint16_t) bytes[1] << 8);
}

static uint32_t
ii42_accelerator_directory_read_u32(const uint8_t *bytes)
{
    return (uint32_t) bytes[0] |
        ((uint32_t) bytes[1] << 8) |
        ((uint32_t) bytes[2] << 16) |
        ((uint32_t) bytes[3] << 24);
}

static uint64_t
ii42_accelerator_directory_read_u64(const uint8_t *bytes)
{
    uint64_t value = 0;
    size_t index;

    for (index = 0; index < sizeof(value); index++)
    {
        value |= (uint64_t) bytes[index] << (index * 8);
    }
    return value;
}

static uint64_t
ii42_accelerator_directory_checksum(const uint8_t *bytes, size_t size)
{
    uint64_t checksum = UINT64_C(14695981039346656037);
    size_t index;

    for (index = 0; index < size; index++)
    {
        uint8_t value =
            index >= II42_ACCELERATOR_DIRECTORY_CHECKSUM_OFFSET &&
            index < II42_ACCELERATOR_DIRECTORY_CHECKSUM_OFFSET +
                sizeof(uint64_t)
                ? 0
                : bytes[index];

        checksum ^= value;
        checksum *= UINT64_C(1099511628211);
    }
    return checksum;
}

ii42_status
ii42_semantic_accelerator_directory_summary_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory_summary *summary_out
)
{
    ii42_semantic_accelerator_directory_summary summary;
    size_t entry_bytes;
    size_t forward_bytes;
    size_t forward_term_work_bytes;
    size_t forward_chunk_cost_bytes;
    size_t forward_row_offset_bytes;
    size_t forward_term_bytes;
    size_t forward_bound_term_bytes;
    size_t forward_bound_ref_bytes;
    size_t expected_size;
    uint16_t version;
    uint16_t header_size;
    uint16_t term_entry_size;
    uint16_t forward_entry_size;

    if (bytes == NULL || summary_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (size < II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE)
    {
        return II42_ERR_FORMAT;
    }
    memset(&summary, 0, sizeof(summary));
    version = ii42_accelerator_directory_read_u16(bytes + 4);
    header_size = ii42_accelerator_directory_read_u16(bytes + 6);
    term_entry_size = ii42_accelerator_directory_read_u16(bytes + 8);
    forward_entry_size = ii42_accelerator_directory_read_u16(bytes + 10);
    if (ii42_accelerator_directory_read_u32(bytes + 0) !=
            II42_ACCELERATOR_DIRECTORY_MAGIC ||
        (version < II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION ||
         version > II42_ACCELERATOR_DIRECTORY_VERSION) ||
        header_size != II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE ||
        size < header_size ||
        term_entry_size != II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE ||
        forward_entry_size !=
            (version == II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION
                ? II42_ACCELERATOR_RETIREMENT_V5_FORWARD_ENTRY_SIZE
                : II42_ACCELERATOR_FORWARD_ENTRY_SIZE))
    {
        return II42_ERR_FORMAT;
    }
    summary.format_version = version;
    summary.header_size = header_size;
    summary.term_entry_size = term_entry_size;
    summary.forward_entry_size = forward_entry_size;
    summary.document_count = ii42_accelerator_directory_read_u32(bytes + 12);
    summary.vocab_size = ii42_accelerator_directory_read_u32(bytes + 16);
    summary.term_count = ii42_accelerator_directory_read_u32(bytes + 20);
    summary.forward_chunk_count =
        ii42_accelerator_directory_read_u32(bytes + 24);
    summary.forward_document_shift =
        ii42_accelerator_directory_read_u32(bytes + 28);
    summary.source_manifest_id =
        ii42_accelerator_directory_read_u64(bytes + 32);
    summary.source_authority_checksum =
        ii42_accelerator_directory_read_u64(bytes + 40);
    summary.owner_manifest_id =
        ii42_accelerator_directory_read_u64(bytes + 48);
    summary.total_size = ii42_accelerator_directory_read_u64(bytes + 56);
    summary.builder_policy_id =
        ii42_accelerator_directory_read_u32(bytes + 72);
    summary.retained_document_cap =
        ii42_accelerator_directory_read_u32(bytes + 76);
    summary.forward_term_work_count =
        version >= UINT16_C(7)
            ? summary.vocab_size
            : 0;
    summary.forward_chunk_cost_count =
        version >= II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION
            ? summary.forward_chunk_count
            : 0;
    summary.forward_row_offset_count =
        version == II42_ACCELERATOR_DIRECTORY_VERSION
            ? ii42_accelerator_directory_expected_forward_row_offsets(
                  summary.document_count,
                  summary.forward_chunk_count
              )
            : 0;
    summary.forward_term_bytes_count =
        version >= II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION
            ? summary.vocab_size
            : 0;
    summary.forward_bound_term_bytes_count =
        version >= II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION
            ? summary.vocab_size
            : 0;
    summary.forward_bound_shard_count =
        version >= UINT16_C(8)
            ? ii42_accelerator_directory_expected_forward_bound_shards(
                  summary.vocab_size
              )
            : 0;
    ii42_accelerator_directory_read_ref(bytes + 80, &summary.scope_object);
    ii42_accelerator_directory_read_ref(
        bytes + II42_ACCELERATOR_DIRECTORY_SCOPE_HEADER_SIZE,
        &summary.tid_lookup_object
    );

    entry_bytes = (size_t) summary.term_count *
        summary.term_entry_size;
    forward_bytes = (size_t) summary.forward_chunk_count *
        summary.forward_entry_size;
    forward_term_work_bytes =
        (size_t) summary.forward_term_work_count * sizeof(uint64_t);
    forward_chunk_cost_bytes =
        (size_t) summary.forward_chunk_cost_count *
        2U * sizeof(uint64_t);
    if (summary.forward_row_offset_count > SIZE_MAX / sizeof(uint32_t))
    {
        return II42_ERR_RANGE;
    }
    forward_row_offset_bytes =
        (size_t) summary.forward_row_offset_count * sizeof(uint32_t);
    forward_term_bytes =
        (size_t) summary.forward_term_bytes_count * sizeof(uint64_t);
    forward_bound_term_bytes =
        (size_t) summary.forward_bound_term_bytes_count * sizeof(uint64_t);
    forward_bound_ref_bytes =
        (size_t) summary.forward_bound_shard_count *
        II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
    if ((summary.term_count != 0 &&
         entry_bytes / summary.term_entry_size !=
             summary.term_count) ||
        (summary.forward_chunk_count != 0 &&
         forward_bytes / summary.forward_entry_size !=
             summary.forward_chunk_count) ||
        (summary.forward_term_work_count != 0 &&
         forward_term_work_bytes / sizeof(uint64_t) !=
             summary.forward_term_work_count) ||
        (summary.forward_chunk_cost_count != 0 &&
         forward_chunk_cost_bytes / (2U * sizeof(uint64_t)) !=
             summary.forward_chunk_cost_count) ||
        (summary.forward_row_offset_count != 0 &&
         forward_row_offset_bytes / sizeof(uint32_t) !=
             summary.forward_row_offset_count) ||
        (summary.forward_term_bytes_count != 0 &&
         forward_term_bytes / sizeof(uint64_t) !=
             summary.forward_term_bytes_count) ||
        (summary.forward_bound_term_bytes_count != 0 &&
         forward_bound_term_bytes / sizeof(uint64_t) !=
             summary.forward_bound_term_bytes_count) ||
        (summary.forward_bound_shard_count != 0 &&
         forward_bound_ref_bytes /
                II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE !=
             summary.forward_bound_shard_count) ||
        entry_bytes > SIZE_MAX - header_size ||
        forward_bytes >
            SIZE_MAX - header_size - entry_bytes ||
        forward_term_work_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes ||
        forward_chunk_cost_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes ||
        forward_row_offset_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes - forward_chunk_cost_bytes ||
        forward_term_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes ||
        forward_bound_term_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes ||
        forward_bound_ref_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes -
                forward_bound_term_bytes)
    {
        return II42_ERR_RANGE;
    }
    expected_size = header_size +
        entry_bytes + forward_bytes + forward_term_work_bytes +
        forward_chunk_cost_bytes + forward_row_offset_bytes +
        forward_term_bytes +
        forward_bound_term_bytes + forward_bound_ref_bytes;
    if (summary.source_manifest_id == 0 ||
        summary.source_authority_checksum == 0 ||
        summary.owner_manifest_id <= summary.source_manifest_id ||
        summary.document_count == 0 || summary.vocab_size == 0 ||
        summary.term_count == 0 || summary.total_size != expected_size ||
        ((summary.builder_policy_id == 0) !=
         (summary.retained_document_cap == 0)) ||
        (summary.forward_chunk_count > 0 &&
         (summary.forward_document_shift == 0 ||
          summary.forward_document_shift >= 32)) ||
        (!ii42_accelerator_directory_ref_is_zero(
                &summary.scope_object) &&
         (summary.scope_object.object_kind !=
            II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_SCOPE ||
          summary.scope_object.object_id != 1 ||
          summary.scope_object.owner_manifest_id !=
            summary.owner_manifest_id ||
          ii42_segment_object_ref_validate(
            &summary.scope_object,
            UINT32_MAX) != II42_OK)) ||
        (!ii42_accelerator_directory_ref_is_zero(
                &summary.tid_lookup_object) &&
         (summary.tid_lookup_object.object_kind !=
            II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_TID_LOOKUP ||
          summary.tid_lookup_object.object_id != 1 ||
          summary.tid_lookup_object.owner_manifest_id !=
            summary.owner_manifest_id ||
          ii42_segment_object_ref_validate(
            &summary.tid_lookup_object,
            UINT32_MAX) != II42_OK)))
    {
        return II42_ERR_FORMAT;
    }
    *summary_out = summary;
    return II42_OK;
}

bool
ii42_semantic_accelerator_directory_summary_format_is_current(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    return summary != NULL &&
        summary->format_version == II42_ACCELERATOR_DIRECTORY_VERSION &&
        summary->header_size ==
            II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE &&
        summary->term_entry_size ==
            II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE &&
        summary->forward_entry_size ==
            II42_ACCELERATOR_FORWARD_ENTRY_SIZE &&
        summary->forward_term_work_count == summary->vocab_size &&
        summary->forward_chunk_cost_count ==
            summary->forward_chunk_count &&
        summary->forward_row_offset_count ==
            ii42_accelerator_directory_expected_forward_row_offsets(
                summary->document_count,
                summary->forward_chunk_count
            ) &&
        summary->forward_term_bytes_count == summary->vocab_size &&
        summary->forward_bound_term_bytes_count == summary->vocab_size &&
        summary->forward_bound_shard_count ==
            ii42_accelerator_directory_expected_forward_bound_shards(
                summary->vocab_size
            );
}

bool
ii42_semantic_accelerator_directory_summary_is_current(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    return
        ii42_semantic_accelerator_directory_summary_format_is_current(
            summary
        ) &&
        summary->builder_policy_id ==
            II42_SEMANTIC_ACCELERATOR_CURRENT_POLICY &&
        summary->retained_document_cap ==
            II42_SEMANTIC_ACCELERATOR_RETAINED_DOCUMENT_CAP &&
        ii42_semantic_accelerator_directory_summary_has_tid_lookup(summary);
}

bool
ii42_semantic_accelerator_directory_summary_has_complete_forward(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    uint64_t documents_per_chunk;
    uint64_t expected_chunks;

    if (!ii42_semantic_accelerator_directory_summary_is_current(summary) ||
        !ii42_accelerator_directory_policy_is_queryable(
            summary->builder_policy_id) ||
        summary->retained_document_cap !=
            II42_SEMANTIC_ACCELERATOR_RETAINED_DOCUMENT_CAP ||
        summary->forward_chunk_count == 0 ||
        summary->forward_document_shift == 0 ||
        summary->forward_document_shift >= 32)
    {
        return false;
    }
    documents_per_chunk = UINT64_C(1) <<
        summary->forward_document_shift;
    expected_chunks =
        ((uint64_t) summary->document_count + documents_per_chunk - 1) /
        documents_per_chunk;
    return expected_chunks == summary->forward_chunk_count;
}

ii42_status
ii42_semantic_accelerator_directory_forward_slice(
    const uint8_t *header_bytes,
    size_t header_size,
    ii42_semantic_accelerator_directory_summary *summary_out,
    size_t *offset_out,
    size_t *size_out
)
{
    ii42_semantic_accelerator_directory_summary summary;
    size_t offset;
    ii42_status status;

    if (summary_out == NULL || offset_out == NULL || size_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_semantic_accelerator_directory_summary_deserialize(
        header_bytes,
        header_size,
        &summary
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (!ii42_semantic_accelerator_directory_summary_has_complete_forward(
            &summary))
    {
        return II42_ERR_FORMAT;
    }
    offset = summary.header_size +
        (size_t) summary.term_count * II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE;
    if (offset > summary.total_size)
    {
        return II42_ERR_FORMAT;
    }
    *summary_out = summary;
    *offset_out = offset;
    *size_out =
        (size_t) summary.forward_chunk_count *
            II42_ACCELERATOR_FORWARD_ENTRY_SIZE +
        (size_t) summary.forward_term_work_count * sizeof(uint64_t) +
        (size_t) summary.forward_chunk_cost_count *
            2U * sizeof(uint64_t) +
        (size_t) summary.forward_row_offset_count * sizeof(uint32_t) +
        (size_t) summary.forward_term_bytes_count * sizeof(uint64_t) +
        (size_t) summary.forward_bound_term_bytes_count *
            sizeof(uint64_t) +
        (size_t) summary.forward_bound_shard_count *
            II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
    return II42_OK;
}

static void
ii42_accelerator_directory_write_ref(
    uint8_t *bytes,
    const ii42_segment_object_ref *ref
)
{
    ii42_accelerator_directory_write_u32(
        bytes + 0,
        (uint32_t) ref->object_kind
    );
    ii42_accelerator_directory_write_u32(bytes + 8, ref->start_block);
    ii42_accelerator_directory_write_u32(bytes + 12, ref->page_count);
    ii42_accelerator_directory_write_u64(bytes + 16, ref->object_id);
    ii42_accelerator_directory_write_u64(
        bytes + 24,
        ref->owner_manifest_id
    );
    ii42_accelerator_directory_write_u64(bytes + 32, ref->object_bytes);
    ii42_accelerator_directory_write_u64(bytes + 40, ref->object_checksum);
}

static void
ii42_accelerator_directory_read_ref(
    const uint8_t *bytes,
    ii42_segment_object_ref *ref
)
{
    memset(ref, 0, sizeof(*ref));
    ref->object_kind = (ii42_segment_object_kind)
        ii42_accelerator_directory_read_u32(bytes + 0);
    ref->start_block = ii42_accelerator_directory_read_u32(bytes + 8);
    ref->page_count = ii42_accelerator_directory_read_u32(bytes + 12);
    ref->object_id = ii42_accelerator_directory_read_u64(bytes + 16);
    ref->owner_manifest_id =
        ii42_accelerator_directory_read_u64(bytes + 24);
    ref->object_bytes = ii42_accelerator_directory_read_u64(bytes + 32);
    ref->object_checksum = ii42_accelerator_directory_read_u64(bytes + 40);
}

ii42_status
ii42_semantic_accelerator_forward_directory_deserialize(
    const ii42_semantic_accelerator_directory_summary *summary,
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
)
{
    ii42_semantic_accelerator_directory directory;
    size_t forward_bytes;
    size_t forward_term_work_bytes;
    size_t forward_chunk_cost_bytes;
    size_t forward_row_offset_bytes;
    size_t forward_term_bytes;
    size_t forward_bound_term_bytes;
    size_t forward_bound_ref_bytes;
    size_t expected_size;

    if (summary == NULL || bytes == NULL || directory_out == NULL ||
        !ii42_semantic_accelerator_directory_summary_has_complete_forward(
            summary))
    {
        return II42_ERR_INVALID;
    }
    forward_bytes = (size_t) summary->forward_chunk_count *
        II42_ACCELERATOR_FORWARD_ENTRY_SIZE;
    forward_term_work_bytes =
        (size_t) summary->forward_term_work_count * sizeof(uint64_t);
    forward_chunk_cost_bytes =
        (size_t) summary->forward_chunk_cost_count *
        2U * sizeof(uint64_t);
    if (summary->forward_row_offset_count > SIZE_MAX / sizeof(uint32_t))
    {
        return II42_ERR_RANGE;
    }
    forward_row_offset_bytes =
        (size_t) summary->forward_row_offset_count * sizeof(uint32_t);
    forward_term_bytes =
        (size_t) summary->forward_term_bytes_count * sizeof(uint64_t);
    forward_bound_term_bytes =
        (size_t) summary->forward_bound_term_bytes_count * sizeof(uint64_t);
    forward_bound_ref_bytes =
        (size_t) summary->forward_bound_shard_count *
        II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
    expected_size = forward_bytes + forward_term_work_bytes +
        forward_chunk_cost_bytes + forward_row_offset_bytes +
        forward_term_bytes + forward_bound_term_bytes +
        forward_bound_ref_bytes;
    if (size != expected_size)
    {
        return II42_ERR_FORMAT;
    }
    ii42_semantic_accelerator_directory_init(&directory);
    directory.source_manifest_id = summary->source_manifest_id;
    directory.source_authority_checksum =
        summary->source_authority_checksum;
    directory.owner_manifest_id = summary->owner_manifest_id;
    directory.document_count = summary->document_count;
    directory.vocab_size = summary->vocab_size;
    directory.term_count = summary->term_count;
    directory.forward_chunk_count = summary->forward_chunk_count;
    directory.forward_document_shift = summary->forward_document_shift;
    directory.builder_policy_id = summary->builder_policy_id;
    directory.retained_document_cap = summary->retained_document_cap;
    directory.forward_bound_shard_count =
        summary->forward_bound_shard_count;
    directory.scope_object = summary->scope_object;
    directory.tid_lookup_object = summary->tid_lookup_object;
    directory.forward_chunks = calloc(
        directory.forward_chunk_count,
        sizeof(*directory.forward_chunks)
    );
    directory.forward_term_work = calloc(
        directory.vocab_size,
        sizeof(*directory.forward_term_work)
    );
    directory.forward_row_data_bytes = calloc(
        directory.forward_chunk_count,
        sizeof(*directory.forward_row_data_bytes)
    );
    directory.forward_transpose_fixed_bytes = calloc(
        directory.forward_chunk_count,
        sizeof(*directory.forward_transpose_fixed_bytes)
    );
    directory.forward_row_offsets = calloc(
        (size_t) summary->forward_row_offset_count,
        sizeof(*directory.forward_row_offsets)
    );
    directory.forward_term_bytes = calloc(
        directory.vocab_size,
        sizeof(*directory.forward_term_bytes)
    );
    directory.forward_bound_term_bytes = calloc(
        directory.vocab_size,
        sizeof(*directory.forward_bound_term_bytes)
    );
    if (directory.forward_bound_shard_count > 0)
    {
        directory.forward_bound_shards = calloc(
            directory.forward_bound_shard_count,
            sizeof(*directory.forward_bound_shards)
        );
    }
    if (directory.forward_chunks == NULL ||
        directory.forward_term_work == NULL ||
        directory.forward_row_data_bytes == NULL ||
        directory.forward_transpose_fixed_bytes == NULL ||
        directory.forward_row_offsets == NULL ||
        directory.forward_term_bytes == NULL ||
        directory.forward_bound_term_bytes == NULL ||
        (directory.forward_bound_shard_count > 0 &&
         directory.forward_bound_shards == NULL))
    {
        ii42_semantic_accelerator_directory_free(&directory);
        return II42_ERR_NOMEM;
    }
    for (uint32_t index = 0;
         index < directory.forward_chunk_count;
         index++)
    {
        const uint8_t *entry = bytes +
            (size_t) index * II42_ACCELERATOR_FORWARD_ENTRY_SIZE;
        ii42_semantic_accelerator_forward_entry *forward =
            &directory.forward_chunks[index];
        uint32_t expected_first = index == 0
            ? 0
            : directory.forward_chunks[index - 1U].first_document +
                directory.forward_chunks[index - 1U].document_count;

        forward->first_document =
            ii42_accelerator_directory_read_u32(entry + 0);
        forward->document_count =
            ii42_accelerator_directory_read_u32(entry + 4);
        forward->posting_count =
            ii42_accelerator_directory_read_u32(entry + 8);
        forward->row_data_offset =
            ii42_accelerator_directory_read_u32(entry + 12);
        ii42_accelerator_directory_read_ref(
            entry + 16,
            &forward->forward_object
        );
        if (forward->first_document != expected_first ||
            forward->document_count == 0 ||
            forward->document_count > directory.document_count ||
            forward->first_document >
                directory.document_count - forward->document_count ||
            forward->row_data_offset == 0 ||
            forward->forward_object.object_kind !=
                II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD ||
            forward->forward_object.object_id != (uint64_t) index + 1U ||
            forward->forward_object.owner_manifest_id !=
                directory.owner_manifest_id ||
            ii42_segment_object_ref_validate(
                &forward->forward_object,
                UINT32_MAX) != II42_OK)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_FORMAT;
        }
    }
    for (uint64_t offset_index = 0;
         offset_index < summary->forward_row_offset_count;
         offset_index++)
    {
        directory.forward_row_offsets[offset_index] =
            ii42_accelerator_directory_read_u32(
                bytes + forward_bytes + forward_term_work_bytes +
                    forward_chunk_cost_bytes +
                    (size_t) offset_index * sizeof(uint32_t)
            );
    }
    if (directory.forward_chunks[
            directory.forward_chunk_count - 1U
        ].first_document + directory.forward_chunks[
            directory.forward_chunk_count - 1U
        ].document_count != directory.document_count)
    {
        ii42_semantic_accelerator_directory_free(&directory);
        return II42_ERR_FORMAT;
    }
    for (uint32_t term_id = 0;
         term_id < directory.vocab_size;
         term_id++)
    {
        directory.forward_term_work[term_id] =
            ii42_accelerator_directory_read_u64(
                bytes + forward_bytes +
                    (size_t) term_id * sizeof(uint64_t)
            );
    }
    for (uint32_t chunk = 0;
         chunk < directory.forward_chunk_count;
         chunk++)
    {
        size_t offset = forward_bytes + forward_term_work_bytes +
            (size_t) chunk * 2U * sizeof(uint64_t);

        directory.forward_row_data_bytes[chunk] =
            ii42_accelerator_directory_read_u64(
                bytes + offset
            );
        directory.forward_transpose_fixed_bytes[chunk] =
            ii42_accelerator_directory_read_u64(
                bytes + offset + sizeof(uint64_t)
            );
        {
            const ii42_semantic_accelerator_forward_entry *forward =
                &directory.forward_chunks[chunk];
            uint64_t row_offset_index =
                (uint64_t) forward->first_document + chunk;
            uint32_t previous =
                directory.forward_row_offsets[row_offset_index];

            if (previous != 0 ||
                forward->row_data_offset >
                    forward->forward_object.object_bytes ||
                directory.forward_row_data_bytes[chunk] >
                    forward->forward_object.object_bytes -
                        forward->row_data_offset)
            {
                ii42_semantic_accelerator_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
            for (uint32_t row = 0; row < forward->document_count; row++)
            {
                uint32_t next = directory.forward_row_offsets[
                    row_offset_index + row + 1U
                ];

                if (next < previous)
                {
                    ii42_semantic_accelerator_directory_free(&directory);
                    return II42_ERR_FORMAT;
                }
                previous = next;
            }
            if (previous != directory.forward_row_data_bytes[chunk])
            {
                ii42_semantic_accelerator_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
        }
    }
    for (uint32_t term_id = 0;
         term_id < directory.vocab_size;
         term_id++)
    {
        size_t offset = forward_bytes + forward_term_work_bytes +
            forward_chunk_cost_bytes + forward_row_offset_bytes +
            (size_t) term_id * sizeof(uint64_t);

        directory.forward_term_bytes[term_id] =
            ii42_accelerator_directory_read_u64(bytes + offset);
        directory.forward_bound_term_bytes[term_id] =
            ii42_accelerator_directory_read_u64(
                bytes + offset + forward_term_bytes
            );
    }
    for (uint32_t index = 0;
         index < directory.forward_bound_shard_count;
         index++)
    {
        const uint8_t *entry = bytes + forward_bytes +
            forward_term_work_bytes + forward_chunk_cost_bytes +
            forward_row_offset_bytes + forward_term_bytes +
            forward_bound_term_bytes +
            (size_t) index * II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
        ii42_segment_object_ref *bound =
            &directory.forward_bound_shards[index];

        ii42_accelerator_directory_read_ref(entry, bound);
        if (bound->object_kind !=
                II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD_BOUND ||
            bound->object_id != (uint64_t) index + 1U ||
            bound->owner_manifest_id != directory.owner_manifest_id ||
            ii42_segment_object_ref_validate(bound, UINT32_MAX) != II42_OK)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_FORMAT;
        }
        for (uint32_t previous = 0; previous < index; previous++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    bound,
                    &directory.forward_bound_shards[previous]))
            {
                ii42_semantic_accelerator_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t forward = 0;
             forward < directory.forward_chunk_count;
             forward++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    bound,
                    &directory.forward_chunks[forward].forward_object))
            {
                ii42_semantic_accelerator_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
        }
    }
    ii42_semantic_accelerator_directory_free(directory_out);
    *directory_out = directory;
    return II42_OK;
}

static bool
ii42_accelerator_directory_refs_overlap(
    const ii42_segment_object_ref *left,
    const ii42_segment_object_ref *right
)
{
    uint64_t left_end = (uint64_t) left->start_block + left->page_count;
    uint64_t right_end = (uint64_t) right->start_block + right->page_count;

    return (uint64_t) left->start_block < right_end &&
        (uint64_t) right->start_block < left_end;
}

static bool
ii42_accelerator_directory_ref_is_zero(
    const ii42_segment_object_ref *ref
)
{
    static const ii42_segment_object_ref zero_ref = {0};

    return ref != NULL && memcmp(ref, &zero_ref, sizeof(*ref)) == 0;
}

void
ii42_semantic_accelerator_directory_init(
    ii42_semantic_accelerator_directory *directory
)
{
    if (directory != NULL)
    {
        memset(directory, 0, sizeof(*directory));
    }
}

void
ii42_semantic_accelerator_directory_free(
    ii42_semantic_accelerator_directory *directory
)
{
    if (directory == NULL)
    {
        return;
    }
    free(directory->terms);
    free(directory->forward_chunks);
    free(directory->forward_term_work);
    free(directory->forward_row_data_bytes);
    free(directory->forward_transpose_fixed_bytes);
    free(directory->forward_row_offsets);
    free(directory->forward_term_bytes);
    free(directory->forward_bound_term_bytes);
    free(directory->forward_bound_shards);
    memset(directory, 0, sizeof(*directory));
}

ii42_status
ii42_semantic_accelerator_directory_validate(
    const ii42_semantic_accelerator_directory *directory
)
{
    uint32_t index;

    if (directory == NULL || directory->source_manifest_id == 0 ||
        directory->source_authority_checksum == 0 ||
        directory->owner_manifest_id <= directory->source_manifest_id ||
        directory->document_count == 0 || directory->vocab_size == 0 ||
        directory->term_count == 0 || directory->terms == NULL ||
        ((directory->builder_policy_id == 0) !=
         (directory->retained_document_cap == 0)) ||
        (directory->forward_chunk_count > 0 &&
         (directory->forward_chunks == NULL ||
          directory->forward_document_shift == 0 ||
          directory->forward_document_shift >= 32)) ||
        ((directory->forward_row_offsets == NULL) !=
         (directory->forward_row_data_bytes == NULL)) ||
        (directory->forward_bound_shard_count == 0 &&
         directory->forward_bound_shards != NULL) ||
        (directory->forward_bound_shard_count > 0 &&
         (directory->forward_bound_shards == NULL ||
          directory->forward_bound_shard_count !=
            ii42_accelerator_directory_expected_forward_bound_shards(
                directory->vocab_size
            ))))
    {
        return II42_ERR_FORMAT;
    }
    if (!ii42_accelerator_directory_ref_is_zero(
            &directory->scope_object) &&
        (directory->scope_object.object_kind !=
            II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_SCOPE ||
         directory->scope_object.object_id != 1 ||
         directory->scope_object.owner_manifest_id !=
            directory->owner_manifest_id ||
         ii42_segment_object_ref_validate(
            &directory->scope_object,
            UINT32_MAX) != II42_OK))
    {
        return II42_ERR_FORMAT;
    }
    if (!ii42_accelerator_directory_ref_is_zero(
            &directory->tid_lookup_object) &&
        (directory->tid_lookup_object.object_kind !=
            II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_TID_LOOKUP ||
         directory->tid_lookup_object.object_id != 1 ||
         directory->tid_lookup_object.owner_manifest_id !=
            directory->owner_manifest_id ||
         ii42_segment_object_ref_validate(
            &directory->tid_lookup_object,
            UINT32_MAX) != II42_OK))
    {
        return II42_ERR_FORMAT;
    }
    for (index = 0; index < directory->term_count; index++)
    {
        const ii42_semantic_accelerator_directory_entry *entry =
            &directory->terms[index];
        const ii42_segment_object_ref *ref = &entry->term_object;
        uint32_t other_index;

        if (entry->term_id >= directory->vocab_size ||
            (index > 0 &&
             directory->terms[index - 1].term_id >= entry->term_id) ||
            ref->object_kind !=
                II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_TERM ||
            ref->object_id != (uint64_t) entry->term_id + 1 ||
            ref->owner_manifest_id != directory->owner_manifest_id ||
            ii42_segment_object_ref_validate(ref, UINT32_MAX) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        for (other_index = 0; other_index < index; other_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->terms[other_index].term_object))
            {
                return II42_ERR_FORMAT;
            }
        }
    }
    for (index = 0; index < directory->forward_chunk_count; index++)
    {
        const ii42_semantic_accelerator_forward_entry *entry =
            &directory->forward_chunks[index];
        const ii42_segment_object_ref *ref = &entry->forward_object;
        uint32_t expected_first = index == 0
            ? 0
            : directory->forward_chunks[index - 1U].first_document +
                directory->forward_chunks[index - 1U].document_count;

        if (entry->first_document != expected_first ||
            entry->document_count == 0 ||
            entry->document_count > directory->document_count ||
            entry->first_document >
                directory->document_count - entry->document_count ||
            (directory->forward_row_offsets != NULL
                ? entry->row_data_offset == 0
                : entry->row_data_offset != 0) ||
            ref->object_kind !=
                II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD ||
            ref->object_id != (uint64_t) index + 1U ||
            ref->owner_manifest_id != directory->owner_manifest_id ||
            ii42_segment_object_ref_validate(ref, UINT32_MAX) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        if (directory->forward_row_offsets != NULL)
        {
            uint64_t offset_index = (uint64_t) entry->first_document + index;
            uint32_t previous = directory->forward_row_offsets[offset_index];

            if (previous != 0)
            {
                return II42_ERR_FORMAT;
            }
            for (uint32_t row = 0; row < entry->document_count; row++)
            {
                uint32_t next = directory->forward_row_offsets[
                    offset_index + row + 1U
                ];

                if (next < previous)
                {
                    return II42_ERR_FORMAT;
                }
                previous = next;
            }
            if (directory->forward_row_data_bytes != NULL &&
                previous != directory->forward_row_data_bytes[index])
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t term_index = 0;
             term_index < directory->term_count;
             term_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->terms[term_index].term_object))
            {
                return II42_ERR_FORMAT;
            }
        }
        if (!ii42_accelerator_directory_ref_is_zero(
                &directory->scope_object) &&
            ii42_accelerator_directory_refs_overlap(
                ref,
                &directory->scope_object))
        {
            return II42_ERR_FORMAT;
        }
        if (!ii42_accelerator_directory_ref_is_zero(
                &directory->tid_lookup_object) &&
            ii42_accelerator_directory_refs_overlap(
                ref,
                &directory->tid_lookup_object))
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t other = 0; other < index; other++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->forward_chunks[other].forward_object))
            {
                return II42_ERR_FORMAT;
            }
        }
    }
    if (!ii42_accelerator_directory_ref_is_zero(
            &directory->scope_object))
    {
        for (uint32_t term_index = 0;
             term_index < directory->term_count;
             term_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    &directory->scope_object,
                    &directory->terms[term_index].term_object))
            {
                return II42_ERR_FORMAT;
            }
        }
    }
    if (!ii42_accelerator_directory_ref_is_zero(
            &directory->tid_lookup_object))
    {
        for (uint32_t term_index = 0;
             term_index < directory->term_count;
             term_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    &directory->tid_lookup_object,
                    &directory->terms[term_index].term_object))
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t forward_index = 0;
             forward_index < directory->forward_chunk_count;
             forward_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    &directory->tid_lookup_object,
                    &directory->forward_chunks[
                        forward_index
                    ].forward_object))
            {
                return II42_ERR_FORMAT;
            }
        }
        if (!ii42_accelerator_directory_ref_is_zero(
                &directory->scope_object) &&
            ii42_accelerator_directory_refs_overlap(
                &directory->tid_lookup_object,
                &directory->scope_object))
        {
            return II42_ERR_FORMAT;
        }
    }
    for (index = 0;
         index < directory->forward_bound_shard_count;
         index++)
    {
        const ii42_segment_object_ref *ref =
            &directory->forward_bound_shards[index];

        if (ref->object_kind !=
                II42_SEGMENT_OBJECT_SEMANTIC_ACCELERATOR_FORWARD_BOUND ||
            ref->object_id != (uint64_t) index + 1U ||
            ref->owner_manifest_id != directory->owner_manifest_id ||
            ii42_segment_object_ref_validate(ref, UINT32_MAX) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t other = 0; other < index; other++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->forward_bound_shards[other]))
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t term_index = 0;
             term_index < directory->term_count;
             term_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->terms[term_index].term_object))
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t forward_index = 0;
             forward_index < directory->forward_chunk_count;
             forward_index++)
        {
            if (ii42_accelerator_directory_refs_overlap(
                    ref,
                    &directory->forward_chunks[
                        forward_index
                    ].forward_object))
            {
                return II42_ERR_FORMAT;
            }
        }
        if ((!ii42_accelerator_directory_ref_is_zero(
                 &directory->scope_object) &&
             ii42_accelerator_directory_refs_overlap(
                 ref,
                 &directory->scope_object)) ||
            (!ii42_accelerator_directory_ref_is_zero(
                 &directory->tid_lookup_object) &&
             ii42_accelerator_directory_refs_overlap(
                 ref,
                 &directory->tid_lookup_object)))
        {
            return II42_ERR_FORMAT;
        }
    }
    if (directory->forward_chunk_count > 0)
    {
        const ii42_semantic_accelerator_forward_entry *last =
            &directory->forward_chunks[directory->forward_chunk_count - 1U];

        if (last->first_document + last->document_count !=
            directory->document_count)
        {
            return II42_ERR_FORMAT;
        }
    }
    return II42_OK;
}

const ii42_semantic_accelerator_directory_entry *
ii42_semantic_accelerator_directory_find(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t term_id
)
{
    uint32_t low = 0;
    uint32_t high;

    if (directory == NULL || directory->terms == NULL)
    {
        return NULL;
    }
    high = directory->term_count;
    while (low < high)
    {
        uint32_t middle = low + (high - low) / 2;
        uint32_t candidate = directory->terms[middle].term_id;

        if (candidate < term_id)
        {
            low = middle + 1;
        }
        else
        {
            high = middle;
        }
    }
    return low < directory->term_count &&
        directory->terms[low].term_id == term_id
        ? &directory->terms[low]
        : NULL;
}

const ii42_semantic_accelerator_forward_entry *
ii42_semantic_accelerator_directory_find_forward(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t document_id
)
{
    uint32_t low = 0;
    uint32_t high;

    if (directory == NULL || directory->forward_chunks == NULL ||
        document_id >= directory->document_count)
    {
        return NULL;
    }
    high = directory->forward_chunk_count;
    while (low < high)
    {
        uint32_t middle = low + (high - low) / 2U;
        const ii42_semantic_accelerator_forward_entry *entry =
            &directory->forward_chunks[middle];

        if (document_id < entry->first_document)
        {
            high = middle;
        }
        else if (document_id >= entry->first_document + entry->document_count)
        {
            low = middle + 1U;
        }
        else
        {
            return entry;
        }
    }
    return NULL;
}

const ii42_segment_object_ref *
ii42_semantic_accelerator_directory_find_forward_bound(
    const ii42_semantic_accelerator_directory *directory,
    uint32_t term_id
)
{
    uint32_t shard;

    if (!ii42_semantic_accelerator_directory_has_complete_forward_bounds(
            directory) ||
        term_id >= directory->vocab_size)
    {
        return NULL;
    }
    shard = term_id / II42_SEMANTIC_FORWARD_BOUND_TERMS_PER_SHARD;
    return &directory->forward_bound_shards[shard];
}

bool
ii42_semantic_accelerator_directory_has_scope(
    const ii42_semantic_accelerator_directory *directory
)
{
    return directory != NULL &&
        directory->builder_policy_id ==
            II42_SEMANTIC_ACCELERATOR_POLICY_SCOPE_FORWARD_INT8 &&
        !ii42_accelerator_directory_ref_is_zero(&directory->scope_object);
}

bool
ii42_semantic_accelerator_directory_summary_has_scope(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    return
        ii42_semantic_accelerator_directory_summary_format_is_current(
            summary
        ) &&
        summary->builder_policy_id ==
            II42_SEMANTIC_ACCELERATOR_POLICY_SCOPE_FORWARD_INT8 &&
        !ii42_accelerator_directory_ref_is_zero(&summary->scope_object);
}

bool
ii42_semantic_accelerator_directory_has_tid_lookup(
    const ii42_semantic_accelerator_directory *directory
)
{
    return directory != NULL &&
        !ii42_accelerator_directory_ref_is_zero(
            &directory->tid_lookup_object
        );
}

bool
ii42_semantic_accelerator_directory_summary_has_tid_lookup(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    return
        ii42_semantic_accelerator_directory_summary_format_is_current(
            summary
        ) &&
        !ii42_accelerator_directory_ref_is_zero(
            &summary->tid_lookup_object
        );
}

bool
ii42_semantic_accelerator_directory_has_complete_forward(
    const ii42_semantic_accelerator_directory *directory
)
{
    return directory != NULL &&
        ii42_accelerator_directory_policy_is_queryable(
            directory->builder_policy_id) &&
        directory->retained_document_cap ==
            II42_SEMANTIC_ACCELERATOR_RETAINED_DOCUMENT_CAP &&
        directory->forward_term_work != NULL &&
        directory->forward_chunk_count > 0 &&
        ii42_semantic_accelerator_directory_validate(directory) == II42_OK;
}

bool
ii42_semantic_accelerator_directory_has_complete_forward_bounds(
    const ii42_semantic_accelerator_directory *directory
)
{
    return directory != NULL &&
        directory->forward_bound_shard_count ==
            ii42_accelerator_directory_expected_forward_bound_shards(
                directory->vocab_size
            ) &&
        directory->forward_bound_shards != NULL;
}

bool
ii42_semantic_accelerator_directory_summary_has_complete_forward_bounds(
    const ii42_semantic_accelerator_directory_summary *summary
)
{
    return summary != NULL &&
        ii42_semantic_accelerator_directory_summary_format_is_current(
            summary
        ) &&
        summary->forward_bound_shard_count ==
            ii42_accelerator_directory_expected_forward_bound_shards(
                summary->vocab_size
            );
}

bool
ii42_semantic_accelerator_directory_is_current(
    const ii42_semantic_accelerator_directory *directory
)
{
    return directory != NULL &&
        directory->builder_policy_id ==
            II42_SEMANTIC_ACCELERATOR_CURRENT_POLICY &&
        directory->retained_document_cap ==
            II42_SEMANTIC_ACCELERATOR_RETAINED_DOCUMENT_CAP &&
        directory->forward_term_work != NULL &&
        directory->forward_row_data_bytes != NULL &&
        directory->forward_transpose_fixed_bytes != NULL &&
        directory->forward_row_offsets != NULL &&
        directory->forward_term_bytes != NULL &&
        directory->forward_bound_term_bytes != NULL &&
        ii42_semantic_accelerator_directory_has_complete_forward_bounds(
            directory
        ) &&
        ii42_semantic_accelerator_directory_has_tid_lookup(directory) &&
        ii42_semantic_accelerator_directory_validate(directory) == II42_OK;
}

ii42_status
ii42_semantic_accelerator_directory_serialize(
    const ii42_semantic_accelerator_directory *directory,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    uint8_t *bytes;
    size_t entry_bytes;
    size_t forward_bytes;
    size_t forward_term_work_bytes;
    size_t forward_chunk_cost_bytes;
    size_t forward_row_offset_bytes;
    size_t forward_term_bytes;
    size_t forward_bound_term_bytes;
    size_t forward_bound_ref_bytes;
    size_t total_size;
    uint32_t index;
    ii42_status status;

    if (bytes_out == NULL || size_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    *bytes_out = NULL;
    *size_out = 0;
    status = ii42_semantic_accelerator_directory_validate(directory);
    if (status != II42_OK ||
        !ii42_semantic_accelerator_directory_is_current(directory))
    {
        return status == II42_OK ? II42_ERR_FORMAT : status;
    }
    entry_bytes = (size_t) directory->term_count *
        II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE;
    forward_bytes = (size_t) directory->forward_chunk_count *
        II42_ACCELERATOR_FORWARD_ENTRY_SIZE;
    forward_term_work_bytes =
        (size_t) directory->vocab_size * sizeof(uint64_t);
    forward_chunk_cost_bytes =
        (size_t) directory->forward_chunk_count *
        2U * sizeof(uint64_t);
    if (ii42_accelerator_directory_expected_forward_row_offsets(
            directory->document_count,
            directory->forward_chunk_count
        ) > SIZE_MAX / sizeof(uint32_t))
    {
        return II42_ERR_RANGE;
    }
    forward_row_offset_bytes =
        (size_t) ii42_accelerator_directory_expected_forward_row_offsets(
            directory->document_count,
            directory->forward_chunk_count
        ) * sizeof(uint32_t);
    forward_term_bytes =
        (size_t) directory->vocab_size * sizeof(uint64_t);
    forward_bound_term_bytes =
        (size_t) directory->vocab_size * sizeof(uint64_t);
    forward_bound_ref_bytes =
        (size_t) directory->forward_bound_shard_count *
        II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
    if ((directory->term_count != 0 &&
         entry_bytes / II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE !=
             directory->term_count) ||
        (directory->forward_chunk_count != 0 &&
         forward_bytes / II42_ACCELERATOR_FORWARD_ENTRY_SIZE !=
             directory->forward_chunk_count) ||
        (directory->forward_chunk_count != 0 &&
         forward_chunk_cost_bytes / (2U * sizeof(uint64_t)) !=
             directory->forward_chunk_count) ||
        (forward_row_offset_bytes != 0 &&
         forward_row_offset_bytes / sizeof(uint32_t) !=
            ii42_accelerator_directory_expected_forward_row_offsets(
                directory->document_count,
                directory->forward_chunk_count
            )) ||
        (directory->vocab_size != 0 &&
         (forward_term_work_bytes / sizeof(uint64_t) !=
              directory->vocab_size ||
          forward_term_bytes / sizeof(uint64_t) !=
              directory->vocab_size ||
          forward_bound_term_bytes / sizeof(uint64_t) !=
              directory->vocab_size)) ||
        (directory->forward_bound_shard_count != 0 &&
         forward_bound_ref_bytes /
                II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE !=
             directory->forward_bound_shard_count) ||
        entry_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE ||
        forward_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
            entry_bytes ||
        forward_term_work_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
            entry_bytes - forward_bytes ||
        forward_chunk_cost_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
                entry_bytes - forward_bytes - forward_term_work_bytes ||
        forward_row_offset_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
                entry_bytes - forward_bytes - forward_term_work_bytes -
                forward_chunk_cost_bytes ||
        forward_term_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
                entry_bytes - forward_bytes - forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes ||
        forward_bound_term_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
                entry_bytes - forward_bytes - forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes ||
        forward_bound_ref_bytes >
            SIZE_MAX - II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE -
                entry_bytes - forward_bytes - forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes -
                forward_bound_term_bytes)
    {
        return II42_ERR_RANGE;
    }
    total_size = II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
        entry_bytes + forward_bytes + forward_term_work_bytes +
        forward_chunk_cost_bytes + forward_row_offset_bytes +
        forward_term_bytes +
        forward_bound_term_bytes +
        forward_bound_ref_bytes;
    bytes = calloc(total_size, 1);
    if (bytes == NULL)
    {
        return II42_ERR_NOMEM;
    }

    ii42_accelerator_directory_write_u32(
        bytes + 0,
        II42_ACCELERATOR_DIRECTORY_MAGIC
    );
    ii42_accelerator_directory_write_u16(
        bytes + 4,
        II42_ACCELERATOR_DIRECTORY_VERSION
    );
    ii42_accelerator_directory_write_u16(
        bytes + 6,
        II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE
    );
    ii42_accelerator_directory_write_u16(
        bytes + 8,
        II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE
    );
    ii42_accelerator_directory_write_u16(
        bytes + 10,
        II42_ACCELERATOR_FORWARD_ENTRY_SIZE
    );
    ii42_accelerator_directory_write_u32(
        bytes + 12,
        directory->document_count
    );
    ii42_accelerator_directory_write_u32(bytes + 16, directory->vocab_size);
    ii42_accelerator_directory_write_u32(bytes + 20, directory->term_count);
    ii42_accelerator_directory_write_u32(
        bytes + 24,
        directory->forward_chunk_count
    );
    ii42_accelerator_directory_write_u32(
        bytes + 28,
        directory->forward_document_shift
    );
    ii42_accelerator_directory_write_u64(
        bytes + 32,
        directory->source_manifest_id
    );
    ii42_accelerator_directory_write_u64(
        bytes + 40,
        directory->source_authority_checksum
    );
    ii42_accelerator_directory_write_u64(
        bytes + 48,
        directory->owner_manifest_id
    );
    ii42_accelerator_directory_write_u64(bytes + 56, total_size);
    ii42_accelerator_directory_write_u32(
        bytes + 72,
        directory->builder_policy_id
    );
    ii42_accelerator_directory_write_u32(
        bytes + 76,
        directory->retained_document_cap
    );
    ii42_accelerator_directory_write_ref(
        bytes + 80,
        &directory->scope_object
    );
    ii42_accelerator_directory_write_ref(
        bytes + II42_ACCELERATOR_DIRECTORY_SCOPE_HEADER_SIZE,
        &directory->tid_lookup_object
    );
    for (index = 0; index < directory->term_count; index++)
    {
        uint8_t *entry = bytes +
            II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
            (size_t) index * II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE;

        ii42_accelerator_directory_write_u32(
            entry + 0,
            directory->terms[index].term_id
        );
        ii42_accelerator_directory_write_ref(
            entry + 8,
            &directory->terms[index].term_object
        );
    }
    for (index = 0; index < directory->forward_chunk_count; index++)
    {
        uint8_t *entry = bytes +
            II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
            entry_bytes +
            (size_t) index * II42_ACCELERATOR_FORWARD_ENTRY_SIZE;

        ii42_accelerator_directory_write_u32(
            entry + 0,
            directory->forward_chunks[index].first_document
        );
        ii42_accelerator_directory_write_u32(
            entry + 4,
            directory->forward_chunks[index].document_count
        );
        ii42_accelerator_directory_write_u32(
            entry + 8,
            directory->forward_chunks[index].posting_count
        );
        ii42_accelerator_directory_write_u32(
            entry + 12,
            directory->forward_chunks[index].row_data_offset
        );
        ii42_accelerator_directory_write_ref(
            entry + 16,
            &directory->forward_chunks[index].forward_object
        );
    }
    for (index = 0; index < directory->vocab_size; index++)
    {
        size_t metrics_offset =
            II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
            entry_bytes + forward_bytes + forward_term_work_bytes +
            forward_chunk_cost_bytes + forward_row_offset_bytes;

        ii42_accelerator_directory_write_u64(
            bytes + II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
                entry_bytes + forward_bytes +
                (size_t) index * sizeof(uint64_t),
            directory->forward_term_work[index]
        );

        ii42_accelerator_directory_write_u64(
            bytes + metrics_offset +
                (size_t) index * sizeof(uint64_t),
            directory->forward_term_bytes[index]
        );
        ii42_accelerator_directory_write_u64(
            bytes + metrics_offset + forward_term_bytes +
                (size_t) index * sizeof(uint64_t),
            directory->forward_bound_term_bytes[index]
        );
    }
    for (uint64_t offset_index = 0;
         offset_index <
            ii42_accelerator_directory_expected_forward_row_offsets(
                directory->document_count,
                directory->forward_chunk_count
            );
         offset_index++)
    {
        ii42_accelerator_directory_write_u32(
            bytes + II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
                entry_bytes + forward_bytes + forward_term_work_bytes +
                forward_chunk_cost_bytes +
                (size_t) offset_index * sizeof(uint32_t),
            directory->forward_row_offsets[offset_index]
        );
    }
    for (index = 0; index < directory->forward_chunk_count; index++)
    {
        size_t cost_offset =
            II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
            entry_bytes + forward_bytes + forward_term_work_bytes +
            (size_t) index * 2U * sizeof(uint64_t);

        ii42_accelerator_directory_write_u64(
            bytes + cost_offset,
            directory->forward_row_data_bytes[index]
        );
        ii42_accelerator_directory_write_u64(
            bytes + cost_offset + sizeof(uint64_t),
            directory->forward_transpose_fixed_bytes[index]
        );
    }
    for (index = 0;
         index < directory->forward_bound_shard_count;
         index++)
    {
        ii42_accelerator_directory_write_ref(
            bytes + II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE +
                entry_bytes + forward_bytes + forward_term_work_bytes +
                forward_chunk_cost_bytes + forward_row_offset_bytes +
                forward_term_bytes +
                forward_bound_term_bytes +
                (size_t) index *
                    II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE,
            &directory->forward_bound_shards[index]
        );
    }
    ii42_accelerator_directory_write_u64(
        bytes + II42_ACCELERATOR_DIRECTORY_CHECKSUM_OFFSET,
        ii42_accelerator_directory_checksum(bytes, total_size)
    );
    *bytes_out = bytes;
    *size_out = total_size;
    return II42_OK;
}

static ii42_status
ii42_semantic_accelerator_directory_deserialize_internal(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out,
    bool allow_retirement
)
{
    ii42_semantic_accelerator_directory directory;
    uint64_t total_size;
    size_t entry_bytes;
    size_t forward_bytes;
    size_t forward_term_work_bytes;
    size_t forward_chunk_cost_bytes;
    size_t forward_row_offset_bytes;
    size_t forward_term_bytes;
    size_t forward_bound_term_bytes;
    size_t forward_bound_ref_bytes;
    uint16_t version;
    uint16_t header_size;
    uint16_t forward_entry_size;
    uint32_t index;
    ii42_status status;

    if (bytes == NULL || directory_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (size < II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE)
    {
        return II42_ERR_FORMAT;
    }
    version = ii42_accelerator_directory_read_u16(bytes + 4);
    header_size = ii42_accelerator_directory_read_u16(bytes + 6);
    forward_entry_size = ii42_accelerator_directory_read_u16(bytes + 10);
    if (ii42_accelerator_directory_read_u32(bytes + 0) !=
            II42_ACCELERATOR_DIRECTORY_MAGIC ||
        (version != II42_ACCELERATOR_DIRECTORY_VERSION &&
         (!allow_retirement ||
          version < II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION ||
          version > II42_ACCELERATOR_DIRECTORY_RETIREMENT_MAX_VERSION)) ||
        header_size != II42_SEMANTIC_ACCELERATOR_DIRECTORY_HEADER_SIZE ||
        size < header_size ||
        ii42_accelerator_directory_read_u16(bytes + 8) !=
            II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE ||
        forward_entry_size !=
            (version == II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION
                ? II42_ACCELERATOR_RETIREMENT_V5_FORWARD_ENTRY_SIZE
                : II42_ACCELERATOR_FORWARD_ENTRY_SIZE) ||
        ii42_accelerator_directory_read_u64(
            bytes + II42_ACCELERATOR_DIRECTORY_CHECKSUM_OFFSET) !=
            ii42_accelerator_directory_checksum(bytes, size))
    {
        return II42_ERR_FORMAT;
    }
    total_size = ii42_accelerator_directory_read_u64(bytes + 56);
    ii42_semantic_accelerator_directory_init(&directory);
    directory.document_count =
        ii42_accelerator_directory_read_u32(bytes + 12);
    directory.vocab_size = ii42_accelerator_directory_read_u32(bytes + 16);
    directory.term_count = ii42_accelerator_directory_read_u32(bytes + 20);
    directory.forward_chunk_count =
        ii42_accelerator_directory_read_u32(bytes + 24);
    directory.forward_document_shift =
        ii42_accelerator_directory_read_u32(bytes + 28);
    directory.source_manifest_id =
        ii42_accelerator_directory_read_u64(bytes + 32);
    directory.source_authority_checksum =
        ii42_accelerator_directory_read_u64(bytes + 40);
    directory.owner_manifest_id =
        ii42_accelerator_directory_read_u64(bytes + 48);
    directory.builder_policy_id =
        ii42_accelerator_directory_read_u32(bytes + 72);
    directory.retained_document_cap =
        ii42_accelerator_directory_read_u32(bytes + 76);
    ii42_accelerator_directory_read_ref(bytes + 80, &directory.scope_object);
    ii42_accelerator_directory_read_ref(
        bytes + II42_ACCELERATOR_DIRECTORY_SCOPE_HEADER_SIZE,
        &directory.tid_lookup_object
    );
    entry_bytes = (size_t) directory.term_count *
        II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE;
    forward_bytes = (size_t) directory.forward_chunk_count *
        forward_entry_size;
    forward_term_work_bytes =
        version >= UINT16_C(7)
            ? (size_t) directory.vocab_size * sizeof(uint64_t)
            : 0;
    forward_chunk_cost_bytes =
        version >= II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION
            ? (size_t) directory.forward_chunk_count *
                2U * sizeof(uint64_t)
            : 0;
    if (version == II42_ACCELERATOR_DIRECTORY_VERSION &&
        ii42_accelerator_directory_expected_forward_row_offsets(
            directory.document_count,
            directory.forward_chunk_count
        ) > SIZE_MAX / sizeof(uint32_t))
    {
        return II42_ERR_RANGE;
    }
    forward_row_offset_bytes =
        version == II42_ACCELERATOR_DIRECTORY_VERSION
            ? (size_t)
                ii42_accelerator_directory_expected_forward_row_offsets(
                    directory.document_count,
                    directory.forward_chunk_count
                ) * sizeof(uint32_t)
            : 0;
    forward_term_bytes =
        version >= II42_ACCELERATOR_DIRECTORY_PHYSICAL_COST_VERSION
            ? (size_t) directory.vocab_size * sizeof(uint64_t)
            : 0;
    forward_bound_term_bytes = forward_term_bytes;
    directory.forward_bound_shard_count =
        version >= UINT16_C(8)
            ? ii42_accelerator_directory_expected_forward_bound_shards(
                  directory.vocab_size
              )
            : 0;
    forward_bound_ref_bytes =
        (size_t) directory.forward_bound_shard_count *
        II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE;
    if (total_size != size ||
        directory.term_count >
            (size - header_size) /
                II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE ||
        directory.forward_chunk_count >
            (size - header_size) /
                forward_entry_size ||
        entry_bytes > SIZE_MAX - header_size ||
        forward_bytes > SIZE_MAX - header_size - entry_bytes ||
        forward_term_work_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes ||
        forward_chunk_cost_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes ||
        forward_row_offset_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes - forward_chunk_cost_bytes ||
        forward_term_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes ||
        forward_bound_term_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes ||
        forward_bound_ref_bytes >
            SIZE_MAX - header_size - entry_bytes - forward_bytes -
                forward_term_work_bytes -
                forward_chunk_cost_bytes - forward_row_offset_bytes -
                forward_term_bytes -
                forward_bound_term_bytes ||
        header_size + entry_bytes + forward_bytes +
            forward_term_work_bytes + forward_chunk_cost_bytes +
            forward_row_offset_bytes + forward_term_bytes +
            forward_bound_term_bytes +
            forward_bound_ref_bytes != size)
    {
        return II42_ERR_FORMAT;
    }
    directory.terms = calloc(directory.term_count, sizeof(*directory.terms));
    if (directory.terms == NULL)
    {
        return II42_ERR_NOMEM;
    }
    if (directory.forward_chunk_count > 0)
    {
        directory.forward_chunks = calloc(
            directory.forward_chunk_count,
            sizeof(*directory.forward_chunks)
        );
        if (directory.forward_chunks == NULL)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_NOMEM;
        }
    }
    for (index = 0; index < directory.term_count; index++)
    {
        const uint8_t *entry =
            bytes + header_size +
            (size_t) index * II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE;

        if (ii42_accelerator_directory_read_u32(entry + 4) != 0)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_FORMAT;
        }
        directory.terms[index].term_id =
            ii42_accelerator_directory_read_u32(entry + 0);
        ii42_accelerator_directory_read_ref(
            entry + 8,
            &directory.terms[index].term_object
        );
    }
    for (index = 0; index < directory.forward_chunk_count; index++)
    {
        const uint8_t *entry =
            bytes + header_size +
            (size_t) directory.term_count *
                II42_ACCELERATOR_DIRECTORY_ENTRY_SIZE +
            (size_t) index * forward_entry_size;

        directory.forward_chunks[index].first_document =
            ii42_accelerator_directory_read_u32(entry + 0);
        directory.forward_chunks[index].document_count =
            ii42_accelerator_directory_read_u32(entry + 4);
        if (version != II42_ACCELERATOR_DIRECTORY_RETIREMENT_MIN_VERSION)
        {
            directory.forward_chunks[index].posting_count =
                ii42_accelerator_directory_read_u32(entry + 8);
            directory.forward_chunks[index].row_data_offset =
                version == II42_ACCELERATOR_DIRECTORY_VERSION
                    ? ii42_accelerator_directory_read_u32(entry + 12)
                    : 0;
            if (version != II42_ACCELERATOR_DIRECTORY_VERSION &&
                ii42_accelerator_directory_read_u32(entry + 12) != 0)
            {
                ii42_semantic_accelerator_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
            ii42_accelerator_directory_read_ref(
                entry + 16,
                &directory.forward_chunks[index].forward_object
            );
        }
        else
        {
            directory.forward_chunks[index].posting_count = 0;
            ii42_accelerator_directory_read_ref(
                entry + 8,
                &directory.forward_chunks[index].forward_object
            );
        }
    }
    if (version >= UINT16_C(7))
    {
        directory.forward_term_work = calloc(
            directory.vocab_size,
            sizeof(*directory.forward_term_work)
        );
        if (directory.forward_term_work == NULL)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_NOMEM;
        }
        for (index = 0; index < directory.vocab_size; index++)
        {
            directory.forward_term_work[index] =
                ii42_accelerator_directory_read_u64(
                    bytes + header_size + entry_bytes + forward_bytes +
                        (size_t) index * sizeof(uint64_t)
                );
        }
    }
    if (version == II42_ACCELERATOR_DIRECTORY_VERSION)
    {
        directory.forward_row_data_bytes = calloc(
            directory.forward_chunk_count,
            sizeof(*directory.forward_row_data_bytes)
        );
        directory.forward_transpose_fixed_bytes = calloc(
            directory.forward_chunk_count,
            sizeof(*directory.forward_transpose_fixed_bytes)
        );
        directory.forward_row_offsets = calloc(
            (size_t)
                ii42_accelerator_directory_expected_forward_row_offsets(
                    directory.document_count,
                    directory.forward_chunk_count
                ),
            sizeof(*directory.forward_row_offsets)
        );
        directory.forward_term_bytes = calloc(
            directory.vocab_size,
            sizeof(*directory.forward_term_bytes)
        );
        directory.forward_bound_term_bytes = calloc(
            directory.vocab_size,
            sizeof(*directory.forward_bound_term_bytes)
        );
        if (directory.forward_row_data_bytes == NULL ||
            directory.forward_transpose_fixed_bytes == NULL ||
            directory.forward_row_offsets == NULL ||
            directory.forward_term_bytes == NULL ||
            directory.forward_bound_term_bytes == NULL)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_NOMEM;
        }
        for (uint64_t offset_index = 0;
             offset_index <
                ii42_accelerator_directory_expected_forward_row_offsets(
                    directory.document_count,
                    directory.forward_chunk_count
                );
             offset_index++)
        {
            directory.forward_row_offsets[offset_index] =
                ii42_accelerator_directory_read_u32(
                    bytes + header_size + entry_bytes + forward_bytes +
                        forward_term_work_bytes +
                        forward_chunk_cost_bytes +
                        (size_t) offset_index * sizeof(uint32_t)
                );
        }
        for (index = 0; index < directory.forward_chunk_count; index++)
        {
            size_t offset = header_size + entry_bytes + forward_bytes +
                forward_term_work_bytes +
                (size_t) index * 2U * sizeof(uint64_t);

            directory.forward_row_data_bytes[index] =
                ii42_accelerator_directory_read_u64(bytes + offset);
            directory.forward_transpose_fixed_bytes[index] =
                ii42_accelerator_directory_read_u64(
                    bytes + offset + sizeof(uint64_t)
                );
        }
        for (index = 0; index < directory.vocab_size; index++)
        {
            size_t offset = header_size + entry_bytes + forward_bytes +
                forward_term_work_bytes + forward_chunk_cost_bytes +
                forward_row_offset_bytes +
                (size_t) index * sizeof(uint64_t);

            directory.forward_term_bytes[index] =
                ii42_accelerator_directory_read_u64(
                    bytes + offset
                );
            directory.forward_bound_term_bytes[index] =
                ii42_accelerator_directory_read_u64(
                    bytes + offset + forward_term_bytes
                );
        }
    }
    if (directory.forward_bound_shard_count > 0)
    {
        directory.forward_bound_shards = calloc(
            directory.forward_bound_shard_count,
            sizeof(*directory.forward_bound_shards)
        );
        if (directory.forward_bound_shards == NULL)
        {
            ii42_semantic_accelerator_directory_free(&directory);
            return II42_ERR_NOMEM;
        }
        for (index = 0;
             index < directory.forward_bound_shard_count;
             index++)
        {
            ii42_accelerator_directory_read_ref(
                bytes + header_size + entry_bytes + forward_bytes +
                    forward_term_work_bytes +
                    forward_chunk_cost_bytes + forward_row_offset_bytes +
                    forward_term_bytes +
                    forward_bound_term_bytes +
                    (size_t) index *
                        II42_ACCELERATOR_FORWARD_BOUND_REF_SIZE,
                &directory.forward_bound_shards[index]
            );
        }
    }
    status = ii42_semantic_accelerator_directory_validate(&directory);
    if (status != II42_OK)
    {
        ii42_semantic_accelerator_directory_free(&directory);
        return status;
    }
    ii42_semantic_accelerator_directory_free(directory_out);
    *directory_out = directory;
    return II42_OK;
}

ii42_status
ii42_semantic_accelerator_directory_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
)
{
    return ii42_semantic_accelerator_directory_deserialize_internal(
        bytes,
        size,
        directory_out,
        false
    );
}

ii42_status
ii42_semantic_accelerator_directory_deserialize_retired(
    const uint8_t *bytes,
    size_t size,
    ii42_semantic_accelerator_directory *directory_out
)
{
    return ii42_semantic_accelerator_directory_deserialize_internal(
        bytes,
        size,
        directory_out,
        true
    );
}
