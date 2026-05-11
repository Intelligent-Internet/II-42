# Index Policy

`psql_bm25s` uses one public maintenance switch:
`consistency`. The option selects the contract between write cost, query
freshness, and automatic convergence.

For the complete `CREATE INDEX ... WITH (...)` parameter reference, see
[Index Parameters](index-parameters.md).

## Consistency Policies

| Policy | Best Fit | Query Contract | Write And Convergence | Optional Parameters |
| --- | --- | --- | --- | --- |
| `realtime` | Default for ordinary mutable tables.<br>Committed writes should be searchable immediately. | Exact committed BM25 semantics.<br>Queries may wait, use exact overlay, or trigger required foreground maintenance. | Records exact maintenance debt.<br>`INSERT`, `UPDATE`, `DELETE`, and `VACUUM` participate automatically. | `auto_rebuild_threshold`<br>`auto_rebuild_delta_bytes`<br>`auto_rebuild_churn_ratio` |
| `eventual` | Large RAG and AI knowledge-base tables.<br>Read/write latency matters more than immediate BM25 freshness. | Query-first and responsive.<br>May return stable base-index results while maintenance catches up. | Records durable debt.<br>Bounded workers or schedulers converge the index later. | `auto_rebuild_threshold`<br>`auto_rebuild_delta_bytes`<br>`query_overlay_max_records`<br>`query_overlay_max_bytes` |
| `manual` | Mostly static corpora, benchmarks, and explicit refresh jobs. | Reads the current base index.<br>Emits a stale-index `NOTICE` when pending stale state exists. | Marks stale with lightweight counters.<br>No self-triggered maintenance. | Usually none.<br>Use explicit maintenance APIs or an external scheduler. |

## Policy Details

### `realtime`

Use `realtime` for mutable tables where BM25 results must reflect committed
writes immediately. This is the default and is closest to PostgreSQL's usual
index freshness expectation.

Internally, the index records exact maintenance debt. Pending state is either
merged through an exact bounded overlay or consolidated by foreground
maintenance when required. If a single text-like delta payload is too large for
the compact delta page format, `realtime` marks the index stale and forces
foreground consolidation instead of failing the write.

Canonical reads preserve exact committed BM25 semantics. A query may wait for
in-flight maintenance to stabilize, use an exact base-plus-delta overlay, or
trigger required foreground maintenance.

`INSERT`, `UPDATE`, `DELETE`, and `VACUUM` participate in automatic
maintenance. Repeated writes in one top-level transaction are batched.

### `eventual`

Use `eventual` for large RAG and AI knowledge-base tables where foreground
read/write latency matters more than immediate BM25 freshness.

Internally, the index is query-first. Writes record durable maintenance debt,
but foreground queries and writers do not perform long rebuilds. Small pending
deltas can be overlaid; larger or unstable pending state returns the stable
base index and converges later. If one changed row is too large for the delta
page format, `eventual` skips that exact payload, marks the index stale, and
leaves convergence to background or explicit maintenance.

Queries stay responsive by design. They use bounded overlays when cheap and
otherwise return base-index results with maintenance debt left for convergence.
`query_overlay_max_records` and `query_overlay_max_bytes` are foreground query
budgets only; exceeding them does not by itself trigger a full background
rebuild. Use `auto_rebuild_threshold` and `auto_rebuild_delta_bytes` to control
when background workers should consolidate the durable delta. Short-term stale
BM25 results are allowed.

The recommended deployment path loads `psql_bm25s` through
`shared_preload_libraries`. That starts a timer-based generic catch-up
supervisor which periodically wakes bounded workers. Each worker maintains at
most one due index, exits, and uses advisory locks to avoid duplicate work on
the same index. Due indexes are prioritized by stale state, hot shared-cache
presence, urgency, and debt. Deployments without `shared_preload_libraries`
fall back to lightweight touch wakeups from commit/query/vacuum paths; those
touches only launch the same generic worker and never wait for maintenance.
External schedulers may also call the maintenance helpers. Canceled maintenance
before the final swap is retryable and leaves the old index plus debt intact.

### `manual`

