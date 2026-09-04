#include "ii42_term_cow.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define II42_TERM_COW_MAGIC UINT32_C(0x574F4332)
#define II42_TERM_COW_VERSION UINT16_C(8)
#define II42_TERM_COW_HEADER_SIZE 64U
#define II42_TERM_COW_CHECKSUM_OFFSET 56U
#define II42_TERM_COW_CHILD_SIZE 56U
#define II42_TERM_COW_RECORD_SIZE 248U
#define II42_TERM_COW_EXTENT_SIZE 32U
#define II42_TERM_COW_OBJECT_REF_SIZE 48U

typedef struct ii42_term_cow_build_ref
{
    uint64_t key;
    uint32_t max_extent_count;
    ii42_term_cow_ref ref;
} ii42_term_cow_build_ref;

static void
ii42_term_cow_write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t) (value & UINT16_C(0x00ff));
    dst[1] = (uint8_t) ((value >> 8) & UINT16_C(0x00ff));
}

static uint16_t
ii42_term_cow_read_u16(const uint8_t *src)
{
    return (uint16_t) src[0] |
        (uint16_t) ((uint16_t) src[1] << 8);
}

static void
ii42_term_cow_write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t) (value & UINT32_C(0xff));
    dst[1] = (uint8_t) ((value >> 8) & UINT32_C(0xff));
    dst[2] = (uint8_t) ((value >> 16) & UINT32_C(0xff));
    dst[3] = (uint8_t) ((value >> 24) & UINT32_C(0xff));
}

static uint32_t
ii42_term_cow_read_u32(const uint8_t *src)
{
    return (uint32_t) src[0] |
        ((uint32_t) src[1] << 8) |
        ((uint32_t) src[2] << 16) |
        ((uint32_t) src[3] << 24);
}

static void
ii42_term_cow_write_u64(uint8_t *dst, uint64_t value)
{
    for (uint32_t byte_index = 0; byte_index < 8; byte_index++)
    {
        dst[byte_index] = (uint8_t) (
            (value >> (byte_index * 8)) & UINT64_C(0xff)
        );
    }
}

static uint64_t
ii42_term_cow_read_u64(const uint8_t *src)
{
    uint64_t value = 0;

    for (uint32_t byte_index = 0; byte_index < 8; byte_index++)
    {
        value |= (uint64_t) src[byte_index] << (byte_index * 8);
    }
    return value;
}

static uint64_t
ii42_term_cow_checksum(const uint8_t *bytes, size_t size)
{
    uint64_t checksum = UINT64_C(14695981039346656037);

    for (size_t index = 0; index < size; index++)
    {
        uint8_t value =
            index >= II42_TERM_COW_CHECKSUM_OFFSET &&
            index < II42_TERM_COW_CHECKSUM_OFFSET + sizeof(uint64_t)
                ? 0
                : bytes[index];

        checksum ^= value;
        checksum *= UINT64_C(1099511628211);
    }
    return checksum;
}

static bool
ii42_term_cow_ref_valid(const ii42_term_cow_ref *ref)
{
    bool has_locator;

    if (ref == NULL ||
        (ref->kind != II42_TERM_COW_OBJECT_NODE &&
         ref->kind != II42_TERM_COW_OBJECT_LEAF) ||
        ref->reserved != 0 ||
        ref->object_id == 0 ||
        ref->checksum == 0 ||
        ref->blob_checksum == 0)
    {
        return false;
    }
    has_locator =
        ref->start_block != 0 ||
        ref->page_count != 0 ||
        ref->owner_manifest_id != 0;
    if (!has_locator)
    {
        return ref->start_block == 0 &&
            ref->page_count == 0 &&
            ref->owner_manifest_id == 0;
    }
    return ref->start_block != UINT32_MAX &&
        ref->page_count != 0 &&
        ref->owner_manifest_id != 0 &&
        ref->object_bytes != 0 &&
        (uint64_t) ref->start_block + ref->page_count <= UINT32_MAX;
}

static bool
ii42_term_cow_ref_is_unbound(const ii42_term_cow_ref *ref)
{
    return ref != NULL &&
        ref->start_block == 0 &&
        ref->page_count == 0 &&
        ref->owner_manifest_id == 0;
}

static bool
ii42_term_cow_refs_equal(
    const ii42_term_cow_ref *left,
    const ii42_term_cow_ref *right
)
{
    return left != NULL && right != NULL &&
        left->kind == right->kind &&
        left->start_block == right->start_block &&
        left->page_count == right->page_count &&
        left->reserved == right->reserved &&
        left->object_id == right->object_id &&
        left->owner_manifest_id == right->owner_manifest_id &&
        left->object_bytes == right->object_bytes &&
        left->checksum == right->checksum &&
        left->blob_checksum == right->blob_checksum;
}

static bool
ii42_term_cow_object_ref_absent(
    const ii42_segment_object_ref *ref
)
{
    static const ii42_segment_object_ref zero_ref = {0};

    return ref != NULL &&
        memcmp(ref, &zero_ref, sizeof(*ref)) == 0;
}

static bool
ii42_term_cow_object_ref_valid(
    const ii42_segment_object_ref *ref,
    ii42_segment_object_kind expected_kind
)
{
    return ref != NULL &&
        ref->object_kind == expected_kind &&
        ii42_segment_object_ref_validate(ref, UINT32_MAX) == II42_OK;
}

static bool
ii42_term_cow_extent_kind_valid(ii42_posting_extent_kind kind)
{
    return kind == II42_POSTING_EXTENT_LEXICAL_NEUTRAL ||
        kind == II42_POSTING_EXTENT_SEMANTIC_IMPACT ||
        kind == II42_POSTING_EXTENT_LEXICAL_IMPACT;
}

static ii42_status
ii42_term_cow_record_validate(const ii42_term_cow_record *record)
{
    bool has_neutral;
    bool has_neutral_minor;
    bool has_impact;

    if (record == NULL ||
        (record->flags &
         ~(II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD |
           II42_TERM_COW_RECORD_FLAG_IMPACT_FOLD |
           II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD)) != 0 ||
        record->extent_count >
            II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM)
    {
        return II42_ERR_FORMAT;
    }
    has_neutral =
        (record->flags & II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD) != 0;
    has_impact =
        (record->flags & II42_TERM_COW_RECORD_FLAG_IMPACT_FOLD) != 0;
    has_neutral_minor =
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0;
    if (has_neutral !=
            ii42_term_cow_object_ref_valid(
                &record->neutral_fold,
                II42_SEGMENT_OBJECT_NEUTRAL_FOLD
            ) ||
        (!has_neutral &&
         (!ii42_term_cow_object_ref_absent(&record->neutral_fold) ||
          record->neutral_fold_coverage != 0)) ||
        has_neutral_minor !=
            ii42_term_cow_object_ref_valid(
                &record->neutral_minor_fold,
                II42_SEGMENT_OBJECT_NEUTRAL_FOLD
            ) ||
        (!has_neutral_minor &&
         (!ii42_term_cow_object_ref_absent(
             &record->neutral_minor_fold
          ) ||
          record->neutral_minor_fold_coverage != 0)) ||
        (has_neutral_minor &&
         (!has_neutral ||
          record->neutral_minor_fold_coverage <=
            record->neutral_fold_coverage ||
          memcmp(
              &record->neutral_minor_fold,
              &record->neutral_fold,
              sizeof(record->neutral_fold)
          ) == 0)) ||
        has_impact !=
            ii42_term_cow_object_ref_valid(
                &record->impact_fold,
                II42_SEGMENT_OBJECT_IMPACT_FOLD
            ) ||
        (!has_impact &&
         (!ii42_term_cow_object_ref_absent(&record->impact_fold) ||
          record->impact_fold_coverage != 0 ||
          record->impact_statistics_epoch != 0)) ||
        (has_impact &&
         (!has_neutral ||
          record->impact_statistics_epoch == 0 ||
          record->impact_fold_coverage >
            (
                has_neutral_minor
                    ? record->neutral_minor_fold_coverage
                    : record->neutral_fold_coverage
            ))))
    {
        return II42_ERR_FORMAT;
    }
    if (!ii42_term_cow_object_ref_absent(
            &record->lexical_catalog) &&
        !ii42_term_cow_object_ref_valid(
            &record->lexical_catalog,
            II42_SEGMENT_OBJECT_LEXICAL_CATALOG
        ))
    {
        return II42_ERR_FORMAT;
    }

    for (uint32_t extent_index = 0;
         extent_index < record->extent_count;
         extent_index++)
    {
        const ii42_stable_term_extent *extent =
            &record->extents[extent_index];

        if (extent->segment_id == 0 ||
            !ii42_term_cow_extent_kind_valid(extent->kind) ||
            extent->posting_count == 0)
        {
            return II42_ERR_FORMAT;
        }
        if (extent_index > 0)
        {
            const ii42_stable_term_extent *previous =
                &record->extents[extent_index - 1];

            if (previous->segment_id > extent->segment_id ||
                (previous->segment_id == extent->segment_id &&
                 previous->posting_offset >= extent->posting_offset))
            {
                return II42_ERR_FORMAT;
            }
        }
    }
    for (uint32_t extent_index = record->extent_count;
         extent_index < II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM;
         extent_index++)
    {
        static const ii42_stable_term_extent zero_extent = {0};

        if (memcmp(
                &record->extents[extent_index],
                &zero_extent,
                sizeof(zero_extent)) != 0)
        {
            return II42_ERR_FORMAT;
        }
    }
    return II42_OK;
}

static ii42_status
ii42_term_cow_object_validate(const ii42_term_cow_object *object)
{
    if (object == NULL || !ii42_term_cow_ref_valid(&object->ref))
    {
        return II42_ERR_FORMAT;
    }
    if (object->ref.kind == II42_TERM_COW_OBJECT_NODE)
    {
        const ii42_term_cow_node *node = &object->value.node;

        if (node->level >= II42_TERM_COW_RADIX_LEVELS ||
            node->reserved != 0 ||
            node->child_count > II42_TERM_COW_RADIX_FANOUT)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t child_index = 0;
             child_index < node->child_count;
             child_index++)
        {
            const ii42_term_cow_child *child =
                &node->children[child_index];
            ii42_term_cow_object_kind expected_kind =
                node->level == 0
                    ? II42_TERM_COW_OBJECT_LEAF
                    : II42_TERM_COW_OBJECT_NODE;

            if (child->slot >= II42_TERM_COW_RADIX_FANOUT ||
                child->reserved != 0 ||
                child->max_extent_count >
                    II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM ||
                child->ref.kind != expected_kind ||
                !ii42_term_cow_ref_valid(&child->ref) ||
                (!ii42_term_cow_ref_is_unbound(&object->ref) &&
                 (child->ref.owner_manifest_id >
                      object->ref.owner_manifest_id ||
                  (child->ref.owner_manifest_id ==
                       object->ref.owner_manifest_id &&
                   child->ref.object_id >= object->ref.object_id))) ||
                (ii42_term_cow_ref_is_unbound(&object->ref) &&
                 ii42_term_cow_ref_is_unbound(&child->ref) &&
                 child->ref.object_id >= object->ref.object_id) ||
                (child_index > 0 &&
                 node->children[child_index - 1].slot >= child->slot))
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t child_index = node->child_count;
             child_index < II42_TERM_COW_RADIX_FANOUT;
             child_index++)
        {
            static const ii42_term_cow_child zero_child = {0};

            if (memcmp(
                    &node->children[child_index],
                    &zero_child,
                    sizeof(zero_child)) != 0)
            {
                return II42_ERR_FORMAT;
            }
        }
        return II42_OK;
    }
    else
    {
        const ii42_term_cow_leaf *leaf = &object->value.leaf;

        if (leaf->base_term_id % II42_TERM_COW_LEAF_TERMS != 0 ||
            leaf->record_count == 0 ||
            leaf->record_count > II42_TERM_COW_LEAF_TERMS)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t record_index = 0;
             record_index < leaf->record_count;
             record_index++)
        {
            if (leaf->records[record_index].term_id !=
                    leaf->base_term_id + record_index ||
                ii42_term_cow_record_validate(
                    &leaf->records[record_index]) != II42_OK)
            {
                return II42_ERR_FORMAT;
            }
        }
        for (uint32_t record_index = leaf->record_count;
             record_index < II42_TERM_COW_LEAF_TERMS;
             record_index++)
        {
            static const ii42_term_cow_record zero_record = {0};

            if (memcmp(
                    &leaf->records[record_index],
                    &zero_record,
                    sizeof(zero_record)) != 0)
            {
                return II42_ERR_FORMAT;
            }
        }
        return II42_OK;
    }
}

static uint32_t
ii42_term_cow_leaf_stored_extent_count(const ii42_term_cow_leaf *leaf)
{
    uint32_t stored_extent_count = 1;

    for (uint32_t record_index = 0;
         record_index < leaf->record_count;
         record_index++)
    {
        if (leaf->records[record_index].extent_count > stored_extent_count)
        {
            stored_extent_count =
                leaf->records[record_index].extent_count;
        }
    }
    return stored_extent_count;
}

static size_t
ii42_term_cow_object_size(const ii42_term_cow_object *object)
{
    uint32_t stored_extent_count;

    if (object->ref.kind == II42_TERM_COW_OBJECT_NODE)
    {
        return II42_TERM_COW_HEADER_SIZE +
            (size_t) object->value.node.child_count *
                II42_TERM_COW_CHILD_SIZE;
    }
    stored_extent_count = ii42_term_cow_leaf_stored_extent_count(
        &object->value.leaf
    );
    return II42_TERM_COW_HEADER_SIZE +
        (size_t) object->value.leaf.record_count *
            II42_TERM_COW_RECORD_SIZE +
        (size_t) object->value.leaf.record_count *
            stored_extent_count *
            II42_TERM_COW_EXTENT_SIZE;
}

static void
ii42_term_cow_encode_object_ref(
    uint8_t *bytes,
    const ii42_segment_object_ref *ref
)
{
    ii42_term_cow_write_u32(bytes + 0, (uint32_t) ref->object_kind);
    ii42_term_cow_write_u32(bytes + 4, 0);
    ii42_term_cow_write_u32(bytes + 8, ref->start_block);
    ii42_term_cow_write_u32(bytes + 12, ref->page_count);
    ii42_term_cow_write_u64(bytes + 16, ref->object_id);
    ii42_term_cow_write_u64(bytes + 24, ref->owner_manifest_id);
    ii42_term_cow_write_u64(bytes + 32, ref->object_bytes);
    ii42_term_cow_write_u64(bytes + 40, ref->object_checksum);
}

static ii42_status
ii42_term_cow_decode_object_ref(
    const uint8_t *bytes,
    ii42_segment_object_ref *ref
)
{
    if (ii42_term_cow_read_u32(bytes + 4) != 0)
    {
        return II42_ERR_FORMAT;
    }
    memset(ref, 0, sizeof(*ref));
    ref->object_kind =
        (ii42_segment_object_kind) ii42_term_cow_read_u32(bytes + 0);
    ref->start_block = ii42_term_cow_read_u32(bytes + 8);
    ref->page_count = ii42_term_cow_read_u32(bytes + 12);
    ref->object_id = ii42_term_cow_read_u64(bytes + 16);
    ref->owner_manifest_id = ii42_term_cow_read_u64(bytes + 24);
    ref->object_bytes = ii42_term_cow_read_u64(bytes + 32);
    ref->object_checksum = ii42_term_cow_read_u64(bytes + 40);
    return II42_OK;
}

static void
ii42_term_cow_encode_child(
    uint8_t *bytes,
    const ii42_term_cow_child *child
)
{
    ii42_term_cow_write_u16(bytes + 0, child->slot);
    ii42_term_cow_write_u16(bytes + 2, (uint16_t) child->ref.kind);
    ii42_term_cow_write_u32(bytes + 4, child->ref.start_block);
    ii42_term_cow_write_u32(bytes + 8, child->ref.page_count);
    ii42_term_cow_write_u32(bytes + 12, child->max_extent_count);
    ii42_term_cow_write_u64(bytes + 16, child->ref.object_id);
    ii42_term_cow_write_u64(bytes + 24, child->ref.owner_manifest_id);
    ii42_term_cow_write_u64(bytes + 32, child->ref.object_bytes);
    ii42_term_cow_write_u64(bytes + 40, child->ref.checksum);
    ii42_term_cow_write_u64(bytes + 48, child->ref.blob_checksum);
}

