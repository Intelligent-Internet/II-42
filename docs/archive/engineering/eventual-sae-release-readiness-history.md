# Eventual SAE Release Readiness History

Closed: 2026-08-01

Archived: 2026-08-03

This is a closed historical ledger. All work packages and findings in this
document reached their recorded terminal states. The current product contract
is defined by the
[Convergent Segmented Index](../../convergent-segmented-index.md).

## Archive Guide

Preserved evidence includes the [convergence closure ledger](#eventual-convergence-closure-ledger),
[concurrency review](#concurrency-review-disposition),
[failure-safety closure](#post-commit-failure-safety-closure),
[runtime/build closure](#runtime-and-build-architecture-closure), and
[qualification matrix](#qualification-matrix). Results, counters, old commands,
and terminal dispositions below retain their original historical context.

The old base-generation and unified-delta implementation is superseded. This
ledger does not prescribe today's query API, accelerator invalidation rules,
worker scheduling, or deployment procedure. Use
[Query semantics](../../query-semantics.md),
[Maintenance lifecycle](../../maintenance-lifecycle.md), and the
[Product roadmap](../../product-roadmap.md) for those contracts. References to
"current" or "remaining" work in the ledger are relative to the recorded
milestone. The [archive index](README.md) is its navigation entry point.

## Objective

Prepare the repository for the next public release without reopening retired
research architectures. The release candidate must preserve one access
method, one application query route, and one relation-owned lifecycle for both
BM25-only and model-backed indexes.

## Work Packages

1. **Repository hygiene**
   - archive root-level research reports and machine evidence under `docs/`;
   - update every tracked reference and script default;
   - reject new root-level experiment artifacts in the static product gate.
2. **Public contract**
   - keep `CREATE INDEX ... USING ii42`, `ii42_query(...)`,
     `ii42_index_status(...)`, common maintenance, and drop as the documented
     product surface;
   - keep owner-only diagnostics explicit and absent from ordinary examples;
   - update quickstart, API, operations, migration, and current-status documents.
3. **Release metadata**
   - ship the project and third-party licenses;
   - maintain changelog, security, contribution, version, checksum, and clean
     source provenance contracts.
4. **Lifecycle qualification**
   - test fresh install and atomic BM25-to-model `REINDEX` conversion;
   - test side-by-side source-table migration from a package-bound
     `psql_bm25s` installation in a separate extension schema;
   - test BM25 realtime/eventual/manual and SAE eventual CRUD convergence;
   - test rollback, savepoints, HOT update, VACUUM, REINDEX, concurrent DDL,
     restart, immediate crash, corruption, and drop;
   - test primary/standby physical replication through build, mutation,
     maintenance, and descendant-manifest publication.
5. **Package qualification**
   - build from a clean commit with pinned ONNX Runtime;
   - bind every maturity test to the staged package;
   - require identical preflight/postflight fingerprints;
   - run PostgreSQL 17/18 ZIP dry runs and the PostgreSQL 18 container smoke.
6. **Defensive source qualification**
   - build the extension with and without ONNX Runtime;
   - run normal and ASAN/UBSAN unit tests;
   - reject malformed serialized payloads, inconsistent readers, invalid
     scoring parameters, and over-complex raw queries;
   - review PostgreSQL access-method and semantic-runtime analyzer output,
     classifying intentional exception-cleanup dead stores separately from
     actionable memory, bounds, initialization, and nullability findings;
   - bind isolated source-tree smokes to the staged extension library and SQL
     files rather than accepting an already installed system extension.
7. **High-concurrency qualification**
   - require latch-driven runtime request/response handoff rather than
     millisecond busy polling;
   - keep VACUUM memory proportional to identity/tombstone storage rather than
     posting payload size;
   - measure same-index writer serialization, runtime response
     head-of-line blocking, page-native L0 traversal, and maintenance
     discovery cost before changing their shared-memory or lock protocols;
   - close `SAE-CAP-1` through `SAE-CAP-5` before claiming that model-backed
     indexes are qualified for high-concurrency, multi-model, or sustained
     high-churn production workloads.

## Beta Review Disposition

The 2026-07-29 whole-product review records the following priorities for the
first beta:

1. CI and release-workflow enforcement are deferred. The final clean-package
   gate remains required before publication, but it is not part of the current
   lifecycle design work.
2. Single-model runtime capacity is the immediate target. Qualification must
   prove that worker and backend RSS converge after warmup under sustained
   query, CRUD, maintenance, restart, and cancellation workloads. Multi-model
   operation needs a sound bounded-session design review, but does not need
   first-beta performance qualification.
3. SAE `eventual` consistency uses asynchronous semantic encoding during
   maintenance rather than document inference in the writer transaction. The
   accepted contract keeps one index relation, one durable mutation stream, one
   scheduler, and one query API. It must not restore semantic side tables or
   separately published lexical and semantic generations.
4. The first beta must make one configured model operationally bounded and
   stable. More model checkouts may multiply the explicitly bounded worker
   session cache, but are outside the urgent release envelope.
5. The only historical compatibility path is side-by-side source-table
   migration from `psql_bm25s` to the current II-42 catalog. Current II-42
   BM25-to-SAE conversion remains an atomic reloption change plus `REINDEX`.
   Intermediate experimental II-42 catalogs and minor-version payloads are not
   supported and must not leave compatibility code or contradictory
   documentation.

### Accepted Asynchronous Eventual Contract

For `sae=true`, which always selects `consistency=eventual`:

- foreground `INSERT` and changed `UPDATE` compile and append the exact lexical
  posting map plus a semantic-pending identity; they do not invoke the model;
- committed pending rows are immediately searchable through lexical postings;
  semantic contribution is absent until the matching completion is durable;
- a completion belongs to one exact mutation identity. A newer update, delete,
  `REINDEX`, or model-contract change makes an older completion inapplicable;
- query reads one checked manifest/document-COW authority plus snapshot-visible
  active and pending linked-L0 records as one logical posting surface;
- the existing maintenance scheduler performs one bounded action per
  transaction. Required active-L0 rotation and pending-L0 sealing may precede
  semantic completion; later compaction preserves unresolved semantic state;
- SAE realtime and manual definitions are rejected. Pure BM25 retains realtime,
  eventual, and manual policies without model inference;
- status exposes lexical freshness separately from semantic convergence,
  including exact pending state, completion throughput, retries, failures, and
  the last runtime error. Exact failed-row identity remains owner-only;
- foreground append is bounded by the physical active/pending linked-L0 hard
  frontier. There is no operator threshold or decoded-cache admission policy;
  exhaustion fails closed until bounded maintenance restores capacity.

The implementation must close these lifecycle requirements:

- what durable pending record is appended at commit, including tuple/version
  identity and enough information for later encoding;
- what queries return while semantic work is pending, and how old semantic
  postings are prevented from representing an updated or deleted row;
- how a semantic completion record is atomically associated with the exact
  pending mutation and ignored after a newer update, delete, `REINDEX`, or
  model-contract change;
- how rollback, savepoints, prepared transactions, immediate crash, physical
  replication, and failover preserve or resume pending work;
- what physical frontiers bound pending work, retries, and worker backlog, and
  when the system fails closed;
- which status fields expose lexical freshness, semantic freshness, completion
  throughput, failures, and lag without introducing a second lifecycle.

Implementation order is fixed:

1. version linked-L0 and immutable semantic state and prove
   pending/complete/quarantine/retire parsing and supersession without a model
   runtime;
2. make query lexical-first and exact for the durable state it exposes;
3. add bounded background batch encoding and completion publication;
4. preserve one bounded scheduler across rotation, sealing, completion,
   compaction, VACUUM, and reclamation;
5. prove transaction, crash, replication, root replacement, and hard-frontier
   behavior before enabling the mode in product examples.

### Asynchronous Eventual Implementation Evidence

The current tree implements the basic single-relation contract shape without
adding another relation, scheduler, cache policy, or public API:

- linked-L0 record version 4 distinguishes lexical upsert, retire, semantic
  completion, and quarantine transitions. Immutable segment payload version 5
  and document-COW version 5 preserve the same semantic identity;
- an upsert stores the exact heap TID, transaction identity, document slot,
  semantic-input fingerprint, and lexical posting map.
  Completion follows a HOT chain and accepts a visible descendant only when
  the semantic-input fingerprint still matches;
- a completion or quarantine transition belongs to the exact fingerprint and
  sequence. Heap MVCC, mutation identity, and runtime contracts reject stale HOT
  descendants, non-HOT supersession, CTID reuse, and deletes;
- a quarantined state durably stores its original pending time, latest retry
  time, failure count, SQLSTATE, and error hash. Update, delete, successful
  completion, or `REINDEX` supersedes it through the same COW/L0 chronology;
- the owner-only diagnostic walks the checked document COW and applies committed
  linked-L0 transitions under one transaction-status classification window.
  An unresolved transaction reports `exact=false` without exposing its identity;
- `ii42_index_status(...)` exposes aggregate completion state and bounded
  telemetry under `generation.delta.semantic_completion`. It does not recreate
  a decoded corpus-sized delta or a second lifecycle.

The current staged PostgreSQL 18 evidence passes native v3 lifecycle `80/80`,
real-model mutable lifecycle `71/71`, and transaction/2PC/crash/VACUUM lifecycle
`21/21`. The dedicated quarantine gate passes head/middle/tail failures,
runtime-wide rollback, bounded retry, owner ACL, exact JSON identity,
update/delete/REINDEX supersession, unresolved-XID suppression, restart, and
normal/oracle parity. Physical replication, standby exact-root preload,
runtime-service restart, storage-boundary recovery, and isolated extension
regression also pass against the staged package.

The focused adversarial gates now cover HOT update before completion, poison
rows, concurrent threshold crossing, sustained ingress, cross-index fairness,
temporary relations, and aggregate transaction memory. Final publication still
requires the clean-package execution matrix below.

## Eventual Convergence Closure Ledger

This ledger preserves the original first-beta gaps and their acceptance
criteria. All items were closed without adding a semantic side table, second
publication generation, or separate model-index lifecycle.

| ID | Priority | Confirmed gap | Required closure | Acceptance evidence |
| --- | --- | --- | --- | --- |
| EVT-1 | P0 | Semantic completion calls `table_tuple_fetch_row_version()` for the pending root TID and requires the visible tuple's raw xmin to equal the original mutation. That API checks only the exact TID and does not follow a heap HOT chain. A committed HOT update before completion can therefore make the still-live row look retired; later compaction may remove its only posting. | Version the pending payload with a fingerprint of the exact semantic input. Resolve the pending TID through index-entry/HOT-chain semantics, accept a visible HOT descendant only when its semantic-input fingerprint still matches, and retain CTID-reuse protection. Superseding non-HOT updates and deletes must continue to retire the old pending identity. | Insert and update lexical-pending rows, perform multiple HOT updates before completion, prune/redirect with `VACUUM`, exercise CTID reuse, then complete and compact. Normal search, full-overlay oracle, restart, crash, and physical-replication results must remain exact with no missing live row or stale semantic contribution. |
| EVT-2 | P1 | Due discovery, status, and every 256-row completion batch load and parse the complete physical delta from offset zero, rebuild the full tombstone set, and rescan all records. Commit classification also reacquires `XactTruncationLock` per examined record. Completion appends a complete record and tombstone for every row, increasing the next scan. Draining a large pending queue is therefore approximately quadratic in accumulated delta size. | Add a relation-owned, crash-validatable work frontier keyed by generation and delta identity. It must track a scan block/offset, conservative pending counters, oldest age, and a bounded set of unresolved earlier transactions. Ordinary due checks and status must use this metadata; completion resumes at the frontier and classifies a batch under one safe transaction-status window. A full scan is only a reconstruction fallback after restart, identity mismatch, or detected corruption. Compaction normalizes the frontier in the same relation. | Drain 1K, 10K, 50K, and 200K pending rows. Bytes parsed and transaction-status lock acquisitions per steady-state batch must remain bounded by the batch plus changed tail, total drain time must scale near-linearly, and restart at arbitrary batches must reconstruct and resume exactly. Query/oracle, crash, 2PC, and replication parity remain mandatory. |
| EVT-3 | P1 | One completion batch is encoded as one all-or-nothing runtime request. A row-local permanent error aborts the maintenance transaction, leaves the same row at the queue head, and retries without durable isolation or backoff. One poison row can block all later semantic work and compaction. | Classify runtime-wide transient failures separately from row-local failures. Bisect a failed batch until a singleton is isolated, record bounded retry/backoff or quarantine state in the same unified delta, keep the row lexical-visible and explicitly non-converged, and continue later rows. Update/delete/reindex must supersede the failed identity. Quarantine is operational state, not successful completion. Public status exposes counts and lag; exact failed-row identity remains owner-only. | Inject transient whole-runtime failure and permanent errors at the head, middle, and tail. Later rows must complete, retries must be bounded rather than a hot loop, public status must expose blocked count and next retry, owner-only diagnostics must identify the mutation, and fixing/updating/deleting the row must restore convergence across restart and replication. |
| EVT-4 | P1 | Foreground backlog capacity is checked before a backend-local batch is filled. The check and later append are not one serialized admission action, so many writers can all pass against the same metapage and later append up to 256 rows or 8 MiB each. The documented global one-batch overrun is not established. | Serialize the complete batch first, then take the per-index append mutex, reread current physical debt, atomically admit or reject against actual record and byte counts, and append under the same fence. If one batch may cross the threshold, only the first globally serialized crossing batch may do so; all following batches fail until headroom returns. | Run 32- and 64-writer barriers immediately below both limits. Aggregate physical debt may exceed a limit only by the explicitly documented single global batch bound. Aborted/savepoint/prepared batches remain invisible and reclaimable, and no accepted batch is lost after crash or replication. |
| EVT-5 | P1 | If any semantic pending row exists, one maintenance call returns immediately after completion and never reaches compaction. Under sustained ingress, every cycle can see another pending row, so complete records and tombstones grow even when completion throughput exceeds writes; foreground backpressure can eventually fire despite healthy encoding. | Add an explicit per-index phase policy. A completion transaction must still commit before its records are compacted, but a later cycle must compact an eligible completed prefix after a bounded number of completion batches or when physical debt crosses a high watermark, while carrying unresolved pending tail records forward. Persist or reconstruct enough phase state to survive worker restart. | Under continuous writes below measured completion throughput, run long enough to cross many batch boundaries. Semantic lag and physical delta debt must remain bounded, both completion and compaction must make periodic progress, foreground writes must not hit false backpressure, and query/oracle results must remain exact. |
| EVT-6 | P1 | Candidate ordering always prefers semantic work and then the largest record/byte debt. With the default single maintenance worker, one continuously busy large index can win every cycle; smaller indexes have no last-served or age-based fairness guarantee. | Preserve corrupt/rebuild-required urgency, then add oldest-work age and bounded fair service within each priority class. Use a database-local round-robin or deficit cursor and cap consecutive successful batches for one index. Fairness state may be shared and reconstructible; correctness must not depend on retaining it across restart. | Drive one hot large index and several sparse indexes with one and multiple workers. Every index must have a bounded completion and compaction lag, the hot index must retain throughput, no work hint may be lost, and restart/reconciliation must resume fair progress. |
| EVT-7 | P1 | There is no explicit product contract for temporary-table indexes. A background worker cannot safely own another backend's temporary relation, but build and reloption validation do not reject an automatic eventual lifecycle. | Define and enforce the beta boundary at build and reloption validation. Recommended closure is to reject `sae=true, consistency=eventual` on temporary relations with an actionable error; any supported temporary route must use same-backend explicit maintenance and must never advertise background convergence. Keep UNLOGGED behavior unchanged. | Add temporary BM25 and SAE policy tests for create, DML, query, maintenance, drop, and backend exit. Unsupported combinations must fail at definition time rather than later in a worker. Supported combinations must be exact and leave no shared registry or work-hint residue. |
| EVT-8 | P2 | Transaction-local SAE mutation memory is bounded per index at 256 rows or 8 MiB, but the transaction can retain one batch for every touched SAE index. A transaction spanning many indexes therefore has no aggregate retained-memory bound. | Add one transaction-wide retained-byte budget. Flush the largest or oldest active batch before accepting a mutation that would cross it, while preserving subtransaction cleanup and the per-index limits. Expose only a configuration/status value if an operator can act on it. | In one transaction, write through enough SAE indexes to exceed the aggregate budget, including savepoint aborts and an injected late failure. Peak backend RSS and retained accounting must stay bounded, read-your-writes must remain exact, and commit/abort/prepare cleanup must leave no batch state. |

### Implemented Dependency Order

1. EVT-1 closed before changing queue layout. This was a live-row correctness
   blocker and requires a versioned pending identity.
2. EVT-2 was built on that versioned record format. The work frontier is the common
   foundation for efficient retries, status, scheduling, and phase decisions.
3. EVT-3 closed using the frontier, without a post-hoc skip list or a second
   relation.
4. EVT-4 closed at the physical append boundary. An early foreground check may
   remain advisory, but only append-time admission is authoritative.
5. EVT-5 and EVT-6 closed together so per-index phase fairness and cross-index
   service fairness use one scheduling model.
6. EVT-7 and EVT-8 closed as explicit support and capacity boundaries.
7. Rerun the complete clean-package matrix and add sustained high-churn,
   multi-index, and concurrent-admission evidence to the release artifact.

### Historical 2026-07-29 Closure State

The 2026-07-29 implementation was closed in dependency order. This table is
retained as historical evidence and is superseded by the current beta contract
above. A code
change is not treated as closed until its focused gate and the complete mutable
lifecycle both pass.

| ID | State | Current evidence |
| --- | --- | --- |
| EVT-1 | Closed | Unified pending payload v4 fingerprints the semantic input, completion follows the HOT chain through table index-fetch semantics, and accepts a descendant only when the fingerprint is unchanged. The HOT/VACUUM/CTID-reuse gate and the complete mutable lifecycle pass. |
| EVT-2 | Closed | One relation-owned frontier marker resumes pending discovery without the removed full-overlay pending-cache path. Status exposes exactness, records, bytes, transaction-status lock windows, and reconstruction events. The dedicated benchmark passes 1K/10K locally and 50K/200K remotely. The 50K row drains at 168.668 rows/s in 195 semantic batches with a 579-record steady scan maximum and 134,333 total scans against a 304,096 linear bound. The 200K row drains at 130.705 rows/s in 782 batches with a 578-record steady maximum and 520,256 total scans against a 1,204,096 bound. Both restart, resume without stale state, and finish with exact pending/oracle state and zero delta debt. |
| EVT-3 | Closed | Linked-L0 v4, immutable segment payload v5, and document-COW v5 record durable row-local quarantine state, bounded retry, SQLSTATE/error hash, pending time, and semantic-input identity in the same v3 lifecycle. Failed batches are bisected under subtransactions; runtime-wide failures roll back, while head/middle/tail poison rows remain lexical-visible and later rows complete. The current dedicated gate additionally proves exact COW plus linked-L0 owner diagnostics, unresolved-XID identity suppression, update/delete/REINDEX recovery, normal/oracle parity, and restart persistence. |
| EVT-4 | Closed | Serialized append-time admission is authoritative under the per-index append fence; concurrent threshold and atomic-rejection gates pass. |
| EVT-5 | Closed | The semantic phase policy forces periodic exact compaction after bounded completion debt, carries unresolved pending tail records into the replacement generation, and re-arms the work hint when concurrent ingress remains. The sustained-ingress gate observes compaction while writers remain active, bounded drain, zero hint overflow, and exact normal/oracle results with one and four workers. The complete mutable lifecycle passes 80/80. |
| EVT-6 | Closed | Maintenance candidates use one database-local last-served cursor within their priority class, work hints are cleared only when their observed sequence remains unchanged, and the shared supervisor limit is hot-reloaded through one atomic shared control. The focused gate gives all four indexes first service within 3.668 seconds with one worker and observes four concurrent index-maintenance workers at a configured limit of four. After postmaster restart, catalog reconciliation drains four eventual SAE indexes, one realtime SAE index, and one realtime BM25 index without a new query or write wakeup. Normal/oracle parity and the complete mutable lifecycle pass. |
| EVT-7 | Closed | Temporary relations support manual BM25 only. Model-backed and automatic-consistency temporary indexes fail at definition time; the complete temporary lifecycle gate passes. |
| EVT-8 | Closed | The dedicated aggregate-memory gate writes one approximately 768-KiB mutation through 24 SAE indexes under a 16-MiB transaction budget. It observes 72 precommit-flushed delta records, exact read-your-writes normal/oracle parity, 11.5 MiB peak RSS growth against a 96-MiB ceiling, and zero retained mutation contexts after savepoint rollback, commit, injected late precommit failure, `PREPARE TRANSACTION`, and `COMMIT PREPARED`. |

The current source passes the dedicated EVT-3 quarantine and EVT-5/EVT-6
sustained-ingress/fairness gates after the linked-L0/COW cutover and removal of
the obsolete decoded-cache authority. Native v3 lifecycle passes `80/80`, the
real-model mutable lifecycle passes `71/71`, transaction/2PC/crash/VACUUM
passes `21/21`, and the package-bound suite passes physical replication. The
final authorized clean-commit package rerun remains an external publication
gate.

### Promotion And Stop Rules

All P0/P1 eventual gates are closed for the declared first-beta support
boundary. A quarantined row remains lexical-visible and explicitly
non-converged; it is never silently treated as a semantic completion. CI
workflow automation and multi-model throughput remain deferred, but they do
not waive the clean-package, correctness, bounded-memory, or forward-progress
publication gates.

## Concurrency Review Disposition

The line-level lifecycle review separates safe local fixes from changes that
alter PostgreSQL lock or shared-memory contracts:

| Area | Current finding | Disposition and acceptance |
| --- | --- | --- |
| Runtime request handoff | Admitted requests, responses, cancellation, and worker restart are latch-driven. Queue-full callers use interruptible 10 ms `WaitLatch` backpressure before they own a slot, not CPU polling. | Implemented. Runtime restart, privilege, required-service, provider, cancellation, concurrent-query, and 1,000-request retained-memory smokes pass. |
| VACUUM delete discovery | PostgreSQL supplies a deadness callback, so exact deletion visits the fixed-depth document COW directory plus bounded linked L0 and publishes retirement authority in bounded COW batches. It does not decode posting payloads or allocate a corpus-sized bitmap. | Implemented. CRUD, repeat-VACUUM idempotence, injected partial-batch failure, restart, and physical-replication gates pass. Discovery is `O(document slots + bounded L0)` with fixed batch memory; ordinary background convergence remains changed-set bounded. |
| Same-index writers | Automatic semantic writers use a transaction-long shared writer barrier and a short append mutex; they never hold the exclusive generation barrier for the complete transaction. BM25-only realtime transactions retain the exclusive writer barrier because exact corpus statistics require one committed writer boundary, but commit callbacks append only delta records and never replace a generation. Post-commit maintenance performs replacement. Snapshot readers hold the generation barrier in shared mode and do not conflict with append-only writers. | Implemented. Eight semantic writers and eight readers completed 864/864 CRUD operations while 9,613 maintenance probes, 16,538 VACUUM calls, and 1,650 reader queries ran. The second semantic writer acquired its path in 0.806 ms, and final normal/oracle results were identical. Transactional BM25 gates prove append-only commit and prepared-transaction visibility. |
| Runtime responses | One explicit eight-request admission pool uses eight independent 4 MiB response slots and a bounded pool of two inference workers by default. Document work may occupy at most `N - 1` healthy workers and seven response slots. | Implemented. True concurrent execution, a physical query lane with reserved admission, cancellation, owner death, orphan cleanup, per-worker restart, bounded queue depth, and provider/resource smokes pass under runtime ABI version 15. Query/document role affinity prevents the default two-worker pool from repeatedly replacing hot sessions. |
| Shared control synchronization | Runtime queue/result copies and generation-registry scans previously ran while holding process-wide spinlocks. A status call could scan every cache slot, and a response publication could copy MiBs while all backends spun. | Implemented. Two named add-in LWLocks independently protect runtime-service and generation-cache controls. Read-only snapshots use shared mode; mutation, admission, publication, cancellation, and LRU state use exclusive mode. Runtime, cache-waiter, same-index concurrency, mutable lifecycle, and physical-replication gates pass. |
| Exact mutable-cache waits | Exact cache identities use 64 generation-keyed PostgreSQL condition-variable stripes with a one-second watchdog. | Implemented. Sixteen-way stampede, timeout, cancellation, and publisher-death gates pass. A modifying transaction or an older MVCC snapshot uses a bounded, reusable transaction-local exact view qualified by `SAE-CAP-4`. |
| Maintenance discovery | A 4,096-entry shared work-hint ring drives ordinary cycles; a 256-database reconciliation registry and five-minute catalog authority over every non-manual index close loss, overflow, and restart cases. | Implemented. No hint overflow occurred at 10,000 indexes; catalog scan measured 12.409 ms and the complete worker cycle 453.008 ms. Restart recovery drains eventual and realtime BM25/SAE debt without foreground traffic. |

These implementations preserve one relation-owned manifest lineage, fail-closed
semantic execution, standby behavior, and the existing public API. They close
the known correctness failures, but they do not by themselves qualify the
capacity envelope recorded below.

## Closed SAE Capacity And Stability Work

The 2026-07-28 review identified five capacity and availability risks beyond
the existing small- and medium-scale suites. The 2026-07-29 implementation and
focused qualification close these design risks without reopening the
single-relation architecture: durable corpus state remains one manifest-owned
segment/fold graph, linked L0, and retirement stream. Final release claims
still require a clean-commit staged package and the large-corpus matrix below.

| ID | Priority and finding | Proposed closure | Required acceptance |
| --- | --- | --- | --- |
| `SAE-CAP-1` | **P1: runtime serialization and head-of-line blocking.** One postmaster worker serialized query and document encoding through one FIFO. A request deadline or queue priority could only bound or reorder saturation; neither created inference capacity. | Replaced by a bounded postmaster-managed inference-worker pool. Each worker owns its ONNX sessions; shared requests carry per-worker ownership, health, cancellation, and recovery. Model affinity preserves hot sessions while bounded stealing prevents serialization. Document work is capped below healthy pool capacity. Worker and session-cache counts are restart-only memory bounds; deployments use measured checkout/provider RSS because ONNX Runtime has no portable expanded-session estimator. | The 1/2/4-worker matrix, mixed query/document work, cancellation, worker death/restart, checkout affinity, ranking parity, and RSS soak pass. Two workers are the accepted default. Four workers remain optional because `1.936x` missed the separate `2x` promotion floor. |
| `SAE-CAP-2` | **P1: full-delta cache recompilation under append fencing.** Every queried or preloaded exact delta identity can reload and compile the complete durable delta while holding the per-index append mutex. Continuous small commits plus queries can therefore turn one-row changes into O(delta) publication work, repeatedly stall writers, and churn the shared cache before compaction catches up. | Keep the one durable relation delta, but make its volatile compiled cache incremental: an immutable compiled prefix plus bounded append segments keyed by the exact record sequence. Compile only unseen records, publish segments single-flight, merge them in the existing score accumulator, and consolidate volatile segments at count/byte thresholds. Hold the append mutex only to capture and validate identity boundaries; never across full-prefix compilation. Compaction still folds the same durable delta into one replacement generation. | Compare delta sizes 1K/10K/100K while publishing one additional record. Incremental publication latency and append-lock hold time must remain approximately constant: the 100K case may not exceed the 1K case by more than `1.5x`. Normal ranking must exactly match the full-overlay oracle through concurrent append, abort, savepoint, VACUUM, restart, crash, compaction, and physical replication. No second durable relation, WAL stream, scheduler, or public lifecycle may appear. |
| `SAE-CAP-3` | **P1: global registry lock and linear lookup.** Shared generation resolution takes the process-wide preload `LWLock` in exclusive mode and scans 1,024 to 65,536 metadata slots. At large arena sizes or many indexes, every SAE query can contend on one O(registry capacity) critical section. | Add a shared hash from the complete generation key and kind to a stable slot. Shard lookup/refcount synchronization or use atomic refcounts where PostgreSQL memory-ordering rules permit; keep allocation, eviction, and LRU mutation outside the common lookup path. Preserve the slot array as the arena allocator if replacing it would add unnecessary risk. | Benchmark exact attach/release at 1K/10K/65K registered entries with 1/16/64 clients. The 65K single-client lookup p95 must be no more than `2x` the 1K result, and 64-client throughput must not serialize on one exclusive registry lock. Stampede, eviction, cancellation, stale-generation retirement, cache clear, restart, and lease-leak gates must remain exact. |
| `SAE-CAP-4` | **P1: unbounded statement-local snapshot cache.** Same-transaction read-your-writes and older MVCC snapshots rebuild the complete durable delta into backend-local memory. The view is freed after the statement, so repeated searches can repeat O(delta) work. The API reference also overstates that SAE never has a local unified cache, while other documents correctly describe the exception. | Document both exceptions consistently. Add explicit local-view record/byte limits and diagnostics. Reuse a transaction-local compiled view when the delta identity and visibility fingerprint are unchanged; invalidate it after local DML, command/snapshot changes that alter visibility, subtransaction end, prepare, abort, or transaction end. If safe reuse cannot be proven, retain statement scope but fail fast above the configured budget instead of risking backend memory amplification. | Repeated same-transaction and `REPEATABLE READ` searches over 1K/10K/100K delta records must remain oracle-exact. One hundred unchanged searches must perform one reusable build when permitted, while intervening DML must force a rebuild. Backend retained RSS must stay within the configured budget, and oversized local views must produce a deterministic actionable error without publishing private state. |
| `SAE-CAP-5` | **P1: cache-admission and compaction forward-progress gap.** An oversized exact cache triggers compaction, but automatic compaction can independently return `reason=memory_budget`. Queries then wait up to 30 seconds and fail closed on every attempt. This is safe for correctness but can become a persistent availability outage under arena pressure or sustained writes. | Add proactive high/low watermarks based on estimated compiled-cache bytes, not only durable delta bytes. Reserve configurable arena headroom for active unified deltas, start compaction before admission is impossible, and expose projected headroom in `ii42_index_status`. Provide a bounded spill-backed incremental compaction path so an exact base-plus-delta state always has a forward-progress route without corpus re-encoding. When the engine already knows admission is impossible, fail immediately with the blocking estimate instead of consuming the full query wait timeout. | In a forced small-arena and low-memory-budget test, sustained writes must remain queryable or converge through bounded spill compaction without `REINDEX`. The test must prove no repeated 30-second failures, no OOM/swap event, exact ranking before and after compaction, bounded temporary disk use, restart/crash safety, and recovery after pressure is removed. |

The final `SAE-CAP-5` qualification closes the two multi-index boundaries left
by the initial single-index run. Exact oversized identities now use a bounded
keyed registry, and proactive pressure accounts for aggregate active
unified-delta bytes plus the current index's missing tail. Safely evictable
base generations do not force semantic compaction.

### SAE-CAP-1 Implementation Evidence

The 2026-07-29 current-tree qualification replaces the serial worker with a
bounded, restart-only postmaster worker pool. This is real inference
parallelism, not a deadline or queue-priority workaround:

- each worker owns an independent ONNX Runtime session cache;
- shared requests carry per-worker ownership, cancellation, health, and
  recovery state;
- model-affinity dispatch reserves one singleton for a hot worker but permits
  immediate stealing when the same model has a backlog;
- with more than one healthy worker, at most `N - 1` workers may run document
  batches, and document admission may occupy at most seven of eight response
  slots, preserving both execution and admission for an online query;
- a failed worker interrupts only its owned request while the remaining pool
  stays available, and postmaster restarts the failed slot;
- 64-client admission, parallel batches, mixed query/document work,
  cancellation, in-flight worker death, two-checkout affinity, and a
  120-round RSS soak pass.

On the qualified 24-logical-CPU host, two workers increased 64-client
throughput from 282.73 to 438.21 QPS (`+55.0%`) and reduced request p95 from
207.74 ms to 129.20 ms. Four workers reached 547.29 QPS and 98.09 ms, or
`1.936x` single-worker throughput. The original `2x` four-worker promotion
gate therefore remains failed; four workers are supported as an explicitly
qualified deployment choice but are not the product default. Explicit two-
and four-thread ORT variants were slower than the automatic three-thread
four-worker configuration.

The accepted default is two workers. Its measured aggregate worker RSS for
the 382 MiB test checkout was 3,216,384 KiB. Worker count and per-worker
session-cache count are restart-only capacity bounds; no inaccurate automatic
ONNX expanded-RSS estimator is claimed. See
`docs/performance/reports/runtime-worker-pool-cap1-2026-07-29.md` and its raw
JSON evidence.

### SAE-CAP-2 Implementation Evidence

The compiled mutable-delta cache is now an immutable prefix plus bounded
append segments:

- each generation records the physical durable-delta scan cursor it covers;
- a publisher reads and compiles only records after the best exact compatible
  prefix;
- prefix and append segment identities are validated before publication;
- the shared chain is bounded to eight entries and then consolidated;
- append-lock timing covers only cursor capture and identity validation, not
  full-prefix compilation.

Five sampled one-record appends at 1K, 10K, and 100K prefixes all published a
one-record segment and matched the full-overlay oracle. Median publication was
70, 49, and 58 microseconds respectively; median append-lock hold time was one
microsecond at every level. The 100K/1K median publish ratio is `0.8286`, below
the `1.5x` gate. Update, delete, VACUUM/tombstone, bounded-chain consolidation,
and post-consolidation append tests also remain oracle exact.

The final current-tree chain gate additionally proves that every leased parent
segment contributes to aggregate pressure: chain depth and resident entry
count both advance from one through eight. The ninth append consolidates to
one root entry and retires the stale parent chain rather than retaining both
the old chain and new root. A staged-package same-index race then exposed and
closed one availability defect: an online compactor may read an older exact
meta boundary after a concurrent append extends the same physical page. The
loader now accepts that suffix only when a metapage reread proves the durable
identity advanced; an unaccounted suffix under a stable identity still fails
as corruption. Eight writers, eight readers, maintenance, and VACUUM pass with
normal/oracle parity.

### SAE-CAP-3 Implementation Evidence

Complete generation keys now resolve through a shared open-addressed hash.
Exact attach takes the registry lock in shared mode, acquires an atomic lease,
and updates LRU state atomically; allocation, retirement, hash rebuild, and
eviction remain exclusive lifecycle operations.

The accepted current-tree three-trial matrix used 1,000 queries per client at
1,024, 10,000, and 65,536 slots. The 65K/1K single-client p95 ratio is
`1.0855`; 65K 64-client throughput scales `1.2702x` over one client and
remains `0.9819x` the 1K 64-client QPS. Invalid startup fill is rejected.
Churn, nine evictions,
1,024-entry clear, reload, and before/after query fingerprints all pass.

### SAE-CAP-4 Implementation Evidence

The current page-native path projects snapshot-visible linked-L0 records into
bounded query scratch. It does not build or retain a complete transaction-local
semantic index image. The focused `REPEATABLE READ` gate performs 100 unchanged
`ii42_query(...)` calls, verifies stable rows and checked-root identity, then
adds another row and proves immediate L0 visibility. It records backend memory
and RSS after warmup, rejects growth beyond the fixed gate, and proves rollback
leaves no pending mutation or decoded-cache context.

The separate many-index transaction gate applies one large mutation across 24
SAE indexes. It verifies the shared transaction budget, read-your-writes parity
against the independent oracle, bounded RSS growth, savepoint/late-failure/2PC
cleanup, durable L0 publication, and zero pending contexts after completion.
Legacy decoded-cache capacity matrices remain historical oracle evidence, not
product release gates.

### SAE-CAP-5 Implementation Evidence

Shared-delta admission now uses configurable high/low watermarks with 25
percent default arena headroom. The pressure calculation sums all active
unified-delta segments across indexes and adds only the current index's
estimated missing tail. Exact compiled payloads known to exceed the arena are
kept in a bounded 256-entry keyed registry and fail immediately with SQLSTATE
`54000`; alternating indexes cannot overwrite one global last-miss state.

The accepted current-tree two-index, 1 MiB-arena test reached proactive
pressure with a `713,472` byte current-index estimate below the `786,432` byte
high watermark because resident parent segments contributed `435,064` bytes
and projected aggregate use reached `1,148,536` bytes. Both indexes remained
oracle exact. Two exact oversized identities were retained concurrently; the
initial failure returned SQLSTATE `54000` in `0.129` seconds and repeated
lookups returned in `0.006` and `0.019` seconds rather than consuming the
30-second wait timeout. Incremental spill-backed compaction converged without
`REINDEX`, final pressure was `none`, temporary-file growth was about 4.6 MB,
and both fast restart and immediate crash/restart recovered 100 exact results
from each index.

The combined evidence is recorded in
`docs/performance/reports/sae-capacity-qualification-2026-07-29.md`.

### Implementation Closure And Final Scale Gate

The first five implementation steps are complete. The final combined scale
matrix remains a clean-package release qualification gate rather than an open
design defect.

1. Completed: add baseline counters and reproducible capacity harnesses for
   queue time,
   runtime time, session loads, registry-lock time, cache-build records/bytes,
   append-lock hold time, local-view builds, and admission headroom.
2. Completed: implement the `SAE-CAP-1` per-worker control ABI and bounded
   two-worker
   execution path, then add model-affinity dispatch and adaptive batching only
   when measurements justify them. In parallel, implement `SAE-CAP-4`
   documentation, budget, and safe reuse. These do not alter posting layout.
3. Completed: implement `SAE-CAP-3` shared-registry lookup indexing and rerun
   every
   shared-cache lifecycle/failure gate.
4. Completed: promote the `SAE-CAP-2` incremental immutable prefix and bounded
   append-segment design after oracle, transaction, and replication parity;
   remove the old full-prefix publication path.
5. Completed: implement `SAE-CAP-5` proactive watermarks and spill-backed
   forward
   progress on top of the accepted incremental cache identity.
6. Pending clean-package qualification: run the combined large-corpus matrix:
   at least one million documents,
   64 query clients, sustained single-row and batched writes, two model
   checkouts, maintenance, VACUUM, restart, immediate crash, and standby replay.

Do not trade exactness for throughput in this work package. If a worker pool,
incremental cache, or spill compactor cannot preserve native/oracle parity, keep
the existing fail-closed implementation and explicitly limit the supported
deployment envelope rather than silently weakening ranking or MVCC behavior.

## Lifecycle Remediation Closure

The 2026-07-28 transaction and maintenance audit found additional lifecycle
and observability blockers that were not covered by the earlier lifecycle
matrix. They are closed in dependency order rather than treated as independent
local fixes.

| Priority | Finding | Closure | Current acceptance evidence |
| --- | --- | --- | --- |
| P0 | `PREPARE TRANSACTION` could publish a replacement generation from the preparing transaction's snapshot. | `PRE_PREPARE` flushes complete mutation records but never publishes a replacement generation. Prepared heap visibility remains PostgreSQL-owned. | `test_transactional_delta_lifecycle.py` passes SAE and BM25 commit-prepared, rollback-prepared, generation, restart, and reclamation gates; physical-replication coverage includes both 2PC outcomes. |
| P0 | BM25 realtime still published a full replacement generation from `PRE_COMMIT`. Relation page writes are not transactionally undone, so an error in any later pre-commit callback could leave an aborted heap update published in the index. `COMMIT PREPARED` has the inverse failure: it does not run the original backend's publication callback, so a prepared BM25 realtime write had no exact committed representation. | `PRE_COMMIT` and `PRE_PREPARE` now publish only complete append-only BM25/SAE delta records and counter debt. No transaction callback rebuilds or replaces a generation. Heap MVCC controls visibility; the ordinary post-commit maintenance worker later folds committed debt. | The 21/21 transactional delta lifecycle passes deterministic late-pre-commit failure, BM25 and SAE commit-prepared/rollback-prepared, immediate visibility, restart, VACUUM, and zero-debt convergence gates. |
| P0 | A materialized BM25 overlay is filtered by the current statement snapshot, but its cache identity contains only generation/delta metadata. A query whose snapshot predates a writer commit can therefore cache the correct old view and incorrectly reuse it in a later statement. Replacement-chain pruning also ran before visibility filtering, so an aborted latest TID could hide the prior committed row. | Heap-MVCC-filtered overlays are statement-scoped. Visibility filtering precedes replacement-chain pruning, and an old TID is suppressed only when its replacement is visible to the same snapshot. Snapshot-dependent entries are released on scan completion and reset on abort, subtransaction abort, and prepare. | The transactional suite passes old-snapshot/new-snapshot, late-abort, savepoint-abort, repeated-statement, and heap-visible-oracle parity gates. |
| P0 | A same-transaction SAE query flushed its queued mutation to the relation-owned delta and then waited for a background shared unified-delta cache. The publisher cannot observe the writer's uncommitted heap version, so read-your-writes timed out instead of using the exact local state. | A transaction that modified an SAE index builds an exact bounded backend-local unified-delta cache from its pinned snapshot and never publishes it. An older MVCC snapshot may use the same bounded exception. Complete delta identity plus visibility fingerprint permits transaction-local reuse; DML and transaction resolution invalidate it. Other backends still require the shared arena, so ordinary query memory remains process-independent. | Mutable lifecycle passes same-transaction insert, update, delete, savepoint, rollback, MVCC isolation, compaction, restart, and `REINDEX` parity without waiting for a publisher. The focused reuse gate proves 100 unchanged searches use one build, DML forces one rebuild, rollback frees retained state, and budget overflow fails with SQLSTATE `54000`. |
| P0 | PostgreSQL can call `aminsert` with `indexUnchanged=true` for a non-HOT update forced by another index. The old ii42 TID then becomes invisible, but the current automatic paths recorded only counter debt and omitted the unchanged value at its new TID. | Every non-HOT `aminsert` records the indexed value and new heap TID, including `indexUnchanged=true`. Pure HOT updates remain untouched because PostgreSQL does not invoke the index AM for them. | Mutable and transactional lifecycle gates use a second B-tree index and prove immediate visibility, exact compaction, restart, and `REINDEX` parity. |
| P1 | Realtime counter-only debt can require a heap rebuild when a value cannot fit one delta record. Rebuilding while the current transaction owns the writer barrier would publish an uncommitted snapshot. | A zero-record overlay is accepted only for non-stale debt that heap visibility can represent, such as a NULL transition. Stale or oversized same-transaction state fails closed; rebuilding occurs only after transaction resolution. | NULL-transition and oversized-input gates prove exact representable reads, explicit failure without generation change, and exact committed recovery. |
| P0 | Delta bytes and metapage counters used separate WAL records. A crash between them could leave an uncounted record before a later committed append. | Delta page bytes and metapage extent/count/bytes/debt are modified by one PostgreSQL Generic WAL action and receive the same LSN. | Mutable lifecycle checkpoints the relation and requires equal nonzero metapage/delta-page LSNs; the current run observed `0/1A0E990` on both pages. Crash, restart, and physical-replication suites remain exact. |
| P0 | The no-prefix online-maintenance fallback published the replacement manifest before replaying the concurrent delta tail. A crash could therefore expose a partial tail, and replay also counted tail debt twice. | The locked fallback now writes the complete replacement, semantic stream, and carried tail before one manifest switch. Tail record count, bytes, and debt are published exactly once; the old record-by-record post-publication replay path is removed. | The same-index gate pauses after the build snapshot, commits one writer, and forces `segment_reused=false`. It observes exactly one tail record and one pending write, exact native/oracle parity, fast-restart persistence, zero debt after compaction, and the normal page-reuse plateau afterward. |
| P0 | BM25 lightweight delta fold did not pin an all-committed start boundary or protect the final metapage comparison and publication with the writer barrier. A concurrent append could therefore be folded from an uncommitted state or be lost between the comparison and manifest switch. | Lightweight fold keeps `AccessShareLock` for relation lifetime, takes an initial exclusive writer barrier before its latest snapshot, preserves every untombstoned physical TID, stages only fenced retired pages, then reacquires the writer barrier and generation barrier for the final identity check and manifest switch. Any changed identity or busy barrier is retryable and cannot publish. Public maintenance rejects an index modified by the same transaction. | The deterministic BM25 race gate observes `lock_busy` while a writer is open, `meta_changed` when a tail commits during the paused fold, and exact retry convergence. Restart, crash, `REINDEX`, and oracle parity all pass. |
| P0 | BM25 lightweight fold used a latest MVCC snapshot as a physical-reclamation oracle. It could remove an updated tuple version that remained visible to an older `REPEATABLE READ` transaction, even though VACUUM had not declared that TID globally dead. | Statement overlays use their own MVCC snapshot, but physical fold preserves every untombstoned base/delta TID. Only an exact VACUUM tombstone can authorize physical reclamation. | The 21/21 transactional suite holds a long snapshot across update and fold, preserves the old token for that snapshot and the new token for a fresh snapshot, then reclaims only after snapshot end plus VACUUM. Cold restart, crash restart, and `REINDEX` parity pass. |
| P0 | Query snapshots, append serialization, generation replacement, and VACUUM XID cleanup shared one advisory lock. A query could hold `ShareLock` while waiting for a cache publisher; queued VACUUM `ExclusiveLock` then blocked that publisher behind PostgreSQL lock fairness. The condition-variable edge was invisible to the deadlock detector. | The lock is split into a shared/exclusive generation barrier and an independent append mutex. Queries refresh an advanced append-only delta identity while waiting. Single-flight publication briefly fences appends, and VACUUM XID normalization uses a nonblocking generation barrier so it defers instead of joining a wait queue. | Three 8-writer/8-reader stress runs completed without timeout or cache-attach failure. The final run completed 864/864 CRUD operations, 9,613 maintenance probes, 16,538 VACUUM calls, 1,650 reader queries, and exact normal/oracle parity. |
| P0 | Runtime attachment treated any `shared_preload_libraries` value containing the substring `ii42` as proof that the named shared-memory tranche existed. An unrelated library such as `my_ii42_helper` could therefore send a dynamically loaded backend into an invalid tranche lookup. | Preload membership is parsed with PostgreSQL's library-list parser and requires the exact library basename `ii42`. Dynamic loading without exact preload remains fail-closed and never touches add-in shared memory. | Product inventory rejects substring matching; runtime required-service and shared-preload smokes validate preloaded and non-preloaded behavior. |
| P0 | Once a realtime SAE shared delta was already current, `ii42_index_try_maintain()` returned `shared_unified_delta` forever when debt remained below the configured threshold. The scheduler correctly marked every realtime debt item due, but the execution path never promoted that state to compaction. | Realtime SAE debt is now always a generation-convergence candidate. A missing shared delta is published first and compaction is deferred one round; an already-current delta proceeds directly to incremental unified compaction. The bounded test driver rejects debt that survives four explicit rounds. | Same-index restart, 8-writer/8-reader/VACUUM stress, and repeated insert/delete page-reuse cycles finish with zero delta records, writes, and deletes. The current run completed 864/864 expected operations, 9,613 maintenance calls, and 16,538 VACUUM calls with exact normal/oracle parity and no timeout. |
| P1 | Abort and savepoint rollback could leave physical delta without tracked debt. | The atomic delta WAL action increments physical extent and reclaimable debt together. Heap MVCC filters aborted versions; later VACUUM/compaction reclaims them. | Mutable lifecycle proves abort/savepoint invisibility, exact overlay behavior, debt accounting, and zero debt after compaction. |
| P1 | VACUUM computed ordinals before pinning the generation used for tombstones. | VACUUM holds the writer barrier across generation read, dead-TID discovery, and tombstone publication. | Concurrent VACUUM/maintenance stress passes. A deterministic delete/VACUUM/insert test reuses CTID `(0,1)`, does not resurrect the old posting, and matches oracle, compaction, and `REINDEX` with zero score delta. |
| P1 | Semantic realtime writers were serialized by a transaction-long exclusive barrier. | Semantic automatic writers use a shared transaction barrier plus only a short append mutex. Replacement publication alone takes the exclusive barrier. | Eight writers/eight readers complete 288/288 CRUD operations with exact final results; the independent second-writer gate is below one millisecond. |
| P1 | Incremental semantic compaction bypassed rebuild-memory admission. | Every automatic semantic compaction uses one estimate covering TID maps, delta input, `work_mem`, and bounded overhead. Over-budget work remains exact base plus delta and is deferred. | Runtime-service lifecycle rejects an intentionally undersized budget without changing results, then explicit `REINDEX` recovers a clean generation. |
| P2 | Existing-index `COPY` and `INSERT ... SELECT` encoded one row per runtime request. | Transaction-scoped mutation queues use bounded 256-row/8-MiB chunks and the runtime batch limit. Successful flushes release copied datums; query encoding remains single-text. | Mutable lifecycle encodes 96-row insert-select in three requests and 64-row COPY in two requests, with rollback/savepoint and compaction parity. |
| P1 | A failed subtransaction could retain inactive copied mutation payloads in `TopTransactionContext` until the outer transaction ended. Repeated caught errors could therefore bypass the active 8-MiB batch bound. A later-row compiler failure also had to clean rows already appended by that batch, while an error after successful batch cleanup could re-free its temporary pointer. | Every queued mutation owns a child context, tracks accounting independently from physical append state, and restores the caller memory context on error. Subtransaction abort removes every affected mutation and empty batch shell immediately. Temporary batch ownership is cleared before any later error-capable scheduling step. | Mutable lifecycle catches 68 intentional multi-row failures with 64-KiB rows inside one outer transaction, forces an oversized second row after one valid row was appended, and injects a failure after a complete 256-row batch cleanup. All paths leave zero pending contexts or visible rows; the durable aborted appends remain reclaimable delta debt. |
| P2 | Legacy policy status exposed cache-derived count and byte targets that do not control page-native v3 maintenance. | Pre-beta ARCH-2 removes those reloptions and columns. Status reports durable L0/root debt; fixed physical frontiers and bounded worker rounds own v3 convergence. | Extension SQL regression and focused reloption rejection, low-rate convergence, frontier, and worker gates pass. |
| P0 | A semantic reader could capture the old base manifest, then acquire the generation snapshot after an online swap and combine that old base with the new delta identity. | Query and preload readers now acquire the index relation lock, then the shared generation barrier, then read the manifest. They keep that barrier through base/delta cache acquisition and release it before scoring. Append-only delta identity may refresh under the same generation; replacement publication uses the same relation-before-barrier order and remains nonblocking. | A deterministic pause holds the query after snapshot acquisition while online maintenance reaches publication. Maintenance returns `reason=lock_busy`, generation and query digest remain unchanged, and the next maintenance converges. The complete mutable lifecycle passes. |
| P2 | Lifecycle tests still assumed that realtime commit callbacks or one maintenance call always published a replacement generation. That assumption could reject the deliberate shared-delta-first protocol or accept a current cache without proving eventual compaction. | Runtime tests now permit only a bounded first-round shared-delta publication, then require the intended admission or clean-generation result. SQL regression separately asserts oversized realtime fail-closed debt and explicit post-commit convergence. Empty and UNLOGGED indexes validate exact base-plus-delta counters and query visibility across fast and crash restart instead of fabricating synchronous rebuilds. | Runtime-service smoke, isolated PostgreSQL regression, the complete empty/UNLOGGED and mutable lifecycles, and the bounded same-index convergence gate all pass. |
| P0 | Durable delta records retained ordinary 32-bit transaction IDs indefinitely. After PostgreSQL truncated their CLOG status, a cold reader could no longer distinguish committed and aborted appends; wraparound also made raw long-lived XIDs unsafe as permanent index state. | Every record carries its creating XID. Index VACUUM opportunistically takes the generation barrier without waiting, then the append mutex, and uses PostgreSQL's CLOG truncation lock to rewrite finished records older than `GetOldestNonRemovableTransactionId()` to permanent markers with Generic WAL. If PostgreSQL skips or defers index cleanup, status-less records remain only possible candidates and heap-TID MVCC is authoritative; a later exact cleanup records dead-TID tombstones before normalizing them. Exact VACUUM tombstones are born committed. | The mutable lifecycle mixes commit, abort, savepoint rollback, update, delete, and tombstone records; proves `INDEX_CLEANUP OFF` preserves MVCC results; then runs index cleanup, clears caches, and rejects any remaining normal XID before proving exact query and later compaction parity. |

Two storage invariants are also explicit release gates:

- a replacement built before the publication lock is obtained may write only
  into a fenced retired prefix. If that lock is lost, no live-tail page was
  appended. A non-prefix replacement is written only after publication locks
  are held. Its complete carried tail is written before the one manifest
  switch; no record is replayed after publication;
- repeated 4-document/5-document semantic replacement cycles remain at
  81/82 physical pages. Eight observed replacements plateaued immediately and
  stayed below the two-generation footprint bound.

The focused gates and the complete lifecycle matrix are both required. The
historical transaction and failure-safety queues in this section are closed in
source and installed-extension tests. The residual eventual maintenance queue
is separately reopened as EVT-1 through EVT-8 above. Clean-commit package
qualification remains the separate external release gate.

## Post-Commit Failure-Safety Closure

The post-commit review of `4b395de5` found no new transaction, lock-order, or
index-consistency regression. It did find ownership gaps around allocations
that PostgreSQL memory contexts and resource owners cannot reclaim after an
`ERROR` or query cancellation. These are a separate release blocker class:

| Priority | Finding | Closure | Acceptance evidence |
| --- | --- | --- | --- |
| P0 | `ii42_am_generation_build_shared_from_payload()` pins a DSM segment before interruptible fill and validation, but transfers ownership to the cache entry only afterward. Cancellation before transfer, validation failure, or descriptor-write failure can leave a pinned segment with no descriptor through which it can later be pruned. | Unpublished DSM ownership lives in one stable cleanup object. Failures before descriptor publication detach and unpin the segment; an invalid attachment removes the stale descriptor and globally unpins its segment before fallback publication. | Repeated publish, attach, and invalid-attach faults leave descriptor and mmap DSM inventories bounded. A subsequent publish and attach returns exact results. |
| P1 | Cold local generation construction allocates the generation block and sorted-vocabulary workspace with `malloc()` before interruptible work. Until ownership is published to the cache entry, its outer `PG_CATCH` cannot reclaim either allocation. | Local generation block and sort ownership are registered in stable cleanup state before interruptible fill and transferred exactly once to the cache entry. | Repeated local allocation/fill faults in one surviving backend have bounded RSS, leave no partial cache entry, and retry exactly. |
| P1 | Payload readers, BM25 overlay construction, and exact VACUUM deletion use local libc-owned payload/index builders across `CHECK_FOR_INTERRUPTS()`, buffer reads, visibility checks, and callback-driven work. Explicit corruption branches free them, but asynchronous `ERROR` paths can bypass cleanup before ownership reaches the cache entry. | Payload, overlay, incremental-compaction, shared-delta publisher, and exact-delete builders each have a stable exception cleanup owner. Visibility, relation, file, builder, and native-array ownership transfers are explicit. | Injected payload, overlay, and VACUUM traversal faults keep RSS bounded and leave no stale cache, lock, or tombstone state. Retry, compaction, and `REINDEX` match the heap-visible oracle. |
| P1 | AM scan initialization, direct search SRFs, match operators, and highlight/snippet helpers acquire cache leases, relation references, visibility state, and libc-owned query/ranking buffers before error-capable parsing, scoring, and heap visibility work. Their normal tail cleanup is skipped by `ERROR`. | Public search, raw-query, verified-hit, delta-merge, AM scan, match, highlight, and snippet boundaries now use stable cleanup owners. Ordinary and ordered `@@` scans transfer deferred/query ownership only after successful ranking and verification setup. | Repeated direct-search, index-scan, and post-parse operator faults in one backend permit exact retry (`20` direct hits and `5,000` scan hits), cache clear, VACUUM, and `REINDEX` without excessive RSS growth. |
| P0 | Several cleanup sentinels and stack-owned query, visibility, semantic-contract, and incremental-REINDEX structures were modified inside `PG_TRY` and read by `PG_CATCH` or `PG_FINALLY` without `volatile`. POSIX leaves those automatic values indeterminate after `siglongjmp`, so optimized builds could double-free, leak native allocations, or clean an invalid partial state even when ordinary fault tests passed. | Scalar ownership sentinels that must cross the error boundary are `volatile`. Multi-field cleanup state is allocated before `PG_TRY` and mutated only through a stable heap owner; match/highlight/snippet cleanup is centralized. | Product inventory enforces the longjmp-safe ownership shapes. Batch post-cleanup and query post-parse fault injection, optimized builds, static analysis, mutable lifecycle, and retry/rebuild gates validate both error and normal paths. |
| P2 | Existing cancellation coverage targets a waiting query, and corruption coverage checks fail-closed behavior once. Neither gate repeatedly interrupts the publishing backend at allocation/ownership boundaries or checks DSM/RSS stability. | Deterministic fault points cover local fill, DSM fill/attach/publication, payload reads, overlay construction, search/scan ranking, and VACUUM traversal. Backend-local RSS and cluster DSM inventories are asserted. | `test_cache_failure_safety_temp_pg.py --iterations 8` passes, and the focused suite is included in the product maturity runner. |

The queue was closed in dependency order:

1. define exception-safe ownership helpers for local generation blocks, payload
   buffers, overlay builders, and unpublished DSM segments;
2. apply those helpers to cold cache loading, shared publication, BM25 overlay
   construction, exact VACUUM deletion, and every public query boundary;
3. add deterministic failure injection and same-backend RSS/cluster DSM
   assertions before broad stress;
4. rerun the focused failure suite, mutable and transactional lifecycle,
   replication, corruption, sanitizer, static-analysis, and maturity gates.

## Runtime And Build Architecture Closure

The 2026-07-29 current-tree review found no new split lifecycle or index
consistency defect. It did reopen the runtime/build capacity boundary. These
items must be closed in dependency order; a request deadline or larger queue is
not an acceptable substitute for execution capacity or worker recovery.

| Priority | Finding | Required closure | Acceptance gate |
| --- | --- | --- | --- |
| P1 | One `CREATE INDEX` or `REINDEX` submits and waits for one document batch at a time. The worker pool therefore permits concurrent callers but does not parallelize one large build. | Add a bounded multi-flight document pipeline. A builder may keep multiple runtime batches in flight, must consume results in source-row order, must preserve one healthy query lane, and must cancel every outstanding request on `ERROR`. PostgreSQL parallel heap scan support is not required for this closure. | With three or more runtime workers, one build must show two simultaneously active document batches and preserve one healthy query lane. Rebuilding the same corpus must preserve ranking and scores. Cancellation, failed build, restart, and memory bounds must remain within the declared support envelope. |
| P1 | Caller cancellation makes an in-flight response ownerless but cannot stop an ONNX run. PID-only health checks cannot distinguish a live worker from a permanently hung provider call. | Track per-run progress and cancellation state. Use cooperative ONNX Runtime termination where supported and a supervisor liveness grace that recycles only the affected worker. Ordinary valid requests remain deadline-free. | Deterministically block one runtime run, cancel or terminate its owner, and prove that the affected worker is interrupted or recycled while another worker continues serving. The pool must return to its configured healthy count without postmaster restart or leaked response slots. |
| P1 | Runtime workers and the maintenance supervisor hard-code a database named `postgres`. Dropping or renaming that database disables cluster-wide model inference and automatic maintenance. | Give both catalog-validating runtime workers and the cluster maintenance supervisor one explicit, restart-only `ii42.control_database` contract. Application-database maintenance remains short-lived and round-robin; no application database is pinned by the long-lived workers. | Configure a non-`postgres` control database, rename `postgres`, and prove runtime encoding plus eventual maintenance across two ordinary application databases. |
| P1 | Attaching to an existing shared runtime segment with a different magic or ABI version reinitializes the live control block in place. | Initialize only a newly created segment. Any mismatch on an existing segment must fail closed with an actionable postmaster-restart error and must not modify shared state. | A deterministic ABI-mismatch gate proves unchanged control bytes, explicit failure, and successful recovery after a clean postmaster restart. |
| P2 | The runtime declares 64 request slots but reserves one of only eight 4-MiB response slots before admission, so the effective admitted-request limit is eight. | Make the bounded admission contract explicit and internally consistent. Either decouple queued descriptors from response buffers or declare the response-slot boundary as the actual admission capacity; do not allocate unbounded shared responses. | Status, documentation, and overload tests report the same effective capacity. At 64 clients, memory remains bounded, cancellation releases admission promptly, and query/document fairness remains deterministic. |

### Runtime And Build Implementation Evidence

The current tree closes the five implementation defects without changing
posting generation, scoring, or the single-relation lifecycle:

- one builder keeps two document batches in flight, consumes them in source
  order, and cancels every outstanding request on failure;
- with three workers, the staged runtime smoke observes two simultaneously busy
  document workers during one 512-document build while the query lane remains
  reserved; rebuilding the same corpus preserves rankings and scores;
- every worker publishes its active request, progress timestamps, PostgreSQL
  process identity, cooperative-termination count, and liveness-timeout count;
  owner cancellation terminates the ONNX run, releases its response slot, and
  leaves the remaining pool available;
- `ii42.runtime_liveness_timeout` is a restart-only execution-liveness guard,
  not a SQL deadline. A terminated run gets a five-second cooperative grace;
  only the affected worker exits if the provider ignores termination;
- `ii42.control_database` is the single restart-only database contract for the
  runtime pool and maintenance supervisor. A staged gate uses `template1` as
  control, renames `postgres`, and proves encoding and eventual maintenance in
  two application databases;
- existing shared memory is initialized only when newly allocated. Magic,
  version, size, worker-count, queue-capacity, or response-capacity mismatch
  fails closed with an explicit clean-postmaster-restart instruction;
- queue descriptors and response buffers now have the same eight-request
  capacity. Document admission reserves one response slot for queries, and
  privileged status reports all three bounds.

The final current-tree qualification used one PostgreSQL 18 staged package and
the production runtime checkout. ORT and no-ORT builds pass. The staged
two-worker, three-worker, and eight-worker runtime smokes pass. The
eight-worker run saturates seven document execution/admission lanes and proves
that a query uses the reserved worker and response slot before all document
requests complete. Worker-restart, required-service, privilege, isolated
extension regression, and shared-preload lifecycle closures also pass. The
Python suite reports `80 passed`; all scripts compile, the product convergence
inventory passes, and `git diff --check` is clean. The maturity runner's package
preflight compares the staged library, control file, extension SQL, and bundled
runtime against the selected PostgreSQL installation before any smoke runs;
its postflight requires the same fingerprint. This deliberately tests the
installed package boundary while proving that it is byte-for-byte bound to the
staged artifact.

### Post-Closure Product Review

The 2026-07-29 follow-up review found and closed thirteen defects not represented by
the earlier queue-depth and small-build gates:

- document admission counted queued and actively executing work, but not a
  completed response retained by a caller that had not consumed it. Seven such
  responses could therefore be present while another document request took the
  eighth response slot. Shared runtime ABI 15 records each response kind and
  reserves query admission against every occupied document response, including
  completed responses;
- the semantic builder kept one SPI connection open for prepared-plan reuse,
  but did not release each decoder tuple table. A large `CREATE INDEX` or
  `REINDEX` could consequently retain every decoded batch rowset until
  `SPI_finish()`. Each batch now frees its tuple table immediately while
  retaining only the prepared plan and bounded in-flight input batches;
- runtime response ownership previously used only an operating-system PID. If
  an owner exited and that PID was reused before orphan cleanup, a stale slot
  could be treated as live indefinitely. ABI 15 carries PostgreSQL
  `ProcNumber` beside every caller/owner PID, validates both before delivery or
  wakeup, reclaims stale responses by the same identity pair, and prevents a
  stale health probe from marking a newly restarted worker unavailable;
- the ONNX session cache keyed a model only by checkout path, encoder path, and
  provider. An atomic valid checkout update at the same path could therefore
  pass artifact validation and receive a new generation signature while still
  using the old loaded session. The canonical manifest signature now forms
  part of every session-cache key;
- each worker could cache 16 ONNX sessions but artifact validation remembered
  only the last checkout. Alternating models therefore rehashed every artifact
  even when both sessions were hot. Validation now uses a bounded 16-entry LRU;
  every cache hit still restats every artifact and any identity change forces
  full SHA-256 validation;
- worker-local ONNX session ownership was checked by PID alone. The owner now
  also carries PostgreSQL `ProcNumber`, preventing PID reuse from attaching a
  replacement worker to the departed worker's session;
- tokenizer and compiler artifact caches used only checkout path identity.
  Their keys now include the canonical checkout signature, so a valid
  same-path replacement cannot retain stale tokenizer or compiler state;
- a long semantic build validated the checkout at initialization but could
  publish after an atomic same-path replacement. It now verifies the runtime
  contract at batch submission, batch completion, payload finalization, and
  generation publication;
- query encoding and native scoring did not carry one generation identity
  through the complete operation. The encoder now returns its runtime
  signature and the scorer opens only a matching generation under the relation
  lock and generation barrier;
- the shared arena treated small payloads as invalid blocks and shrank the
  physical capacity of reused blocks. Shared preload ABI 20 tracks logical
  payload and aligned allocation separately, accounts warm markers and
  tombstones during rewind, and preserves reusable interior capacity;
- an undersized retired interior block was considered a free metadata slot.
  Appending a larger payload through that slot replaced its allocation
  metadata and left the old physical hole untracked. New tail allocations now
  require a truly empty slot; retired blocks remain tracked until an
  allocation fits them or arena rewind can reclaim them;
- runtime worker affinity keyed only the checkout path. Query and document
  requests for the same model could therefore alternate session configuration
  and evict each other in the default two-worker pool. Affinity now includes the
  request role, query admission prefers a query worker, document admission
  prefers a document worker, and bounded stealing remains available for
  liveness;
- CPU document sessions retained ONNX Runtime memory-pattern and arena
  high-water allocations sized by the largest build batch. Document sessions
  now disable those two retention optimizations while query sessions retain
  them for latency. The session-cache key includes the role so incompatible
  session options cannot alias.

The isolated two-worker runtime smoke pauses seven caller backends after their
document responses are complete, proves that an eighth document cannot enter,
and then completes a query through the reserved response and worker lane. The
same staged run covers low-`work_mem` semantic rebuild, build batching,
cancellation, control-database switching, ordinary-role search, and result
parity. It also atomically changes a valid manifest at one checkout path,
proves exactly one replacement session load, and then proves the new session is
cached. Runtime restart and privilege smokes pass against runtime ABI 15.
Shared preload ABI 20 and the complete 80/80 mutable lifecycle include a live
warm-marker plus unified-delta allocation gate. ORT and no-ORT PostgreSQL 18
builds, the 82-test Python suite, script compilation, product convergence
inventory, and `git diff --check` also pass.

The current single-model resource gate warms both request roles, runs a
500-round mixed allocator-settling phase, then applies the unchanged 16 MiB
growth and 64 KiB-per-iteration slope limits to 1,000 formal two-client query
rounds and 1,000 formal mixed query/document rounds. Both formal phases perform
zero session loads and zero evictions. Query throughput is 102.738 requests/s
with 19.522 ms p95; aggregate RSS grows by 5.64 MiB with a 1.222
KiB-per-iteration tail slope, and the two worker slopes are 0.570 and 0.652.
Mixed throughput is 17.047 requests/s with 124.633 ms p95; aggregate RSS grows
by 528 KiB with a 0.026 KiB-per-iteration tail slope. The settling phase
releases about 3.92 GiB of allocator high-water memory before formal
measurement. This closes first-beta single-model RSS convergence without
misclassifying delayed allocator release as a leak; it does not claim
multi-model throughput qualification.

## Current Technical Closure

The current lifecycle gates provide evidence for ordinary CRUD, rebuild,
restart, corruption, and replication paths:

- the complete mutable CRUD, transaction, maintenance, restart, crash, rebuild,
  conversion, and drop lifecycle;
- the 21/21 SAE/BM25 transaction and 2PC lifecycle, plus primary/standby
  prepared-transaction replay;
- same-index concurrent writers, readers, maintenance, VACUUM, and runtime
  requests, including bounded replacement-page reuse;
- exact generation wait, cancellation, timeout, and publisher-death handling;
- physical replication and corruption fail-closed gates;
- ORT/no-ORT, normal, ASAN, UBSAN, static-analyzer, and resource-soak checks;
- exact delta-versus-compacted and delta-versus-full-overlay comparisons;
- the formal paired latency, memory-scaling, and 10,000-index discovery gates.

The earlier transaction, failure-safety, `SAE-CAP-1` through `SAE-CAP-5`, and
EVT-1 through EVT-8 queues are closed with the qualified support boundaries
above. The measured two-worker default remains intentionally conservative;
three workers are required to execute two batches from one build concurrently
while preserving a query execution lane.
One of the eight response slots is always withheld from document admission.
Current staged PostgreSQL 18 smokes cover two- and three-worker runtime
operation, multi-flight build parity, cancellation termination,
control-database independence, and exact admission reporting. Release
readiness still requires the complete source, package, schema, migration,
clean-commit, and applicable capacity matrix below; those external gates are
qualification work, not an unresolved architecture split.

## External Release Execution Gate

Final release evidence must be generated from an authorized clean commit. It
must:

1. stage both the II-42 release artifact and the supported `psql_bm25s` source
   package;
2. preserve identical staged-package preflight and postflight fingerprints;
3. run an isolated mixed-binary ABI-mismatch injection, proving unchanged
   shared control bytes, explicit failure, and clean-restart recovery;
4. pass the PostgreSQL 17 and PostgreSQL 18 package matrix;
5. pass the PostgreSQL 18 container smoke;
6. record one matching commit, source ZIP, package, runtime, and checksum set.

The maturity runner deliberately refuses dirty-tree evidence. Do not bypass it
with an allow-dirty build. Until a commit is explicitly authorized, these are
external release execution gates, not unresolved product defects.

## Qualification Matrix

| Contract | Required current evidence |
| --- | --- |
| Source and documentation closure | Full `pytest`, Python compile, shell syntax, product convergence inventory, and `git diff --check` |
| Eventual SAE residual closure | EVT-1 through EVT-8 adversarial correctness, bounded drain, poison isolation, atomic admission, sustained-ingress phase progress, cross-index fairness, temporary-relation handling, and transaction-wide retained-memory bounds |
| Fresh package and schema | PostgreSQL 17/18 build plus regression; fresh and pinned install in a non-`public` schema |
| Supported historical migration | Package-bound `psql_bm25s` source-table migration with concurrent build, CRUD parity, rollback, and cutover |
| Current index conversion | Same relation converts BM25 to model-backed and back only after an atomic `REINDEX` |
| Mutable lifecycle | Realtime, eventual, and manual CRUD; savepoints, abort, HOT update, VACUUM, compaction, and drop |
| Failure lifecycle | Fast restart, immediate crash, corrupt payload/model, failed rebuild, and bounded runtime-resource soak |
| Concurrent operations | `CREATE INDEX CONCURRENTLY`, `REINDEX CONCURRENTLY`, active writer, relation rewrite, truncate, and concurrent drop |
| Physical replication | Primary and standby agree after build, all CRUD families, maintenance, type conversion, `REINDEX`, and drop |
| Defensive C/runtime boundaries | ORT and no-ORT builds, ASAN/UBSAN tests, malformed-payload and query-complexity tests, and no unclassified analyzer findings |
| Evidence binding | Isolated smokes name and load the staged extension library, control files, model checkout, and source identity under test |
| Release artifacts | Clean source identity, pinned runtime checksum/license, staged-package pre/post fingerprint, PG17/18 ZIP, and PG18 container |

Evidence is valid only when it names the tested commit and staged package.
Historical reports may establish design history, but they cannot substitute for
a current package-bound result.

## Stop Conditions

Release readiness is complete only when:

- no tracked research artifact remains at repository root;
- documentation contains no retired product API or split-lifecycle guidance;
- EVT-1 through EVT-8 are closed with current package-bound evidence;
- `SAE-CAP-1` through `SAE-CAP-5` have accepted evidence, or the public support
  statement explicitly excludes high-concurrency, multi-model, and sustained
  high-churn SAE workloads;
- current C exports and installed SQL bindings close in both directions;
- ORT/no-ORT builds, sanitizers, adversarial inputs, and analyzer review have
  no unclassified failure;
- the full Python, shell, PostgreSQL, source-table migration, current-index
  conversion, lifecycle, replication, and package-bound maturity gates pass
  from a clean commit;
- the final worktree is clean and every evidence artifact identifies the same
  commit and package fingerprint.

Any partial generation, historical catalog residue, split lexical/semantic
publication, package drift, replication mismatch, or undocumented public API
is a release blocker rather than an accepted warning.
