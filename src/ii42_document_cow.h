#ifndef II42_DOCUMENT_COW_H
#define II42_DOCUMENT_COW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ii42_block_ranges.h"
#include "ii42_segments.h"

#define II42_DOCUMENT_COW_LEAF_RECORDS UINT32_C(16)
#define II42_DOCUMENT_COW_RADIX_BITS UINT32_C(6)
#define II42_DOCUMENT_COW_RADIX_FANOUT UINT32_C(64)
#define II42_DOCUMENT_COW_RADIX_LEVELS UINT32_C(5)

typedef enum ii42_document_cow_object_kind
{
    II42_DOCUMENT_COW_OBJECT_INVALID = 0,
    II42_DOCUMENT_COW_OBJECT_NODE = 1,
    II42_DOCUMENT_COW_OBJECT_LEAF = 2
} ii42_document_cow_object_kind;

typedef struct ii42_document_cow_record
{
    ii42_document_version_record version;
    ii42_document_retirement_record retirement;
    ii42_semantic_state_record semantic_state;
    /* Physical references reachable from the current published root. */
    uint64_t lexical_residency;
    uint64_t semantic_residency;
    uint64_t event_residency;
} ii42_document_cow_record;

typedef struct ii42_document_cow_ref
{
    ii42_document_cow_object_kind kind;
    uint32_t start_block;
    uint32_t page_count;
    uint32_t reserved;
    uint64_t object_id;
    uint64_t owner_manifest_id;
    uint64_t object_bytes;
    uint64_t checksum;
    uint64_t first_document_slot;
    uint64_t document_slot_count;
    uint64_t live_document_count;
    uint64_t semantic_pending_count;
    int64_t earliest_retry_after;
    uint64_t bounded_document_count;
    uint32_t min_document_length;
    uint32_t max_document_length;
    uint64_t reusable_document_count;
    uint64_t first_reusable_document_slot;
    /* Minimum born sequence among root-relative live incarnations. */
    uint64_t min_live_born_sequence;
} ii42_document_cow_ref;

typedef struct ii42_document_cow_child
{
    uint16_t slot;
    uint16_t reserved;
    uint32_t reserved2;
    ii42_document_cow_ref ref;
} ii42_document_cow_child;

typedef struct ii42_document_cow_node
{
    uint16_t level;
    uint16_t reserved;
    uint32_t child_count;
    uint64_t prefix;
    ii42_document_cow_child children[
        II42_DOCUMENT_COW_RADIX_FANOUT
    ];
} ii42_document_cow_node;

typedef struct ii42_document_cow_leaf
{
    uint64_t base_document_slot;
    uint32_t record_count;
    uint32_t reserved;
    ii42_document_cow_record records[
        II42_DOCUMENT_COW_LEAF_RECORDS
    ];
} ii42_document_cow_leaf;

typedef struct ii42_document_cow_object
{
    ii42_document_cow_ref ref;
    union
    {
        ii42_document_cow_node node;
        ii42_document_cow_leaf leaf;
    } value;
} ii42_document_cow_object;

typedef struct ii42_document_cow_tree
{
    uint64_t document_slot_count;
    uint64_t next_object_id;
    ii42_document_cow_ref root;
    ii42_document_cow_object *objects;
    size_t object_count;
    size_t object_capacity;
    ii42_block_range *retired_ranges;
    size_t retired_range_count;
    size_t retired_range_capacity;
} ii42_document_cow_tree;

typedef struct ii42_document_cow_update_stats
{
    uint32_t changed_records;
    uint32_t written_nodes;
    uint32_t written_leaves;
    uint64_t written_bytes;
} ii42_document_cow_update_stats;

typedef struct ii42_document_cow_born_prefix_stats
{
    uint64_t objects_loaded;
    uint64_t records_examined;
    uint64_t heap_peak;
} ii42_document_cow_born_prefix_stats;

typedef bool (*ii42_document_cow_record_predicate)(
    void *context,
    const ii42_document_cow_record *record
);

/* Compare the complete root-relative state of one slot incarnation. */
bool ii42_document_cow_records_equal(
    const ii42_document_cow_record *left,
    const ii42_document_cow_record *right
);

bool ii42_document_cow_versions_equal(
    const ii42_document_version_record *left,
    const ii42_document_version_record *right
);

bool ii42_document_cow_record_is_l0_owned(
    const ii42_document_cow_record *record
);

uint64_t ii42_document_cow_record_last_sequence(
    const ii42_document_cow_record *record
);

typedef ii42_document_block_extrema ii42_document_cow_length_extrema;

typedef ii42_status (*ii42_document_cow_object_loader)(
    void *context,
    const ii42_document_cow_ref *ref,
    ii42_document_cow_object *object_out
);

typedef ii42_status (*ii42_document_cow_object_visitor)(
    void *context,
    const ii42_document_cow_object *object
);

void ii42_document_cow_tree_init(ii42_document_cow_tree *tree);
void ii42_document_cow_tree_free(ii42_document_cow_tree *tree);

ii42_status ii42_document_cow_tree_build(
    const ii42_document_cow_record *records,
    uint64_t record_count,
    uint64_t owner_manifest_id,
    ii42_document_cow_tree *tree_out
);

