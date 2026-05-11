# Reliable Maintenance, Preload, and Generation Publish Plan

## Problem Statement

Large eventual-consistency indexes can be updated by background maintenance
while production queries are active. The previous online maintenance path
rebuilt a complete replacement payload, truncated the index relation, wrote a
new metapage, and then wrote data pages. That shape has two unacceptable
failure modes for large `auto_preload` indexes:

- if the process crashes, is cancelled, or is killed after the new metapage is
  visible but before every payload page is durable, the catalog still points to
  an index whose metapage claims more bytes than the relation physically holds;
- first query or preload after such a failure may attempt a large cold load,
  pin memory, block on maintenance locks, or fail late with a truncated payload
  error.

The target design is an immutable-generation model:

- queries use a resident generation whenever possible;
- maintenance builds a replacement generation out of line;
- publishing is a small metapage switch to a fully written generation;
- old generations remain usable until no backend/shared-cache reader can
  observe them;
- automatic warmup and automatic maintenance are independent schedulers.

## Phase 1: Immediate Guard Rails

Phase 1 keeps the current builder logic but adds hard safety checks.

1. Split preload and maintenance timers.
   - `psql_bm25s.preload_timer_interval_ms` controls warmup cadence.
   - `psql_bm25s.maintenance_timer_interval_ms` controls rebuild/catch-up.
   - The supervisor wakes at the lower of both intervals, but maintenance only
     runs when its own interval is due.

2. Drain all due `auto_preload` indexes per preload cycle.
   - Startup warmup should not be limited to one index per minute.
   - There is no separate preload batch-size setting; the worker walks all
     currently due marked indexes in priority order and skips busy candidates.
   - Equal-priority candidates are ordered by descending relation size, then OID,
     so large hot indexes do not wait behind small indexes after restart.
   - The resident registry is automatically sized from the shared-preload
     arena, so the old fixed 128-entry ceiling no longer limits startup
     warmup.
   - Preload remains best-effort and uses the same worker limit as maintenance.

3. Add cheap payload health checks before payload allocation.
   - Validate `tid_bytes_len == num_docs * sizeof(ItemPointerData)`.
   - Validate claimed payload bytes fit within the active payload page capacity.
   - Mark impossible states as `corrupt/rebuild_required` in diagnostics.
   - Fail fast before allocating multi-GB buffers for an impossible payload.

4. Shorten shared-preload leases.
   - A backend may hold a shared-preload generation while a scan, SRF call, or
     transaction is using it.
   - Scan/SRF completion releases shared-preload refs promptly; the
     transaction callback is the final cleanup guard at commit/abort.
   - Idle connection-pool backends must not pin obsolete generations forever.

5. Add a rebuild memory admission guard.
   - Estimate peak rebuild memory from the current payload size.
   - If the estimate exceeds
     `psql_bm25s.maintenance_rebuild_memory_budget`, skip maintenance with a
     clear `memory_budget` reason.
   - This prevents PubMed/arXiv-sized automatic rebuilds from pushing a 128 GB
     host into swap.
   - Automatic maintenance then chooses the standard, compact, or spill
     builder by estimate. Standard requires `60%` budget headroom, compact
     requires `75%` budget headroom, and only spill may use the full configured
     envelope. If even the spill estimate exceeds the budget, it keeps the
     readable resident generation stable and reports the skip reason clearly
     instead of entering swap pressure.

## Phase 2: Reliable Generation Publish

Phase 2 changes online maintenance publish semantics while keeping the builder
algorithm intact.

1. Write the new generation to append-only generation pages.
   - The active metapage still points at the old generation during the write.
   - Crash before publish leaves the old generation active.

2. Publish by atomically updating the metapage.
   - The metapage switch records active generation id, start block, page count,
     payload lengths, and a new delta start block.
   - Crash after publish leaves the new generation fully readable.

3. Scope delta pages to the active generation.
   - Delta records appended before the switch belong to the old generation.
   - New writes after publish append after the new generation and are counted
     from the new `delta_start_blkno`.
   - Query-time delta overlay is only enabled for append-only generations.
     Older storage layouts are no longer query-compatible in this test line;
     they are reported as `unsupported_storage_layout` and must be rebuilt from
     the heap.
   - If concurrent writes occurred during build and the previous active
     generation is append-only-compatible, carry concrete delta records forward
     to the new generation and keep the new base non-stale with a normal query
     overlay.
   - If the concurrent tail also contains unmaterialized `indexUnchanged`
     updates or oversized eventual-mode text payloads, carry them as non-stale
     freshness debt. They do not require an immediate full-table rebuild; the
     normal rebuild threshold folds them into a later base generation.
   - If the tail cannot be mapped safely because the previous generation is
     corrupt or incompatible, publish the rebuilt base as stale and let a later
     full maintenance pass repair it.

