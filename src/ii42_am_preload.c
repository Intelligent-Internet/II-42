#include "postgres.h"

#include <sys/mman.h>
#include <unistd.h>

#include "miscadmin.h"
#include "port/atomics.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "utils/guc.h"

#include "ii42_am_hot_fold.h"
#include "ii42_am_options.h"
#include "ii42_am_preload.h"

#define II42_AM_PRELOAD_MAGIC UINT32_C(0x50323542)
#define II42_AM_PRELOAD_VERSION 32
#define II42_AM_PRELOAD_MIN_ENTRIES 1024
#define II42_AM_PRELOAD_MAX_ENTRIES 65536
#define II42_AM_PRELOAD_ENTRY_TARGET_BYTES (4 * 1024 * 1024)
#define II42_AM_PRELOAD_MAX_MB 1048576

typedef struct ii42_am_preload_entry
{
    bool in_use;
    bool ready;
    bool obsolete;
    uint8 generation_kind;
    int auto_preload_priority;
    Oid database_oid;
    Oid index_oid;
    RelFileLocator locator;
    ii42_am_meta_page meta;
    Size offset;
    Size mapped_size;
    Size allocation_size;
    uint64 key_hash;
    pg_atomic_uint32 refcount;
    pg_atomic_uint64 last_access_counter;
} ii42_am_preload_entry;

typedef struct ii42_am_preload_hash_slot
{
    uint64 key_hash;
    uint32 entry_slot_plus_one;
} ii42_am_preload_hash_slot;

typedef struct ii42_am_preload_control
{
    uint32 magic;
    uint32 version;
    Size arena_size;
    Size used;
    uint64 relation_entry_evictions;
    uint64 relation_entry_eviction_bytes;
    pg_atomic_uint64 access_clock;
    uint32 entry_capacity;
    uint32 hash_capacity;
    ii42_am_preload_entry entries[FLEXIBLE_ARRAY_MEMBER];
} ii42_am_preload_control;

static bool ii42_am_preload_gucs_initialized = false;
static int ii42_shared_runtime_size_mb = 0;
static int ii42_test_shared_preload_registry_capacity = 0;
static int ii42_test_shared_preload_registry_fill = 0;
static ii42_am_preload_control *ii42_preload = NULL;
static LWLock *ii42_preload_lock = NULL;
static uintptr_t ii42_active_publication_token = 0;

static bool
ii42_am_preload_checked_add(Size left, Size right, Size *result_out)
{
    if (left > SIZE_MAX - right)
    {
        return false;
    }
    *result_out = left + right;
    return true;
}

static bool
ii42_am_preload_checked_mul(Size left, Size right, Size *result_out)
{
    if (left == 0 || right == 0)
    {
        *result_out = 0;
        return true;
    }
    if (left > SIZE_MAX / right)
    {
        return false;
    }
    *result_out = left * right;
    return true;
}

static bool
ii42_am_preload_kind_valid(ii42_am_preload_kind kind)
{
    return kind == II42_AM_PRELOAD_UNIFIED_WARM ||
        kind == II42_AM_PRELOAD_HOT_FOLD ||
        kind == II42_AM_PRELOAD_DOCUMENT_LENGTHS ||
        kind == II42_AM_PRELOAD_VALIDATED_PAGES ||
        kind == II42_AM_PRELOAD_RESIDENT_FOLD ||
        kind == II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP;
}

static bool
ii42_am_preload_kind_rekey_safe(ii42_am_preload_kind kind)
{
    /*
     * Query metadata carries and validates serving authority in its payload,
     * so it can survive manifest-only successors. Resident folds remain tied
     * to an exact immutable generation.
     */
    return kind == II42_AM_PRELOAD_UNIFIED_WARM ||
        kind == II42_AM_PRELOAD_HOT_FOLD ||
        kind == II42_AM_PRELOAD_DOCUMENT_LENGTHS ||
        kind == II42_AM_PRELOAD_VALIDATED_PAGES ||
        kind == II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP;
}

static bool
ii42_am_preload_kind_uses_base_identity(
    ii42_am_preload_kind kind
)
{
    return kind == II42_AM_PRELOAD_UNIFIED_WARM ||
        kind == II42_AM_PRELOAD_DOCUMENT_LENGTHS ||
        kind == II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP;
}

static bool
ii42_am_preload_payload_size_valid(
    ii42_am_preload_kind kind,
    Size payload_size
)
{
    if (kind == II42_AM_PRELOAD_UNIFIED_WARM)
    {
        return payload_size == sizeof(ii42_am_unified_warm_marker);
    }
    if (kind == II42_AM_PRELOAD_HOT_FOLD)
    {
        return payload_size >= sizeof(ii42_am_hot_fold_header);
    }
    if (kind == II42_AM_PRELOAD_DOCUMENT_LENGTHS)
    {
        return payload_size >= sizeof(uint32) * 4 &&
            payload_size % sizeof(uint32) == 0;
    }
    if (kind == II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP)
    {
        return payload_size >= sizeof(uint32) * 4;
    }
    if (kind == II42_AM_PRELOAD_RESIDENT_FOLD)
    {
        return payload_size > 0;
    }
    return kind == II42_AM_PRELOAD_VALIDATED_PAGES &&
        payload_size >= sizeof(pg_atomic_uint64) &&
        payload_size % sizeof(pg_atomic_uint64) == 0;
}

static Size
ii42_am_preload_arena_size(void)
{
    uint64 bytes;

    if (ii42_shared_runtime_size_mb <= 0)
    {
        return 0;
    }
    bytes = (uint64) ii42_shared_runtime_size_mb * 1024ULL *
        1024ULL;
    if (bytes > (uint64) SIZE_MAX)
    {
        ereport(ERROR, (errmsg("ii42 shared runtime arena is too large")));
    }
    return (Size) bytes;
}

static uint32
ii42_am_preload_entry_capacity_for_arena(Size arena_size)
{
    uint64 capacity = II42_AM_PRELOAD_MIN_ENTRIES;
    uint64 arena_capacity;

    if (ii42_test_shared_preload_registry_capacity > 0)
    {
        return (uint32) ii42_test_shared_preload_registry_capacity;
    }
    arena_capacity = (uint64) arena_size /
        II42_AM_PRELOAD_ENTRY_TARGET_BYTES;
    if (arena_capacity > capacity)
    {
        capacity = arena_capacity;
    }
    if (capacity > II42_AM_PRELOAD_MAX_ENTRIES)
    {
        capacity = II42_AM_PRELOAD_MAX_ENTRIES;
    }
    return (uint32) capacity;
}

static uint32
ii42_am_preload_entry_capacity(void)
{
    if (ii42_preload == NULL || ii42_preload->entry_capacity == 0)
    {
        return 0;
    }
    return ii42_preload->entry_capacity;
}

static uint32
ii42_am_preload_hash_capacity_for_entries(uint32 entry_capacity)
{
    uint32 capacity = 1;
    uint64 target = (uint64) entry_capacity * 2;

    while ((uint64) capacity < target)
    {
        if (capacity > UINT32_MAX / 2)
        {
            ereport(ERROR, (errmsg("ii42 shared preload hash is too large")));
        }
        capacity *= 2;
    }
    return capacity;
}