static ii42_status
ii42_term_cow_object_encode(
    const ii42_term_cow_object *object,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    uint8_t *bytes;
    size_t size;

    if (bytes_out == NULL || size_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    *bytes_out = NULL;
    *size_out = 0;
    if (ii42_term_cow_object_validate(object) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    size = ii42_term_cow_object_size(object);
    bytes = calloc(size, 1);
    if (bytes == NULL)
    {
        return II42_ERR_NOMEM;
    }
    ii42_term_cow_write_u32(bytes + 0, II42_TERM_COW_MAGIC);
    ii42_term_cow_write_u16(bytes + 4, II42_TERM_COW_VERSION);
    ii42_term_cow_write_u16(bytes + 6, II42_TERM_COW_HEADER_SIZE);
    ii42_term_cow_write_u16(bytes + 8, (uint16_t) object->ref.kind);
    ii42_term_cow_write_u16(
        bytes + 10,
        object->ref.kind == II42_TERM_COW_OBJECT_NODE
            ? object->value.node.level
            : 0
    );
    ii42_term_cow_write_u32(
        bytes + 12,
        object->ref.kind == II42_TERM_COW_OBJECT_NODE
            ? object->value.node.child_count
            : object->value.leaf.record_count
    );
    ii42_term_cow_write_u64(bytes + 16, object->ref.object_id);
    ii42_term_cow_write_u64(
        bytes + 24,
        object->ref.kind == II42_TERM_COW_OBJECT_NODE
            ? object->value.node.prefix
            : object->value.leaf.base_term_id
    );
    ii42_term_cow_write_u64(bytes + 32, size);
    ii42_term_cow_write_u64(bytes + 56, object->ref.checksum);

    if (object->ref.kind == II42_TERM_COW_OBJECT_NODE)
    {
        for (uint32_t child_index = 0;
             child_index < object->value.node.child_count;
             child_index++)
        {
            uint8_t *child_bytes =
                bytes + II42_TERM_COW_HEADER_SIZE +
                (size_t) child_index * II42_TERM_COW_CHILD_SIZE;
            const ii42_term_cow_child *child =
                &object->value.node.children[child_index];

            ii42_term_cow_encode_child(child_bytes, child);
        }
    }
    else
    {
        size_t records_offset = II42_TERM_COW_HEADER_SIZE;
        uint32_t stored_extent_count =
            ii42_term_cow_leaf_stored_extent_count(
                &object->value.leaf
            );
        size_t extents_offset =
            records_offset +
            (size_t) object->value.leaf.record_count *
                II42_TERM_COW_RECORD_SIZE;

        for (uint32_t record_index = 0;
             record_index < object->value.leaf.record_count;
             record_index++)
        {
            const ii42_term_cow_record *record =
                &object->value.leaf.records[record_index];
            uint8_t *record_bytes =
                bytes + records_offset +
                (size_t) record_index * II42_TERM_COW_RECORD_SIZE;

            ii42_term_cow_write_u32(record_bytes + 0, record->term_id);
            ii42_term_cow_write_u32(
                record_bytes + 4,
                record->raw_document_frequency
            );
            ii42_term_cow_write_u32(record_bytes + 8, record->flags);
            ii42_term_cow_write_u32(
                record_bytes + 12,
                record->extent_count
            );
            ii42_term_cow_write_u64(
                record_bytes + 16,
                record->neutral_fold_coverage
            );
            ii42_term_cow_write_u64(
                record_bytes + 24,
                record->neutral_minor_fold_coverage
            );
            ii42_term_cow_write_u64(
                record_bytes + 32,
                record->impact_fold_coverage
            );
            ii42_term_cow_write_u64(
                record_bytes + 40,
                record->impact_statistics_epoch
            );
            ii42_term_cow_encode_object_ref(
                record_bytes + 48,
                &record->neutral_fold
            );
            ii42_term_cow_encode_object_ref(
                record_bytes + 48 + II42_TERM_COW_OBJECT_REF_SIZE,
                &record->neutral_minor_fold
            );
            ii42_term_cow_encode_object_ref(
                record_bytes + 48 +
                    2 * II42_TERM_COW_OBJECT_REF_SIZE,
                &record->impact_fold
            );
            ii42_term_cow_encode_object_ref(
                record_bytes + 48 +
                    3 * II42_TERM_COW_OBJECT_REF_SIZE,
                &record->lexical_catalog
            );

            for (uint32_t extent_index = 0;
                 extent_index < record->extent_count;
                 extent_index++)
            {
                const ii42_stable_term_extent *extent =
                    &record->extents[extent_index];
                uint8_t *extent_bytes =
                    bytes + extents_offset +
                    ((size_t) record_index *
                         stored_extent_count +
                     extent_index) * II42_TERM_COW_EXTENT_SIZE;

                ii42_term_cow_write_u64(
                    extent_bytes + 0,
                    extent->segment_id
                );
                ii42_term_cow_write_u32(
                    extent_bytes + 8,
                    (uint32_t) extent->kind
                );
                ii42_term_cow_write_u64(
                    extent_bytes + 16,
                    extent->posting_offset
                );
                ii42_term_cow_write_u64(
                    extent_bytes + 24,
                    extent->posting_count
                );
            }
        }
    }

    *bytes_out = bytes;
    *size_out = size;
    return II42_OK;
}

static ii42_status
ii42_term_cow_object_refresh_integrity(
    ii42_term_cow_object *object,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    uint8_t *bytes = NULL;
    size_t size = 0;
    ii42_status status;

    if (object == NULL || !ii42_term_cow_ref_is_unbound(&object->ref))
    {
        return II42_ERR_INVALID;
    }
    if (bytes_out != NULL)
    {
        *bytes_out = NULL;
    }
    if (size_out != NULL)
    {
        *size_out = 0;
    }

    object->ref.object_bytes = 0;
    object->ref.checksum = UINT64_C(1);
    object->ref.blob_checksum = UINT64_C(1);
    status = ii42_term_cow_object_encode(object, &bytes, &size);
    if (status != II42_OK)
    {
        return status;
    }
    object->ref.checksum = ii42_term_cow_checksum(bytes, size);
    free(bytes);
    bytes = NULL;
    if (object->ref.checksum == 0)
    {
        return II42_ERR_FORMAT;
    }

    status = ii42_term_cow_object_encode(object, &bytes, &size);
    if (status != II42_OK)
    {
        return status;
    }
    if (ii42_term_cow_checksum(bytes, size) != object->ref.checksum)
    {
        free(bytes);
        return II42_ERR_FORMAT;
    }
    object->ref.object_bytes = size;
    object->ref.blob_checksum =
        ii42_segment_blob_checksum(bytes, size);
    if (object->ref.blob_checksum == 0)
    {
        free(bytes);
        return II42_ERR_FORMAT;
    }

    if (bytes_out != NULL)
    {
        *bytes_out = bytes;
    }
    else
    {
        free(bytes);
    }
    if (size_out != NULL)
    {
        *size_out = size;
    }
    return II42_OK;
}

static const ii42_term_cow_object *
ii42_term_cow_tree_find_object(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *ref
)
{
    const ii42_term_cow_object *object;

    if (tree == NULL || !ii42_term_cow_ref_valid(ref) ||
        ref->object_id > tree->object_count)
    {
        return NULL;
    }
    object = &tree->objects[ref->object_id - 1];
    if (object->ref.kind != ref->kind ||
        object->ref.object_id != ref->object_id ||
        object->ref.start_block != ref->start_block ||
        object->ref.page_count != ref->page_count ||
        object->ref.owner_manifest_id != ref->owner_manifest_id ||
        object->ref.object_bytes != ref->object_bytes ||
        object->ref.checksum != ref->checksum ||
        object->ref.blob_checksum != ref->blob_checksum)
    {
        return NULL;
    }
    return object;
}

static ii42_status
ii42_term_cow_tree_append_object(
    ii42_term_cow_tree *tree,
    ii42_term_cow_object *object,
    ii42_term_cow_ref *ref_out,
    size_t *written_bytes_out
)
{
    uint8_t *bytes = NULL;
    size_t size = 0;
    ii42_term_cow_object *resized;
    ii42_status status;

    if (tree == NULL || object == NULL || ref_out == NULL ||
        tree->next_object_id == 0 ||
        tree->next_object_id == UINT64_MAX)
    {
        return II42_ERR_INVALID;
    }
    object->ref.object_id = tree->next_object_id;
    status = ii42_term_cow_object_refresh_integrity(
        object,
        &bytes,
        &size
    );
    if (status != II42_OK)
    {
        return status;
    }
    free(bytes);

    if (tree->object_count == tree->object_capacity)
    {
        size_t next_capacity = tree->object_capacity == 0
            ? 16
            : tree->object_capacity * 2;

        if (next_capacity < tree->object_capacity ||
            next_capacity >
                SIZE_MAX / sizeof(*tree->objects))
        {
            return II42_ERR_RANGE;
        }
        resized = realloc(
            tree->objects,
            next_capacity * sizeof(*tree->objects)
        );
        if (resized == NULL)
        {
            return II42_ERR_NOMEM;
        }
        tree->objects = resized;
        tree->object_capacity = next_capacity;
    }
    tree->objects[tree->object_count++] = *object;
    tree->next_object_id++;
    *ref_out = object->ref;
    if (written_bytes_out != NULL)
    {
        *written_bytes_out = size;
    }
    return II42_OK;
}

static uint16_t
ii42_term_cow_path_slot(uint64_t leaf_index, uint16_t level)
{
    return (uint16_t) (
        (leaf_index >> ((uint32_t) level * II42_TERM_COW_RADIX_BITS)) &
        (II42_TERM_COW_RADIX_FANOUT - 1)
    );
}

static uint64_t
ii42_term_cow_path_prefix(uint64_t leaf_index, uint16_t level)
{
    return leaf_index >>
        ((uint32_t) (level + 1) * II42_TERM_COW_RADIX_BITS);
}

static uint32_t
ii42_term_cow_leaf_max_extent_count(const ii42_term_cow_leaf *leaf)
{
    uint32_t max_extent_count = 0;

    for (uint32_t record_index = 0;
         record_index < leaf->record_count;
         record_index++)
    {
        if (leaf->records[record_index].extent_count > max_extent_count)
        {
            max_extent_count =
                leaf->records[record_index].extent_count;
        }
    }
    return max_extent_count;
}

static uint32_t
ii42_term_cow_node_max_extent_count(const ii42_term_cow_node *node)
{
    uint32_t max_extent_count = 0;

    for (uint32_t child_index = 0;
         child_index < node->child_count;
         child_index++)
    {
        if (node->children[child_index].max_extent_count >
            max_extent_count)
        {
            max_extent_count =
                node->children[child_index].max_extent_count;
        }
    }
    return max_extent_count;
}

static const ii42_term_cow_child *
ii42_term_cow_node_child(
    const ii42_term_cow_node *node,
    uint16_t slot
)
{
    uint32_t left = 0;
    uint32_t right = node->child_count;

    while (left < right)
    {
        uint32_t middle = left + (right - left) / 2;

        if (node->children[middle].slot < slot)
        {
            left = middle + 1;
        }
        else
        {
            right = middle;
        }
    }
    if (left >= node->child_count ||
        node->children[left].slot != slot)
    {
        return NULL;
    }
    return &node->children[left];
}

static ii42_status
ii42_term_cow_node_set_child(
    ii42_term_cow_node *node,
    uint16_t slot,
    uint32_t max_extent_count,
    const ii42_term_cow_ref *ref
)
{
    uint32_t position = 0;

    if (node == NULL || !ii42_term_cow_ref_valid(ref) ||
        slot >= II42_TERM_COW_RADIX_FANOUT ||
        max_extent_count >
            II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM)
    {
        return II42_ERR_INVALID;
    }
    while (position < node->child_count &&
           node->children[position].slot < slot)
    {
        position++;
    }
    if (position < node->child_count &&
        node->children[position].slot == slot)
    {
        node->children[position].max_extent_count =
            max_extent_count;
        node->children[position].ref = *ref;
        return II42_OK;
    }
    if (node->child_count >= II42_TERM_COW_RADIX_FANOUT)
    {
        return II42_ERR_RANGE;
    }
    memmove(
        &node->children[position + 1],
        &node->children[position],
        (size_t) (node->child_count - position) *
            sizeof(*node->children)
    );
    memset(
        &node->children[position],
        0,
        sizeof(*node->children)
    );
    node->children[position].slot = slot;
    node->children[position].max_extent_count = max_extent_count;
    node->children[position].ref = *ref;
    node->child_count++;
    return II42_OK;
}

void
ii42_term_cow_tree_init(ii42_term_cow_tree *tree)
{
    if (tree != NULL)
    {
        memset(tree, 0, sizeof(*tree));
        tree->next_object_id = 1;
    }
}

void
ii42_term_cow_tree_free(ii42_term_cow_tree *tree)
{
    if (tree == NULL)
    {
        return;
    }
    free(tree->objects);
    free(tree->retired_ranges);
    memset(tree, 0, sizeof(*tree));
}

static ii42_status
ii42_term_cow_tree_add_retired_ref(
    ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *ref
)
{
    ii42_block_range *ranges;
    size_t next_capacity;

    if (tree == NULL || ref == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (ii42_term_cow_ref_is_unbound(ref))
    {
        return II42_OK;
    }
    if (!ii42_term_cow_ref_valid(ref))
    {
        return II42_ERR_FORMAT;
    }
    for (size_t index = 0; index < tree->retired_range_count; index++)
    {
        if (tree->retired_ranges[index].start_block == ref->start_block &&
            tree->retired_ranges[index].block_count == ref->page_count)
        {
            return II42_OK;
        }
    }
    if (tree->retired_range_count == tree->retired_range_capacity)
    {
        next_capacity = tree->retired_range_capacity == 0
            ? 8
            : tree->retired_range_capacity * 2;
        if (next_capacity < tree->retired_range_capacity ||
            next_capacity > SIZE_MAX / sizeof(*tree->retired_ranges))
        {
            return II42_ERR_RANGE;
        }
        ranges = realloc(
            tree->retired_ranges,
            next_capacity * sizeof(*tree->retired_ranges)
        );
        if (ranges == NULL)
        {
            return II42_ERR_NOMEM;
        }
        tree->retired_ranges = ranges;
        tree->retired_range_capacity = next_capacity;
    }
    tree->retired_ranges[tree->retired_range_count].start_block =
        ref->start_block;
    tree->retired_ranges[tree->retired_range_count].block_count =
        ref->page_count;
    tree->retired_range_count++;
    return II42_OK;
}

static ii42_status
ii42_term_cow_leaf_from_flat(
    uint32_t base_term_id,
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *fold_refs,
    ii42_term_cow_leaf *leaf_out,
    bool *nonempty_out
)
{
    ii42_term_cow_leaf leaf;
    bool nonempty = false;

    memset(&leaf, 0, sizeof(leaf));
    leaf.base_term_id = base_term_id;
    leaf.record_count = manifest->vocab_size - base_term_id;
    if (leaf.record_count > II42_TERM_COW_LEAF_TERMS)
    {
        leaf.record_count = II42_TERM_COW_LEAF_TERMS;
    }
    for (uint32_t record_index = 0;
         record_index < leaf.record_count;
         record_index++)
    {
        uint32_t term_id = base_term_id + record_index;
        ii42_term_cow_record *record = &leaf.records[record_index];

        record->term_id = term_id;
        record->raw_document_frequency =
            manifest->doc_frequencies[term_id];
        if (fold_refs != NULL)
        {
            const ii42_segment_object_ref *fold_ref =
                &fold_refs[term_id];

            if (record->raw_document_frequency != 0)
            {
                if (!ii42_term_cow_object_ref_valid(
                        fold_ref,
                        II42_SEGMENT_OBJECT_NEUTRAL_FOLD) ||
                    fold_ref->owner_manifest_id !=
                        manifest->manifest_id)
                {
                    return II42_ERR_FORMAT;
                }
                record->flags |=
                    II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD;
                record->neutral_fold_coverage =
                    manifest->max_sequence;
                record->neutral_fold = *fold_ref;
            }
            else if (!ii42_term_cow_object_ref_absent(fold_ref))
            {
                return II42_ERR_FORMAT;
            }
        }
        else
        {
            uint64_t start = directory->term_offsets[term_id];
            uint64_t end = directory->term_offsets[term_id + 1];

            record->extent_count = (uint32_t) (end - start);
            for (uint32_t extent_index = 0;
                 extent_index < record->extent_count;
                 extent_index++)
            {
                const ii42_term_extent_descriptor *source =
                    &directory->extents[start + extent_index];
                ii42_stable_term_extent *target =
                    &record->extents[extent_index];

                target->segment_id =
                    manifest->segments[source->segment_index].segment_id;
                target->kind = source->kind;
                target->posting_offset = source->posting_offset;
                target->posting_count = source->posting_count;
            }
        }
        nonempty = nonempty ||
            record->raw_document_frequency != 0 ||
            record->extent_count != 0;
        if (ii42_term_cow_record_validate(record) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
    }
    *leaf_out = leaf;
    *nonempty_out = nonempty;
    return II42_OK;
}

static ii42_status
ii42_term_cow_build_parent_level(
    ii42_term_cow_tree *tree,
    const ii42_term_cow_build_ref *children,
    size_t child_count,
    uint16_t level,
    ii42_term_cow_build_ref **parents_out,
    size_t *parent_count_out
)
{
    ii42_term_cow_build_ref *parents;
    size_t parent_count = 0;
    size_t child_index = 0;

    if (tree == NULL || parents_out == NULL ||
        parent_count_out == NULL || level >= II42_TERM_COW_RADIX_LEVELS ||
        (child_count > 0 && children == NULL))
    {
        return II42_ERR_INVALID;
    }
    parents = child_count > 0
        ? calloc(child_count, sizeof(*parents))
        : NULL;
    if (child_count > 0 && parents == NULL)
    {
        return II42_ERR_NOMEM;
    }
    while (child_index < child_count)
    {
        ii42_term_cow_object object;
        uint64_t parent_key =
            children[child_index].key >> II42_TERM_COW_RADIX_BITS;
        ii42_status status;

        memset(&object, 0, sizeof(object));
        object.ref.kind = II42_TERM_COW_OBJECT_NODE;
        object.value.node.level = level;
        object.value.node.prefix = parent_key;
        while (child_index < child_count &&
               (children[child_index].key >>
                II42_TERM_COW_RADIX_BITS) == parent_key)
        {
            uint16_t slot = (uint16_t) (
                children[child_index].key &
                (II42_TERM_COW_RADIX_FANOUT - 1)
            );

            status = ii42_term_cow_node_set_child(
                &object.value.node,
                slot,
                children[child_index].max_extent_count,
                &children[child_index].ref
            );
            if (status != II42_OK)
            {
                free(parents);
                return status;
            }
            child_index++;
        }
        status = ii42_term_cow_tree_append_object(
            tree,
            &object,
            &parents[parent_count].ref,
            NULL
        );
        if (status != II42_OK)
        {
            free(parents);
            return status;
        }
        parents[parent_count].key = parent_key;
        parents[parent_count].max_extent_count =
            ii42_term_cow_node_max_extent_count(
                &object.value.node
            );
        parent_count++;
    }
    *parents_out = parents;
    *parent_count_out = parent_count;
    return II42_OK;
}

static ii42_status
ii42_term_cow_tree_build_internal(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *lexical_catalog,
    const ii42_segment_object_ref *fold_refs,
    ii42_term_cow_tree *tree_out
)
{
    ii42_term_cow_tree tree;
    ii42_term_cow_build_ref *current = NULL;
    size_t current_count = 0;
    size_t current_capacity = 0;
    ii42_status status;

    if (manifest == NULL || tree_out == NULL ||
        ((directory == NULL) == (fold_refs == NULL)))
    {
        return II42_ERR_INVALID;
    }
    if (directory != NULL)
    {
        status = ii42_term_directory_validate_logical(
            directory,
            manifest
        );
        if (status != II42_OK)
        {
            return status;
        }
    }
    else if (manifest->vocab_size > 0 &&
             manifest->doc_frequencies == NULL)
    {
        return II42_ERR_FORMAT;
    }
    if (lexical_catalog != NULL &&
        (!ii42_term_cow_object_ref_valid(
             lexical_catalog,
             II42_SEGMENT_OBJECT_LEXICAL_CATALOG
         ) ||
         lexical_catalog->owner_manifest_id > manifest->manifest_id))
    {
        return II42_ERR_FORMAT;
    }
    ii42_term_cow_tree_init(&tree);
    tree.vocab_size = manifest->vocab_size;

    for (uint32_t base_term_id = 0;
         base_term_id < manifest->vocab_size;
         base_term_id += II42_TERM_COW_LEAF_TERMS)
    {
        ii42_term_cow_object object;
        bool nonempty;

        memset(&object, 0, sizeof(object));
        object.ref.kind = II42_TERM_COW_OBJECT_LEAF;
        status = ii42_term_cow_leaf_from_flat(
            base_term_id,
            directory,
            manifest,
            fold_refs,
            &object.value.leaf,
            &nonempty
        );
        if (status != II42_OK)
        {
            goto fail;
        }
        if (lexical_catalog != NULL)
        {
            for (uint32_t record_index = 0;
                 record_index < object.value.leaf.record_count;
                 record_index++)
            {
                object.value.leaf.records[record_index].lexical_catalog =
                    *lexical_catalog;
            }
            nonempty = object.value.leaf.record_count > 0;
        }
        if (!nonempty)
        {
            continue;
        }
        if (current_count == current_capacity)
        {
            size_t next_capacity = current_capacity == 0
                ? 16
                : current_capacity * 2;
            ii42_term_cow_build_ref *resized;

            if (next_capacity < current_capacity ||
                next_capacity > SIZE_MAX / sizeof(*current))
            {
                status = II42_ERR_RANGE;
                goto fail;
            }
            resized = realloc(
                current,
                next_capacity * sizeof(*current)
            );
            if (resized == NULL)
            {
                status = II42_ERR_NOMEM;
                goto fail;
            }
            current = resized;
            current_capacity = next_capacity;
        }
        status = ii42_term_cow_tree_append_object(
            &tree,
            &object,
            &current[current_count].ref,
            NULL
        );
        if (status != II42_OK)
        {
            goto fail;
        }
        current[current_count].key =
            base_term_id / II42_TERM_COW_LEAF_TERMS;
        current[current_count].max_extent_count =
            ii42_term_cow_leaf_max_extent_count(
                &object.value.leaf
            );
        current_count++;
    }

    for (uint16_t level = 0;
         level < II42_TERM_COW_RADIX_LEVELS;
         level++)
    {
        ii42_term_cow_build_ref *parents = NULL;
        size_t parent_count = 0;

        if (current_count == 0)
        {
            ii42_term_cow_object root;

            memset(&root, 0, sizeof(root));
            root.ref.kind = II42_TERM_COW_OBJECT_NODE;
            root.value.node.level =
                II42_TERM_COW_RADIX_LEVELS - 1;
            status = ii42_term_cow_tree_append_object(
                &tree,
                &root,
                &tree.root,
                NULL
            );
            if (status != II42_OK)
            {
                goto fail;
            }
            break;
        }
        status = ii42_term_cow_build_parent_level(
            &tree,
            current,
            current_count,
            level,
            &parents,
            &parent_count
        );
        free(current);
        current = parents;
        current_count = parent_count;
        current_capacity = parent_count;
        if (status != II42_OK)
        {
            goto fail;
        }
        if (level == II42_TERM_COW_RADIX_LEVELS - 1)
        {
            if (current_count != 1 || current[0].key != 0)
            {
                status = II42_ERR_FORMAT;
                goto fail;
            }
            tree.root = current[0].ref;
        }
    }
    free(current);
    status = ii42_term_cow_tree_validate(&tree);
    if (status != II42_OK)
    {
        ii42_term_cow_tree_free(&tree);
        return status;
    }
    ii42_term_cow_tree_free(tree_out);
    *tree_out = tree;
    return II42_OK;

fail:
    free(current);
    ii42_term_cow_tree_free(&tree);
    return status;
}

ii42_status
ii42_term_cow_tree_build(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *lexical_catalog,
    ii42_term_cow_tree *tree_out
)
{
    return ii42_term_cow_tree_build_internal(
        directory,
        manifest,
        lexical_catalog,
        NULL,
        tree_out
    );
}

ii42_status
ii42_term_cow_tree_build_initial_folds(
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *lexical_catalog,
    const ii42_segment_object_ref *fold_refs,
    ii42_term_cow_tree *tree_out
)
{
    return ii42_term_cow_tree_build_internal(
        NULL,
        manifest,
        lexical_catalog,
        fold_refs,
        tree_out
    );
}

static ii42_status
ii42_term_cow_tree_validate_ref(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *ref,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint32_t *max_extent_count_out
)
{
    const ii42_term_cow_object *object =
        ii42_term_cow_tree_find_object(tree, ref);
    uint32_t max_extent_count = 0;

    if (max_extent_count_out == NULL || object == NULL ||
        ii42_term_cow_object_validate(object) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    if (expected_level < II42_TERM_COW_RADIX_LEVELS)
    {
        const ii42_term_cow_node *node;

        if (object->ref.kind != II42_TERM_COW_OBJECT_NODE)
        {
            return II42_ERR_FORMAT;
        }
        node = &object->value.node;
        if (node->level != expected_level ||
            node->prefix != expected_prefix)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t child_index = 0;
             child_index < node->child_count;
             child_index++)
        {
            const ii42_term_cow_child *child =
                &node->children[child_index];
            uint64_t child_prefix =
                (expected_prefix << II42_TERM_COW_RADIX_BITS) |
                child->slot;
            ii42_status status;
            uint32_t child_max_extent_count;

            if (expected_level == 0)
            {
                const ii42_term_cow_object *leaf_object =
                    ii42_term_cow_tree_find_object(tree, &child->ref);
                uint64_t expected_base =
                    child_prefix * II42_TERM_COW_LEAF_TERMS;

                if (leaf_object == NULL ||
                    leaf_object->ref.kind !=
                        II42_TERM_COW_OBJECT_LEAF ||
                    ii42_term_cow_object_validate(leaf_object) !=
                        II42_OK ||
                    leaf_object->value.leaf.base_term_id !=
                        expected_base ||
                    expected_base >= tree->vocab_size)
                {
                    return II42_ERR_FORMAT;
                }
                child_max_extent_count =
                    ii42_term_cow_leaf_max_extent_count(
                        &leaf_object->value.leaf
                    );
            }
            else
            {
                status = ii42_term_cow_tree_validate_ref(
                    tree,
                    &child->ref,
                    expected_level - 1,
                    child_prefix,
                    &child_max_extent_count
                );
                if (status != II42_OK)
                {
                    return status;
                }
            }
            if (child->max_extent_count != child_max_extent_count)
            {
                return II42_ERR_FORMAT;
            }
            if (child_max_extent_count > max_extent_count)
            {
                max_extent_count = child_max_extent_count;
            }
        }
    }
    *max_extent_count_out = max_extent_count;
    return II42_OK;
}

ii42_status
ii42_term_cow_tree_validate(const ii42_term_cow_tree *tree)
{
    uint32_t max_extent_count;

    if (tree == NULL || tree->next_object_id == 0 ||
        tree->next_object_id != tree->object_count + 1 ||
        tree->root.kind != II42_TERM_COW_OBJECT_NODE)
    {
        return II42_ERR_FORMAT;
    }
    for (size_t object_index = 0;
         object_index < tree->object_count;
         object_index++)
    {
        const ii42_term_cow_object *object =
            &tree->objects[object_index];
        uint8_t *bytes = NULL;
        size_t size = 0;
        ii42_status status;

        if (object->ref.object_id != object_index + 1)
        {
            return II42_ERR_FORMAT;
        }
        status = ii42_term_cow_object_encode(object, &bytes, &size);
        if (status != II42_OK)
        {
            return status;
        }
        if (ii42_term_cow_checksum(bytes, size) !=
                object->ref.checksum ||
            ii42_segment_blob_checksum(bytes, size) !=
                object->ref.blob_checksum ||
            object->ref.object_bytes != size)
        {
            free(bytes);
            return II42_ERR_FORMAT;
        }
        free(bytes);
    }
    return ii42_term_cow_tree_validate_ref(
        tree,
        &tree->root,
        II42_TERM_COW_RADIX_LEVELS - 1,
        0,
        &max_extent_count
    );
}

static ii42_status
ii42_term_cow_tree_find_leaf(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *root,
    uint64_t leaf_index,
    ii42_term_cow_ref *leaf_ref_out,
    const ii42_term_cow_leaf **leaf_out
)
{
    ii42_term_cow_ref current = *root;

    for (int level = II42_TERM_COW_RADIX_LEVELS - 1;
         level >= 0;
         level--)
    {
        const ii42_term_cow_object *object =
            ii42_term_cow_tree_find_object(tree, &current);
        const ii42_term_cow_child *child;

        if (object == NULL ||
            object->ref.kind != II42_TERM_COW_OBJECT_NODE ||
            object->value.node.level != (uint16_t) level ||
            object->value.node.prefix !=
                ii42_term_cow_path_prefix(
                    leaf_index,
                    (uint16_t) level))
        {
            return II42_ERR_FORMAT;
        }
        child = ii42_term_cow_node_child(
            &object->value.node,
            ii42_term_cow_path_slot(
                leaf_index,
                (uint16_t) level)
        );
        if (child == NULL)
        {
            *leaf_out = NULL;
            memset(leaf_ref_out, 0, sizeof(*leaf_ref_out));
            return II42_OK;
        }
        current = child->ref;
    }
    {
        const ii42_term_cow_object *object =
            ii42_term_cow_tree_find_object(tree, &current);

        if (object == NULL ||
            object->ref.kind != II42_TERM_COW_OBJECT_LEAF ||
            object->value.leaf.base_term_id !=
                leaf_index * II42_TERM_COW_LEAF_TERMS)
        {
            return II42_ERR_FORMAT;
        }
        *leaf_ref_out = current;
        *leaf_out = &object->value.leaf;
    }
    return II42_OK;
}

ii42_status
ii42_term_cow_tree_lookup_at(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t term_id,
    ii42_term_cow_record *record_out
)
{
    const ii42_term_cow_leaf *leaf = NULL;
    ii42_term_cow_ref leaf_ref;
    uint64_t leaf_index;
    uint32_t record_index;
    ii42_status status;

    if (tree == NULL || root == NULL || record_out == NULL ||
        term_id >= vocab_size || vocab_size > tree->vocab_size)
    {
        return II42_ERR_INVALID;
    }
    leaf_index = term_id / II42_TERM_COW_LEAF_TERMS;
    record_index = term_id % II42_TERM_COW_LEAF_TERMS;
    status = ii42_term_cow_tree_find_leaf(
        tree,
        root,
        leaf_index,
        &leaf_ref,
        &leaf
    );
    if (status != II42_OK)
    {
        return status;
    }
    memset(record_out, 0, sizeof(*record_out));
    record_out->term_id = term_id;
    if (leaf == NULL || record_index >= leaf->record_count)
    {
        return II42_OK;
    }
    *record_out = leaf->records[record_index];
    return II42_OK;
}

ii42_status
ii42_term_cow_tree_lookup(
    const ii42_term_cow_tree *tree,
    uint32_t term_id,
    ii42_term_cow_record *record_out
)
{
    if (tree == NULL)
    {
        return II42_ERR_INVALID;
    }
    return ii42_term_cow_tree_lookup_at(
        tree,
        &tree->root,
        tree->vocab_size,
        term_id,
        record_out
    );
}

ii42_status
ii42_term_cow_lookup_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t term_id,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_record *record_out
)
{
    ii42_term_cow_ref current;
    uint64_t leaf_index;
    uint32_t record_index;

    if (root == NULL || loader == NULL || record_out == NULL ||
        !ii42_term_cow_ref_valid(root) ||
        ii42_term_cow_ref_is_unbound(root) ||
        root->kind != II42_TERM_COW_OBJECT_NODE ||
        vocab_size == 0 || term_id >= vocab_size)
    {
        return II42_ERR_INVALID;
    }
    leaf_index = term_id / II42_TERM_COW_LEAF_TERMS;
    record_index = term_id % II42_TERM_COW_LEAF_TERMS;
    current = *root;

    for (int level = II42_TERM_COW_RADIX_LEVELS - 1;
         level >= 0;
         level--)
    {
        ii42_term_cow_object object;
        const ii42_term_cow_child *child;
        ii42_status status = loader(
            loader_context,
            &current,
            &object
        );

        if (status != II42_OK)
        {
            return status;
        }
        if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
            ii42_term_cow_object_validate(&object) != II42_OK ||
            object.ref.kind != II42_TERM_COW_OBJECT_NODE ||
            object.value.node.level != (uint16_t) level ||
            object.value.node.prefix !=
                ii42_term_cow_path_prefix(
                    leaf_index,
                    (uint16_t) level))
        {
            return II42_ERR_FORMAT;
        }
        child = ii42_term_cow_node_child(
            &object.value.node,
            ii42_term_cow_path_slot(
                leaf_index,
                (uint16_t) level)
        );
        if (child == NULL)
        {
            memset(record_out, 0, sizeof(*record_out));
            record_out->term_id = term_id;
            return II42_OK;
        }
        if (ii42_term_cow_ref_is_unbound(&child->ref))
        {
            return II42_ERR_FORMAT;
        }
        current = child->ref;
    }

    {
        ii42_term_cow_object object;
        uint64_t expected_base =
            leaf_index * II42_TERM_COW_LEAF_TERMS;
        ii42_status status = loader(
            loader_context,
            &current,
            &object
        );

        if (status != II42_OK)
        {
            return status;
        }
        if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
            ii42_term_cow_object_validate(&object) != II42_OK ||
            object.ref.kind != II42_TERM_COW_OBJECT_LEAF ||
            object.value.leaf.base_term_id != expected_base ||
            record_index >= object.value.leaf.record_count)
        {
            return II42_ERR_FORMAT;
        }
        *record_out = object.value.leaf.records[record_index];
    }
    return II42_OK;
}

ii42_status
ii42_term_cow_find_extent_pressure_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t threshold,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_record *record_out,
    bool *found_out
)
{
    ii42_term_cow_ref current;
    uint64_t prefix = 0;
    uint32_t expected_max_extent_count = 0;
    bool has_expected_max = false;

    if (root == NULL || loader == NULL || record_out == NULL ||
        found_out == NULL || threshold == 0 ||
        threshold > II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM ||
        !ii42_term_cow_ref_valid(root) ||
        ii42_term_cow_ref_is_unbound(root) ||
        root->kind != II42_TERM_COW_OBJECT_NODE)
    {
        return II42_ERR_INVALID;
    }
    memset(record_out, 0, sizeof(*record_out));
    *found_out = false;
    current = *root;

    for (int level = II42_TERM_COW_RADIX_LEVELS - 1;
         level >= 0;
         level--)
    {
        ii42_term_cow_object object;
        const ii42_term_cow_child *selected = NULL;
        uint32_t node_max_extent_count;
        ii42_status status = loader(
            loader_context,
            &current,
            &object
        );

        if (status != II42_OK)
        {
            return status;
        }
        if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
            ii42_term_cow_object_validate(&object) != II42_OK ||
            object.ref.kind != II42_TERM_COW_OBJECT_NODE ||
            object.value.node.level != (uint16_t) level ||
            object.value.node.prefix != prefix)
        {
            return II42_ERR_FORMAT;
        }
        node_max_extent_count =
            ii42_term_cow_node_max_extent_count(&object.value.node);
        if ((has_expected_max &&
             node_max_extent_count != expected_max_extent_count) ||
            node_max_extent_count < threshold)
        {
            if (!has_expected_max)
            {
                return II42_OK;
            }
            return II42_ERR_FORMAT;
        }
        for (uint32_t child_index = 0;
             child_index < object.value.node.child_count;
             child_index++)
        {
            const ii42_term_cow_child *child =
                &object.value.node.children[child_index];

            if (child->max_extent_count >= threshold)
            {
                selected = child;
                break;
            }
        }
        if (selected == NULL ||
            ii42_term_cow_ref_is_unbound(&selected->ref) ||
            prefix >
                (UINT64_MAX - selected->slot) /
                    II42_TERM_COW_RADIX_FANOUT)
        {
            return II42_ERR_FORMAT;
        }
        prefix =
            prefix * II42_TERM_COW_RADIX_FANOUT + selected->slot;
        expected_max_extent_count = selected->max_extent_count;
        has_expected_max = true;
        current = selected->ref;
    }

    {
        ii42_term_cow_object object;
        uint64_t base_term_id = prefix * II42_TERM_COW_LEAF_TERMS;
        uint32_t leaf_max_extent_count;
        ii42_status status = loader(
            loader_context,
            &current,
            &object
        );

        if (status != II42_OK)
        {
            return status;
        }
        if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
            ii42_term_cow_object_validate(&object) != II42_OK ||
            object.ref.kind != II42_TERM_COW_OBJECT_LEAF ||
            base_term_id >= vocab_size ||
            base_term_id > UINT32_MAX ||
            object.value.leaf.base_term_id != (uint32_t) base_term_id)
        {
            return II42_ERR_FORMAT;
        }
        leaf_max_extent_count =
            ii42_term_cow_leaf_max_extent_count(&object.value.leaf);
        if (!has_expected_max ||
            leaf_max_extent_count != expected_max_extent_count)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t record_index = 0;
             record_index < object.value.leaf.record_count;
             record_index++)
        {
            const ii42_term_cow_record *record =
                &object.value.leaf.records[record_index];

            if (record->extent_count >= threshold)
            {
                *record_out = *record;
                *found_out = true;
                return II42_OK;
            }
        }
    }
    return II42_ERR_FORMAT;
}

