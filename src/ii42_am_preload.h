#ifndef II42_AM_PRELOAD_H
#define II42_AM_PRELOAD_H

#include "postgres.h"

#include "storage/lwlock.h"
#include "utils/rel.h"

#include "ii42_am_meta.h"

#define II42_AM_UNIFIED_WARM_MARKER_MAGIC UINT32_C(0x4957514d)
#define II42_AM_UNIFIED_WARM_MARKER_VERSION UINT16_C(2)
#define II42_AM_UNIFIED_WARM_QUERY_METADATA_COMPLETE UINT16_C(0x0001)
#define II42_AM_UNIFIED_WARM_ACCELERATOR_AUTHORITY UINT16_C(0x0002)

typedef struct ii42_am_unified_warm_marker
{
    uint32 magic;
    uint16 version;
    uint16 flags;
    uint64 pages_warmed;
    uint64 authority_id;
    ii42_segment_object_ref authority;
} ii42_am_unified_warm_marker;

typedef enum ii42_am_preload_kind
{
    II42_AM_PRELOAD_UNIFIED_WARM = 4,
    II42_AM_PRELOAD_HOT_FOLD = 5,
    II42_AM_PRELOAD_DOCUMENT_LENGTHS = 6,
    II42_AM_PRELOAD_VALIDATED_PAGES = 7,
    II42_AM_PRELOAD_RESIDENT_FOLD = 8,
    II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP = 9
} ii42_am_preload_kind;

typedef enum ii42_am_preload_reserve_result
{
    II42_AM_PRELOAD_RESERVE_FAILED = 0,
    II42_AM_PRELOAD_RESERVE_READY,
    II42_AM_PRELOAD_RESERVE_LOADING,
    II42_AM_PRELOAD_RESERVE_NEW
} ii42_am_preload_reserve_result;

typedef enum ii42_am_preload_admission_state
{
    II42_AM_PRELOAD_ADMISSION_UNAVAILABLE = 0,
    II42_AM_PRELOAD_ADMISSION_RESIDENT,
    II42_AM_PRELOAD_ADMISSION_LOADING,
    II42_AM_PRELOAD_ADMISSION_ZERO_PAYLOAD,
    II42_AM_PRELOAD_ADMISSION_OVERSIZED,
    II42_AM_PRELOAD_ADMISSION_ADMISSIBLE,
    II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SLOT,
    II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SPACE
} ii42_am_preload_admission_state;

/*
 * These handles are values, not registry authority. Callers must use the
 * typed accessors and release/commit/abort functions below.
 */
typedef struct ii42_am_preload_lease
{
    uintptr_t private_token;
    const void *private_payload;
    Size private_payload_size;
} ii42_am_preload_lease;

typedef struct ii42_am_preload_reservation
{
    uintptr_t private_token;
    void *private_payload;
    Size private_payload_size;
} ii42_am_preload_reservation;

typedef struct ii42_am_preload_status
{
    bool available;
    bool resident;
    bool loading;
    bool hot_fold_current;
    bool hot_fold_loading;
    bool resident_fold_current;
    bool resident_fold_loading;
    bool candidate_fits_empty;
    bool has_free_slot;
    bool has_reusable_slot;
    bool has_evictable_relation;
    Size arena_size;
    Size used;
    Size free_bytes;
    Size resident_fold_bytes;
    Size candidate_bytes;
    Size reusable_bytes;
    uint32 entry_capacity;
    uint32 hash_capacity;
    uint32 entries;
    uint32 ready_entries;
    uint32 obsolete_entries;
    uint32 reusable_entries;
    uint32 refcounted_entries;
    uint32 unified_warm_entries;
    uint32 hot_fold_entries;
    uint32 resident_fold_entries;
    uint32 document_length_entries;
    uint32 document_tid_lookup_entries;
    uint32 validated_page_entries;
    uint64 relation_entry_evictions;
    uint64 relation_entry_eviction_bytes;
    uint64 access_clock;
    ii42_am_preload_admission_state admission_state;
} ii42_am_preload_status;

void ii42_am_preload_define_gucs(void);
int ii42_am_preload_configured_mb(void);
int ii42_am_preload_test_registry_fill(void);

Size ii42_am_preload_shmem_size(void);
void ii42_am_preload_shmem_startup(LWLock *lock);

bool ii42_am_preload_available(void);
bool ii42_am_preload_cache_available(void);
bool ii42_am_preload_exact_state(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    bool *resident_out,
    bool *loading_out
);
void ii42_am_preload_retire_obsolete(
    Relation index_relation,
    const ii42_am_meta_page *meta
);
void ii42_am_preload_rekey_generation(
    Relation index_relation,
    const ii42_am_meta_page *old_meta,
    const ii42_am_meta_page *current_meta
);
void ii42_am_preload_retire_exact(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind
);

bool ii42_am_preload_attach(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    ii42_am_preload_lease *lease_out
);
const void *ii42_am_preload_lease_payload(
    const ii42_am_preload_lease *lease
);
void *ii42_am_preload_lease_mutable_payload(
    ii42_am_preload_lease *lease
);
Size ii42_am_preload_lease_payload_size(
    const ii42_am_preload_lease *lease
);
void ii42_am_preload_lease_release(ii42_am_preload_lease *lease);
void ii42_am_preload_lease_retire(ii42_am_preload_lease *lease);

ii42_am_preload_reserve_result ii42_am_preload_reserve(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    Size payload_size,
    ii42_am_preload_reservation *reservation_out
);
void *ii42_am_preload_reservation_payload(
    ii42_am_preload_reservation *reservation
);
bool ii42_am_preload_reservation_commit(
    ii42_am_preload_reservation *reservation
);
void ii42_am_preload_reservation_abort(
    ii42_am_preload_reservation *reservation
);
void ii42_am_preload_abort_active_publication(void);

int ii42_am_preload_clear(void);
void ii42_am_preload_status_snapshot(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind requested_kind,
    Size candidate_bytes,
    ii42_am_preload_status *status_out
);
const char *ii42_am_preload_admission_state_name(
    ii42_am_preload_admission_state state
);

#endif