static Size
ii42_am_preload_control_size(uint32 entry_capacity)
{
    Size entries_size;
    Size hash_size;
    Size control_size;
    uint32 hash_capacity;

    if (!ii42_am_preload_checked_mul(
            entry_capacity,
            sizeof(ii42_am_preload_entry),
            &entries_size) ||
        !ii42_am_preload_checked_add(
            offsetof(ii42_am_preload_control, entries),
            entries_size,
            &control_size))
    {
        ereport(ERROR, (errmsg("ii42 shared preload registry is too large")));
    }
    control_size = MAXALIGN(control_size);
    hash_capacity =
        ii42_am_preload_hash_capacity_for_entries(entry_capacity);
    if (!ii42_am_preload_checked_mul(
            hash_capacity,
            sizeof(ii42_am_preload_hash_slot),
            &hash_size) ||
        !ii42_am_preload_checked_add(
            control_size,
            hash_size,
            &control_size))
    {
        ereport(ERROR, (errmsg("ii42 shared preload hash is too large")));
    }
    return MAXALIGN(control_size);
}

Size
ii42_am_preload_shmem_size(void)
{
    Size arena_size = ii42_am_preload_arena_size();
    uint32 entry_capacity =
        ii42_am_preload_entry_capacity_for_arena(arena_size);

    return ii42_am_preload_control_size(entry_capacity) + arena_size;
}

bool
ii42_am_preload_available(void)
{
    return ii42_preload != NULL &&
        ii42_preload_lock != NULL &&
        ii42_preload->magic == II42_AM_PRELOAD_MAGIC &&
        ii42_preload->version == II42_AM_PRELOAD_VERSION &&
        ii42_preload->entry_capacity > 0 &&
        ii42_preload->hash_capacity >= ii42_preload->entry_capacity * 2 &&
        (ii42_preload->hash_capacity &
         (ii42_preload->hash_capacity - 1)) == 0;
}

bool
ii42_am_preload_cache_available(void)
{
    return ii42_am_preload_available() && ii42_preload->arena_size > 0;
}

static char *
ii42_am_preload_arena_base(void)
{
    if (!ii42_am_preload_available())
    {
        return NULL;
    }
    return ((char *) ii42_preload) +
        ii42_am_preload_control_size(ii42_preload->entry_capacity);
}

static ii42_am_preload_hash_slot *
ii42_am_preload_hash_slots(void)
{
    Size entries_size;
    Size offset;

    if (!ii42_am_preload_available())
    {
        return NULL;
    }
    if (!ii42_am_preload_checked_mul(
            ii42_preload->entry_capacity,
            sizeof(ii42_am_preload_entry),
            &entries_size) ||
        !ii42_am_preload_checked_add(
            offsetof(ii42_am_preload_control, entries),
            entries_size,
            &offset))
    {
        ereport(ERROR, (errmsg("ii42 shared preload hash is too large")));
    }
    return (ii42_am_preload_hash_slot *) (
        ((char *) ii42_preload) + MAXALIGN(offset)
    );
}

static uint64
ii42_am_preload_hash_mix(uint64 hash, uint64 value)
{
    return hash ^ (
        value +
        UINT64_C(0x9e3779b97f4a7c15) +
        (hash << 6) +
        (hash >> 2)
    );
}

static bool
ii42_am_preload_base_identity_matches(
    const ii42_am_meta_page *left,
    const ii42_am_meta_page *right
)
{
    return left != NULL && right != NULL &&
        left->magic == right->magic &&
        left->version == right->version &&
        left->page_kind == right->page_kind &&
        left->cache_epoch == right->cache_epoch &&
        left->storage_version == right->storage_version &&
        left->source_type == right->source_type;
}

static uint64
ii42_am_preload_entry_key_hash(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind
)
{
    const unsigned char *signature;
    uint64 hash = UINT64_C(0xcbf29ce484222325);
    size_t index;

    hash = ii42_am_preload_hash_mix(hash, MyDatabaseId);
    hash = ii42_am_preload_hash_mix(
        hash,
        RelationGetRelid(index_relation)
    );
    hash = ii42_am_preload_hash_mix(
        hash,
        index_relation->rd_locator.spcOid
    );
    hash = ii42_am_preload_hash_mix(
        hash,
        index_relation->rd_locator.dbOid
    );
    hash = ii42_am_preload_hash_mix(
        hash,
        index_relation->rd_locator.relNumber
    );
    hash = ii42_am_preload_hash_mix(hash, kind);
    hash = ii42_am_preload_hash_mix(hash, meta->magic);
    hash = ii42_am_preload_hash_mix(hash, meta->version);
    hash = ii42_am_preload_hash_mix(hash, meta->page_kind);
    hash = ii42_am_preload_hash_mix(hash, meta->cache_epoch);
    hash = ii42_am_preload_hash_mix(hash, meta->storage_version);
    hash = ii42_am_preload_hash_mix(hash, meta->source_type);
    if (!ii42_am_meta_uses_convergent_segment_storage(meta))
    {
        ereport(
            ERROR,
            (errmsg("ii42 shared residency requires a convergent root"))
        );
    }
    if (!ii42_am_preload_kind_uses_base_identity(kind))
    {
        signature =
            (const unsigned char *) meta->segment_read_root_bytes;
        for (index = 0;
             index < sizeof(meta->segment_read_root_bytes);
             index++)
        {
            hash = ii42_am_preload_hash_mix(hash, signature[index]);
        }
    }
    /* Base-keyed query metadata authenticates authority in its payload. */
    return hash == 0 ? UINT64_C(1) : hash;
}

static bool
ii42_am_preload_entry_has_block(const ii42_am_preload_entry *entry)
{
    if (entry == NULL ||
        !ii42_am_preload_cache_available() ||
        entry->mapped_size == 0 ||
        entry->allocation_size == 0 ||
        entry->mapped_size > entry->allocation_size ||
        entry->offset > ii42_preload->arena_size ||
        entry->allocation_size >
            ii42_preload->arena_size - entry->offset)
    {
        return false;
    }
    return true;
}

static void
ii42_am_preload_entry_reset(ii42_am_preload_entry *entry)
{
    if (entry == NULL)
    {
        return;
    }
    memset(entry, 0, sizeof(*entry));
    pg_atomic_init_u32(&entry->refcount, 0);
    pg_atomic_init_u64(&entry->last_access_counter, 0);
}

static void
ii42_am_preload_try_rewind_locked(void)
{
    for (;;)
    {
        Size new_used = 0;
        int tail_slot = -1;
        uint32 index;

        for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
        {
            ii42_am_preload_entry *entry = &ii42_preload->entries[index];
            Size end;

            if (entry->allocation_size == 0)
            {
                continue;
            }
            if (!ii42_am_preload_entry_has_block(entry))
            {
                if (!entry->in_use)
                {
                    ii42_am_preload_entry_reset(entry);
                }
                continue;
            }
            end = entry->offset + entry->allocation_size;
            if (end > new_used)
            {
                new_used = end;
                tail_slot = (int) index;
            }
        }
        if (tail_slot >= 0 && !ii42_preload->entries[tail_slot].in_use)
        {
            ii42_am_preload_entry_reset(&ii42_preload->entries[tail_slot]);
            continue;
        }
        ii42_preload->used = new_used;
        break;
    }
}

