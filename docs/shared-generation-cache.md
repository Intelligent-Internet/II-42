# Shared Generation Cache

`psql_bm25s` can keep large immutable decoded index payloads in a
server-visible generation cache. The goal is to reduce repeated cold-load,
decode, copy, and memory cost when many PostgreSQL backends query the same
large BM25 index.

For per-backend workspace limits, connection-pool sizing, and active warmup
commands, see [Connection Memory and Index Prewarming](connection-memory.md).

The durable source of truth remains the PostgreSQL index relation. Shared
generation cache entries are volatile acceleration structures. Small indexes
without the optional shared-preload arena can still use a selected
backend-local cache path. For generations that are intended to be shared, the
extension waits for a publisher or reports a shared-cache error instead of
silently loading many private copies across a connection pool.

## Why This Matters For Connection Pools

Large RAG and knowledge-base deployments often have:

- a large upstream connection pool;
- several application services wrapping the same database;
- many PostgreSQL backend processes touching the same large BM25 indexes;
- cold or recycled connections that need a fast first query.

Without a server-visible generation cache, each backend may independently load
and decode the same large immutable payload. That wastes memory and makes
first-query latency expensive under connection churn.

The cache design is tiered so high-performance deployments can opt into a
stronger PostgreSQL configuration, while smaller deployments still work without
extra setup.

## Cache Tiers

### Tier 0: Backend-Local Selected Path

This path is selected when the immutable generation is small and the optional
shared-preload arena is not active. A backend reads the index relation,
decodes the payload, and keeps its own local cache.

Use cases:

- small indexes;
- development environments;
- materialized delta overlays that are query/transaction-sensitive.

Properties:

- no special PostgreSQL configuration;
- lowest operational risk;
- highest repeated memory and first-query cost under many backends.

### Tier 1: DSM V2 Zero-Configuration Cache

The DSM path keeps immutable generation blocks in PostgreSQL dynamic shared
memory. It is automatic and does not require
`shared_preload_libraries`.

This path is the zero-configuration production sharing tier:

- the first backend publishes a large immutable generation;
- later backends attach the same generation when the generation key matches;
- descriptors, failure markers, and registry state are volatile;
- the index relation remains the durable source of truth;
- large share-eligible indexes wait behind the active publisher. If shared
  publication itself cannot succeed, the query errors rather than silently
  loading many private copies across a connection pool.

DSM V2 adds:

- per-generation single-flight coordination with PostgreSQL advisory locks, so
  many cold backends do not all publish the same generation;
- blocking, interruptible waits behind the active publisher for large
  share-eligible generations. A waiting backend rechecks shared cache state
  after the publisher releases the generation lock instead of timing out into
  its own backend-local copy;
- serialized large-segment attach after a publish, avoiding concurrent attach
  stampedes under connection churn;
- cold-load `ShareLock` protection so maintenance rewrites cannot change the
  index relation while a backend is reading raw payload pages;
- publish-failure markers and cache clear cleanup for failed publishes,
  corrupt descriptors, interrupted temp descriptors, and old lock files;
- a connection-churn benchmark that measures first-query latency across fresh
  PostgreSQL backends.

DSM still has a hard limit: every backend may need to attach/map the DSM
segment. For very large indexes, that mapping cost can be visible even when
decode/copy is avoided.

### Tier 2: Optional Shared-Preload Arena

The strongest path is an optional shared-memory arena initialized by
`shared_preload_libraries`.

Deployment shape:

```conf
shared_preload_libraries = 'psql_bm25s'
psql_bm25s.shared_generation_cache_size = '8GB'
```

In this mode, PostgreSQL reserves the cache arena during server start. Backend
processes inherit the mapping from the postmaster, so a fresh backend should
only need registry lookup and a lightweight local view before querying a
resident generation.

The resident-generation registry is sized automatically from the configured
arena. It is not a per-cycle preload drain limit: startup warmup keeps walking
all marked `auto_preload` indexes until every due resident that fits the arena
has been attempted.

On Linux, the arena is marked with `MADV_HUGEPAGE` when the extension
initializes shared memory. This is a best-effort latency hint for deployments
that set `/sys/kernel/mm/transparent_hugepage/shmem_enabled=advise`: resident
generations can then use shared transparent huge pages, reducing the page-table
fault cost when a fresh PostgreSQL backend first scans a large resident index.
The hint is optional and does not affect correctness.

This is the intended path for large connection-pool services where first-query
latency matters.

Indexes can opt into best-effort background preload with the `auto_preload`
reloption:

```sql
CREATE INDEX docs_bm25_idx
    ON docs USING psql_bm25s (body)
    WITH (auto_preload = 10);
```

