#ifndef II42_AM_BUILD_H
#define II42_AM_BUILD_H

#include "postgres.h"

#include "common/relpath.h"
#include "storage/buffile.h"
#include "storage/itemptr.h"
#include "utils/rel.h"

#include "ii42_core.h"
#include "ii42_semantic.h"
#include "ii42_segments.h"

typedef enum ii42_am_rebuild_builder
{
    II42_AM_REBUILD_BUILDER_STANDARD = 0,
    II42_AM_REBUILD_BUILDER_COMPACT,
    II42_AM_REBUILD_BUILDER_SPILL,
    II42_AM_REBUILD_BUILDER_SEMANTIC_STREAM
} ii42_am_rebuild_builder;

typedef struct ii42_am_rebuild_workload
{
    uint64 live_payload_bytes;
    uint64 identity_source_bytes;
    uint64 mutation_bytes;
} ii42_am_rebuild_workload;

typedef struct ii42_am_rebuild_output
{
    Oid source_type;
    ItemPointerData *doc_tids;
    size_t num_docs;
    uint8_t *index_bytes;
    size_t index_bytes_len;
    char semantic_signature[II42_AM_SEMANTIC_SIGNATURE_LEN + 1];
    ii42_index index;
    bool index_valid;
    bool segment_sae;
    BufFile *segment_semantic_postings;
    size_t segment_semantic_posting_count;
    uint8 *segment_semantic_input_fingerprints;
    size_t segment_semantic_input_fingerprint_count;
    double heap_tuples;
    double index_tuples;
} ii42_am_rebuild_output;

void ii42_am_rebuild_output_release(
    ii42_am_rebuild_output *output
);

uint64 ii42_am_prepare_replacement_relation(
    Relation index_relation
);

void ii42_am_publish_replacement_segments(
    Relation index_relation,
    ForkNumber fork_number,
    ii42_am_rebuild_output *replacement,
    const uint8 contract_hash[II42_SEGMENT_CONTRACT_HASH_BYTES],
    uint16 cache_epoch,
    uint16 flags,
    uint64 rebuild_count
);

bool ii42_am_rebuild_memory_budget_choose(
    bool semantic_streaming,
    const ii42_am_rebuild_workload *workload,
    int rebuild_memory_budget_mb,
    int work_mem_kb,
    ii42_am_rebuild_builder *builder_out,
    uint64 *standard_estimated_bytes_out,
    uint64 *compact_estimated_bytes_out,
    uint64 *spill_estimated_bytes_out,
    uint64 *budget_bytes_out
);

const char *ii42_am_rebuild_builder_name(
    ii42_am_rebuild_builder builder
);

#endif