static void
ii42_am_preload_touch_entry_locked(ii42_am_preload_entry *entry)
{
    uint64 access_counter;

    if (entry == NULL)
    {
        return;
    }
    access_counter = pg_atomic_fetch_add_u64(
        &ii42_preload->access_clock,
        1
    ) + 1;
    pg_atomic_write_u64(&entry->last_access_counter, access_counter);
}

static void
ii42_am_preload_hash_remove_locked(const ii42_am_preload_entry *entry);

static void
ii42_am_preload_retire_entry_locked(ii42_am_preload_entry *entry)
{
    if (entry == NULL || !entry->in_use)
    {
        return;
    }
    ii42_am_preload_hash_remove_locked(entry);
    entry->ready = false;
    if (pg_atomic_read_u32(&entry->refcount) == 0)
    {
        entry->in_use = false;
        entry->obsolete = false;
    }
    else
    {
        entry->obsolete = true;
    }
}

static bool
ii42_am_preload_entry_same_generation(
    const ii42_am_preload_entry *entry,
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind
)
{
    if (entry == NULL || !entry->in_use || entry->obsolete ||
        entry->generation_kind != kind ||
        !ii42_am_preload_kind_valid(kind) ||
        !ii42_am_preload_payload_size_valid(kind, entry->mapped_size))
    {
        return false;
    }
    if (entry->database_oid != MyDatabaseId ||
        entry->index_oid != RelationGetRelid(index_relation) ||
        !RelFileLocatorEquals(entry->locator, index_relation->rd_locator))
    {
        return false;
    }
    if (ii42_am_preload_kind_uses_base_identity(kind))
    {
        return ii42_am_preload_base_identity_matches(&entry->meta, meta);
    }
    return ii42_am_generation_identity_matches(&entry->meta, meta);
}

static bool
ii42_am_preload_entry_matches_current_kind(
    const ii42_am_preload_entry *entry,
    Relation index_relation,
    const ii42_am_meta_page *meta
)
{
    if (entry == NULL)
    {
        return false;
    }
    return ii42_am_preload_entry_same_generation(
        entry,
        index_relation,
        meta,
        (ii42_am_preload_kind) entry->generation_kind
    );
}

static int
ii42_am_preload_hash_lookup_locked(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind
)
{
    ii42_am_preload_hash_slot *hash_slots =
        ii42_am_preload_hash_slots();
    uint32 hash_capacity = ii42_preload->hash_capacity;
    uint64 key_hash;
    uint32 probe;

    if (hash_slots == NULL || hash_capacity == 0)
    {
        return -1;
    }
    key_hash = ii42_am_preload_entry_key_hash(index_relation, meta, kind);
    for (probe = 0; probe < hash_capacity; probe++)
    {
        uint32 bucket = (uint32) (
            (key_hash + probe) & (hash_capacity - 1)
        );
        ii42_am_preload_hash_slot *hash_slot = &hash_slots[bucket];
        uint32 entry_slot_plus_one = hash_slot->entry_slot_plus_one;

        if (entry_slot_plus_one == 0)
        {
            return -1;
        }
        if (entry_slot_plus_one > ii42_preload->entry_capacity)
        {
            ereport(ERROR, (errmsg("ii42 shared preload hash is corrupt")));
        }
        if (hash_slot->key_hash == key_hash &&
            ii42_am_preload_entry_same_generation(
                &ii42_preload->entries[entry_slot_plus_one - 1],
                index_relation,
                meta,
                kind))
        {
            return (int) entry_slot_plus_one - 1;
        }
    }
    return -1;
}

static void
ii42_am_preload_hash_insert_locked(int entry_slot)
{
    ii42_am_preload_hash_slot *hash_slots;
    ii42_am_preload_entry *entry;
    uint64 key_hash;
    uint32 hash_capacity;
    uint32 probe;

    if (entry_slot < 0 ||
        (uint32) entry_slot >= ii42_preload->entry_capacity)
    {
        ereport(ERROR, (errmsg("ii42 shared preload slot is invalid")));
    }
    hash_slots = ii42_am_preload_hash_slots();
    hash_capacity = ii42_preload->hash_capacity;
    entry = &ii42_preload->entries[entry_slot];
    key_hash = entry->key_hash;
    if (hash_slots == NULL || hash_capacity == 0 || key_hash == 0)
    {
        ereport(
            ERROR,
            (errmsg("ii42 shared preload hash is unavailable"))
        );
    }
    for (probe = 0; probe < hash_capacity; probe++)
    {
        uint32 bucket = (uint32) (
            (key_hash + probe) & (hash_capacity - 1)
        );
        ii42_am_preload_hash_slot *hash_slot = &hash_slots[bucket];

        if (hash_slot->entry_slot_plus_one != 0)
        {
            continue;
        }
        hash_slot->key_hash = key_hash;
        hash_slot->entry_slot_plus_one = (uint32) entry_slot + 1;
        return;
    }
    ereport(ERROR, (errmsg("ii42 shared preload hash is full")));
}

static void
ii42_am_preload_hash_remove_locked(const ii42_am_preload_entry *entry)
{
    ii42_am_preload_hash_slot *hash_slots;
    uint32 entry_slot;
    uint32 hash_capacity;
    uint32 mask;
    uint32 probe;

    if (entry == NULL || entry->key_hash == 0)
    {
        return;
    }
    entry_slot = (uint32) (entry - ii42_preload->entries);
    if (entry_slot >= ii42_preload->entry_capacity)
    {
        ereport(ERROR, (errmsg("ii42 shared preload slot is invalid")));
    }
    hash_slots = ii42_am_preload_hash_slots();
    hash_capacity = ii42_preload->hash_capacity;
    mask = hash_capacity - 1;
    for (probe = 0; probe < hash_capacity; probe++)
    {
        uint32 bucket = (uint32) ((entry->key_hash + probe) & mask);
        ii42_am_preload_hash_slot *hash_slot = &hash_slots[bucket];

        if (hash_slot->entry_slot_plus_one == 0)
        {
            return;
        }
        if (hash_slot->entry_slot_plus_one == entry_slot + 1 &&
            hash_slot->key_hash == entry->key_hash)
        {
            uint32 hole = bucket;
            uint32 scan = (hole + 1) & mask;

            while (hash_slots[scan].entry_slot_plus_one != 0)
            {
                uint32 home = (uint32) (
                    hash_slots[scan].key_hash & mask
                );
                uint32 scan_distance = (scan - home) & mask;
                uint32 hole_distance = (hole - home) & mask;

                if (scan_distance > hole_distance)
                {
                    hash_slots[hole] = hash_slots[scan];
                    hole = scan;
                }
                scan = (scan + 1) & mask;
            }
            memset(&hash_slots[hole], 0, sizeof(hash_slots[hole]));
            return;
        }
    }
}

