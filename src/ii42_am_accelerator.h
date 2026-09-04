#ifndef II42_AM_ACCELERATOR_H
#define II42_AM_ACCELERATOR_H

#include "postgres.h"

#include "common/relpath.h"
#include "utils/rel.h"

#include "ii42_core.h"
#include "ii42_segment_pages.h"

#define II42_AM_ACCELERATOR_SORT_MEMORY_MAX_KB \
    (UINT64_C(1024) * 1024)
#define II42_AM_ACCELERATOR_MAX_DOCUMENT_SHIFT 12U

extern double ii42_test_semantic_accelerator_posting_mass;
extern int ii42_test_semantic_accelerator_forward_document_shift;

/*
 * Derive an accelerator from one immutable query-authority snapshot. This
 * never runs the encoder. The caller publishes the baseline with the normal
 * root compare-and-swap path while preserving any newer active-L0 frontier.
 */
bool ii42_am_prepare_accelerator_baseline(
    Relation index_relation,
    const ii42_segment_read_root *root,
    const ii42_segment_query_context *context,
    ii42_segment_page_reuse_arena *reuse_arena,
    uint64 scope_output_budget,
    volatile bool *reader_fence_locked_out,
    bool *memory_blocked_out,
    uint64 *scope_required_bytes_out,
    ii42_segment_manifest *next_manifest,
    ii42_segment_cow_result *result_out
);

#endif
