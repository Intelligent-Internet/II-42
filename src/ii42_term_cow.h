#ifndef II42_TERM_COW_H
#define II42_TERM_COW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ii42_block_ranges.h"
#include "ii42_segments.h"

#define II42_TERM_COW_LEAF_TERMS UINT32_C(16)
#define II42_TERM_COW_RADIX_BITS UINT32_C(6)
#define II42_TERM_COW_RADIX_FANOUT UINT32_C(64)
#define II42_TERM_COW_RADIX_LEVELS UINT32_C(5)

#define II42_TERM_COW_RECORD_FLAG_NEUTRAL_FOLD UINT32_C(0x0001)
#define II42_TERM_COW_RECORD_FLAG_IMPACT_FOLD UINT32_C(0x0002)
#define II42_TERM_COW_RECORD_FLAG_NEUTRAL_MINOR_FOLD UINT32_C(0x0004)

typedef enum ii42_term_cow_object_kind
{
    II42_TERM_COW_OBJECT_INVALID = 0,
    II42_TERM_COW_OBJECT_NODE = 1,
    II42_TERM_COW_OBJECT_LEAF = 2
} ii42_term_cow_object_kind;

typedef struct ii42_term_cow_ref
{
    ii42_term_cow_object_kind kind;
    uint32_t start_block;
    uint32_t page_count;
    uint32_t reserved;
    uint64_t object_id;
    uint64_t owner_manifest_id;
    uint64_t object_bytes;
    uint64_t checksum;
    uint64_t blob_checksum;
} ii42_term_cow_ref;

typedef struct ii42_stable_term_extent
{
    uint64_t segment_id;
    ii42_posting_extent_kind kind;
    uint64_t posting_offset;
    uint64_t posting_count;
} ii42_stable_term_extent;

typedef struct ii42_term_cow_record
{
    uint32_t term_id;
    uint32_t raw_document_frequency;
    uint32_t flags;
    uint32_t extent_count;
    uint64_t neutral_fold_coverage;
    uint64_t neutral_minor_fold_coverage;
    uint64_t impact_fold_coverage;
    uint64_t impact_statistics_epoch;
    ii42_segment_object_ref neutral_fold;
    ii42_segment_object_ref neutral_minor_fold;
    ii42_segment_object_ref impact_fold;
    ii42_segment_object_ref lexical_catalog;
    ii42_stable_term_extent extents[
        II42_TERM_DIRECTORY_MAX_EXTENTS_PER_TERM
    ];
} ii42_term_cow_record;

typedef struct ii42_term_cow_fold_state
{
    uint64_t neutral_coverage;
    uint64_t neutral_minor_coverage;
    uint64_t impact_coverage;
    uint64_t impact_statistics_epoch;
    ii42_segment_object_ref neutral_ref;
    ii42_segment_object_ref neutral_minor_ref;
    ii42_segment_object_ref impact_ref;
} ii42_term_cow_fold_state;

typedef struct ii42_term_cow_child
{
    uint16_t slot;
    uint16_t reserved;
    uint32_t max_extent_count;
    ii42_term_cow_ref ref;
} ii42_term_cow_child;

typedef struct ii42_term_cow_node
{
    uint16_t level;
    uint16_t reserved;
    uint32_t child_count;
    uint64_t prefix;
    ii42_term_cow_child children[II42_TERM_COW_RADIX_FANOUT];
} ii42_term_cow_node;

typedef struct ii42_term_cow_leaf
{
    uint32_t base_term_id;
    uint32_t record_count;
    ii42_term_cow_record records[II42_TERM_COW_LEAF_TERMS];
} ii42_term_cow_leaf;

typedef struct ii42_term_cow_object
{
    ii42_term_cow_ref ref;
    union
    {
        ii42_term_cow_node node;
        ii42_term_cow_leaf leaf;
    } value;
} ii42_term_cow_object;

typedef struct ii42_term_cow_tree
{
    uint32_t vocab_size;
    uint64_t next_object_id;
    ii42_term_cow_ref root;
    ii42_term_cow_object *objects;
    size_t object_count;
    size_t object_capacity;
    ii42_block_range *retired_ranges;
    size_t retired_range_count;
    size_t retired_range_capacity;
} ii42_term_cow_tree;