static bool
ii42_am_preload_try_acquire_ref(ii42_am_preload_entry *entry)
{
    uint32 expected = pg_atomic_read_u32(&entry->refcount);

    for (;;)
    {
        if (expected == UINT32_MAX)
        {
            return false;
        }
        if (pg_atomic_compare_exchange_u32(
                &entry->refcount,
                &expected,
                expected + 1))
        {
            return true;
        }
    }
}

static void
ii42_am_preload_release_token(uintptr_t token)
{
    ii42_am_preload_entry *entry;
    uint32 expected;
    uint32 slot;

    if (token == 0 || !ii42_am_preload_available())
    {
        return;
    }
    slot = (uint32) (token - 1);
    if (slot >= ii42_am_preload_entry_capacity())
    {
        return;
    }
    entry = &ii42_preload->entries[slot];
    expected = pg_atomic_read_u32(&entry->refcount);
    for (;;)
    {
        if (expected == 0)
        {
            return;
        }
        if (pg_atomic_compare_exchange_u32(
                &entry->refcount,
                &expected,
                expected - 1))
        {
            break;
        }
    }
    if (expected > 1)
    {
        return;
    }
    LWLockAcquire(ii42_preload_lock, LW_SHARED);
    if (pg_atomic_read_u32(&entry->refcount) != 0 || !entry->obsolete)
    {
        LWLockRelease(ii42_preload_lock);
        return;
    }
    LWLockRelease(ii42_preload_lock);

    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    if (pg_atomic_read_u32(&entry->refcount) == 0 && entry->obsolete)
    {
        entry->in_use = false;
        entry->ready = false;
        entry->obsolete = false;
    }
    ii42_am_preload_try_rewind_locked();
    LWLockRelease(ii42_preload_lock);
}

static void
ii42_am_preload_retire_obsolete_locked(
    Relation index_relation,
    const ii42_am_meta_page *meta
)
{
    Oid index_oid = RelationGetRelid(index_relation);
    uint32 index;

    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];

        if (!entry->in_use ||
            entry->database_oid != MyDatabaseId ||
            entry->index_oid != index_oid)
        {
            continue;
        }
        if (!ii42_am_preload_entry_matches_current_kind(
                entry,
                index_relation,
                meta))
        {
            ii42_am_preload_retire_entry_locked(entry);
        }
    }
    ii42_am_preload_try_rewind_locked();
}

void
ii42_am_preload_retire_obsolete(
    Relation index_relation,
    const ii42_am_meta_page *meta
)
{
    if (!ii42_am_preload_available())
    {
        return;
    }
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    ii42_am_preload_retire_obsolete_locked(index_relation, meta);
    LWLockRelease(ii42_preload_lock);
}

void
ii42_am_preload_rekey_generation(
    Relation index_relation,
    const ii42_am_meta_page *old_meta,
    const ii42_am_meta_page *current_meta
)
{
    Oid index_oid;
    uint32 index;

    if (!ii42_am_preload_available() || index_relation == NULL ||
        old_meta == NULL || current_meta == NULL ||
        !ii42_am_meta_uses_convergent_segment_storage(old_meta) ||
        !ii42_am_meta_uses_convergent_segment_storage(current_meta))
    {
        return;
    }
    index_oid = RelationGetRelid(index_relation);
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];
        ii42_am_preload_kind kind;
        int current_slot;

        if (!entry->in_use || entry->obsolete ||
            entry->database_oid != MyDatabaseId ||
            entry->index_oid != index_oid ||
            !RelFileLocatorEquals(
                entry->locator,
                index_relation->rd_locator))
        {
            continue;
        }
        kind = (ii42_am_preload_kind) entry->generation_kind;
        if (!ii42_am_preload_entry_same_generation(
                entry,
                index_relation,
                old_meta,
                kind))
        {
            continue;
        }
        if (!ii42_am_preload_kind_rekey_safe(kind))
        {
            ii42_am_preload_retire_entry_locked(entry);
            continue;
        }
        current_slot = ii42_am_preload_hash_lookup_locked(
            index_relation,
            current_meta,
            kind
        );
        if (current_slot >= 0 && current_slot != (int) index)
        {
            ii42_am_preload_retire_entry_locked(entry);
            continue;
        }
        ii42_am_preload_hash_remove_locked(entry);
        entry->meta = *current_meta;
        entry->key_hash = ii42_am_preload_entry_key_hash(
            index_relation,
            current_meta,
            kind
        );
        ii42_am_preload_hash_insert_locked((int) index);
    }
    ii42_am_preload_retire_obsolete_locked(index_relation, current_meta);
    LWLockRelease(ii42_preload_lock);
}

void
ii42_am_preload_retire_exact(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind
)
{
    int slot;

    if (!ii42_am_preload_available())
    {
        return;
    }
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    slot = ii42_am_preload_hash_lookup_locked(index_relation, meta, kind);
    if (slot >= 0)
    {
        ii42_am_preload_retire_entry_locked(&ii42_preload->entries[slot]);
        ii42_am_preload_try_rewind_locked();
    }
    LWLockRelease(ii42_preload_lock);
}

static bool
ii42_am_preload_exact_state_locked(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    bool *resident_out,
    bool *loading_out
)
{
    bool resident = false;
    bool loading = false;
    int slot;

    slot = ii42_am_preload_hash_lookup_locked(index_relation, meta, kind);
    if (slot >= 0)
    {
        resident = ii42_preload->entries[slot].ready;
        loading = !resident;
    }
    if (resident_out != NULL)
    {
        *resident_out = resident;
    }
    if (loading_out != NULL)
    {
        *loading_out = loading;
    }
    return resident || loading;
}

bool
ii42_am_preload_exact_state(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    bool *resident_out,
    bool *loading_out
)
{
    bool found;

    if (resident_out != NULL)
    {
        *resident_out = false;
    }
    if (loading_out != NULL)
    {
        *loading_out = false;
    }
    if (!ii42_am_preload_available())
    {
        return false;
    }
    LWLockAcquire(ii42_preload_lock, LW_SHARED);
    found = ii42_am_preload_exact_state_locked(
        index_relation,
        meta,
        kind,
        resident_out,
        loading_out
    );
    LWLockRelease(ii42_preload_lock);
    return found;
}

bool
ii42_am_preload_attach(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    ii42_am_preload_lease *lease_out
)
{
    ii42_am_preload_entry *entry;
    char *arena_base;
    int slot;

    if (lease_out == NULL)
    {
        ereport(ERROR, (errmsg("ii42 preload lease output is required")));
    }
    memset(lease_out, 0, sizeof(*lease_out));
    if (!ii42_am_preload_available())
    {
        return false;
    }
    LWLockAcquire(ii42_preload_lock, LW_SHARED);
    slot = ii42_am_preload_hash_lookup_locked(index_relation, meta, kind);
    if (slot < 0)
    {
        LWLockRelease(ii42_preload_lock);
        return false;
    }
    entry = &ii42_preload->entries[slot];
    if (!entry->ready || !ii42_am_preload_try_acquire_ref(entry))
    {
        LWLockRelease(ii42_preload_lock);
        return false;
    }
    ii42_am_preload_touch_entry_locked(entry);
    arena_base = ii42_am_preload_arena_base();
    if (arena_base == NULL || !ii42_am_preload_entry_has_block(entry))
    {
        LWLockRelease(ii42_preload_lock);
        ii42_am_preload_release_token((uintptr_t) slot + 1);
        return false;
    }
    lease_out->private_token = (uintptr_t) slot + 1;
    lease_out->private_payload = arena_base + entry->offset;
    lease_out->private_payload_size = entry->mapped_size;
    LWLockRelease(ii42_preload_lock);
    return true;
}

