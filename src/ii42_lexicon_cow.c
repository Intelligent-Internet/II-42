#include "ii42_lexicon_cow.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define II42_LEXICON_COW_MAGIC UINT32_C(0x58454C32)
#define II42_LEXICON_COW_VERSION UINT16_C(1)
#define II42_LEXICON_COW_HEADER_SIZE 64U
#define II42_LEXICON_COW_CHECKSUM_OFFSET 40U
#define II42_LEXICON_COW_CHILD_SIZE 64U
#define II42_LEXICON_COW_ENTRY_HEADER_SIZE 16U

static void
ii42_lexicon_cow_write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t) (value & UINT16_C(0xff));
    dst[1] = (uint8_t) ((value >> 8) & UINT16_C(0xff));
}

static uint16_t
ii42_lexicon_cow_read_u16(const uint8_t *src)
{
    return (uint16_t) src[0] |
        (uint16_t) ((uint16_t) src[1] << 8);
}

static void
ii42_lexicon_cow_write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t) (value & UINT32_C(0xff));
    dst[1] = (uint8_t) ((value >> 8) & UINT32_C(0xff));
    dst[2] = (uint8_t) ((value >> 16) & UINT32_C(0xff));
    dst[3] = (uint8_t) ((value >> 24) & UINT32_C(0xff));
}

static uint32_t
ii42_lexicon_cow_read_u32(const uint8_t *src)
{
    return (uint32_t) src[0] |
        ((uint32_t) src[1] << 8) |
        ((uint32_t) src[2] << 16) |
        ((uint32_t) src[3] << 24);
}

static void
ii42_lexicon_cow_write_u64(uint8_t *dst, uint64_t value)
{
    for (uint32_t byte_index = 0; byte_index < 8; byte_index++)
    {
        dst[byte_index] = (uint8_t) (
            (value >> (byte_index * 8)) & UINT64_C(0xff)
        );
    }
}

static uint64_t
ii42_lexicon_cow_read_u64(const uint8_t *src)
{
    uint64_t value = 0;

    for (uint32_t byte_index = 0; byte_index < 8; byte_index++)
    {
        value |= (uint64_t) src[byte_index] << (byte_index * 8);
    }
    return value;
}

static uint64_t
ii42_lexicon_cow_checksum(const uint8_t *bytes, size_t size)
{
    uint64_t checksum = UINT64_C(14695981039346656037);

    for (size_t index = 0; index < size; index++)
    {
        uint8_t value =
            index >= II42_LEXICON_COW_CHECKSUM_OFFSET &&
            index < II42_LEXICON_COW_CHECKSUM_OFFSET + sizeof(uint64_t)
                ? 0
                : bytes[index];

        checksum ^= value;
        checksum *= UINT64_C(1099511628211);
    }
    return checksum;
}

uint64_t
ii42_lexicon_cow_hash(
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t seed
)
{
    uint64_t hash = UINT64_C(14695981039346656037) ^ seed;

    if (bytes_len > 0 && bytes == NULL)
    {
        return 0;
    }
    for (size_t index = 0; index < bytes_len; index++)
    {
        hash ^= bytes[index];
        hash *= UINT64_C(1099511628211);
    }
    hash ^= hash >> 32;
    hash *= UINT64_C(0xd6e8feb86659fd93);
    hash ^= hash >> 32;
    return hash;
}

static uint64_t
ii42_lexicon_cow_prefix_mask(uint16_t depth)
{
    uint32_t bits = (uint32_t) depth * II42_LEXICON_COW_RADIX_BITS;

    if (bits == 0)
    {
        return 0;
    }
    if (bits >= 64)
    {
        return UINT64_MAX;
    }
    return (UINT64_C(1) << bits) - 1;
}

static uint16_t
ii42_lexicon_cow_hash_slot(uint64_t hash, uint16_t level)
{
    uint32_t shift = (uint32_t) level * II42_LEXICON_COW_RADIX_BITS;

    if (shift >= 64)
    {
        return 0;
    }
    return (uint16_t) (
        (hash >> shift) & (II42_LEXICON_COW_RADIX_FANOUT - 1)
    );
}

static int
ii42_lexicon_cow_compare_bytes(
    const uint8_t *left,
    uint32_t left_len,
    const uint8_t *right,
    uint32_t right_len
)
{
    size_t common = left_len < right_len ? left_len : right_len;
    int order = common == 0 ? 0 : memcmp(left, right, common);

    if (order != 0)
    {
        return order;
    }
    if (left_len < right_len)
    {
        return -1;
    }
    if (left_len > right_len)
    {
        return 1;
    }
    return 0;
}

static int
ii42_lexicon_cow_compare_entries(const void *left_ptr, const void *right_ptr)
{
    const ii42_lexicon_cow_entry *left = left_ptr;
    const ii42_lexicon_cow_entry *right = right_ptr;

    if (left->hash < right->hash)
    {
        return -1;
    }
    if (left->hash > right->hash)
    {
        return 1;
    }
    return ii42_lexicon_cow_compare_bytes(
        left->bytes,
        left->bytes_len,
        right->bytes,
        right->bytes_len
    );
}

static int
ii42_lexicon_cow_compare_keys_by_bytes(
    const void *left_ptr,
    const void *right_ptr
)
{
    const ii42_lexicon_cow_key *left = left_ptr;
    const ii42_lexicon_cow_key *right = right_ptr;

    return ii42_lexicon_cow_compare_bytes(
        left->bytes,
        left->bytes_len,
        right->bytes,
        right->bytes_len
    );
}

static bool
ii42_lexicon_cow_refs_equal(
    const ii42_lexicon_cow_ref *left,
    const ii42_lexicon_cow_ref *right
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
ii42_lexicon_cow_ref_valid(const ii42_lexicon_cow_ref *ref)
{
    bool has_locator;

    if (ref == NULL ||
        (ref->kind != II42_LEXICON_COW_OBJECT_NODE &&
         ref->kind != II42_LEXICON_COW_OBJECT_BUCKET) ||
        ref->reserved != 0 || ref->object_id == 0 ||
        ref->owner_manifest_id == 0 ||
        ref->object_bytes < II42_LEXICON_COW_HEADER_SIZE ||
        ref->checksum == 0 || ref->blob_checksum == 0)
    {
        return false;
    }
    has_locator = ref->start_block != 0 || ref->page_count != 0;
    if (!has_locator)
    {
        return ref->start_block == 0 && ref->page_count == 0;
    }
    return ref->start_block != 0 && ref->start_block != UINT32_MAX &&
        ref->page_count != 0 &&
        (uint64_t) ref->start_block + ref->page_count <= UINT32_MAX;
}

static bool
ii42_lexicon_cow_ref_is_unbound(const ii42_lexicon_cow_ref *ref)
{
    return ref != NULL && ref->start_block == 0 && ref->page_count == 0;
}

static size_t
ii42_lexicon_cow_bucket_size(
    const ii42_lexicon_cow_entry *entries,
    uint32_t entry_count,
    bool *valid_out
)
{
    size_t size = II42_LEXICON_COW_HEADER_SIZE;

    *valid_out = false;
    for (uint32_t index = 0; index < entry_count; index++)
    {
        size_t entry_size = II42_LEXICON_COW_ENTRY_HEADER_SIZE +
            (size_t) entries[index].bytes_len;

        if (entry_size < entries[index].bytes_len ||
            size > SIZE_MAX - entry_size)
        {
            return 0;
        }
        size += entry_size;
    }
    *valid_out = true;
    return size;
}

static size_t
ii42_lexicon_cow_object_size(
    const ii42_lexicon_cow_object *object,
    bool *valid_out
)
{
    *valid_out = false;
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        size_t child_bytes;

        if (object->value.node.child_count >
            II42_LEXICON_COW_RADIX_FANOUT)
        {
            return 0;
        }
        child_bytes = (size_t) object->value.node.child_count *
            II42_LEXICON_COW_CHILD_SIZE;
        *valid_out = true;
        return II42_LEXICON_COW_HEADER_SIZE + child_bytes;
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        return ii42_lexicon_cow_bucket_size(
            object->value.bucket.entries,
            object->value.bucket.entry_count,
            valid_out
        );
    }
    return 0;
}

static ii42_status
ii42_lexicon_cow_object_shape_validate(
    const ii42_lexicon_cow_object *object,
    bool require_checksum
)
{
    bool size_valid;
    size_t object_size;

    if (object == NULL ||
        (object->ref.kind != II42_LEXICON_COW_OBJECT_NODE &&
         object->ref.kind != II42_LEXICON_COW_OBJECT_BUCKET) ||
        object->ref.reserved != 0 ||
        object->ref.object_id == 0 ||
        object->ref.owner_manifest_id == 0)
    {
        return II42_ERR_FORMAT;
    }
    object_size = ii42_lexicon_cow_object_size(object, &size_valid);
    if (!size_valid || object_size > UINT64_MAX ||
        (require_checksum &&
         (object->ref.object_bytes != object_size ||
          object->ref.checksum == 0 ||
          object->ref.blob_checksum == 0)))
    {
        return II42_ERR_FORMAT;
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        const ii42_lexicon_cow_node *node = &object->value.node;
        uint64_t term_count = 0;

        if (node->level >= II42_LEXICON_COW_RADIX_LEVELS ||
            node->reserved != 0 || node->reserved2 != 0 ||
            node->child_count == 0 ||
            node->child_count > II42_LEXICON_COW_RADIX_FANOUT ||
            node->term_count == 0 ||
            node->prefix !=
                (node->prefix &
                 ii42_lexicon_cow_prefix_mask(node->level)))
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < node->child_count; index++)
        {
            const ii42_lexicon_cow_child *child = &node->children[index];

            if (child->slot >= II42_LEXICON_COW_RADIX_FANOUT ||
                child->reserved != 0 || child->term_count == 0 ||
                !ii42_lexicon_cow_ref_valid(&child->ref) ||
                (index > 0 &&
                 node->children[index - 1].slot >= child->slot))
            {
                return II42_ERR_FORMAT;
            }
            term_count += child->term_count;
        }
        if (term_count != node->term_count)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = node->child_count;
             index < II42_LEXICON_COW_RADIX_FANOUT;
             index++)
        {
            static const ii42_lexicon_cow_child zero_child = {0};

            if (memcmp(
                    &node->children[index],
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
        const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;
        uint64_t mask;

        if (bucket->depth > II42_LEXICON_COW_RADIX_LEVELS ||
            bucket->reserved != 0 || bucket->entry_count == 0 ||
            bucket->entries == NULL)
        {
            return II42_ERR_FORMAT;
        }
        mask = ii42_lexicon_cow_prefix_mask(bucket->depth);
        if (bucket->prefix != (bucket->prefix & mask))
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < bucket->entry_count; index++)
        {
            const ii42_lexicon_cow_entry *entry = &bucket->entries[index];

            if ((entry->bytes_len > 0 && entry->bytes == NULL) ||
                (entry->hash & mask) != bucket->prefix ||
                (index > 0 &&
                 ii42_lexicon_cow_compare_entries(
                     &bucket->entries[index - 1],
                     entry
                 ) >= 0))
            {
                return II42_ERR_FORMAT;
            }
        }
        return II42_OK;
    }
}

static void
ii42_lexicon_cow_encode_ref(
    uint8_t *bytes,
    const ii42_lexicon_cow_ref *ref
)
{
    ii42_lexicon_cow_write_u32(bytes + 0, (uint32_t) ref->kind);
    ii42_lexicon_cow_write_u32(bytes + 4, ref->start_block);
    ii42_lexicon_cow_write_u32(bytes + 8, ref->page_count);
    ii42_lexicon_cow_write_u32(bytes + 12, ref->reserved);
    ii42_lexicon_cow_write_u64(bytes + 16, ref->object_id);
    ii42_lexicon_cow_write_u64(bytes + 24, ref->owner_manifest_id);
    ii42_lexicon_cow_write_u64(bytes + 32, ref->object_bytes);
    ii42_lexicon_cow_write_u64(bytes + 40, ref->checksum);
    ii42_lexicon_cow_write_u64(bytes + 48, ref->blob_checksum);
}

