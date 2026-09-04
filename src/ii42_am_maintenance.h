#ifndef II42_AM_MAINTENANCE_H
#define II42_AM_MAINTENANCE_H

#include "postgres.h"

#include "access/transam.h"
#include "storage/lock.h"
#include "utils/rel.h"

#include "ii42_core.h"

bool ii42_am_try_maintenance_lock(Oid index_oid);
void ii42_am_lock_maintenance_xact(Oid index_oid);
bool ii42_am_try_accelerator_build_lock(Oid index_oid);
bool ii42_am_accelerator_build_in_progress(Oid index_oid);
void ii42_am_accelerator_build_unlock(Oid index_oid);
bool ii42_am_try_resident_build_lock(void);
void ii42_am_resident_build_unlock(void);
bool ii42_am_try_session_maintenance_lock(Oid index_oid);
bool ii42_am_maintenance_lock_held(Oid index_oid);
bool ii42_am_session_maintenance_lock_held(Oid index_oid);
void ii42_am_maintenance_unlock(Oid index_oid);
void ii42_am_session_maintenance_unlock(Oid index_oid);

void ii42_am_lock_append(Relation index_relation);
void ii42_am_unlock_append(Relation index_relation);

LockAcquireResult ii42_am_lock_writer_barrier_oid(
    Oid index_oid,
    LOCKMODE lock_mode,
    bool dont_wait
);
void ii42_am_unlock_writer_barrier_oid(
    Oid index_oid,
    LOCKMODE lock_mode
);

void ii42_am_pin_maintenance_xact(Relation index_relation);
LOCKMODE ii42_am_pinned_maintenance_mode(Oid index_oid);
bool ii42_am_pinned_maintenances_present(void);
void ii42_am_reparent_pinned_maintenances(
    SubTransactionId my_subid,
    SubTransactionId parent_subid,
    bool aborting
);
void ii42_am_clear_pinned_maintenances(void);

bool ii42_am_maintenance_tracking_enabled(Relation index_relation);
bool ii42_am_eventual_policy_enabled(Relation index_relation);
bool ii42_am_automatic_policy_enabled(Relation index_relation);
bool ii42_am_foreground_maintenance_enabled(Relation index_relation);
void ii42_am_maintenance_codec_error(
    const char *operation,
    ii42_status status
);

#endif