const void *
ii42_am_preload_lease_payload(const ii42_am_preload_lease *lease)
{
    return lease == NULL ? NULL : lease->private_payload;
}

void *
ii42_am_preload_lease_mutable_payload(ii42_am_preload_lease *lease)
{
    return lease == NULL ? NULL : (void *) lease->private_payload;
}

Size
ii42_am_preload_lease_payload_size(const ii42_am_preload_lease *lease)
{
    return lease == NULL ? 0 : lease->private_payload_size;
}

void
ii42_am_preload_lease_release(ii42_am_preload_lease *lease)
{
    uintptr_t token;

    if (lease == NULL)
    {
        return;
    }
    token = lease->private_token;
    memset(lease, 0, sizeof(*lease));
    ii42_am_preload_release_token(token);
}

void
ii42_am_preload_lease_retire(ii42_am_preload_lease *lease)
{
    ii42_am_preload_entry *entry;
    uintptr_t token;
    uint32 slot;

    if (lease == NULL)
    {
        return;
    }
    token = lease->private_token;
    memset(lease, 0, sizeof(*lease));
    if (token == 0 || !ii42_am_preload_available())
    {
        return;
    }
    slot = (uint32) (token - 1);
    if (slot >= ii42_am_preload_entry_capacity())
    {
        ii42_am_preload_release_token(token);
        return;
    }

    /* The held reference prevents this slot from being reused mid-retire. */
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    entry = &ii42_preload->entries[slot];
    if (entry->in_use && pg_atomic_read_u32(&entry->refcount) > 0)
    {
        ii42_am_preload_retire_entry_locked(entry);
    }
    LWLockRelease(ii42_preload_lock);
    ii42_am_preload_release_token(token);
}

static bool
ii42_am_preload_entry_relation_evictable(
    ii42_am_preload_entry *entry,
    Relation index_relation,
    const ii42_am_meta_page *meta,
    bool allow_preload_entries,
    ii42_am_preload_kind requested_kind
)
{
    if (entry == NULL || !entry->in_use || entry->obsolete ||
        !entry->ready ||
        !ii42_am_preload_kind_valid(
            (ii42_am_preload_kind) entry->generation_kind) ||
        pg_atomic_read_u32(&entry->refcount) != 0 ||
        entry->database_oid != MyDatabaseId)
    {
        return false;
    }
    if (!allow_preload_entries && entry->auto_preload_priority > 0)
    {
        return false;
    }
    if (ii42_am_preload_entry_same_generation(
            entry,
            index_relation,
            meta,
            (ii42_am_preload_kind) entry->generation_kind))
    {
        /*
         * The resident fold serves the unfiltered peak route while the
         * reverse TID directory serves exact filtered membership. They are
         * complementary views of one root, not interchangeable cache space.
         * Letting either evict the other makes background preload turn a
         * previously warm product route into an O(N) fallback.
         */
        return entry->generation_kind != II42_AM_PRELOAD_UNIFIED_WARM &&
            entry->generation_kind != II42_AM_PRELOAD_RESIDENT_FOLD &&
            entry->generation_kind !=
                II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP &&
            entry->generation_kind != requested_kind;
    }
    if (allow_preload_entries &&
        entry->auto_preload_priority >
            ii42_am_auto_preload_priority(index_relation))
    {
        /* A lower-priority preload must not displace a preferred relation. */
        return false;
    }
    return true;
}

static bool
ii42_am_preload_has_evictable_relation_locked(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    Size payload_size,
    bool allow_preload_entries,
    ii42_am_preload_kind requested_kind
)
{
    Size used = MAXALIGN(ii42_preload->used);
    uint32 index;

    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];
        Size end;

        if (!ii42_am_preload_entry_relation_evictable(
                entry,
                index_relation,
                meta,
                allow_preload_entries,
                requested_kind))
        {
            continue;
        }
        end = entry->offset + entry->allocation_size;
        if (entry->allocation_size >= payload_size || end == used)
        {
            return true;
        }
    }
    return false;
}

static bool
ii42_am_preload_evict_relation_entry_locked(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    Size payload_size,
    bool allow_preload_entries,
    ii42_am_preload_kind requested_kind
)
{
    Size used = MAXALIGN(ii42_preload->used);
    int best_slot = -1;
    uint32 index;

    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];
        ii42_am_preload_entry *best;
        bool entry_fits;
        bool best_fits;
        Size end;

        if (!ii42_am_preload_entry_relation_evictable(
                entry,
                index_relation,
                meta,
                allow_preload_entries,
                requested_kind))
        {
            continue;
        }
        end = entry->offset + entry->allocation_size;
        entry_fits = entry->allocation_size >= payload_size || end == used;
        if (!entry_fits)
        {
            continue;
        }
        if (best_slot < 0)
        {
            best_slot = (int) index;
            continue;
        }
        best = &ii42_preload->entries[best_slot];
        if ((entry->auto_preload_priority > 0) !=
            (best->auto_preload_priority > 0))
        {
            if (entry->auto_preload_priority == 0)
            {
                best_slot = (int) index;
            }
            continue;
        }
        if (entry->auto_preload_priority != best->auto_preload_priority)
        {
            if (entry->auto_preload_priority < best->auto_preload_priority)
            {
                best_slot = (int) index;
            }
            continue;
        }
        best_fits = best->allocation_size >= payload_size;
        if (entry->allocation_size >= payload_size && !best_fits)
        {
            best_slot = (int) index;
            continue;
        }
        if ((entry->allocation_size >= payload_size) == best_fits &&
            entry->allocation_size != best->allocation_size)
        {
            if (entry->allocation_size < best->allocation_size)
            {
                best_slot = (int) index;
            }
            continue;
        }
        if ((entry->allocation_size >= payload_size) == best_fits &&
            pg_atomic_read_u64(&entry->last_access_counter) !=
                pg_atomic_read_u64(&best->last_access_counter) &&
            pg_atomic_read_u64(&entry->last_access_counter) <
                pg_atomic_read_u64(&best->last_access_counter))
        {
            best_slot = (int) index;
        }
    }
    if (best_slot < 0)
    {
        return false;
    }
    if (ii42_preload->relation_entry_evictions < UINT64_MAX)
    {
        ii42_preload->relation_entry_evictions++;
    }
    if (UINT64_MAX - ii42_preload->relation_entry_eviction_bytes >=
        ii42_preload->entries[best_slot].mapped_size)
    {
        ii42_preload->relation_entry_eviction_bytes +=
            ii42_preload->entries[best_slot].mapped_size;
    }
    else
    {
        ii42_preload->relation_entry_eviction_bytes = UINT64_MAX;
    }
    ii42_am_preload_retire_entry_locked(&ii42_preload->entries[best_slot]);
    ii42_am_preload_try_rewind_locked();
    return true;
}

