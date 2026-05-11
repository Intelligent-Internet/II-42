# Online Maintenance Future Plan

> Historical note: the active implementation has moved to staged append-only
> generation publish. See
> [Reliable Maintenance, Preload, and Generation Publish Plan](reliable-maintenance-preload-generations.md)
> for the current design. This document remains as background for future
> on-disk compaction and more advanced tail catch-up work.

This document records the earlier next-step design after query-first eventual
maintenance and tail handling.

The current implementation is intentionally conservative:

- build a replacement payload from a normal MVCC heap snapshot;
- let writers continue while the replacement is being built;
- carry forward append-only delta records that arrive during staging;
- take a short non-blocking swap lock;
- if the index changed in a way that is not an append-only tail, discard the
  staged payload and retry later.

That is enough to avoid the worst production failure mode where sustained
writes make staged rebuilds repeatedly do all the expensive work and then throw
it away. It is not yet the same design class as PostgreSQL's concurrent index
build.

## Target

The long-term target is an online maintenance path that is closer to
`CREATE INDEX CONCURRENTLY` in operational shape:

- most work happens against a shadow generation;
- foreground reads keep using the active generation;
- foreground writes append small durable delta records;
- the final publish step is a short metadata flip;
- interruption leaves either the old generation or a fully published new
  generation readable;
- old generations are garbage-collected only after they are no longer visible
  to active readers.

## Proposed Storage Shape

Introduce generationed index storage:

- metapage field: `active_generation`
- metapage field: `building_generation`
- metapage field: `generation_state`
- data pages tagged with a generation id
- delta pages either tagged with a generation id or kept in a separate delta
  chain with a durable low-watermark

The active generation is the only generation used by normal queries. A staged
maintenance job writes a new generation without truncating the active one. When
the new generation is complete, the job applies or records the delta tail and
publishes the generation by updating the metapage.

## Delta Tail Model

The V1 carry-forward path copies append-only tail records after a staged
rebuild. A more complete design should track:

- `base_snapshot_lsn` or equivalent build watermark;
- `delta_start_record`
- `delta_end_record`
- durable tail ownership by generation;
- whether a tail record has been incorporated into a shadow generation;
- whether a tail record must remain visible as overlay after publish.

This makes high sustained write workloads converge more predictably. A
maintenance run can build generation `N + 1`, carry the first tail segment, and
leave only the tail segment that arrived after the final catch-up pass.

## Publish Protocol

A candidate publish protocol:

1. Create a shadow generation id.
2. Write replacement data pages for the shadow generation.
3. Record the build watermark.
4. Re-read the delta chain up to a bounded catch-up watermark.
5. Apply or carry forward those delta records.
6. Take a short publish lock.
7. Verify that metapage generation state is still compatible.
8. Flip `active_generation` to the shadow generation.
9. Mark the previous generation as retired.
10. Let a later cleanup pass reclaim retired pages.

The important difference from the current staged swap is that the active
generation is never truncated during the long build. The final publish is
metadata-only or close to metadata-only.

## Crash And Cancellation Semantics

The future design should make every interruption state boring:

- crash before publish: keep serving the old active generation and discard or
  resume the shadow generation;
- crash during publish: replay WAL and land on exactly one active generation;
- crash after publish before cleanup: serve the new active generation and clean
  retired pages later;
- canceled maintenance: leave the old active generation and pending debt.

This requires enough metadata to distinguish abandoned shadow pages from the
active generation. It also requires cleanup to be idempotent.

## Reader Safety

Readers need a clear lifetime rule before old generations can be removed. The
candidate options are:

- rely on PostgreSQL relation locks and transaction snapshots to ensure no
  reader can still reference retired pages;
- keep a generation refcount or pin in backend-local state;
- use epoch-based cleanup where retired generations are reclaimed only after a
  safe horizon.

The simplest correct implementation should be preferred even if cleanup is
initially conservative.

## Why This Is Bigger Than Delta-Tail Carry-Forward

Delta-tail carry-forward is a targeted improvement to the current staged swap:
it preserves append-only work that arrived during staging and avoids throwing
away a useful replacement payload.

Generationed online maintenance changes the physical index format and publish
protocol. It needs new metadata, cleanup rules, WAL/crash review, and reader
lifetime rules. That is why it belongs in a future storage-format iteration
rather than the current query-first maintenance patch.

## Development Gates

Before implementing this design, collect:

- a sustained-write benchmark where writes continue during every maintenance
  run;
- a query-latency benchmark that compares active-generation reads against the
  current staged path;
- crash tests for before-publish, during-publish, and after-publish states;
- WAL/restart tests proving abandoned shadow generations are harmless;
- compatibility tests proving exact policies do not silently adopt eventual
  semantics.

The expected outcome is not just fewer retries. The target is predictable
maintenance convergence under continuous write pressure while keeping foreground
queries and writes inside explicit latency budgets.
