#ifndef II42_SCOPE_PG_H
#define II42_SCOPE_PG_H

#include "postgres.h"

#include "utils/rel.h"

#include "ii42_core.h"

ii42_status ii42_scope_build_for_index(
    Relation index_relation,
    const ItemPointerData *document_tids,
    uint32 document_count,
    uint64 source_authority_checksum,
    size_t maximum_size,
    size_t *required_size_out,
    uint8 **bytes_out,
    size_t *size_out
);

#endif