static bool
ii42_am_preload_publish_token(uintptr_t token, bool ready)
{
    ii42_am_preload_entry *entry;
    bool published = false;
    uint32 slot;

    if (token == 0 || !ii42_am_preload_available())
    {
        return false;
    }
    slot = (uint32) (token - 1);
    if (slot >= ii42_am_preload_entry_capacity())
    {
        return false;
    }
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    entry = &ii42_preload->entries[slot];
    if (ready && entry->in_use && !entry->obsolete &&
        pg_atomic_read_u32(&entry->refcount) > 0)
    {
        entry->ready = true;
        entry->obsolete = false;
        published = true;
    }
    else
    {
        ii42_am_preload_retire_entry_locked(entry);
        ii42_am_preload_try_rewind_locked();
    }
    LWLockRelease(ii42_preload_lock);
    return published;
}

ii42_am_preload_reserve_result
ii42_am_preload_reserve(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind kind,
    Size payload_size,
    ii42_am_preload_reservation *reservation_out
)
{
    Size offset;
    Size allocation_size;
    Size end;
    int free_slot = -1;
    int reusable_slot = -1;
    int existing_slot;
    uint32 index;
    uint32 eviction_attempts = 0;
    uint32 eviction_limit;

    if (reservation_out == NULL)
    {
        ereport(ERROR, (errmsg("ii42 preload reservation is required")));
    }
    memset(reservation_out, 0, sizeof(*reservation_out));
    if (!ii42_am_preload_cache_available() ||
        !ii42_am_preload_payload_size_valid(kind, payload_size))
    {
        return II42_AM_PRELOAD_RESERVE_FAILED;
    }
    if (ii42_active_publication_token != 0)
    {
        ereport(ERROR, (errmsg("ii42 preload publication is already active")));
    }
    eviction_limit = ii42_am_preload_entry_capacity();
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
retry_allocate:
    free_slot = -1;
    reusable_slot = -1;
    ii42_am_preload_retire_obsolete_locked(index_relation, meta);
    existing_slot = ii42_am_preload_hash_lookup_locked(
        index_relation,
        meta,
        kind
    );
    if (existing_slot >= 0)
    {
        bool ready = ii42_preload->entries[existing_slot].ready;

        LWLockRelease(ii42_preload_lock);
        return ready
            ? II42_AM_PRELOAD_RESERVE_READY
            : II42_AM_PRELOAD_RESERVE_LOADING;
    }
    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];

        if (!entry->in_use && !ii42_am_preload_entry_has_block(entry) &&
            free_slot < 0)
        {
            free_slot = (int) index;
        }
        if (!entry->in_use && ii42_am_preload_entry_has_block(entry) &&
            entry->allocation_size >= payload_size &&
            (reusable_slot < 0 ||
             entry->allocation_size <
                ii42_preload->entries[reusable_slot].allocation_size))
        {
            reusable_slot = (int) index;
        }
    }
    if (reusable_slot >= 0)
    {
        free_slot = reusable_slot;
        offset = ii42_preload->entries[free_slot].offset;
        allocation_size =
            ii42_preload->entries[free_slot].allocation_size;
    }
    else
    {
        offset = MAXALIGN(ii42_preload->used);
        allocation_size = MAXALIGN(payload_size);
        if (free_slot < 0 ||
            !ii42_am_preload_checked_add(
                offset,
                allocation_size,
                &end) ||
            end > ii42_preload->arena_size)
        {
            if (eviction_attempts < eviction_limit &&
                ii42_am_preload_evict_relation_entry_locked(
                    index_relation,
                    meta,
                    payload_size,
                    true,
                    kind))
            {
                eviction_attempts++;
                goto retry_allocate;
            }
            LWLockRelease(ii42_preload_lock);
            return II42_AM_PRELOAD_RESERVE_FAILED;
        }
        ii42_preload->used = MAXALIGN(end);
    }
    if (free_slot < 0)
    {
        LWLockRelease(ii42_preload_lock);
        return II42_AM_PRELOAD_RESERVE_FAILED;
    }
    ii42_am_preload_entry_reset(&ii42_preload->entries[free_slot]);
    ii42_preload->entries[free_slot].in_use = true;
    ii42_preload->entries[free_slot].generation_kind = kind;
    ii42_preload->entries[free_slot].auto_preload_priority =
        ii42_am_auto_preload_priority(index_relation);
    ii42_preload->entries[free_slot].database_oid = MyDatabaseId;
    ii42_preload->entries[free_slot].index_oid =
        RelationGetRelid(index_relation);
    ii42_preload->entries[free_slot].locator = index_relation->rd_locator;
    ii42_preload->entries[free_slot].meta = *meta;
    ii42_preload->entries[free_slot].offset = offset;
    ii42_preload->entries[free_slot].mapped_size = payload_size;
    ii42_preload->entries[free_slot].allocation_size = allocation_size;
    ii42_preload->entries[free_slot].key_hash =
        ii42_am_preload_entry_key_hash(index_relation, meta, kind);
    pg_atomic_write_u32(&ii42_preload->entries[free_slot].refcount, 1);
    ii42_am_preload_touch_entry_locked(&ii42_preload->entries[free_slot]);
    ii42_am_preload_hash_insert_locked(free_slot);
    reservation_out->private_token = (uintptr_t) free_slot + 1;
    reservation_out->private_payload =
        ii42_am_preload_arena_base() + offset;
    reservation_out->private_payload_size = payload_size;
    ii42_active_publication_token = reservation_out->private_token;
    LWLockRelease(ii42_preload_lock);
    return II42_AM_PRELOAD_RESERVE_NEW;
}

void *
ii42_am_preload_reservation_payload(
    ii42_am_preload_reservation *reservation
)
{
    return reservation == NULL ? NULL : reservation->private_payload;
}

bool
ii42_am_preload_reservation_commit(
    ii42_am_preload_reservation *reservation
)
{
    bool published;
    uintptr_t token;

    if (reservation == NULL || reservation->private_token == 0)
    {
        return false;
    }
    token = reservation->private_token;
    published = ii42_am_preload_publish_token(token, true);
    ii42_am_preload_release_token(token);
    memset(reservation, 0, sizeof(*reservation));
    if (ii42_active_publication_token == token)
    {
        ii42_active_publication_token = 0;
    }
    return published;
}

void
ii42_am_preload_reservation_abort(
    ii42_am_preload_reservation *reservation
)
{
    uintptr_t token;

    if (reservation == NULL || reservation->private_token == 0)
    {
        return;
    }
    token = reservation->private_token;
    ii42_am_preload_publish_token(token, false);
    ii42_am_preload_release_token(token);
    memset(reservation, 0, sizeof(*reservation));
    if (ii42_active_publication_token == token)
    {
        ii42_active_publication_token = 0;
    }
}

void
ii42_am_preload_abort_active_publication(void)
{
    ii42_am_preload_reservation reservation = {0};

    reservation.private_token = ii42_active_publication_token;
    ii42_am_preload_reservation_abort(&reservation);
}