ii42_status
ii42_term_cow_append_has_capacity_external(
    const ii42_term_cow_ref *root,
    uint32_t old_vocab_size,
    const ii42_segment_payload_view *new_payload,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    bool *has_capacity_out
)
{
    uint32_t run_index = 0;

    if (root == NULL || new_payload == NULL || loader == NULL ||
        has_capacity_out == NULL ||
        !ii42_term_cow_ref_valid(root) ||
        ii42_term_cow_ref_is_unbound(root) ||
        root->kind != II42_TERM_COW_OBJECT_NODE ||
        (new_payload->run_count > 0 && new_payload->runs == NULL))
    {
        return II42_ERR_INVALID;
    }
    *has_capacity_out = false;
    while (run_index < new_payload->run_count)
    {
        ii42_term_cow_record record;
        uint32_t term_id = new_payload->runs[run_index].term_id;
        uint32_t new_extent_count = 0;
        uint32_t old_extent_count = 0;

        while (run_index < new_payload->run_count &&
               new_payload->runs[run_index].term_id == term_id)
        {
            new_extent_count++;
            run_index++;
        }
        if (term_id < old_vocab_size)
        {
            ii42_status status = ii42_term_cow_lookup_external(
                root,
                old_vocab_size,
                term_id,
                loader,
                loader_context,
                &record
            );

            if (status != II42_OK)
            {
                return status;
            }
            old_extent_count = record.extent_count;
        }
        if (new_extent_count >
                II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM ||
            old_extent_count >
                II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM -
                    new_extent_count)
        {
            return II42_OK;
        }
    }
    *has_capacity_out = true;
    return II42_OK;
}

typedef ii42_status (*ii42_term_cow_leaf_visitor)(
    void *context,
    const ii42_term_cow_leaf *leaf
);

typedef struct ii42_term_cow_external_walk
{
    uint32_t vocab_size;
    size_t max_objects;
    ii42_term_cow_ref *visited;
    size_t visited_count;
    ii42_term_cow_object_loader loader;
    void *loader_context;
    ii42_term_cow_leaf_visitor leaf_visitor;
    void *leaf_context;
    ii42_term_cow_object_visitor object_visitor;
    void *object_context;
} ii42_term_cow_external_walk;

static bool
ii42_term_cow_identity_equal(
    const ii42_term_cow_ref *left,
    const ii42_term_cow_ref *right
)
{
    return left->owner_manifest_id == right->owner_manifest_id &&
        left->object_id == right->object_id;
}

static ii42_status
ii42_term_cow_external_visit_ref(
    ii42_term_cow_external_walk *walk,
    const ii42_term_cow_ref *ref
)
{
    ii42_term_cow_ref *resized;

    if (walk->visited_count >= walk->max_objects)
    {
        return II42_ERR_FORMAT;
    }
    for (size_t index = 0; index < walk->visited_count; index++)
    {
        if (ii42_term_cow_identity_equal(&walk->visited[index], ref))
        {
            return II42_ERR_FORMAT;
        }
    }
    resized = realloc(
        walk->visited,
        (walk->visited_count + 1) * sizeof(*walk->visited)
    );
    if (resized == NULL)
    {
        return II42_ERR_NOMEM;
    }
    walk->visited = resized;
    walk->visited[walk->visited_count++] = *ref;
    return II42_OK;
}

