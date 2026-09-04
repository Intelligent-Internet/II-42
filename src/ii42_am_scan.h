#ifndef II42_AM_SCAN_H
#define II42_AM_SCAN_H

#include "postgres.h"

#include "access/tableam.h"
#include "executor/tuptable.h"
#include "storage/itemptr.h"
#include "utils/rel.h"
#include "utils/snapshot.h"

typedef struct ii42_am_visibility_ctx
{
    IndexFetchTableData *fetch;
    Snapshot snapshot;
    TupleTableSlot *slot;
} ii42_am_visibility_ctx;

void ii42_am_visibility_begin(
    Relation heap_relation,
    ii42_am_visibility_ctx *visibility_out
);
void ii42_am_visibility_begin_with_snapshot(
    Relation heap_relation,
    Snapshot snapshot,
    ii42_am_visibility_ctx *visibility_out
);
bool ii42_am_tid_visible(
    ii42_am_visibility_ctx *visibility,
    const ItemPointerData *tid
);
bool ii42_am_tid_visible_as(
    ii42_am_visibility_ctx *visibility,
    const ItemPointerData *tid,
    ItemPointerData *visible_tid_out
);
void ii42_am_visibility_end(ii42_am_visibility_ctx *visibility);

#endif