Use `manual` for mostly static corpora, benchmark runs, one-shot imports, or
deployments that refresh indexes explicitly.

Internally, writes do not record exact delta payloads or delete tombstones.
`INSERT` and `UPDATE` mark the index stale and may increment lightweight
pending-write counters. `VACUUM` marks stale for deletes. The base index remains
readable until an explicit maintenance job rebuilds it.

Queries return the current base-index result and emit a stale-index `NOTICE`
when the index has pending stale state. Results are usable, but they are not
fresh until maintenance runs.

No self-triggered background maintenance runs for this policy. Use
`psql_bm25s_index_maintain(...)`, `psql_bm25s_index_try_maintain(...)`,
`psql_bm25s_index_refresh(...)`, or `REINDEX INDEX` on an explicit schedule.

## Defaults and Valid Parameters

When `consistency` is omitted, `psql_bm25s` uses `realtime`. This matches the
PostgreSQL default expectation that committed writes should be visible to later
queries without opting into stale results.

Maintenance reloptions are validated against the selected consistency policy.
Only explicitly supplied reloptions are checked; internal default values do not
make unrelated parameters active.

| Policy | Valid maintenance parameters | Invalid maintenance parameters |
| --- | --- | --- |
| `realtime` | `auto_rebuild_threshold`, `auto_rebuild_delta_bytes`, `auto_rebuild_churn_ratio` | `query_overlay_max_records`, `query_overlay_max_bytes` |
| `eventual` | `auto_rebuild_threshold`, `auto_rebuild_delta_bytes`, `query_overlay_max_records`, `query_overlay_max_bytes` | `auto_rebuild_churn_ratio` |
| `manual` | none | all automatic-maintenance, overlay, and worker parameters |

The default realtime thresholds are all disabled:

- `auto_rebuild_threshold = 0`
- `auto_rebuild_delta_bytes = 0`
- `auto_rebuild_churn_ratio = 0.0`

That preserves the original eager strong-consistency behavior unless the user
explicitly chooses deferred realtime consolidation.

The default eventual overlay limits are intentionally conservative:

- `auto_rebuild_threshold = 50000`
- `auto_rebuild_delta_bytes = 0`
- `query_overlay_max_records = 50000`
- `query_overlay_max_bytes = 16777216`

These defaults allow bounded committed deltas to stay searchable before
convergence and bound foreground query work by record and byte size. The
rebuild thresholds are deliberately separate from the query overlay limits so
large indexes do not rebuild a multi-gigabyte active generation merely because
a small delta became too expensive to merge inline. Global maintenance scheduling is
controlled by `psql_bm25s.maintenance_worker_limit` and
`psql_bm25s.maintenance_timer_interval_ms`; shared-preload startup warmup is
controlled separately by `psql_bm25s.preload_timer_interval_ms` so maintenance
throttling does not delay resident-cache warmup. Set them in
`postgresql.conf` and reload or restart PostgreSQL. Set
`auto_rebuild_threshold = 0` explicitly only when the lowest write-path payload
cost is more important than short-term delta searchability.

## Scenario Presets

Default realtime index:

```sql
CREATE INDEX docs_body_bm25_idx
    ON docs USING psql_bm25s (body)
    WITH (
        consistency = 'realtime'
    );
```

Query-first knowledge-base index:

```sql
CREATE INDEX docs_body_bm25_idx
    ON docs USING psql_bm25s (body)
    WITH (
        consistency = 'eventual'
    );
```

Manual-refresh index:

```sql
CREATE INDEX docs_body_bm25_idx
    ON docs USING psql_bm25s (body)
    WITH (
        consistency = 'manual'
    );
```

## Maintenance Mechanics

The index stores exact BM25 payloads plus maintenance metadata on the metapage:

- `pending_write_tuples`
- `pending_delete_tuples`
- `delta_records`
- `delta_bytes`
- `stale`
- `rebuild_count`

Realtime and eventual policies record automatic-maintenance debt. Manual policy
does not store exact delta payloads and does not materialize delete tombstones.
Its pending counters are only lightweight stale-state signals for explicit
refresh tooling, not data that can be overlaid into exact query results.