static ii42_status
ii42_term_cow_external_walk_ref(
    ii42_term_cow_external_walk *walk,
    const ii42_term_cow_ref *ref,
    ii42_term_cow_object_kind expected_kind,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint32_t *max_extent_count_out
)
{
    ii42_term_cow_object object;
    uint32_t max_extent_count = 0;
    ii42_status status;

    if (max_extent_count_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_term_cow_external_visit_ref(walk, ref);
    if (status != II42_OK)
    {
        return status;
    }
    status = walk->loader(walk->loader_context, ref, &object);
    if (status != II42_OK)
    {
        return status;
    }
    if (!ii42_term_cow_refs_equal(&object.ref, ref) ||
        ii42_term_cow_object_validate(&object) != II42_OK ||
        object.ref.kind != expected_kind)
    {
        return II42_ERR_FORMAT;
    }

    if (expected_kind == II42_TERM_COW_OBJECT_NODE)
    {
        const ii42_term_cow_node *node = &object.value.node;

        if (node->level != expected_level ||
            node->prefix != expected_prefix)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t child_index = 0;
             child_index < node->child_count;
             child_index++)
        {
            const ii42_term_cow_child *child =
                &node->children[child_index];
            uint64_t child_prefix;
            uint32_t child_max_extent_count;

            if (expected_prefix >
                (UINT64_MAX - child->slot) /
                    II42_TERM_COW_RADIX_FANOUT)
            {
                return II42_ERR_FORMAT;
            }
            child_prefix =
                expected_prefix * II42_TERM_COW_RADIX_FANOUT +
                child->slot;
            status = ii42_term_cow_external_walk_ref(
                walk,
                &child->ref,
                expected_level == 0
                    ? II42_TERM_COW_OBJECT_LEAF
                    : II42_TERM_COW_OBJECT_NODE,
                expected_level == 0
                    ? 0
                    : (uint16_t) (expected_level - 1),
                child_prefix,
                &child_max_extent_count
            );
            if (status != II42_OK)
            {
                return status;
            }
            if (child->max_extent_count != child_max_extent_count)
            {
                return II42_ERR_FORMAT;
            }
            if (child_max_extent_count > max_extent_count)
            {
                max_extent_count = child_max_extent_count;
            }
        }
        *max_extent_count_out = max_extent_count;
    }
    else
    {
        const ii42_term_cow_leaf *leaf = &object.value.leaf;
        uint64_t base_term_id =
            expected_prefix * II42_TERM_COW_LEAF_TERMS;
        uint32_t expected_records;

        if (base_term_id >= walk->vocab_size ||
            base_term_id > UINT32_MAX ||
            leaf->base_term_id != (uint32_t) base_term_id)
        {
            return II42_ERR_FORMAT;
        }
        expected_records = walk->vocab_size - leaf->base_term_id;
        if (expected_records > II42_TERM_COW_LEAF_TERMS)
        {
            expected_records = II42_TERM_COW_LEAF_TERMS;
        }
        if (leaf->record_count != expected_records)
        {
            return II42_ERR_FORMAT;
        }
        *max_extent_count_out =
            ii42_term_cow_leaf_max_extent_count(leaf);
        if (walk->leaf_visitor != NULL)
        {
            status = walk->leaf_visitor(walk->leaf_context, leaf);
            if (status != II42_OK)
            {
                return status;
            }
        }
    }
    if (walk->object_visitor != NULL)
    {
        return walk->object_visitor(walk->object_context, &object);
    }
    return II42_OK;
}

static ii42_status
ii42_term_cow_walk_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_leaf_visitor leaf_visitor,
    void *leaf_context,
    ii42_term_cow_object_visitor object_visitor,
    void *object_context
)
{
    ii42_term_cow_external_walk walk;
    size_t leaf_count;
    uint32_t max_extent_count;
    ii42_status status;

    if (root == NULL || loader == NULL ||
        !ii42_term_cow_ref_valid(root) ||
        ii42_term_cow_ref_is_unbound(root) ||
        root->kind != II42_TERM_COW_OBJECT_NODE)
    {
        return II42_ERR_INVALID;
    }
    leaf_count =
        ((size_t) vocab_size + II42_TERM_COW_LEAF_TERMS - 1) /
        II42_TERM_COW_LEAF_TERMS;
    if (leaf_count >
        (SIZE_MAX - 1) / (II42_TERM_COW_RADIX_LEVELS + 1))
    {
        return II42_ERR_RANGE;
    }

    memset(&walk, 0, sizeof(walk));
    walk.vocab_size = vocab_size;
    walk.max_objects =
        1 + leaf_count * (II42_TERM_COW_RADIX_LEVELS + 1);
    walk.loader = loader;
    walk.loader_context = loader_context;
    walk.leaf_visitor = leaf_visitor;
    walk.leaf_context = leaf_context;
    walk.object_visitor = object_visitor;
    walk.object_context = object_context;
    status = ii42_term_cow_external_walk_ref(
        &walk,
        root,
        II42_TERM_COW_OBJECT_NODE,
        II42_TERM_COW_RADIX_LEVELS - 1,
        0,
        &max_extent_count
    );
    free(walk.visited);
    return status;
}

ii42_status
ii42_term_cow_validate_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    ii42_term_cow_object_loader loader,
    void *loader_context
)
{
    return ii42_term_cow_walk_external(
        root,
        vocab_size,
        loader,
        loader_context,
        NULL,
        NULL,
        NULL,
        NULL
    );
}

ii42_status
ii42_term_cow_visit_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_object_visitor visitor,
    void *visitor_context
)
{
    if (visitor == NULL)
    {
        return II42_ERR_INVALID;
    }
    return ii42_term_cow_walk_external(
        root,
        vocab_size,
        loader,
        loader_context,
        NULL,
        NULL,
        visitor,
        visitor_context
    );
}

typedef struct ii42_term_cow_materialize_context
{
    const ii42_segment_manifest *manifest;
    ii42_term_directory *directory;
    uint32_t *doc_frequencies;
    ii42_segment_object_ref *lexical_catalog_refs;
    ii42_term_cow_fold_state *fold_states;
    uint32_t next_term_id;
    size_t extent_capacity;
} ii42_term_cow_materialize_context;

static bool
ii42_term_cow_manifest_segment_index(
    const ii42_segment_manifest *manifest,
    uint64_t segment_id,
    uint32_t *segment_index_out
);

static int
ii42_term_cow_compare_materialized_extent(
    const void *left,
    const void *right
)
{
    const ii42_term_extent_descriptor *left_extent = left;
    const ii42_term_extent_descriptor *right_extent = right;

    if (left_extent->segment_index != right_extent->segment_index)
    {
        return left_extent->segment_index <
            right_extent->segment_index
                ? -1
                : 1;
    }
    if (left_extent->posting_offset != right_extent->posting_offset)
    {
        return left_extent->posting_offset <
            right_extent->posting_offset
                ? -1
                : 1;
    }
    return 0;
}

static ii42_status
ii42_term_cow_materialize_leaf(
    void *context,
    const ii42_term_cow_leaf *leaf
)
{
    ii42_term_cow_materialize_context *materialize = context;
    ii42_term_directory *directory = materialize->directory;

    if (leaf->base_term_id < materialize->next_term_id)
    {
        return II42_ERR_FORMAT;
    }
    while (materialize->next_term_id < leaf->base_term_id)
    {
        directory->term_offsets[materialize->next_term_id++] =
            directory->extent_count;
    }
    for (uint32_t record_index = 0;
         record_index < leaf->record_count;
         record_index++)
    {
        const ii42_term_cow_record *record =
            &leaf->records[record_index];
        uint32_t first_extent = directory->extent_count;
        uint32_t tail_extent_count = 0;
        size_t required_extents;
        bool has_neutral =
            (record->flags &
             II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD) != 0;
        bool has_neutral_minor =
            (record->flags &
             II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0;
        uint64_t neutral_coverage = has_neutral_minor
            ? record->neutral_minor_fold_coverage
            : record->neutral_fold_coverage;

        if (record->term_id != materialize->next_term_id ||
            record->raw_document_frequency >
                materialize->manifest->document_slot_count ||
            (has_neutral &&
             record->neutral_fold.owner_manifest_id >
                materialize->manifest->manifest_id) ||
            (has_neutral_minor &&
             record->neutral_minor_fold.owner_manifest_id >
                materialize->manifest->manifest_id) ||
            ((record->flags &
              II42_TERM_COW_RECORD_FLAG_IMPACT_FOLD) != 0 &&
             record->impact_fold.owner_manifest_id >
                materialize->manifest->manifest_id))
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t extent_index = 0;
             extent_index < record->extent_count;
             extent_index++)
        {
            const ii42_stable_term_extent *source =
                &record->extents[extent_index];
            uint32_t segment_index;
            const ii42_segment_descriptor *segment;

            if (!ii42_term_cow_manifest_segment_index(
                    materialize->manifest,
                    source->segment_id,
                    &segment_index
                ))
            {
                return II42_ERR_FORMAT;
            }
            segment = &materialize->manifest->segments[segment_index];
            if (has_neutral &&
                segment->min_sequence <=
                    neutral_coverage &&
                segment->max_sequence >
                    neutral_coverage)
            {
                return II42_ERR_FORMAT;
            }
            if (!has_neutral ||
                segment->max_sequence >
                    neutral_coverage)
            {
                tail_extent_count++;
            }
        }
        required_extents =
            (size_t) directory->extent_count + tail_extent_count;
        if (required_extents > UINT32_MAX)
        {
            return II42_ERR_FORMAT;
        }
        directory->term_offsets[materialize->next_term_id] =
            directory->extent_count;
        materialize->doc_frequencies[materialize->next_term_id] =
            record->raw_document_frequency;
        if (materialize->lexical_catalog_refs != NULL)
        {
            materialize->lexical_catalog_refs[
                materialize->next_term_id] =
                record->lexical_catalog;
        }
        if (materialize->fold_states != NULL)
        {
            ii42_term_cow_fold_state *state =
                &materialize->fold_states[materialize->next_term_id];

            state->neutral_coverage =
                record->neutral_fold_coverage;
            state->neutral_minor_coverage =
                record->neutral_minor_fold_coverage;
            state->impact_coverage =
                record->impact_fold_coverage;
            state->impact_statistics_epoch =
                record->impact_statistics_epoch;
            state->neutral_ref = record->neutral_fold;
            state->neutral_minor_ref =
                record->neutral_minor_fold;
            state->impact_ref = record->impact_fold;
        }
        if (required_extents > materialize->extent_capacity)
        {
            size_t next_capacity =
                materialize->extent_capacity == 0
                    ? 16
                    : materialize->extent_capacity;
            ii42_term_extent_descriptor *resized;

            while (next_capacity < required_extents)
            {
                if (next_capacity > SIZE_MAX / 2)
                {
                    return II42_ERR_RANGE;
                }
                next_capacity *= 2;
            }
            resized = realloc(
                directory->extents,
                next_capacity * sizeof(*directory->extents)
            );
            if (resized == NULL)
            {
                return II42_ERR_NOMEM;
            }
            directory->extents = resized;
            materialize->extent_capacity = next_capacity;
        }
        for (uint32_t extent_index = 0;
             extent_index < record->extent_count;
             extent_index++)
        {
            const ii42_stable_term_extent *source =
                &record->extents[extent_index];
            ii42_term_extent_descriptor *target =
                &directory->extents[directory->extent_count];
            uint32_t segment_index;
            const ii42_segment_descriptor *segment;

            if (!ii42_term_cow_manifest_segment_index(
                    materialize->manifest,
                    source->segment_id,
                    &segment_index))
            {
                return II42_ERR_FORMAT;
            }
            segment = &materialize->manifest->segments[segment_index];
            if (has_neutral &&
                segment->max_sequence <=
                    neutral_coverage)
            {
                continue;
            }
            target->segment_index = segment_index;
            target->kind = source->kind;
            target->posting_offset = source->posting_offset;
            target->posting_count = source->posting_count;
            directory->extent_count++;
        }
        if (tail_extent_count > 1)
        {
            qsort(
                &directory->extents[first_extent],
                tail_extent_count,
                sizeof(*directory->extents),
                ii42_term_cow_compare_materialized_extent
            );
        }
        materialize->next_term_id++;
    }
    return II42_OK;
}