`auto_preload = 0` is the default and only disables proactive background
preload. Positive values mark an index as preloadable; larger values are
attempted first. Equal-priority indexes are attempted by descending relation
size, then OID, so large hot generations reach the shared arena before smaller
indexes when services restart. The shared-preload background worker uses the
same global worker cap as eventual-consistency maintenance, but warmup has its
own
`psql_bm25s.preload_timer_interval_ms`. Each warmup cycle drains all currently
due marked indexes in priority order, so startup does not need a separate
batch-size setting and can finish residency before rebuild/catch-up work is
considered. This lets startup drain marked indexes quickly even when
rebuild/catch-up is throttled by
`psql_bm25s.maintenance_timer_interval_ms`. Workers skip indexes that are
already resident, currently loading, locked, physically corrupt, or too large
for the remaining arena. A stale but physically readable generation is still
preloaded first; the next maintenance cycle can then rebuild and publish a
clean generation without forcing queries into a cold path. Startup and
catch-up cycles prioritize preload before maintenance so marked indexes reach
the low-latency query path before background rebuild work starts. After
maintenance publishes a new generation, the old resident generation is retired
and the rebuild worker publishes the replacement into shared-preload when
it still has the finished index in memory. A later preload cycle is the fallback
when direct shared-preload publish is not possible.

For eventual-consistency indexes, the auto-preload worker also warms bounded
append-only delta pages from the PostgreSQL index relation when those delta
records remain eligible for query-time overlay. The immutable base generation
lives in the shared arena, but the delta tail remains in the index relation;
warming it in the background keeps first foreground queries from paying cold
`DataFileRead` latency after restart or active ingest. Indexes whose metapage
does not identify an append-only active generation are no longer
query-compatible; diagnostics report `unsupported_storage_layout`, and
background or manual maintenance must rebuild them from the heap.

When the shared-preload arena is configured, first use of an unmarked
share-capable index still uses shared publication. The difference is that
`auto_preload = 0` does not spend background startup capacity on that index
before it is queried.

Automatic maintenance also obeys
`psql_bm25s.maintenance_rebuild_memory_budget`. The worker chooses rebuild
builders conservatively because the shared-preload arena is usually protecting
foreground query latency:

| Builder | Automatic condition |
| --- | --- |
| `standard` | `standard_estimated_bytes <= budget_bytes * 0.60` and active payload is below the standard payload cap. |
| `compact` | Standard was rejected, `compact_estimated_bytes <= budget_bytes * 0.75`, and active payload is below the compact payload cap. |
| `spill` | Compact was rejected and `spill_estimated_bytes <= budget_bytes`. |
| skip | Spill estimate also exceeds `budget_bytes`; maintenance returns `reason=memory_budget`. |

The standard and compact headroom is deliberate. Their coarse estimates do not
fully capture allocator fragmentation, PostgreSQL executor state, or the memory
pressure from resident shared-preload generations. A skipped rebuild does not
evict a readable resident generation. This is intentionally conservative for
large indexes: queries should continue to use the resident generation, while an
operator can raise the budget or schedule a controlled rebuild window.

Typical large-index configuration:

```conf
shared_preload_libraries = 'psql_bm25s'
psql_bm25s.shared_generation_cache_size = '64GB'
psql_bm25s.maintenance_worker_limit = 1
psql_bm25s.preload_timer_interval_ms = 1000
psql_bm25s.maintenance_timer_interval_ms = 60000
psql_bm25s.maintenance_rebuild_memory_budget = '32768MB'
```

Verify runtime state with:

```sql
SELECT psql_bm25s_generation_cache_state('docs_bm25_idx'::regclass);
```

The state includes `shared_preload_resident`, `rebuild_builder`,
`standard_estimated_bytes`, `compact_estimated_bytes`,
`spill_estimated_bytes`, `rebuild_budget_bytes`, `active_background_workers`,
`active_preload_workers`, and `active_index_maintenance_workers`.
The background worker slot is shared by preload and rebuild catch-up, so use
the phase-specific counters and `pg_stat_activity.application_name` to
distinguish warmup from true index maintenance.

Standby servers run the same preload-only path. They never rebuild an index
while in recovery, but when WAL replay makes a newer generation visible, the
standby can retire its old resident generation and preload the replicated
current generation into its own shared-memory arena.

Properties:

- optional, never required for correctness;
- best fit for large indexes and many PostgreSQL backends;
- requires PostgreSQL configuration and restart;
- cache size is bounded by configured shared memory;
- when the arena is active, immutable generations are required to use a shared
  tier. If publication cannot succeed and no DSM tier can publish the
  generation, the query reports an error instead of silently falling back to
  private backend-local copies. This protects connection-pool deployments from
  one large generation copy per backend.

## Lookup Order

The intended production lookup order is:

1. Try the optional shared-preload arena when it is configured and the
   generation is resident.