For text-like indexes, eventual consistency has an important write-path
optimization: when PostgreSQL reports that an indexed value is unchanged,
`psql_bm25s` can skip storing a full delta payload and batch lightweight debt
until commit. This is the path intended for large backfills that update
non-indexed columns on knowledge-base tables.

`DELETE` correctness is synchronized through normal PostgreSQL `VACUUM`.
VACUUM materializes delete tombstones for exact deferred overlays in automatic
policies. In manual policy, VACUUM marks the index stale.

## Parameters

Core policy parameter:

| Parameter | Default | Applies to | Meaning |
| --- | --- | --- | --- |
| `consistency` | `'realtime'` | all indexes | Selects the maintenance contract: `'realtime'`, `'eventual'`, or `'manual'`. |

Automatic maintenance debt parameters:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `auto_rebuild_threshold` | `0` for `realtime`, `50000` for `eventual` | Valid with `realtime` and `eventual`. In realtime, pending write/delete counts at or above this value make consolidation eligible. In eventual, a positive value enables exact delta payloads for small pending changes before convergence. |
| `auto_rebuild_delta_bytes` | `0` | Valid with `realtime` and `eventual`. Pending delta-byte threshold for consolidation. `0` disables this byte threshold. |
| `auto_rebuild_churn_ratio` | `0.0` | Valid only with `realtime`. Pending changes divided by indexed document count. Positive values consolidate after enough churn. |

Eventual query parameters:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `query_overlay_max_records` | `50000` | Maximum pending delta records a query may merge inline. `0` disables inline overlay. |
| `query_overlay_max_bytes` | `16777216` | Maximum pending delta bytes a query may merge inline. `0` disables inline overlay. |

These parameters can only be explicitly set when
`consistency = 'eventual'`. They do not weaken realtime query behavior.

Shared-preload parameters:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `auto_preload` | `0` | Best-effort shared-preload priority. `0` disables proactive background preload only. Positive values mark the index for background preload when `psql_bm25s` is loaded through `shared_preload_libraries` and `psql_bm25s.shared_generation_cache_size` is positive. Larger values are attempted first; equal values have no ordering guarantee. |

`auto_preload` is independent of `consistency`: it controls proactive immutable
generation residency, not write/query freshness. In a shared-preload
deployment, share-capable frontend queries require a shared generation. They
wait for the background preloader for marked indexes, wait for an active shared
publisher when one exists, or publish an unmarked first-use generation into a
shared tier themselves. They only error when shared publication is not
possible. This avoids one private copy per backend in large connection pools.
Without shared-preload, ordinary lazy DSM/backend-local behavior still applies.
The shared-preload resident registry is sized automatically from the configured
arena, so there is no separate startup preload batch limit beyond arena
admission.

Global maintenance GUCs:

| GUC | Default | Meaning |
| --- | --- | --- |
| `psql_bm25s.maintenance_worker_limit` | `1` | Extension-level worker cap, clamped to PostgreSQL `max_worker_processes`. |
| `psql_bm25s.preload_timer_interval_ms` | `1000` | Shared-preload warmup interval. Each warmup cycle drains all currently due marked indexes in priority order; values below `1000` are clamped to one second. |
| `psql_bm25s.maintenance_timer_interval_ms` | `60000` | Rebuild/catch-up interval and no-preload touch cooldown; values below `1000` are clamped to one second. |
| `psql_bm25s.maintenance_rebuild_memory_budget` | `32768MB` | Automatic rebuild memory admission budget. Workers use the standard in-memory builder only when its estimate has substantial headroom and the active payload is small enough for the in-memory path, fall back to compact for smaller medium payloads, then to the spill builder, and skip with `reason=memory_budget` when no estimate fits. |

These GUCs are global `SIGHUP` settings. Configure them through PostgreSQL
configuration and reload or restart the server; they are not per-index
reloptions and are not meant to be adjusted per query session.

Example service configuration:

```conf
shared_preload_libraries = 'psql_bm25s'
psql_bm25s.shared_generation_cache_size = '64GB'
psql_bm25s.maintenance_worker_limit = 1
psql_bm25s.preload_timer_interval_ms = 1000
psql_bm25s.maintenance_timer_interval_ms = 60000
psql_bm25s.maintenance_rebuild_memory_budget = '32768MB'
```