ii42_status
ii42_term_cow_materialize_external(
    const ii42_term_cow_ref *root,
    const ii42_segment_manifest *manifest,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_directory *directory_out,
    uint32_t **doc_frequencies_out,
    ii42_segment_object_ref **lexical_catalog_refs_out,
    ii42_term_cow_fold_state **fold_states_out
)
{
    ii42_term_cow_materialize_context materialize;
    ii42_term_directory directory;
    uint32_t *doc_frequencies;
    ii42_segment_object_ref *lexical_catalog_refs = NULL;
    ii42_term_cow_fold_state *fold_states = NULL;
    ii42_status status;

    if (root == NULL || manifest == NULL || loader == NULL ||
        directory_out == NULL || doc_frequencies_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    *doc_frequencies_out = NULL;
    if (lexical_catalog_refs_out != NULL)
    {
        *lexical_catalog_refs_out = NULL;
    }
    if (fold_states_out != NULL)
    {
        *fold_states_out = NULL;
    }
    ii42_term_directory_init(&directory);
    directory.vocab_size = manifest->vocab_size;
    directory.term_offsets = calloc(
        (size_t) directory.vocab_size + 1,
        sizeof(*directory.term_offsets)
    );
    doc_frequencies = directory.vocab_size == 0
        ? NULL
        : calloc(directory.vocab_size, sizeof(*doc_frequencies));
    if (lexical_catalog_refs_out != NULL &&
        directory.vocab_size > 0)
    {
        lexical_catalog_refs = calloc(
            directory.vocab_size,
            sizeof(*lexical_catalog_refs)
        );
    }
    if (fold_states_out != NULL && directory.vocab_size > 0)
    {
        fold_states = calloc(
            directory.vocab_size,
            sizeof(*fold_states)
        );
    }
    if (directory.term_offsets == NULL ||
        (directory.vocab_size > 0 && doc_frequencies == NULL) ||
        (lexical_catalog_refs_out != NULL &&
         directory.vocab_size > 0 &&
         lexical_catalog_refs == NULL) ||
        (fold_states_out != NULL &&
         directory.vocab_size > 0 &&
         fold_states == NULL))
    {
        free(fold_states);
        free(lexical_catalog_refs);
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return II42_ERR_NOMEM;
    }

    memset(&materialize, 0, sizeof(materialize));
    materialize.manifest = manifest;
    materialize.directory = &directory;
    materialize.doc_frequencies = doc_frequencies;
    materialize.lexical_catalog_refs = lexical_catalog_refs;
    materialize.fold_states = fold_states;
    status = ii42_term_cow_walk_external(
        root,
        manifest->vocab_size,
        loader,
        loader_context,
        ii42_term_cow_materialize_leaf,
        &materialize,
        NULL,
        NULL
    );
    if (status != II42_OK)
    {
        free(fold_states);
        free(lexical_catalog_refs);
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return status;
    }
    while (materialize.next_term_id <= directory.vocab_size)
    {
        directory.term_offsets[materialize.next_term_id++] =
            directory.extent_count;
    }
    status = ii42_term_directory_validate(&directory, manifest);
    if (status != II42_OK)
    {
        free(fold_states);
        free(lexical_catalog_refs);
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return status;
    }

    ii42_term_directory_free(directory_out);
    *directory_out = directory;
    *doc_frequencies_out = doc_frequencies;
    if (lexical_catalog_refs_out != NULL)
    {
        *lexical_catalog_refs_out = lexical_catalog_refs;
    }
    if (fold_states_out != NULL)
    {
        *fold_states_out = fold_states;
    }
    return II42_OK;
}

typedef struct ii42_term_cow_patch_leaf
{
    uint64_t leaf_index;
    bool has_source;
    ii42_term_cow_ref source_ref;
    uint32_t max_extent_count;
    ii42_term_cow_leaf leaf;
    ii42_term_cow_ref ref;
} ii42_term_cow_patch_leaf;

typedef struct ii42_term_cow_patch_node
{
    uint64_t prefix;
    bool has_source;
    ii42_term_cow_ref source_ref;
    uint32_t max_extent_count;
    ii42_term_cow_node node;
    ii42_term_cow_ref ref;
} ii42_term_cow_patch_node;

typedef struct ii42_term_cow_patch_level
{
    ii42_term_cow_patch_node *nodes;
    size_t count;
    size_t capacity;
} ii42_term_cow_patch_level;

typedef struct ii42_term_cow_patch_builder
{
    const ii42_term_cow_ref *old_root;
    uint32_t old_vocab_size;
    uint32_t next_vocab_size;
    ii42_term_cow_object_loader loader;
    void *loader_context;
    ii42_term_cow_patch_leaf *leaves;
    size_t leaf_count;
    ii42_term_cow_patch_level levels[II42_TERM_COW_RADIX_LEVELS];
} ii42_term_cow_patch_builder;

static ii42_status ii42_term_cow_record_add_extent(
    ii42_term_cow_record *record,
    uint64_t segment_id,
    const ii42_segment_term_run *run
);

static bool
ii42_term_cow_segment_is_replaced(
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    uint32_t replaced_segment_count,
    uint64_t segment_id
)
{
    uint32_t replaced_end =
        first_segment_index + replaced_segment_count;

    for (uint32_t segment_index = first_segment_index;
         segment_index < replaced_end;
         segment_index++)
    {
        if (manifest->segments[segment_index].segment_id == segment_id)
        {
            return true;
        }
    }
    return false;
}

static ii42_status
ii42_term_cow_record_remove_replaced_extents(
    ii42_term_cow_record *record,
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    uint32_t replaced_segment_count,
    uint64_t *removed_extent_count,
    uint64_t *removed_lexical_posting_count
)
{
    uint32_t write_index = 0;

    if (record == NULL || manifest == NULL ||
        removed_extent_count == NULL ||
        removed_lexical_posting_count == NULL ||
        replaced_segment_count == 0 ||
        first_segment_index >= manifest->segment_count ||
        replaced_segment_count >
            manifest->segment_count - first_segment_index)
    {
        return II42_ERR_INVALID;
    }
    for (uint32_t read_index = 0;
         read_index < record->extent_count;
         read_index++)
    {
        const ii42_stable_term_extent *source =
            &record->extents[read_index];

        if (ii42_term_cow_segment_is_replaced(
                manifest,
                first_segment_index,
                replaced_segment_count,
                source->segment_id))
        {
            (*removed_extent_count)++;
            if (source->kind ==
                II42_POSTING_EXTENT_LEXICAL_NEUTRAL)
            {
                if (source->posting_count >
                        record->raw_document_frequency ||
                    *removed_lexical_posting_count >
                        UINT64_MAX - source->posting_count)
                {
                    return II42_ERR_FORMAT;
                }
                record->raw_document_frequency -=
                    (uint32_t) source->posting_count;
                *removed_lexical_posting_count +=
                    source->posting_count;
            }
            continue;
        }
        if (write_index != read_index)
        {
            record->extents[write_index] = *source;
        }
        write_index++;
    }
    memset(
        &record->extents[write_index],
        0,
        (size_t) (record->extent_count - write_index) *
            sizeof(*record->extents)
    );
    record->extent_count = write_index;
    return ii42_term_cow_record_validate(record);
}

typedef enum ii42_term_cow_replace_fold_relation
{
    II42_TERM_COW_REPLACE_TAIL = 0,
    II42_TERM_COW_REPLACE_COVERED = 1,
    II42_TERM_COW_REPLACE_STRADDLE = 2
} ii42_term_cow_replace_fold_relation;

static ii42_status
ii42_term_cow_classify_replace_fold_relation(
    const ii42_term_cow_record *record,
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload_view *replaced_payloads,
    uint32_t replaced_payload_count,
    ii42_term_cow_replace_fold_relation *relation_out
)
{
    bool has_covered_run = false;
    bool has_tail_run = false;
    uint64_t neutral_coverage;

    if (record == NULL || manifest == NULL ||
        replaced_payloads == NULL || relation_out == NULL ||
        replaced_payload_count == 0 ||
        first_segment_index >= manifest->segment_count ||
        replaced_payload_count >
            manifest->segment_count - first_segment_index)
    {
        return II42_ERR_INVALID;
    }
    if ((record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD) == 0)
    {
        *relation_out = II42_TERM_COW_REPLACE_TAIL;
        return II42_OK;
    }
    neutral_coverage =
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0
            ? record->neutral_minor_fold_coverage
            : record->neutral_fold_coverage;
    for (uint32_t payload_index = 0;
         payload_index < replaced_payload_count;
         payload_index++)
    {
        const ii42_segment_descriptor *segment =
            &manifest->segments[first_segment_index + payload_index];
        const ii42_segment_payload_view *payload =
            &replaced_payloads[payload_index];

        for (uint32_t run_index = 0;
             run_index < payload->run_count;
             run_index++)
        {
            const ii42_segment_term_run *run =
                &payload->runs[run_index];

            if (run->term_id < record->term_id)
            {
                continue;
            }
            if (run->term_id > record->term_id)
            {
                break;
            }
            if (segment->max_sequence <=
                    neutral_coverage)
            {
                has_covered_run = true;
            }
            else if (segment->min_sequence >
                     neutral_coverage)
            {
                has_tail_run = true;
            }
            else
            {
                bool retained_tail = false;

                for (uint32_t extent_index = 0;
                     extent_index < record->extent_count;
                     extent_index++)
                {
                    const ii42_stable_term_extent *extent =
                        &record->extents[extent_index];

                    if (extent->segment_id != segment->segment_id ||
                        extent->kind != run->kind)
                    {
                        continue;
                    }
                    if (extent->posting_offset != run->posting_offset ||
                        extent->posting_count != run->posting_count)
                    {
                        return II42_ERR_FORMAT;
                    }
                    retained_tail = true;
                    break;
                }
                /*
                 * A run omitted from the COW tail record is already covered
                 * by the fold, even if its segment descriptor crosses the
                 * watermark. A retained tail run cannot share that crossing
                 * replacement descriptor and remains unsafe.
                 */
                if (retained_tail)
                {
                    *relation_out = II42_TERM_COW_REPLACE_STRADDLE;
                    return II42_OK;
                }
                has_covered_run = true;
            }
        }
    }
    if (!has_covered_run && !has_tail_run)
    {
        return II42_ERR_FORMAT;
    }
    if (has_covered_run && has_tail_run)
    {
        *relation_out = II42_TERM_COW_REPLACE_STRADDLE;
        return II42_OK;
    }
    if (has_tail_run &&
        manifest->segments[first_segment_index].min_sequence <=
            neutral_coverage &&
        manifest->segments[
            first_segment_index + replaced_payload_count - 1
        ].max_sequence > neutral_coverage)
    {
        /*
         * A retained tail run cannot reference a replacement descriptor
         * that crosses its fold watermark. Covered-only runs are omitted
         * from the next term record, so the descriptor is irrelevant to
         * them and compaction remains safe.
         */
        *relation_out = II42_TERM_COW_REPLACE_STRADDLE;
        return II42_OK;
    }
    *relation_out = has_covered_run
        ? II42_TERM_COW_REPLACE_COVERED
        : II42_TERM_COW_REPLACE_TAIL;
    return II42_OK;
}

static void
ii42_term_cow_patch_builder_free(ii42_term_cow_patch_builder *builder)
{
    if (builder == NULL)
    {
        return;
    }
    free(builder->leaves);
    for (uint16_t level = 0;
         level < II42_TERM_COW_RADIX_LEVELS;
         level++)
    {
        free(builder->levels[level].nodes);
    }
    memset(builder, 0, sizeof(*builder));
}

static int
ii42_term_cow_compare_leaf_index(const void *left, const void *right)
{
    uint64_t left_value = *(const uint64_t *) left;
    uint64_t right_value = *(const uint64_t *) right;

    if (left_value < right_value)
    {
        return -1;
    }
    if (left_value > right_value)
    {
        return 1;
    }
    return 0;
}

static ii42_status
ii42_term_cow_collect_append_leaves(
    uint32_t old_vocab_size,
    uint32_t next_vocab_size,
    const ii42_segment_payload_view *new_payload,
    uint64_t **leaf_indices_out,
    size_t *leaf_count_out
)
{
    uint64_t *leaf_indices;
    size_t capacity;
    size_t count = 0;

    if (new_payload == NULL || leaf_indices_out == NULL ||
        leaf_count_out == NULL || next_vocab_size < old_vocab_size ||
        (uint64_t) new_payload->run_count + 1 > (uint64_t) SIZE_MAX)
    {
        return II42_ERR_INVALID;
    }
    *leaf_indices_out = NULL;
    *leaf_count_out = 0;
    capacity = (size_t) new_payload->run_count + 1;
    leaf_indices = capacity == 0
        ? NULL
        : calloc(capacity, sizeof(*leaf_indices));
    if (capacity > 0 && leaf_indices == NULL)
    {
        return II42_ERR_NOMEM;
    }

    for (uint32_t run_index = 0;
         run_index < new_payload->run_count;
         run_index++)
    {
        uint64_t leaf_index =
            new_payload->runs[run_index].term_id /
            II42_TERM_COW_LEAF_TERMS;

        if (count == 0 || leaf_indices[count - 1] != leaf_index)
        {
            leaf_indices[count++] = leaf_index;
        }
    }
    if (next_vocab_size > old_vocab_size &&
        old_vocab_size > 0 &&
        old_vocab_size % II42_TERM_COW_LEAF_TERMS != 0)
    {
        leaf_indices[count++] =
            (old_vocab_size - 1) / II42_TERM_COW_LEAF_TERMS;
    }
    if (count > 1)
    {
        size_t write_index = 1;

        qsort(
            leaf_indices,
            count,
            sizeof(*leaf_indices),
            ii42_term_cow_compare_leaf_index
        );
        for (size_t read_index = 1; read_index < count; read_index++)
        {
            if (leaf_indices[read_index] !=
                leaf_indices[write_index - 1])
            {
                leaf_indices[write_index++] = leaf_indices[read_index];
            }
        }
        count = write_index;
    }
    *leaf_indices_out = leaf_indices;
    *leaf_count_out = count;
    return II42_OK;
}

static int
ii42_term_cow_compare_term_id(const void *left, const void *right)
{
    uint32_t left_value = *(const uint32_t *) left;
    uint32_t right_value = *(const uint32_t *) right;

    if (left_value < right_value)
    {
        return -1;
    }
    if (left_value > right_value)
    {
        return 1;
    }
    return 0;
}

static ii42_status
ii42_term_cow_collect_replace_terms(
    const ii42_segment_payload_view *replaced_payloads,
    uint32_t replaced_payload_count,
    const ii42_segment_payload_view *replacement_payload,
    uint32_t **term_ids_out,
    size_t *term_count_out,
    uint64_t **leaf_indices_out,
    size_t *leaf_count_out
)
{
    uint64_t total_runs = replacement_payload->run_count;
    uint32_t *term_ids = NULL;
    uint64_t *leaf_indices = NULL;
    size_t term_count = 0;
    size_t leaf_count = 0;

    if (replaced_payloads == NULL || replaced_payload_count == 0 ||
        replacement_payload == NULL || term_ids_out == NULL ||
        term_count_out == NULL || leaf_indices_out == NULL ||
        leaf_count_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    *term_ids_out = NULL;
    *term_count_out = 0;
    *leaf_indices_out = NULL;
    *leaf_count_out = 0;
    for (uint32_t payload_index = 0;
         payload_index < replaced_payload_count;
         payload_index++)
    {
        if (total_runs >
            (uint64_t) SIZE_MAX - replaced_payloads[payload_index].run_count)
        {
            return II42_ERR_RANGE;
        }
        total_runs += replaced_payloads[payload_index].run_count;
    }
    if (total_runs > SIZE_MAX / sizeof(*term_ids))
    {
        return II42_ERR_RANGE;
    }
    if (total_runs > 0)
    {
        term_ids = calloc((size_t) total_runs, sizeof(*term_ids));
        leaf_indices = calloc(
            (size_t) total_runs,
            sizeof(*leaf_indices)
        );
        if (term_ids == NULL || leaf_indices == NULL)
        {
            free(leaf_indices);
            free(term_ids);
            return II42_ERR_NOMEM;
        }
    }
    for (uint32_t payload_index = 0;
         payload_index < replaced_payload_count;
         payload_index++)
    {
        const ii42_segment_payload_view *payload =
            &replaced_payloads[payload_index];

        for (uint32_t run_index = 0;
             run_index < payload->run_count;
             run_index++)
        {
            term_ids[term_count++] = payload->runs[run_index].term_id;
        }
    }
    for (uint32_t run_index = 0;
         run_index < replacement_payload->run_count;
         run_index++)
    {
        term_ids[term_count++] =
            replacement_payload->runs[run_index].term_id;
    }
    if (term_count > 1)
    {
        size_t write_index = 1;

        qsort(
            term_ids,
            term_count,
            sizeof(*term_ids),
            ii42_term_cow_compare_term_id
        );
        for (size_t read_index = 1;
             read_index < term_count;
             read_index++)
        {
            if (term_ids[read_index] != term_ids[write_index - 1])
            {
                term_ids[write_index++] = term_ids[read_index];
            }
        }
        term_count = write_index;
    }
    for (size_t term_index = 0;
         term_index < term_count;
         term_index++)
    {
        uint64_t leaf_index =
            term_ids[term_index] / II42_TERM_COW_LEAF_TERMS;

        if (leaf_count == 0 ||
            leaf_indices[leaf_count - 1] != leaf_index)
        {
            leaf_indices[leaf_count++] = leaf_index;
        }
    }
    *term_ids_out = term_ids;
    *term_count_out = term_count;
    *leaf_indices_out = leaf_indices;
    *leaf_count_out = leaf_count;
    return II42_OK;
}

static ii42_status
ii42_term_cow_patch_level_get(
    ii42_term_cow_patch_level *patch_level,
    uint16_t level,
    uint64_t prefix,
    const ii42_term_cow_ref *source_ref,
    const ii42_term_cow_node *source_node,
    ii42_term_cow_patch_node **node_out
)
{
    ii42_term_cow_patch_node *node;

    if (patch_level == NULL || node_out == NULL ||
        level >= II42_TERM_COW_RADIX_LEVELS ||
        ((source_ref == NULL) != (source_node == NULL)))
    {
        return II42_ERR_INVALID;
    }
    if (patch_level->count > 0)
    {
        node = &patch_level->nodes[patch_level->count - 1];
        if (node->prefix == prefix)
        {
            if (node->has_source != (source_ref != NULL) ||
                (source_ref != NULL &&
                 (!ii42_term_cow_refs_equal(
                      &node->source_ref,
                      source_ref) ||
                  memcmp(
                      &node->node,
                      source_node,
                      sizeof(node->node)) != 0)))
            {
                return II42_ERR_FORMAT;
            }
            *node_out = node;
            return II42_OK;
        }
        if (node->prefix > prefix)
        {
            return II42_ERR_FORMAT;
        }
    }
    if (patch_level->count == patch_level->capacity)
    {
        size_t next_capacity = patch_level->capacity == 0
            ? 4
            : patch_level->capacity * 2;
        ii42_term_cow_patch_node *resized;

        if (next_capacity < patch_level->capacity ||
            next_capacity >
                SIZE_MAX / sizeof(*patch_level->nodes))
        {
            return II42_ERR_RANGE;
        }
        resized = realloc(
            patch_level->nodes,
            next_capacity * sizeof(*patch_level->nodes)
        );
        if (resized == NULL)
        {
            return II42_ERR_NOMEM;
        }
        patch_level->nodes = resized;
        patch_level->capacity = next_capacity;
    }
    node = &patch_level->nodes[patch_level->count++];
    memset(node, 0, sizeof(*node));
    node->prefix = prefix;
    if (source_ref != NULL)
    {
        node->has_source = true;
        node->source_ref = *source_ref;
        node->node = *source_node;
    }
    else
    {
        node->node.level = level;
        node->node.prefix = prefix;
    }
    node->max_extent_count =
        ii42_term_cow_node_max_extent_count(&node->node);
    *node_out = node;
    return II42_OK;
}

static ii42_status
ii42_term_cow_patch_load_leaf(
    ii42_term_cow_patch_builder *builder,
    uint64_t leaf_index,
    ii42_term_cow_patch_leaf *patch_leaf
)
{
    ii42_term_cow_ref current = *builder->old_root;
    bool path_exists = builder->old_vocab_size > 0;
    uint64_t base_term_id =
        leaf_index * II42_TERM_COW_LEAF_TERMS;
    uint32_t record_count;

    if (base_term_id >= builder->next_vocab_size ||
        base_term_id > UINT32_MAX)
    {
        return II42_ERR_RANGE;
    }
    memset(patch_leaf, 0, sizeof(*patch_leaf));
    patch_leaf->leaf_index = leaf_index;

    for (int level = II42_TERM_COW_RADIX_LEVELS - 1;
         level >= 0;
         level--)
    {
        ii42_term_cow_patch_node *patch_node;
        const ii42_term_cow_ref *source_ref = NULL;
        const ii42_term_cow_node *source_node = NULL;
        ii42_term_cow_object object;
        uint64_t prefix = ii42_term_cow_path_prefix(
            leaf_index,
            (uint16_t) level
        );
        ii42_status status;

        if (path_exists)
        {
            status = builder->loader(
                builder->loader_context,
                &current,
                &object
            );
            if (status != II42_OK)
            {
                return status;
            }
            if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
                ii42_term_cow_object_validate(&object) != II42_OK ||
                object.ref.kind != II42_TERM_COW_OBJECT_NODE ||
                object.value.node.level != (uint16_t) level ||
                object.value.node.prefix != prefix)
            {
                return II42_ERR_FORMAT;
            }
            source_ref = &current;
            source_node = &object.value.node;
        }
        status = ii42_term_cow_patch_level_get(
            &builder->levels[level],
            (uint16_t) level,
            prefix,
            source_ref,
            source_node,
            &patch_node
        );
        if (status != II42_OK)
        {
            return status;
        }
        if (path_exists)
        {
            const ii42_term_cow_child *child =
                ii42_term_cow_node_child(
                    &patch_node->node,
                    ii42_term_cow_path_slot(
                        leaf_index,
                        (uint16_t) level)
                );

            if (child == NULL)
            {
                path_exists = false;
            }
            else if (ii42_term_cow_ref_is_unbound(&child->ref))
            {
                return II42_ERR_FORMAT;
            }
            else
            {
                current = child->ref;
            }
        }
    }
    if (path_exists)
    {
        ii42_term_cow_object object;
        ii42_status status = builder->loader(
            builder->loader_context,
            &current,
            &object
        );

        if (status != II42_OK)
        {
            return status;
        }
        if (!ii42_term_cow_refs_equal(&object.ref, &current) ||
            ii42_term_cow_object_validate(&object) != II42_OK ||
            object.ref.kind != II42_TERM_COW_OBJECT_LEAF ||
            object.value.leaf.base_term_id != (uint32_t) base_term_id)
        {
            return II42_ERR_FORMAT;
        }
        patch_leaf->has_source = true;
        patch_leaf->source_ref = current;
        patch_leaf->leaf = object.value.leaf;
    }
    else
    {
        patch_leaf->leaf.base_term_id = (uint32_t) base_term_id;
    }

    record_count = builder->next_vocab_size - (uint32_t) base_term_id;
    if (record_count > II42_TERM_COW_LEAF_TERMS)
    {
        record_count = II42_TERM_COW_LEAF_TERMS;
    }
    if (patch_leaf->leaf.record_count > record_count)
    {
        return II42_ERR_FORMAT;
    }
    for (uint32_t record_index = patch_leaf->leaf.record_count;
         record_index < record_count;
         record_index++)
    {
        memset(
            &patch_leaf->leaf.records[record_index],
            0,
            sizeof(patch_leaf->leaf.records[record_index])
        );
        patch_leaf->leaf.records[record_index].term_id =
            (uint32_t) base_term_id + record_index;
    }
    patch_leaf->leaf.record_count = record_count;
    return II42_OK;
}

static ii42_status
ii42_term_cow_patch_append_objects(
    ii42_term_cow_patch_builder *builder,
    ii42_term_cow_tree *patch,
    ii42_term_cow_update_stats *stats
)
{
    ii42_status status;

    for (size_t leaf_index = 0;
         leaf_index < builder->leaf_count;
         leaf_index++)
    {
        ii42_term_cow_object object;
        size_t written_bytes = 0;

        memset(&object, 0, sizeof(object));
        if (builder->leaves[leaf_index].has_source)
        {
            status = ii42_term_cow_tree_add_retired_ref(
                patch,
                &builder->leaves[leaf_index].source_ref
            );
            if (status != II42_OK)
            {
                return status;
            }
        }
        object.ref.kind = II42_TERM_COW_OBJECT_LEAF;
        object.value.leaf = builder->leaves[leaf_index].leaf;
        builder->leaves[leaf_index].max_extent_count =
            ii42_term_cow_leaf_max_extent_count(
                &builder->leaves[leaf_index].leaf
            );
        status = ii42_term_cow_tree_append_object(
            patch,
            &object,
            &builder->leaves[leaf_index].ref,
            &written_bytes
        );
        if (status != II42_OK)
        {
            return status;
        }
        stats->written_leaves++;
        stats->written_bytes += written_bytes;
    }

    for (uint16_t level = 0;
         level < II42_TERM_COW_RADIX_LEVELS;
         level++)
    {
        ii42_term_cow_patch_level *patch_level =
            &builder->levels[level];
        size_t child_cursor = 0;

        for (size_t node_index = 0;
             node_index < patch_level->count;
             node_index++)
        {
            ii42_term_cow_patch_node *patch_node =
                &patch_level->nodes[node_index];
            ii42_term_cow_object object;
            size_t written_bytes = 0;

            if (level == 0)
            {
                while (child_cursor < builder->leaf_count &&
                       builder->leaves[child_cursor].leaf_index >>
                           II42_TERM_COW_RADIX_BITS ==
                           patch_node->prefix)
                {
                    status = ii42_term_cow_node_set_child(
                        &patch_node->node,
                        (uint16_t) (
                            builder->leaves[child_cursor].leaf_index &
                            (II42_TERM_COW_RADIX_FANOUT - 1)
                        ),
                        builder->leaves[
                            child_cursor
                        ].max_extent_count,
                        &builder->leaves[child_cursor].ref
                    );
                    if (status != II42_OK)
                    {
                        return status;
                    }
                    child_cursor++;
                }
            }
            else
            {
                ii42_term_cow_patch_level *child_level =
                    &builder->levels[level - 1];

                while (child_cursor < child_level->count &&
                       child_level->nodes[child_cursor].prefix >>
                           II42_TERM_COW_RADIX_BITS ==
                           patch_node->prefix)
                {
                    status = ii42_term_cow_node_set_child(
                        &patch_node->node,
                        (uint16_t) (
                            child_level->nodes[child_cursor].prefix &
                            (II42_TERM_COW_RADIX_FANOUT - 1)
                        ),
                        child_level->nodes[
                            child_cursor
                        ].max_extent_count,
                        &child_level->nodes[child_cursor].ref
                    );
                    if (status != II42_OK)
                    {
                        return status;
                    }
                    child_cursor++;
                }
            }
            memset(&object, 0, sizeof(object));
            if (patch_node->has_source)
            {
                status = ii42_term_cow_tree_add_retired_ref(
                    patch,
                    &patch_node->source_ref
                );
                if (status != II42_OK)
                {
                    return status;
                }
            }
            object.ref.kind = II42_TERM_COW_OBJECT_NODE;
            patch_node->max_extent_count =
                ii42_term_cow_node_max_extent_count(
                    &patch_node->node
                );
            object.value.node = patch_node->node;
            status = ii42_term_cow_tree_append_object(
                patch,
                &object,
                &patch_node->ref,
                &written_bytes
            );
            if (status != II42_OK)
            {
                return status;
            }
            stats->written_nodes++;
            stats->written_bytes += written_bytes;
        }
        if ((level == 0 && child_cursor != builder->leaf_count) ||
            (level > 0 &&
             child_cursor != builder->levels[level - 1].count))
        {
            return II42_ERR_FORMAT;
        }
    }
    if (builder->levels[II42_TERM_COW_RADIX_LEVELS - 1].count != 1 ||
        builder->levels[II42_TERM_COW_RADIX_LEVELS - 1].
            nodes[0].prefix != 0)
    {
        return II42_ERR_FORMAT;
    }
    patch->root =
        builder->levels[II42_TERM_COW_RADIX_LEVELS - 1].nodes[0].ref;
    return II42_OK;
}

ii42_status
ii42_term_cow_build_external_append_patch(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_payload_view *new_payload,
    const ii42_segment_object_ref *lexical_catalog,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_tree *patch_out,
    ii42_term_cow_update_stats *stats_out
)
{
    ii42_term_cow_patch_builder builder;
    ii42_term_cow_tree patch;
    ii42_term_cow_update_stats stats = {0};
    uint64_t *leaf_indices = NULL;
    size_t leaf_count = 0;
    uint32_t new_segment_index;
    uint32_t run_index = 0;
    bool old_has_lexical_catalog = false;
    ii42_status status;

    if (old_root == NULL || old_manifest == NULL ||
        next_manifest == NULL || new_payload == NULL ||
        loader == NULL || patch_out == NULL || stats_out == NULL ||
        !ii42_term_cow_ref_valid(old_root) ||
        ii42_term_cow_ref_is_unbound(old_root) ||
        old_root->kind != II42_TERM_COW_OBJECT_NODE ||
        next_manifest->vocab_size < old_manifest->vocab_size ||
        next_manifest->segment_count != old_manifest->segment_count + 1)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_segment_manifest_validate(old_manifest);
    if (status != II42_OK)
    {
        return status;
    }
    if (lexical_catalog != NULL &&
        (next_manifest->vocab_size == old_manifest->vocab_size ||
         !ii42_term_cow_object_ref_valid(
             lexical_catalog,
             II42_SEGMENT_OBJECT_LEXICAL_CATALOG
         ) ||
         lexical_catalog->owner_manifest_id !=
             next_manifest->manifest_id))
    {
        return II42_ERR_FORMAT;
    }
    if (next_manifest->vocab_size > old_manifest->vocab_size &&
        old_manifest->vocab_size > 0)
    {
        ii42_term_cow_record first_record;

        status = ii42_term_cow_lookup_external(
            old_root,
            old_manifest->vocab_size,
            0,
            loader,
            loader_context,
            &first_record
        );
        if (status != II42_OK)
        {
            return status;
        }
        old_has_lexical_catalog =
            !ii42_term_cow_object_ref_absent(
                &first_record.lexical_catalog
            );
        if (old_has_lexical_catalog != (lexical_catalog != NULL))
        {
            return II42_ERR_FORMAT;
        }
    }
    for (new_segment_index = 0;
         new_segment_index < old_manifest->segment_count;
         new_segment_index++)
    {
        if (!ii42_segment_descriptor_equal(
                &old_manifest->segments[new_segment_index],
                &next_manifest->segments[new_segment_index]))
        {
            return II42_ERR_FORMAT;
        }
    }
    new_segment_index = old_manifest->segment_count;
    status = ii42_segment_posting_payload_validate(
        next_manifest,
        &next_manifest->segments[new_segment_index],
        new_payload
    );
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_term_cow_collect_append_leaves(
        old_manifest->vocab_size,
        next_manifest->vocab_size,
        new_payload,
        &leaf_indices,
        &leaf_count
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (leaf_count > UINT32_MAX)
    {
        free(leaf_indices);
        return II42_ERR_RANGE;
    }

    memset(&builder, 0, sizeof(builder));
    ii42_term_cow_tree_init(&patch);
    builder.old_root = old_root;
    builder.old_vocab_size = old_manifest->vocab_size;
    builder.next_vocab_size = next_manifest->vocab_size;
    builder.loader = loader;
    builder.loader_context = loader_context;
    builder.leaf_count = leaf_count;
    builder.leaves = calloc(leaf_count, sizeof(*builder.leaves));
    if (builder.leaves == NULL)
    {
        free(leaf_indices);
        return II42_ERR_NOMEM;
    }
    patch.vocab_size = next_manifest->vocab_size;
    patch.root = *old_root;

    if (leaf_count == 0)
    {
        free(leaf_indices);
        ii42_term_cow_tree_free(patch_out);
        *patch_out = patch;
        memset(stats_out, 0, sizeof(*stats_out));
        return II42_OK;
    }

    for (size_t leaf_index = 0; leaf_index < leaf_count; leaf_index++)
    {
        status = ii42_term_cow_patch_load_leaf(
            &builder,
            leaf_indices[leaf_index],
            &builder.leaves[leaf_index]
        );
        if (status != II42_OK)
        {
            goto fail;
        }
    }
    if (lexical_catalog != NULL)
    {
        for (size_t leaf_index = 0;
             leaf_index < builder.leaf_count;
             leaf_index++)
        {
            ii42_term_cow_leaf *leaf =
                &builder.leaves[leaf_index].leaf;

            for (uint32_t record_index = 0;
                 record_index < leaf->record_count;
                 record_index++)
            {
                ii42_term_cow_record *record =
                    &leaf->records[record_index];

                if (record->term_id >= old_manifest->vocab_size)
                {
                    record->lexical_catalog = *lexical_catalog;
                }
            }
        }
    }
    while (run_index < new_payload->run_count)
    {
        const ii42_segment_term_run *run =
            &new_payload->runs[run_index];
        uint64_t leaf_index =
            run->term_id / II42_TERM_COW_LEAF_TERMS;
        size_t patch_leaf_index = 0;
        ii42_term_cow_record *record;

        while (patch_leaf_index < builder.leaf_count &&
               builder.leaves[patch_leaf_index].leaf_index < leaf_index)
        {
            patch_leaf_index++;
        }
        if (patch_leaf_index == builder.leaf_count ||
            builder.leaves[patch_leaf_index].leaf_index != leaf_index)
        {
            status = II42_ERR_FORMAT;
            goto fail;
        }
        record = &builder.leaves[patch_leaf_index].leaf.records[
            run->term_id % II42_TERM_COW_LEAF_TERMS
        ];
        if (run_index == 0 ||
            new_payload->runs[run_index - 1].term_id != run->term_id)
        {
            stats.changed_terms++;
        }
        status = ii42_term_cow_record_add_extent(
            record,
            next_manifest->segments[new_segment_index].segment_id,
            run
        );
        if (status != II42_OK)
        {
            goto fail;
        }
        run_index++;
    }
    stats.changed_leaves = (uint32_t) builder.leaf_count;
    status = ii42_term_cow_patch_append_objects(
        &builder,
        &patch,
        &stats
    );
    if (status != II42_OK)
    {
        goto fail;
    }

    free(leaf_indices);
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(patch_out);
    *patch_out = patch;
    *stats_out = stats;
    return II42_OK;

fail:
    free(leaf_indices);
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(&patch);
    memset(stats_out, 0, sizeof(*stats_out));
    return status;
}

ii42_status
ii42_term_cow_build_external_replace_patch(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload_view *replaced_payloads,
    uint32_t replaced_payload_count,
    const ii42_segment_payload_view *replacement_payload,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_tree *patch_out,
    ii42_term_cow_update_stats *stats_out
)
{
    ii42_term_cow_patch_builder builder;
    ii42_term_cow_tree patch;
    ii42_term_cow_update_stats stats = {0};
    uint32_t *term_ids = NULL;
    uint64_t *leaf_indices = NULL;
    size_t term_count = 0;
    size_t leaf_count = 0;
    uint64_t expected_removed_extents = 0;
    uint64_t expected_removed_lexical = 0;
    uint64_t removed_extents = 0;
    uint64_t removed_lexical = 0;
    uint32_t replacement_run_index = 0;
    uint32_t replaced_end;
    ii42_status status;

    if (old_root == NULL || old_manifest == NULL ||
        next_manifest == NULL || replaced_payloads == NULL ||
        replacement_payload == NULL || loader == NULL ||
        patch_out == NULL || stats_out == NULL ||
        !ii42_term_cow_ref_valid(old_root) ||
        ii42_term_cow_ref_is_unbound(old_root) ||
        old_root->kind != II42_TERM_COW_OBJECT_NODE ||
        replaced_payload_count < 2 ||
        first_segment_index >= old_manifest->segment_count ||
        replaced_payload_count >
            old_manifest->segment_count - first_segment_index ||
        next_manifest->segment_count !=
            old_manifest->segment_count - replaced_payload_count + 1 ||
        next_manifest->vocab_size != old_manifest->vocab_size)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_segment_manifest_validate(old_manifest);
    if (status != II42_OK)
    {
        return status;
    }
    replaced_end = first_segment_index + replaced_payload_count;
    for (uint32_t segment_index = 0;
         segment_index < first_segment_index;
         segment_index++)
    {
        if (!ii42_segment_descriptor_equal(
                &old_manifest->segments[segment_index],
                &next_manifest->segments[segment_index]))
        {
            return II42_ERR_FORMAT;
        }
    }
    for (uint32_t segment_index = replaced_end;
         segment_index < old_manifest->segment_count;
         segment_index++)
    {
        uint32_t next_segment_index =
            segment_index - replaced_payload_count + 1;

        if (!ii42_segment_descriptor_equal(
                &old_manifest->segments[segment_index],
                &next_manifest->segments[next_segment_index]))
        {
            return II42_ERR_FORMAT;
        }
    }
    for (uint32_t payload_index = 0;
         payload_index < replaced_payload_count;
         payload_index++)
    {
        const ii42_segment_payload_view *payload =
            &replaced_payloads[payload_index];

        status = ii42_segment_posting_payload_validate(
            old_manifest,
            &old_manifest->segments[
                first_segment_index + payload_index
            ],
            payload
        );
        if (status != II42_OK)
        {
            return status;
        }
    }
    status = ii42_segment_posting_payload_validate(
        next_manifest,
        &next_manifest->segments[first_segment_index],
        replacement_payload
    );
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_term_cow_collect_replace_terms(
        replaced_payloads,
        replaced_payload_count,
        replacement_payload,
        &term_ids,
        &term_count,
        &leaf_indices,
        &leaf_count
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (term_count > UINT32_MAX || leaf_count > UINT32_MAX)
    {
        status = II42_ERR_RANGE;
        goto fail_lists;
    }

    memset(&builder, 0, sizeof(builder));
    ii42_term_cow_tree_init(&patch);
    builder.old_root = old_root;
    builder.old_vocab_size = old_manifest->vocab_size;
    builder.next_vocab_size = next_manifest->vocab_size;
    builder.loader = loader;
    builder.loader_context = loader_context;
    builder.leaf_count = leaf_count;
    builder.leaves = calloc(leaf_count, sizeof(*builder.leaves));
    if (leaf_count > 0 && builder.leaves == NULL)
    {
        status = II42_ERR_NOMEM;
        goto fail_lists;
    }
    patch.vocab_size = next_manifest->vocab_size;
    patch.root = *old_root;

    if (term_count == 0)
    {
        free(leaf_indices);
        free(term_ids);
        ii42_term_cow_tree_free(patch_out);
        *patch_out = patch;
        memset(stats_out, 0, sizeof(*stats_out));
        return II42_OK;
    }
    for (size_t leaf_index = 0;
         leaf_index < leaf_count;
         leaf_index++)
    {
        status = ii42_term_cow_patch_load_leaf(
            &builder,
            leaf_indices[leaf_index],
            &builder.leaves[leaf_index]
        );
        if (status != II42_OK)
        {
            goto fail_builder;
        }
    }
    for (size_t term_index = 0;
         term_index < term_count;
         term_index++)
    {
        uint32_t term_id = term_ids[term_index];
        uint64_t leaf_index =
            term_id / II42_TERM_COW_LEAF_TERMS;
        size_t patch_leaf_index = 0;
        ii42_term_cow_record *record;
        ii42_term_cow_replace_fold_relation fold_relation;

        while (patch_leaf_index < builder.leaf_count &&
               builder.leaves[patch_leaf_index].leaf_index < leaf_index)
        {
            patch_leaf_index++;
        }
        if (patch_leaf_index == builder.leaf_count ||
            builder.leaves[patch_leaf_index].leaf_index != leaf_index)
        {
            status = II42_ERR_FORMAT;
            goto fail_builder;
        }
        record = &builder.leaves[patch_leaf_index].leaf.records[
            term_id % II42_TERM_COW_LEAF_TERMS
        ];
        status = ii42_term_cow_classify_replace_fold_relation(
            record,
            old_manifest,
            first_segment_index,
            replaced_payloads,
            replaced_payload_count,
            &fold_relation
        );
        if (status != II42_OK)
        {
            goto fail_builder;
        }
        if (fold_relation == II42_TERM_COW_REPLACE_STRADDLE)
        {
            status = II42_ERR_FORMAT;
            goto fail_builder;
        }
        if (fold_relation == II42_TERM_COW_REPLACE_TAIL)
        {
            for (uint32_t payload_index = 0;
                 payload_index < replaced_payload_count;
                 payload_index++)
            {
                const ii42_segment_payload_view *payload =
                    &replaced_payloads[payload_index];

                for (uint32_t run_index = 0;
                     run_index < payload->run_count;
                     run_index++)
                {
                    const ii42_segment_term_run *run =
                        &payload->runs[run_index];

                    if (run->term_id != term_id)
                    {
                        continue;
                    }
                    if (expected_removed_extents == UINT64_MAX)
                    {
                        status = II42_ERR_RANGE;
                        goto fail_builder;
                    }
                    expected_removed_extents++;
                    if (run->kind ==
                            II42_POSTING_EXTENT_LEXICAL_NEUTRAL)
                    {
                        if (expected_removed_lexical >
                            UINT64_MAX - run->posting_count)
                        {
                            status = II42_ERR_RANGE;
                            goto fail_builder;
                        }
                        expected_removed_lexical +=
                            run->posting_count;
                    }
                }
            }
            status = ii42_term_cow_record_remove_replaced_extents(
                record,
                old_manifest,
                first_segment_index,
                replaced_payload_count,
                &removed_extents,
                &removed_lexical
            );
            if (status != II42_OK)
            {
                goto fail_builder;
            }
        }
        while (replacement_run_index <
                   replacement_payload->run_count &&
               replacement_payload->runs[
                   replacement_run_index
               ].term_id == term_id)
        {
            if (fold_relation == II42_TERM_COW_REPLACE_TAIL)
            {
                status = ii42_term_cow_record_add_extent(
                    record,
                    next_manifest->segments[
                        first_segment_index
                    ].segment_id,
                    &replacement_payload->runs[
                        replacement_run_index
                    ]
                );
                if (status != II42_OK)
                {
                    goto fail_builder;
                }
            }
            replacement_run_index++;
        }
    }
    if (removed_extents != expected_removed_extents ||
        removed_lexical != expected_removed_lexical ||
        replacement_run_index != replacement_payload->run_count)
    {
        status = II42_ERR_FORMAT;
        goto fail_builder;
    }
    stats.changed_terms = (uint32_t) term_count;
    stats.changed_leaves = (uint32_t) leaf_count;
    status = ii42_term_cow_patch_append_objects(
        &builder,
        &patch,
        &stats
    );
    if (status != II42_OK)
    {
        goto fail_builder;
    }

    free(leaf_indices);
    free(term_ids);
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(patch_out);
    *patch_out = patch;
    *stats_out = stats;
    return II42_OK;

fail_builder:
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(&patch);
fail_lists:
    free(leaf_indices);
    free(term_ids);
    memset(stats_out, 0, sizeof(*stats_out));
    return status;
}

ii42_status
ii42_term_cow_replace_fold_preflight_external(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload_view *replaced_payloads,
    uint32_t replaced_payload_count,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    bool *safe_out,
    uint32_t *conflict_term_id_out
)
{
    ii42_term_cow_patch_builder builder;
    ii42_segment_payload_view empty_replacement = {0};
    uint32_t *term_ids = NULL;
    uint64_t *leaf_indices = NULL;
    size_t term_count = 0;
    size_t leaf_count = 0;
    ii42_status status;

    if (old_root == NULL || manifest == NULL ||
        replaced_payloads == NULL || loader == NULL ||
        safe_out == NULL || conflict_term_id_out == NULL ||
        !ii42_term_cow_ref_valid(old_root) ||
        ii42_term_cow_ref_is_unbound(old_root) ||
        old_root->kind != II42_TERM_COW_OBJECT_NODE ||
        replaced_payload_count < 2 ||
        first_segment_index >= manifest->segment_count ||
        replaced_payload_count >
            manifest->segment_count - first_segment_index)
    {
        return II42_ERR_INVALID;
    }
    *safe_out = false;
    *conflict_term_id_out = UINT32_MAX;
    status = ii42_segment_manifest_validate(manifest);
    if (status != II42_OK)
    {
        return status;
    }
    for (uint32_t payload_index = 0;
         payload_index < replaced_payload_count;
         payload_index++)
    {
        status = ii42_segment_posting_payload_validate(
            manifest,
            &manifest->segments[first_segment_index + payload_index],
            &replaced_payloads[payload_index]
        );
        if (status != II42_OK)
        {
            return status;
        }
    }
    status = ii42_term_cow_collect_replace_terms(
        replaced_payloads,
        replaced_payload_count,
        &empty_replacement,
        &term_ids,
        &term_count,
        &leaf_indices,
        &leaf_count
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (term_count > UINT32_MAX || leaf_count > UINT32_MAX)
    {
        status = II42_ERR_RANGE;
        goto done;
    }

    memset(&builder, 0, sizeof(builder));
    builder.old_root = old_root;
    builder.old_vocab_size = manifest->vocab_size;
    builder.next_vocab_size = manifest->vocab_size;
    builder.loader = loader;
    builder.loader_context = loader_context;
    builder.leaf_count = leaf_count;
    builder.leaves = calloc(leaf_count, sizeof(*builder.leaves));
    if (leaf_count > 0 && builder.leaves == NULL)
    {
        status = II42_ERR_NOMEM;
        goto done;
    }
    for (size_t leaf_index = 0;
         leaf_index < leaf_count;
         leaf_index++)
    {
        status = ii42_term_cow_patch_load_leaf(
            &builder,
            leaf_indices[leaf_index],
            &builder.leaves[leaf_index]
        );
        if (status != II42_OK)
        {
            goto done_builder;
        }
    }
    for (size_t term_index = 0;
         term_index < term_count;
         term_index++)
    {
        uint32_t term_id = term_ids[term_index];
        uint64_t leaf_index =
            term_id / II42_TERM_COW_LEAF_TERMS;
        size_t patch_leaf_index = 0;
        ii42_term_cow_record *record;
        ii42_term_cow_replace_fold_relation relation;

        while (patch_leaf_index < builder.leaf_count &&
               builder.leaves[patch_leaf_index].leaf_index < leaf_index)
        {
            patch_leaf_index++;
        }
        if (patch_leaf_index == builder.leaf_count ||
            builder.leaves[patch_leaf_index].leaf_index != leaf_index)
        {
            status = II42_ERR_FORMAT;
            goto done_builder;
        }
        record = &builder.leaves[patch_leaf_index].leaf.records[
            term_id % II42_TERM_COW_LEAF_TERMS
        ];
        status = ii42_term_cow_classify_replace_fold_relation(
            record,
            manifest,
            first_segment_index,
            replaced_payloads,
            replaced_payload_count,
            &relation
        );
        if (status != II42_OK)
        {
            goto done_builder;
        }
        if (relation == II42_TERM_COW_REPLACE_STRADDLE)
        {
            *conflict_term_id_out = term_id;
            status = II42_OK;
            goto done_builder;
        }
    }
    *safe_out = true;
    status = II42_OK;

done_builder:
    ii42_term_cow_patch_builder_free(&builder);
done:
    free(leaf_indices);
    free(term_ids);
    return status;
}

ii42_status
ii42_term_cow_build_external_neutral_fold_patch(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *manifest,
    uint64_t next_owner_manifest_id,
    uint32_t term_id,
    const ii42_segment_object_ref *fold_ref,
    uint64_t coverage_sequence,
    ii42_term_cow_neutral_fold_level fold_level,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_tree *patch_out,
    ii42_term_cow_update_stats *stats_out
)
{
    ii42_term_cow_patch_builder builder;
    ii42_term_cow_tree patch;
    ii42_term_cow_update_stats stats = {0};
    ii42_term_cow_record *record;
    uint64_t leaf_index;
    uint64_t prior_coverage;
    uint32_t retained_extent_count = 0;
    bool is_manifest_boundary = false;
    bool consumed_extent = false;
    bool promoting_existing_minor = false;
    ii42_status status;

    if (old_root == NULL || manifest == NULL || fold_ref == NULL ||
        loader == NULL || patch_out == NULL || stats_out == NULL ||
        !ii42_term_cow_ref_valid(old_root) ||
        ii42_term_cow_ref_is_unbound(old_root) ||
        old_root->kind != II42_TERM_COW_OBJECT_NODE ||
        term_id >= manifest->vocab_size ||
        next_owner_manifest_id <= manifest->manifest_id ||
        fold_ref->owner_manifest_id != next_owner_manifest_id ||
        !ii42_term_cow_object_ref_valid(
            fold_ref,
            II42_SEGMENT_OBJECT_NEUTRAL_FOLD
        ) ||
        coverage_sequence == 0 ||
        coverage_sequence > manifest->max_sequence ||
        (fold_level != II42_TERM_COW_NEUTRAL_FOLD_MAJOR &&
         fold_level != II42_TERM_COW_NEUTRAL_FOLD_MINOR))
    {
        return II42_ERR_INVALID;
    }
    status = ii42_segment_manifest_validate(manifest);
    if (status != II42_OK)
    {
        return status;
    }
    for (uint32_t segment_index = 0;
         segment_index < manifest->segment_count;
         segment_index++)
    {
        if (manifest->segments[segment_index].max_sequence ==
            coverage_sequence)
        {
            is_manifest_boundary = true;
            break;
        }
    }
    memset(&builder, 0, sizeof(builder));
    ii42_term_cow_tree_init(&patch);
    builder.old_root = old_root;
    builder.old_vocab_size = manifest->vocab_size;
    builder.next_vocab_size = manifest->vocab_size;
    builder.loader = loader;
    builder.loader_context = loader_context;
    builder.leaf_count = 1;
    builder.leaves = calloc(1, sizeof(*builder.leaves));
    if (builder.leaves == NULL)
    {
        return II42_ERR_NOMEM;
    }
    patch.vocab_size = manifest->vocab_size;
    patch.root = *old_root;
    leaf_index = term_id / II42_TERM_COW_LEAF_TERMS;
    status = ii42_term_cow_patch_load_leaf(
        &builder,
        leaf_index,
        &builder.leaves[0]
    );
    if (status != II42_OK)
    {
        goto fail;
    }
    record = &builder.leaves[0].leaf.records[
        term_id % II42_TERM_COW_LEAF_TERMS
    ];
    prior_coverage =
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0
            ? record->neutral_minor_fold_coverage
            : record->neutral_fold_coverage;
    promoting_existing_minor =
        fold_level == II42_TERM_COW_NEUTRAL_FOLD_MAJOR &&
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0 &&
        coverage_sequence == prior_coverage;
    if (record->term_id != term_id ||
        (!is_manifest_boundary && !promoting_existing_minor) ||
        coverage_sequence < prior_coverage ||
        (coverage_sequence == prior_coverage &&
         !promoting_existing_minor) ||
        (fold_level == II42_TERM_COW_NEUTRAL_FOLD_MINOR &&
         (record->flags &
          II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD) == 0))
    {
        status = II42_ERR_FORMAT;
        goto fail;
    }
    for (uint32_t extent_index = 0;
         extent_index < record->extent_count;
         extent_index++)
    {
        const ii42_stable_term_extent *extent =
            &record->extents[extent_index];
        uint32_t segment_index;
        const ii42_segment_descriptor *segment;

        if (!ii42_term_cow_manifest_segment_index(
                manifest,
                extent->segment_id,
                &segment_index
            ))
        {
            status = II42_ERR_FORMAT;
            goto fail;
        }
        segment = &manifest->segments[segment_index];
        if (segment->max_sequence <= coverage_sequence)
        {
            consumed_extent = true;
            continue;
        }
        if (segment->min_sequence <= coverage_sequence)
        {
            status = II42_ERR_FORMAT;
            goto fail;
        }
        if (retained_extent_count != extent_index)
        {
            record->extents[retained_extent_count] = *extent;
        }
        retained_extent_count++;
    }
    if (!consumed_extent && !promoting_existing_minor)
    {
        status = II42_ERR_FORMAT;
        goto fail;
    }
    memset(
        &record->extents[retained_extent_count],
        0,
        (size_t) (record->extent_count - retained_extent_count) *
            sizeof(*record->extents)
    );
    record->extent_count = retained_extent_count;

    if (fold_level == II42_TERM_COW_NEUTRAL_FOLD_MAJOR)
    {
        record->flags |= II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD;
        record->flags &=
            ~II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD;
        record->neutral_fold_coverage = coverage_sequence;
        record->neutral_fold = *fold_ref;
        record->neutral_minor_fold_coverage = 0;
        memset(
            &record->neutral_minor_fold,
            0,
            sizeof(record->neutral_minor_fold)
        );
    }
    else
    {
        record->flags |=
            II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD;
        record->neutral_minor_fold_coverage = coverage_sequence;
        record->neutral_minor_fold = *fold_ref;
    }
    status = ii42_term_cow_record_validate(record);
    if (status != II42_OK)
    {
        goto fail;
    }
    stats.changed_terms = 1;
    stats.changed_leaves = 1;
    status = ii42_term_cow_patch_append_objects(
        &builder,
        &patch,
        &stats
    );
    if (status != II42_OK)
    {
        goto fail;
    }

    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(patch_out);
    *patch_out = patch;
    *stats_out = stats;
    return II42_OK;

fail:
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(&patch);
    memset(stats_out, 0, sizeof(*stats_out));
    return status;
}

ii42_status
ii42_term_cow_build_external_impact_fold_patch(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *manifest,
    uint64_t next_owner_manifest_id,
    uint32_t term_id,
    const ii42_segment_object_ref *fold_ref,
    uint64_t coverage_sequence,
    uint64_t statistics_epoch,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_tree *patch_out,
    ii42_term_cow_update_stats *stats_out
)
{
    ii42_term_cow_patch_builder builder;
    ii42_term_cow_tree patch;
    ii42_term_cow_update_stats stats = {0};
    ii42_term_cow_record *record;
    uint64_t effective_coverage;
    uint64_t leaf_index;
    ii42_status status;

    if (old_root == NULL || manifest == NULL || fold_ref == NULL ||
        loader == NULL || patch_out == NULL || stats_out == NULL ||
        !ii42_term_cow_ref_valid(old_root) ||
        ii42_term_cow_ref_is_unbound(old_root) ||
        old_root->kind != II42_TERM_COW_OBJECT_NODE ||
        term_id >= manifest->vocab_size ||
        next_owner_manifest_id <= manifest->manifest_id ||
        fold_ref->owner_manifest_id != next_owner_manifest_id ||
        !ii42_term_cow_object_ref_valid(
            fold_ref,
            II42_SEGMENT_OBJECT_IMPACT_FOLD
        ) ||
        coverage_sequence == 0 ||
        coverage_sequence > manifest->max_sequence ||
        statistics_epoch == 0 ||
        statistics_epoch != manifest->statistics_epoch)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_segment_manifest_validate(manifest);
    if (status != II42_OK)
    {
        return status;
    }

    memset(&builder, 0, sizeof(builder));
    ii42_term_cow_tree_init(&patch);
    builder.old_root = old_root;
    builder.old_vocab_size = manifest->vocab_size;
    builder.next_vocab_size = manifest->vocab_size;
    builder.loader = loader;
    builder.loader_context = loader_context;
    builder.leaf_count = 1;
    builder.leaves = calloc(1, sizeof(*builder.leaves));
    if (builder.leaves == NULL)
    {
        return II42_ERR_NOMEM;
    }
    patch.vocab_size = manifest->vocab_size;
    patch.root = *old_root;
    leaf_index = term_id / II42_TERM_COW_LEAF_TERMS;
    status = ii42_term_cow_patch_load_leaf(
        &builder,
        leaf_index,
        &builder.leaves[0]
    );
    if (status != II42_OK)
    {
        goto fail;
    }
    record = &builder.leaves[0].leaf.records[
        term_id % II42_TERM_COW_LEAF_TERMS
    ];
    effective_coverage =
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD) != 0
            ? record->neutral_minor_fold_coverage
            : record->neutral_fold_coverage;
    if (record->term_id != term_id ||
        (record->flags &
         II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD) == 0 ||
        coverage_sequence != effective_coverage)
    {
        status = II42_ERR_FORMAT;
        goto fail;
    }

    record->flags |= II42_TERM_COW_RECORD_FLAG_IMPACT_FOLD;
    record->impact_fold_coverage = coverage_sequence;
    record->impact_statistics_epoch = statistics_epoch;
    record->impact_fold = *fold_ref;
    status = ii42_term_cow_record_validate(record);
    if (status != II42_OK)
    {
        goto fail;
    }
    stats.changed_terms = 1;
    stats.changed_leaves = 1;
    status = ii42_term_cow_patch_append_objects(
        &builder,
        &patch,
        &stats
    );
    if (status != II42_OK)
    {
        goto fail;
    }

    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(patch_out);
    *patch_out = patch;
    *stats_out = stats;
    return II42_OK;

fail:
    ii42_term_cow_patch_builder_free(&builder);
    ii42_term_cow_tree_free(&patch);
    memset(stats_out, 0, sizeof(*stats_out));
    return status;
}

static ii42_status
ii42_term_cow_tree_replace_leaf(
    ii42_term_cow_tree *tree,
    uint64_t leaf_index,
    const ii42_term_cow_ref *leaf_ref,
    uint32_t leaf_max_extent_count,
    ii42_term_cow_update_stats *stats
)
{
    ii42_term_cow_node path[II42_TERM_COW_RADIX_LEVELS];
    bool path_present[II42_TERM_COW_RADIX_LEVELS] = {false};
    ii42_term_cow_ref current = tree->root;
    ii42_term_cow_ref child_ref = *leaf_ref;
    uint32_t child_max_extent_count = leaf_max_extent_count;
    ii42_status status;

    for (int level = II42_TERM_COW_RADIX_LEVELS - 1;
         level >= 0;
         level--)
    {
        const ii42_term_cow_object *object =
            ii42_term_cow_tree_find_object(tree, &current);
        const ii42_term_cow_child *child;

        if (object == NULL ||
            object->ref.kind != II42_TERM_COW_OBJECT_NODE ||
            object->value.node.level != (uint16_t) level)
        {
            return II42_ERR_FORMAT;
        }
        path[level] = object->value.node;
        path_present[level] = true;
        child = ii42_term_cow_node_child(
            &object->value.node,
            ii42_term_cow_path_slot(
                leaf_index,
                (uint16_t) level)
        );
        if (child == NULL)
        {
            break;
        }
        current = child->ref;
    }

    for (uint16_t level = 0;
         level < II42_TERM_COW_RADIX_LEVELS;
         level++)
    {
        ii42_term_cow_object object;
        size_t written_bytes = 0;

        memset(&object, 0, sizeof(object));
        object.ref.kind = II42_TERM_COW_OBJECT_NODE;
        if (path_present[level])
        {
            object.value.node = path[level];
        }
        else
        {
            object.value.node.level = level;
            object.value.node.prefix =
                ii42_term_cow_path_prefix(leaf_index, level);
        }
        status = ii42_term_cow_node_set_child(
            &object.value.node,
            ii42_term_cow_path_slot(leaf_index, level),
            child_max_extent_count,
            &child_ref
        );
        if (status != II42_OK)
        {
            return status;
        }
        child_max_extent_count =
            ii42_term_cow_node_max_extent_count(
                &object.value.node
            );
        status = ii42_term_cow_tree_append_object(
            tree,
            &object,
            &child_ref,
            &written_bytes
        );
        if (status != II42_OK)
        {
            return status;
        }
        stats->written_nodes++;
        stats->written_bytes += written_bytes;
    }
    tree->root = child_ref;
    return II42_OK;
}

static ii42_status
ii42_term_cow_record_add_extent(
    ii42_term_cow_record *record,
    uint64_t segment_id,
    const ii42_segment_term_run *run
)
{
    ii42_stable_term_extent extent;
    uint32_t position = 0;

    if (record == NULL || segment_id == 0 || run == NULL ||
        record->extent_count >=
            II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM)
    {
        return II42_ERR_RANGE;
    }
    if (run->kind == II42_POSTING_EXTENT_LEXICAL_NEUTRAL &&
        (run->posting_count > UINT32_MAX ||
         record->raw_document_frequency >
             UINT32_MAX - (uint32_t) run->posting_count))
    {
        return II42_ERR_RANGE;
    }
    memset(&extent, 0, sizeof(extent));
    extent.segment_id = segment_id;
    extent.kind = run->kind;
    extent.posting_offset = run->posting_offset;
    extent.posting_count = run->posting_count;
    while (position < record->extent_count &&
           (record->extents[position].segment_id < segment_id ||
            (record->extents[position].segment_id == segment_id &&
             record->extents[position].posting_offset <
                 run->posting_offset)))
    {
        position++;
    }
    if (position < record->extent_count &&
        record->extents[position].segment_id == segment_id &&
        record->extents[position].posting_offset ==
            run->posting_offset)
    {
        return II42_ERR_FORMAT;
    }
    memmove(
        &record->extents[position + 1],
        &record->extents[position],
        (size_t) (record->extent_count - position) *
            sizeof(*record->extents)
    );
    record->extents[position] = extent;
    record->extent_count++;
    if (run->kind == II42_POSTING_EXTENT_LEXICAL_NEUTRAL)
    {
        record->raw_document_frequency += (uint32_t) run->posting_count;
    }
    return ii42_term_cow_record_validate(record);
}

ii42_status
ii42_term_cow_tree_append_payload(
    ii42_term_cow_tree *tree,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_payload_view *new_payload,
    ii42_term_cow_update_stats *stats_out
)
{
    ii42_term_cow_update_stats stats = {0};
    size_t saved_object_count;
    uint64_t saved_next_object_id;
    ii42_term_cow_ref saved_root;
    uint32_t saved_vocab_size;
    uint32_t new_segment_index;
    uint32_t run_index = 0;
    ii42_status status;

    if (tree == NULL || old_manifest == NULL ||
        next_manifest == NULL || new_payload == NULL ||
        stats_out == NULL ||
        old_manifest->vocab_size != tree->vocab_size ||
        next_manifest->vocab_size < old_manifest->vocab_size ||
        next_manifest->segment_count != old_manifest->segment_count + 1)
    {
        return II42_ERR_INVALID;
    }
    status = ii42_segment_manifest_validate(old_manifest);
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_segment_manifest_validate(next_manifest);
    if (status != II42_OK)
    {
        return status;
    }
    for (new_segment_index = 0;
         new_segment_index < old_manifest->segment_count;
         new_segment_index++)
    {
        if (!ii42_segment_descriptor_equal(
                &old_manifest->segments[new_segment_index],
                &next_manifest->segments[new_segment_index]))
        {
            return II42_ERR_FORMAT;
        }
    }
    new_segment_index = old_manifest->segment_count;
    status = ii42_segment_posting_payload_validate(
        next_manifest,
        &next_manifest->segments[new_segment_index],
        new_payload
    );
    if (status != II42_OK)
    {
        return status;
    }

    saved_object_count = tree->object_count;
    saved_next_object_id = tree->next_object_id;
    saved_root = tree->root;
    saved_vocab_size = tree->vocab_size;
    tree->vocab_size = next_manifest->vocab_size;

    while (run_index < new_payload->run_count)
    {
        uint32_t first_run = run_index;
        uint32_t leaf_base =
            new_payload->runs[run_index].term_id /
            II42_TERM_COW_LEAF_TERMS *
            II42_TERM_COW_LEAF_TERMS;
        uint64_t leaf_index =
            leaf_base / II42_TERM_COW_LEAF_TERMS;
        const ii42_term_cow_leaf *old_leaf = NULL;
        ii42_term_cow_ref old_leaf_ref;
        ii42_term_cow_object leaf_object;
        ii42_term_cow_ref new_leaf_ref;
        size_t written_bytes = 0;

        status = ii42_term_cow_tree_find_leaf(
            tree,
            &tree->root,
            leaf_index,
            &old_leaf_ref,
            &old_leaf
        );
        if (status != II42_OK)
        {
            goto rollback;
        }
        memset(&leaf_object, 0, sizeof(leaf_object));
        leaf_object.ref.kind = II42_TERM_COW_OBJECT_LEAF;
        if (old_leaf != NULL)
        {
            leaf_object.value.leaf = *old_leaf;
        }
        else
        {
            leaf_object.value.leaf.base_term_id = leaf_base;
        }
        leaf_object.value.leaf.record_count =
            next_manifest->vocab_size - leaf_base;
        if (leaf_object.value.leaf.record_count >
            II42_TERM_COW_LEAF_TERMS)
        {
            leaf_object.value.leaf.record_count =
                II42_TERM_COW_LEAF_TERMS;
        }
        for (uint32_t record_index = 0;
             record_index < leaf_object.value.leaf.record_count;
             record_index++)
        {
            if (leaf_object.value.leaf.records[record_index].term_id == 0 &&
                leaf_base + record_index != 0)
            {
                leaf_object.value.leaf.records[record_index].term_id =
                    leaf_base + record_index;
            }
        }
        while (run_index < new_payload->run_count &&
               new_payload->runs[run_index].term_id <
                   leaf_base + II42_TERM_COW_LEAF_TERMS)
        {
            uint32_t term_id = new_payload->runs[run_index].term_id;
            ii42_term_cow_record *record =
                &leaf_object.value.leaf.records[term_id - leaf_base];

            if (term_id >= next_manifest->vocab_size)
            {
                status = II42_ERR_FORMAT;
                goto rollback;
            }
            if (run_index == first_run ||
                new_payload->runs[run_index - 1].term_id != term_id)
            {
                stats.changed_terms++;
            }
            status = ii42_term_cow_record_add_extent(
                record,
                next_manifest->segments[new_segment_index].segment_id,
                &new_payload->runs[run_index]
            );
            if (status != II42_OK)
            {
                goto rollback;
            }
            run_index++;
        }
        status = ii42_term_cow_tree_append_object(
            tree,
            &leaf_object,
            &new_leaf_ref,
            &written_bytes
        );
        if (status != II42_OK)
        {
            goto rollback;
        }
        stats.changed_leaves++;
        stats.written_leaves++;
        stats.written_bytes += written_bytes;
        status = ii42_term_cow_tree_replace_leaf(
            tree,
            leaf_index,
            &new_leaf_ref,
            ii42_term_cow_leaf_max_extent_count(
                &leaf_object.value.leaf
            ),
            &stats
        );
        if (status != II42_OK)
        {
            goto rollback;
        }
    }
    *stats_out = stats;
    return II42_OK;

rollback:
    tree->object_count = saved_object_count;
    tree->next_object_id = saved_next_object_id;
    tree->root = saved_root;
    tree->vocab_size = saved_vocab_size;
    memset(stats_out, 0, sizeof(*stats_out));
    return status;
}

static bool
ii42_term_cow_manifest_segment_index(
    const ii42_segment_manifest *manifest,
    uint64_t segment_id,
    uint32_t *segment_index_out
)
{
    for (uint32_t segment_index = 0;
         segment_index < manifest->segment_count;
         segment_index++)
    {
        if (manifest->segments[segment_index].segment_id == segment_id)
        {
            *segment_index_out = segment_index;
            return true;
        }
    }
    return false;
}

ii42_status
ii42_term_cow_tree_materialize_flat(
    const ii42_term_cow_tree *tree,
    const ii42_segment_manifest *manifest,
    ii42_term_directory *directory_out,
    uint32_t **doc_frequencies_out
)
{
    ii42_term_directory directory;
    uint32_t *doc_frequencies = NULL;
    uint64_t extent_count = 0;
    uint64_t extent_cursor = 0;
    ii42_status status;

    if (tree == NULL || manifest == NULL || directory_out == NULL ||
        doc_frequencies_out == NULL ||
        tree->vocab_size != manifest->vocab_size)
    {
        return II42_ERR_INVALID;
    }
    *doc_frequencies_out = NULL;
    ii42_term_directory_init(&directory);
    directory.vocab_size = tree->vocab_size;
    directory.term_offsets = calloc(
        (size_t) directory.vocab_size + 1,
        sizeof(*directory.term_offsets)
    );
    doc_frequencies = calloc(
        directory.vocab_size,
        sizeof(*doc_frequencies)
    );
    if (directory.term_offsets == NULL ||
        (directory.vocab_size > 0 && doc_frequencies == NULL))
    {
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return II42_ERR_NOMEM;
    }
    for (uint32_t term_id = 0;
         term_id < directory.vocab_size;
         term_id++)
    {
        ii42_term_cow_record record;

        status = ii42_term_cow_tree_lookup(tree, term_id, &record);
        if (status != II42_OK ||
            extent_count > UINT32_MAX - record.extent_count)
        {
            free(doc_frequencies);
            ii42_term_directory_free(&directory);
            return status == II42_OK ? II42_ERR_RANGE : status;
        }
        directory.term_offsets[term_id] = extent_count;
        doc_frequencies[term_id] = record.raw_document_frequency;
        extent_count += record.extent_count;
    }
    directory.term_offsets[directory.vocab_size] = extent_count;
    directory.extent_count = (uint32_t) extent_count;
    if (directory.extent_count > 0)
    {
        directory.extents = calloc(
            directory.extent_count,
            sizeof(*directory.extents)
        );
        if (directory.extents == NULL)
        {
            free(doc_frequencies);
            ii42_term_directory_free(&directory);
            return II42_ERR_NOMEM;
        }
    }
    for (uint32_t term_id = 0;
         term_id < directory.vocab_size;
         term_id++)
    {
        ii42_term_cow_record record;

        status = ii42_term_cow_tree_lookup(tree, term_id, &record);
        if (status != II42_OK)
        {
            free(doc_frequencies);
            ii42_term_directory_free(&directory);
            return status;
        }
        for (uint32_t extent_index = 0;
             extent_index < record.extent_count;
             extent_index++)
        {
            const ii42_stable_term_extent *source =
                &record.extents[extent_index];
            ii42_term_extent_descriptor *target =
                &directory.extents[extent_cursor];
            uint32_t segment_index;

            if (!ii42_term_cow_manifest_segment_index(
                    manifest,
                    source->segment_id,
                    &segment_index))
            {
                free(doc_frequencies);
                ii42_term_directory_free(&directory);
                return II42_ERR_FORMAT;
            }
            target->segment_index = segment_index;
            target->kind = source->kind;
            target->posting_offset = source->posting_offset;
            target->posting_count = source->posting_count;
            extent_cursor++;
        }
    }
    if (extent_cursor != directory.extent_count)
    {
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return II42_ERR_FORMAT;
    }
    status = ii42_term_directory_validate(&directory, manifest);
    if (status != II42_OK)
    {
        free(doc_frequencies);
        ii42_term_directory_free(&directory);
        return status;
    }
    ii42_term_directory_free(directory_out);
    *directory_out = directory;
    *doc_frequencies_out = doc_frequencies;
    return II42_OK;
}

ii42_status
ii42_term_cow_tree_object_serialize(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *ref,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    const ii42_term_cow_object *object =
        ii42_term_cow_tree_find_object(tree, ref);

    if (object == NULL)
    {
        return II42_ERR_FORMAT;
    }
    return ii42_term_cow_object_encode(object, bytes_out, size_out);
}

ii42_status
ii42_term_cow_tree_prepare_object_for_storage(
    ii42_term_cow_tree *tree,
    uint64_t object_id,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    ii42_term_cow_object *object;
    ii42_term_cow_object prepared;
    ii42_status status;

    if (tree == NULL || bytes_out == NULL || size_out == NULL ||
        object_id == 0 || object_id > tree->object_count)
    {
        return II42_ERR_INVALID;
    }
    *bytes_out = NULL;
    *size_out = 0;
    object = &tree->objects[object_id - 1];
    if (object->ref.object_id != object_id ||
        !ii42_term_cow_ref_is_unbound(&object->ref))
    {
        return II42_ERR_FORMAT;
    }
    prepared = *object;

    if (prepared.ref.kind == II42_TERM_COW_OBJECT_NODE)
    {
        ii42_term_cow_node *node = &prepared.value.node;

        for (uint32_t child_index = 0;
             child_index < node->child_count;
             child_index++)
        {
            ii42_term_cow_child *child =
                &node->children[child_index];
            ii42_term_cow_object *stored_child;

            if (!ii42_term_cow_ref_is_unbound(&child->ref))
            {
                if (!ii42_term_cow_ref_valid(&child->ref))
                {
                    return II42_ERR_FORMAT;
                }
                continue;
            }
            if (child->ref.object_id == 0 ||
                child->ref.object_id >= object_id ||
                child->ref.object_id > tree->object_count)
            {
                return II42_ERR_FORMAT;
            }
            stored_child =
                &tree->objects[child->ref.object_id - 1];
            if (stored_child->ref.kind != child->ref.kind ||
                stored_child->ref.object_id != child->ref.object_id ||
                ii42_term_cow_ref_is_unbound(&stored_child->ref) ||
                !ii42_term_cow_ref_valid(&stored_child->ref))
            {
                return II42_ERR_FORMAT;
            }
            child->ref = stored_child->ref;
        }
    }
    status = ii42_term_cow_object_refresh_integrity(
        &prepared,
        bytes_out,
        size_out
    );
    if (status != II42_OK)
    {
        return status;
    }
    *object = prepared;
    return II42_OK;
}

ii42_status
ii42_term_cow_object_bind_storage(
    ii42_term_cow_object *object,
    const ii42_segment_object_ref *storage_ref
)
{
    ii42_term_cow_object bound;

    if (object == NULL || storage_ref == NULL ||
        !ii42_term_cow_ref_is_unbound(&object->ref) ||
        storage_ref->object_kind !=
            II42_SEGMENT_OBJECT_TERM_DIRECTORY ||
        storage_ref->object_id != object->ref.object_id ||
        storage_ref->object_bytes != object->ref.object_bytes ||
        storage_ref->object_checksum != object->ref.blob_checksum ||
        ii42_segment_object_ref_validate(
            storage_ref,
            UINT32_MAX) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }

    bound = *object;
    bound.ref.start_block = storage_ref->start_block;
    bound.ref.page_count = storage_ref->page_count;
    bound.ref.owner_manifest_id = storage_ref->owner_manifest_id;
    if (!ii42_term_cow_ref_valid(&bound.ref) ||
        ii42_term_cow_object_validate(&bound) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    *object = bound;
    return II42_OK;
}

ii42_status
ii42_term_cow_tree_bind_object_storage(
    ii42_term_cow_tree *tree,
    uint64_t object_id,
    const ii42_segment_object_ref *storage_ref
)
{
    ii42_term_cow_object *object;
    ii42_status status;

    if (tree == NULL || object_id == 0 ||
        object_id > tree->object_count)
    {
        return II42_ERR_INVALID;
    }
    object = &tree->objects[object_id - 1];
    status = ii42_term_cow_object_bind_storage(object, storage_ref);
    if (status != II42_OK)
    {
        return status;
    }
    if (tree->root.object_id == object_id)
    {
        tree->root = object->ref;
    }
    return II42_OK;
}

ii42_status
ii42_term_cow_ref_as_segment_object_ref(
    const ii42_term_cow_ref *ref,
    ii42_segment_object_ref *storage_ref_out
)
{
    ii42_segment_object_ref storage_ref;

    if (storage_ref_out == NULL ||
        !ii42_term_cow_ref_valid(ref) ||
        ii42_term_cow_ref_is_unbound(ref))
    {
        return II42_ERR_INVALID;
    }
    memset(&storage_ref, 0, sizeof(storage_ref));
    storage_ref.object_kind = II42_SEGMENT_OBJECT_TERM_DIRECTORY;
    storage_ref.start_block = ref->start_block;
    storage_ref.page_count = ref->page_count;
    storage_ref.object_id = ref->object_id;
    storage_ref.owner_manifest_id = ref->owner_manifest_id;
    storage_ref.object_bytes = ref->object_bytes;
    storage_ref.object_checksum = ref->blob_checksum;
    if (ii42_segment_object_ref_validate(
            &storage_ref,
            UINT32_MAX) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    *storage_ref_out = storage_ref;
    return II42_OK;
}

ii42_status
ii42_term_cow_object_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_term_cow_object *object_out
)
{
    ii42_term_cow_object object;
    ii42_term_cow_object_kind kind;
    uint16_t version;
    uint16_t level;
    uint32_t count;
    uint64_t key;
    size_t expected_size;

    if (bytes == NULL || object_out == NULL ||
        size < II42_TERM_COW_HEADER_SIZE ||
        ii42_term_cow_read_u32(bytes + 0) != II42_TERM_COW_MAGIC ||
        ii42_term_cow_read_u16(bytes + 6) !=
            II42_TERM_COW_HEADER_SIZE ||
        ii42_term_cow_read_u64(bytes + 32) != size)
    {
        return II42_ERR_FORMAT;
    }
    version = ii42_term_cow_read_u16(bytes + 4);
    if (version != II42_TERM_COW_VERSION)
    {
        return II42_ERR_FORMAT;
    }
    kind = (ii42_term_cow_object_kind)
        ii42_term_cow_read_u16(bytes + 8);
    level = ii42_term_cow_read_u16(bytes + 10);
    count = ii42_term_cow_read_u32(bytes + 12);
    key = ii42_term_cow_read_u64(bytes + 24);
    memset(&object, 0, sizeof(object));
    object.ref.kind = kind;
    object.ref.object_id = ii42_term_cow_read_u64(bytes + 16);
    object.ref.object_bytes = size;
    object.ref.checksum = ii42_term_cow_read_u64(
        bytes + II42_TERM_COW_CHECKSUM_OFFSET
    );
    object.ref.blob_checksum =
        ii42_segment_blob_checksum(bytes, size);
    if (object.ref.checksum == 0 ||
        object.ref.blob_checksum == 0 ||
        object.ref.checksum != ii42_term_cow_checksum(bytes, size))
    {
        return II42_ERR_FORMAT;
    }

    if (kind == II42_TERM_COW_OBJECT_NODE)
    {
        expected_size = II42_TERM_COW_HEADER_SIZE +
            (size_t) count * II42_TERM_COW_CHILD_SIZE;
        if (level >= II42_TERM_COW_RADIX_LEVELS ||
            count > II42_TERM_COW_RADIX_FANOUT ||
            expected_size != size)
        {
            return II42_ERR_FORMAT;
        }
        object.value.node.level = level;
        object.value.node.prefix = key;
        object.value.node.child_count = count;
        for (uint32_t child_index = 0;
             child_index < count;
             child_index++)
        {
            const uint8_t *child_bytes =
                bytes + II42_TERM_COW_HEADER_SIZE +
                (size_t) child_index * II42_TERM_COW_CHILD_SIZE;
            ii42_term_cow_child *child =
                &object.value.node.children[child_index];

            child->slot = ii42_term_cow_read_u16(child_bytes + 0);
            child->ref.kind = (ii42_term_cow_object_kind)
                ii42_term_cow_read_u16(child_bytes + 2);
            child->ref.start_block =
                ii42_term_cow_read_u32(child_bytes + 4);
            child->ref.page_count =
                ii42_term_cow_read_u32(child_bytes + 8);
            child->max_extent_count =
                ii42_term_cow_read_u32(child_bytes + 12);
            child->ref.object_id =
                ii42_term_cow_read_u64(child_bytes + 16);
            child->ref.owner_manifest_id =
                ii42_term_cow_read_u64(child_bytes + 24);
            child->ref.object_bytes =
                ii42_term_cow_read_u64(child_bytes + 32);
            child->ref.checksum =
                ii42_term_cow_read_u64(child_bytes + 40);
            child->ref.blob_checksum =
                ii42_term_cow_read_u64(child_bytes + 48);
        }
    }
    else if (kind == II42_TERM_COW_OBJECT_LEAF)
    {
        size_t records_offset = II42_TERM_COW_HEADER_SIZE;
        size_t extents_offset;
        size_t fixed_size;
        size_t extents_size;
        size_t extent_stride;
        size_t stored_max_extents_per_term;

        if (level != 0 || key > UINT32_MAX ||
            count == 0 || count > II42_TERM_COW_LEAF_TERMS)
        {
            return II42_ERR_FORMAT;
        }
        fixed_size = II42_TERM_COW_HEADER_SIZE +
            (size_t) count * II42_TERM_COW_RECORD_SIZE;
        if (size < fixed_size)
        {
            return II42_ERR_FORMAT;
        }
        extents_size = size - fixed_size;
        extent_stride = (size_t) count * II42_TERM_COW_EXTENT_SIZE;
        if (extent_stride == 0 || extents_size % extent_stride != 0)
        {
            return II42_ERR_FORMAT;
        }
        stored_max_extents_per_term = extents_size / extent_stride;
        if (stored_max_extents_per_term == 0 ||
            stored_max_extents_per_term >
                II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM)
        {
            return II42_ERR_FORMAT;
        }
        expected_size = fixed_size +
            (size_t) count *
                stored_max_extents_per_term *
                II42_TERM_COW_EXTENT_SIZE;
        if (expected_size != size)
        {
            return II42_ERR_FORMAT;
        }
        object.value.leaf.base_term_id = (uint32_t) key;
        object.value.leaf.record_count = count;
        extents_offset =
            records_offset + (size_t) count * II42_TERM_COW_RECORD_SIZE;
        for (uint32_t record_index = 0;
             record_index < count;
             record_index++)
        {
            const uint8_t *record_bytes =
                bytes + records_offset +
                (size_t) record_index * II42_TERM_COW_RECORD_SIZE;
            ii42_term_cow_record *record =
                &object.value.leaf.records[record_index];

            record->term_id =
                ii42_term_cow_read_u32(record_bytes + 0);
            record->raw_document_frequency =
                ii42_term_cow_read_u32(record_bytes + 4);
            record->flags =
                ii42_term_cow_read_u32(record_bytes + 8);
            record->extent_count =
                ii42_term_cow_read_u32(record_bytes + 12);
            record->neutral_fold_coverage =
                ii42_term_cow_read_u64(record_bytes + 16);
            record->neutral_minor_fold_coverage =
                ii42_term_cow_read_u64(record_bytes + 24);
            record->impact_fold_coverage =
                ii42_term_cow_read_u64(record_bytes + 32);
            record->impact_statistics_epoch =
                ii42_term_cow_read_u64(record_bytes + 40);
            if (ii42_term_cow_decode_object_ref(
                    record_bytes + 48,
                    &record->neutral_fold
                ) != II42_OK ||
                ii42_term_cow_decode_object_ref(
                    record_bytes + 48 +
                        II42_TERM_COW_OBJECT_REF_SIZE,
                    &record->neutral_minor_fold
                ) != II42_OK ||
                ii42_term_cow_decode_object_ref(
                    record_bytes + 48 +
                        2 * II42_TERM_COW_OBJECT_REF_SIZE,
                    &record->impact_fold
                ) != II42_OK ||
                ii42_term_cow_decode_object_ref(
                    record_bytes + 48 +
                        3 * II42_TERM_COW_OBJECT_REF_SIZE,
                    &record->lexical_catalog
                ) != II42_OK ||
                ii42_term_cow_read_u64(record_bytes + 240) != 0)
            {
                return II42_ERR_FORMAT;
            }
            for (uint32_t extent_index = 0;
                 extent_index < stored_max_extents_per_term;
                 extent_index++)
            {
                const uint8_t *extent_bytes =
                    bytes + extents_offset +
                    ((size_t) record_index *
                         stored_max_extents_per_term +
                     extent_index) * II42_TERM_COW_EXTENT_SIZE;
                ii42_stable_term_extent *extent =
                    &record->extents[extent_index];

                extent->segment_id =
                    ii42_term_cow_read_u64(extent_bytes + 0);
                extent->kind = (ii42_posting_extent_kind)
                    ii42_term_cow_read_u32(extent_bytes + 8);
                if (ii42_term_cow_read_u32(extent_bytes + 12) != 0)
                {
                    return II42_ERR_FORMAT;
                }
                extent->posting_offset =
                    ii42_term_cow_read_u64(extent_bytes + 16);
                extent->posting_count =
                    ii42_term_cow_read_u64(extent_bytes + 24);
            }
        }
    }
    else
    {
        return II42_ERR_FORMAT;
    }
    if (ii42_term_cow_object_validate(&object) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    *object_out = object;
    return II42_OK;
}
