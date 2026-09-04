#ifndef II42_AM_RESIDENT_FOLD_H
#define II42_AM_RESIDENT_FOLD_H

#include "postgres.h"

#include "utils/rel.h"

#include "ii42_am_meta.h"
#include "ii42_am_preload.h"
#include "ii42_segment_pages.h"

typedef enum ii42_am_resident_fold_publish_result
{
    II42_AM_RESIDENT_FOLD_PUBLISH_FAILED = 0,
    II42_AM_RESIDENT_FOLD_PUBLISH_READY,
    II42_AM_RESIDENT_FOLD_PUBLISH_LOADING,
    II42_AM_RESIDENT_FOLD_PUBLISH_OVERSIZED,
    II42_AM_RESIDENT_FOLD_PUBLISH_DONE
} ii42_am_resident_fold_publish_result;

typedef struct ii42_am_resident_fold_view
{
    ii42_am_preload_lease private_lease;
    const void *private_header;
    const void *private_terms;
    const void *private_extents;
    const void *private_vocab_offsets;
    const uint32 *private_sorted_vocab_ids;
} ii42_am_resident_fold_view;

void ii42_am_resident_fold_view_init(
    ii42_am_resident_fold_view *view
);

void ii42_am_resident_fold_view_release(
    ii42_am_resident_fold_view *view
);

bool ii42_am_resident_fold_attach(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_resident_fold_view *view
);

bool ii42_am_resident_fold_lookup_token(
    const ii42_am_resident_fold_view *view,
    const char *token,
    uint32 *term_id_out
);

ii42_status ii42_am_resident_fold_topk(
    const ii42_am_resident_fold_view *view,
    const uint32 *query_ids,
    const float *query_weights,
    size_t query_len,
    size_t k,
    ii42_topk_result *result_out,
    ii42_blockmax_stats *stats_out
);

ii42_status ii42_am_resident_fold_topk_filtered(
    const ii42_am_resident_fold_view *view,
    const uint32 *query_ids,
    const float *query_weights,
    size_t query_len,
    const uint8 *allowed_document_bitmap,
    size_t allowed_document_count,
    size_t k,
    ii42_topk_result *result_out,
    ii42_blockmax_stats *stats_out
);

const ItemPointerData *ii42_am_resident_fold_document_tids(
    const ii42_am_resident_fold_view *view
);

uint32 ii42_am_resident_fold_document_count(
    const ii42_am_resident_fold_view *view
);

uint64 ii42_am_resident_fold_root_id(
    const ii42_am_resident_fold_view *view
);

Size ii42_am_resident_fold_required_size(
    const ii42_segment_storage_snapshot *snapshot
);

bool ii42_am_resident_fold_materialization_fits(
    uint64 relation_bytes,
    uint64 arena_bytes,
    uint64 physical_bytes,
    uint64 shared_buffer_bytes
);

ii42_am_resident_fold_publish_result ii42_am_resident_fold_publish(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    const ii42_segment_storage_snapshot *snapshot,
    Size *published_size_out
);

#endif