Equivalent SQL configuration for a running cluster:

```sql
ALTER SYSTEM SET psql_bm25s.shared_generation_cache_size = '64GB';
ALTER SYSTEM SET psql_bm25s.maintenance_worker_limit = '1';
ALTER SYSTEM SET psql_bm25s.preload_timer_interval_ms = '1000';
ALTER SYSTEM SET psql_bm25s.maintenance_timer_interval_ms = '60000';
ALTER SYSTEM SET psql_bm25s.maintenance_rebuild_memory_budget = '32768MB';
SELECT pg_reload_conf();
```

`shared_generation_cache_size` requires `shared_preload_libraries` and a
server restart because the shared arena is allocated during postmaster startup.
The other GUCs are reloadable, but a restart is often simpler when changing the
shared-preload deployment shape.

Indexes opt into proactive warmup with a reloption:

```sql
ALTER INDEX docs_bm25_idx SET (auto_preload = 1);
```

`auto_preload` changes residency priority only. It does not make an index
fresher or change scoring. Rebuild eligibility still comes from stale/pending
maintenance state plus the automatic builder admission rules below.

## Automatic Builder Selection

`maintenance_rebuild_memory_budget` deliberately prefers query stability over
automatic convergence. Automatic maintenance evaluates builders in this order:

| Builder | Automatic admission condition | Operational intent |
| --- | --- | --- |
| `standard` | `standard_estimated_bytes <= budget_bytes * 0.60` and active payload is below the standard-builder payload cap. | Fast path for genuinely small indexes with large memory headroom. |
| `compact` | Standard was rejected, `compact_estimated_bytes <= budget_bytes * 0.75`, and active payload is below the compact-builder payload cap. | Lower-memory in-memory path for smaller medium indexes. |
| `spill` | Compact was rejected and `spill_estimated_bytes <= budget_bytes`. | Default safe path for large indexes; spills term entries to PostgreSQL temp files and streams publish. |
| skip | Spill estimate also exceeds `budget_bytes`. | Keep the readable resident generation and report `reason=memory_budget`. |

The standard and compact payload caps are hard-coded safety rails in addition
to the budget headroom. They prevent large payloads from entering builders with
known live-memory overlap or PostgreSQL single-allocation risk. The current
code uses a standard-builder cap of `1GB` active payload and a compact-builder
cap of `512MB` active payload. Larger payloads go directly to spill when the
spill estimate fits.

The estimates are coarse by design:

| Estimate field | Formula | What it approximates |
| --- | --- | --- |
| `standard_estimated_bytes` | `active_payload_bytes * 6` | Raw docs, term stats, COO arrays, CSC arrays, serialized bytes, and allocator overhead. |
| `compact_estimated_bytes` | `active_payload_bytes * 4` | Compact term entries plus final arrays and publish state. |
| `spill_estimated_bytes` | `active_payload_bytes * 2` | Final arrays and streaming publish state after term entries move to PostgreSQL temp files. |

The extra `40%` standard-builder and `25%` compact-builder budget headroom is
intentional. Rebuild RSS includes allocator fragmentation, PostgreSQL executor
state, shared-buffer effects, and resident shared-preload pressure that are
not perfectly captured by the payload multipliers. If a host shows rebuild RSS
too close to the configured budget, lower the budget or let the headroom push
more work to spill.

Automatic maintenance and explicit operator rebuilds differ in one important
way. Automatic workers skip when no builder fits the budget. Explicit
`CREATE INDEX` / `REINDEX` still builds because the operator asked for a
controlled rebuild now; if no builder is admitted, it emits a `NOTICE` and uses
the spill builder.

The skip result includes `standard_estimated_bytes`,
`compact_estimated_bytes`, `spill_estimated_bytes`, and `budget_bytes`; use
those fields to decide whether the configured budget is intentionally
protective or too low for the deployment. The spill builder is the automatic
low-memory path: it spills term entries to PostgreSQL temp files and streams
publish for both online maintenance and explicit rebuilds, leaving final CSC
arrays and the shared-cache preload target as the main memory consumers.