2. If the arena is configured but the generation is not resident, marked
   `auto_preload` indexes wake the background preloader and wait for residency.
   Unmarked indexes wait for an active shared publisher when one exists;
   otherwise the first backend performs a single-flight publish into the
   shared-preload arena and later backends attach the resident generation. If
   shared publication cannot succeed, report an error instead of privately
   cold-loading one copy per backend.
3. Try the zero-configuration DSM cache for large generations when the arena is
   unavailable or cannot accept the generation.
4. Use backend-local decode only for small unmarked indexes or materialized
   overlays when the shared-preload arena is not configured.

This keeps deployment flexible:

- operators who can change PostgreSQL config get the lowest connection-pool
  first-query cost;
- operators who cannot change config still get DSM-based memory sharing;
- share-intended generations avoid silent private-copy amplification.

## Generation Key

A shared generation is valid only when the current index metapage matches the
generation key.

The key includes:

- database OID;
- index OID;
- relfilenode locator;
- metapage version and flags;
- `cache_epoch`;
- source type;
- payload sizes;
- document count;
- pending delta counters.

`REINDEX`, `psql_bm25s_index_refresh(...)`,
`psql_bm25s_index_maintain(...)`, and staged eventual maintenance all produce
a new observable generation key.

## Operational APIs

Current diagnostics:

- `psql_bm25s_generation_cache_state(index regclass)` reports observable
  generation key details, DSM share eligibility, whether sharing is currently
  required, descriptor validity, mapped DSM size, shared-preload
  configuration, shared-preload availability, resident entry counts, whether
  the requested index is resident or loading, arena usage, reusable arena
  blocks, obsolete entries, active shared-preload references, background worker
  slots split into preload versus index-maintenance phases, and cheap payload
  health fields such as `payload_health`, `payload_health_reason`, and
  `rebuild_required`.
- In `pg_stat_activity`, preload work reports `application_name =
  'psql_bm25s preload'` and real rebuild catch-up reports
  `application_name = 'psql_bm25s maintenance'`. The supervisor and
  maintenance-capable launch slot use `psql_bm25s background`; that label means
  the worker may run both phases, not that query readiness waits for rebuild.
- `psql_bm25s_generation_cache_clear()` clears backend-local state and
  best-effort volatile shared-generation descriptors, failure markers,
  interrupted temp descriptors, old lock files, and shared-preload registry
  entries. Main shared-memory blocks that no backend still references become
  immediately reusable; blocks with live backend-local views are marked
  obsolete and reclaimed when those backends release them.
- `psql_bm25s_generation_cache_preload(index regclass)` warms the best
  available cache tier for one index. In a configured shared-preload
  deployment, it can populate the main shared-memory arena before application
  traffic reaches the connection pool.

Shared-preload references are leased by the active scan, SRF call, or
transaction, not by the lifetime of an idle connection-pool backend. This keeps
obsolete generations reclaimable after a publish without waiting for
application connections to be closed.

Workspace retention is intentionally controlled separately from immutable
generation sharing. `psql_bm25s.workspace_cache_bytes` defaults to `32MB` per
backend and `psql_bm25s.workspace_idle_timeout` defaults to `60s`; see
[Connection Memory and Index Prewarming](connection-memory.md) for sizing
guidance.

Planned production diagnostics should also report:

- active cache tier: `shared_preload`, `dsm`, or `backend_local`;
- failed attach count;
- evicted or invalidated generation count;

## Benchmark Requirements

The normal query/build benchmark matrix must remain non-regressing.

Additional cache-specific benchmarks should cover:

- many fresh PostgreSQL backends querying the same large index;
- first-query latency distribution for backend-local, DSM, and shared-preload
  tiers;
- resident shared-memory bytes and per-backend private memory;
- repeated `REINDEX` or maintenance generation changes;
- descriptor corruption, shared-memory attach failure, and required-share
  error behavior.

Current connection-churn benchmark script:

```bash
python3 scripts/benchmark_generation_cache_churn.py \
    --dataset webis-touche2020 \
    --max-cases 50 \
    --connections 4 \
    --parallelism 4 \
    --cases-per-connection 1
```

The key production benchmark is not only query QPS. It is also:

```text
fresh backend first-query latency under connection-pool churn
```

That benchmark is required before promoting the optional shared-preload path
as the recommended deployment for large multi-application services.

Current shared-preload smoke:

```bash
python3 scripts/test_shared_preload_generation_cache.py \
    --bindir /path/to/postgresql/bin
```

The smoke starts a temporary PostgreSQL cluster with `psql_bm25s` in
`shared_preload_libraries`, configures a bounded main shared-memory arena,
builds a small BM25 index, prewarms it, runs queries from independent `psql`
backends, and asserts that the shared-preload arena has one ready resident
generation.