typedef struct ii42_term_cow_update_stats
{
    uint32_t changed_terms;
    uint32_t changed_leaves;
    uint32_t written_nodes;
    uint32_t written_leaves;
    uint64_t written_bytes;
} ii42_term_cow_update_stats;

typedef enum ii42_term_cow_neutral_fold_level
{
    II42_TERM_COW_NEUTRAL_FOLD_MAJOR = 0,
    II42_TERM_COW_NEUTRAL_FOLD_MINOR = 1
} ii42_term_cow_neutral_fold_level;

typedef ii42_status (*ii42_term_cow_object_loader)(
    void *context,
    const ii42_term_cow_ref *ref,
    ii42_term_cow_object *object_out
);

typedef ii42_status (*ii42_term_cow_object_visitor)(
    void *context,
    const ii42_term_cow_object *object
);

void ii42_term_cow_tree_init(ii42_term_cow_tree *tree);
void ii42_term_cow_tree_free(ii42_term_cow_tree *tree);

ii42_status ii42_term_cow_tree_build(
    const ii42_term_directory *directory,
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *lexical_catalog,
    ii42_term_cow_tree *tree_out
);

/*
 * Build the initial term authority directly from complete neutral folds.
 * fold_refs contains one entry per vocabulary term; terms without postings
 * use an all-zero ref. No immutable payload extent remains in the tree.
 */
ii42_status ii42_term_cow_tree_build_initial_folds(
    const ii42_segment_manifest *manifest,
    const ii42_segment_object_ref *lexical_catalog,
    const ii42_segment_object_ref *fold_refs,
    ii42_term_cow_tree *tree_out
);

ii42_status ii42_term_cow_tree_validate(
    const ii42_term_cow_tree *tree
);

ii42_status ii42_term_cow_tree_lookup(
    const ii42_term_cow_tree *tree,
    uint32_t term_id,
    ii42_term_cow_record *record_out
);

/*
 * Resolve one term through an external immutable object store. This is the
 * bounded cold/restart path: it reads one radix path, not the complete
 * vocabulary or a flat derived directory.
 */
ii42_status ii42_term_cow_lookup_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t term_id,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_record *record_out
);

/*
 * Descend through persistent subtree maxima to find one term whose immutable
 * extent count reaches the requested pressure threshold. The cold path loads
 * at most one radix path and one leaf, independent of vocabulary size.
 */
ii42_status ii42_term_cow_find_extent_pressure_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t threshold,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_record *record_out,
    bool *found_out
);

/*
 * Check only terms present in one new immutable payload. Each distinct term
 * performs one bounded COW lookup; untouched vocabulary is never scanned.
 */
ii42_status ii42_term_cow_append_has_capacity_external(
    const ii42_term_cow_ref *root,
    uint32_t old_vocab_size,
    const ii42_segment_payload_view *new_payload,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    bool *has_capacity_out
);

/*
 * Validate one complete externally stored tree. Each reachable immutable
 * object is loaded exactly once and checked by owner-local identity, radix
 * shape, physical reference, and checksum.
 */
ii42_status ii42_term_cow_validate_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    ii42_term_cow_object_loader loader,
    void *loader_context
);

/*
 * Validate the complete external tree and visit every reachable immutable
 * object exactly once. The callback runs only after the object's checked
 * radix subtree has been accepted.
 */
ii42_status ii42_term_cow_visit_external(
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_object_visitor visitor,
    void *visitor_context
);

/*
 * Reconstruct a disposable flat query accelerator from the authoritative COW
 * tree. This is an attach/cache operation, never a publication authority.
 */
ii42_status ii42_term_cow_materialize_external(
    const ii42_term_cow_ref *root,
    const ii42_segment_manifest *manifest,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_directory *directory_out,
    uint32_t **doc_frequencies_out,
    ii42_segment_object_ref **lexical_catalog_refs_out,
    ii42_term_cow_fold_state **fold_states_out
);

/*
 * Build one restart-safe append patch from an externally stored ancestor
 * root. The returned tree contains only new leaves and shared radix paths;
 * unchanged children remain bound references to ancestor-owned objects.
 */
ii42_status ii42_term_cow_build_external_append_patch(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_payload_view *new_payload,
    const ii42_segment_object_ref *lexical_catalog,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    ii42_term_cow_tree *patch_out,
    ii42_term_cow_update_stats *stats_out
);

