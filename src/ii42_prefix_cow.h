#ifndef II42_PREFIX_COW_H
#define II42_PREFIX_COW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ii42_block_ranges.h"
#include "ii42_segments.h"

#define II42_PREFIX_COW_LEAF_MAX_ENTRIES UINT32_C(128)
#define II42_PREFIX_COW_NODE_MAX_CHILDREN UINT32_C(48)
#define II42_PREFIX_COW_MAX_DEPTH UINT32_C(32)

typedef enum ii42_prefix_cow_object_kind
{
    II42_PREFIX_COW_OBJECT_INVALID = 0,
    II42_PREFIX_COW_OBJECT_NODE = 1,
    II42_PREFIX_COW_OBJECT_LEAF = 2
} ii42_prefix_cow_object_kind;

typedef struct ii42_prefix_cow_ref
{
    ii42_prefix_cow_object_kind kind;
    uint32_t start_block;
    uint32_t page_count;
    uint32_t reserved;
    uint64_t object_id;
    uint64_t owner_manifest_id;
    uint64_t object_bytes;
    uint64_t checksum;
    uint64_t blob_checksum;
    uint32_t term_count;
    uint32_t reserved2;
} ii42_prefix_cow_ref;

typedef struct ii42_prefix_cow_key
{
    const uint8_t *bytes;
    uint32_t bytes_len;
    uint32_t term_id;
} ii42_prefix_cow_key;

typedef struct ii42_prefix_cow_entry
{
    uint8_t *bytes;
    uint32_t bytes_len;
    uint32_t term_id;
} ii42_prefix_cow_entry;

typedef struct ii42_prefix_cow_child
{
    ii42_prefix_cow_ref ref;
    uint8_t *max_key;
    uint32_t max_key_len;
} ii42_prefix_cow_child;

typedef struct ii42_prefix_cow_node
{
    uint32_t child_count;
    ii42_prefix_cow_child *children;
} ii42_prefix_cow_node;

typedef struct ii42_prefix_cow_leaf
{
    uint32_t entry_count;
    ii42_prefix_cow_entry *entries;
} ii42_prefix_cow_leaf;

typedef struct ii42_prefix_cow_object
{
    ii42_prefix_cow_ref ref;
    union
    {
        ii42_prefix_cow_node node;
        ii42_prefix_cow_leaf leaf;
    } value;
} ii42_prefix_cow_object;

typedef struct ii42_prefix_cow_tree
{
    uint32_t term_count;
    uint32_t reserved;
    uint64_t next_object_id;
    ii42_prefix_cow_ref root;
    ii42_prefix_cow_object *objects;
    size_t object_count;
    size_t object_capacity;
    ii42_block_range *retired_ranges;
    size_t retired_range_count;
    size_t retired_range_capacity;
} ii42_prefix_cow_tree;

typedef struct ii42_prefix_cow_update_stats
{
    uint32_t changed_terms;
    uint32_t read_nodes;
    uint32_t read_leaves;
    uint32_t written_nodes;
    uint32_t written_leaves;
    uint64_t written_bytes;
} ii42_prefix_cow_update_stats;

typedef ii42_status (*ii42_prefix_cow_object_loader)(
    void *context,
    const ii42_prefix_cow_ref *ref,
    ii42_prefix_cow_object *object_out
);

typedef ii42_status (*ii42_prefix_cow_object_visitor)(
    void *context,
    const ii42_prefix_cow_object *object
);

typedef ii42_status (*ii42_prefix_cow_entry_visitor)(
    void *context,
    const ii42_prefix_cow_entry *entry
);

void ii42_prefix_cow_tree_init(ii42_prefix_cow_tree *tree);
void ii42_prefix_cow_tree_free(ii42_prefix_cow_tree *tree);
void ii42_prefix_cow_object_free(ii42_prefix_cow_object *object);

ii42_status ii42_prefix_cow_tree_build(
    const ii42_prefix_cow_key *keys,
    uint32_t key_count,
    uint64_t owner_manifest_id,
    ii42_prefix_cow_tree *tree_out
);

ii42_status ii42_prefix_cow_build_external_append_patch(
    const ii42_prefix_cow_ref *old_root,
    uint32_t old_term_count,
    const ii42_prefix_cow_key *keys,
    uint32_t key_count,
    uint64_t owner_manifest_id,
    ii42_prefix_cow_object_loader loader,
    void *loader_context,
    ii42_prefix_cow_tree *patch_out,
    ii42_prefix_cow_update_stats *stats_out
);

ii42_status ii42_prefix_cow_scan_prefix_external(
    const ii42_prefix_cow_ref *root,
    uint32_t expected_term_count,
    const uint8_t *prefix,
    size_t prefix_len,
    ii42_prefix_cow_object_loader loader,
    void *loader_context,
    ii42_prefix_cow_entry_visitor visitor,
    void *visitor_context
);

ii42_status ii42_prefix_cow_validate_external(
    const ii42_prefix_cow_ref *root,
    uint32_t expected_term_count,
    ii42_prefix_cow_object_loader loader,
    void *loader_context
);

ii42_status ii42_prefix_cow_visit_external(
    const ii42_prefix_cow_ref *root,
    uint32_t expected_term_count,
    ii42_prefix_cow_object_loader loader,
    void *loader_context,
    ii42_prefix_cow_object_visitor visitor,
    void *visitor_context
);

ii42_status ii42_prefix_cow_object_serialize(
    const ii42_prefix_cow_object *object,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_prefix_cow_object_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_prefix_cow_object *object_out
);

ii42_status ii42_prefix_cow_tree_prepare_object_for_storage(
    ii42_prefix_cow_tree *tree,
    uint64_t object_id,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_prefix_cow_object_bind_storage(
    ii42_prefix_cow_object *object,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_prefix_cow_tree_bind_object_storage(
    ii42_prefix_cow_tree *tree,
    uint64_t object_id,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_prefix_cow_ref_as_segment_object_ref(
    const ii42_prefix_cow_ref *ref,
    ii42_segment_object_ref *storage_ref_out
);

#endif