ii42_status ii42_document_cow_tree_validate(
    const ii42_document_cow_tree *tree
);

ii42_status ii42_document_cow_tree_lookup(
    const ii42_document_cow_tree *tree,
    uint64_t document_slot,
    ii42_document_cow_record *record_out
);

ii42_status ii42_document_cow_lookup_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint64_t document_slot,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *record_out
);

/*
 * Read one contiguous document-slot range through a single bounded radix
 * walk. The caller owns records_out, which must hold exactly
 * range_document_slot_count records. No allocation scales with the corpus.
 */
ii42_status ii42_document_cow_read_range_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint64_t first_document_slot,
    uint64_t range_document_slot_count,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *records_out
);

/*
 * Read one conservative document-length range from the same COW tree that
 * owns document versions. Retired versions remain in the summary; aborted
 * slot holes do not. A zero document_count means the requested range contains
 * no scoreable version and must not be used as a lexical pruning bound.
 */
ii42_status ii42_document_cow_length_extrema_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint64_t first_document_slot,
    uint64_t range_document_slot_count,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_length_extrema *extrema_out
);

ii42_status ii42_document_cow_tree_length_extrema(
    const ii42_document_cow_tree *tree,
    uint64_t first_document_slot,
    uint64_t range_document_slot_count,
    ii42_document_cow_length_extrema *extrema_out
);

ii42_status ii42_document_cow_block_extrema_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint32_t block_shift,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_length_extrema **extrema_out,
    size_t *extrema_count_out
);

ii42_status ii42_document_cow_tree_block_extrema(
    const ii42_document_cow_tree *tree,
    uint32_t block_shift,
    ii42_document_cow_length_extrema **extrema_out,
    size_t *extrema_count_out
);

void ii42_document_cow_block_extrema_free(
    ii42_document_cow_length_extrema *extrema
);

/*
 * Validate every object reachable from one published root. This verifies the
 * complete radix shape, contiguous document-slot coverage, immutable object
 * identity, physical storage ranges, checksums, and subtree summaries.
 */
ii42_status ii42_document_cow_validate_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    ii42_document_cow_object_loader loader,
    void *loader_context
);

ii42_status ii42_document_cow_visit_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_object_visitor visitor,
    void *visitor_context
);

/*
 * Read the first live root-relative incarnations in exact born-sequence order.
 * The best-first cursor expands only subtrees that can contain the next
 * result. Its allocation is proportional to limit times the fixed radix
 * fanout/depth, never document_slot_count.
 */
ii42_status ii42_document_cow_collect_live_born_prefix_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    size_t limit,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *records_out,
    size_t record_capacity,
    size_t *record_count_out,
    ii42_document_cow_born_prefix_stats *stats_out
);

/*
 * Read the first matching live incarnations in exact born-sequence order.
 * Rejected records are consumed by the same best-first cursor, so output and
 * scratch allocation remain proportional to limit and the fixed radix shape.
 */
ii42_status ii42_document_cow_collect_live_born_prefix_matching_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    size_t limit,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record_predicate predicate,
    void *predicate_context,
    ii42_document_cow_record *records_out,
    size_t record_capacity,
    size_t *record_count_out,
    ii42_document_cow_born_prefix_stats *stats_out
);

ii42_status ii42_document_cow_find_actionable_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    int64_t now,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *record_out,
    bool *found_out
);

ii42_status ii42_document_cow_find_actionable_external_from(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint64_t first_document_slot,
    int64_t now,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *record_out,
    bool *found_out
);

/*
 * Find the first root-relative score slot whose prior incarnation has no
 * remaining posting or payload-event residency. A zero reusable summary
 * returns without loading any COW object.
 */
ii42_status ii42_document_cow_find_reusable_external(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *record_out,
    bool *found_out
);

ii42_status ii42_document_cow_find_reusable_external_from(
    const ii42_document_cow_ref *root,
    uint64_t document_slot_count,
    uint64_t first_document_slot,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_record *record_out,
    bool *found_out
);

ii42_status ii42_document_cow_build_external_patch(
    const ii42_document_cow_ref *old_root,
    uint64_t old_document_slot_count,
    uint64_t next_document_slot_count,
    const ii42_document_cow_record *updates,
    size_t update_count,
    uint64_t owner_manifest_id,
    ii42_document_cow_object_loader loader,
    void *loader_context,
    ii42_document_cow_tree *patch_out,
    ii42_document_cow_update_stats *stats_out
);

ii42_status ii42_document_cow_tree_prepare_object_for_storage(
    ii42_document_cow_tree *tree,
    uint64_t object_id,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_document_cow_tree_bind_object_storage(
    ii42_document_cow_tree *tree,
    uint64_t object_id,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_document_cow_object_bind_storage(
    ii42_document_cow_object *object,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_document_cow_ref_as_segment_object_ref(
    const ii42_document_cow_ref *ref,
    ii42_segment_object_ref *storage_ref_out
);

ii42_status ii42_document_cow_tree_object_serialize(
    const ii42_document_cow_tree *tree,
    const ii42_document_cow_ref *ref,
    uint8_t **bytes_out,
    size_t *size_out
);

ii42_status ii42_document_cow_object_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_document_cow_object *object_out
);

#endif