4. Query behavior after a bad publish attempt.
   - If the current metapage points to a corrupt generation and no resident
     generation can be attached, query fails fast with a deterministic
     corruption error.
   - If the metapage does not identify an append-only active generation, query
     fails fast and background/manual maintenance rebuilds the index into the
     current layout.
   - It must not segfault, block indefinitely, or cold-load an impossible
     payload.

5. Garbage collection.
   - Shared-preload obsolete entries are released after lease refs drop.
   - On-disk obsolete generation pages are not reclaimed on the query path.
   - Disk compaction is a separate maintenance operation because an append-only
     relation cannot reclaim old pages at the front without rewriting the
     active generation.

## Phase 3: Lower-Memory Builders

Phase 3 reduces rebuild peak memory, then adds a spill path.

1. Compact in-memory builder.
   - Implemented as the first automatic fallback after the standard in-memory
     builder is rejected by the memory budget.
   - During online maintenance heap scan, it maps text tokens to token ids and
     stores compact `(term_id, doc_id, tf)` entries plus document lengths
     directly, avoiding long-lived raw token documents and the full docs-to-ids
     conversion copy.
   - It builds CSC arrays directly from compact entries and is validated against
     the standard builder for exact layout and score parity.
   - It is lower-memory, not fully memory-bounded: compact entries, final CSC
     arrays, and serialized payload bytes still overlap near publish time.
     Therefore automatic maintenance admits compact only when the compact
     estimate fits within `75%` of the configured rebuild budget and the active
     payload is below the compact-builder safety cap.

2. Streaming serializer and staged writer.
   - Implemented for online maintenance publish and explicit
     `CREATE INDEX` / `REINDEX` when the compact or spill builder is selected.
   - Serializes the finished BM25 index through a write callback directly into
     staged append-only generation pages.
   - Avoids allocating a complete `index_bytes` copy before publish.

3. Spill builder.
   - Implemented as the automatic fallback after standard and compact estimates
     exceed the rebuild memory budget, or when the active payload is too large
     for those in-memory builders to stay below PostgreSQL's single-allocation
     cap.
   - Writes compact `(term_id, doc_id, tf)` entries to PostgreSQL temporary
     files during heap scan.
   - Reads the temp entries through a rewindable term-entry reader to compute
     document frequencies and fill final CSC arrays.
   - Uses streaming publish, including explicit rebuilds, so the full
     serialized payload is not copied in memory before the metapage switch.
   - This is the path intended for PubMed/arXiv-scale automatic maintenance
     under a 32 GB class rebuild budget. Standard and compact are intentionally
     conservative; PubMed/arXiv-sized indexes should normally show
     `rebuild_builder=spill` in `psql_bm25s_generation_cache_state(...)`,
     assuming the final CSC arrays and resident shared-cache target fit the
     host.

## Validation Plan

Local validation:

- build with PostgreSQL 18 headers;
- run extension schema smoke;
- run shared-preload auto-preload smoke;
- run shared-preload generation cache smoke;
- run shared-preload maintenance smoke;
- run compact maintenance builder smoke;
- run spill maintenance builder smoke;
- run query-first eventual delta-tail smoke;
- run query-first eventual background/cancel smoke;
- run replication lifecycle smoke if local primary/standby helpers are
  available.

Cluster validation:

- deploy the new extension binary to the AWS primary and standby;
- restart with `shared_preload_libraries = 'psql_bm25s'`;
- configure shared generation cache size, preload timer, maintenance timer, and
  rebuild memory budget explicitly;
- set `auto_preload = 1` on the `commons` BM25 indexes;
- verify startup drains all due warmup indexes without waiting one minute per
  index;
- verify stale/corrupt diagnostics through `psql_bm25s_index_details` and
  `psql_bm25s_generation_cache_state`;
- verify primary maintenance publish is WAL-replayed on standby;
- verify standby auto-preloads replayed generations and does not build new
  generations itself.
