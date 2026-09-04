#ifndef II42_AM_RECLAMATION_H
#define II42_AM_RECLAMATION_H

#include "postgres.h"

#include "utils/rel.h"

#include "ii42_segment_pages.h"
#include "ii42_segments.h"

bool ii42_am_acquire_convergent_reader_fence(
    Relation index_relation,
    const ii42_segment_read_root *build_root,
    const ii42_segment_manifest *old_manifest,
    ii42_segment_page_reuse_arena *arena
);

#endif