int
ii42_am_preload_clear(void)
{
    int cleared = 0;
    uint32 index;

    if (!ii42_am_preload_available())
    {
        return 0;
    }
    LWLockAcquire(ii42_preload_lock, LW_EXCLUSIVE);
    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        if (ii42_preload->entries[index].in_use)
        {
            ii42_am_preload_retire_entry_locked(
                &ii42_preload->entries[index]
            );
            cleared++;
        }
    }
    ii42_am_preload_try_rewind_locked();
    LWLockRelease(ii42_preload_lock);
    return cleared;
}

void
ii42_am_preload_status_snapshot(
    Relation index_relation,
    const ii42_am_meta_page *meta,
    ii42_am_preload_kind requested_kind,
    Size candidate_bytes,
    ii42_am_preload_status *status_out
)
{
    uint32 index;

    if (status_out == NULL)
    {
        return;
    }
    memset(status_out, 0, sizeof(*status_out));
    status_out->candidate_bytes = candidate_bytes;
    status_out->available = ii42_am_preload_available();
    status_out->admission_state = II42_AM_PRELOAD_ADMISSION_UNAVAILABLE;
    if (!status_out->available)
    {
        return;
    }
    LWLockAcquire(ii42_preload_lock, LW_SHARED);
    ii42_am_preload_exact_state_locked(
        index_relation,
        meta,
        requested_kind,
        &status_out->resident,
        &status_out->loading
    );
    ii42_am_preload_exact_state_locked(
        index_relation,
        meta,
        II42_AM_PRELOAD_HOT_FOLD,
        &status_out->hot_fold_current,
        &status_out->hot_fold_loading
    );
    ii42_am_preload_exact_state_locked(
        index_relation,
        meta,
        II42_AM_PRELOAD_RESIDENT_FOLD,
        &status_out->resident_fold_current,
        &status_out->resident_fold_loading
    );
    if (status_out->resident_fold_current)
    {
        int slot = ii42_am_preload_hash_lookup_locked(
            index_relation,
            meta,
            II42_AM_PRELOAD_RESIDENT_FOLD
        );

        if (slot >= 0 && ii42_preload->entries[slot].ready)
        {
            status_out->resident_fold_bytes =
                ii42_preload->entries[slot].mapped_size;
        }
    }
    status_out->arena_size = ii42_preload->arena_size;
    status_out->entry_capacity = ii42_preload->entry_capacity;
    status_out->hash_capacity = ii42_preload->hash_capacity;
    status_out->used = ii42_preload->used;
    if (status_out->arena_size > status_out->used)
    {
        status_out->free_bytes =
            status_out->arena_size - status_out->used;
    }
    status_out->candidate_fits_empty =
        candidate_bytes <= status_out->arena_size;
    status_out->relation_entry_evictions =
        ii42_preload->relation_entry_evictions;
    status_out->relation_entry_eviction_bytes =
        ii42_preload->relation_entry_eviction_bytes;
    status_out->access_clock = pg_atomic_read_u64(
        &ii42_preload->access_clock
    );
    for (index = 0; index < ii42_am_preload_entry_capacity(); index++)
    {
        ii42_am_preload_entry *entry = &ii42_preload->entries[index];

        if (entry->in_use)
        {
            status_out->entries++;
            if (entry->generation_kind == II42_AM_PRELOAD_UNIFIED_WARM)
            {
                status_out->unified_warm_entries++;
            }
            else if (entry->generation_kind == II42_AM_PRELOAD_HOT_FOLD)
            {
                status_out->hot_fold_entries++;
            }
            else if (entry->generation_kind ==
                         II42_AM_PRELOAD_RESIDENT_FOLD)
            {
                status_out->resident_fold_entries++;
            }
            else if (entry->generation_kind ==
                         II42_AM_PRELOAD_DOCUMENT_LENGTHS)
            {
                status_out->document_length_entries++;
            }
            else if (entry->generation_kind ==
                         II42_AM_PRELOAD_DOCUMENT_TID_LOOKUP)
            {
                status_out->document_tid_lookup_entries++;
            }
            else if (entry->generation_kind ==
                         II42_AM_PRELOAD_VALIDATED_PAGES)
            {
                status_out->validated_page_entries++;
            }
            if (entry->obsolete)
            {
                status_out->obsolete_entries++;
            }
            if (pg_atomic_read_u32(&entry->refcount) > 0)
            {
                status_out->refcounted_entries++;
            }
            if (entry->ready)
            {
                status_out->ready_entries++;
            }
        }
        else if (ii42_am_preload_entry_has_block(entry))
        {
            status_out->reusable_entries++;
            status_out->reusable_bytes += entry->allocation_size;
        }
        if (!entry->in_use)
        {
            if (ii42_am_preload_entry_has_block(entry) &&
                entry->allocation_size >= candidate_bytes)
            {
                status_out->has_reusable_slot = true;
            }
            else if (!ii42_am_preload_entry_has_block(entry))
            {
                status_out->has_free_slot = true;
            }
        }
    }
    status_out->has_evictable_relation =
        ii42_am_preload_has_evictable_relation_locked(
            index_relation,
            meta,
            candidate_bytes,
            true,
            requested_kind
        );
    LWLockRelease(ii42_preload_lock);

    if (status_out->resident)
    {
        status_out->admission_state = II42_AM_PRELOAD_ADMISSION_RESIDENT;
    }
    else if (status_out->loading)
    {
        status_out->admission_state = II42_AM_PRELOAD_ADMISSION_LOADING;
    }
    else if (candidate_bytes == 0)
    {
        status_out->admission_state =
            II42_AM_PRELOAD_ADMISSION_ZERO_PAYLOAD;
    }
    else if (!status_out->candidate_fits_empty)
    {
        status_out->admission_state = II42_AM_PRELOAD_ADMISSION_OVERSIZED;
    }
    else if (status_out->has_reusable_slot ||
             status_out->has_evictable_relation ||
             (status_out->has_free_slot &&
              candidate_bytes <= status_out->free_bytes))
    {
        status_out->admission_state = II42_AM_PRELOAD_ADMISSION_ADMISSIBLE;
    }
    else if (!status_out->has_free_slot)
    {
        status_out->admission_state =
            II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SLOT;
    }
    else
    {
        status_out->admission_state =
            II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SPACE;
    }
}

const char *
ii42_am_preload_admission_state_name(ii42_am_preload_admission_state state)
{
    switch (state)
    {
        case II42_AM_PRELOAD_ADMISSION_UNAVAILABLE:
            return "unavailable";
        case II42_AM_PRELOAD_ADMISSION_RESIDENT:
            return "resident";
        case II42_AM_PRELOAD_ADMISSION_LOADING:
            return "loading";
        case II42_AM_PRELOAD_ADMISSION_ZERO_PAYLOAD:
            return "zero_payload";
        case II42_AM_PRELOAD_ADMISSION_OVERSIZED:
            return "oversized";
        case II42_AM_PRELOAD_ADMISSION_ADMISSIBLE:
            return "admissible";
        case II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SLOT:
            return "blocked_no_slot";
        case II42_AM_PRELOAD_ADMISSION_BLOCKED_NO_SPACE:
            return "blocked_no_space";
    }
    return "unknown";
}

