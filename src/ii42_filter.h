#ifndef II42_FILTER_H
#define II42_FILTER_H

#include "postgres.h"

#include "utils/jsonb.h"
#include "utils/relcache.h"

#include "ii42_segments.h"

typedef struct ii42_filter_scope_trace
{
    uint32 predicate_count;
    uint32 resolved_predicate_count;
    uint64 ilike_comparisons;
    uint64 ilike_values_examined;
    uint64 ilike_blocks_considered;
    uint64 ilike_blocks_skipped;
    uint32 ilike_values;
    bool ilike_comparison_limit_exceeded;
} ii42_filter_scope_trace;

uint64 ii42_filter_tid_key(const ItemPointerData *tid);

/* Sort canonical TID keys in place and return the duplicate-free count. */
size_t ii42_filter_sort_unique_tid_keys(uint64 *keys, size_t key_count);

/* True only when every structured predicate is a numeric range. */
bool ii42_filter_is_range_only(Jsonb *filters);

/*
 * Resolve every supported predicate from same-root INCLUDE scope postings.
 * A partial result is retained so an exact SQL residual can be intersected
 * with it instead of discarding already resolved scope membership.
 */
bool ii42_filter_try_scope_bitmap(
    Relation index_relation,
    Relation heap_relation,
    const ii42_segment_read_root *root,
    Jsonb *filters,
    uint8 **bitmap_out,
    uint64 *document_slot_count_out,
    uint64 *allowed_document_count_out,
    bool *fully_resolved_out,
    ii42_filter_scope_trace *trace_out
);

/* Resolve a structured predicate under the caller's active MVCC snapshot. */
bool ii42_filter_collect_tid_keys(
    Relation heap_relation,
    Jsonb *filters,
    uint64 result_upper_bound,
    uint64 row_limit,
    uint64 **keys_out,
    size_t *key_count_out
);

/* Evaluate a structured predicate only for an already ranked TID prefix. */
void ii42_filter_match_tid_candidates(
    Relation heap_relation,
    Jsonb *filters,
    const ItemPointerData *candidate_tids,
    size_t candidate_count,
    bool *matches_out,
    size_t *matched_count_out
);

#endif
