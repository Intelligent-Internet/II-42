#ifndef II42_AM_HOT_FOLD_H
#define II42_AM_HOT_FOLD_H

#include "postgres.h"

#include "utils/rel.h"

#include "ii42_am_preload.h"

#define II42_AM_HOT_FOLD_MAGIC UINT32_C(0x46323449)
#define II42_AM_HOT_FOLD_VERSION UINT32_C(1)

typedef struct ii42_segment_storage_snapshot
    ii42_segment_storage_snapshot;
typedef struct ii42_term_fold_bundle ii42_term_fold_bundle;

/* Worker-published, exact-root read image for converged hot impact folds. */
typedef struct ii42_am_hot_fold_header
{
    uint32 magic;
    uint32 version;
    Size total_size;
    uint64 checksum;
    uint32 term_count;
    uint32 reserved;
    uint64 entry_count;
    Size terms_offset;
    Size entries_offset;
} ii42_am_hot_fold_header;

typedef struct ii42_am_hot_fold_term
{
    uint32 term_id;
    uint32 reserved;
    uint64 first_entry;
    uint64 entry_count;
} ii42_am_hot_fold_term;

typedef struct ii42_am_hot_fold_entry
{
    uint64 tie_break_key;
    uint32 document_slot;
    float score;
    ItemPointerData tid;
    uint16 reserved;
} ii42_am_hot_fold_entry;

/* Value handle only; the HOT_FOLD module owns payload interpretation. */
typedef struct ii42_am_hot_fold_view
{
    const void *private_header;
    const void *private_terms;
    const void *private_entries;
    ii42_am_preload_lease private_lease;
} ii42_am_hot_fold_view;

void ii42_am_hot_fold_view_init(ii42_am_hot_fold_view *view);
void ii42_am_hot_fold_view_release(ii42_am_hot_fold_view *view);

bool ii42_am_hot_fold_attach(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_hot_fold_view *view
);

const ii42_am_hot_fold_term *ii42_am_hot_fold_find_term(
    const ii42_am_hot_fold_view *view,
    uint32 term_id
);

const ii42_am_hot_fold_entry *ii42_am_hot_fold_term_entries(
    const ii42_am_hot_fold_view *view,
    const ii42_am_hot_fold_term *term
);

bool ii42_am_hot_fold_publish(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_hot_fold_view *prior_view,
    uint32 term_id,
    const ii42_term_fold_bundle *impact_fold,
    const ii42_segment_storage_snapshot *snapshot
);

#endif