static void
ii42_am_preload_advise_hugepage(void)
{
#ifdef MADV_HUGEPAGE
    char *arena_base;
    long page_size;
    uintptr_t address;
    uintptr_t aligned_address;
    Size adjust;
    Size advised_size;

    if (!ii42_am_preload_cache_available())
    {
        return;
    }
    arena_base = ii42_am_preload_arena_base();
    if (arena_base == NULL)
    {
        return;
    }
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
    {
        return;
    }
    address = (uintptr_t) arena_base;
    aligned_address = address - (address % (uintptr_t) page_size);
    adjust = (Size) (address - aligned_address);
    if (!ii42_am_preload_checked_add(
            ii42_preload->arena_size,
            adjust,
            &advised_size))
    {
        return;
    }
    if (madvise(
            (void *) aligned_address,
            advised_size,
            MADV_HUGEPAGE) != 0)
    {
        ereport(
            DEBUG1,
            (errmsg("ii42 shared preload hugepage advice failed: %m"))
        );
    }
#endif
}

void
ii42_am_preload_shmem_startup(LWLock *lock)
{
    Size shmem_size = ii42_am_preload_shmem_size();
    Size arena_size;
    uint32 index;
    bool found;

    if (lock == NULL)
    {
        ereport(FATAL, (errmsg("ii42 preload lock is unavailable")));
    }
    ii42_preload_lock = lock;
    ii42_preload = ShmemInitStruct(
        "ii42 shared runtime arena",
        shmem_size,
        &found
    );
    if (!found)
    {
        arena_size = ii42_am_preload_arena_size();
        memset(ii42_preload, 0, shmem_size);
        ii42_preload->magic = II42_AM_PRELOAD_MAGIC;
        ii42_preload->version = II42_AM_PRELOAD_VERSION;
        ii42_preload->arena_size = arena_size;
        ii42_preload->entry_capacity =
            ii42_am_preload_entry_capacity_for_arena(arena_size);
        ii42_preload->hash_capacity =
            ii42_am_preload_hash_capacity_for_entries(
                ii42_preload->entry_capacity
            );
        pg_atomic_init_u64(&ii42_preload->access_clock, 0);
        for (index = 0; index < ii42_preload->entry_capacity; index++)
        {
            pg_atomic_init_u32(&ii42_preload->entries[index].refcount, 0);
            pg_atomic_init_u64(
                &ii42_preload->entries[index].last_access_counter,
                0
            );
        }
        if (ii42_test_shared_preload_registry_fill > 0)
        {
            uint32 fill =
                (uint32) ii42_test_shared_preload_registry_fill;

            if (fill >= ii42_preload->entry_capacity)
            {
                ereport(
                    FATAL,
                    (
                        errmsg("ii42 test registry fill exceeds its capacity"),
                        errdetail(
                            "fill=%u capacity=%u",
                            fill,
                            ii42_preload->entry_capacity
                        )
                    )
                );
            }
            for (index = 0; index < fill; index++)
            {
                ii42_am_preload_entry *entry =
                    &ii42_preload->entries[index];
                uint64 key_hash = UINT64_C(0xcbf29ce484222325);

                key_hash = ii42_am_preload_hash_mix(
                    key_hash,
                    (uint64) index + 1
                );
                key_hash = ii42_am_preload_hash_mix(key_hash, fill);
                entry->in_use = true;
                entry->ready = true;
                entry->generation_kind = II42_AM_PRELOAD_UNIFIED_WARM;
                entry->key_hash = key_hash == 0 ? UINT64_C(1) : key_hash;
                ii42_am_preload_hash_insert_locked((int) index);
            }
            pg_atomic_write_u64(&ii42_preload->access_clock, fill);
        }
    }
    else if (!ii42_am_preload_available())
    {
        ereport(
            FATAL,
            (
                errmsg("ii42 shared preload ABI does not match this binary"),
                errdetail(
                    "found magic=%08x version=%u, expected magic=%08x "
                    "version=%u",
                    ii42_preload->magic,
                    ii42_preload->version,
                    II42_AM_PRELOAD_MAGIC,
                    II42_AM_PRELOAD_VERSION
                ),
                errhint("Restart PostgreSQL with one consistent ii42 binary.")
            )
        );
    }
    ii42_am_preload_advise_hugepage();
}

static bool
ii42_am_preload_check_test_registry_capacity(
    int *new_value,
    void **extra,
    GucSource source
)
{
    (void) extra;
    (void) source;
    if (*new_value != 0 && *new_value < II42_AM_PRELOAD_MIN_ENTRIES)
    {
        GUC_check_errdetail(
            "The value must be zero or at least %u.",
            II42_AM_PRELOAD_MIN_ENTRIES
        );
        return false;
    }
    return true;
}

void
ii42_am_preload_define_gucs(void)
{
    if (!process_shared_preload_libraries_in_progress ||
        ii42_am_preload_gucs_initialized)
    {
        return;
    }
    DefineCustomIntVariable(
        "ii42.shared_runtime_size",
        "Sets the shared runtime and derived-residency arena size.",
        "When ii42 is loaded through shared_preload_libraries, this "
        "reserves shared memory for runtime queues, exact-root markers, and "
        "optional HOT_FOLD and exact-root resident projections. Durable "
        "posting authority remains in relation pages. A positive value is "
        "required for SAE indexes; BM25-only indexes remain page-native "
        "when it is 0.",
        &ii42_shared_runtime_size_mb,
        0,
        0,
        II42_AM_PRELOAD_MAX_MB,
        PGC_POSTMASTER,
        GUC_UNIT_MB,
        NULL,
        NULL,
        NULL
    );
    DefineCustomIntVariable(
        "ii42.test_shared_preload_registry_capacity",
        "Overrides shared preload registry metadata capacity for tests.",
        "This restart-only test hook changes metadata capacity without "
        "inflating the payload arena. Zero keeps production auto-sizing.",
        &ii42_test_shared_preload_registry_capacity,
        0,
        0,
        II42_AM_PRELOAD_MAX_ENTRIES,
        PGC_POSTMASTER,
        GUC_NOT_IN_SAMPLE,
        ii42_am_preload_check_test_registry_capacity,
        NULL,
        NULL
    );
    DefineCustomIntVariable(
        "ii42.test_shared_preload_registry_fill",
        "Pre-fills shared preload hash entries for capacity tests.",
        "This restart-only test hook creates metadata-only occupancy before "
        "backends start. Zero disables synthetic entries.",
        &ii42_test_shared_preload_registry_fill,
        0,
        0,
        II42_AM_PRELOAD_MAX_ENTRIES - 1,
        PGC_POSTMASTER,
        GUC_NOT_IN_SAMPLE,
        NULL,
        NULL,
        NULL
    );
    ii42_am_preload_gucs_initialized = true;
}

int
ii42_am_preload_configured_mb(void)
{
    return ii42_shared_runtime_size_mb;
}

int
ii42_am_preload_test_registry_fill(void)
{
    return ii42_test_shared_preload_registry_fill;
}