static void
ii42_lexicon_cow_decode_ref(
    const uint8_t *bytes,
    ii42_lexicon_cow_ref *ref
)
{
    memset(ref, 0, sizeof(*ref));
    ref->kind =
        (ii42_lexicon_cow_object_kind) ii42_lexicon_cow_read_u32(bytes + 0);
    ref->start_block = ii42_lexicon_cow_read_u32(bytes + 4);
    ref->page_count = ii42_lexicon_cow_read_u32(bytes + 8);
    ref->reserved = ii42_lexicon_cow_read_u32(bytes + 12);
    ref->object_id = ii42_lexicon_cow_read_u64(bytes + 16);
    ref->owner_manifest_id = ii42_lexicon_cow_read_u64(bytes + 24);
    ref->object_bytes = ii42_lexicon_cow_read_u64(bytes + 32);
    ref->checksum = ii42_lexicon_cow_read_u64(bytes + 40);
    ref->blob_checksum = ii42_lexicon_cow_read_u64(bytes + 48);
}

static ii42_status
ii42_lexicon_cow_object_encode(
    const ii42_lexicon_cow_object *object,
    bool require_checksum,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    uint8_t *bytes;
    size_t size;
    bool size_valid;

    if (bytes_out == NULL || size_out == NULL)
    {
        return II42_ERR_INVALID;
    }
    *bytes_out = NULL;
    *size_out = 0;
    if (ii42_lexicon_cow_object_shape_validate(
            object,
            require_checksum) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    size = ii42_lexicon_cow_object_size(object, &size_valid);
    if (!size_valid)
    {
        return II42_ERR_FORMAT;
    }
    bytes = calloc(size, 1);
    if (bytes == NULL)
    {
        return II42_ERR_NOMEM;
    }
    ii42_lexicon_cow_write_u32(bytes + 0, II42_LEXICON_COW_MAGIC);
    ii42_lexicon_cow_write_u16(bytes + 4, II42_LEXICON_COW_VERSION);
    ii42_lexicon_cow_write_u16(bytes + 6, II42_LEXICON_COW_HEADER_SIZE);
    ii42_lexicon_cow_write_u16(bytes + 8, (uint16_t) object->ref.kind);
    ii42_lexicon_cow_write_u16(
        bytes + 10,
        object->ref.kind == II42_LEXICON_COW_OBJECT_NODE
            ? object->value.node.level
            : object->value.bucket.depth
    );
    ii42_lexicon_cow_write_u32(bytes + 12, 0);
    ii42_lexicon_cow_write_u64(bytes + 16, object->ref.object_id);
    ii42_lexicon_cow_write_u64(
        bytes + 24,
        object->ref.owner_manifest_id
    );
    ii42_lexicon_cow_write_u64(bytes + 32, size);
    ii42_lexicon_cow_write_u64(bytes + 40, object->ref.checksum);
    ii42_lexicon_cow_write_u64(
        bytes + 48,
        object->ref.kind == II42_LEXICON_COW_OBJECT_NODE
            ? object->value.node.prefix
            : object->value.bucket.prefix
    );
    ii42_lexicon_cow_write_u32(
        bytes + 56,
        object->ref.kind == II42_LEXICON_COW_OBJECT_NODE
            ? object->value.node.child_count
            : object->value.bucket.entry_count
    );
    ii42_lexicon_cow_write_u32(
        bytes + 60,
        object->ref.kind == II42_LEXICON_COW_OBJECT_NODE
            ? object->value.node.term_count
            : object->value.bucket.entry_count
    );

    if (object->ref.kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        const ii42_lexicon_cow_node *node = &object->value.node;

        for (uint32_t index = 0; index < node->child_count; index++)
        {
            const ii42_lexicon_cow_child *child = &node->children[index];
            uint8_t *child_bytes = bytes + II42_LEXICON_COW_HEADER_SIZE +
                (size_t) index * II42_LEXICON_COW_CHILD_SIZE;

            ii42_lexicon_cow_write_u16(child_bytes + 0, child->slot);
            ii42_lexicon_cow_write_u16(child_bytes + 2, child->reserved);
            ii42_lexicon_cow_write_u32(
                child_bytes + 4,
                child->term_count
            );
            ii42_lexicon_cow_encode_ref(child_bytes + 8, &child->ref);
        }
    }
    else
    {
        const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;
        size_t offset = II42_LEXICON_COW_HEADER_SIZE;

        for (uint32_t index = 0; index < bucket->entry_count; index++)
        {
            const ii42_lexicon_cow_entry *entry = &bucket->entries[index];

            ii42_lexicon_cow_write_u64(bytes + offset, entry->hash);
            ii42_lexicon_cow_write_u32(
                bytes + offset + 8,
                entry->term_id
            );
            ii42_lexicon_cow_write_u32(
                bytes + offset + 12,
                entry->bytes_len
            );
            offset += II42_LEXICON_COW_ENTRY_HEADER_SIZE;
            if (entry->bytes_len > 0)
            {
                memcpy(bytes + offset, entry->bytes, entry->bytes_len);
                offset += entry->bytes_len;
            }
        }
    }
    *bytes_out = bytes;
    *size_out = size;
    return II42_OK;
}

static ii42_status
ii42_lexicon_cow_object_refresh(ii42_lexicon_cow_object *object)
{
    uint8_t *bytes = NULL;
    size_t size = 0;
    ii42_status status;

    if (object == NULL || !ii42_lexicon_cow_ref_is_unbound(&object->ref))
    {
        return II42_ERR_INVALID;
    }
    object->ref.object_bytes = 0;
    object->ref.checksum = 0;
    object->ref.blob_checksum = UINT64_C(1);
    status = ii42_lexicon_cow_object_encode(
        object,
        false,
        &bytes,
        &size
    );
    if (status != II42_OK)
    {
        return status;
    }
    object->ref.object_bytes = size;
    object->ref.checksum = ii42_lexicon_cow_checksum(bytes, size);
    free(bytes);
    bytes = NULL;
    if (object->ref.checksum == 0)
    {
        return II42_ERR_FORMAT;
    }
    status = ii42_lexicon_cow_object_encode(
        object,
        false,
        &bytes,
        &size
    );
    if (status != II42_OK)
    {
        return status;
    }
    object->ref.object_bytes = size;
    object->ref.blob_checksum = ii42_segment_blob_checksum(bytes, size);
    free(bytes);
    return object->ref.blob_checksum == 0
        ? II42_ERR_FORMAT
        : II42_OK;
}

static ii42_status
ii42_lexicon_cow_object_checksum_validate(
    const ii42_lexicon_cow_object *object
)
{
    uint8_t *bytes = NULL;
    size_t size = 0;
    uint64_t checksum;
    uint64_t blob_checksum;
    ii42_status status;

    status = ii42_lexicon_cow_object_encode(
        object,
        true,
        &bytes,
        &size
    );
    if (status != II42_OK)
    {
        return status;
    }
    checksum = ii42_lexicon_cow_checksum(bytes, size);
    blob_checksum = ii42_segment_blob_checksum(bytes, size);
    free(bytes);
    return checksum == object->ref.checksum &&
        blob_checksum == object->ref.blob_checksum
        ? II42_OK
        : II42_ERR_FORMAT;
}

void
ii42_lexicon_cow_object_free(ii42_lexicon_cow_object *object)
{
    if (object == NULL)
    {
        return;
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        if (object->value.bucket.owned_bytes == NULL)
        {
            for (uint32_t index = 0;
                 index < object->value.bucket.entry_count;
                 index++)
            {
                free(object->value.bucket.entries[index].bytes);
            }
        }
        free(object->value.bucket.owned_bytes);
        free(object->value.bucket.entries);
    }
    memset(object, 0, sizeof(*object));
}

void
ii42_lexicon_cow_tree_init(ii42_lexicon_cow_tree *tree)
{
    if (tree != NULL)
    {
        memset(tree, 0, sizeof(*tree));
        tree->next_object_id = 1;
    }
}

void
ii42_lexicon_cow_tree_free(ii42_lexicon_cow_tree *tree)
{
    if (tree == NULL)
    {
        return;
    }
    for (size_t index = 0; index < tree->object_count; index++)
    {
        ii42_lexicon_cow_object_free(&tree->objects[index]);
    }
    free(tree->objects);
    free(tree->retired_ranges);
    ii42_lexicon_cow_tree_init(tree);
}

static ii42_status
ii42_lexicon_cow_tree_add_retired_ref(
    ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *ref
)
{
    ii42_block_range *ranges;
    size_t next_capacity;

    if (tree == NULL || ref == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (ii42_lexicon_cow_ref_is_unbound(ref))
    {
        return II42_OK;
    }
    if (!ii42_lexicon_cow_ref_valid(ref))
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

static const ii42_lexicon_cow_object *
ii42_lexicon_cow_tree_find_object(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *ref
)
{
    const ii42_lexicon_cow_object *object;

    if (tree == NULL || !ii42_lexicon_cow_ref_valid(ref))
    {
        return NULL;
    }
    if (ref->object_id > tree->object_count)
    {
        return NULL;
    }
    object = &tree->objects[ref->object_id - 1];
    return ii42_lexicon_cow_refs_equal(&object->ref, ref)
        ? object
        : NULL;
}

static ii42_status
ii42_lexicon_cow_tree_store_object(
    ii42_lexicon_cow_tree *tree,
    ii42_lexicon_cow_object *object,
    ii42_lexicon_cow_update_stats *stats,
    ii42_lexicon_cow_ref *ref_out
)
{
    ii42_lexicon_cow_object *next_objects;
    size_t next_capacity;
    ii42_status status;

    if (tree == NULL || object == NULL || ref_out == NULL ||
        object->ref.owner_manifest_id == 0 ||
        tree->next_object_id == 0 || tree->next_object_id == UINT64_MAX ||
        tree->next_object_id != tree->object_count + 1)
    {
        return II42_ERR_INVALID;
    }
    object->ref.object_id = tree->next_object_id;
    status = ii42_lexicon_cow_object_refresh(object);
    if (status != II42_OK)
    {
        return status;
    }
    if (tree->object_count == tree->object_capacity)
    {
        next_capacity = tree->object_capacity == 0
            ? 16
            : tree->object_capacity * 2;
        if (next_capacity < tree->object_capacity ||
            next_capacity > SIZE_MAX / sizeof(*tree->objects))
        {
            return II42_ERR_RANGE;
        }
        next_objects = realloc(
            tree->objects,
            next_capacity * sizeof(*tree->objects)
        );
        if (next_objects == NULL)
        {
            return II42_ERR_NOMEM;
        }
        tree->objects = next_objects;
        tree->object_capacity = next_capacity;
    }
    tree->next_object_id++;
    tree->objects[tree->object_count++] = *object;
    *ref_out = object->ref;
    if (stats != NULL)
    {
        if (object->ref.kind == II42_LEXICON_COW_OBJECT_NODE)
        {
            stats->written_nodes++;
        }
        else
        {
            stats->written_buckets++;
        }
        stats->written_bytes += object->ref.object_bytes;
    }
    memset(object, 0, sizeof(*object));
    return II42_OK;
}

static ii42_status
ii42_lexicon_cow_copy_entry(
    const ii42_lexicon_cow_entry *source,
    ii42_lexicon_cow_entry *destination
)
{
    memset(destination, 0, sizeof(*destination));
    destination->bytes_len = source->bytes_len;
    destination->term_id = source->term_id;
    destination->hash = source->hash;
    if (source->bytes_len > 0)
    {
        destination->bytes = malloc(source->bytes_len);
        if (destination->bytes == NULL)
        {
            return II42_ERR_NOMEM;
        }
        memcpy(destination->bytes, source->bytes, source->bytes_len);
    }
    return II42_OK;
}

static bool
ii42_lexicon_cow_bucket_fits(
    ii42_lexicon_cow_entry *const *entries,
    uint32_t entry_count
)
{
    size_t size = II42_LEXICON_COW_HEADER_SIZE;

    for (uint32_t index = 0; index < entry_count; index++)
    {
        size_t entry_size = II42_LEXICON_COW_ENTRY_HEADER_SIZE +
            (size_t) entries[index]->bytes_len;

        if (entry_size < entries[index]->bytes_len ||
            size > SIZE_MAX - entry_size)
        {
            return false;
        }
        size += entry_size;
    }
    return size <= II42_LEXICON_COW_BUCKET_TARGET_BYTES;
}

static ii42_status
ii42_lexicon_cow_create_bucket(
    ii42_lexicon_cow_tree *tree,
    ii42_lexicon_cow_entry *const *entries,
    uint32_t entry_count,
    uint16_t depth,
    uint64_t prefix,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_update_stats *stats,
    ii42_lexicon_cow_ref *ref_out
)
{
    ii42_lexicon_cow_object object = {0};
    ii42_status status;

    if (entry_count == 0 || depth > II42_LEXICON_COW_RADIX_LEVELS)
    {
        return II42_ERR_INVALID;
    }
    object.ref.kind = II42_LEXICON_COW_OBJECT_BUCKET;
    object.ref.owner_manifest_id = owner_manifest_id;
    object.value.bucket.depth = depth;
    object.value.bucket.prefix = prefix;
    object.value.bucket.entry_count = entry_count;
    object.value.bucket.entries = calloc(
        entry_count,
        sizeof(*object.value.bucket.entries)
    );
    if (object.value.bucket.entries == NULL)
    {
        return II42_ERR_NOMEM;
    }
    for (uint32_t index = 0; index < entry_count; index++)
    {
        status = ii42_lexicon_cow_copy_entry(
            entries[index],
            &object.value.bucket.entries[index]
        );
        if (status != II42_OK)
        {
            ii42_lexicon_cow_object_free(&object);
            return status;
        }
    }
    qsort(
        object.value.bucket.entries,
        entry_count,
        sizeof(*object.value.bucket.entries),
        ii42_lexicon_cow_compare_entries
    );
    status = ii42_lexicon_cow_tree_store_object(
        tree,
        &object,
        stats,
        ref_out
    );
    ii42_lexicon_cow_object_free(&object);
    return status;
}

static ii42_status
ii42_lexicon_cow_build_subtree(
    ii42_lexicon_cow_tree *tree,
    ii42_lexicon_cow_entry *const *entries,
    uint32_t entry_count,
    uint16_t depth,
    uint64_t prefix,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_update_stats *stats,
    ii42_lexicon_cow_ref *ref_out
)
{
    uint32_t counts[II42_LEXICON_COW_RADIX_FANOUT] = {0};
    uint32_t offsets[II42_LEXICON_COW_RADIX_FANOUT] = {0};
    uint32_t cursors[II42_LEXICON_COW_RADIX_FANOUT] = {0};
    ii42_lexicon_cow_entry **ordered = NULL;
    ii42_lexicon_cow_object object = {0};
    uint32_t child_count = 0;
    ii42_status status = II42_OK;

    if (entry_count == 0 || depth > II42_LEXICON_COW_RADIX_LEVELS)
    {
        return II42_ERR_INVALID;
    }
    if (depth == II42_LEXICON_COW_RADIX_LEVELS ||
        ii42_lexicon_cow_bucket_fits(entries, entry_count))
    {
        return ii42_lexicon_cow_create_bucket(
            tree,
            entries,
            entry_count,
            depth,
            prefix,
            owner_manifest_id,
            stats,
            ref_out
        );
    }
    for (uint32_t index = 0; index < entry_count; index++)
    {
        counts[ii42_lexicon_cow_hash_slot(entries[index]->hash, depth)]++;
    }
    for (uint32_t slot = 0; slot < II42_LEXICON_COW_RADIX_FANOUT; slot++)
    {
        if (slot > 0)
        {
            offsets[slot] = offsets[slot - 1] + counts[slot - 1];
        }
        cursors[slot] = offsets[slot];
        if (counts[slot] > 0)
        {
            child_count++;
        }
    }
    if (child_count < 2)
    {
        if (depth + 1 >= II42_LEXICON_COW_RADIX_LEVELS)
        {
            return ii42_lexicon_cow_create_bucket(
                tree,
                entries,
                entry_count,
                depth,
                prefix,
                owner_manifest_id,
                stats,
                ref_out
            );
        }
    }
    ordered = malloc((size_t) entry_count * sizeof(*ordered));
    if (ordered == NULL)
    {
        return II42_ERR_NOMEM;
    }
    for (uint32_t index = 0; index < entry_count; index++)
    {
        uint16_t slot =
            ii42_lexicon_cow_hash_slot(entries[index]->hash, depth);

        ordered[cursors[slot]++] = entries[index];
    }
    object.ref.kind = II42_LEXICON_COW_OBJECT_NODE;
    object.ref.owner_manifest_id = owner_manifest_id;
    object.value.node.level = depth;
    object.value.node.prefix = prefix;
    object.value.node.term_count = entry_count;
    for (uint32_t slot = 0; slot < II42_LEXICON_COW_RADIX_FANOUT; slot++)
    {
        ii42_lexicon_cow_child *child;
        uint64_t child_prefix;

        if (counts[slot] == 0)
        {
            continue;
        }
        child = &object.value.node.children[
            object.value.node.child_count
        ];
        child_prefix = prefix |
            ((uint64_t) slot <<
             ((uint32_t) depth * II42_LEXICON_COW_RADIX_BITS));
        status = ii42_lexicon_cow_build_subtree(
            tree,
            ordered + offsets[slot],
            counts[slot],
            depth + 1,
            child_prefix,
            owner_manifest_id,
            stats,
            &child->ref
        );
        if (status != II42_OK)
        {
            free(ordered);
            return status;
        }
        child->slot = (uint16_t) slot;
        child->term_count = counts[slot];
        object.value.node.child_count++;
    }
    free(ordered);
    return ii42_lexicon_cow_tree_store_object(
        tree,
        &object,
        stats,
        ref_out
    );
}

static ii42_status
ii42_lexicon_cow_prepare_entries(
    const ii42_lexicon_cow_key *keys,
    uint32_t key_count,
    uint32_t first_term_id,
    uint64_t hash_seed,
    ii42_lexicon_cow_entry **entries_out,
    ii42_lexicon_cow_entry ***entry_refs_out
)
{
    ii42_lexicon_cow_key *sorted_keys = NULL;
    ii42_lexicon_cow_entry *entries = NULL;
    ii42_lexicon_cow_entry **entry_refs = NULL;
    bool *seen_ids = NULL;

    *entries_out = NULL;
    *entry_refs_out = NULL;
    if (key_count == 0)
    {
        return II42_OK;
    }
    if (keys == NULL ||
        (uint64_t) first_term_id + key_count > UINT32_MAX)
    {
        return II42_ERR_INVALID;
    }
    sorted_keys = malloc((size_t) key_count * sizeof(*sorted_keys));
    entries = calloc(key_count, sizeof(*entries));
    entry_refs = malloc((size_t) key_count * sizeof(*entry_refs));
    seen_ids = calloc(key_count, sizeof(*seen_ids));
    if (sorted_keys == NULL || entries == NULL || entry_refs == NULL ||
        seen_ids == NULL)
    {
        free(sorted_keys);
        free(entries);
        free(entry_refs);
        free(seen_ids);
        return II42_ERR_NOMEM;
    }
    memcpy(sorted_keys, keys, (size_t) key_count * sizeof(*sorted_keys));
    qsort(
        sorted_keys,
        key_count,
        sizeof(*sorted_keys),
        ii42_lexicon_cow_compare_keys_by_bytes
    );
    for (uint32_t index = 0; index < key_count; index++)
    {
        const ii42_lexicon_cow_key *key = &sorted_keys[index];
        uint32_t local_id;

        if ((key->bytes_len > 0 && key->bytes == NULL) ||
            key->term_id < first_term_id ||
            key->term_id >= first_term_id + key_count)
        {
            free(sorted_keys);
            free(entries);
            free(entry_refs);
            free(seen_ids);
            return II42_ERR_FORMAT;
        }
        if (index > 0 &&
            ii42_lexicon_cow_compare_bytes(
                sorted_keys[index - 1].bytes,
                sorted_keys[index - 1].bytes_len,
                key->bytes,
                key->bytes_len
            ) == 0)
        {
            free(sorted_keys);
            free(entries);
            free(entry_refs);
            free(seen_ids);
            return II42_ERR_FORMAT;
        }
        local_id = key->term_id - first_term_id;
        if (seen_ids[local_id])
        {
            free(sorted_keys);
            free(entries);
            free(entry_refs);
            free(seen_ids);
            return II42_ERR_FORMAT;
        }
        seen_ids[local_id] = true;
        entries[index].bytes = (uint8_t *) key->bytes;
        entries[index].bytes_len = key->bytes_len;
        entries[index].term_id = key->term_id;
        entries[index].hash = ii42_lexicon_cow_hash(
            key->bytes,
            key->bytes_len,
            hash_seed
        );
        entry_refs[index] = &entries[index];
    }
    free(sorted_keys);
    free(seen_ids);
    *entries_out = entries;
    *entry_refs_out = entry_refs;
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_tree_build(
    const ii42_lexicon_cow_key *keys,
    uint32_t key_count,
    uint64_t hash_seed,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_tree *tree_out
)
{
    ii42_lexicon_cow_tree tree;
    ii42_lexicon_cow_entry *entries = NULL;
    ii42_lexicon_cow_entry **entry_refs = NULL;
    ii42_status status;

    if (tree_out == NULL || owner_manifest_id == 0)
    {
        return II42_ERR_INVALID;
    }
    ii42_lexicon_cow_tree_init(&tree);
    tree.hash_seed = hash_seed;
    status = ii42_lexicon_cow_prepare_entries(
        keys,
        key_count,
        0,
        hash_seed,
        &entries,
        &entry_refs
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (key_count > 0)
    {
        status = ii42_lexicon_cow_build_subtree(
            &tree,
            entry_refs,
            key_count,
            0,
            0,
            owner_manifest_id,
            NULL,
            &tree.root
        );
    }
    free(entry_refs);
    free(entries);
    if (status != II42_OK)
    {
        ii42_lexicon_cow_tree_free(&tree);
        return status;
    }
    tree.term_count = key_count;
    ii42_lexicon_cow_tree_free(tree_out);
    *tree_out = tree;
    return II42_OK;
}

static const ii42_lexicon_cow_child *
ii42_lexicon_cow_node_find_child(
    const ii42_lexicon_cow_node *node,
    uint16_t slot
)
{
    uint32_t left = 0;
    uint32_t right = node->child_count;

    while (left < right)
    {
        uint32_t middle = left + (right - left) / 2;
        uint16_t middle_slot = node->children[middle].slot;

        if (middle_slot < slot)
        {
            left = middle + 1;
        }
        else
        {
            right = middle;
        }
    }
    if (left < node->child_count && node->children[left].slot == slot)
    {
        return &node->children[left];
    }
    return NULL;
}

static const ii42_lexicon_cow_entry *
ii42_lexicon_cow_bucket_find_entry(
    const ii42_lexicon_cow_bucket *bucket,
    const uint8_t *bytes,
    uint32_t bytes_len,
    uint64_t hash
)
{
    uint32_t left = 0;
    uint32_t right = bucket->entry_count;

    while (left < right)
    {
        uint32_t middle = left + (right - left) / 2;
        const ii42_lexicon_cow_entry *entry = &bucket->entries[middle];
        int order;

        if (entry->hash < hash)
        {
            order = -1;
        }
        else if (entry->hash > hash)
        {
            order = 1;
        }
        else
        {
            order = ii42_lexicon_cow_compare_bytes(
                entry->bytes,
                entry->bytes_len,
                bytes,
                bytes_len
            );
        }
        if (order < 0)
        {
            left = middle + 1;
        }
        else
        {
            right = middle;
        }
    }
    if (left < bucket->entry_count)
    {
        const ii42_lexicon_cow_entry *entry = &bucket->entries[left];

        if (entry->hash == hash && entry->bytes_len == bytes_len &&
            (bytes_len == 0 ||
             memcmp(entry->bytes, bytes, bytes_len) == 0))
        {
            return entry;
        }
    }
    return NULL;
}

static ii42_lexicon_cow_child *
ii42_lexicon_cow_node_find_child_mutable(
    ii42_lexicon_cow_node *node,
    uint16_t slot
)
{
    return (ii42_lexicon_cow_child *) ii42_lexicon_cow_node_find_child(
        node,
        slot
    );
}

static ii42_status
ii42_lexicon_cow_lookup_prehashed_at(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *root,
    const uint8_t *bytes,
    uint32_t bytes_len,
    uint64_t hash,
    uint32_t *term_id_out,
    bool *found_out
)
{
    ii42_lexicon_cow_ref current;
    uint16_t expected_level = 0;
    uint64_t expected_prefix = 0;

    if (tree == NULL || root == NULL || found_out == NULL ||
        term_id_out == NULL || (bytes_len > 0 && bytes == NULL))
    {
        return II42_ERR_INVALID;
    }
    *found_out = false;
    *term_id_out = 0;
    if (root->kind == II42_LEXICON_COW_OBJECT_INVALID)
    {
        return II42_OK;
    }
    current = *root;
    for (;;)
    {
        const ii42_lexicon_cow_object *object =
            ii42_lexicon_cow_tree_find_object(tree, &current);

        if (object == NULL ||
            ii42_lexicon_cow_object_checksum_validate(object) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
        {
            const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;
            const ii42_lexicon_cow_entry *entry;

            if (bucket->depth != expected_level ||
                bucket->prefix != expected_prefix)
            {
                return II42_ERR_FORMAT;
            }
            entry = ii42_lexicon_cow_bucket_find_entry(
                bucket,
                bytes,
                bytes_len,
                hash
            );
            if (entry != NULL)
            {
                *term_id_out = entry->term_id;
                *found_out = true;
            }
            return II42_OK;
        }
        else
        {
            const ii42_lexicon_cow_node *node = &object->value.node;
            uint16_t slot;
            const ii42_lexicon_cow_child *child;

            if (node->level != expected_level ||
                node->prefix != expected_prefix ||
                expected_level >= II42_LEXICON_COW_RADIX_LEVELS)
            {
                return II42_ERR_FORMAT;
            }
            slot = ii42_lexicon_cow_hash_slot(hash, node->level);
            child = ii42_lexicon_cow_node_find_child(node, slot);

            if (child == NULL)
            {
                return II42_OK;
            }
            expected_prefix |= (uint64_t) slot <<
                ((uint32_t) expected_level *
                 II42_LEXICON_COW_RADIX_BITS);
            expected_level++;
            current = child->ref;
        }
    }
}

ii42_status
ii42_lexicon_cow_tree_lookup_at(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *root,
    const uint8_t *bytes,
    size_t bytes_len,
    uint32_t *term_id_out,
    bool *found_out
)
{
    if (tree == NULL || bytes_len > UINT32_MAX)
    {
        return II42_ERR_RANGE;
    }
    return ii42_lexicon_cow_lookup_prehashed_at(
        tree,
        root,
        bytes,
        (uint32_t) bytes_len,
        ii42_lexicon_cow_hash(bytes, bytes_len, tree->hash_seed),
        term_id_out,
        found_out
    );
}

ii42_status
ii42_lexicon_cow_tree_lookup(
    const ii42_lexicon_cow_tree *tree,
    const uint8_t *bytes,
    size_t bytes_len,
    uint32_t *term_id_out,
    bool *found_out
)
{
    if (tree == NULL)
    {
        return II42_ERR_INVALID;
    }
    return ii42_lexicon_cow_tree_lookup_at(
        tree,
        &tree->root,
        bytes,
        bytes_len,
        term_id_out,
        found_out
    );
}

static ii42_status
ii42_lexicon_cow_load_patch_object(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *ref,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_object *loaded,
    const ii42_lexicon_cow_object **object_out,
    bool *owned_out
)
{
    const ii42_lexicon_cow_object *object =
        ii42_lexicon_cow_tree_find_object(tree, ref);
    ii42_status status;

    *object_out = NULL;
    *owned_out = false;
    if (object != NULL)
    {
        if (ii42_lexicon_cow_object_checksum_validate(object) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        *object_out = object;
        return II42_OK;
    }
    if (loader == NULL)
    {
        return II42_ERR_FORMAT;
    }
    memset(loaded, 0, sizeof(*loaded));
    status = loader(loader_context, ref, loaded);
    if (status != II42_OK)
    {
        return status;
    }
    if (!ii42_lexicon_cow_refs_equal(&loaded->ref, ref) ||
        ii42_lexicon_cow_object_checksum_validate(loaded) != II42_OK)
    {
        ii42_lexicon_cow_object_free(loaded);
        return II42_ERR_FORMAT;
    }
    *object_out = loaded;
    *owned_out = true;
    return II42_OK;
}

static ii42_status
ii42_lexicon_cow_patch_subtree(
    ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *old_ref,
    ii42_lexicon_cow_entry *const *updates,
    uint32_t update_count,
    uint16_t expected_depth,
    uint64_t expected_prefix,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_update_stats *stats,
    ii42_lexicon_cow_ref *ref_out
)
{
    ii42_lexicon_cow_object loaded = {0};
    const ii42_lexicon_cow_object *old_object;
    bool owned = false;
    ii42_status status;

    if (update_count == 0)
    {
        *ref_out = *old_ref;
        return II42_OK;
    }
    status = ii42_lexicon_cow_load_patch_object(
        tree,
        old_ref,
        loader,
        loader_context,
        &loaded,
        &old_object,
        &owned
    );
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_lexicon_cow_tree_add_retired_ref(tree, old_ref);
    if (status != II42_OK)
    {
        if (owned)
        {
            ii42_lexicon_cow_object_free(&loaded);
        }
        return status;
    }
    if (old_object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        const ii42_lexicon_cow_bucket *old_bucket =
            &old_object->value.bucket;
        ii42_lexicon_cow_entry **combined;
        ii42_status build_status;

        if (old_bucket->depth != expected_depth ||
            old_bucket->prefix != expected_prefix)
        {
            if (owned)
            {
                ii42_lexicon_cow_object_free(&loaded);
            }
            return II42_ERR_FORMAT;
        }
        stats->read_buckets++;
        if ((uint64_t) old_bucket->entry_count + update_count > UINT32_MAX)
        {
            if (owned)
            {
                ii42_lexicon_cow_object_free(&loaded);
            }
            return II42_ERR_RANGE;
        }
        combined = malloc(
            ((size_t) old_bucket->entry_count + update_count) *
            sizeof(*combined)
        );
        if (combined == NULL)
        {
            if (owned)
            {
                ii42_lexicon_cow_object_free(&loaded);
            }
            return II42_ERR_NOMEM;
        }
        for (uint32_t index = 0; index < old_bucket->entry_count; index++)
        {
            combined[index] = &old_bucket->entries[index];
        }
        memcpy(
            combined + old_bucket->entry_count,
            updates,
            (size_t) update_count * sizeof(*updates)
        );
        build_status = ii42_lexicon_cow_build_subtree(
            tree,
            combined,
            old_bucket->entry_count + update_count,
            old_bucket->depth,
            old_bucket->prefix,
            owner_manifest_id,
            stats,
            ref_out
        );
        free(combined);
        if (owned)
        {
            ii42_lexicon_cow_object_free(&loaded);
        }
        return build_status;
    }
    else
    {
        ii42_lexicon_cow_node next_node = old_object->value.node;
        uint32_t counts[II42_LEXICON_COW_RADIX_FANOUT] = {0};
        uint32_t offsets[II42_LEXICON_COW_RADIX_FANOUT] = {0};
        uint32_t cursors[II42_LEXICON_COW_RADIX_FANOUT] = {0};
        ii42_lexicon_cow_entry **ordered;
        ii42_lexicon_cow_object object = {0};
        ii42_status status = II42_OK;

        if (next_node.level != expected_depth ||
            next_node.prefix != expected_prefix ||
            expected_depth >= II42_LEXICON_COW_RADIX_LEVELS)
        {
            if (owned)
            {
                ii42_lexicon_cow_object_free(&loaded);
            }
            return II42_ERR_FORMAT;
        }
        if (owned)
        {
            ii42_lexicon_cow_object_free(&loaded);
            owned = false;
        }
        stats->read_nodes++;
        for (uint32_t index = 0; index < update_count; index++)
        {
            counts[ii42_lexicon_cow_hash_slot(
                updates[index]->hash,
                next_node.level
            )]++;
        }
        for (uint32_t slot = 1;
             slot < II42_LEXICON_COW_RADIX_FANOUT;
             slot++)
        {
            offsets[slot] = offsets[slot - 1] + counts[slot - 1];
        }
        memcpy(cursors, offsets, sizeof(cursors));
        ordered = malloc((size_t) update_count * sizeof(*ordered));
        if (ordered == NULL)
        {
            return II42_ERR_NOMEM;
        }
        for (uint32_t index = 0; index < update_count; index++)
        {
            uint16_t slot = ii42_lexicon_cow_hash_slot(
                updates[index]->hash,
                next_node.level
            );

            ordered[cursors[slot]++] = updates[index];
        }
        for (uint32_t slot = 0;
             slot < II42_LEXICON_COW_RADIX_FANOUT;
             slot++)
        {
            ii42_lexicon_cow_child *child;
            ii42_lexicon_cow_ref next_ref;

            if (counts[slot] == 0)
            {
                continue;
            }
            child = ii42_lexicon_cow_node_find_child_mutable(
                &next_node,
                (uint16_t) slot
            );
            if (child == NULL)
            {
                uint32_t insert_at = 0;
                uint64_t child_prefix;

                if (next_node.child_count >=
                    II42_LEXICON_COW_RADIX_FANOUT)
                {
                    status = II42_ERR_FORMAT;
                    break;
                }
                while (insert_at < next_node.child_count &&
                       next_node.children[insert_at].slot < slot)
                {
                    insert_at++;
                }
                memmove(
                    &next_node.children[insert_at + 1],
                    &next_node.children[insert_at],
                    (size_t) (next_node.child_count - insert_at) *
                        sizeof(*next_node.children)
                );
                memset(
                    &next_node.children[insert_at],
                    0,
                    sizeof(*next_node.children)
                );
                child = &next_node.children[insert_at];
                child_prefix = next_node.prefix |
                    ((uint64_t) slot <<
                     ((uint32_t) next_node.level *
                      II42_LEXICON_COW_RADIX_BITS));
                status = ii42_lexicon_cow_build_subtree(
                    tree,
                    ordered + offsets[slot],
                    counts[slot],
                    next_node.level + 1,
                    child_prefix,
                    owner_manifest_id,
                    stats,
                    &next_ref
                );
                if (status != II42_OK)
                {
                    break;
                }
                child->slot = (uint16_t) slot;
                child->term_count = counts[slot];
                child->ref = next_ref;
                next_node.child_count++;
            }
            else
            {
                status = ii42_lexicon_cow_patch_subtree(
                    tree,
                    &child->ref,
                    ordered + offsets[slot],
                    counts[slot],
                    expected_depth + 1,
                    expected_prefix |
                        ((uint64_t) slot <<
                         ((uint32_t) expected_depth *
                          II42_LEXICON_COW_RADIX_BITS)),
                    owner_manifest_id,
                    loader,
                    loader_context,
                    stats,
                    &next_ref
                );
                if (status != II42_OK)
                {
                    break;
                }
                child->ref = next_ref;
                child->term_count += counts[slot];
            }
        }
        free(ordered);
        if (status != II42_OK)
        {
            return status;
        }
        next_node.term_count += update_count;
        object.ref.kind = II42_LEXICON_COW_OBJECT_NODE;
        object.ref.owner_manifest_id = owner_manifest_id;
        object.value.node = next_node;
        return ii42_lexicon_cow_tree_store_object(
            tree,
            &object,
            stats,
            ref_out
        );
    }
}

ii42_status
ii42_lexicon_cow_tree_append(
    ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *old_root,
    uint32_t old_term_count,
    const ii42_lexicon_cow_key *keys,
    uint32_t key_count,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_update_stats *stats_out
)
{
    ii42_lexicon_cow_entry *entries = NULL;
    ii42_lexicon_cow_entry **entry_refs = NULL;
    ii42_lexicon_cow_update_stats stats = {0};
    ii42_lexicon_cow_ref next_root = {0};
    ii42_status status;

    if (tree == NULL || old_root == NULL || owner_manifest_id == 0 ||
        tree->term_count != old_term_count ||
        (old_term_count == 0 &&
         old_root->kind != II42_LEXICON_COW_OBJECT_INVALID) ||
        (old_term_count > 0 &&
         ii42_lexicon_cow_tree_find_object(tree, old_root) == NULL))
    {
        return II42_ERR_INVALID;
    }
    if (stats_out != NULL)
    {
        memset(stats_out, 0, sizeof(*stats_out));
    }
    if (key_count == 0)
    {
        tree->root = *old_root;
        return II42_OK;
    }
    status = ii42_lexicon_cow_prepare_entries(
        keys,
        key_count,
        old_term_count,
        tree->hash_seed,
        &entries,
        &entry_refs
    );
    if (status != II42_OK)
    {
        return status;
    }
    for (uint32_t index = 0; index < key_count; index++)
    {
        uint32_t existing_id;
        bool found;

        status = ii42_lexicon_cow_lookup_prehashed_at(
            tree,
            old_root,
            entries[index].bytes,
            entries[index].bytes_len,
            entries[index].hash,
            &existing_id,
            &found
        );
        if (status != II42_OK || found)
        {
            free(entry_refs);
            free(entries);
            return status == II42_OK ? II42_ERR_FORMAT : status;
        }
    }
    stats.changed_terms = key_count;
    if (old_term_count == 0)
    {
        status = ii42_lexicon_cow_build_subtree(
            tree,
            entry_refs,
            key_count,
            0,
            0,
            owner_manifest_id,
            &stats,
            &next_root
        );
    }
    else
    {
        status = ii42_lexicon_cow_patch_subtree(
            tree,
            old_root,
            entry_refs,
            key_count,
            0,
            0,
            owner_manifest_id,
            NULL,
            NULL,
            &stats,
            &next_root
        );
    }
    free(entry_refs);
    free(entries);
    if (status != II42_OK)
    {
        return status;
    }
    tree->root = next_root;
    tree->term_count = old_term_count + key_count;
    if (stats_out != NULL)
    {
        *stats_out = stats;
    }
    return II42_OK;
}

typedef struct ii42_lexicon_cow_validation_state
{
    const ii42_lexicon_cow_tree *tree;
    bool *seen_term_ids;
    uint32_t expected_term_count;
    ii42_lexicon_cow_ref *visited;
    size_t visited_count;
    size_t visited_capacity;
} ii42_lexicon_cow_validation_state;

static ii42_status
ii42_lexicon_cow_validation_mark(
    ii42_lexicon_cow_validation_state *state,
    const ii42_lexicon_cow_ref *ref
)
{
    ii42_lexicon_cow_ref *next_visited;
    size_t next_capacity;

    for (size_t index = 0; index < state->visited_count; index++)
    {
        if (ii42_lexicon_cow_refs_equal(&state->visited[index], ref))
        {
            return II42_ERR_FORMAT;
        }
    }
    if (state->visited_count == state->visited_capacity)
    {
        next_capacity = state->visited_capacity == 0
            ? 16
            : state->visited_capacity * 2;
        if (next_capacity < state->visited_capacity ||
            next_capacity > SIZE_MAX / sizeof(*state->visited))
        {
            return II42_ERR_RANGE;
        }
        next_visited = realloc(
            state->visited,
            next_capacity * sizeof(*state->visited)
        );
        if (next_visited == NULL)
        {
            return II42_ERR_NOMEM;
        }
        state->visited = next_visited;
        state->visited_capacity = next_capacity;
    }
    state->visited[state->visited_count++] = *ref;
    return II42_OK;
}

static ii42_status
ii42_lexicon_cow_validate_subtree(
    ii42_lexicon_cow_validation_state *state,
    const ii42_lexicon_cow_ref *ref,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint32_t *term_count_out
)
{
    const ii42_lexicon_cow_object *object;
    ii42_status status;

    status = ii42_lexicon_cow_validation_mark(state, ref);
    if (status != II42_OK)
    {
        return status;
    }
    object = ii42_lexicon_cow_tree_find_object(state->tree, ref);
    if (object == NULL ||
        ii42_lexicon_cow_object_checksum_validate(object) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;

        if (bucket->depth != expected_level ||
            bucket->prefix != expected_prefix)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < bucket->entry_count; index++)
        {
            const ii42_lexicon_cow_entry *entry = &bucket->entries[index];

            if (entry->term_id >= state->expected_term_count ||
                state->seen_term_ids[entry->term_id] ||
                entry->hash != ii42_lexicon_cow_hash(
                    entry->bytes,
                    entry->bytes_len,
                    state->tree->hash_seed
                ))
            {
                return II42_ERR_FORMAT;
            }
            state->seen_term_ids[entry->term_id] = true;
        }
        *term_count_out = bucket->entry_count;
        return II42_OK;
    }
    else
    {
        const ii42_lexicon_cow_node *node = &object->value.node;
        uint32_t total = 0;

        if (node->level != expected_level ||
            node->prefix != expected_prefix)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < node->child_count; index++)
        {
            const ii42_lexicon_cow_child *child = &node->children[index];
            uint64_t child_prefix = expected_prefix |
                ((uint64_t) child->slot <<
                 ((uint32_t) expected_level *
                  II42_LEXICON_COW_RADIX_BITS));
            uint32_t child_terms = 0;

            status = ii42_lexicon_cow_validate_subtree(
                state,
                &child->ref,
                expected_level + 1,
                child_prefix,
                &child_terms
            );
            if (status != II42_OK || child_terms != child->term_count ||
                UINT32_MAX - total < child_terms)
            {
                return status == II42_OK ? II42_ERR_FORMAT : status;
            }
            total += child_terms;
        }
        if (total != node->term_count)
        {
            return II42_ERR_FORMAT;
        }
        *term_count_out = total;
        return II42_OK;
    }
}

ii42_status
ii42_lexicon_cow_tree_validate_at(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *root,
    uint32_t expected_term_count
)
{
    ii42_lexicon_cow_validation_state state = {0};
    uint32_t term_count = 0;
    ii42_status status;

    if (tree == NULL || root == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (expected_term_count == 0)
    {
        static const ii42_lexicon_cow_ref zero_ref = {0};

        return memcmp(root, &zero_ref, sizeof(*root)) == 0
            ? II42_OK
            : II42_ERR_FORMAT;
    }
    if (!ii42_lexicon_cow_ref_valid(root))
    {
        return II42_ERR_FORMAT;
    }
    state.tree = tree;
    state.expected_term_count = expected_term_count;
    state.seen_term_ids = calloc(
        expected_term_count,
        sizeof(*state.seen_term_ids)
    );
    if (state.seen_term_ids == NULL)
    {
        return II42_ERR_NOMEM;
    }
    status = ii42_lexicon_cow_validate_subtree(
        &state,
        root,
        0,
        0,
        &term_count
    );
    free(state.seen_term_ids);
    free(state.visited);
    if (status != II42_OK)
    {
        return status;
    }
    return term_count == expected_term_count
        ? II42_OK
        : II42_ERR_FORMAT;
}

ii42_status
ii42_lexicon_cow_tree_object_serialize(
    const ii42_lexicon_cow_tree *tree,
    const ii42_lexicon_cow_ref *ref,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    const ii42_lexicon_cow_object *object;
    uint64_t checksum;
    uint64_t blob_checksum;
    ii42_status status;

    if (tree == NULL || ref == NULL)
    {
        return II42_ERR_INVALID;
    }
    object = ii42_lexicon_cow_tree_find_object(tree, ref);
    if (object == NULL)
    {
        return II42_ERR_FORMAT;
    }
    status = ii42_lexicon_cow_object_encode(
        object,
        true,
        bytes_out,
        size_out
    );
    if (status != II42_OK)
    {
        return status;
    }
    checksum = ii42_lexicon_cow_checksum(*bytes_out, *size_out);
    blob_checksum = ii42_segment_blob_checksum(*bytes_out, *size_out);
    if (checksum != object->ref.checksum ||
        blob_checksum != object->ref.blob_checksum)
    {
        free(*bytes_out);
        *bytes_out = NULL;
        *size_out = 0;
        return II42_ERR_FORMAT;
    }
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_object_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_lexicon_cow_object *object_out
)
{
    ii42_lexicon_cow_object object = {0};
    ii42_lexicon_cow_object_kind kind;
    uint16_t level;
    uint32_t count;
    uint32_t term_count;
    uint64_t prefix;
    size_t expected_size;
    ii42_status status;

    if (bytes == NULL || object_out == NULL ||
        size < II42_LEXICON_COW_HEADER_SIZE ||
        ii42_lexicon_cow_read_u32(bytes + 0) !=
            II42_LEXICON_COW_MAGIC ||
        ii42_lexicon_cow_read_u16(bytes + 4) !=
            II42_LEXICON_COW_VERSION ||
        ii42_lexicon_cow_read_u16(bytes + 6) !=
            II42_LEXICON_COW_HEADER_SIZE ||
        ii42_lexicon_cow_read_u32(bytes + 12) != 0 ||
        ii42_lexicon_cow_read_u64(bytes + 32) != size ||
        ii42_lexicon_cow_read_u64(bytes + 40) == 0 ||
        ii42_lexicon_cow_checksum(bytes, size) !=
            ii42_lexicon_cow_read_u64(bytes + 40))
    {
        return II42_ERR_FORMAT;
    }
    kind = (ii42_lexicon_cow_object_kind)
        ii42_lexicon_cow_read_u16(bytes + 8);
    level = ii42_lexicon_cow_read_u16(bytes + 10);
    prefix = ii42_lexicon_cow_read_u64(bytes + 48);
    count = ii42_lexicon_cow_read_u32(bytes + 56);
    term_count = ii42_lexicon_cow_read_u32(bytes + 60);
    object.ref.kind = kind;
    object.ref.object_id = ii42_lexicon_cow_read_u64(bytes + 16);
    object.ref.owner_manifest_id = ii42_lexicon_cow_read_u64(bytes + 24);
    object.ref.object_bytes = size;
    object.ref.checksum = ii42_lexicon_cow_read_u64(bytes + 40);
    object.ref.blob_checksum = ii42_segment_blob_checksum(bytes, size);

    if (kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        if (count > II42_LEXICON_COW_RADIX_FANOUT)
        {
            return II42_ERR_FORMAT;
        }
        expected_size = II42_LEXICON_COW_HEADER_SIZE +
            (size_t) count * II42_LEXICON_COW_CHILD_SIZE;
        if (expected_size != size)
        {
            return II42_ERR_FORMAT;
        }
        object.value.node.level = level;
        object.value.node.prefix = prefix;
        object.value.node.child_count = count;
        object.value.node.term_count = term_count;
        for (uint32_t index = 0; index < count; index++)
        {
            const uint8_t *child_bytes =
                bytes + II42_LEXICON_COW_HEADER_SIZE +
                (size_t) index * II42_LEXICON_COW_CHILD_SIZE;
            ii42_lexicon_cow_child *child =
                &object.value.node.children[index];

            child->slot = ii42_lexicon_cow_read_u16(child_bytes + 0);
            child->reserved = ii42_lexicon_cow_read_u16(child_bytes + 2);
            child->term_count = ii42_lexicon_cow_read_u32(child_bytes + 4);
            ii42_lexicon_cow_decode_ref(child_bytes + 8, &child->ref);
        }
    }
    else if (kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        size_t offset = II42_LEXICON_COW_HEADER_SIZE;
        size_t entry_headers_size;
        size_t owned_bytes_size;
        size_t owned_bytes_offset = 0;

        if (term_count != count || count == 0)
        {
            return II42_ERR_FORMAT;
        }
        if ((size_t) count >
                (SIZE_MAX - II42_LEXICON_COW_HEADER_SIZE) /
                    II42_LEXICON_COW_ENTRY_HEADER_SIZE)
        {
            return II42_ERR_RANGE;
        }
        entry_headers_size =
            (size_t) count * II42_LEXICON_COW_ENTRY_HEADER_SIZE;
        if (size < II42_LEXICON_COW_HEADER_SIZE + entry_headers_size)
        {
            return II42_ERR_FORMAT;
        }
        owned_bytes_size = size - II42_LEXICON_COW_HEADER_SIZE -
            entry_headers_size;
        object.value.bucket.depth = level;
        object.value.bucket.prefix = prefix;
        object.value.bucket.entry_count = count;
        object.value.bucket.entries = calloc(
            count,
            sizeof(*object.value.bucket.entries)
        );
        if (object.value.bucket.entries == NULL)
        {
            return II42_ERR_NOMEM;
        }
        if (owned_bytes_size > 0)
        {
            object.value.bucket.owned_bytes = malloc(owned_bytes_size);
            if (object.value.bucket.owned_bytes == NULL)
            {
                ii42_lexicon_cow_object_free(&object);
                return II42_ERR_NOMEM;
            }
        }
        for (uint32_t index = 0; index < count; index++)
        {
            ii42_lexicon_cow_entry *entry =
                &object.value.bucket.entries[index];

            if (size - offset < II42_LEXICON_COW_ENTRY_HEADER_SIZE)
            {
                ii42_lexicon_cow_object_free(&object);
                return II42_ERR_FORMAT;
            }
            entry->hash = ii42_lexicon_cow_read_u64(bytes + offset);
            entry->term_id = ii42_lexicon_cow_read_u32(bytes + offset + 8);
            entry->bytes_len =
                ii42_lexicon_cow_read_u32(bytes + offset + 12);
            offset += II42_LEXICON_COW_ENTRY_HEADER_SIZE;
            if (entry->bytes_len > size - offset)
            {
                ii42_lexicon_cow_object_free(&object);
                return II42_ERR_FORMAT;
            }
            if (entry->bytes_len > 0)
            {
                if (owned_bytes_offset > owned_bytes_size ||
                    entry->bytes_len >
                    owned_bytes_size - owned_bytes_offset)
                {
                    ii42_lexicon_cow_object_free(&object);
                    return II42_ERR_FORMAT;
                }
                entry->bytes = object.value.bucket.owned_bytes +
                    owned_bytes_offset;
                memcpy(entry->bytes, bytes + offset, entry->bytes_len);
                owned_bytes_offset += entry->bytes_len;
                offset += entry->bytes_len;
            }
        }
        if (offset != size || owned_bytes_offset != owned_bytes_size)
        {
            ii42_lexicon_cow_object_free(&object);
            return II42_ERR_FORMAT;
        }
    }
    else
    {
        return II42_ERR_FORMAT;
    }
    status = ii42_lexicon_cow_object_shape_validate(&object, true);
    if (status != II42_OK)
    {
        ii42_lexicon_cow_object_free(&object);
        return status;
    }
    *object_out = object;
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_tree_prepare_object_for_storage(
    ii42_lexicon_cow_tree *tree,
    uint64_t object_id,
    uint8_t **bytes_out,
    size_t *size_out
)
{
    ii42_lexicon_cow_object *object;
    ii42_lexicon_cow_object prepared;
    uint64_t object_offset;
    ii42_status status;

    if (tree == NULL || bytes_out == NULL || size_out == NULL ||
        object_id == 0 || object_id > tree->object_count)
    {
        return II42_ERR_INVALID;
    }
    object_offset = object_id - 1;
    *bytes_out = NULL;
    *size_out = 0;
    object = &tree->objects[object_offset];
    if (object->ref.object_id != object_id ||
        !ii42_lexicon_cow_ref_is_unbound(&object->ref))
    {
        return II42_ERR_FORMAT;
    }
    prepared = *object;
    if (prepared.ref.kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        ii42_lexicon_cow_node *node = &prepared.value.node;

        for (uint32_t index = 0; index < node->child_count; index++)
        {
            ii42_lexicon_cow_child *child = &node->children[index];
            const ii42_lexicon_cow_object *stored_child;

            if (!ii42_lexicon_cow_ref_is_unbound(&child->ref))
            {
                if (!ii42_lexicon_cow_ref_valid(&child->ref))
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
            stored_child = &tree->objects[
                child->ref.object_id - 1
            ];
            if (stored_child->ref.kind != child->ref.kind ||
                stored_child->ref.object_id != child->ref.object_id ||
                ii42_lexicon_cow_ref_is_unbound(&stored_child->ref) ||
                !ii42_lexicon_cow_ref_valid(&stored_child->ref))
            {
                return II42_ERR_FORMAT;
            }
            child->ref = stored_child->ref;
        }
    }
    status = ii42_lexicon_cow_object_refresh(&prepared);
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_lexicon_cow_object_encode(
        &prepared,
        true,
        bytes_out,
        size_out
    );
    if (status != II42_OK)
    {
        return status;
    }
    *object = prepared;
    if (tree->root.object_id == object_id)
    {
        tree->root = object->ref;
    }
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_object_bind_storage(
    ii42_lexicon_cow_object *object,
    const ii42_segment_object_ref *storage_ref
)
{
    ii42_lexicon_cow_object bound;

    if (object == NULL || storage_ref == NULL ||
        !ii42_lexicon_cow_ref_is_unbound(&object->ref) ||
        storage_ref->object_kind !=
            II42_SEGMENT_OBJECT_LEXICON_LOOKUP ||
        storage_ref->object_id != object->ref.object_id ||
        storage_ref->owner_manifest_id !=
            object->ref.owner_manifest_id ||
        storage_ref->object_bytes != object->ref.object_bytes ||
        storage_ref->object_checksum != object->ref.blob_checksum ||
        ii42_segment_object_ref_validate(
            storage_ref,
            UINT32_MAX
        ) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    bound = *object;
    bound.ref.start_block = storage_ref->start_block;
    bound.ref.page_count = storage_ref->page_count;
    if (!ii42_lexicon_cow_ref_valid(&bound.ref) ||
        ii42_lexicon_cow_object_checksum_validate(&bound) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    *object = bound;
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_tree_bind_object_storage(
    ii42_lexicon_cow_tree *tree,
    uint64_t object_id,
    const ii42_segment_object_ref *storage_ref
)
{
    ii42_lexicon_cow_object *object;
    uint64_t object_offset;
    ii42_status status;

    if (tree == NULL || object_id == 0 ||
        object_id > tree->object_count)
    {
        return II42_ERR_INVALID;
    }
    object_offset = object_id - 1;
    object = &tree->objects[object_offset];
    status = ii42_lexicon_cow_object_bind_storage(object, storage_ref);
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
ii42_lexicon_cow_ref_as_segment_object_ref(
    const ii42_lexicon_cow_ref *ref,
    ii42_segment_object_ref *storage_ref_out
)
{
    ii42_segment_object_ref storage_ref;

    if (storage_ref_out == NULL || !ii42_lexicon_cow_ref_valid(ref) ||
        ii42_lexicon_cow_ref_is_unbound(ref))
    {
        return II42_ERR_INVALID;
    }
    memset(&storage_ref, 0, sizeof(storage_ref));
    storage_ref.object_kind = II42_SEGMENT_OBJECT_LEXICON_LOOKUP;
    storage_ref.start_block = ref->start_block;
    storage_ref.page_count = ref->page_count;
    storage_ref.object_id = ref->object_id;
    storage_ref.owner_manifest_id = ref->owner_manifest_id;
    storage_ref.object_bytes = ref->object_bytes;
    storage_ref.object_checksum = ref->blob_checksum;
    if (ii42_segment_object_ref_validate(
            &storage_ref,
            UINT32_MAX
        ) != II42_OK)
    {
        return II42_ERR_FORMAT;
    }
    *storage_ref_out = storage_ref;
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_lookup_external(
    const ii42_lexicon_cow_ref *root,
    uint64_t hash_seed,
    const uint8_t *bytes,
    size_t bytes_len,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    uint32_t *term_id_out,
    bool *found_out
)
{
    ii42_lexicon_cow_ref current;
    uint64_t hash;
    uint64_t expected_prefix = 0;
    uint16_t expected_level = 0;

    if (root == NULL || loader == NULL || term_id_out == NULL ||
        found_out == NULL || bytes_len > UINT32_MAX ||
        (bytes_len > 0 && bytes == NULL) ||
        !ii42_lexicon_cow_ref_valid(root) ||
        ii42_lexicon_cow_ref_is_unbound(root))
    {
        return II42_ERR_INVALID;
    }
    *term_id_out = 0;
    *found_out = false;
    current = *root;
    hash = ii42_lexicon_cow_hash(bytes, bytes_len, hash_seed);

    for (uint16_t hop = 0;
         hop <= II42_LEXICON_COW_RADIX_LEVELS;
         hop++)
    {
        ii42_lexicon_cow_object loaded = {0};
        const ii42_lexicon_cow_object *object;
        bool owned = false;
        ii42_status status;

        if (ii42_lexicon_cow_ref_is_unbound(&current))
        {
            return II42_ERR_FORMAT;
        }
        status = ii42_lexicon_cow_load_patch_object(
            NULL,
            &current,
            loader,
            loader_context,
            &loaded,
            &object,
            &owned
        );
        if (status != II42_OK)
        {
            return status;
        }
        if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
        {
            const ii42_lexicon_cow_bucket *bucket =
                &object->value.bucket;
            const ii42_lexicon_cow_entry *entry;

            if (bucket->depth != expected_level ||
                bucket->prefix != expected_prefix)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return II42_ERR_FORMAT;
            }
            entry = ii42_lexicon_cow_bucket_find_entry(
                bucket,
                bytes,
                (uint32_t) bytes_len,
                hash
            );
            if (entry != NULL)
            {
                *term_id_out = entry->term_id;
                *found_out = true;
            }
            ii42_lexicon_cow_object_free(&loaded);
            return II42_OK;
        }
        else
        {
            const ii42_lexicon_cow_node *node = &object->value.node;
            const ii42_lexicon_cow_child *child;
            uint16_t slot;

            if (node->level != expected_level ||
                node->prefix != expected_prefix ||
                expected_level >= II42_LEXICON_COW_RADIX_LEVELS)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return II42_ERR_FORMAT;
            }
            slot = ii42_lexicon_cow_hash_slot(hash, node->level);
            child = ii42_lexicon_cow_node_find_child(node, slot);
            if (child == NULL)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return II42_OK;
            }
            current = child->ref;
            expected_prefix |= (uint64_t) slot <<
                ((uint32_t) expected_level *
                 II42_LEXICON_COW_RADIX_BITS);
            expected_level++;
            ii42_lexicon_cow_object_free(&loaded);
        }
    }
    return II42_ERR_FORMAT;
}

typedef struct ii42_lexicon_cow_serialized_probe
{
    ii42_lexicon_cow_object_kind kind;
    bool child_found;
    bool term_found;
    uint32_t term_id;
    ii42_lexicon_cow_ref child;
} ii42_lexicon_cow_serialized_probe;

static ii42_status
ii42_lexicon_cow_probe_serialized_object(
    const ii42_lexicon_cow_ref *ref,
    const uint8_t *object_bytes,
    size_t object_size,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint64_t hash,
    const uint8_t *key_bytes,
    uint32_t key_bytes_len,
    ii42_lexicon_cow_serialized_probe *probe_out
)
{
    ii42_lexicon_cow_serialized_probe probe = {0};
    ii42_lexicon_cow_object_kind kind;
    uint16_t level;
    uint32_t count;
    uint32_t term_count;
    uint64_t prefix;

    if (ref == NULL || object_bytes == NULL || probe_out == NULL ||
        object_size < II42_LEXICON_COW_HEADER_SIZE ||
        ii42_lexicon_cow_read_u32(object_bytes + 0) !=
            II42_LEXICON_COW_MAGIC ||
        ii42_lexicon_cow_read_u16(object_bytes + 4) !=
            II42_LEXICON_COW_VERSION ||
        ii42_lexicon_cow_read_u16(object_bytes + 6) !=
            II42_LEXICON_COW_HEADER_SIZE ||
        ii42_lexicon_cow_read_u32(object_bytes + 12) != 0 ||
        ii42_lexicon_cow_read_u64(object_bytes + 16) !=
            ref->object_id ||
        ii42_lexicon_cow_read_u64(object_bytes + 24) !=
            ref->owner_manifest_id ||
        ii42_lexicon_cow_read_u64(object_bytes + 32) != object_size ||
        ref->object_bytes != object_size ||
        ii42_lexicon_cow_read_u64(object_bytes + 40) != ref->checksum ||
        ii42_lexicon_cow_checksum(object_bytes, object_size) !=
            ref->checksum ||
        ii42_segment_blob_checksum(object_bytes, object_size) !=
            ref->blob_checksum)
    {
        return II42_ERR_FORMAT;
    }
    kind = (ii42_lexicon_cow_object_kind)
        ii42_lexicon_cow_read_u16(object_bytes + 8);
    level = ii42_lexicon_cow_read_u16(object_bytes + 10);
    prefix = ii42_lexicon_cow_read_u64(object_bytes + 48);
    count = ii42_lexicon_cow_read_u32(object_bytes + 56);
    term_count = ii42_lexicon_cow_read_u32(object_bytes + 60);
    if (kind != ref->kind || level != expected_level ||
        prefix != expected_prefix ||
        prefix != (prefix & ii42_lexicon_cow_prefix_mask(level)))
    {
        return II42_ERR_FORMAT;
    }
    probe.kind = kind;

    if (kind == II42_LEXICON_COW_OBJECT_NODE)
    {
        uint16_t wanted_slot;
        uint16_t previous_slot = 0;
        uint64_t child_term_count = 0;

        if (level >= II42_LEXICON_COW_RADIX_LEVELS || count == 0 ||
            count > II42_LEXICON_COW_RADIX_FANOUT || term_count == 0 ||
            object_size != II42_LEXICON_COW_HEADER_SIZE +
                (size_t) count * II42_LEXICON_COW_CHILD_SIZE)
        {
            return II42_ERR_FORMAT;
        }
        wanted_slot = ii42_lexicon_cow_hash_slot(hash, level);
        for (uint32_t index = 0; index < count; index++)
        {
            const uint8_t *child_bytes = object_bytes +
                II42_LEXICON_COW_HEADER_SIZE +
                (size_t) index * II42_LEXICON_COW_CHILD_SIZE;
            ii42_lexicon_cow_ref child;
            uint16_t slot = ii42_lexicon_cow_read_u16(child_bytes + 0);
            uint16_t reserved =
                ii42_lexicon_cow_read_u16(child_bytes + 2);
            uint32_t child_terms =
                ii42_lexicon_cow_read_u32(child_bytes + 4);

            ii42_lexicon_cow_decode_ref(child_bytes + 8, &child);
            if (slot >= II42_LEXICON_COW_RADIX_FANOUT || reserved != 0 ||
                child_terms == 0 || !ii42_lexicon_cow_ref_valid(&child) ||
                (index > 0 && previous_slot >= slot))
            {
                return II42_ERR_FORMAT;
            }
            previous_slot = slot;
            child_term_count += child_terms;
            if (slot == wanted_slot)
            {
                probe.child = child;
                probe.child_found = true;
            }
        }
        if (child_term_count != term_count)
        {
            return II42_ERR_FORMAT;
        }
    }
    else if (kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        size_t offset = II42_LEXICON_COW_HEADER_SIZE;
        const uint8_t *previous_bytes = NULL;
        uint32_t previous_bytes_len = 0;
        uint64_t previous_hash = 0;
        uint64_t mask = ii42_lexicon_cow_prefix_mask(level);

        if (count == 0 || term_count != count ||
            count > (object_size - II42_LEXICON_COW_HEADER_SIZE) /
                II42_LEXICON_COW_ENTRY_HEADER_SIZE)
        {
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < count; index++)
        {
            uint64_t entry_hash;
            uint32_t entry_term_id;
            uint32_t entry_bytes_len;
            const uint8_t *entry_bytes;
            int order = -1;

            if (object_size - offset <
                II42_LEXICON_COW_ENTRY_HEADER_SIZE)
            {
                return II42_ERR_FORMAT;
            }
            entry_hash = ii42_lexicon_cow_read_u64(
                object_bytes + offset
            );
            entry_term_id = ii42_lexicon_cow_read_u32(
                object_bytes + offset + 8
            );
            entry_bytes_len = ii42_lexicon_cow_read_u32(
                object_bytes + offset + 12
            );
            offset += II42_LEXICON_COW_ENTRY_HEADER_SIZE;
            if (entry_bytes_len > object_size - offset ||
                (entry_hash & mask) != prefix)
            {
                return II42_ERR_FORMAT;
            }
            entry_bytes = object_bytes + offset;
            if (index > 0)
            {
                order = previous_hash < entry_hash
                    ? -1
                    : previous_hash > entry_hash
                        ? 1
                        : ii42_lexicon_cow_compare_bytes(
                            previous_bytes,
                            previous_bytes_len,
                            entry_bytes,
                            entry_bytes_len
                        );
                if (order >= 0)
                {
                    return II42_ERR_FORMAT;
                }
            }
            if (entry_hash == hash && entry_bytes_len == key_bytes_len &&
                (entry_bytes_len == 0 ||
                 memcmp(entry_bytes, key_bytes, entry_bytes_len) == 0))
            {
                probe.term_id = entry_term_id;
                probe.term_found = true;
            }
            previous_hash = entry_hash;
            previous_bytes = entry_bytes;
            previous_bytes_len = entry_bytes_len;
            offset += entry_bytes_len;
        }
        if (offset != object_size)
        {
            return II42_ERR_FORMAT;
        }
    }
    else
    {
        return II42_ERR_FORMAT;
    }
    *probe_out = probe;
    return II42_OK;
}

ii42_status
ii42_lexicon_cow_lookup_serialized_external(
    const ii42_lexicon_cow_object *root_object,
    uint64_t hash_seed,
    const uint8_t *bytes,
    size_t bytes_len,
    ii42_lexicon_cow_serialized_loader loader,
    ii42_lexicon_cow_serialized_releaser releaser,
    void *loader_context,
    uint32_t *term_id_out,
    bool *found_out
)
{
    ii42_lexicon_cow_ref current;
    uint64_t hash;
    uint64_t expected_prefix = 0;
    uint16_t expected_level = 0;

    if (root_object == NULL || loader == NULL || releaser == NULL ||
        term_id_out == NULL || found_out == NULL ||
        bytes_len > UINT32_MAX || (bytes_len > 0 && bytes == NULL) ||
        !ii42_lexicon_cow_ref_valid(&root_object->ref) ||
        ii42_lexicon_cow_ref_is_unbound(&root_object->ref) ||
        ii42_lexicon_cow_object_shape_validate(root_object, true) !=
            II42_OK)
    {
        return II42_ERR_INVALID;
    }
    *term_id_out = 0;
    *found_out = false;
    hash = ii42_lexicon_cow_hash(bytes, bytes_len, hash_seed);

    if (root_object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        const ii42_lexicon_cow_entry *entry =
            ii42_lexicon_cow_bucket_find_entry(
                &root_object->value.bucket,
                bytes,
                (uint32_t) bytes_len,
                hash
            );

        if (entry != NULL)
        {
            *term_id_out = entry->term_id;
            *found_out = true;
        }
        return II42_OK;
    }
    else
    {
        const ii42_lexicon_cow_node *node = &root_object->value.node;
        uint16_t slot = ii42_lexicon_cow_hash_slot(hash, 0);
        const ii42_lexicon_cow_child *child;

        if (node->level != 0 || node->prefix != 0)
        {
            return II42_ERR_FORMAT;
        }
        child = ii42_lexicon_cow_node_find_child(node, slot);
        if (child == NULL)
        {
            return II42_OK;
        }
        current = child->ref;
        expected_prefix = slot;
        expected_level = 1;
    }

    for (uint16_t hop = 0;
         hop <= II42_LEXICON_COW_RADIX_LEVELS;
         hop++)
    {
        ii42_lexicon_cow_serialized_probe probe = {0};
        uint8_t *object_bytes = NULL;
        size_t object_size = 0;
        ii42_status status;

        status = loader(
            loader_context,
            &current,
            &object_bytes,
            &object_size
        );
        if (status != II42_OK)
        {
            return status;
        }
        status = ii42_lexicon_cow_probe_serialized_object(
            &current,
            object_bytes,
            object_size,
            expected_level,
            expected_prefix,
            hash,
            bytes,
            (uint32_t) bytes_len,
            &probe
        );
        releaser(loader_context, object_bytes);
        if (status != II42_OK)
        {
            return status;
        }
        if (probe.kind == II42_LEXICON_COW_OBJECT_BUCKET)
        {
            if (probe.term_found)
            {
                *term_id_out = probe.term_id;
                *found_out = true;
            }
            return II42_OK;
        }
        if (!probe.child_found)
        {
            return II42_OK;
        }
        current = probe.child;
        expected_prefix |= (uint64_t) ii42_lexicon_cow_hash_slot(
            hash,
            expected_level
        ) << ((uint32_t) expected_level *
              II42_LEXICON_COW_RADIX_BITS);
        expected_level++;
    }
    return II42_ERR_FORMAT;
}

typedef struct ii42_lexicon_cow_external_validation_state
{
    uint64_t hash_seed;
    uint32_t expected_term_count;
    bool *seen_term_ids;
    ii42_lexicon_cow_ref *visited;
    size_t visited_count;
    size_t visited_capacity;
    ii42_lexicon_cow_object_loader loader;
    void *loader_context;
    ii42_lexicon_cow_object_visitor visitor;
    void *visitor_context;
} ii42_lexicon_cow_external_validation_state;

static ii42_status
ii42_lexicon_cow_external_validation_mark(
    ii42_lexicon_cow_external_validation_state *state,
    const ii42_lexicon_cow_ref *ref
)
{
    ii42_lexicon_cow_ref *resized;
    size_t next_capacity;

    if (ii42_lexicon_cow_ref_is_unbound(ref))
    {
        return II42_ERR_FORMAT;
    }
    for (size_t index = 0; index < state->visited_count; index++)
    {
        if (ii42_lexicon_cow_refs_equal(&state->visited[index], ref))
        {
            return II42_ERR_FORMAT;
        }
    }
    if (state->visited_count == state->visited_capacity)
    {
        next_capacity = state->visited_capacity == 0
            ? 16
            : state->visited_capacity * 2;
        if (next_capacity < state->visited_capacity ||
            next_capacity > SIZE_MAX / sizeof(*state->visited))
        {
            return II42_ERR_RANGE;
        }
        resized = realloc(
            state->visited,
            next_capacity * sizeof(*state->visited)
        );
        if (resized == NULL)
        {
            return II42_ERR_NOMEM;
        }
        state->visited = resized;
        state->visited_capacity = next_capacity;
    }
    state->visited[state->visited_count++] = *ref;
    return II42_OK;
}

static ii42_status
ii42_lexicon_cow_validate_external_subtree(
    ii42_lexicon_cow_external_validation_state *state,
    const ii42_lexicon_cow_ref *ref,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint32_t *term_count_out
)
{
    ii42_lexicon_cow_object loaded = {0};
    const ii42_lexicon_cow_object *object;
    bool owned = false;
    ii42_status status;

    status = ii42_lexicon_cow_external_validation_mark(state, ref);
    if (status != II42_OK)
    {
        return status;
    }
    status = ii42_lexicon_cow_load_patch_object(
        NULL,
        ref,
        state->loader,
        state->loader_context,
        &loaded,
        &object,
        &owned
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (state->visitor != NULL)
    {
        status = state->visitor(state->visitor_context, object);
        if (status != II42_OK)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return status;
        }
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;

        if (bucket->depth != expected_level ||
            bucket->prefix != expected_prefix)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < bucket->entry_count; index++)
        {
            const ii42_lexicon_cow_entry *entry =
                &bucket->entries[index];

            if (entry->term_id >= state->expected_term_count ||
                state->seen_term_ids[entry->term_id] ||
                entry->hash != ii42_lexicon_cow_hash(
                    entry->bytes,
                    entry->bytes_len,
                    state->hash_seed
                ))
            {
                ii42_lexicon_cow_object_free(&loaded);
                return II42_ERR_FORMAT;
            }
            state->seen_term_ids[entry->term_id] = true;
        }
        *term_count_out = bucket->entry_count;
        ii42_lexicon_cow_object_free(&loaded);
        return II42_OK;
    }
    else
    {
        const ii42_lexicon_cow_node *node = &object->value.node;
        uint32_t total = 0;

        if (node->level != expected_level ||
            node->prefix != expected_prefix)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < node->child_count; index++)
        {
            const ii42_lexicon_cow_child *child = &node->children[index];
            uint64_t child_prefix = expected_prefix |
                ((uint64_t) child->slot <<
                 ((uint32_t) expected_level *
                  II42_LEXICON_COW_RADIX_BITS));
            uint32_t child_terms = 0;

            status = ii42_lexicon_cow_validate_external_subtree(
                state,
                &child->ref,
                expected_level + 1,
                child_prefix,
                &child_terms
            );
            if (status != II42_OK || child_terms != child->term_count ||
                UINT32_MAX - total < child_terms)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return status == II42_OK ? II42_ERR_FORMAT : status;
            }
            total += child_terms;
        }
        if (total != node->term_count)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        *term_count_out = total;
        ii42_lexicon_cow_object_free(&loaded);
        return II42_OK;
    }
}

static ii42_status
ii42_lexicon_cow_validate_external_common(
    const ii42_lexicon_cow_ref *root,
    uint64_t hash_seed,
    uint32_t expected_term_count,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_object_visitor visitor,
    void *visitor_context
)
{
    ii42_lexicon_cow_external_validation_state state = {0};
    uint32_t term_count = 0;
    ii42_status status;

    if (root == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (expected_term_count == 0)
    {
        static const ii42_lexicon_cow_ref zero_ref = {0};

        return memcmp(root, &zero_ref, sizeof(*root)) == 0
            ? II42_OK
            : II42_ERR_FORMAT;
    }
    if (loader == NULL || !ii42_lexicon_cow_ref_valid(root) ||
        ii42_lexicon_cow_ref_is_unbound(root))
    {
        return II42_ERR_INVALID;
    }
    state.hash_seed = hash_seed;
    state.expected_term_count = expected_term_count;
    state.loader = loader;
    state.loader_context = loader_context;
    state.visitor = visitor;
    state.visitor_context = visitor_context;
    state.seen_term_ids = calloc(
        expected_term_count,
        sizeof(*state.seen_term_ids)
    );
    if (state.seen_term_ids == NULL)
    {
        return II42_ERR_NOMEM;
    }
    status = ii42_lexicon_cow_validate_external_subtree(
        &state,
        root,
        0,
        0,
        &term_count
    );
    free(state.seen_term_ids);
    free(state.visited);
    if (status != II42_OK)
    {
        return status;
    }
    return term_count == expected_term_count
        ? II42_OK
        : II42_ERR_FORMAT;
}

ii42_status
ii42_lexicon_cow_validate_external(
    const ii42_lexicon_cow_ref *root,
    uint64_t hash_seed,
    uint32_t expected_term_count,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context
)
{
    return ii42_lexicon_cow_validate_external_common(
        root,
        hash_seed,
        expected_term_count,
        loader,
        loader_context,
        NULL,
        NULL
    );
}

ii42_status
ii42_lexicon_cow_visit_external(
    const ii42_lexicon_cow_ref *root,
    uint64_t hash_seed,
    uint32_t expected_term_count,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_object_visitor visitor,
    void *visitor_context
)
{
    if (visitor == NULL)
    {
        return II42_ERR_INVALID;
    }
    return ii42_lexicon_cow_validate_external_common(
        root,
        hash_seed,
        expected_term_count,
        loader,
        loader_context,
        visitor,
        visitor_context
    );
}

typedef struct ii42_lexicon_cow_external_scan_state
{
    uint64_t hash_seed;
    uint32_t expected_term_count;
    ii42_lexicon_cow_object_loader loader;
    void *loader_context;
    ii42_lexicon_cow_entry_visitor visitor;
    void *visitor_context;
} ii42_lexicon_cow_external_scan_state;

static ii42_status
ii42_lexicon_cow_scan_external_subtree(
    ii42_lexicon_cow_external_scan_state *state,
    const ii42_lexicon_cow_ref *ref,
    uint16_t expected_level,
    uint64_t expected_prefix,
    uint32_t *term_count_out
)
{
    ii42_lexicon_cow_object loaded = {0};
    const ii42_lexicon_cow_object *object;
    bool owned = false;
    ii42_status status;

    if (state == NULL || ref == NULL || term_count_out == NULL ||
        expected_level > II42_LEXICON_COW_RADIX_LEVELS ||
        ii42_lexicon_cow_ref_is_unbound(ref))
    {
        return II42_ERR_FORMAT;
    }
    status = ii42_lexicon_cow_load_patch_object(
        NULL,
        ref,
        state->loader,
        state->loader_context,
        &loaded,
        &object,
        &owned
    );
    if (status != II42_OK)
    {
        return status;
    }
    if (object->ref.kind == II42_LEXICON_COW_OBJECT_BUCKET)
    {
        const ii42_lexicon_cow_bucket *bucket = &object->value.bucket;

        if (bucket->depth != expected_level ||
            bucket->prefix != expected_prefix)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < bucket->entry_count; index++)
        {
            const ii42_lexicon_cow_entry *entry = &bucket->entries[index];

            if (entry->term_id >= state->expected_term_count ||
                entry->hash != ii42_lexicon_cow_hash(
                    entry->bytes,
                    entry->bytes_len,
                    state->hash_seed
                ))
            {
                ii42_lexicon_cow_object_free(&loaded);
                return II42_ERR_FORMAT;
            }
            status = state->visitor(state->visitor_context, entry);
            if (status != II42_OK)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return status;
            }
        }
        *term_count_out = bucket->entry_count;
        ii42_lexicon_cow_object_free(&loaded);
        return II42_OK;
    }
    else
    {
        const ii42_lexicon_cow_node *node = &object->value.node;
        uint32_t total = 0;

        if (node->level != expected_level ||
            node->prefix != expected_prefix ||
            expected_level >= II42_LEXICON_COW_RADIX_LEVELS)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        for (uint32_t index = 0; index < node->child_count; index++)
        {
            const ii42_lexicon_cow_child *child = &node->children[index];
            uint64_t child_prefix = expected_prefix |
                ((uint64_t) child->slot <<
                 ((uint32_t) expected_level *
                  II42_LEXICON_COW_RADIX_BITS));
            uint32_t child_terms = 0;

            status = ii42_lexicon_cow_scan_external_subtree(
                state,
                &child->ref,
                expected_level + 1,
                child_prefix,
                &child_terms
            );
            if (status != II42_OK || child_terms != child->term_count ||
                UINT32_MAX - total < child_terms)
            {
                ii42_lexicon_cow_object_free(&loaded);
                return status == II42_OK ? II42_ERR_FORMAT : status;
            }
            total += child_terms;
        }
        if (total != node->term_count)
        {
            ii42_lexicon_cow_object_free(&loaded);
            return II42_ERR_FORMAT;
        }
        *term_count_out = total;
        ii42_lexicon_cow_object_free(&loaded);
        return II42_OK;
    }
}

ii42_status
ii42_lexicon_cow_scan_external(
    const ii42_lexicon_cow_ref *root,
    uint64_t hash_seed,
    uint32_t expected_term_count,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_entry_visitor visitor,
    void *visitor_context
)
{
    ii42_lexicon_cow_external_scan_state state = {0};
    uint32_t term_count = 0;
    ii42_status status;

    if (root == NULL || loader == NULL || visitor == NULL)
    {
        return II42_ERR_INVALID;
    }
    if (expected_term_count == 0)
    {
        static const ii42_lexicon_cow_ref zero_ref = {0};

        return memcmp(root, &zero_ref, sizeof(*root)) == 0
            ? II42_OK
            : II42_ERR_FORMAT;
    }
    if (!ii42_lexicon_cow_ref_valid(root) ||
        ii42_lexicon_cow_ref_is_unbound(root))
    {
        return II42_ERR_INVALID;
    }
    state.hash_seed = hash_seed;
    state.expected_term_count = expected_term_count;
    state.loader = loader;
    state.loader_context = loader_context;
    state.visitor = visitor;
    state.visitor_context = visitor_context;
    status = ii42_lexicon_cow_scan_external_subtree(
        &state,
        root,
        0,
        0,
        &term_count
    );
    if (status != II42_OK)
    {
        return status;
    }
    return term_count == expected_term_count
        ? II42_OK
        : II42_ERR_FORMAT;
}

ii42_status
ii42_lexicon_cow_build_external_append_patch(
    const ii42_lexicon_cow_ref *old_root,
    uint64_t hash_seed,
    uint32_t old_term_count,
    const ii42_lexicon_cow_key *keys,
    uint32_t key_count,
    uint64_t owner_manifest_id,
    ii42_lexicon_cow_object_loader loader,
    void *loader_context,
    ii42_lexicon_cow_tree *patch_out,
    ii42_lexicon_cow_update_stats *stats_out
)
{
    ii42_lexicon_cow_tree patch;
    ii42_lexicon_cow_entry *entries = NULL;
    ii42_lexicon_cow_entry **entry_refs = NULL;
    ii42_lexicon_cow_update_stats stats = {0};
    ii42_lexicon_cow_ref next_root = {0};
    ii42_status status;

    if (old_root == NULL || old_term_count == 0 || loader == NULL ||
        patch_out == NULL || owner_manifest_id == 0 ||
        owner_manifest_id == old_root->owner_manifest_id ||
        !ii42_lexicon_cow_ref_valid(old_root) ||
        ii42_lexicon_cow_ref_is_unbound(old_root))
    {
        return II42_ERR_INVALID;
    }
    if (stats_out != NULL)
    {
        memset(stats_out, 0, sizeof(*stats_out));
    }
    ii42_lexicon_cow_tree_init(&patch);
    patch.hash_seed = hash_seed;
    patch.term_count = old_term_count;
    patch.root = *old_root;
    if (key_count == 0)
    {
        ii42_lexicon_cow_tree_free(patch_out);
        *patch_out = patch;
        return II42_OK;
    }
    status = ii42_lexicon_cow_prepare_entries(
        keys,
        key_count,
        old_term_count,
        hash_seed,
        &entries,
        &entry_refs
    );
    if (status != II42_OK)
    {
        return status;
    }
    for (uint32_t index = 0; index < key_count; index++)
    {
        uint32_t existing_id;
        bool found;

        status = ii42_lexicon_cow_lookup_external(
            old_root,
            hash_seed,
            entries[index].bytes,
            entries[index].bytes_len,
            loader,
            loader_context,
            &existing_id,
            &found
        );
        if (status != II42_OK || found)
        {
            status = status == II42_OK ? II42_ERR_FORMAT : status;
            goto fail;
        }
    }
    stats.changed_terms = key_count;
    status = ii42_lexicon_cow_patch_subtree(
        &patch,
        old_root,
        entry_refs,
        key_count,
        0,
        0,
        owner_manifest_id,
        loader,
        loader_context,
        &stats,
        &next_root
    );
    if (status != II42_OK)
    {
        goto fail;
    }
    patch.root = next_root;
    patch.term_count = old_term_count + key_count;
    free(entry_refs);
    free(entries);
    ii42_lexicon_cow_tree_free(patch_out);
    *patch_out = patch;
    if (stats_out != NULL)
    {
        *stats_out = stats;
    }
    return II42_OK;

fail:
    free(entry_refs);
    free(entries);
    ii42_lexicon_cow_tree_free(&patch);
    return status;
}