/*
 * Replace one contiguous descriptor range by copying only leaves containing
 * input or replacement runs and their shared radix paths. Fold-covered runs
 * remain hidden, post-coverage runs are replaced normally, and a term whose
 * selected runs cross its fold watermark fails closed. The input payloads
 * prove the complete touched-term set, so restart-time compaction never scans
 * the vocabulary or materializes the flat directory.
 */
ii42_status ii42_term_cow_build_external_replace_patch(
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
);

/*
 * Check whether one physical replacement range is compatible with every
 * touched term-local neutral-fold watermark. The touched-term set comes only
 * from replaced_payloads and is resolved through bounded COW leaf paths; the
 * vocabulary is never materialized or scanned. A valid range that would mix
 * covered and post-coverage runs returns II42_OK with safe_out=false.
 */
ii42_status ii42_term_cow_replace_fold_preflight_external(
    const ii42_term_cow_ref *old_root,
    const ii42_segment_manifest *manifest,
    uint32_t first_segment_index,
    const ii42_segment_payload_view *replaced_payloads,
    uint32_t replaced_payload_count,
    ii42_term_cow_object_loader loader,
    void *loader_context,
    bool *safe_out,
    uint32_t *conflict_term_id_out
);

/*
 * Install one persistent neutral folded prefix by copying exactly one leaf
 * and its radix path. coverage_sequence must be a complete immutable segment
 * boundary for every consumed extent of the term. Covered source extents are
 * removed from the term record; their source segments remain manifest-visible
 * for unrelated terms, while this term continues as fold plus post-coverage
 * tail.
 */
ii42_status ii42_term_cow_build_external_neutral_fold_patch(
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
);

/*
 * Attach one epoch-bound impact specialization to the complete effective
 * neutral prefix without changing authoritative extents or fold coverage.
 * The caller must separately prove that no lexical L0 mutation is attached to
 * the publication root.
 */
ii42_status ii42_term_cow_build_external_impact_fold_patch(
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
);

ii42_status ii42_term_cow_tree_lookup_at(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *root,
    uint32_t vocab_size,
    uint32_t term_id,
    ii42_term_cow_record *record_out
);

/*
 * Append one immutable payload by writing only touched leaves and their radix
 * paths. The object store is append-only, so a saved prior root remains valid.
 */
ii42_status ii42_term_cow_tree_append_payload(
    ii42_term_cow_tree *tree,
    const ii42_segment_manifest *old_manifest,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_payload_view *new_payload,
    ii42_term_cow_update_stats *stats_out
);

/*
 * Reconstruct the transitional flat directory and DF array for differential
 * tests or migration. This is deliberately not a normal publication path.
 */
ii42_status ii42_term_cow_tree_materialize_flat(
    const ii42_term_cow_tree *tree,
    const ii42_segment_manifest *manifest,
    ii42_term_directory *directory_out,
    uint32_t **doc_frequencies_out
);

ii42_status ii42_term_cow_tree_object_serialize(
    const ii42_term_cow_tree *tree,
    const ii42_term_cow_ref *ref,
    uint8_t **bytes_out,
    size_t *size_out
);

/*
 * Prepare one not-yet-published object for bottom-up physical storage.
 * Every node child must already have a bound page-chain locator. The call
 * refreshes copied child refs and the object's checksums before returning the
 * canonical bytes that must be written.
 */
ii42_status ii42_term_cow_tree_prepare_object_for_storage(
    ii42_term_cow_tree *tree,
    uint64_t object_id,
    uint8_t **bytes_out,
    size_t *size_out
);

/*
 * Bind the exact immutable page chain returned by the storage layer. Binding
 * does not change the object's canonical bytes; a later parent preparation
 * copies this complete locator into its child ref.
 */
ii42_status ii42_term_cow_tree_bind_object_storage(
    ii42_term_cow_tree *tree,
    uint64_t object_id,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_term_cow_object_bind_storage(
    ii42_term_cow_object *object,
    const ii42_segment_object_ref *storage_ref
);

ii42_status ii42_term_cow_ref_as_segment_object_ref(
    const ii42_term_cow_ref *ref,
    ii42_segment_object_ref *storage_ref_out
);

ii42_status ii42_term_cow_object_deserialize(
    const uint8_t *bytes,
    size_t size,
    ii42_term_cow_object *object_out
);

#endif