Inspect current state and the selected automatic builder with:

```sql
SELECT psql_bm25s_generation_cache_state('docs_bm25_idx'::regclass);
```

For a compact fleet view:

```sql
SELECT
    n.nspname || '.' || c.relname AS index_name,
    psql_bm25s_generation_cache_state(c.oid) AS state
FROM pg_class c
JOIN pg_namespace n ON n.oid = c.relnamespace
JOIN pg_am am ON am.oid = c.relam
WHERE am.amname = 'psql_bm25s'
ORDER BY n.nspname, c.relname;
```

The state string includes `rebuild_builder`, `standard_estimated_bytes`,
`compact_estimated_bytes`, `spill_estimated_bytes`, `rebuild_budget_bytes`,
`payload_health`, `shared_preload_resident`, `active_background_workers`,
`active_preload_workers`, and `active_index_maintenance_workers`.
`active_maintenance_workers` is retained as a legacy alias for the shared
background worker-slot count; it does not mean every active worker is currently
rebuilding an index.
Maintenance API results include the same builder name when a rebuild actually
runs.

Online publish distinguishes true stale data from heap-TID freshness debt.
Concurrent `indexUnchanged` updates can advance pending counters without
materialized delta text because the indexed terms did not change. After a full
heap snapshot rebuild, that debt remains non-stale and is folded into a later
base generation by the normal threshold instead of forcing immediate repeated
full-table rebuilds under steady non-indexed-column update traffic.
Eventual indexes treat oversized delta payloads the same way: if a changed text
value is too large for the compact delta page format, the write contributes
pending freshness debt and is folded into the next threshold-driven rebuild
instead of making the resident generation stale immediately. Eager/realtime
policies still mark such cases stale because they promise tighter refresh
semantics.

## Maintenance APIs

- `psql_bm25s_index_details(regclass)` reports index metadata, consistency
  mode, pending writes, pending deletes, delta records, delta bytes, rebuild
  count, stale state, and active maintenance policy parameters as structured
  columns.
- `psql_bm25s_index_policy_recommend(regclass, profile text DEFAULT
  'balanced')` returns a structured recommendation for a workload profile. It
  is advisory only and does not mutate the index.
- `psql_bm25s_index_maintain(regclass)` rebuilds when pending or stale state
  exists, and otherwise returns a no-op result.
- `psql_bm25s_index_try_maintain(regclass)` is the non-blocking scheduler
  primitive. It returns retryable no-op results when locks are busy or the
  staged payload is invalidated by non-tail-compatible concurrent changes.
- `psql_bm25s_index_maintain_due(max_indexes integer DEFAULT 1)` scans due
  `eventual` indexes owned by the current role in the current database and
  calls the same non-blocking maintenance primitive. It uses the same stale and
  debt priority as the built-in catch-up worker.
- `psql_bm25s_index_refresh(regclass)` forces a refresh of the named index.
- `psql_bm25s_generation_cache_clear()` clears backend-local cache state and
  best-effort volatile shared-generation descriptors.
- `psql_bm25s_generation_cache_state(regclass)` reports the observable
  generation key, shared-cache eligibility, descriptor validity, and mapped
  shared memory size for one index.
- `psql_bm25s_generation_cache_preload(regclass)` warms the best available
  generation-cache tier for one index before application traffic arrives.

For query-first eventual indexes, `psql_bm25s_index_try_maintain(...)` uses
staged generation maintenance: it builds the replacement payload from a normal
MVCC heap snapshot, appends the new generation to the index relation, and then
publishes with a small metapage switch. A crash before publish keeps the old
generation active; a crash after publish sees a complete new generation. If
uncommitted writer deltas already exist, maintenance skips and retries later so
the build snapshot cannot accidentally clear invisible delta records. If
concurrent writes arrive during the build, online maintenance carries the
append-only delta tail forward when it can map the records safely. The new base
then remains non-stale and queries use the normal bounded overlay. If tail
mapping is not safe, the published base is marked stale and a later full pass
repairs it.

## Shared Generation Cache

Large immutable index payloads may be decoded into a server-wide DSM-backed
generation cache. The durable source of truth remains the PostgreSQL index
relation; the shared generation is a volatile acceleration structure.

