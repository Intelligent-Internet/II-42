#ifndef II42_AM_MUTATION_H
#define II42_AM_MUTATION_H

#include "postgres.h"

#include "access/transam.h"
#include "access/xlogdefs.h"
#include "storage/bufpage.h"
#include "utils/rel.h"

#include "ii42_am_meta.h"
#include "ii42_segment_pages.h"

#define II42_AM_ABORTED_DELTA_XID BootstrapTransactionId

typedef enum ii42_am_delta_xid_state
{
    II42_AM_DELTA_XID_COMMITTED = 0,
    II42_AM_DELTA_XID_ABORTED,
    II42_AM_DELTA_XID_UNRESOLVED
} ii42_am_delta_xid_state;

uint32 ii42_am_active_l0_rotation_record_limit(void);
bool ii42_am_active_l0_checkpoint_due(
    const ii42_segment_read_root *root
);
uint32 ii42_am_accelerator_refresh_record_limit(void);
bool ii42_am_delta_record_states(
    const TransactionId *record_xids,
    ii42_am_delta_xid_state *states_out,
    uint32 record_count
);
bool ii42_am_l0_expected_source_matches(
    uint8 record_kind,
    const ii42_document_cow_record *expected,
    const ii42_document_cow_record *current
);

void ii42_am_l0_require_meta_root(
    const ii42_am_meta_page *meta,
    const ii42_am_meta_page *expected
);
void ii42_am_l0_store_root(
    Page meta_page,
    const ii42_segment_read_root *root
);
bool ii42_am_mutation_publish_cow_manifest(
    Relation index_relation,
    const ii42_segment_read_root *build_root,
    const ii42_segment_manifest *next_manifest,
    const ii42_segment_cow_result *cow_result,
    bool seal_pending,
    bool require_exact_frontiers,
    bool publish_fsm_handoff
);
XLogRecPtr ii42_am_l0_rotate_active_locked(
    Relation index_relation,
    ii42_am_meta_page *expected_meta,
    ii42_segment_read_root *root
);
bool ii42_am_mutation_append_l0_record(
    Relation index_relation,
    ii42_l0_record *record,
    const ii42_document_cow_record *expected_source,
    uint64 expected_l0_born_sequence,
    bool *maintenance_due_out
);

#endif