For the full deployment model, connection-pool guidance, DSM V2 behavior, and
optional `shared_preload_libraries` arena, see
[Shared Generation Cache](shared-generation-cache.md).

The generation key includes:

- database OID
- index OID
- relfilenode locator
- index metapage version and flags
- `cache_epoch`
- payload sizes, document counts, and pending-delta counters

Backends attach to a shared generation only when the current metapage still
matches the descriptor. Small indexes without the optional shared-preload
arena can use the selected backend-local cache path. For generations that are
intended to be shared, the extension waits for the active publisher, retries
attach, publishes into shared memory, or reports a shared-cache error. It does
not silently load many private backend-local copies for a share-required
generation.

`REINDEX`, `psql_bm25s_index_refresh(...)`, blocking maintenance, and staged
eventual maintenance all publish a new observable generation key. Existing
readers keep using the generation they already attached to; new readers attach
the latest valid generation or rebuild a local view from the index relation.
When the optional shared-preload arena is enabled, old resident generations are
retired after a rebuild and the background worker warms marked
`auto_preload` indexes again on a later preload cycle. Standby servers use the
same preload path for replicated generations, but skip maintenance while in
recovery.

Operational notes:

- The shared cache is enabled by default for sufficiently large payloads.
- Small and medium indexes skip descriptor lookup to avoid adding overhead to
  the hot path unless the optional shared-preload arena is active.
- `psql_bm25s_generation_cache_state(...)` is for diagnostics and tests; it is
  not needed in normal query paths.
- `psql_bm25s_generation_cache_preload(...)` is useful in deployment hooks,
  rolling restarts, or scheduled warmup jobs for large connection pools.
- `psql_bm25s_generation_cache_clear()` is an operational cleanup tool. It can
  remove descriptors, but the next reader can always reconstruct from the
  index relation.
- The first backend still pays the cost to read and publish the generation.
  Later backends can share immutable memory, but first-query latency can still
  be dominated by DSM attach/mapping cost on some platforms.
- Large connection-pool deployments should treat the current DSM path as the
  zero-configuration sharing tier. The optional shared-preload arena is the
  target path for the lowest fresh-backend first-query cost.

## Scheduling With pg_cron

`pg_cron` is optional. The extension can already wake a dynamic worker for
`eventual` indexes when committed write debt, VACUUM cleanup, or later query
traffic is observed. Use pg_cron only when you want a time-based maintenance
wakeup.

Minimal setup:

1. Add `pg_cron` to `shared_preload_libraries` and restart PostgreSQL.
2. Create `pg_cron` in the scheduling database.
3. Schedule one lightweight SQL maintenance command per target database.

For `eventual` indexes, schedule the due-index helper:

```sql
SELECT psql_bm25s_index_maintain_due(2);
```

For `manual` indexes, schedule explicit index maintenance:

```sql
SELECT psql_bm25s_index_try_maintain('docs_body_bm25_idx'::regclass);
```

Use `psql_bm25s_index_maintain(...)` instead of
`psql_bm25s_index_try_maintain(...)` only when the scheduled job is allowed to
wait for the required locks. Manual indexes are not discovered by
`psql_bm25s_index_maintain_due(...)`; list them explicitly in the scheduled
SQL.

## Operational Guidance

- Use `realtime` unless there is a clear reason to trade freshness for
  foreground latency.
- Use `eventual` for large knowledge-base tables where short-term stale search
  results are acceptable and automatic convergence is desired.
- Use `manual` for static benchmark paths, one-shot imports, or systems where
  index refresh is controlled by an external job.
- Monitor maintenance with
  `psql_bm25s_index_details('index_name'::regclass)`.
- Prefer `psql_bm25s_index_try_maintain(...)` in unattended jobs because it is
  retryable and avoids waiting on busy locks.

Maintenance-path validation guidance is collected in
[Testing and Validation](testing-and-validation.md#benchmark-validation).
For the larger storage-format direction behind a more PostgreSQL-concurrent
maintenance design, see
[Online Maintenance Future Plan](online-maintenance-future-plan.md).
