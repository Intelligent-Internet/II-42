# Convergent Segmented Index Development Record

Archived: 2026-08-03

Historical milestone: `CSG-BETA-ARCH-1`

Baseline tag: `ii42-pre-convergent-segments-20260730`

Qualified milestone tag: `csg-beta-arch-1-qualified`

This document preserves the implementation programme, decisions, issue history,
and acceptance evidence that produced the current convergent segmented index.
It is a closed stage-development record, not the current product design or an
active TODO authority.

The current contract is defined by the
[Convergent Segmented Index](../../convergent-segmented-index.md). Historical
wording below is retained as evidence of the implementation path and must not
be used to reopen retired APIs, physical formats, or duplicate lifecycles.

At archival time, two non-blocking post-beta boundaries remained recorded:

- `CSG-I114`: optional behavior-preserving extraction from the remaining
  access-method orchestration;
- `CSG-I132`: deferred evaluation of PostgreSQL parallel heap build, parallel
  VACUUM discovery, and parallel AM scan.

Neither item blocks the qualified functional milestone. Their product-facing
constraints are carried forward in the current design document.

## Memory Ownership Invariant

- A small BM25-only index may use a bounded backend-local compatibility cache
  when no shared tier is available. This is a degraded BM25-only path, not the
  default architecture.
- SAE never has that fallback. Model sessions, tokenizers, runtime queues,
  exact-root registries, and derived residency are owned by postmaster-started
  shared-runtime workers. If that service is unavailable, SAE fails closed.
  The current runtime ABI version 15 is owned by the shared service.
  Relation pages and the checked root remain posting authority; the runtime
  does not own a complete mutable unified cache.
- Ordinary BM25 and SAE queries retain only bounded query scratch. They must
  not decode or retain an index-sized posting, vocabulary, document, or score
  workspace in backend-private memory. Relation payloads remain in PostgreSQL
  pages and shared buffers.
- A bounded transaction-local SAE read-your-writes view is the sole exception;
  it must be destroyed at transaction cleanup and cannot become a general
  backend cache.

## 2026-08-01 Architecture Review

The convergent v3 storage shape remains the approved product direction. One
checked read root, immutable segments and folds, linked L0, document/term COW,
one global directory, and one page-native scorer form a coherent lifecycle.
The current beta blocker is contract drift around that design, not a need for
another storage architecture.

### Approved Beta Decisions

These decisions are the reviewed beta contract:

1. SAE mutation is lexical-first and eventual-only for the first beta. Model
   inference remains worker-owned; foreground writes never run document
   inference. Pure BM25 may retain both realtime and eventual policies.
2. Ordinary v3 queries are page-native. Relation pages, PostgreSQL shared
   buffers, exact-root markers, and optional HOT_FOLD residency are the current
   read architecture. The legacy complete unified-delta cache is a v2
   differential-oracle mechanism, not v3 query authority.
3. Every public reloption and policy recommendation must describe an actual v3
   control. Capacity guidance must derive from linked-L0 frontier, read
   amplification, worker throughput, and semantic-completion SLA rather than
   legacy decoded-cache expansion.
4. Legacy v2 remains temporarily available only as an independent test oracle.
   It is not a supported storage, migration, query, or maintenance fallback.
5. Future modularization follows existing authority boundaries and must not
   introduce another lifecycle, scorer, root, or public API.

### Execution Order

1. Close the public consistency contract and add a mutation regression for
   the documented default SAE index.
2. Realign architecture, memory, parameter, policy, API, and quickstart
   documentation with the page-native v3 implementation.
3. Audit and measure the remaining threshold/worker controls; remove or
   redefine options that do not control the behavior their names advertise.
4. Freeze independent oracle artifacts, then isolate v2 from installed product
   dispatch and recommendations.
5. Plan an authority-boundary-only split of the oversized AM implementation
   after the P0/P1 contract work is complete.

### Accepted Beta Boundaries

- Partitioned-parent global ranking is unsupported; use one unpartitioned
  indexed relation when corpus-wide statistics and top-k are required.
- PostgreSQL parallel heap build and parallel AM scan are not implemented.
- Physical standbys replay index pages but require the same external model
  checkout and contract to be provisioned independently.
- Hot-working-set convergence is the peak-latency contract; a first cold term
  may still traverse a bounded fragmented representation.
- Row-level security is unsupported because post-filtering top-k would be both
  incomplete and unsafe. Use a non-RLS materialized search relation.

## Beta Architecture Remediation Programme

### Goal: CSG-BETA-ARCH-1

Converge the public v3 contract and implementation into one lexical-first,
page-native product path, then split the oversized access-method implementation
along existing authority boundaries without changing index behavior.

The completed result must be easier to reason about, test, and extend while
retaining one root, one mutation lifecycle, one maintenance selector, one
page-native scorer, and one SQL/API surface.

### Non-Goals

- Do not redesign the v3 on-disk format, scoring mathematics, COW tree formats,
  linked-L0 protocol, or HOT_FOLD representation.
- Do not introduce a repository/service layer, generic framework, C++ rewrite,
  alternate scheduler, per-segment top-k, or a second semantic lifecycle.
- Do not retain compatibility for unpublished experimental v2 layouts after
  independent oracle coverage is frozen.
- Do not add partitioned-parent ranking, PostgreSQL parallel build/scan, RLS,
  or multi-model scheduling in this programme.
- Do not combine a behavior change with a large mechanical file move.

### Mandatory Invariants

1. `CREATE INDEX`, foreground mutation, worker completion, query, `VACUUM`,
   `REINDEX`, crash recovery, replication, and `DROP INDEX` share one checked
   v3 authority.
2. SAE foreground mutation performs no document inference. Lexical evidence is
   durable and visible first; semantic completion is worker-owned and eventual.
3. Ordinary v3 queries retain bounded backend scratch and never attach a
   complete index-sized decoded snapshot.
4. Folds, impact images, root markers, and prewarm state are exact derived
   accelerators. Losing them may increase latency but cannot change results.
5. Pure BM25 correctness, latency, memory, and write behavior must not regress
   when SAE is disabled.
6. PostgreSQL memory contexts, resource owners, locks, WAL, snapshots, and error
   cleanup remain explicit at every new module boundary.
7. Internal APIs use narrow typed inputs and opaque owners. No catch-all
   internal header may recreate the current monolith through shared structs and
   globals.

### ARCH-0: Freeze Behavior And Dependency Map

- [x] Record the exact branch, commit, build flags, PostgreSQL version, model
  checkout contract, and current dirty-state boundary.
- [x] Capture current public SQL functions, reloptions, GUCs, AM callbacks,
  exported C symbols, and Makefile object inventory.
- [x] Produce a call/dependency map for build, mutation, root publication,
  worker selection, semantic completion, query, VACUUM, and reclamation.
- [x] Record the canonical lock order, transaction/resource-owner boundaries,
  memory ownership, root-revalidation points, and WAL publication points.
- [x] Freeze exact result, status-JSON, error, memory, and performance baselines
  needed to distinguish behavior changes from mechanical moves.

Gate: no implementation move starts until every mutable authority and lock
edge has one named owner. This phase changes documentation and test fixtures
only.

#### ARCH-0 Live Baseline

- Frozen parent: `sae` at
  `2084a7e3688cbc0df6238290e122f26fc7dbbd6f`. Architecture work is measured
  against this parent until ARCH-0 closes.
- Toolchain: PostgreSQL 18.4 from the Homebrew PostgreSQL 18 PGXS and
  ONNX Runtime 1.28.0. The real-model fixture is
  `/Volumes/Betty/Tmp/ii42-runtime-model-p22`, runtime ABI
  `ii42_p2_unified_text_atoms_v2`.
- Frozen product inventory: 123 installed SQL function declarations, 20 GUC
  registrations, 23 reloptions, and the current AM callback table and Makefile
  object list. Source hashes are recorded in the ARCH-0 execution evidence.
- Fresh core build/test passed `1/1`; staged native v3 lifecycle passed `80/80`;
  staged real-model SAE lifecycle passed `11/11`; transaction, savepoint, 2PC,
  VACUUM, and crash/restart lifecycle passed `21/21`.
- The authoritative durable state is the checked metapage read root and its
  manifest/COW closure. Linked active and pending L0 own the immediate mutation
  frontier. Shared runtime state owns model sessions, queues, exact-root
  markers, and derived residency only; full decoded snapshots remain
  maintenance/test oracles.
- Canonical lock order starts with PostgreSQL relation ownership. V3 DML then
  pins the transaction writer barrier and takes short append serialization for
  root-checked L0 publication. Legacy snapshot readers use the generation
  barrier until ARCH-3 removes that route. Maintenance root publication is
  serialized by the append lock and flushes the final root WAL record. Relation
  `AccessExclusiveLock` is conditional and limited to old-reader fencing for
  truncation/reuse; it is not an ordinary mutation or query lock.

#### ARCH-0 Frozen Product Inventory

- Frozen source hashes: installed SQL
  `cd02c7fd2e9b9fa70d207dfddfaff265fdae958fce8647dca72aa52969240d1c`,
  pre-remediation `ii42_am.c`
  `c0a4a5ceaae29388b3cfb42d2ee582ad1d091dfaf106b61aabdf5b1345eac1fb`,
  `Makefile`
  `c2322bb6612587bcf2fc3d9cf6fbe255d3d29ec32cbfd915ccbe7d30fbadae63`,
  and `ii42.control`
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- The installed SQL declares 123 functions. The extension registers 20 GUCs
  and 23 reloptions. The built dynamic-symbol inventory is frozen from
  `nm -gU ii42.dylib`; it includes product SQL/AM/runtime entrypoints and
  page-native test hooks that ARCH-3 must remove from the product binary.
- The AM callback surface is exactly `ambuild`, `ambuildempty`, `aminsert`,
  `aminsertcleanup`, `ambulkdelete`, `amvacuumcleanup`, `amcanreturn`,
  `amcostestimate`, `amoptions`, `amvalidate`, `ambeginscan`, `amrescan`,
  `amgettuple`, `amgetbitmap`, and `amendscan`. Parallel build and scan are
  disabled.
- The frozen PGXS object list is `ii42_am`, `ii42_pg`, `ii42_pg_common`,
  `ii42_semantic`, `ii42_block_ranges`, `ii42_core`, `ii42_page_query`,
  `ii42_query`, `ii42_p2_runtime`, `ii42_document_cow`, `ii42_lexicon_cow`,
  `ii42_prefix_cow`, `ii42_posting_heat`, `ii42_segment_pages`,
  `ii42_segments`, `ii42_term_cow`, `ii42_storage`, `ii42_stem`, and
  `ii42_text`.

#### ARCH-0 Frozen Authority And Dependency Map

| Flow | Current entry and call direction | Mutable/durable authority | Boundary |
| --- | --- | --- | --- |
| Build / REINDEX | `ii42_ambuild` -> `ii42_am_build_common` -> replacement builder -> convergent segment writer | New relation pages and the final checked metapage root | PostgreSQL owns heap/index DDL locks; a dedicated build context and spill `BufFile`s die before callback return; INIT and MAIN pages are WAL protected |
| Foreground mutation | `ii42_aminsert` -> lexical encoding -> `ii42_am_append_convergent_l0_record` | Active linked L0 plus its atomically advanced metapage root | PostgreSQL relation lock -> transaction writer-barrier share lock -> short append lock -> buffer locks; no SAE document inference |
| Root publication | bounded action builds immutable pages -> `ii42_am_publish_cow_manifest` | Existing checked root remains authority until one metapage switch | Per-index maintenance ownership; append lock revalidates root/frontiers; final root Generic WAL record is flushed before maintenance success |
| Worker selection | supervisor/work hint or `ii42_maintain_due` -> `ii42_am_get_background_due_candidate` -> `ii42_am_try_maintain_index_oid` | One scheduler classifies linked-L0, semantic, structural, and optional debt | Cluster worker slot and transaction-scoped per-index maintenance lock; one bounded action per transaction |
| Semantic completion | pending document COW scan -> shared runtime request -> visible tuple/fingerprint revalidation -> semantic transition append | Same document COW and linked-L0 chronology as lexical mutation | Model sessions are runtime-worker-owned; inference runs before publication locks; stale/HOT output is retired or rejected, never patched in place |
| Query | AM scan / SQL query -> page-native query preparation -> `ii42_page_query` -> `ii42_segment_pages` cursors | One checked root, COW document authority, and snapshot-visible linked-L0 projection | Index relation and PostgreSQL snapshot outlive scan; scan context owns only query runs, matching L0 data, top-k, and fixed document blocks |
| VACUUM | `ii42_ambulkdelete` -> document-COW walk and bounded L0 collection -> COW retirement publication | Document/version COW is tuple/retirement/statistics authority | Writer barrier excludes unsafe version transitions; callback-driven discovery is bounded-memory and root publication follows the same COW switch |
| Reclamation / fold | maintenance plan -> nonblocking old-reader fence -> root/high-water revalidation -> COW publication/FSM handoff | Manifest-authenticated retirement hints and current root; FSM is reconstructible, never authority | Conditional relation `AccessExclusiveLock` only for page reuse/truncation; busy readers force append/yield; reused pages receive Generic WAL full-page images |
| Preload / HOT_FOLD | exact-root marker and PostgreSQL shared-buffer residency | Durable fold remains relation-owned; marker, heat, and residency are disposable | Shared arena state may vanish or evict without changing rows/scores; it owns neither root publication nor model sessions |

The intended one-way module dependency after ARCH-4 is:

```text
AM callbacks / SQL control
    -> options + meta projection + build/mutation/maintenance/scan owners
        -> page query or segment-page publication
            -> document/term/lexicon/prefix COW codecs + segment codecs
                -> core scoring/text primitives

semantic maintenance -> shared runtime client -> mutation publisher
preload/heat ---------> checked root and segment pages (derived state only)
```

`ii42_am.c` currently also contains the generation-delta v2 cache/scorer and
test hooks. That is the sole side dependency to quarantine under ARCH-3; it is
not allowed to become an input to v3 publication, maintenance selection, or
page-native scoring.

#### ARCH-0 Lock, Snapshot, Memory, And WAL Contract

1. Foreground DML enters with PostgreSQL heap/index relation ownership, pins
   the transaction writer barrier, then takes the append lock only while
   revalidating and publishing linked L0. Transaction callbacks release
   transaction-scoped state; subtransaction rollback discards its pending
   state.
2. Online maintenance first owns a cluster worker slot and transaction-scoped
   per-index maintenance lock, then obtains index `AccessShareLock`. It builds
   immutable output without the append lock, takes that lock only to re-read
   the root and publish, and performs at most one selected action.
3. Full refresh/REINDEX uses heap `ShareLock` before index
   `AccessExclusiveLock`. Old-reader page reuse uses a separate nonblocking
   relation `AccessExclusiveLock` fence and must revalidate root identity and
   physical high-water mark after the fence.
4. Normal v3 scans hold the index relation and visibility snapshot; they do
   not acquire a corpus-wide AM cache lock. The scan memory context owns only
   bounded page-native state. Build state owns its dedicated context and spill
   files. Model sessions live only in bounded shared-preload runtime workers.
5. Every L0 inline append WAL-logs root and tail together. Multi-page records
   write an unreachable complete chain before publishing its tail link and
   root. Immutable COW objects are WAL-protected before the final root switch;
   successful maintenance flushes that root LSN. Reuse markers use Generic WAL
   full-page images. Foreground appends retain PostgreSQL commit durability.
6. Publication always revalidates the metapage/root identity. COW switches
   additionally check the build root and required L0 frontiers. Semantic
   completion revalidates tuple visibility and input fingerprint. Reclamation
   revalidates root and relation size after the reader fence.

Exact rows/scores and status/error shapes are frozen by the 80/80 native,
11/11 real-model SAE, 21/21 transaction, and 8x8 concurrency artifacts below.
The frozen performance/memory references remain
`docs/performance/data/diagnostics/convergent-query-states-2026-07-31/`,
`run-v3-impact-40k.json`, `run-v3-impact-250k.json`, and
`/tmp/ii42-final-prefix-fix-backend-rss-20260801.json`. ARCH-4 mechanical
commits must compare against these references; ARCH-5 will regenerate them from
the final staged package.

The initial current-source 8-writer/8-reader real-model run exposed CSG-I115:
concurrent VACUUM tried to convert an already materialized same-sequence COW
version into an `L0_OWNED` record. The transition validator correctly rejected
that ownership mutation. The repaired classifier now preserves materialized
ownership and residency while adding retirement, closes same-sequence aborted
placeholders as retired L0 records, and ignores stale L0 incarnations.

Current-source closure evidence from the final staged PG18 build:

- `/tmp/ii42-csg-i115-final-8x8.json`: 288/288 foreground mutations,
  8/8 readers, 8/8 writers, 678 maintenance calls, and 694 VACUUM calls passed;
  native/oracle query IDs match and the 24-cycle storage plateau passed.
- `/tmp/ii42-csg-i115-final-native80.json`: native v3 exactness and lifecycle
  passed 80/80 gates.
- `/tmp/ii42-csg-i115-final-sae11.json`: real-model lexical-first SAE lifecycle
  passed 11/11 gates, including restart and semantic completion.
- `/tmp/ii42-csg-i115-final-txn21.json`: BM25/SAE MVCC, 2PC, crash, and VACUUM
  lifecycle passed 21/21 gates.
- `/tmp/ii42-csg-i115-final-frontier140k.json`: the 140,000-row bounded VACUUM
  frontier passed 10/10 gates with one 96-byte fence and no corpus-sized L0.

CSG-I115 is closed. The inventory and authority map above close ARCH-0. ARCH-1
is the next product change; no mechanical extraction starts before ARCH-3
removes the duplicate runtime route.

### ARCH-1: Close The SAE Consistency Contract

Closes CSG-I110 before any large refactor.

- [x] Make omitted `consistency` resolve to `eventual` when `sae=true`, while
  preserving the pure-BM25 default and behavior.
- [x] Reject explicit `sae=true, consistency=realtime` during index definition,
  before an index can be built in an unusable state.
- [x] Update policy recommendations, reloption validation, status output,
  errors, README, quickstart, API, parameter, policy, migration, and operations
  documentation to state one contract.
- [x] Remove realtime-SAE test assumptions rather than preserving an
  unpublished transition route.
- [x] Add gates for default SAE, explicit eventual SAE, rejected realtime SAE,
  BM25 realtime, CRUD, same-transaction behavior, worker completion, REINDEX,
  restart, and physical standby replay.

Gate: the exact documented default SAE command must survive its first INSERT,
UPDATE, DELETE, maintenance cycle, restart, and query without hidden options.

#### ARCH-1 Execution Slices

1. **Contract cutover**: resolve an omitted `consistency` after parsing all
   reloptions. `sae=true` selects `eventual`; pure BM25 retains `realtime`.
   Reject explicit SAE `realtime` and `manual` during index definition while
   retaining the foreground runtime check as a defensive invariant. Remove the
   now-unreachable SAE-realtime threshold and recommendation branches.
2. **Focused contract gate**: prove default SAE and explicit-eventual SAE report
   `eventual`; explicit SAE realtime/manual fail before relation publication;
   default and explicit-realtime BM25 remain realtime. The default SAE fixture
   must exercise lexical-first INSERT/UPDATE/DELETE, same-transaction reads,
   bounded worker completion, REINDEX, restart, and query parity.
3. **Recovery closure**: run the existing transaction/2PC and physical-standby
   suites with at least one SAE index omitting `consistency`, so parser-default
   behavior is covered by WAL/restart rather than only by status inspection.
4. **Public contract closure**: update current README, quickstart, API,
   parameters, policy, migration, operations, testing, and release guidance.
   Static inventory must reject any product claim that SAE realtime/manual is
   supported. Historical research artifacts are not rewritten.

The public-contract slice covers `README.md`, current architecture, API,
parameter, policy, memory, shared-runtime, testing, upgrading, quickstart,
operations, and runtime-contract documents. It may add static inventory checks
for those files. Research reports, performance evidence, and this ledger retain
their dated diagnosis; they are not rewritten into current product claims.
Control semantics remain an ARCH-2 behavior change: this slice may stop
recommending cache-derived sizing, but it does not remove or redefine a GUC or
reloption before its call sites and focused gates are audited.

The contract change, documentation closure, and later mechanical extraction
remain separate commits. Any BM25 behavior, score, root bytes, lock/WAL order,
RSS ownership, or native exactness change stops ARCH-1 for diagnosis.

Current contract-cutover evidence from one staged PG18 build:

- `/tmp/ii42-arch1-sae-contract.json`: 15/15 gates. A default SAE index reports
  `eventual`; explicit eventual succeeds; explicit realtime/manual fail with
  SQLSTATE `22023` and leave no relation; default/explicit-realtime BM25 remain
  realtime. Default SAE also passes REINDEX, same-transaction rollback, CRUD,
  semantic completion, crash, and cold restart.
- `/tmp/ii42-arch1-txn21.json`: transaction, savepoint, 2PC, crash, and VACUUM
  lifecycle passes 21/21 with the primary SAE fixture omitting consistency.
- `/tmp/ii42-arch1-native80.json`: pure BM25/native v3 exactness and lifecycle
  remain 80/80. The product-convergence inventory and `git diff --check` pass.
- The staged runtime-service temporary-PG smoke passes after replacing its
  legacy identity-map/cache oracles with v3 primary-payload, dual-debt
  convergence, runtime-idle, and reachable/physical-block assertions. It proves
  default-SAE lexical-first CRUD, bounded worker completion, exact single-row
  inference, segment reuse, VACUUM retirement, restart, and explicit REINDEX.
- `/tmp/ii42-arch1-concurrent-ddl.json`: concurrent DDL, REINDEX, relation
  rewrite, TRUNCATE, post-TRUNCATE lexical visibility, and drop pass 8/8.

Physical-standby replay and the full mutable-lifecycle gate now pass. Policy
recommendations are index-type-aware: SAE profiles emit eventual-only options,
and BM25-only realtime profiles are rejected for SAE. Current product docs now
state one eventual-only, lexical-first, page-native contract. The static
inventory scans those documents for unsupported SAE definitions and stale v2
cache authority; `python3 scripts/test_product_convergence_inventory.py`, its
`py_compile`, and `git diff --check` pass. CSG-I110 and CSG-I111 are resolved.

The active maturity-suite inventory still contained realtime-SAE fixtures in
the mutable lifecycle, runtime service, provider matrix, concurrent DDL, and
three unified-delta pressure/capacity scripts. They are not historical files:
the maturity runner invokes all of them. DDL/provider fixtures will use the
dependent default or explicit eventual policy. Tests that counted foreground
model requests must instead prove zero foreground inference, immediate lexical
visibility, bounded worker batching, and eventual semantic convergence. The
unified-delta scripts remain legacy-oracle coverage until ARCH-3, but may not
advertise or require an unsupported product consistency mode in the interim.

The legacy `test_unified_delta_incremental_cache.py` gate now fails against a
healthy page-native v3 root because it requires a complete
`shared_unified_delta_cache_current` image even when linked L0 is empty. This is
not an ARCH-1 regression and must not revive that cache. ARCH-2 replaces the
maturity-suite requirement with page-native frontier, exact-root, and optional
residency evidence; ARCH-3 then quarantines the old cache oracle.

The mutable-lifecycle suite also carried a v2 transaction-batch gate that
expected oversized document text and a post-cleanup fault hook to fail before a
decoded delta was published. V3 has no such transaction-local semantic batch:
foreground mutation appends bounded lexical L0 records. The active gate now
tests savepoint rollback, bounded backend memory, immediate lexical visibility,
active-L0/root WAL atomicity, and eventual semantic convergence. Its old helper
is explicitly legacy and will leave the installed maturity path under ARCH-3.

The next mutable-lifecycle replay exposed a second stale assumption in its
long-snapshot audit: it required a generation switch within four maintenance
calls while a repeatable-read snapshot still owned the replaced tuple. V3
correctly rotated the active L0, then returned `xid_horizon` rather than sealing
state that the old reader still needed. The active gate must therefore prove
old/new snapshot visibility and horizon deferral first, then require complete
generation, retirement, and semantic convergence only after the old snapshot is
released and VACUUM supplies the deadness boundary. A legacy shared
unified-delta-cache assertion is not part of that gate.

The same replay found a v2-only frozen-delta-XID fault probe. V3 intentionally
retains transaction identity in active/pending linked L0 until the safe horizon;
forcing every visible record to look frozen before seal contradicts CSG-I40 and
correctly fails closed. The product gate instead requires exact MVCC queryability
while the active L0 is pinned, followed by zero L0 and semantic debt after the
normal horizon-aware maintenance chain.

Semantic-completion observability also no longer exposes the retired single
chronological `frontier` object. The v3 contract is the exact COW pending count,
sealed/unsealed actionable summaries, linked-L0 bytes/records, and bounded
runtime telemetry. Mutable-lifecycle gates must consume those authorities;
capacity safety remains covered by the linked-L0 hard-frontier regression.

The mutable suite's 4,096-record decoded-delta backpressure and complete
unified-delta preload checks are also v2 contracts. V3 ingress is bounded by
active/pending linked-L0 rotation and its fixed hard frontier, while preload
warms checked relation pages and records an exact-root marker. The active suite
will retain immediate L0 visibility, hard-frontier coverage from the native
lifecycle matrix, root-snapshot race safety, and generation-replacement
prewarm, but will require zero complete unified-delta entries.

The BM25 maintenance race gate still expected the retired v2
`lightweight_delta_fold` to reject a concurrent append with `meta_changed`.
That is the wrong v3 invariant. A pending L0 is already frozen, so maintenance
must be able to seal that exact prefix while a writer appends independently to
the active L0. The replacement gate will pause the current seal path after its
checked snapshot, append one committed active-L0 record, require successful
pending publication without losing either prefix, and then require bounded
follow-up rounds to converge the remaining tail. This closes CSG-I117 without
adding another maintenance action or generation authority.

The later BM25 contract-drift gate also expected ordinary incremental
`ii42_index_maintain()` to rebuild the complete heap after changing a tokenizer
reloption. That conflicts with the frozen v3 rule that normal maintenance is
input-bounded and never hides a whole-index rebuild. Contract changes must fail
closed, then recover only through explicit `REINDEX` or refresh. The active gate
will exercise that explicit boundary and leave the incremental selector free of
an exceptional corpus scan.

The BM25-to-SAE conversion replay then reached the new definition-time guard:
its source index explicitly stored `consistency=realtime`, and the old gate
enabled SAE without replacing that policy. The rejection is correct and leaves
the relation unchanged. The conversion gate and later public migration example
must set SAE plus `consistency=eventual` atomically; the reverse conversion may
explicitly restore BM25 realtime. This is CSG-I110 contract closure, not a
compatibility fallback.

The completed mutable report also showed that its two shared readiness helpers
still defined healthy storage as the v2 contiguous `identity_map` plus
`pages>0`. Current v3 status deliberately reports COW `segment_versions`, zero
contiguous payload pages, and nonzero checked `physical_blocks` and
`reachable_blocks`; unified SAE reports its posting payload as the primary
authority. These helpers will be corrected once and reused by every CRUD,
restart, REINDEX, and conversion gate rather than weakening those gates one by
one. The same report proves that an open writer may be safely rotated into
pending L0; its transaction horizon, not a relation-wide lock, prevents unsafe
seal. CSG-I117 must assert that higher-concurrency invariant.

The current page-native replay also invalidates two remaining v2 test
contracts. An aborted multi-row statement may leave one MVCC-dead physical L0
record, just as PostgreSQL indexes may retain dead tuples until VACUUM; the
required invariants are zero visible rows, zero pending backend memory, bounded
physical debt, and later maintenance convergence, not an immediate byte-for-
byte rollback of append-only pages. Likewise, repeated same-transaction reads
now project the checked linked L0 directly and intentionally allocate no
complete transaction-local unified-delta cache. The active gate must prove
stable read-your-writes results and bounded zero index-sized backend state,
without reviving the retired decoded cache or its overflow GUC semantics.

Eventual completion is also a staged v3 action chain, not the former one-call
operation. The current evidence is active rotation, pending seal, semantic
completion, and a final generated-L0 seal. Observability must select the actual
semantic-completion phase and require final convergence rather than assuming
the first maintenance call performs inference. Preload similarly reports
checked PostgreSQL buffer-cache pages plus one exact-root marker; it does not
publish an index-sized shared image.

The same replay exposed a separate P0 correctness blocker, CSG-I119. Pending
overlay results match the independent full-overlay oracle exactly, but after
semantic completion and after explicit REINDEX the same live corpus can receive
materially different scores. The observed maximum deltas are about `54.28` in
the mixed CRUD differential and `41.57` between compaction and REINDEX in the
TID-reuse case. Membership and MVCC hiding remain correct, so this must be
isolated across document encoding, generated-posting publication, corpus
statistics, and the one native scorer. The release gate may not be weakened to
rank-only equality: a converged incremental index and REINDEX over the same
snapshot must produce the same score authority within the frozen numeric
tolerance.

### ARCH-2: Align Page-Native Runtime And Control Semantics

Confirms the closed CSG-I111 boundary while closing CSG-I112.

- [x] Define the current memory taxonomy in one place: durable authoritative
  relation state, durable exact derived folds, and volatile exact derived
  markers/residency. Label complete snapshots and unified-delta caches as
  maintenance/test or legacy-v2 machinery where they remain.
- [x] Trace `auto_rebuild_threshold`, `auto_rebuild_delta_bytes`, overlay limits,
  wait timeouts, and policy profiles from parse to every v3 call site.
- [x] For each option, choose exactly one outcome: retain with measured v3
  semantics, rename/redefine before beta, restrict to BM25/legacy tests, or
  remove. Do not retain inert compatibility knobs.
- [x] Replace cache-expansion capacity guidance with linked-L0 frontier,
  page/read amplification, worker throughput, semantic backlog age, and shared
  runtime saturation measurements.
- [x] Make status and recommendation JSON expose only actionable v3 controls
  and distinguish correctness debt from optional performance debt.
- [x] Add a static product-inventory gate that rejects stale v3 cache,
  realtime-SAE, and obsolete option claims across SQL, code, tests, and docs.

Gate: every public knob must have a focused test proving that changing it
changes the documented v3 behavior; otherwise it is removed from the product.

#### ARCH-2 Live Control Audit

The current parse-to-call-site trace gives each control one beta outcome:

| Control | Current effective path | Beta outcome |
| --- | --- | --- |
| `auto_rebuild_threshold` | Legacy generation-delta code reads it, but convergent v3 marks every nonempty active L0 due and performs one bounded rotate/seal action | Remove from the product. Making it effective would strand low-rate SAE documents or require a second age scheduler; neither is compatible with the current simple convergence contract. |
| `auto_rebuild_delta_bytes` | Legacy generation-delta code reads it; its SAE default is also incorrectly derived from shared-arena size. Convergent v3 does not use it to select work | Remove from the product. V3 capacity and safety come from physical linked-L0 frontiers, not decoded-cache or operator-derived byte thresholds. |
| `auto_rebuild_churn_ratio` | Parsed, reported, and recommended, but no v3 maintenance action reads it | Remove from reloptions, recommendations, status, SQL, tests, and current docs. |
| `query_overlay_max_records`, `query_overlay_max_bytes` | Read only by the generation-delta decoded-overlay route; convergent v3 rejects or bypasses that route | Remove from product reloptions/status. Keep any differential limit private to the ARCH-3 oracle. |
| `ii42.sae_delta_cache_wait_timeout`, `ii42.sae_delta_cache_headroom_percent` | Control v2 shared unified-delta publication and arena-pressure scheduling | Unregister from the product. Page-native v3 never waits for or sizes a complete delta cache. |
| `ii42.sae_local_delta_cache_max_records`, `ii42.sae_local_delta_cache_max_bytes` | Bound the v2 compiled transaction-local delta LRU | Unregister from the product. V3 read-your-writes uses query-bounded matching-L0 projection. |
| `ii42.shared_generation_cache_size` | Sizes the actual postmaster runtime/residency arena and BM25 compatibility cache | Retain for this beta, but redefine every current description as shared runtime/residency capacity. It owns no SAE posting authority. |

ARCH-2 is split so one failure has one cause:

1. [x] Remove inert `auto_rebuild_churn_ratio` from the public contract and prove
   build, reloption rejection, status/recommendation shape, and BM25 behavior.
2. [x] Remove v2-only query-overlay reloptions from the v3 product contract; keep
   any oracle limit private and prove page-native linked-L0 exactness is
   unchanged.
3. [x] Unregister the four v2-only SAE cache GUCs, remove their current status/docs,
   and replace active maturity gates with page-native root/L0/RSS evidence.
4. [x] Retire both `auto_rebuild_*` reloptions and replace their advertised
   backlog/compaction role with physical linked-L0 frontier and bounded-worker
   evidence. Preserve immediate low-rate SAE completion and quiescent
   convergence; do not add an aging scheduler or another maintenance state.
5. [x] Reduce status and policy recommendations to actionable v3 state, then run
   the complete ARCH-1/native/transaction/replication regression before ARCH-3.

ARCH-2 slices 4-5 implementation status (2026-08-01): the product parser,
installed SQL row types, structured status, recommendations, current
operations docs, and inventory gate now remove both `auto_rebuild_*` controls.
Legacy generation-delta code temporarily retains private fixed guards only;
they no longer depend on shared-arena sizing and are removed with that runtime
in ARCH-3. Validation and active-script fixture conversion remain open before
these two checklist items can close.

Validation follow-up: the first real-model replay reached the intended product
rejection for the retired SAE `heavy_insert_skew` profile, but its Python
fixture still expected the pre-audit `unknown SAE policy profile` text. This is
test-contract drift, not a runtime failure. Align that one assertion with the
new explicit `sae=false` scope and rerun the complete gate; do not change the
product rejection to preserve an obsolete profile taxonomy.

Removing a public field or reloption is intentional pre-beta contract closure,
not an experimental upgrade path. Historical raw evidence is left unchanged.

ARCH-2 slice 1 closure evidence:

- The reloption parser, status/recommendation row types, benchmark fixtures,
  current documentation, and active SQL tests no longer expose
  `auto_rebuild_churn_ratio`. The static product inventory rejects its return.
- The isolated staged PG18 `ii42_integration` suite passes `1/1`, including an
  explicit negative CREATE INDEX gate for the retired reloption and unchanged
  BM25 score/page/root snapshots apart from the intentional rowtype field.
- The staged real-model runtime-service smoke passes default-eventual SAE CRUD,
  bounded worker completion, policy/status inspection, restart, VACUUM, and
  REINDEX using the reduced public row types.
- The PG18 PGXS build, modified-script `py_compile`, product inventory, and
  `git diff --check` pass. No score, root byte, lock/WAL, RSS, or BM25 behavior
  change was observed.

ARCH-2 slice 2 closure evidence:

- `query_overlay_max_records` and `query_overlay_max_bytes` are absent from the
  v3 reloption parser, installed SQL row types, active benchmarks, status,
  current documentation, and positive fixtures. Two negative CREATE INDEX
  gates prove that the retired names fail before relation publication.
- The legacy generation-delta oracle retains fixed private materialization
  limits. Convergent v3 never consults them: immutable extents plus the checked
  linked-L0 frontier remain the complete lexical query authority.
- The isolated staged PG18 integration suite passes `1/1`; the default v3
  sealed-segment parity gate passes `80/80`; and the staged real-model runtime
  smoke passes lexical-first SAE CRUD, completion, VACUUM, restart, and REINDEX.
  Golden score, page, root-byte, and maintenance outputs are unchanged apart
  from the intentional two-field status contraction.
- PGXS build, modified-script `py_compile`, product inventory, and
  `git diff --check` pass.

ARCH-2 slice 3 closure evidence:

- The four v2-only SAE decoded-cache GUCs are no longer registered or exported
  by product C code. Installed structured status and current operator guidance
  no longer expose decoded-delta pressure, headroom, wait, or backend-cache
  tuning as v3 controls. The product inventory rejects their return.
- The active maturity runner now exercises page-native transaction memory and
  RSS behavior instead of decoded-cache capacity/pressure scripts. A staged
  real-model mutable lifecycle passes `71/71`: 100 repeated queries keep the
  root stable, grow backend-accounted memory by only 217,088 bytes and RSS by
  2,016 KiB, and observe linked-L0 records `1 -> 2` across intervening DML.
- The staged 24-index transaction/RSS gate preserves exact query/oracle parity,
  savepoint, late-failure, and 2PC cleanup with 3,280 KiB RSS growth against a
  98,304 KiB cap and zero pending cache contexts after transaction end.
- The isolated staged PG18 integration suite passes `1/1`, including explicit
  absence of all four retired GUCs. Native v3 read parity passes `80/80` and
  transaction, 2PC, crash/restart, and reclamation lifecycle passes `21/21`.
- The remaining unified-delta pressure calculation is confined to the legacy
  non-v3 oracle/scheduler branch. It is neither parsed by structured product
  status nor reachable from convergent v3 maintenance, and will be removed
  with that entire runtime route under ARCH-3 rather than mixed into this
  contract-only slice.

ARCH-2 slice 4 audit decision:

- Both `auto_rebuild_*` values are read by the generation-delta branch, status,
  recommendation code, and legacy backlog admission. The convergent v3 branch
  exits before those call sites: any nonempty active L0 is a coalesced worker
  candidate, and each worker transaction performs one bounded rotate, seal,
  semantic-completion, compaction, fold, or reclamation action.
- SAE completion cannot wait for a configurable record or byte threshold. A
  newly inserted low-rate document first has to rotate and seal before its
  document-COW entry can be encoded; semantic transitions then have to rotate
  and seal before they become searchable. Thresholding either boundary can
  prevent a quiescent index from ever converging.
- Making the controls effective without that failure would require durable L0
  age in the root or a second restart-sensitive aging scheduler. That is a
  larger and less reliable design than the existing coalesced work-hint and
  periodic-reconciliation path, and is rejected for beta.
- The page-native product will therefore retire both controls rather than
  merely replacing the shared-arena-derived SAE default. Status and tests will
  use active/pending L0 records, bytes, pages, worker progress, semantic age,
  and the fixed physical hard frontier as the actionable capacity contract.

CSG-I120 diagnosis update (2026-08-01):

- The failure reproduces at the minimum `1 writer x 1 reader x 1 CRUD cycle`,
  so it is not an 8x8 saturation artifact. After one INSERT/UPDATE/DELETE,
  concurrent explicit maintenance and VACUUM leave a pending frontier whose
  effective events have already been incorporated by the document-COW
  authority. Final sealing currently aborts with `empty ii42 pending segment`.
- This is a missing legal representation, not permission to ignore records.
  The manifest format already defines a checked empty `HISTORY_BARRIER` for a
  sequence range whose document/posting history has been absorbed by COW.
  Pending sealing must use that same representation only when its computed
  lexical, semantic, and retirement payload is empty; it must still consume
  the exact frozen pending frontier through the normal checked publication.
- First repair slice: emit the existing history-barrier descriptor instead of
  rejecting this no-op frontier. Then rerun the 1x1 case before diagnosing any
  remaining retirement-target or document-patch failure. No lock, WAL, root,
  scorer, or maintenance-selector change belongs in this slice.
- The narrow repair now emits a sealed empty `HISTORY_BARRIER` only when the
  exact frozen pending frontier produces no lexical, semantic, or retirement
  payload after COW/VACUUM absorption. The normal checked manifest publication
  still advances that frontier; no root format, lock order, WAL protocol,
  scorer, or action-selection rule changed.
- Staged PG18 replay passes at `1x1x1`, `2x2x3`, and `4x4x6`. Two independent
  full `8 writers x 8 readers x 12 CRUD cycles` runs each completed `288/288`
  foreground mutations while readers, semantic completion, maintenance, and
  VACUUM remained active. Both converged to zero L0/semantic debt with exact
  native/oracle rows, zero runtime failures, zero busy rejections, and a
  passing 24-cycle fixed-live storage plateau. CSG-I120 remains open until the
  same staged binary also passes the focused VACUUM-frontier, native, SAE,
  transaction, replication, and SQL-regression closure.

ARCH-2 slices 4-5 and CSG-I120 closure evidence:

- The staged package at `/tmp/ii42-arch2-stage-0801e` passes core `1/1`,
  isolated SQL regression `1/1`, native v3 `80/80`, the 140,000-row bounded
  VACUUM frontier `10/10`, real-model default/eventual SAE lifecycle, and
  transaction/savepoint/2PC/crash-restart `21/21`.
- Physical replication passes linked-L0, semantic completion, prepared
  transactions, CRUD, REINDEX, and BM25/SAE layout conversion on primary and
  standby. Two independent full `8x8x12` concurrency runs pass all `288/288`
  mutations, exact native/oracle results, quiescent convergence, and fixed-live
  storage plateau with no runtime failures or busy rejections.
- Product reloptions, installed row types, status/recommendation JSON, active
  scripts, and current operations documentation expose no retired rebuild or
  decoded-cache controls. Modified-script `py_compile`, the product inventory,
  and `git diff --check` pass. The only remaining fixed legacy guards are
  private to the generation-delta oracle and leave with that route in ARCH-3.

### ARCH-3: Isolate The Legacy Oracle

Closes CSG-I113 after ARCH-1 and ARCH-2 establish the sole product contract.

- [x] Freeze independent golden fixtures for numeric/text BM25, semantic
  scoring, MVCC replacement, zero-score order, prefix/phrase/boolean queries,
  and representative linked-L0 overlays.
- [x] Make installed SQL dispatch enter the v3 page-native executor directly;
  storage authority is read once in C rather than selected by PL/pgSQL status
  JSON and dynamic function names.
- [x] Move generation-delta v2 construction/scoring behind a test-only build or
  standalone oracle fixture. It must not ship as product query, mutation,
  maintenance, migration, or fallback behavior.
- [x] Remove v2-only reloptions, recommendations, GUC descriptions, status
  fields, error hints, and product tests after golden parity is established.
- [x] Keep only the supported `psql_bm25s` to II-42 migration boundary and
  explicit v3 `REINDEX`; do not preserve experimental II-42 upgrade chains.

Gate: a production build contains one installed query/mutation lifecycle, while
the independent oracle can still detect score and ordering regressions.

#### ARCH-3 Live Inventory And Slices

The source audit separates three concepts that previously shared legacy names:

- The installed `ii42_query_semantic_internal` reads generation-status JSON
  and dynamically chooses between two SQL-visible C scorers. This is a real
  product authority split and must be removed.
- `ii42.test_legacy_v2_build` can still create generation-delta storage in a
  normal product binary. Its writer, reader, query, maintenance, preload, and
  telemetry branches are experimental-v2 product code, not a supported
  migration boundary.
- The current v3 full-overlay test mode and linked-L0 record decoder are not
  legacy storage. They independently recompute the checked v3 snapshot and
  remain required correctness oracles. Their current `unified_delta` names
  must not justify retaining the decoded generation-delta cache or a second
  lifecycle.

ARCH-3 proceeds in independently reviewable contract slices:

1. [x] Freeze checked, repository-owned v3 golden fixtures for numeric and text
   BM25, model-backed scoring, MVCC replacement, deterministic zero-score
   ordering, prefix/phrase/boolean queries, and linked-L0 overlays. The fixture
   must compare exact IDs/order and bounded float scores without invoking v2.
2. [x] Replace semantic SQL dynamic dispatch with one direct call to the native
   scorer. The native C entry reads and validates the checked v3 metapage/root;
   SQL must not infer storage authority from status JSON or function names.
3. [x] Remove the installed legacy scorer and `ii42.test_legacy_v2_build`.
   Convert still-relevant lifecycle tests to v3 or the frozen fixture; archive
   only evidence that exists solely to prove unsupported-v2 compatibility.
4. [x] Delete generation-delta build/query/mutation/maintenance/preload and
   decoded-cache telemetry from the product binary. Preserve only typed v3
   linked-L0 decoding, query-bounded overlay scratch, and the full-overlay
   oracle, with names and interfaces that describe those authorities.
5. [x] Prove that installed SQL, exported symbols, GUCs, status, preload, tests,
   and current docs contain no v2 runtime fallback. Retain only the supported
   `psql_bm25s` migration input and explicit v3 `REINDEX` behavior.

Stop and diagnose if any slice changes result IDs/order, score tolerance, root
bytes, lock/WAL order, backend RSS, shared-runtime ownership, or the BM25
baseline. Do not hide a product behavior change inside v2 deletion or the
later ARCH-4 mechanical extraction.

ARCH-3 slice 4w v3 rebuild-admission contract:

- The absence audit found a current v3 safety defect, not another legacy name.
  A valid v3 metapage requires the retired `tid_bytes_len`,
  `index_bytes_len`, `delta_bytes_len`, and `semantic_bytes_len` fields to be
  zero, but rebuild-memory admission still reads those fields. Existing v3
  `REINDEX` and status can therefore estimate a live BM25 payload as zero and
  omit the document-map and linked-L0 portions of an SAE rebuild.
- Replace the metapage-shaped estimator input with a typed workload containing
  reachable live object bytes, document identity-source bytes, and current
  linked-L0 mutation bytes. Existing v3 indexes derive those values from the
  checked root/manifest, `num_docs`, and linked-L0 debt. A first `CREATE INDEX`
  retains the current conservative heap-size proxy because no old root exists.
- Preserve all estimate multipliers, payload caps, headroom thresholds,
  explicit-build fallback behavior, and builder implementations. Do not write
  derived estimates into the root and do not introduce a second accounting
  authority. Overflow must remain fail-safe through the existing saturating
  estimate helpers.
- Add focused staged evidence that a nonempty v3 BM25 index reports nonzero
  standard/compact/spill estimates and selects the intended low-memory builder,
  while SAE reports a semantic-stream estimate that includes document identity
  and linked-L0 debt. Then rerun the frozen exactness, lifecycle, recovery,
  replication, RSS, storage, package, and inventory gates before committing.

This contract correction is a standalone commit. It must close before further
generation-delta deletion or any mechanical `ii42_am.c` extraction, so ARCH-4
cannot accidentally preserve an estimator whose input authority is already
invalid under v3.

ARCH-3 slice 1 closure evidence:

- `tests/fixtures/page_native_golden.json` is a checked, versioned fixture;
  `scripts/test_page_native_golden.py` constructs only convergent-v3
  indexes in an isolated preloaded PG18 cluster. It does not set the legacy-v2
  build hook or create generation-delta storage.
- The fixture freezes IDs, ordering, and float scores for seven initial
  text/numeric BM25 surfaces, six prefix/phrase/boolean predicates, seven
  active linked-L0 surfaces including deterministic zero-score completion,
  four UPDATE/DELETE/VACUUM MVCC surfaces, and five model-backed surfaces.
- Semantic evidence is pinned to manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`; a different
  checkout fails rather than silently redefining the golden.
- Two independent runs against the current staged PG18 package pass every
  surface with exact IDs/order and `2e-6` relative/absolute score tolerance:
  `/tmp/ii42-arch3-page-native-golden-current-run3.json` and
  `/tmp/ii42-arch3-page-native-golden-current-run4.json`.

ARCH-3 slice 2 active diagnosis:

- Direct native dispatch passes the isolated SQL regression and the independent
  page-native golden fixture. The only initial failure was a stale privilege
  assertion that expected `ii42.test_unified_overlay_oracle` to redirect product
  search into the legacy generation-payload scorer.
- The GUC is consumed only by that legacy scorer. The page-native scorer has no
  oracle branch, so restoring the old error would recreate test-controlled
  product dispatch. The replacement privilege gate must prove that an
  application role cannot execute the native scorer directly and that setting
  the test GUC leaves ordered product IDs and scores unchanged.
- Reaching the remainder of the privilege fixture exposed an unrelated stale
  transaction shape: it invoked refresh and maintenance in one `DO` block even
  though refresh modifies the same index. Owner capability checks must use
  separate transactions and preserve the writer/maintenance exclusion rule;
  that rule must not be weakened to make an ACL fixture pass.

ARCH-3 slice 2 closure evidence:

- Installed semantic search now calls
  `ii42_index_semantic_query_native_internal` directly. PL/pgSQL no longer reads
  generation-status JSON or constructs a scorer function name; the native C
  entry validates the checked metapage storage and runtime signature.
- `/tmp/ii42-arch3-direct-dispatch-regression-rerun` passes the isolated SQL
  regression `1/1`. `/tmp/ii42-arch3-direct-dispatch-golden-rerun.json` passes
  every frozen BM25, predicate, linked-L0, MVCC, and semantic surface.
- The staged runtime-service privilege smoke passes with the native scorer
  explicitly denied to the application role and with identical ordered product
  results before and after an attempted test-oracle GUC enable. Owner refresh,
  maintenance, and try-maintenance are verified in separate transactions.
- The full staged runtime-service temporary-PG smoke passes lexical-first CRUD,
  semantic completion, restart-sensitive runtime ownership, and native query
  behavior. Modified-script `py_compile`, the product inventory, and
  `git diff --check` pass.

ARCH-3 slice 3 execution split:

1. Remove the SQL-visible legacy semantic scorer and move every current ACL,
   schema, RLS, and model-lifecycle diagnostic to the native scorer. Keep the C
   implementation temporarily unreachable so this installed-contract change is
   not mixed with mechanical legacy-code deletion.
2. Remove `ii42.test_legacy_v2_build` and make CREATE INDEX/REINDEX publication
   unconditionally page-native. Convert current v3 lifecycle and performance
   tests to a page-native baseline; remove legacy decoded-cache/waiter programs
   from executable product validation while retaining checked historical reports.
3. Only after both contract slices pass, delete the unreachable scorer and
   generation-delta implementation under slice 4. No current benchmark may use
   an unsupported-v2 index as its correctness or performance baseline.

The installed-scorer replay reached a stale active maturity assertion in
`test_model_lifecycle_medium_perf.py`: it still required v2 `identity_map`
pages and assumed one maintenance call completed an eventual SAE mutation.
The replacement gate must run the staged package, execute one bounded action per
transaction until a fixed round limit, and require a checked
`convergent_segments` root with positive reachable/physical blocks and zero
linked-L0 and semantic debt. It must not turn one maintenance call into an
unbounded operation.

Removing the legacy build fixture also exposed a scheduler-timing assumption in
the convergent lifecycle smoke: restart was required to have already rotated an
active L0 into pending. Restart correctness is frontier preservation, not worker
selection timing. The gate must accept either an exact unchanged frontier or one
exact bounded active-to-pending rotation, while requiring identical aggregate
bytes, pages, records, and query results.

The old-snapshot fence had the same ownership race: a background worker could
perform the exact active-to-pending rotation before the explicit probe, leaving
that probe to report `xid_horizon`. The gate must require a nonempty pending
frontier and accept either the observed rotate result or an already-rotated
`xid_horizon` result; sealing must still remain blocked until the old snapshot
releases its horizon.

ARCH-3 slice 3 installed-scorer closure evidence:

- Installed SQL no longer creates or grants any legacy generation-payload
  scorer. Schema-placement, privilege, RLS, and medium-lifecycle diagnostics all
  use the one native scorer; the product inventory rejects reinstallation of the
  legacy function.
- The staged package passes isolated SQL regression `1/1`, non-public schema
  placement, application-role privilege isolation, and every independent golden
  surface. The native scorer remains internal and test-oracle GUC requests do not
  alter product results.
- `/tmp/ii42-arch3-native-medium-lifecycle-v3.json` passes a 1,000-document
  model-backed CRUD lifecycle. Two independent bounded maintenance transactions
  converge the mutation, linked-L0 and semantic debt reach zero, native query
  diagnostics return 20 joined rows, explicit REINDEX passes, and drop leaves no
  sidecar relation.
- The old C scorer remains deliberately unreachable in this contract commit. It
  leaves with generation-delta implementation deletion, not through a mixed SQL
  contract/mechanical-code change.

ARCH-3 slice 3 legacy-build-hook closure evidence:

- CREATE INDEX, `REINDEX`, and `ambuildempty` now unconditionally select the
  page-native builder. The product binary no longer recognizes
  `ii42.test_legacy_v2_build`, and current product documentation no longer
  advertises generation-delta builders or decoded-cache telemetry.
- Legacy-only decoded-cache, generation-waiter, and preload-rollout executables
  were removed from current validation. Checked historical research reports
  remain evidence, not runnable product qualification.
- `/tmp/ii42-arch3-no-legacy-build-native-smoke-v3.json` passes all `80/80`
  page-native lifecycle gates, including exact reference parity, linked-L0
  mutation/restart behavior, old-snapshot fences, selective compaction,
  workload folding, VACUUM retirement, and explicit `REINDEX`.
- A second independent replay at
  `/tmp/ii42-arch3-no-legacy-build-native-smoke-v3-rerun.json` also passes
  `80/80`, ruling out a one-run scheduler-timing success.
- The same staged package passes the independent semantic golden fixture at
  `/tmp/ii42-arch3-no-legacy-build-golden.json`, isolated SQL regression `1/1`,
  application-role privilege isolation, and the runtime-service temporary-PG
  lifecycle.
- The smoke accepts only an identical restart frontier or one exact bounded
  active-to-pending rotation with unchanged aggregate bytes/pages/records. Its
  old-snapshot gate likewise accepts an already-completed exact rotation while
  still requiring horizon-delayed sealing. These changes remove scheduler
  timing assumptions without weakening root or query exactness.

ARCH-3 slice 3 mechanical naming cleanup:

- Rename the second current page-native fixture from `v2` to `reference` in the
  lifecycle smoke; rename the benchmark's former `materialized` control to
  `static_reference`. SQL statements, reloptions, mutations, maintenance calls,
  query comparisons, and thresholds must remain byte-for-byte equivalent apart
  from fixture identifiers and emitted diagnostic labels.
- This naming-only slice is committed separately from the build-contract
  removal. It must pass the same `80/80` lifecycle replay twice before closure.

The first renamed query-state replay preserved exact rows/scores and passed
every latency/QPS gate, but exposed an invalid diagnostic assertion. The C
selector admits a workload fold when either byte-only heat covers rewrite cost
or an impact-ready candidate's measured unpruned page work covers it. The old
benchmark instead required successful folds to have root heat below the
byte-only threshold, rejecting the valid byte-only case (`494` observed versus
`80` required). Replace that assertion with an exact replay of the selector's
two cost bases; do not alter the C selector or its thresholds.

ARCH-3 slice 3 mechanical naming closure evidence:

- Two independent runs of the renamed lifecycle smoke pass all `80/80` gates:
  `/tmp/ii42-arch3-reference-naming-smoke-final1.json` and
  `/tmp/ii42-arch3-reference-naming-smoke-final2.json`. Both fixtures are
  current page-native indexes; `reference` denotes the independent comparison
  surface and no longer implies an experimental storage generation.
- `/tmp/ii42-arch3-static-reference-query-states-final.json` passes every
  exactness, maintenance, latency, QPS, hot-fold residency, and cost-basis
  gate on 40,000 documents. Fragmented, folded, and impact-specialized scores
  match the static reference exactly with maximum absolute difference `0.0`.
- Folded/static mean, p50, p95, p99, and QPS ratios are `0.9974x`, `0.9932x`,
  `1.0043x`, `0.9982x`, and `1.0026x`. The workload fold satisfies both the
  existing byte-only condition (`80` required root heat) and the measured
  impact-ready work condition (`2,549,514,240` observed page bytes). No product
  selector, threshold, score, storage, lock, or WAL behavior changed.
- The product inventory now rejects retired `v2` lifecycle-fixture naming and
  retired `materialized` benchmark-control naming. Modified-script
  `py_compile`, the inventory, and `git diff --check` pass before commit.

ARCH-3 slice 4 deletion boundary:

1. Remove the unreachable SQL scorer symbol and its query-only static closure
   from `ii42_semantic.c`. A Clang call-graph audit identifies 157 transitive
   scorer dependencies: 109 are temporarily shared with decoded-cache preload,
   while 48 are query-only. This first deletion may remove only the 48-function
   query closure and scorer-owned types; it must leave preload behavior and all
   linked-L0 authority unchanged.
2. Remove decoded-cache product authority in two commits:
   - first remove its scheduler action, targeted background-worker mode,
     preload branch, installed status/SQL fields, and current validation
     claims while leaving the implementation unreachable;
   - then mechanically remove `SaeUnifiedDeltaCache`, backend-local decoded
     snapshots, shared decoded-payload publication/waiting, decoded-cache
     wait APIs, cache-only shared-registry kind, telemetry storage, and header
     APIs.
   Page-native exact-root markers, PostgreSQL buffer-cache prewarm, lexical
   delta cache, tombstones, unified-warm markers, and HOT_FOLD residency remain;
   none owns a decoded index snapshot.
3. Remove generation-delta construction, incremental replacement, legacy
   semantic payload compaction, and storage dispatch from `ii42_am.c`. Preserve
   linked-L0 append/load/snapshot/visibility under names that state their v3
   ownership, then remove obsolete semantic-reader and generation-payload
   writer interfaces from `ii42_semantic.h`.
4. Remove the corresponding installed telemetry/status fields and invert the
   product inventory so a product build cannot regain any of these roots.

Each deletion is a separate commit. After every commit, the dynamic-symbol
inventory, core build/regression, independent golden, native lifecycle, and
product inventory must pass. The first unexpected undefined symbol or result,
root, lock/WAL, RSS, or BM25 change stops the deletion and reclassifies that
symbol rather than broadening the patch.

ARCH-3 slice 4a scorer-closure evidence:

- The installed scorer root and exactly 48 query-only static dependencies were
  removed from `ii42_semantic.c`, together with seven scorer-owned types. The
  deletion was compiler-driven in four leaf rounds (`8`, `22`, `15`, then `3`
  unused functions); the final PGXS compile has no warning. Minimal diff is a
  pure 3,895-line deletion after removing vacated blank lines and does not
  modify decoded-cache preload,
  generation-delta storage, linked-L0 authority, SQL, locks, WAL, or scoring.
- The current and staged dynamic-symbol inventories contain only
  `ii42_index_semantic_query_native_internal` and its `pg_finfo` symbol. The
  product inventory now rejects the retired scorer name in AM, semantic source,
  semantic header, or installed SQL.
- The staged package at
  `/tmp/ii42-arch3-scorer-prune-stage.eXmS4s` passes core CTest `1/1`, isolated
  extension regression `1/1`, independent page-native golden parity at
  `/tmp/ii42-arch3-scorer-prune-golden-final.json`, and native lifecycle
  `80/80` at `/tmp/ii42-arch3-scorer-prune-native80-final.json`. The golden
  reuses the checked
  manifest digest `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- Modified-script `py_compile`, PG18 PGXS build, product inventory, staged
  dependency inspection, and `git diff --check` pass. Slice 4b remains the
  decoded-cache/preload authority removal; this commit makes no claim about
  that still-present implementation.

ARCH-3 slice 4b decoded-cache authority inventory:

- The decoded cache no longer serves the installed scorer after slice 4a, but
  four product-control branches still name it: `unified_delta_only` targeted
  workers, `unified_delta_publish_due` maintenance classification, legacy
  preload branches reached only by unsupported non-convergent storage, and
  installed status/SQL telemetry. Those branches must leave before the cache
  implementation so contract and mechanical deletion remain reviewable.
- `II42_AM_SHARED_GENERATION_UNIFIED_DELTA_CACHE`, oversized identities,
  incremental-chain publication counters, and the backend-local decoded
  snapshot cache are cache-only state. The generic generation-wait CV stripes
  and counters are not: current base auto-preload uses them while a foreground
  query waits for an ordinary shared generation. Slice 4b removes only the
  decoded-cache wait APIs and preserves that current base-wait owner. None of
  the cache-only state is used by page-native root markers, lexical mini-index
  residency, tombstone cache, unified-warm state, HOT_FOLD, model-runtime
  ownership, or linked-L0 storage.
- `ii42_am_meta_uses_generation_delta_storage()` and the generation-payload
  reader/writer remain a separate slice 4c authority. Removing them in 4b would
  mix cache retirement with storage-format dispatch and obscure regressions.
- Slice 4b-contract removes cache scheduling and public observability while
  leaving an unreachable implementation. Slice 4b-mechanical then removes only
  the proven cache closure, driven by compiler warnings and exact call-site
  inventory. Both commits must pass the staged golden and native lifecycle
  gates before slice 4c starts.

ARCH-3 slice 4b-contract closure evidence:

- Targeted decoded-cache workers, cache-only maintenance classification,
  decoded preload/publication branches, and decoded-cache status/SQL fields
  are absent from the product contract. Background work now uses the ordinary
  preload/maintenance selector; exact-root markers, PostgreSQL buffer-cache
  prewarm, lexical delta cache, tombstones, unified-warm state, HOT_FOLD, and
  generic generation waits remain unchanged.
- Current product smokes no longer consume decoded-cache telemetry. Physical
  replication instead asserts that those JSON keys are absent, while mutable
  lifecycle and standby preload gate page-native root residency directly. The
  product inventory rejects reintroduction of the retired installed fields.
- The staged PG18 package at
  `/tmp/ii42-arch3-4b-contract-stage.nm7okI` passes core CTest `1/1`, isolated
  extension regression `1/1`, independent golden parity at
  `/tmp/ii42-arch3-4b-contract-golden.json`, native lifecycle `80/80` at
  `/tmp/ii42-arch3-4b-contract-native80.json`, SAE lifecycle `15/15` at
  `/tmp/ii42-arch3-4b-contract-sae-lifecycle.json`, and mutable lifecycle
  `71/71` at `/tmp/ii42-arch3-4b-contract-mutable-lifecycle.json`.
- Standby exact-root auto-preload passes at
  `/tmp/ii42-arch3-4b-contract-standby-preload.json`; full physical replication
  passes at `/tmp/ii42-arch3-4b-contract-replication.json`. Current and staged
  dynamic-symbol inventories still expose only the native semantic scorer.
  PG18 PGXS, modified-script `py_compile`, product inventory, and
  `git diff --check` pass. Slice 4b-mechanical is next and may delete only the
  now-unreachable decoded-cache implementation; generation-delta storage stays
  reserved for slice 4c.

ARCH-3 slice 4b-mechanical deletion boundary:

- A fresh Clang call graph over the current source identifies the decoded
  semantic closure at `sae_semantic_reader_read_raw_tid` through end of
  `ii42_semantic.c`: 54 definitions plus cache-owned types and declarations.
  The apparent inbound edge to `sae_unified_delta_document_at` originates from
  that same unreachable helper. Earlier resident-reader primitives remain
  live through the page-native document cursor and are explicitly excluded.
- In `ii42_am.c`, remove only decoded-payload APIs, the cache-only registry
  kind, oversized-identity admission state, chain-publication telemetry, and
  obsolete local-cache invalidation calls. Preserve ordinary generation wait
  stripes/counters, exact-root prewarm markers, PostgreSQL buffer-cache
  prewarming, lexical/tombstone/HOT_FOLD residency, linked-L0 authority, and
  generation-delta storage interfaces reserved for slice 4c.
- Deletion is compiler-driven: remove roots first, rebuild with explicit
  unused-function and unused-variable diagnostics, and prune only newly
  unreachable leaves. Any unexpected caller reclassifies the symbol instead
  of broadening the deletion.

ARCH-3 slice 4b validation correction:

- The shared-registry capacity benchmark still required the retired
  `tier=shared_preload` generation payload even though 4b-contract had already
  made page-native prewarm plus one exact-root marker authoritative. The same
  assertion fails against the untouched 4b-contract staged binary, so it is a
  stale test contract rather than a 4b-mechanical regression.
- The benchmark now keeps its original 1K/10K/65K occupancy,
  eviction/clear/reload, exact-result, and 1/64-client pressure measurements,
  but verifies `tier=postgres_buffer_cache` and exactly one real unified-warm
  marker above synthetic occupancy. Its raw-state parser is local rather than
  imported from the decoded-cache test that 4b removes.
- A bounded replay against the 4b-contract binary passes exact query results,
  one real marker, nine forced evictions, clear-to-zero, reload-to-one, and
  invalid-capacity startup rejection at
  `/tmp/ii42-arch3-4b-contract-registry-baseline-fixed.json`. This test-only
  correction is committed before the mechanical product deletion; the full
  capacity gate must run again against the new staged binary.

ARCH-3 slice 4b-mechanical closure evidence:

- The compiler-driven deletion removes all 54 decoded semantic-cache
  definitions, `SaeUnifiedDeltaCache`, backend-local decoded snapshots, shared
  decoded-payload publication/resolution, decoded-cache-only waits and registry
  state, oversized-identity admission, and decoded-cache telemetry. Six
  resident-reader leaves that became unreachable with that closure are also
  removed; the page-native document/posting cursor leaves remain live.
- `ii42_am.c` no longer owns decoded-cache scheduling, pressure, pinning,
  invalidation, publication, or telemetry. Shared-preload ABI version 25 removes
  the dead shared-memory layout while retaining generic generation CV waits,
  exact-root `UNIFIED_WARM`, `HOT_FOLD`, lexical delta and tombstone residency,
  linked-L0 authority, and PostgreSQL buffer prewarm. Generation-delta storage
  read/write dispatch remains intentionally isolated for slice 4c.
- Four cache-only runnable programs are removed. Product inventory now rejects
  their return and every retired decoded-cache runtime symbol while retaining
  negative absence checks in replication. The staged dynamic-symbol inventory
  exposes only `ii42_index_semantic_query_native_internal` and its `pg_finfo`
  symbol; no decoded-cache string or old semantic scorer remains.
- The final PG18 package at
  `/tmp/ii42-arch3-4b-mechanical-stage.vAe9dg` passes core CTest `1/1`, isolated
  SQL regression `1/1`, independent golden parity at
  `/tmp/ii42-arch3-4b-mechanical-golden-final.json`, native lifecycle `80/80`
  at `/tmp/ii42-arch3-4b-mechanical-native80-final.json`, SAE lifecycle `15/15`
  at `/tmp/ii42-arch3-4b-mechanical-sae-lifecycle-final.json`, and mutable
  lifecycle `71/71` at
  `/tmp/ii42-arch3-4b-mechanical-mutable-lifecycle-final.json`.
- Standby exact-root auto-preload passes at
  `/tmp/ii42-arch3-4b-mechanical-standby-preload-final.json`; physical
  replication, including 2PC, linked-L0, maintained CRUD, REINDEX, layout
  transitions, and drop replay, passes at
  `/tmp/ii42-arch3-4b-mechanical-replication-final.json`.
- The final 1K/10K/65K registry gate passes at
  `/tmp/ii42-arch3-4b-mechanical-shared-registry-final.json`: the 65K/1K
  single-client p95 ratio is `1.0584`, 64-client throughput scale is `4.4394`,
  and the 65K/1K 64-client QPS ratio is `0.873`. Exact results, one real root
  marker, eviction/clear/reload, and invalid startup remain covered.
- PostgreSQL resolves version SQL from its installed share directory even when
  the primary control file and dylib are staged. The final run therefore first
  verifies source, staged, and installed SQL/control/dylib SHA-256 identity;
  `ARCH-5` must preserve this binding check so a stale system SQL file cannot
  masquerade as a staged-package product failure or success.

ARCH-3 slice 4c generation-delta retirement boundary:

- Current build selection is already one-way: normal `ambuild` and
  `ambuildempty` set `convergent_segment_build=true`, and the legacy-v2 test
  creator left the binary in slice 3. No reachable product creator can publish
  storage version 2. The remaining code is compatibility fallback, not a
  current construction path.
- Unsupported generation-delta fallback remains reachable from `aminsert`,
  VACUUM, ordinary maintenance, preload/status, AM scans, and owner-only query
  diagnostics. It also retains old generation writers, incremental replacement,
  physical delta append/overlay/fold, payload readers, and DSM/backend cache
  machinery. Keeping those branches would violate the one-lifecycle contract
  even though fresh indexes never select them.
- Not every decoded structure is legacy. Current v3 weighted diagnostics use a
  checked sealed segment snapshot, and v3 maintenance/full-overlay tests use
  typed linked-L0 decoders. Those authorities remain until their later ARCH-4
  extraction; names alone are not deletion evidence.
- Slice 4c-contract adds one fail-closed storage-layout guard to every installed
  query, mutation, VACUUM, maintenance, status, and preload boundary. A
  physical-metapage fixture must prove storage version 2 is rejected with an
  explicit `REINDEX` hint. The build path remains exempt so explicit `REINDEX`
  can replace an unsupported experimental index with v3.
- After the contract commit, regenerate the Clang call graph and remove only
  newly unreachable closures in independent mechanical commits: legacy
  build/publication, legacy mutation/maintenance/overlay, then legacy
  payload/cache/query support. Rebuild after each root deletion and reclassify
  any caller rather than widening the patch.
- Every sub-slice must preserve v3 root bytes, linked-L0 and semantic completion,
  exact IDs/order/scores, WAL/lock order, backend/shared-memory ownership, BM25
  disabled-SAE performance, and the independent golden oracle. No ordinary
  maintenance route may gain a heap scan or full-index rebuild.

ARCH-3 slice 4c-contract closure evidence:

- One `ii42_am_require_convergent_segment_storage()` guard now rejects retired
  storage at installed query, direct-search, mutation, VACUUM, ordinary
  maintenance, status, policy, signature, and preload boundaries with SQLSTATE
  `0A000` and one `REINDEX` recovery hint. Explicit `REINDEX` and
  `ii42_index_refresh` remain outside that guard and continue to publish v3.
- Background maintenance and auto-preload selectors skip unsupported storage
  without retrying or logging an error. They do not convert the retired layout,
  scan the heap, or create an alternate maintenance lifecycle.
- `scripts/test_storage_layout_boundary.py` uses no SQL construction hook. It
  stops a checksum-disabled temporary PG18 cluster, first asserts storage
  version 3 at physical metapage offset 96, changes only that field to version
  2, and verifies 12 installed boundaries reject it. The scheduler returns zero
  candidates; explicit `REINDEX` republishes physical version 3, after which a
  top-10 query and INSERT both succeed. Structured evidence is
  `/tmp/ii42-arch3-4c-contract-storage-boundary-final.json`.
- The storage fixture, isolated SQL regression (`1/1`), warning-clean PGXS
  build, product inventory, `py_compile`, and `git diff --check` pass. A staged
  page-native lifecycle rerun passes all `80/80` gates at
  `/tmp/ii42-arch3-4c-contract-convergent-v3-rerun.json`. Its first attempt hit
  the existing fixed-churn safe-XID timing window for 32 rounds and the rerun
  converged; ARCH-5 must continue treating repeated horizon exhaustion as a
  failure rather than weakening the gate.
- Source, staged, and installed SQL/control/dylib identities match. The dylib
  SHA-256 is `0b5a7dbbe8170f97b081ab3200677d4693db585c6e52d1b5bd9e476a69c52f30`;
  the SQL and control identities remain the slice-4b values.
- The payload-corruption smoke reached the correct current v3 fail-fast error
  (`invalid ii42 convergent segment payload`, reason
  `segment_root_out_of_bounds`) but still expected a retired v2 error string.
  Correct that stale test assertion in an independent test-only commit before
  mechanical generation-delta deletion; product behavior does not change.
- The old implementation is intentionally still present in this contract
  commit. Regenerate reachability from this fail-closed boundary before each
  4c mechanical deletion, so current v3 sealed-snapshot diagnostics and typed
  linked-L0 oracles are not removed by name-based matching.

ARCH-3 slice 4c qualification correction:

- `scripts/test_payload_health_corruption_smoke.py` now asserts the current v3
  fail-fast error and exact `segment_root_out_of_bounds` health reason rather
  than the retired decoded-payload error. It also uses explicit
  `ii42_index_refresh` for corruption recovery instead of requiring ordinary
  maintenance to perform a forbidden whole-index rebuild. The
  corruption/refresh scenario passes end to end; this is a test-only correction
  and changes no runtime contract.

ARCH-3 slice 4c-mechanical-1 build/publication boundary:

- `ambuild` and `ambuildempty` already select convergent v3 unconditionally,
  but both retain dead generation-delta publication branches. Remove those
  branches and make `ii42_am_build_common()` express the one-way v3 contract
  directly; do not change replacement construction, v3 root bytes, or the
  current builder/memory-budget policy.
- Explicit `REINDEX` and `ii42_index_refresh` are recovery operations, not
  ordinary maintenance. They must always rescan the heap and publish v3,
  including when the input metapage is unsupported or corrupt. Remove the
  legacy incremental-compaction dispatch from this recovery boundary; never
  replace it with a maintenance-triggered whole-index rebuild.
- Delete only build-owned leaves made unused by those dispatch changes. Legacy
  online-maintenance staging/publication still belongs to the next mechanical
  slice, so shared generation writers remain until that caller is removed.
- Stop on any root-byte, score/order, lock/WAL, BM25 baseline, or lifecycle
  change. Validate the storage-layout recovery fixture, corruption/refresh,
  independent golden, native lifecycle, and warning-clean PG18 build before
  committing this slice independently.

ARCH-3 slice 4c-mechanical-1 closure evidence:

- `ii42_am_build_common()` and `ambuildempty()` now have one explicit v3
  publication path. The dead empty-fork generation payload writer and the
  unused truncate-and-publish v2 wrapper are deleted. Replacement building,
  builder selection, memory budgeting, generation-barrier ownership, and the
  page-native publisher are unchanged.
- `ii42_am_reindex_relation()` no longer derives a storage route from the old
  metapage or builds a generation-delta replacement. Explicit refresh and every
  REINDEX caller rescan the heap and publish v3. Its inventory gate now checks
  the remaining longjmp-owned heap relation and generation barrier rather than
  requiring a removed v2 replacement/tail holder.
- The staged package at
  `/tmp/ii42-arch3-4c-mech1-stage.5aV71a` passes core CTest `1/1`, isolated SQL
  regression `1/1`, independent golden parity at
  `/tmp/ii42-arch3-4c-mech1-golden.json`, native lifecycle `80/80` at
  `/tmp/ii42-arch3-4c-mech1-native80.json`, and real-model SAE lifecycle
  `15/15` at `/tmp/ii42-arch3-4c-mech1-sae-lifecycle.json`.
- The physical storage fixture passes all 12 rejection boundaries and recovers
  version `2 -> 3`, ten hits, and one subsequent INSERT at
  `/tmp/ii42-arch3-4c-mech1-storage-boundary.json`. Payload corruption also
  recovers through explicit refresh. The independent golden retains manifest
  digest `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- PG18 compiles without warning. Source, staged, and installed dylibs share
  SHA-256 `9e92266e6b244ffeaa41818ed6a5b2b6ca6f86461108ebf994c892f28091ae37`;
  product inventory, modified-script `py_compile`, and `git diff --check` pass.
  The next slice may remove legacy online-maintenance staging/publication and
  its newly unreachable overlay closure, but must not widen this commit.

ARCH-3 slice 4c-mechanical-2 mutation/maintenance boundary:

- The installed mutation and VACUUM boundaries now reject storage version 2
  before dispatch. Simplify `aminsert`, `ambulkdelete`, and `amvacuumcleanup`
  to the existing v3 linked-L0/COW behavior; remove only the unreachable
  generation-relative delta writer, tombstone, XID-freeze, and rebuild paths.
- The v3 maintenance selector always returns after one bounded rotation,
  seal, compaction, semantic-completion, reclamation, fold, tail-cleanup, or
  `no_pending` result. Delete the unreachable legacy online-generation branch
  below that selector and then remove only staging/publication leaves that
  lose their final caller.
- Preserve the current append lock, transaction writer barrier, checked-root
  revalidation, WAL order, semantic worker ownership, and exact COW retirement
  behavior. Ordinary maintenance must not acquire a heap scan or invoke the
  explicit refresh/REINDEX builder.
- This slice does not alter payload decoding, scan/query preparation, status,
  preload, or sealed-snapshot diagnostics. Any legacy reader reached from
  those boundaries remains for the following independently validated slice.
- Validate foreground BM25/SAE mutation, exact VACUUM retirement, transaction
  and 2PC cleanup, semantic completion, restart, physical replay, storage-v2
  rejection/recovery, independent golden parity, and warning-clean PG18 before
  committing the slice.

ARCH-3 slice 4c-mechanical-2 implementation checkpoint:

- `aminsert`, `ambulkdelete`, `amvacuumcleanup`, and the maintenance selector
  now expose only the current linked-L0/document-COW bounded path. Unreachable
  generation staging, exact-delete tombstones, XID freezing, foreground SAE
  batching, hidden rebuild, and post-selector online-maintenance code are
  removed together with helpers that lost their final caller.
- Automatic v3 debt now has one authority: exact linked-L0 records. The
  transaction-scoped pending-activity list had lost its final producer and is
  removed; transaction end only preserves cache/lease cleanup, deferred worker
  wakeup, and the qualified precommit fault boundary. Manual-consistency BM25
  retains metapage stale counters because it deliberately publishes no L0.
- A repeated large-VACUUM replay exposed a timing-dependent mixed-authority
  state after the worker sealed the retirement frontier:
  `delta.records=0` with legacy `delta.retirements=1`. The v3 no-callback
  estimate could write a metapage counter even though only an exact RETIRE can
  be consumed by the v3 selector. Automatic policies now admit only callback-
  proven RETIRE records; estimates remain manual-only. Repeated frontier runs
  must prove the phantom debt cannot recur before this slice closes.
- The first post-removal 8-writer/8-reader SAE replay then exposed a second
  exactness boundary: heap cardinality converged to four while the sealed
  document directory retained six records. VACUUM had proved the old heap TID
  dead, but its compare-and-append guard required the complete document-COW
  record to remain byte-equivalent. Concurrent semantic completion can change
  mutable semantic state without changing the slot incarnation, so the exact
  retirement was silently abandoned. RETIRE now binds to immutable incarnation
  identity (slot, born sequence, heap TID, length, and semantic-input
  fingerprint); semantic completion and quarantine retain their complete-record
  guard. Three independent 8x8 replays at
  `/tmp/ii42-arch3-retire-identity-concurrency-8x8-{a,b,c}.json` each completed
  `288/288` mutations and converged heap/index/sealed cardinality to `4/4/4`
  with zero linked-L0 records, semantic pending rows, or runtime failures.
- The next 140k fault-injected replay proved the stale counter was also still
  observable: a cleared linked frontier could report `records=0` with
  `retirements=1`. Automatic page-native policies now initialize debt only
  from the checked linked-L0 snapshot, while manual consistency alone may read
  the legacy metapage counters. Every page-native root advance clears those
  counters, preventing stale state from surviving rotation, seal, or COW
  publication. Product inventory freezes both invariants. Three fresh runs at
  `/tmp/ii42-arch3-exact-debt-frontier-{a,b,c}.json` pass all `10/10` failure,
  resume, idempotence, seal, restart, and post-frontier-write gates.
- Semantic completion remains worker-owned and batched. Query readers no
  longer invoke the removed foreground semantic queue.
- The generic cache-failure fixture no longer requires the retired generation
  exact-delete VACUUM hook. Current partial document-COW failure, recovery, and
  restart coverage remains authoritative in
  `test_convergent_vacuum_frontier_smoke.py`.
- The remaining cache-failure matrix intentionally does not qualify this
  mutation slice. Its first failing injection is
  `ii42.test_payload_error_after_alloc`, which is reachable only from the
  retired generation-v2 payload loader. The following read/cache slice must
  remove that loader, its overlay/DSM hooks, and their fixture expectations as
  one authority-preserving change; this slice does not widen into query
  preparation.

ARCH-3 slice 4c-mechanical-2 closure evidence:

- The staged root is
  `/tmp/ii42-arch3-4c-mech2-exact-debt-stage.B1CjCs`. Source and staged dylibs
  share SHA-256
  `342836ec50478af3861bfc0b54d74d34cd204cc6e090c07a049acc8ec48f72ab`.
  A live backend mapping audit loaded only that staged dylib, not the older
  system installation.
- Native lifecycle passes `80/80` at
  `/tmp/ii42-arch3-exact-debt-native.json`; the real-model mutable lifecycle
  passes `71/71` at
  `/tmp/ii42-arch3-exact-debt-sae-lifecycle.json`; and transaction, savepoint,
  2PC, MVCC, VACUUM, and crash-restart pass `21/21` at
  `/tmp/ii42-arch3-exact-debt-transactional.json`.
- Full 8-writer/8-reader SAE concurrency completes `288/288` mutations with
  exact `4/4/4` heap/index/sealed cardinality, zero L0 and semantic debt, and
  zero runtime failures at
  `/tmp/ii42-arch3-exact-debt-concurrency-8x8.json`. The three 140k
  fault-injected VACUUM-frontier replays above remain `10/10` each.
- Physical replication passes initial and linked-L0 replay, prepared
  transactions, maintained CRUD, REINDEX, BM25/SAE type conversion, and DROP
  at `/tmp/ii42-arch3-exact-debt-replication.json`. Independent page-native
  golden parity passes at `/tmp/ii42-arch3-exact-debt-golden.json`.
- The storage-layout fixture now accepts the same staged library/control roots
  as the other PG18 gates. It rejects all 12 product boundaries on a patched
  storage-v2 metapage and proves explicit REINDEX recovery to v3 at
  `/tmp/ii42-arch3-exact-debt-storage-boundary.json`.
- Isolated staged SQL regression passes `1/1` under
  `/tmp/ii42-arch3-exact-debt-regression-20260801`; the core CMake test passes
  `1/1`. Modified Python fixtures compile, product convergence inventory and
  `git diff --check` pass. This independently closes the mutation/maintenance
  deletion boundary; the next commit may address only legacy read/cache
  dispatch.

ARCH-3 slice 4c-mechanical-3 read-dispatch boundary:

- Every installed query entry already calls
  `ii42_am_require_convergent_segment_storage`; therefore the v3 branch in
  `ii42_am_get_cached_index_internal` is the only reachable cache attach.
  Replace the post-return generation-v2 payload/DSM dispatch with one direct
  call to the existing checked segment-snapshot attach.
- Stop invoking generation-delta merge wrappers after page-native token,
  field-aware, raw-query, and id scoring. Linked-L0 is already attached by
  `ii42_am_get_cached_segment_index` through the same immutable root and
  statement snapshot, so a second decoded overlay is both unreachable and a
  duplicate lifecycle.
- Keep the page-native cache entry, lease/workspace reclamation, root pinning,
  shared exact-root preload, visibility filtering, scorer, and all SQL result
  shapes byte-for-byte unchanged. The dispatch cutover makes the decoded
  generation-v2 cache closure unreachable; remove that closure in the same
  mechanical commit so the product build remains warning-clean. This does not
  widen the slice into status, preload policy, or maintenance control-plane
  changes.
- Freeze the cutover in product inventory and validate independent golden
  rows, native and real-model lifecycle, weighted/raw/field/id query surfaces,
  restart, and warning-clean PG18. Any TID, score, root, cache residency, RSS,
  or BM25 baseline change stops this slice.

ARCH-3 slice 4c-mechanical-3 implementation checkpoint:

- Installed query cache dispatch now attaches only the checked convergent-v3
  segment snapshot. Token, field-aware, raw, and id paths no longer invoke the
  duplicate generation-delta merge after page-native scoring.
- The now-unreachable decoded payload loader, local delta-overlay merger,
  generation descriptor/DSM attach path, and their private helpers have been
  mechanically removed. Current segment snapshots, statement-snapshot L0
  attachment, exact-root shared preload, leases, workspaces, and scorers remain
  unchanged.
- A warning-clean PG18 build is the current reachability checkpoint. The
  remaining generation-v2 symbols belong to a separate preload/status health
  island; they require an explicit control-plane contract slice and are not
  silently folded into this query-dispatch commit.

ARCH-3 slice 4c-mechanical-3 closure evidence:

- The staged root is
  `/tmp/ii42-arch3-4c-mech3-read-stage.Hjrfl8`; source and staged dylibs share
  SHA-256
  `d3c9637197cbafe80e4c4bdec9872c63786c60867a65fb8b025ca5a969a34ce0`.
  The staged lifecycle report records the staged library and control roots.
- Independent page-native golden parity passes at
  `/tmp/ii42-arch3-4c-mech3-golden.json`. Native lifecycle passes `80/80` at
  `/tmp/ii42-arch3-4c-mech3-native.json`; real-model lifecycle passes `71/71`
  at `/tmp/ii42-arch3-4c-mech3-sae-lifecycle.json`; transaction, savepoint,
  2PC, MVCC, VACUUM, and crash-restart pass `21/21` at
  `/tmp/ii42-arch3-4c-mech3-transactional.json`.
- Physical replication passes initial and linked-L0 replay, prepared
  transactions, maintained CRUD, REINDEX, BM25/SAE conversion, and DROP at
  `/tmp/ii42-arch3-4c-mech3-replication.json`. Storage-v2 is rejected at all
  12 product boundaries and explicit REINDEX recovers v3 at
  `/tmp/ii42-arch3-4c-mech3-storage-boundary.json`.
- The cache failure fixture now tests only current page-native query,
  operator, scan, linked-L0 retry, and same-backend RSS ownership; retired
  payload/DSM/decoded-overlay injections are no longer product requirements.
  It passes against the staged root. Isolated SQL regression passes `1/1`
  under `/tmp/ii42-arch3-4c-mech3-regression-20260801`; the core CMake test
  passes `1/1`; product inventory, modified-script `py_compile`, warning-clean
  PG18 build, and `git diff --check` pass.
- This independently closes installed query dispatch and its unreachable
  decoded-cache closure. The next contract slice is limited to the remaining
  generation-v2 preload/status health island and its obsolete telemetry; it
  must preserve exact-root shared preload and all v3 scheduler authority.

ARCH-3 slice 4c-contract-2 page-native preload/status boundary:

- Every installed preload, status, and background-candidate entry already
  rejects or skips non-convergent storage. Freeze the remaining product
  contract as one v3 control plane before deleting its unreachable branches:
  preload warms manifest-reachable relation pages through PostgreSQL buffers,
  records one disposable exact-root `UNIFIED_WARM` marker, and may retain one
  exact-root `HOT_FOLD`. Neither object is durable index authority.
- Keep the existing `ii42_generation_cache_*` SQL names in this slice. Renaming
  a privileged diagnostic would add migration work without reducing runtime
  authority. Its text and JSON content must, however, stop exposing retired
  DSM descriptor/share eligibility, decoded base-generation residency,
  lexical-delta/tombstone holders, foreground decoded-delta offload/build
  counters, generation-wait counters with no waiting consumer, generation-v2
  block coordinates, and generation-v2 payload byte partitions.
- Preserve current v3 health, checked cache/root epoch, SAE flag, document
  count, physical relation/root bounds, exact linked-L0 debt, rebuild-memory
  admission, worker/reconcile/work-hint state, exact-root/HOT_FOLD residency,
  shared-arena capacity/admission/eviction state, auto-preload priority, and
  relation locator. Do not rename the widely consumed debt fields in this
  authority-removal slice.
- Structured status must remove the legacy `shared_holder_effective` object.
  Current query visibility is represented by the checked root and linked-L0,
  not by independent lexical/tombstone cache coverage. Current docs and tests
  must assert that removed fields are absent rather than permanently zero.
- `ii42_index_shared_preload_resident` must mean exact-root warm-marker
  residency only. Manual and automatic preload must call the same v3 prewarm
  operation. Background candidate classification must remain the existing
  linked-L0, semantic, physical-tail, and workload-fold decision with no
  generation-v2 health or threshold fallback.
- Commit the public text/JSON/docs/test contract independently. Then simplify
  preload, health, and scheduler dispatch and remove only helpers, shared-entry
  kinds, counters, condition variables, descriptor files, and cache fields that
  lose their final caller. Rebuild warning-clean after each deletion wave.
- Validate status shape and absence, exact-root/HOT_FOLD clear and rewarm,
  standby auto-preload, native and real-model lifecycle, memory ownership,
  independent golden parity, storage-v2 rejection, and BM25 exactness before
  each commit. Stop on any result, root, lock/WAL, RSS, preload-residency,
  scheduler-order, or BM25 baseline change.

ARCH-3 slice 4c-contract-2 implementation checkpoint:

- Raw and structured status now describe only convergent-segment storage,
  checked relation/root health, exact linked-L0 debt, workers and scheduler
  state, relation-page residency, `UNIFIED_WARM`, and optional `HOT_FOLD`.
  Descriptor/share eligibility, decoded base/delta/tombstone holders, dead
  generation-wait counters, foreground decoded-delta counters, v2 block
  coordinates, and v2 byte partitions are absent rather than reported as
  permanently zero.
- Manual preload now has one meaning: warm the manifest-reachable v3 relation
  pages through PostgreSQL buffers and publish the checked-root warm marker.
  `ii42_index_shared_preload_resident` reports only that marker. Automatic
  preload and standby recovery retain the same root identity and page-native
  operation.
- Current product docs and SQL expose this contract consistently. Mutable SAE
  lifecycle coverage now validates relation-page warming and shared-arena
  accounting as `UNIFIED_WARM + HOT_FOLD`; it does not consume retired holder
  fields. The product inventory rejects those fields from current SQL and
  product documentation.
- The contract cutover intentionally leaves two newly unreachable static
  helpers, generation share requirement and descriptor read, as compiler
  warnings. They are the mechanical deletion boundary for the next independent
  commit; no compatibility shim or false caller was added to hide them.

ARCH-3 slice 4c-contract-2 closure evidence:

- The staged root is
  `/tmp/ii42-arch3-4c-contract2-status-stage.4QYHjj`. Source and staged dylibs
  share SHA-256
  `b58161ebbc6e4eaf3de967aa80e1be4729f0bbc2d78a5c8efa238ebafee30954`;
  current and staged control and SQL artifacts are also byte-identical.
- Independent page-native golden parity passes at
  `/tmp/ii42-arch3-4c-contract2-golden.json`. Native lifecycle passes `80/80`
  at `/tmp/ii42-arch3-4c-contract2-native.json`; real-model mutable lifecycle
  passes `71/71` at
  `/tmp/ii42-arch3-4c-contract2-sae-lifecycle.json`; transaction, savepoint,
  2PC, MVCC, VACUUM, and crash-restart pass `21/21` at
  `/tmp/ii42-arch3-4c-contract2-transactional.json`.
- Physical replication passes initial and linked-L0 replay, prepared
  transactions, maintained CRUD, REINDEX, BM25/SAE conversion, and DROP at
  `/tmp/ii42-arch3-4c-contract2-replication.json`. All 12 storage-v2 product
  boundaries reject the retired layout and explicit REINDEX recovers v3 at
  `/tmp/ii42-arch3-4c-contract2-storage-boundary.json`.
- Exact-root preload lifecycle and standby auto-preload pass against the staged
  artifacts; isolated SQL regression passes `1/1` under
  `/tmp/ii42-arch3-4c-contract2-regression-final-20260801`; cache
  failure-safety and the core CMake test pass `1/1`.
- Backend memory ownership passes at
  `/tmp/ii42-arch3-4c-contract2-backend-rss.json`: the 80k-row relation reports
  about 2.69 MB of index-scaled backend-private live writable memory, below the
  16 MB gate, while relation pages remain shared-buffer owned. Modified Python
  fixtures compile, product convergence inventory and `git diff --check` pass.
  This closes the public preload/status contract independently; the next slice
  may only delete its unreachable generation-v2 control-plane closure.

ARCH-3 slice 4c-mechanical-4 generation-v2 control-plane closure:

- Simplify every entry that already requires convergent storage before its
  dispatch: payload health validates only the checked v3 root; manual and
  automatic preload call only relation-page warming plus `UNIFIED_WARM`; and
  background candidate selection returns only linked-L0, semantic completion,
  physical-tail, and workload-fold actions. Preserve their current lock,
  fairness, urgency, and action-class ordering.
- Remove the retired on-disk descriptor/DSM cleanup from cache clear. The
  installed product never creates that directory or descriptor, and the beta
  supports migration from `psql_bm25s`, not cleanup of intermediate research
  cache artifacts. Keep bounded backend cache reset, shared derived-state
  retirement, wakeup hints, and catalog reconcile.
- Delete `BASE`, lexical-delta-cache, and tombstone shared-entry kinds and their
  exact-state, admission, publication, generation-block, and decoded-overlay
  helpers after their final unreachable callers are removed. Shared arena
  identity, reserve/publish/evict logic remains for `UNIFIED_WARM` and
  `HOT_FOLD` only.
- Delete generation descriptor types/constants/path readers, old unified-page
  and generation-delta prewarm helpers, descriptor-only DSM state, dead
  generation waiters/counters, and foreground decoded-delta counters when the
  warning/caller graph proves no current consumer. Do not remove page-native
  cache snapshots, leases/workspaces, exact-root registry, HOT_FOLD, linked-L0
  readers, or current model/runtime shared state.
- Perform deletion in compiler-guided waves inside one mechanical commit. Each
  wave must build before the next, and the final PG18 build must have no newly
  unused static functions. Product inventory must reject the retired symbols
  from installed runtime source rather than retain zero-valued compatibility
  fields or fake callers.
- Re-run the contract-2 matrix against fresh staged artifacts: independent
  golden parity; native `80/80`; real-model mutable `71/71`; transactional
  `21/21`; preload and standby preload; physical replication; storage-v2
  rejection and REINDEX recovery; cache failure safety; backend RSS; SQL
  regression; core tests; `py_compile`; inventory; artifact identity; and
  `git diff --check`. Any score, TID, root bytes, linked-L0 debt, lock/WAL,
  scheduler order, RSS, shared residency, or BM25 result change stops deletion.

Implementation checkpoint (2026-08-01, mechanical wave 1-2):

- Direct v3 dispatch is now in place for auto preload, background candidate
  selection, payload health, preload, generation status, and cache clear. These
  edits preserve the existing relation/maintenance locks, action ordering,
  checked-root validation, page warming, `UNIFIED_WARM`, `HOT_FOLD`, linked-L0,
  and catalog-reconcile authority; they only bypass the unreachable generation
  descriptor and decoded-overlay branches.
- The first PG18 build exposed 13 unused generation-v2 roots. Mechanical wave 2
  removed exactly those definitions and rebuilt successfully. The resulting 18
  warnings are the expected next private closure: five stale prototypes plus
  descriptor identity/path helpers, old shared BASE state, generation-page
  validators, decoded lexical/tombstone publishers, old threshold helpers, and
  semantic pending/status scan helpers. No enum, shared-memory ABI, SQL status,
  score, WAL, lock, or root-format contract has changed yet.
- Continue only through compiler-confirmed dead symbols. After definitions and
  declarations are warning-clean, shared-entry kinds/state and shared control
  fields may be narrowed in a distinct internal-ABI wave before qualification.
  Mechanical wave 3 removed the 18 warned declarations/definitions and rebuilt
  successfully; the closure is now limited to ten old decoded-overlay/preload
  helpers and semantic status-scan helpers. No current linked-L0 completion,
  v3 scorer, HOT_FOLD, root-registry, lock, or WAL helper became unreachable.
  Mechanical wave 4 removed those ten roots and rebuilt successfully; eight
  generation-block construction/type helpers and semantic status-cursor
  helpers remain in the compiler-confirmed dead closure.

Implementation checkpoint (2026-08-01, mechanical wave 5-7):

- Three further warning-guided waves removed the remaining generation-block
  builder, decoded-overlay, descriptor, waiter, and semantic-status cursor
  closure. The final PG18 build is warning-clean; no current linked-L0,
  page-native scorer, semantic-completion, HOT_FOLD, root-registry, lock, WAL,
  or runtime-session owner became unreachable.
- Shared preload identity now admits only `UNIFIED_WARM` and `HOT_FOLD` entries.
  The internal ABI version advances from 25 to 26, and reserve/publish remains
  nonblocking: an entry already loading returns busy rather than adding a
  waiter, timeout, retry loop, or backend-owned payload. Transaction cleanup no
  longer invokes the removed shared-generation lease release, because current
  entries never attach such leases to a backend transaction.
- The old 4,096-record/64-MiB `eventual_backlog_limits` helper was telemetry for
  the unreachable generation-delta status branch, not v3 write admission. The
  v3 writer has, since the mutation-lifecycle cutover, used soft active-L0
  rotation plus the active/pending hard frontier. Inventory now requires that
  hard frontier and forbids the retired helper instead of retaining an inert
  compatibility symbol.
- `make clean all` with the Homebrew PG18 PGXS, product inventory, modified
  inventory `py_compile`, and `git diff --check` pass. Full staged behavioral
  qualification remains the mechanical-slice commit gate.

ARCH-3 slice 4c-mechanical-4 closure evidence:

- Fresh staged root:
  `/tmp/ii42-arch3-4c-mech4-stage.bsfrXb`. Source and staged dylibs have the
  identical SHA-256
  `9cf73a8af549d5adeb8131e76da5eb03439746e3a73c0168e36ccd0f07a349e7`.
  The independent golden fixture passes every frozen BM25, predicate,
  linked-L0, MVCC, and semantic surface at
  `/tmp/ii42-arch3-4c-mech4-golden.json`.
- Native v3 exactness passes `80/80` at
  `/tmp/ii42-arch3-4c-mech4-native.json`; real-model lexical-first mutable
  lifecycle passes `71/71` at `/tmp/ii42-arch3-4c-mech4-mutable.json`; and
  transaction/savepoint/2PC/crash-restart passes `21/21` at
  `/tmp/ii42-arch3-4c-mech4-transaction.json`.
- Exact-root preload lifecycle and standby auto-preload pass. Physical
  replication passes linked-L0, semantic completion, prepared transactions,
  CRUD, REINDEX, BM25/SAE conversion, and DROP at
  `/tmp/ii42-arch3-4c-mech4-replication.json`. All 12 storage-v2 product
  boundaries reject the retired layout, and explicit REINDEX recovers v3 at
  `/tmp/ii42-arch3-4c-mech4-storage-boundary.json`.
- Cache failure safety passes; isolated SQL regression passes `1/1` under
  `/tmp/ii42-arch3-4c-mech4-regression`; and the core CMake test passes `1/1`.
  The 5,000-to-80,000-document backend ownership gate passes at
  `/tmp/ii42-arch3-4c-mech4-backend-rss.json`: index-scaled II-42 context bytes
  are zero, allocated malloc growth is 13,312 bytes, and private-live growth is
  5,668,864 bytes against the 16-MiB ceiling.
- The final PG18 build is warning-clean. Product inventory, modified-script
  `py_compile`, dynamic-symbol and source legacy scans, staged artifact
  identity, and `git diff --check` pass. This closes the generation-v2
  preload/control-plane implementation island without changing product
  results, root authority, lock/WAL ordering, scheduler behavior, shared
  residency, or backend ownership.

ARCH-3 slice 4d-contract stale-admission closure:

- Keep the live shared-arena admission projection: current root residency,
  loading, candidate size, slot/space availability, reusable or evictable
  capacity, and the computed `admission_state`. These values describe the
  current nonblocking `UNIFIED_WARM`/`HOT_FOLD` arena and remain useful.
- Remove preload admission-miss totals and last-miss identity/age from raw and
  structured status. Since generation-v2 publication was retired, no product
  writer updates these counters; permanently zero history is a false contract,
  not operational telemetry.
- Remove the obsolete semantic `backlog_record_limit`/`backlog_byte_limit`
  documentation and the uncalled legacy backpressure audit. V3 write safety is
  the physical active/pending linked-L0 hard frontier; semantic completion
  reports exact pending/quarantine debt and worker progress without claiming a
  separate 4,096-record/64-MiB admission policy.
- Change the status/API/docs/test contract in one commit while leaving the now
  unreachable shared-control fields in place. Remove those fields and advance
  the private shared-memory ABI only in the following mechanical commit.
- Gate both commits with a warning-clean PG18 build, independent golden parity,
  native and real-model lifecycle, preload lifecycle, SQL regression, product
  inventory, modified-script `py_compile`, and `git diff --check`. The
  mechanical commit additionally requires restart/preload evidence against a
  fresh staged binary. Stop on any result, root, lock/WAL, shared residency,
  RSS, or BM25 change.

ARCH-3 slice 4d-contract closure evidence:

- The fresh staged root is
  `/tmp/ii42-arch3-4d-contract-stage.V8H4gY`. Source and staged dylibs have the
  identical SHA-256
  `3a1b3481a20ebf829ad6c25fe346f1920cab816d8a90ecd7e4079f8381ecafa1`.
  The Homebrew PG18 build is warning-clean.
- Independent golden parity passes every frozen initial, predicate,
  linked-L0, MVCC, and semantic surface at
  `/tmp/ii42-arch3-4d-contract-golden.json`. Native v3 lifecycle passes
  `80/80` at `/tmp/ii42-arch3-4d-contract-native-rerun.json`; real-model
  lexical-first mutable lifecycle passes `71/71` at
  `/tmp/ii42-arch3-4d-contract-mutable.json`.
- Exact-root preload lifecycle passes with only current `UNIFIED_WARM` and
  `HOT_FOLD` admission state. Isolated staged SQL regression passes `1/1`
  under `/tmp/ii42-arch3-4d-contract-regression.87cXdO`.
- Product inventory, modified-script `py_compile`, and `git diff --check`
  pass. Current raw and structured status retain live arena capacity,
  eviction, residency, and computed admission state while omitting false
  admission history; current docs and tests no longer claim a semantic
  backlog limit that v3 does not enforce.

ARCH-3 slice 4d-mechanical shared-control closure:

- Remove only the six shared-control fields made unreachable by the 4d
  contract commit. Advance the private preload ABI from 26 to 27 so a server
  restart always constructs the narrowed shared-memory layout; no durable
  page, SQL, scorer, scheduler, or mutation contract changes.
- Re-run warning-clean PG18 build, product inventory, exact-root preload across
  a fresh staged restart, independent golden parity, native lifecycle, and
  `git diff --check`. The slice stops if residency, admission state, result
  order/score, root identity, or backend ownership changes.

Closure evidence:

- The narrowed private ABI is version 27. The fresh staged root is
  `/tmp/ii42-arch3-4d-mechanical-stage.LmHZSA`; source and staged dylibs share
  SHA-256
  `a9c57dbf2728800bb278b10bb1da0161b85542c099a9107d51ad9699a03a882a`.
- The PG18 build is warning-clean. Independent golden parity passes at
  `/tmp/ii42-arch3-4d-mechanical-golden.json`, native lifecycle passes `80/80`
  at `/tmp/ii42-arch3-4d-mechanical-native.json`, and exact-root preload
  lifecycle passes after constructing fresh shared memory from the staged
  binary. Product inventory and `git diff --check` pass.
- Only six unreachable shared-control fields were removed. Arena admission
  state, `UNIFIED_WARM`, `HOT_FOLD`, access clock, eviction, workers, hints,
  reconciliation, semantic telemetry, and all durable relation authority are
  unchanged.

ARCH-3 slice 4e generation-delta loader closure:

- A source-level caller audit found a second, narrower generation-v2 reader
  island after the installed query cache was removed. The global
  `ii42_unified_delta_load*` APIs and `Ii42UnifiedDelta` identity/container have
  no caller outside their own implementation and header declarations. Hidden
  symbol visibility does not make this product code acceptable.
- Remove only those loader APIs, their prefix scanner, generation-delta identity
  initializer, and helpers that lose their final caller. Keep the typed
  `Ii42UnifiedDeltaDocument` decoder for now: the installed owner-only semantic
  quarantine diagnostic still calls it. Keep linked-L0 page/record codecs, XID
  visibility, append serialization, the v3 full-overlay oracle, and current
  document-COW semantic completion unchanged.
- Do not remove the old semantic-generation reader/cursor API in this commit.
  It is independently uncalled and will be a later mechanical slice after its
  generation-page closure is separated from current payload writing. Do not
  combine that larger deletion with the narrow delta-loader removal.
- Extend the product inventory to reject the retired loader API and container
  types. Rebuild warning-clean after root deletion and prune only newly unused
  static leaves. Validate independent golden parity, native lifecycle,
  real-model mutable lifecycle, storage-v2 rejection/recovery, preload,
  isolated SQL regression, and source/staged artifact identity before commit.
- Stop on any result, root, lock/WAL, RSS, linked-L0, semantic-completion,
  preload-residency, or BM25 change. This slice changes no installed SQL or
  supported storage behavior.

Closure evidence:

- The fresh staged root is `/tmp/ii42-arch3-4e-stage.CgGpmx`. Source and staged
  dylibs share SHA-256
  `3d7ac69043d02537fb05646ad52f8c0cc303e95eff8fa4dbe863e05320ae6621`;
  staged control and SQL SHA-256 values are respectively
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity passes every frozen surface at
  `/tmp/ii42-arch3-4e-golden.json`; native v3 lifecycle passes `80/80` at
  `/tmp/ii42-arch3-4e-native.json`; and real-model lexical-first mutable
  lifecycle passes `71/71` at `/tmp/ii42-arch3-4e-mutable.json`.
- Exact-root preload lifecycle passes. All 12 storage-v2 product boundaries
  reject the retired layout and explicit REINDEX restores v3 at
  `/tmp/ii42-arch3-4e-storage-boundary.json`. Isolated staged SQL regression
  passes `1/1` under `/tmp/ii42-arch3-4e-regression`.
- The Homebrew PG18 build and CMake build are warning-clean, core CTest passes
  `1/1`, and product inventory, modified-script `py_compile`, retired-name
  source scan, source/staged artifact identity, and `git diff --check` pass.
  No installed SQL, result, root, linked-L0, completion, preload, lock/WAL, RSS,
  or BM25 contract changed.

CSG-I121 diagnostic follow-up:

- `ii42_index_semantic_quarantine_internal()` correctly rejects unsupported
  storage before execution, but its detail scanner still reads generation-v2
  `meta.delta_*` pages. A valid v3 metapage requires those fields to be zero, so
  the installed owner-only diagnostic cannot enumerate durable v3 quarantine
  rows even though the document-COW/linked-L0 authority and aggregate telemetry
  can contain them.
- Do not hide this by retaining the v2 scanner or weakening the storage guard.
  After slice 4e, establish a dedicated contract slice that projects exact
  quarantine details from the checked v3 document authority and matching
  linked-L0 transitions, preserving owner-only ACL, bounded memory, restart,
  replication, update/delete supersession, and existing JSON shape. Only then
  remove the v2 quarantine scanner and shared legacy document representation.

ARCH-3 slice 4f contract, v3 quarantine identity closure:

- The migration gap is now located precisely. Generation-v2 payload v5 stored
  `pending_since` and `error_hash`, while the linked-L0 v3 quarantine record,
  immutable semantic-state record, and document-COW record retained only
  failure count, SQLSTATE, retry time, and semantic-input fingerprint. The v3
  append path already computes the failure timestamp and receives the error
  hash, but discards both. Consequently, neither the documented JSON shape nor
  restart-stable failed-row identity can be reconstructed from current v3
  authority.
- Preserve the existing owner-only diagnostic contract rather than deriving
  approximate values. Add durable `pending_since` and `error_hash` fields to
  the quarantine transition and semantic-state record. A first failure records
  the current timestamp; a retry of the same quarantined document retains its
  original `pending_since` while replacing retry time, failure count,
  SQLSTATE, and error hash with the latest failure. Complete, upsert, and retire
  records must require these quarantine-only fields to be zero.
- This is an intentional beta storage-contract correction, not a mechanical
  refactor. Bump the linked-L0 record, immutable segment payload, and
  document-COW object versions and their checked record sizes. The project does
  not support upgrades between experimental v3 layouts; the retained upgrade
  boundary remains old `psql_bm25s` to the current product. Existing stale
  layouts must fail closed and recover through explicit `REINDEX`.
- Replace the installed diagnostic with one exact root-relative projection.
  Classify the bounded pending and active L0 frontiers under one XID-status
  window, reduce their visible events by document slot and sequence, then walk
  the COW document authority. Emit only live quarantined incarnations, including
  linked-L0-only tail slots, and let visible upsert/retire/complete transitions
  supersede older quarantine state. Memory may scale with the hard-bounded L0
  frontier plus returned quarantine rows, never corpus size or posting payload.
  Any unresolved XID makes `exact=false`; no uncommitted identity is exposed.
- Correct the dedicated smoke driver so it advances maintenance until the
  semantic-completion action under test is selected. It must not assume that
  the first maintenance call skips required rotate/seal work. This changes only
  the test's scheduling assumption, not product maintenance policy.
- Qualification must cover codec round trips and corruption rejection, exact
  owner ACL and JSON identity, retry age/hash persistence, update/delete/REINDEX
  supersession, restart, physical replication, normal/oracle parity, native and
  real-model mutable lifecycles, 2PC, storage-version rejection/recovery,
  preload, isolated staged SQL, and source/staged artifact identity. Record the
  expected format/hash changes separately; stop on score, visible result,
  query-root semantics, lock/WAL order, RSS, or BM25 regression.

ARCH-3 slice 4f contract closure evidence:

- Linked-L0 record version `3 -> 4`, immutable segment payload version `4 -> 5`,
  and document-COW version `4 -> 5` are the only durable format changes. The
  corresponding records grow by 16 bytes to persist `pending_since` and
  `error_hash`; the isolated SQL golden updates only those physical byte counts.
  Ordered IDs, scores, checked-root semantics, and the independent golden
  manifest digest remain unchanged at
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`.
- Core codec tests now directly round-trip and corruption-check the new fields
  in linked-L0, immutable quarantine state, and document COW. CMake builds
  warning-clean and CTest passes `1/1`.
- The dedicated staged quarantine report at
  `/tmp/ii42-arch3-4f-quarantine.json` passes. It proves owner-only ACL, stable
  CTID/SQLSTATE/error-hash identity, first-failure age preservation across
  retries and restart, head/middle/tail and runtime-wide failure behavior,
  update/delete/REINDEX supersession, and normal/oracle parity. An unresolved
  transaction reports `exact=false` with no uncommitted row identity and returns
  to exact empty state after rollback.
- Native v3 lifecycle passes `80/80` at
  `/tmp/ii42-arch3-4f-native.json`; real-model mutable lifecycle passes `71/71`
  at `/tmp/ii42-arch3-4f-mutable.json`; transaction, savepoint, 2PC, crash, and
  VACUUM lifecycle passes `21/21` at
  `/tmp/ii42-arch3-4f-transaction.json`. Physical replication passes at
  `/tmp/ii42-arch3-4f-replication.json`.
- Runtime-service restart, exact-root shared preload, standby replay and
  auto-preload, and storage-version rejection/recovery pass. The latter reports
  current storage version 3 and recovers a version-2 fixture only through the
  explicit supported recovery boundary. Isolated staged SQL regression passes
  `1/1` under `/tmp/ii42-arch3-4f-regression-rerun`.
- The staged root is `/tmp/ii42-arch3-4f-stage.ZxUywa`. Source and staged
  dylibs share SHA-256
  `6119d762731215dc33ec8204957883404f63e47c8b2ac276a563d64fa0163223`;
  control and installed SQL artifacts are also byte-identical. PG18 builds
  warning-clean; product inventory, modified-script `py_compile`, and
  `git diff --check` pass.
- This closes CSG-I121's installed diagnostic gap without changing public JSON,
  query results, root publication, lock/WAL order, shared-runtime ownership, or
  BM25 behavior. The disabled generation-v2 scanner remains only as the explicit
  4g mechanical deletion boundary and is not compiled or reachable.

ARCH-3 slice 4g mechanical quarantine cleanup:

- Only after slice 4f passes, remove the generation-v2 quarantine page scanner,
  its reserve/remove/free closure, and `Ii42UnifiedDeltaDocument` fields/helpers
  that lose their final installed caller. Extend the product inventory to reject
  those retired names. This commit must not change SQL, storage bytes, JSON,
  scores, roots, locks, WAL, memory ownership, or lifecycle behavior.
- The post-4f caller audit establishes the exact deletion closure. The disabled
  scanner is the sole caller of the generation-v2 data-page and record-header
  structs, semantic-frontier tombstone/cursor structs, unified-delta payload
  header/flags/validators, payload-size/document decoder, and the old
  pending/quarantine containers. `Ii42UnifiedDeltaDocument` has no caller
  outside that closure and therefore leaves `ii42_semantic.h` as well.
- Retain `ii42_am_unified_delta_pair`,
  `ii42_am_unified_delta_lexical_pair`, and the current lexical-parts builder:
  they are used by the page-native semantic compiler. Retain
  `ii42_am_delta_record_states()` and its exported batch wrappers: v3 query
  visibility and quarantine projection still require one exact XID-status
  classification window. Generation-data page helpers also remain current.
- Add source-inventory rejection for the retired document type, v2 quarantine
  scanner, old frontier/page/header types, and payload decoder. The edit is a
  pure unreachable-code deletion: no reloption, GUC, SQL declaration, binary
  format, manifest digest, or expected byte count may move.
- Qualify with warning-clean PG18 and CMake builds, CTest, product inventory,
  exact quarantine diagnostics, frozen golden parity, native and real-model
  mutable lifecycle, transaction/2PC/restart, physical replication, exact-root
  preload, storage-version rejection/recovery, isolated staged SQL, artifact
  identity, modified-script `py_compile`, and `git diff --check`. Stop and split
  the closure if any supposedly retired identifier has a live caller or if any
  result, root, lock/WAL, RSS, preload, semantic-completion, or BM25 evidence
  changes.

ARCH-3 slice 4g closure evidence:

- The generation-v2 quarantine scanner and its complete private page/header,
  frontier, payload-decoder, pending-batch, and document-container closure are
  deleted. `Ii42UnifiedDeltaDocument` and the retired semantic flags leave the
  shared semantic header. Product inventory now rejects every removed entry
  point and representation; the source scan reports zero retired matches.
- The current v3 quarantine projector, linked-L0 XID classifier, semantic
  compiler pair types, lexical-parts builder, and generation-data page helpers
  remain compiled and independently called. Warning-clean PG18 and CMake builds
  pass; CTest passes `1/1`; product inventory, modified-script `py_compile`, and
  `git diff --check` pass.
- Exact quarantine diagnostics pass at
  `/tmp/ii42-arch3-4g-quarantine.json`; frozen golden parity passes at
  `/tmp/ii42-arch3-4g-golden.json` with unchanged manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`.
  Native lifecycle passes `80/80` at `/tmp/ii42-arch3-4g-native.json`,
  real-model mutable lifecycle passes `71/71` at
  `/tmp/ii42-arch3-4g-mutable.json`, and transaction/2PC/restart passes `21/21`
  at `/tmp/ii42-arch3-4g-transaction.json`.
- Physical replication passes at `/tmp/ii42-arch3-4g-replication.json`.
  Exact-root shared preload and runtime-service restart pass; standby replay and
  auto-preload pass at `/tmp/ii42-arch3-4g-standby-preload.json`. All 12 retired
  storage-v2 product boundaries fail closed and explicit REINDEX restores v3 at
  `/tmp/ii42-arch3-4g-storage-boundary.json`. Isolated staged SQL passes `1/1`
  under `/tmp/ii42-arch3-4g-regression`.
- The staged root is `/tmp/ii42-arch3-4g-stage.mjFrF0`. Source and staged
  dylibs share SHA-256
  `13998bcceef8364632953ff0745baeda9547f5eab063a446d6fbab532a92763c`;
  control and installed SQL artifacts are also byte-identical. The slice changes
  no installed SQL, storage bytes, JSON, score, root publication, lock/WAL,
  preload, semantic-completion, RSS ownership, or BM25 contract.

ARCH-3 slice 4h direct-reader deletion boundary:

- The post-4g whole-repository caller audit finds no consumer of
  `Ii42SemanticReader`, `Ii42SemanticDocumentCursor`, or their exported header
  APIs outside their own implementation closure and inventory assertions. The
  document cursor is the only consumer of the callback-backed resident payload
  decoder in `ii42_semantic.c`; the reader is the only consumer of the shared
  generation-snapshot lock wrappers, reader-only pause hook, generation-delta
  identity comparator, generation-data TID-page kind, and TID cache.
- Remove that complete reader/cursor closure and make product inventory reject
  its names. Convert the inventory's old lock-order assertions into absence
  assertions; they described an unsupported generation-v2 reader rather than a
  current product invariant.
- Retain the exclusive generation barrier used to serialize full relation
  replacement and `REINDEX`. Retain the semantic-data page header/helper,
  durable metapage fields, and storage-layout validation that still provide the
  explicit fail-closed/REINDEX boundary for unsupported on-disk generations.
  Retain the unified payload magic/version constants used by the current
  page-native semantic compiler.
- This is a mechanical unreachable-code deletion only. It must not change
  installed SQL, metapage bytes, root identity, scores, JSON, lock/WAL order,
  semantic completion, preload ownership, RSS, or BM25 behavior. Qualify it with
  the same warning-clean builds, exactness, lifecycle, transaction, replication,
  preload, storage-boundary, staged-package, and artifact-identity gates as 4g.
  Stop if any deleted name has a caller or any retained v3 path depends on the
  reader's ShareLock side of the generation barrier.

ARCH-3 slice 4h closure evidence:

- The direct semantic reader, document cursor, callback-backed resident
  decoder, reader-only generation snapshot ShareLock wrappers, pause hook,
  delta-identity comparator, generation-data TID page kind, and public header
  surface are deleted. Product inventory rejects every retired name. The
  exclusive generation barrier remains around full replacement and `REINDEX`;
  semantic-data validation and the durable metapage rejection boundary remain
  unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4h-stage.EA81Gt`. Source, staged,
  and installed dylibs share SHA-256
  `e2d18349e6544238bd2867ebee807bc65318e003e47c1ab7f7448e8b0340d763`.
  Source, staged, and installed control and version SQL artifacts are also
  byte-identical, with SHA-256 values
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity passes at `/tmp/ii42-arch3-4h-golden.json` with
  unchanged manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`.
  Native lifecycle passes `80/80` at `/tmp/ii42-arch3-4h-native.json`,
  real-model mutable lifecycle passes `71/71` at
  `/tmp/ii42-arch3-4h-mutable.json`, and transaction/2PC/restart passes `21/21`
  at `/tmp/ii42-arch3-4h-transaction.json`.
- Exact quarantine diagnostics pass at
  `/tmp/ii42-arch3-4h-quarantine.json`; physical replication passes at
  `/tmp/ii42-arch3-4h-replication.json`; exact-root preload, standby replay and
  auto-preload, and runtime-service restart pass. All 12 retired storage-v2
  product boundaries fail closed and explicit `REINDEX` restores v3 at
  `/tmp/ii42-arch3-4h-storage-boundary.json`. Isolated staged SQL regression
  passes `1/1` under `/tmp/ii42-arch3-4h-regression.A5xaUT`.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; product
  inventory, modified-script `py_compile`, retired-name source scan, and
  `git diff --check` pass. The slice changes no installed SQL, metapage/root
  bytes, result IDs/order/scores, current completion/preload ownership, or BM25
  contract.

ARCH-3 slice 4i runtime-signature fallback boundary:

- The post-4h call audit finds exactly two callers of
  `ii42_am_runtime_signature_for_generation()`: the internal runtime-signature
  SQL function and the native semantic scorer. Both read the metapage and call
  `ii42_am_require_convergent_segment_storage()` before requesting the
  signature. Its non-v3 branch, persisted-generation contract reader,
  semantic-page metadata validator, page header helpers, and semantic-data page
  representation are therefore unreachable in the installed product.
- Replace both calls with the current runtime-signature authority and remove
  only that unreachable closure. Retain `II42_AM_STORAGE_GENERATION_DELTA`, the
  fixed metapage fields, and `ii42_am_normalize_meta_storage()` because current
  storage-boundary tests require enough v2 identity to reject every product
  operation and direct users to explicit `REINDEX`.
- Add inventory absence checks for the removed fallback. This slice changes no
  public SQL declaration, storage bytes, runtime signature value, root, score,
  lock/WAL, shared state, or build/compiler behavior. Run the full 4h matrix and
  stop if either caller can reach the removed branch or any signature/golden
  identity changes.

ARCH-3 slice 4i closure evidence:

- Both post-v3-gate callers now invoke the current runtime-signature authority
  directly. The generation-specific dispatcher, persisted semantic-page
  contract reader, metadata validator, page-size helpers, page representation,
  and semantic-data page kind are deleted, and product inventory rejects every
  retired name. The fixed metapage shape, storage-v2 identity, normalization,
  fail-closed message, and explicit `REINDEX` recovery remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4i-stage.ImnGP7`. Source, staged,
  and installed dylibs share SHA-256
  `dcec24f9a5d7ec8250f14ef608478e7bcbcc2cb9123c58b82a9e4464ca140b37`.
  Source, staged, and installed control and version SQL artifacts remain
  byte-identical, with SHA-256 values
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity passes at `/tmp/ii42-arch3-4i-golden.json` with
  unchanged manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native lifecycle
  passes `80/80` at `/tmp/ii42-arch3-4i-native.json`, real-model mutable
  lifecycle passes `71/71` at `/tmp/ii42-arch3-4i-mutable.json`, and
  transaction/2PC/restart passes `21/21` at
  `/tmp/ii42-arch3-4i-transaction.json`.
- Exact quarantine diagnostics pass at
  `/tmp/ii42-arch3-4i-quarantine.json`; physical replication passes at
  `/tmp/ii42-arch3-4i-replication.json`; exact-root preload, standby replay and
  auto-preload, and runtime-service restart pass. All 12 storage-v2 product
  boundaries fail closed and explicit `REINDEX` restores v3 at
  `/tmp/ii42-arch3-4i-storage-boundary.json`. Isolated staged SQL regression
  passes `1/1` under `/tmp/ii42-arch3-4i-regression`.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; product
  inventory, modified-script `py_compile`, retired-name source scan, and
  `git diff --check` pass. The slice changes no public SQL, metapage/root bytes,
  runtime signature, golden score/order, lock/WAL, completion/preload ownership,
  or BM25 contract.

ARCH-3 slice 4j discarded-payload boundary:

- The post-4i build audit finds one actual caller of
  `ii42_am_build_replacement()`, and it always requests streaming page-native
  segment output. For SAE, `ii42_am_semantic_builder_finish()` currently builds
  the current segment source and then serializes an additional `EATMH004`
  `BufFile`; the caller immediately closes that file and resets its byte count.
  Both convergent publishers explicitly reject a replacement that still owns
  legacy semantic bytes or a semantic file. The serialized payload therefore
  has no current consumer but adds build I/O and ownership branches.
- Make the AM semantic finish boundary return only the current lexical index,
  semantic postings, and input fingerprints. Remove discarded semantic-file
  outputs and replacement ownership checks/cleanup. Retain document merge,
  segment-source validation, runtime signature, legacy writer implementation,
  impact-head/frontier construction, and all storage-v2 rejection fields in
  this slice; their independent deletion audits follow separately.
- This is a mechanical removal of unobservable work, not a representation or
  score change. Product inventory must reject the discarded AM payload writer
  and replacement fields. Qualify the complete 4i matrix and stop on any root,
  score/order, semantic posting count, lock/WAL, lifecycle, RSS, or BM25 change.

ARCH-3 slice 4j closure evidence:

- The AM semantic finish boundary now returns only the current page-native
  lexical index, semantic postings, and input fingerprints. It no longer
  serializes, owns, closes, validates, or publishes a discarded semantic
  `BufFile`; replacement and convergent publication contain no legacy payload
  branch. Impact-head/frontier construction and the standalone legacy writer
  remain deliberately untouched for their separate audits.
- The fresh staged root is `/tmp/ii42-arch3-4j-stage.Yyd3V4`. Source, staged,
  and installed dylibs share SHA-256
  `000666c5d010fbc185017a20011a535f3b423b69c39f0d02509482a15126eb5a`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity passes at `/tmp/ii42-arch3-4j-golden.json` with
  unchanged manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native lifecycle
  passes `80/80`, real-model mutable lifecycle passes `71/71`, and
  transaction/2PC/restart passes `21/21` in the matching `/tmp/ii42-arch3-4j-*`
  JSON artifacts.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
  PG18 and CMake builds are warning-clean; CTest passes `1/1`; inventory,
  modified-script `py_compile`, retired-name scan, and `git diff --check` pass.
  No public SQL, root bytes, runtime signature, score/order, lock/WAL,
  completion/preload ownership, or BM25 contract changed.

ARCH-3 slice 4k serialized-writer boundary:

- After 4j, `ii42_semantic_build_empty_unified_payload()` and
  `ii42_semantic_write_unified_payload()` form a closed self-referential source
  cluster. Their callback types, input structure, byte writer, `EATMH004`
  magic/version constants, and empty-payload adapter have no AM, SQL, test,
  model-runtime, page-native segment, or golden-fixture caller.
- Delete that complete source/header cluster and invert product inventory from
  requiring `EATMH004` to rejecting the retired writer/API names and magic.
  Retain checkout/runtime contract loading, text compiler, shared runtime,
  fingerprints, typed linked-L0 records, metapage storage-v2 rejection identity,
  and all current semantic completion behavior.
- This slice removes no current intermediate produced by the compiler and
  changes no build algorithm. Qualify the complete 4j matrix and stop on any
  symbol consumer, artifact/root/signature drift, score/order difference,
  lifecycle failure, RSS change, or BM25 regression.

ARCH-3 slice 4k closure evidence:

- The zero-caller `EATMH004` serialized semantic writer is absent from the C
  implementation and public semantic header. Product inventory now rejects
  its magic, writer entrypoints, callback types, and payload-input structure
  instead of treating that retired format as current. Checkout/runtime
  loading, text compilation, fingerprints, typed linked-L0 records, current
  segment publication, semantic completion, and storage-v2 rejection remain
  unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4k-stage.I0yUnR`. Source, staged,
  and installed dylibs share SHA-256
  `2e4f7cab6ab939dc534eded73d655c6d27014df009f940e2766b16c1a53b1f60`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. An initial
  concurrent native run encountered only maintenance `lock_busy` setup
  contention; the required isolated rerun passes `80/80`. Real-model mutable
  lifecycle passes `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
  Full JSON evidence is under `/tmp/ii42-arch3-4k-evidence.21ofDa`.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; inventory,
  modified-script `py_compile`, retired-name scan, and `git diff --check` pass.
  No public SQL, current root bytes, runtime signature, score/order, lock/WAL,
  completion/preload ownership, RSS authority, or BM25 contract changed.

ARCH-3 slice 4l discarded-impact-head boundary:

- After 4k, every unified lexical or retained semantic pair is copied into a
  dedicated tuplesort. Finish materializes the best 256 documents per atom
  into `head_starts_file` and `head_pairs`, then closes both temporary files.
  No current segment source, publication path, root, scorer, SQL function,
  test oracle, or status surface receives either file or its counters.
- Delete only this discarded impact-head pipeline: its constant and pair type,
  builder sort/file fields, sort initialization and feed calls, finish
  materialization, and abort cleanup. Retain document merge, semantic budget,
  frontier/auxiliary construction, current segment-source construction, and
  all relation-page publication.
- Product inventory must reject the removed head symbols. Qualify the complete
  4k matrix and stop on any current posting count, root/signature, score/order,
  lifecycle, lock/WAL, RSS, or BM25 difference. Frontier and auxiliary files
  require a separate zero-consumer audit and are outside this slice.

ARCH-3 slice 4l closure evidence:

- The discarded impact-head tuplesort, per-atom top-256 materialization, temp
  files, builder state, and abort cleanup are absent. Product inventory rejects
  every removed symbol. Document merge, semantic budget and retained postings,
  frontier/auxiliary construction, current segment-source construction, and
  relation-page publication remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4l-stage.kfWXdH`; full evidence is
  under `/tmp/ii42-arch3-4l-evidence.FHGVqb`. Source, staged, and installed
  dylibs share SHA-256
  `468a0adadca377ed9c5eb9fe1ec0280fac8c221d74c299e2c42e797069ccd900`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`, real-model mutable lifecycle passes `71/71`, and
  transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; inventory,
  modified-script `py_compile`, retired-name scan, and `git diff --check` pass.
  No public SQL, current root bytes, runtime signature, score/order, lock/WAL,
  completion/preload ownership, RSS authority, or BM25 contract changed.

ARCH-3 slice 4m discarded-auxiliary-index boundary:

- After 4l, finish still derives three temporary inverted views from current
  build intermediates: lexical atom postings, semantic atom postings, and
  semantic local-rank candidates. Their result structure is local to finish;
  no segment source, publisher, root, scorer, SQL/status surface, test oracle,
  or later build step reads it before all three files are closed.
- Delete only the auxiliary result/type closure and its two builders. Retain
  the current lexical document rows used by unified merge, the semantic
  frontier producer and validation, current segment-source construction, and
  every relation-page publication path. The frontier becomes an explicit
  zero-consumer candidate for a later, independently qualified slice.
- Product inventory must reject the removed auxiliary symbols. Qualify the
  complete 4l matrix and stop on any posting count, root/signature,
  score/order, lifecycle, lock/WAL, RSS, or BM25 difference.

ARCH-3 slice 4m closure evidence:

- The lexical-atom, semantic-atom, and semantic-rank auxiliary builders and
  their zero-consumer result closure are absent. Product inventory rejects all
  removed symbols. Unified lexical rows, semantic frontier construction,
  semantic budget, current segment source, relation-page publication, and the
  independent golden oracle remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4m-stage.cOwWMI`; full evidence is
  under `/tmp/ii42-arch3-4m-evidence.gWqzFL`. Source, staged, and installed
  dylibs share SHA-256
  `fa5b1a3b052d6c598f520b9fac0c2b1381f458e0bfca072e64fc758d53ec6796`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`, real-model mutable lifecycle passes `71/71`, and
  transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; inventory,
  modified-script `py_compile`, retired-name scan, and `git diff --check` pass.
  No public SQL, current root bytes, runtime signature, score/order, lock/WAL,
  completion/preload ownership, RSS authority, or BM25 contract changed.

ARCH-3 slice 4n discarded-semantic-frontier boundary:

- After 4m, unified merge still copies every weight-sorted semantic pair into
  a frontier temp file and O(documents) starts array, stores them on the
  builder, rewinds the file, and later only closes it. There is no remaining
  read, validation consumer, segment-source input, publisher, scorer,
  SQL/status projection, or oracle consumer.
- Delete only the frontier file/starts/counters, producer writes, rewind, and
  finish/abort cleanup. Retain the per-document weight sort that selects the
  semantic budget, the atom-id sort of retained pairs, all unified rows and
  fingerprints, current segment-source construction, and relation-page
  publication.
- Product inventory must reject the removed frontier state. Qualify the full
  4m matrix and stop on any retained posting count, root/signature,
  score/order, lifecycle, lock/WAL, RSS, or BM25 difference.

ARCH-3 slice 4n closure evidence:

- The zero-consumer semantic-frontier temp file, starts array, counters,
  producer writes, rewind, and cleanup are absent. Product inventory rejects
  the removed builder state. Weight selection, retained-pair atom ordering,
  unified rows and fingerprints, current segment source, relation-page
  publication, and the independent golden oracle remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4n-stage.KSa5AC`; full evidence is
  under `/tmp/ii42-arch3-4n-evidence.eKuRQr`. Source, staged, and installed
  dylibs share SHA-256
  `4ba0bef48348ec7de04b781889dc221f2f18832eac9753e562b9ea90128e8548`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`, real-model mutable lifecycle passes `71/71`, and
  transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
- PG18 and CMake builds are warning-clean; CTest passes `1/1`; inventory,
  modified-script `py_compile`, retired-name scan, and `git diff --check` pass.
  No public SQL, current root bytes, runtime signature, score/order, lock/WAL,
  completion/preload ownership, RSS authority, or BM25 contract changed.

ARCH-3 slice 4o dead-legacy-guard boundary:

- Five private `LEGACY_*` rebuild, overlay, and decoded-cache guard macros have
  no source consumer after slices 4a-4n. Delete only these definitions and make
  product inventory reject their return. Retain storage-version `2` recognition
  solely for explicit fail-closed diagnostics and v3 `REINDEX` recovery.
- Rebuild both products and require the dylib to remain byte-identical to 4n.
  If it does, reuse the complete 4n runtime matrix as exact executable evidence;
  otherwise stop and run the full matrix before accepting the slice.

ARCH-3 slice 4o closure evidence:

- All five zero-consumer legacy guard macros are absent and product inventory
  rejects their return. Retired storage-version `2` recognition remains only
  in the fail-closed boundary; it cannot select a build, query, mutation,
  maintenance, preload, or fallback implementation.
- The rebuilt dylib was not byte-identical to 4n because relinking changed its
  Mach-O UUID (`2E3C...` to `18F6...`), although its exported symbol set was
  identical. The artifact-identity shortcut was therefore rejected and the
  full runtime matrix was rerun rather than inferred.
- The fresh staged root is `/tmp/ii42-arch3-4o-stage.AAwGYw`; full evidence is
  under `/tmp/ii42-arch3-4o-evidence.bq92pw`. Source, staged, and installed
  dylibs share SHA-256
  `6def3c208dac8d7587022763c19f6c3422e20ffd0e81a51cbdc1d2788c936ac7`.
  Control and version SQL remain byte-identical across all three boundaries at
  SHA-256
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and
  `948bc86d0fa6d49a29078b24ed74c2faa40df31f95e14c42983ed3c13f2db634`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`, real-model mutable lifecycle passes `71/71`, and
  transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
  PG18/CMake builds, CTest `1/1`, inventory, modified-script `py_compile`, and
  `git diff --check` pass. No product behavior or public contract changed.

ARCH-3 slice 4p retired-query-telemetry boundary:

- The native semantic scorer returns 22 columns, but only the first 15 are
  populated from the page-native execution trace. The final seven
  decoded-overlay/cache counters are hard-coded to zero and have no current C,
  script, test, documentation, or policy consumer. They expose retired v2
  architecture through installed internal SQL without observing v3 behavior.
- Remove only those seven columns from both installed internal result types,
  their PL/pgSQL projection, and the C tuple contract. Retain rank, score,
  page-native candidate/posting work, rerank work, and bounded memory telemetry
  unchanged. Product inventory must reject the retired names and verify the
  15-column native contract.
- This is an internal installed-contract change, not a mechanical source move.
  Qualify a fresh staged PG18 package through schema/privilege checks,
  independent golden parity, native and real-model mutable lifecycle,
  transaction/2PC/restart, quarantine, replication, preload/restart,
  storage-v2 fail-closed recovery, and SQL regression. Stop if result identity,
  score, root bytes, lock/WAL order, memory authority, or BM25 behavior changes.

ARCH-3 slice 4p closure evidence:

- Both installed internal result types and the C tuple contract now expose only
  the 15 current page-native fields. The seven decoded-overlay/cache fields and
  their zero placeholders are absent; product inventory rejects their return
  and locks the native tuple width to 15. Public `ii42_query` behavior and its
  three-column result remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4p-stage.kAwHLb`; full evidence is
  under `/tmp/ii42-arch3-4p-evidence.9tq3sL`. Source, staged, and installed
  dylibs share SHA-256
  `b2e2253374920e58d4c50b2d00a875650f0ab738ac2ac4d79b2fee3d7ef061c7`.
  Source, staged, and installed version SQL share SHA-256
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Schema and runtime-service privilege smokes pass. Independent golden parity
  retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
  PG18/CMake builds, CTest `1/1`, inventory, modified-script `py_compile`, and
  `git diff --check` pass. Result identity, score, root authority, lock/WAL,
  bounded memory, runtime ownership, and BM25 behavior did not change.

ARCH-3 slice 4q dead-flat-cache-state boundary:

- Every product cache entry is loaded by
  `ii42_am_get_cached_segment_index`, which allocates a checked
  `segment_snapshot` and aliases its index metadata. No remaining producer
  assigns a flat generation block, vocabulary offsets, owned flat index,
  materialized delta overlay, or nested overlay cache entry.
- Remove only those zero-producer fields, their reset/match/release branches,
  and the unreachable offset-based vocabulary accessor. Preserve snapshot
  sensitivity, segment ownership, sorted vocabulary lookup, workspace leases,
  and all page-native scoring unchanged.
- This mechanical state deletion is separate from removing the still-compiled
  flat sparse scorer. Require warning-clean PG18/CMake builds, inventory and
  full staged lifecycle/recovery/replication gates before committing it.

ARCH-3 slice 4q closure evidence:

- Flat generation block/offset ownership, nested/materialized overlay state,
  and owned-flat-index cleanup are deleted. Cache entries now express their
  actual product ownership: one checked segment snapshot, current metadata,
  sorted vocabulary projection, bounded workspace, and a lease count.
  Snapshot-sensitive active-L0 entries retain their existing no-reuse rule.
- The fresh staged root is `/tmp/ii42-arch3-4q-stage.aA9EtX`; full evidence is
  under `/tmp/ii42-arch3-4q-evidence.jNjVKp`. Source, staged, and installed
  dylibs share SHA-256
  `4760633bb93703d8565244a1f376327365fc51e23ca3953742cdbcb126bd1727`.
  Version SQL and control remain byte-identical to 4p at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- The staged 5,000-to-80,000-document backend-memory gate passes with
  index-scaled private-live writable growth of `5,963,776` bytes below the
  `16,777,216`-byte ceiling and zero II42 memory-context growth. RSS, physical
  footprint, and allocator-empty pages remain observational PostgreSQL cache
  state, not retained index-sized backend authority.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, and staged SQL regression `1/1` pass.
  PG18/CMake builds, CTest `1/1`, inventory, modified-script `py_compile`, and
  `git diff --check` pass. No score, root, lock/WAL, memory, runtime ownership,
  SQL, or BM25 behavior changed.

ARCH-3 slice 4r unreachable-flat-scorer boundary:

- The only cache loader requires convergent-segment storage and returns an
  allocated `segment_snapshot`; no remaining build, migration, test hook, or
  fallback constructs a cache entry backed only by flat
  `indptr/indices/data`. Consequently the `legacy_sparse_path`, its signed
  flat scorer, field scorer, and ordered-scan branch are unreachable product
  code rather than a BM25 compatibility surface.
- Delete the flat-path predicates and branches. Make posting cursors and term
  cardinality read the checked segment snapshot only. Preserve the current
  segment impact sparse optimization, block-max scorer, generic mixed-extent
  scorer, predicate verification, tie-break order, and full-overlay oracle.
- Let warning-clean compilation identify newly dead flat-only helpers and
  remove them in this same mechanical slice. Product inventory must reject the
  legacy predicate/fields. Run the complete fresh staged matrix and stop on
  any rank, score, root, lock/WAL, memory, lifecycle, or BM25 difference.

ARCH-3 slice 4r closure evidence:

- The unreachable flat predicates, cursor state, signed candidate/all-document
  scorer, field-aware flat scorer, and ordered-scan fallback are deleted.
  Posting cursors and term cardinality now require the checked segment
  snapshot. The segment impact sparse path, block-max and mixed-extent
  scorers, predicate verification, tie-break order, and full-overlay oracle
  remain the only product behavior and are unchanged. Inventory rejects all
  retired flat entry points and state names.
- The fresh staged root is `/tmp/ii42-arch3-4r-stage.QxQdVl`; full evidence is
  under `/tmp/ii42-arch3-4r-evidence.YUE6aU`. Source, staged, and installed
  dylibs share SHA-256
  `ac9db54cb235c789249ddaff587b3cd517ea51012cdefc25040431050aadcb79`.
  Version SQL and control remain byte-identical to 4q at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 fail-closed
  boundaries with explicit v3 `REINDEX`, schema and privilege smokes, and
  staged SQL regression `1/1` pass. Warning-clean PG18/CMake builds, CTest
  `1/1`, inventory, modified-script `py_compile`, and `git diff --check` pass.
  No score, rank, root, lock/WAL, memory, lifecycle, SQL, or BM25 behavior
  changed.

ARCH-3 slice 4s retired-layout-normalization boundary:

- No product builder can publish generation-delta storage, and every installed
  query, mutation, VACUUM, maintenance, status, signature, and preload boundary
  rejects a non-v3 metapage before it can consume physical state. The remaining
  generation-delta normalizer therefore interprets an unsupported page layout
  without providing migration or recovery authority.
- Stop parsing active, delta, and semantic block ranges for retired storage.
  A non-v3 metapage must only be marked `REBUILD_REQUIRED`; the existing
  boundary guard continues to emit the frozen `0A000` error and explicit v3
  `REINDEX` hint. Preserve the metapage struct and every serialized byte so an
  explicit rebuild remains the sole recovery operation.
- Retain the complete v3 root normalization exactly: reject nonzero retired
  physical fields, validate the fixed read-root blob, and reject a published
  high-water mark beyond the relation. Remove only the range-overlap helper and
  storage-v2 constant that lose their final runtime caller.
- Freeze the deletion in product inventory. Qualify the same staged package
  through the independent golden, native and real-model mutable lifecycle,
  transaction/2PC/restart, quarantine, replication, preload/restart,
  storage-v2 fail-closed recovery, schema/privilege, and SQL-regression gates.
  Stop on any score, root byte, lock/WAL, RSS, lifecycle, or BM25 change.

ARCH-3 slice 4s closure evidence:

- Runtime metapage normalization no longer recognizes or validates
  generation-delta block ranges. Every non-v3 version is marked rebuild-only;
  the v3 fixed-root checks and serialized metapage layout are unchanged.
  Product inventory rejects the retired storage constant and range-overlap
  helper, while the physical version-2 fixture remains an independent negative
  input rather than runtime compatibility code.
- The fresh staged root is `/tmp/ii42-arch3-4s-stage.v9QgmQ`; full evidence is
  under `/tmp/ii42-arch3-4s-evidence.ywgzIY`. Source, staged, and installed
  dylibs share SHA-256
  `d6e95f351ab3461d03bc237e25c7f002ca4fc9b73950d8bc82fd4b4fa5262a8f`.
  Version SQL and control remain byte-identical at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- The physical fixture rejects all 12 installed boundaries, leaves background
  scheduling at zero, and recovers `2 -> 3`, ten query hits, and one subsequent
  INSERT only through explicit `REINDEX`. Independent golden parity retains
  manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`. Quarantine, physical
  replication, exact-root preload, standby replay/auto-preload, runtime-service
  restart, schema and privilege smokes, and staged SQL regression `1/1` pass.
  Warning-clean PG18/CMake builds, CTest `1/1`, inventory, modified-script
  `py_compile`, and `git diff --check` pass. No score, root, lock/WAL, memory,
  lifecycle, SQL, or BM25 behavior changed.

ARCH-3 slice 4t post-guard-dispatch boundary:

- A caller audit of every `ii42_am_require_convergent_segment_storage()` site
  identifies five projections that guard v3 and then retain an unreachable v2
  branch: exact-BM25 capability, relation description, policy debt,
  generation signature, and index details. Remove only those dead branches and
  read v3 object bytes, manifest contract hash, and linked-L0 debt directly.
- Three owner-only query diagnostics likewise test storage-v3 again after the
  guard. Remove that constant conjunct only. Their alternative path remains a
  current checked-segment path for field-aware or explicit weight-mask scoring;
  it is not a storage fallback and must not be deleted.
- Keep empty-index runtime-signature handling, all capability errors, manual
  BM25 stale-root behavior, result columns, scorer selection, locks, snapshots,
  and cleanup unchanged. Freeze direct reads of retired physical metapage fields
  in product inventory without removing their serialized compatibility slots.
- Qualify this mechanical control-flow deletion through the same fresh staged
  golden, lifecycle, recovery, replication, preload, ACL, storage-boundary, and
  SQL-regression matrix. Stop on any externally visible or authority change.

ARCH-3 slice 4t closure evidence:

- The five guarded projections now consume only checked v3 object bytes,
  manifest contract identity, and linked-L0 mutation debt. Owner-only query
  diagnostics retain their current field-aware and explicit-weight paths but
  no longer retest the already-established storage version. Product inventory
  rejects direct runtime projections from retired physical metapage fields;
  their serialized compatibility slots remain byte-for-byte unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4t-stage.hqvGBu`; evidence is under
  `/tmp/ii42-arch3-4t-evidence.lCAlX9`. Source, staged, and installed dylibs
  share SHA-256
  `bb83ac1dc429999ff083fd3fcd52546859fcc1fc05f9b19111e159c4cf13bbf8`.
  Version SQL and control remain byte-identical at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 rejection boundaries
  plus explicit `REINDEX` recovery, schema and privilege smokes, and staged SQL
  regression `1/1` pass. Warning-clean PG18/CMake builds, CTest `1/1`,
  inventory, modified-script `py_compile`, and `git diff --check` pass. No
  score, root, lock/WAL, memory, lifecycle, SQL, or BM25 behavior changed.

ARCH-3 slice 4u legacy-identity-helper boundary:

- Both callers of `ii42_am_generation_identity_matches()` compare either a
  shared warm/fold entry or the private checked-segment oracle cache against a
  metapage already constrained to v3. Remove its unreachable comparison of
  active-generation, base, and semantic-v2 fields; v3 cache identity remains
  the fixed read-root blob plus the existing relation locator and kind guards.
- Repository-wide caller analysis finds no use of the exported
  `ii42_unified_delta_record_maybe_committed()`, batch variant, or
  `ii42_unified_delta_current_xact_modified()`. Remove these dead compatibility
  helpers and their header declarations. Keep the current page-query linked-L0
  visibility classifier and transaction writer barrier unchanged.
- Do not rename or move the live linked-L0 append lock in this slice. Current
  helpers that still carry `unified_delta` naming are a separate mechanical
  authority-naming pass so deletion and broad symbol movement remain isolated.
- Freeze the removed identity branch and dead exports in product inventory,
  then run the full fresh staged golden/lifecycle/recovery/replication/preload,
  ACL, storage-boundary, and SQL-regression matrix before committing.

ARCH-3 slice 4u closure evidence:

- Generation identity now accepts only checked v3 storage and compares its
  fixed read-root blob after the existing cache-epoch, source-type, relation
  locator, and entry-kind guards. The unreachable v2 payload identity fields
  and three zero-caller compatibility exports are absent from both source and
  semantic header; current linked-L0 XID visibility is unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4u-stage.RFHm67`; evidence is under
  `/tmp/ii42-arch3-4u-evidence.Qhcp7B`. Source, staged, and installed dylibs
  share SHA-256
  `3967aa2fe288a8c6ade279f6fad43e26f0d6241412192ffad0234465e8144931`.
  Version SQL and control remain byte-identical at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 rejection boundaries
  plus explicit `REINDEX` recovery, schema and privilege smokes, and staged SQL
  regression `1/1` pass. PG18 and CMake builds, CTest `1/1`, inventory,
  modified-script `py_compile`, and `git diff --check` pass. No score, root,
  lock/WAL, memory, lifecycle, SQL, or BM25 behavior changed.

ARCH-3 slice 4v live-authority naming boundary:

- The remaining `unified_delta` names no longer describe retired storage.
  They name current linked-L0 append serialization, semantic completion atom
  normalization, lexical atom-frequency extraction, and document
  fingerprints. Rename them to those exact authorities so later ARCH-4 module
  boundaries cannot mistake current code for a compatibility lifecycle.
- Keep the append lock implementation and lock tag byte-for-byte equivalent,
  but make it private to the AM instead of exporting two wrappers used by no
  other translation unit. Keep current linked-L0 XID visibility, semantic
  batch SQL, atom ordering/deduplication, fingerprint width, and all error
  behavior unchanged.
- Remove the duplicate fingerprint-width macro and use the canonical document
  COW width from `ii42_segments.h`. Freeze absence of stale authority names in
  product inventory without rejecting the independent negative fixture or the
  current model ABI `ii42_p2_unified_text_atoms_v2`.
- Qualify this mechanical rename with a fresh staged build and the complete
  golden, lifecycle, recovery, replication, preload, ACL, storage-boundary,
  and SQL-regression matrix before an independent commit.

ARCH-3 slice 4v closure evidence:

- Current source and internal headers contain no `unified_delta` authority
  names. Semantic/lexical atom normalization and document fingerprints now use
  their COW/linked-L0 vocabulary; append serialization is AM-private while
  retaining the same advisory tag and acquire/release sequence. The current
  model ABI and independent negative storage fixture remain unchanged.
- The fresh staged root is `/tmp/ii42-arch3-4v-stage.a8jKMC`; evidence is under
  `/tmp/ii42-arch3-4v-evidence.OQ4qdG`. Source, staged, and installed dylibs
  share SHA-256
  `659b55f0e8186dfdd4ceeafa4f2ccf23f00b1d579e94a14d80365a77017927f0`.
  Version SQL and control remain byte-identical at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 rejection boundaries
  plus explicit `REINDEX` recovery, schema and privilege smokes, and staged SQL
  regression `1/1` pass. PG18 and CMake builds, CTest `1/1`, inventory,
  modified-script `py_compile`, and `git diff --check` pass. No score, root,
  lock/WAL, memory, lifecycle, SQL, or BM25 behavior changed.

ARCH-3 slice 4w closure evidence:

- Rebuild admission now consumes one typed workload rather than retired
  metapage lengths. A healthy v3 root supplies checked reachable object bytes,
  live document identity bytes, and current linked-L0 debt; first build and
  explicit repair of an unsupported or marked-damaged layout retain the
  conservative heap proxy. Existing saturating arithmetic, builder
  multipliers, headroom, caps, and explicit-build fallback are unchanged.
- Focused PG18 probes prove a nonempty BM25 v3 root reports nonzero `6:4:2`
  standard/compact/spill estimates, selects both low-memory admission windows,
  completes explicit `REINDEX`, and preserves query results. The real-model
  SAE probe reports the exact fixed/document workspace and increases its
  semantic-stream estimate by four times the same-snapshot linked-L0 bytes;
  worker-driven completion and explicit maintenance both satisfy the bounded
  convergence contract.
- The fresh staged root is `/tmp/ii42-arch3-4w-stage.m3FUaO`; evidence is under
  `/tmp/ii42-arch3-4w-evidence.TrYCEu`. Source, staged, and installed dylibs
  share SHA-256
  `f146e385776d3da9a340f2488e2c32b728ff69df9ee344bfa340bd0f5c9bd02c`.
  Version SQL and control remain byte-identical at
  `c07c9b57e2ec59a21e081cf4fb8e3a9e6579973dfef05ab276326e89d7ed69ed`
  and
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload, standby replay and
  auto-preload, runtime-service restart, all 12 storage-v2 rejection boundaries
  plus explicit `REINDEX` recovery, schema and privilege smokes, and staged SQL
  regression `1/1` pass. PG18 and CMake builds, CTest `1/1`, inventory,
  modified-script `py_compile`, and `git diff --check` pass. The intended
  correction is nonzero, authority-correct rebuild accounting; score, root
  bytes, lock/WAL order, query behavior, and builder implementation do not
  change.

ARCH-3 slice 4x diagnostic-snapshot contract:

- The 4w real-model probe exposed an observability race: the SQL-language
  `ii42_generation_cache_state_json()` wrapper references one STABLE C function
  many times through an inlineable CTE. Under concurrent worker completion,
  PostgreSQL can reevaluate that expression, so `raw_state` and JSON fields
  parsed from it may describe different linked-L0 frontiers within one result.
- Materialize exactly one `ii42_generation_cache_state()` value and derive all
  structured fields from that immutable text value. Remove the unused
  `ii42_index_details()` CTE and cross join; it contributes no selected field
  and is a second diagnostic read, not a status authority.
- Preserve the JSON schema, C status function, privileges, volatility and
  parallel declarations, root/debt readers, and every maintenance behavior.
  Add a real-worker test that repeatedly compares raw cache epoch, document
  count, pending counts, linked-L0 records, and linked-L0 bytes with their
  structured JSON projections from the same returned object.
- This is one standalone SQL-contract correction before ARCH-4. Stop if the
  staged function shape, ACL/schema placement, result keys, query results, or
  lifecycle gates change beyond removal of the inconsistent side read.

ARCH-3 slice 4x closure evidence:

- Installed SQL now materializes exactly one raw generation-cache state and
  parses every JSON projection from that value. The unused index-details read
  is absent. The C diagnostic, result schema, ACL, volatility, root/debt
  readers, mutation lifecycle, and maintenance behavior are unchanged.
- The real-model runtime-service smoke checks the raw cache epoch, document
  count, pending write/delete counts, and linked-L0 record/byte debt against
  their structured fields before mutation, during worker completion, and after
  convergence. Repeated observations remained internally consistent.
- The fresh staged root is `/tmp/ii42-arch3-4x-stage.5VSOW1`; evidence is under
  `/tmp/ii42-arch3-4x-evidence.NXYr6Y`. Source, staged, and installed dylibs
  share SHA-256
  `f146e385776d3da9a340f2488e2c32b728ff69df9ee344bfa340bd0f5c9bd02c`.
  Version SQL is byte-identical across all three surfaces at
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Native v3 lifecycle passes `80/80`, real-model mutable lifecycle passes
  `71/71`, and transaction/2PC/restart passes `21/21`.
- Quarantine, physical replication, exact-root preload at 80k documents,
  standby replay and auto-preload, runtime-service restart, all 12 storage-v2
  rejection boundaries plus explicit `REINDEX` recovery, schema and privilege
  smokes, and staged SQL regression `1/1` pass. PG18 and CMake builds, CTest
  `1/1`, inventory, modified-script `py_compile`, and `git diff --check` pass.
  No score, root bytes, lock/WAL order, RSS ownership, query result, or BM25
  behavior changed.

ARCH-3 slice 4y installed-boundary absence contract:

- The source, installed-version SQL, Mach-O export table, and compiled-string
  audit now find no generation-delta builder, scorer, mutation, maintenance,
  decoded-cache, or fallback runtime. `Makefile` installs only the current
  `ii42--0.2.0.sql`; the current binary exports only the native semantic scorer.
- Three remaining design phrases are stale or ambiguous: a slot-reuse gate is
  described as preserving `v2/v3` rows, a benchmark revision is called the
  `v2 gate`, and zero-score ordering claims unsupported v2 roots remain
  readable. Replace them with current v3/control terminology and state the
  actual boundary: any non-v3 root fails closed and only explicit `REINDEX`
  publishes a current root.
- Retain the current model ABI string `ii42_p2_unified_text_atoms_v2`, ONNX
  Runtime's CUDA-provider V2 API, and the isolated storage-version-2 corruption
  fixture. They are respectively an immutable model checkout contract, an
  upstream API name, and negative fail-closed evidence; none is an alternate
  index lifecycle. Retain page-native `generation_cache_*` diagnostics because
  they operate on exact root identity, shared markers, PostgreSQL buffers, and
  HOT_FOLD residency rather than a decoded generation payload.
- Extend static inventory to reject the stale public design claims and require
  the fail-closed v3 boundary. Then repeat source/SQL/binary absence commands,
  inventory, documentation checks, staged schema/privilege/regression gates,
  storage-v2 rejection plus `REINDEX` recovery, and the frozen golden.
- On closure, mark ARCH-3 checklist items 4 and 5 and CSG-I113 resolved. This is
  a documentation/static-contract commit with no score, root, lock/WAL, memory,
  API, SQL, or installed-binary change. ARCH-4 may begin only from that clean
  boundary.

ARCH-3 slice 4y closure evidence:

- Current product source and version SQL, compiled binary strings, and exported
  symbols contain no generation-delta or decoded-cache runtime, v2 builder,
  scorer, mutation, maintenance, preload, or fallback. `Makefile` installs one
  version SQL. The native 15-column semantic scorer is the only installed
  semantic query executor.
- Public design now distinguishes pre/post slot-reuse and repeated benchmark
  gates without using storage-version names. It states the implemented product
  boundary: all non-v3 roots fail closed for query, mutation, maintenance,
  preload, and status; explicit `REINDEX` is the sole migration to v3. Static
  inventory rejects regression of those claims.
- The current model ABI, upstream CUDA V2 provider API, and isolated
  storage-version-2 corruption fixture remain for their independent contracts.
  No shipped SQL or runtime dispatch treats them as an alternate lifecycle.
- The fresh staged root is `/tmp/ii42-arch3-4y-stage.edgbmu`; evidence is under
  `/tmp/ii42-arch3-4y-evidence.eVrjeK`. Source, staged, and installed dylibs are
  byte-identical to the fully qualified 4x artifact at SHA-256
  `f146e385776d3da9a340f2488e2c32b728ff69df9ee344bfa340bd0f5c9bd02c`.
  Version SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- Independent golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. All 12
  storage-v2 product boundaries reject the negative fixture and explicit
  `REINDEX` restores v3 plus correct query and insert behavior. Staged schema,
  privilege, and SQL regression `1/1`, PG18 and CMake builds, CTest `1/1`,
  inventory, modified-script `py_compile`, and `git diff --check` pass.
- CSG-I113 and ARCH-3 are closed. The 4x full lifecycle, replication, restart,
  preload, RSS, and exactness matrix applies byte-for-byte to this artifact;
  4y changes only design language, comments at fixed source line positions, and
  static regression checks.

### ARCH-4: Modularize The Access Method

Closes CSG-I114 through behavior-preserving extraction. Each slice is a
separate commit with a clean build, focused tests, and no intended SQL, score,
status, storage, lock, or memory-ownership change.

Proposed module boundaries:

| Module | Owns | Must not own |
| --- | --- | --- |
| `ii42_am.c` | AM handler/table, PostgreSQL callback forwarding, extension entry wiring | Storage algorithms, worker policy, query scoring, legacy dispatch |
| `ii42_am_options.c` | Reloptions, policy validation/recommendation, contract digest inputs | Runtime mutation or maintenance |
| `ii42_am_meta.c` | Metapage/root read-write validation, health/status projection, root identity | Query scoring or action selection |
| `ii42_am_build.c` | Initial build, `ambuildempty`, full `REINDEX` construction, builder cleanup | Incremental convergence |
| `ii42_am_mutation.c` | `aminsert` implementation, transaction-local queues, linked-L0 append/rotation requests | Semantic inference or compaction selection |
| `ii42_am_maintenance.c` | Debt classification, fair candidate/action selection, bounded action dispatch | AM callbacks or query execution |
| `ii42_am_semantic_maintenance.c` | Pending discovery, worker batches, completion/quarantine compare-and-append | Query fallback inference |
| `ii42_am_vacuum.c` | `ambulkdelete`, `amvacuumcleanup`, COW retirement/fence publication | General compaction policy |
| `ii42_am_scan.c` | `ambeginscan`/`amrescan`/`amgettuple` adapters, snapshots, visibility verification, result ownership | Posting codec or alternate scorer |
| `ii42_am_preload.c` | Exact-root registry, root markers, HOT_FOLD residency, preload/eviction telemetry | Durable index authority or model sessions |
| `ii42_am_sql.c` | Owner/ACL-checked SQL control and diagnostic entrypoints | Storage-format selection by JSON/text |

Extraction order:

1. Options/policy and SQL diagnostics, after their contracts are corrected.
2. Build and VACUUM, whose PostgreSQL callback boundaries are explicit.
3. Page-native scan adapters, reusing `ii42_page_query` as the sole scorer.
4. Mutation and transaction queues, preserving exact lock/WAL ordering.
5. Maintenance selection and semantic-maintenance orchestration.
6. Metapage/status and preload ownership, only after all callers are narrow.
7. Reduce `ii42_am.c` to AM registration and forwarding, then remove dead
   declarations, duplicate helpers, compatibility branches, and broad headers.

#### ARCH-4 Slice 1a: Reloption Authority Contract

The first extraction is deliberately smaller than the proposed options/policy
module. It moves only the PostgreSQL reloption schema, parser, defaults, and
definition-time validation into `ii42_am_options.c`. Policy recommendation,
GUC registration, runtime-contract calculation, status projection, and every
mutation, maintenance, root, WAL, and scan path remain in their current owners.

The new private `ii42_am_options.h` owns the immutable parsed layout
`ii42_am_options`, the `ii42_am_consistency` values stored in that layout, and
only these entrypoints:

- `ii42_init_reloptions()` registers the process-local reloption kind and its
  schema once during extension initialization;
- `ii42_amoptions()` is the PostgreSQL AM callback that builds and validates
  one relation's relcache-owned `rd_options` value.

`ii42_am_options.c` privately owns `ii42_relopt_kind`, the initialization flag,
the method and consistency enum members, the `relopt_parse_elt` table, option
name lookup, SAE/BM25 policy validation, and SAE eventual-default application.
Those definitions move rather than being copied. `ii42_am.c` continues to call
the two entrypoints and read the typed immutable layout; no second parser,
default, validator, or compatibility route may remain.

Ownership and dependency contract:

- the module acquires no relation lock, snapshot, root, buffer, WAL resource,
  runtime lease, or model session;
- `build_reloptions()` allocates the returned varlena under PostgreSQL's normal
  reloptions/relcache lifecycle; the module retains no per-index or backend
  copy and owns no cleanup callback;
- validation may allocate the temporary `DefElem` list in the current memory
  context and must free it before return or rely on PostgreSQL error cleanup;
- the module depends only on PostgreSQL reloptions/definition APIs plus the
  current BM25 method constants from `ii42_core.h`; it must not include AM
  implementation-private state;
- PGXS adds `ii42_am_options.o`. The PG-independent CMake core intentionally
  remains unchanged because it does not compile PostgreSQL AM modules.

Mechanical acceptance gate:

1. Source inventory finds exactly one reloption schema, parser, defaulting
   route, and validator, all in `ii42_am_options.c`; `ii42_am.c` retains only
   calls and typed `rd_options` reads.
2. Installed SQL, AM callback assignment, reloption names/defaults/errors,
   omitted-consistency SAE behavior, and BM25 realtime behavior are unchanged.
3. Page-native golden IDs/order/scores and manifest digest are unchanged;
   default-SAE CRUD/worker completion and BM25 lifecycle remain exact.
4. Source, staged, and installed dylibs from the slice are byte-identical to
   one another. A changed dylib relative to ARCH-3 is expected from object
   extraction, but any score, root-byte, SQL/control, lock/WAL, RSS, or measured
   BM25 baseline change stops the slice for diagnosis.
5. PG18 PGXS, CMake/CTest, isolated regression, extension schema/ACL, product
   inventory, modified-script `py_compile`, and `git diff --check` pass from
   fresh stage and evidence directories before the mechanical commit closes.

This contract is a planning-only commit. The subsequent mechanical move is a
separate commit and may not widen into policy or SQL-control extraction.

ARCH-4 slice 1a closure evidence:

- `ii42_am_options.c` is now the sole owner of reloption registration, schema,
  parsing, SAE eventual defaulting, and definition-time policy validation. Its
  private header exposes only the immutable relcache layout and the two planned
  entrypoints. `ii42_am.c` retains callback wiring and typed `rd_options` reads;
  policy recommendation, GUCs, status, runtime contracts, and all lifecycle
  authorities remain unmoved. The monolith is reduced by 475 lines without a
  second parser or compatibility route.
- The PGXS object is added without changing the PG-independent CMake core.
  Normal PostgreSQL 18 compilation is warning-clean; fresh CMake build and
  CTest pass `1/1`. `nm -gU` confirms neither private options entrypoint is a
  product export.
- The exact staged artifact at
  `/tmp/ii42-arch4-options-stage.KH5U5z` passes independent golden parity with
  manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Default-SAE lexical-first lifecycle passes `15/15` at
  `/tmp/ii42-arch4-options-evidence.AadKML/sae-lifecycle.json`; native BM25/v3
  lifecycle passes `80/80` at
  `/tmp/ii42-arch4-options-evidence.AadKML/native80.json`. Extension schema,
  runtime privilege, and isolated SQL regression `1/1` also pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `27fa22ca156f6ce04e01fab681229ba232f7931592f0ab89a2eded370339b7f3`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
  Product inventory now rejects reloption state or validators returning to the
  AM entry module; modified-script `py_compile` and `git diff --check` pass.

#### ARCH-4 Slice 1b: Immutable Option Projection Contract

The second options slice moves only relation-option interpretation into the
existing options authority. It does not move maintenance policy, recommendation
generation, runtime signatures, text tokenization, or SQL control.

Move these existing functions without changing names or behavior:

- `ii42_am_relation_has_explicit_reloptions()` as a read-only catalog
  projection;
- `ii42_am_get_consistency()` and `ii42_am_validate_relation_policy()`;
- `ii42_am_auto_preload_priority()`, `ii42_am_field_aware_enabled()`, and
  `ii42_am_sae_enabled()`;
- `ii42_am_read_params()` for validated BM25 parameters and empty-token policy.

Keep `ii42_am_eventual_policy_enabled()`, automatic/foreground maintenance
selection, policy recommendation, and every meta/debt reader in their current
owners. A recommendation depends on checked root and maintenance debt, so
placing it in the options module would invert the intended dependency graph.
The module table's options row therefore means definition and relation-policy
validation, not root-aware operational recommendation.

Boundary rules:

- callers pass an already-open `Relation`; the options module neither opens nor
  closes relations and acquires no index lock, snapshot, buffer, root, WAL,
  runtime lease, or model session;
- the explicit-reloptions projection may pin `pg_class` through
  `SearchSysCache1` and must release the tuple before return. It exposes only a
  boolean so the not-yet-extracted text-normalizer reader does not force an
  options-to-AM dependency. Parsed `rd_options` remains borrowed immutable
  relcache memory and is never retained;
- the only non-PostgreSQL dependency is the current `ii42_params`/BM25 method
  contract from `ii42_core.h`;
- the AM module may retain direct access only to model-path and text-normalizer
  offsets needed by later dedicated slices. It must no longer interpret SAE,
  consistency, auto-preload, field-aware, or BM25 numeric fields directly.

Mechanical acceptance gate:

1. Each moved projection has exactly one definition in `ii42_am_options.c` and
   no duplicate state, default, catalog check, or validation in `ii42_am.c`.
2. Temporary-index rejection, omitted/default consistency, SAE/BM25 conversion,
   field-aware checks, auto-preload selection, and BM25 finite-range errors are
   byte-for-byte behavior compatible.
3. Golden parity, default-SAE lifecycle `15/15`, native v3 lifecycle `80/80`,
   schema/ACL, isolated regression, PG18/CMake builds, inventory, `py_compile`,
   and artifact identity pass from fresh stage/evidence directories.
4. Any root, score/order, SQL/control, lock/WAL, memory-ownership, or BM25
   baseline change stops the slice. The implementation commit may not absorb
   text parsing, runtime-contract, recommendation, or maintenance code.

This is a planning-only contract and must be committed before the mechanical
projection move.

ARCH-4 slice 1b closure evidence:

- The seven immutable relation-option projections now have one owner in
  `ii42_am_options.c`; `ii42_am.c` retains calls but no direct interpretation
  of SAE, consistency, auto-preload, field-aware, or BM25 numeric fields. The
  explicit-reloptions helper returns only a boolean and releases its syscache
  tuple before return. Maintenance recommendation, root/debt state, text
  normalization, runtime signatures, GUCs, SQL control, locks, WAL, and model
  ownership remain unmoved. The AM entry module is reduced by another 203
  lines without a duplicate policy route.
- The exact staged artifact at
  `/tmp/ii42-arch4-option-projection-stage.N74fJO` passes independent golden
  parity at
  `/tmp/ii42-arch4-option-projection-evidence.kXtqSV/golden.json`, with
  manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`; real-model default-SAE lifecycle passes `15/15`,
  including the SAE eventual-only and BM25 realtime/default contract.
- Fresh PostgreSQL 18 PGXS compilation is warning-clean; fresh CMake/CTest
  passes `1/1`; extension schema, runtime privilege, and isolated SQL
  regression `1/1` pass. `nm -gU` confirms no moved projection is a product
  export. Product inventory rejects duplicate owners and direct AM field
  interpretation; modified-script `py_compile` and `git diff --check` pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `be9e6b2065897f4877d05ae8ca7e91c0f6c1dead70a8b6bd47066fcc8fa4e508`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 1c: Private Parsed-Layout Contract

The final options-only slice makes the PostgreSQL reloptions varlena layout an
implementation detail of `ii42_am_options.c`. It removes the last three direct
`rd_options` readers from the AM entry module without moving runtime or text
processing into the options authority.

Expose only two immutable projections:

- a borrowed per-index `model_path`, or `NULL` when the relation has no such
  option;
- a typed text policy containing lowercase, stemming, diacritic-folding, and a
  borrowed raw comma-separated stopword string. When reloptions are absent it
  returns the existing `true/false/false/NULL` defaults.

Boundary rules:

- returned strings remain owned by the relation's relcache `rd_options`; the
  caller must keep the already-open `Relation` valid and may neither retain nor
  free those pointers after relation/relcache lifetime;
- the options module allocates nothing and acquires no lock, snapshot, catalog
  tuple, buffer, root, WAL, runtime lease, or model session for either
  projection;
- the AM owner retains the `ii42.sae_model_path` fallback, missing-model error,
  checkout signature, stopword splitting/normalization, palloc/free lifecycle,
  and runtime-contract digest assembly;
- `ii42_am_options` and `GET_STRING_RELOPTION` become private to
  `ii42_am_options.c`. No generic string lookup API or second configuration
  representation is introduced.

Mechanical acceptance gate:

1. No source except `ii42_am_options.c` names `ii42_am_options`, dereferences
   `rd_options`, or uses `GET_STRING_RELOPTION`; product inventory enforces the
   boundary.
2. Both current model-path call sites and the index text-policy reader consume
   the typed projections. No fallback, parser, normalizer, allocator, runtime
   signature, or caller behavior moves.
3. Golden parity, native v3 `80/80`, real-model SAE `15/15`, schema/ACL,
   isolated regression, warning-clean PG18, CMake/CTest, hidden-symbol,
   artifact-identity, inventory, `py_compile`, and diff gates all pass from
   fresh directories.
4. Any error-text, default, score/order, root, SQL/control, lock/WAL, RSS, or
   BM25 behavior change stops the slice.

This is a planning-only contract. The mechanical privacy move is a separate
commit and may not absorb text parsing, runtime contract, build, or SQL logic.

ARCH-4 slice 1c closure evidence:

- `ii42_am_options` and every varlena string-offset access are now private to
  `ii42_am_options.c`. The AM consumes only a borrowed model-path pointer and a
  typed text-policy value. The borrowed-pointer lifetime is relation/relcache
  bounded; no new allocation, retained state, lock, catalog lookup, root, WAL,
  runtime lease, or model ownership was introduced. GUC fallback, stopword
  parsing/normalization, runtime digest, and cleanup remain in their original
  owners. The AM entry module is reduced by another 24 lines.
- Product inventory scans every C source and rejects parsed-layout pointers,
  `rd_options` dereferences, or `GET_STRING_RELOPTION` outside the options
  authority. The options header no longer exposes the varlena layout, and
  `nm -gU` confirms neither new projection nor the private catalog helper is a
  product export.
- The exact staged artifact at
  `/tmp/ii42-arch4-private-options-stage.WeaHkk` passes independent golden
  parity at `/tmp/ii42-arch4-private-options-evidence.gqq4mk/golden.json`,
  with manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`; real-model default-SAE lifecycle passes `15/15`.
  Extension schema, runtime privilege, isolated SQL regression `1/1`, fresh
  CMake/CTest `1/1`, warning-clean PG18, inventory, `py_compile`, and diff
  checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `c944ca726a7597da110d4ff96d337f2d7f0c4e288f8d9d16c4501ab38fc7c6be`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 2a: Immutable Distance-Operator Contract

Start the SQL authority with the smallest dependency-closed product surface.
Move only the two immutable overlap entrypoints and their private array helpers
from `ii42_am.c` to a new `ii42_am_sql.c`:

- `ii42_score_ids_op()` and `ii42_score_tokens_op()`;
- the private int4/text overlap implementations;
- the private text-array element-type validator used only by the text overlap
  implementation.

This slice does not move match, prepared-query, highlight, snippet,
normalization, tokenization, cache, status, preload, policy, or maintenance SQL
entrypoints. Those surfaces share query, text-policy, root, or runtime
authorities and require separate dependency contracts.

Boundary rules:

- the new module consumes only PostgreSQL array/type/Datum APIs and owns no
  relation, relcache, snapshot, buffer, root, lock, WAL, cache, model session,
  memory context, or retained backend state;
- the exact nested-loop membership semantics, NULL-element handling,
  one-dimensional validation, text/varchar acceptance, negative-distance
  convention, SQL symbol names, volatility, strictness, and parallel-safety
  contract remain unchanged;
- no generic SQL dispatch layer, callback table, AM-private header, or exported
  helper API is introduced. Only the two existing SQL symbols remain globally
  visible; all three helpers stay file-local;
- PGXS adds `ii42_am_sql.o`. The PG-independent CMake core remains unchanged.

Mechanical acceptance gate:

1. Each SQL symbol has exactly one owner in `ii42_am_sql.c`; neither wrapper nor
   helper remains in `ii42_am.c`. Product inventory enforces this ownership.
2. Existing int4[], text[], varchar[], mixed text/varchar, duplicate-token,
   NULL-element, empty-array, and invalid-shape behavior remains byte-for-byte
   compatible through installed SQL operators.
3. Golden parity, native v3 `80/80`, real-model SAE `15/15`, isolated SQL
   regression, schema/ACL, warning-clean PG18, CMake/CTest, symbol inventory,
   artifact identity, `py_compile`, and diff gates pass from fresh directories.
4. Any SQL definition, result/error, score/order, root, lock/WAL, RSS, or BM25
   baseline change stops the slice.

This is a planning-only contract. The source addition and exact mechanical
move are a separate commit.

ARCH-4 slice 2a closure evidence:

- `ii42_am_sql.c` now solely owns the two immutable distance-operator
  entrypoints and their three file-local array helpers. `ii42_am.c` contains no
  wrapper or helper copy, while the new module owns no relation, reloption,
  root, lock, WAL, cache, semantic-runtime, or retained-memory authority. The
  AM entry module is reduced by 237 lines without adding an inter-module API.
- Product inventory enforces one PGXS object owner, exact symbol ownership, and
  the forbidden-dependency boundary. Object and final-binary symbol checks show
  only `ii42_score_ids_op`, `ii42_score_tokens_op`, and their PostgreSQL finfo
  records exported; all three helpers remain file-local.
- The exact staged artifact at
  `/tmp/ii42-arch4-sql-operator-stage.2WXCmr` passes independent golden parity
  at `/tmp/ii42-arch4-sql-operator-evidence.iAvzr9/golden.json`, with manifest
  SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`; real-model default-SAE lifecycle passes `15/15`;
  isolated SQL regression passes `1/1`.
- Extension schema and runtime privilege smokes pass against the staged PG18
  package. A separate isolated-cluster edge smoke preserves duplicate-query,
  NULL-element, strict-NULL, mixed text/varchar, empty-array rejection, and
  multidimensional rejection behavior. Fresh warning-clean PG18 compilation,
  CMake build and CTest `1/1` at
  `/tmp/ii42-arch4-sql-operator-cmake.stq40A`, inventory, modified-script
  `py_compile`, and diff checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `f1e5bbe45915e260d6b73f15147b1e5fc9cb3d265c1aed8c92c57c37480bb03d`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 2b: PostgreSQL Array-Value Contract

Before moving the remaining dependency-closed SQL SRF, consolidate its three
shared value constructors into the existing `ii42_pg_common` conversion
authority. Move and rename only:

- `ii42_am_float4_array_from_values()` to
  `ii42_array_from_float4_values()`;
- `ii42_am_int4_array_from_values()` to `ii42_array_from_int4_values()`;
- `ii42_am_text_array_from_cstrings()` to
  `ii42_array_from_cstrings()`.

The float4 and int4 constructors are currently used only by hybrid-result
projection. The text constructor is also used by prepared-query resolution,
normalization, and tokenization, so copying it into `ii42_am_sql.c` would create
a second conversion authority. `ii42_pg_common` already owns the reverse
PostgreSQL-array readers and is the narrow existing dependency shared by the AM
and SQL modules. No new module or generic conversion framework is needed.

Boundary rules:

- each function allocates the returned `ArrayType` in the caller's current
  memory context, frees only its temporary `Datum` vector, retains no pointer,
  and acquires no relation, snapshot, root, lock, WAL, cache, runtime, or model
  authority;
- zero length continues to return a correctly typed PostgreSQL empty array;
  non-empty values preserve exact element OID, length, by-value, and alignment
  metadata. Text inputs remain borrowed NUL-terminated C strings and are copied
  into PostgreSQL text Datums;
- the header exposes only these three typed constructors beside the existing
  typed readers. It must not expose AM structs, lifecycle state, allocator
  callbacks, untyped Datum dispatch, or ownership transfer;
- all existing callers change names in the same mechanical commit. SQL
  definitions, scores, errors, ordering, and product API remain unchanged.

Mechanical acceptance gate:

1. Each constructor has exactly one implementation in `ii42_pg_common.c` and
   one declaration in its private product header. No constructor copy or old
   `ii42_am_*_array_from_*` name remains.
2. Normalization, tokenization, prepared-query resolution, and the full hybrid
   fusion SQL matrix preserve exact arrays, rows, ordering, errors, NULL and
   empty behavior through the isolated SQL regression.
3. Golden parity, native v3 `80/80`, real-model SAE `15/15`, schema/ACL,
   warning-clean PG18, CMake/CTest, inventory, `py_compile`, artifact identity,
   and diff gates pass from fresh directories.
4. Any SQL/control, score, root, lock/WAL, allocation-lifetime, RSS, or BM25
   baseline change stops the slice. The implementation commit may not move the
   hybrid SRF or any query/text algorithm.

This is a planning-only contract. The three constructors and their direct call
sites move in a separate mechanical commit.

ARCH-4 slice 2b closure evidence:

- `ii42_pg_common.c` now solely owns the three typed PostgreSQL array-value
  constructors, with the smallest typed declarations in `ii42_pg_common.h`.
  All AM callers use the shared names; neither the old names nor duplicate
  implementations remain. Object inspection places all three definitions in
  `ii42_pg_common.o`, none in `ii42_am.o`, and hidden visibility keeps them out
  of the final dylib's public symbols.
- The move retains current-memory-context ownership of returned arrays, frees
  temporary Datum vectors, and introduces no retained state or relation, root,
  lock, WAL, cache, runtime, or model authority. `ii42_am.c` is reduced by 91
  further lines, to 47,124, without moving the hybrid SRF or query/text logic.
- The exact staged artifact at
  `/tmp/ii42-arch4-array-values-stage.d82aKs` passes golden parity at
  `/tmp/ii42-arch4-array-values-evidence.Y3dvZd/golden.json`, with manifest
  SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Real-model
  default-SAE lifecycle passes its current expanded `71/71`; isolated SQL
  regression passes `1/1`.
- A first native smoke run executed concurrently with three other isolated
  clusters exhausted its fixed-live-set maintenance retry window at
  `xid_horizon`. The same staged artifact then passed native v3 `80/80` twice
  consecutively when run alone, at `native80-rerun.json` and
  `native80-confirm.json` in the evidence directory. No product or test logic
  was changed to mask the scheduling-sensitive first result.
- Extension schema and runtime privilege smokes pass against the staged PG18
  package. Fresh warning-clean PG18 compilation, CMake build and CTest `1/1`
  at `/tmp/ii42-arch4-array-values-cmake.7093Lj`, inventory, modified-script
  `py_compile`, and diff checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `1307e37e67968ab9e627895b3166a9e95b17c94c9ef8799368388f194fbf6fc4`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 2c: Value-Only Hybrid Fusion Contract

Move the dependency-closed `ii42_hybrid_fuse_candidates()` SQL SRF, its private
enums and four state structs, and only its `ii42_hybrid_*` value-processing
helpers from `ii42_am.c` into the existing `ii42_am_sql.c` SQL-value authority.
The routine consumes an array of `ii42_result_hybrid_candidate` composite
values, normalizes and deduplicates them, and projects
`ii42_result_hybrid_hit` rows. It does not generate candidates or acquire an
index, relation, root, snapshot, lock, WAL, cache, runtime, model, worker, or
maintenance authority.

Boundary rules:

- preserve the exact exported C symbol, PostgreSQL finfo record, SQL signature,
  `STABLE PARALLEL SAFE` contract, default arguments, error strings, ordering,
  tie breaks, normalization math, duplicate handling, score projection, NULL
  handling, and empty-result behavior;
- keep every fusion enum, state type, comparator, parser, deform/build helper,
  and source/hit buffer file-local in `ii42_am_sql.c`. Add no public header or
  inter-module callback;
- retain SRF-owned memory in `multi_call_memory_ctx`. Per-hit arrays continue
  to be constructed through the typed `ii42_pg_common` value API. The SRF may
  retain no state after its multi-call context is released;
- do not move `ii42_hybrid_candidate()`, BM25/vector candidate SQL wrappers,
  prepared-query resolution, tokenization, normalization, direct index search,
  or any scorer/index implementation. Those are distinct authorities even
  though their public SQL names include `hybrid`;
- add only the PostgreSQL and C-library includes required by this exact closure.
  `ii42_am_sql.c` must remain unable to include AM-private headers or access AM
  globals.

Mechanical acceptance gate:

1. `ii42_hybrid_fuse_candidates` has one implementation and one finfo owner in
   `ii42_am_sql.c`; no fusion state type or helper remains in `ii42_am.c`.
2. Product inventory enforces exact symbol ownership and rejects relation,
   root, lock/WAL, cache, runtime/model, semantic-maintenance, worker, or AM
   private-option dependencies in the SQL-value module.
3. The isolated full SQL regression preserves every fusion method,
   normalizer/direction, deduplication, rank, tie, NULL, empty, error, and
   output-array case. Golden parity, native v3 `80/80`, current real-model SAE
   lifecycle, schema/ACL, warning-clean PG18, CMake/CTest, artifact identity,
   inventory, `py_compile`, and diff gates pass from fresh directories.
4. Any SQL/control, score/order, root, lock/WAL, allocation-lifetime, RSS, or
   BM25 baseline change stops the slice. The implementation commit may contain
   no other SQL routine or AM authority move.

This is a planning-only contract. The exact dependency closure moves in a
separate mechanical commit.

ARCH-4 slice 2c closure evidence:

- `ii42_am_sql.c` now solely owns the hybrid fusion finfo/entrypoint, three
  private enums, four private state structs, and every `ii42_hybrid_*` value
  helper. `ii42_am.c` contains no `ii42_hybrid_*` symbol. The moved type block,
  19,347-byte helper block, and 3,316-byte SRF wrapper compare byte-for-byte
  with their pre-move source; only required includes and ownership location
  changed.
- Object inspection places the exported entrypoint and finfo record in
  `ii42_am_sql.o`, none in `ii42_am.o`; helpers remain local symbols. Inventory
  enforces this ownership and rejects relation, root, snapshot, lock/WAL,
  cache, runtime/model, worker, maintenance, AM-option, page-query, segment, or
  storage dependencies in the SQL-value module.
- `ii42_am.c` is reduced by 939 further lines, to 46,185. No SQL routine,
  candidate generator, prepared-query path, scorer, relation lifecycle, or AM
  authority moved with the fusion closure.
- The exact staged artifact at
  `/tmp/ii42-arch4-hybrid-sql-stage.iVOSnR` passes golden parity at
  `/tmp/ii42-arch4-hybrid-sql-evidence.hnfTfv/golden.json`, with manifest
  SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80`; real-model default-SAE lifecycle passes `71/71`;
  isolated full SQL regression passes `1/1`.
- Extension schema and runtime privilege smokes pass against the staged PG18
  package. Fresh warning-clean PG18 compilation, CMake build and CTest `1/1`
  at `/tmp/ii42-arch4-hybrid-sql-cmake.9NANjL`, inventory, modified-script
  `py_compile`, and diff checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `0b782029689a109eab8b6213a9ba2f812782c5b1eb84396c61a321af749ddb02`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 3a: Checked Metapage Read Projection Contract

Create `ii42_am_meta.c/.h` as the single checked metapage read-projection
authority. Move only the immutable v3 metapage wire struct and constants plus:

- `ii42_am_meta_uses_convergent_segment_storage()`;
- `ii42_am_require_convergent_segment_storage()`;
- `ii42_am_segment_read_root_from_meta()`;
- `ii42_am_normalize_meta_storage()`;
- `ii42_am_read_meta()`;
- `ii42_am_relation_nblocks()`;
- `ii42_am_meta_has_pending_maintenance()`.

The private product header may expose the fixed value struct because existing
build, mutation, maintenance, VACUUM, scan, preload, and SQL-control owners
consume a copied metapage snapshot. It must expose no `Buffer`, `Page`, lock,
WAL record, mutable global, cache entry, callback, or publication primitive.

Boundary rules:

- preserve the exact on-page field order, widths, fixed root bytes, magic,
  version, page kind, flags, storage version, static size assertion, error
  text, and zero-block smgr-cache refresh behavior;
- `ii42_am_read_meta()` takes the metapage buffer share lock, validates the
  fixed header, copies the payload by value, normalizes only the copy, and
  releases the buffer before returning. Callers receive no page pointer or
  lock ownership;
- v3 normalization continues to reject every retired fixed-layout authority
  and an invalid or out-of-bounds serialized root by setting corrupt/rebuild
  flags on the copied snapshot only. Unsupported storage remains fail-closed;
- root deserialization remains the only projection from fixed metapage bytes
  to `ii42_segment_read_root`;
- do not move metapage initialization, root publication, flag/counter updates,
  append/writer locks, WAL, payload health/status JSON, rebuild admission,
  maintenance selection, preload, or any AM callback in this slice.

Mechanical acceptance gate:

1. The wire struct/constants and seven functions have one owner in
   `ii42_am_meta`; `ii42_am.c` retains no definition or private prototype.
2. The header contains only value ABI and typed read/projection declarations.
   Inventory rejects mutable authority, cache/runtime/model, semantic worker,
   maintenance, query/scorer, or publication dependencies in the meta module.
3. A focused staged edge smoke preserves empty-relation, malformed-header,
   unsupported-layout, retired-field, invalid-root, out-of-bounds root, and
   zero-block refresh behavior. Golden parity, native v3 `80/80`, real-model
   SAE lifecycle, full SQL regression, schema/ACL, warning-clean PG18,
   CMake/CTest, artifact identity, inventory, `py_compile`, and diff gates pass.
4. Any wire-byte, root projection, score, lock/WAL, lifecycle, RSS, SQL/control,
   or BM25 baseline change stops the slice. The implementation commit may not
   move a metapage writer or any other authority.

This is a planning-only contract. Module creation and the exact read closure
move in a separate mechanical commit.

ARCH-4 slice 3a closure evidence:

- `ii42_am_meta.h` now solely owns the 360-byte fixed metapage value layout,
  magic/version/page/flag/storage constants, and the smallest typed projection
  API. `ii42_am_meta.c` solely owns the checked share-lock read, copied-snapshot
  normalization, serialized-root projection, zero-block smgr refresh, and
  pending-debt predicate. Every moved function body, constant, wire field, and
  size assertion compares exactly with the pre-move source.
- Product inventory enforces one layout/constant/function owner, keeps
  normalization file-local, and rejects writer/WAL, lock-manager, cache,
  runtime/model, scheduler, publication, page-query, and storage authority in
  the metapage module. Object inspection places the six cross-module readers
  only in `ii42_am_meta.o`; `ii42_am.o` contains undefined references only and
  the final dylib exports no new public symbol. `ii42_am.c` is reduced by 268
  net lines to 45,917 without moving a writer or callback.
- The new permanent staged boundary smoke at
  `/tmp/ii42-arch4-meta-read-evidence.8z90dG/metapage-boundary.json` rejects a
  malformed header, one nonzero retired physical field, a checksum-invalid
  serialized root, an out-of-bounds root, and a zero-block relation with the
  exact current errors. A clean control index remains `10 -> 10` queryable
  across REINDEX. The smoke is wired into the maturity suite and inventory.
- Retired storage independently rejects all 12 installed boundaries and
  recovers `2 -> 3`, ten hits, and one INSERT at
  `/tmp/ii42-arch4-meta-read-evidence.8z90dG/storage-boundary.json`. Independent
  golden parity passes at
  `/tmp/ii42-arch4-meta-read-evidence.8z90dG/golden.json`, retaining manifest
  SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- Native v3 lifecycle passes `80/80`; the current real-model default-SAE
  lifecycle passes `71/71`; isolated full SQL regression passes `1/1` under
  `/tmp/ii42-arch4-meta-read-evidence.8z90dG/regression`. Schema and runtime
  privilege smokes, fresh warning-clean PG18, fresh CMake/CTest `1/1` at
  `/tmp/ii42-arch4-meta-read-cmake.kYOrNt`, inventory, modified-script
  `py_compile`, and diff checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `20d4f3dffad81712c87842bb4dec4142007cc035edc55c788b1542e226451166`.
  Installed SQL remains
  `628325edc3f61f9ff13ebbaa2e872f7dbddf0a40241459ab64a130b076d2d5f9`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.
- The older broad payload-health fixture exposed a pre-existing operator-read
  defect: after the relation is truncated to one page, diagnostics classify
  the checked root only after trying to load linked-L0 or manifest pages and
  therefore raise `invalid ii42 segment page reference`. The preceding
  accepted dylib and this slice fail identically, so this is not a slice-3a
  regression. ARCH-4 slice 3a.1 below closes the product defect without
  weakening checked page reads or mixing behavior into this mechanical commit.

#### ARCH-4 Slice 3a.1: Corrupt Operator-Read Fail-Safe Contract

Pure operator diagnostics must remain usable when the copied metapage already
proves that the v3 root is invalid, out of bounds, or explicitly corrupt. This
is a read-order correction, not a second recovery path: query, preload,
mutation, and maintenance execution continue to fail closed until explicit
`ii42_index_refresh()` or `REINDEX` publishes a valid root.

Boundary rules:

- `ii42_am_payload_health()` runs before any manifest, linked-L0, object-byte,
  document-summary, reachability, or warm-generation traversal in
  `ii42_index_details()`, generation status, and generation-cache state;
- healthy and merely stale/rebuild-marked roots retain their exact existing
  output and traversal behavior. Only `health.corrupt` takes the bounded
  diagnostic path;
- the bounded path may read the copied metapage, relation block count,
  reloptions, and process/shared-service counters. It must not fabricate
  manifest, mutation-debt, posting, semantic, reachability, or rebuild-workload
  facts. Unknown fields are null or omitted and the status explicitly reports
  incomplete diagnostics, `valid=false`, the health reason, and
  `rebuild_required=true`;
- `ii42_index_details()` reports physical relation bytes and null pending-debt
  fields for a corrupt root. It must not call active-generation object-byte or
  linked-L0 debt readers;
- `ii42_generation_cache_state()` returns a stable bounded corruption record
  containing health, expected/capacity bytes, flags, relation blocks, document
  snapshot, and locator. Existing regexp consumers must still recover the
  health fields;
- `ii42_index_status()` composes only those bounded results while the root is
  corrupt. The private textual relation description remains a post-publication
  maintenance result: its only callers invoke it after a successful rebuild,
  so this slice must not add a public diagnostic API or an unreachable corrupt
  formatting branch;
- no callback, score, root byte, lock order, WAL record, publication,
  scheduler, cache admission, or healthy-path SQL representation may change.

Acceptance gate:

1. A staged corruption smoke truncates a valid relation to one page and proves
   generation-cache state, generation status, index details, index status, and
   their JSON wrapper return bounded diagnostics without touching invalid
   segment pages.
2. The same corrupt root is rejected by native query and preload with the exact
   health reason; explicit refresh restores health and exact top-k results.
3. Healthy diagnostic output, golden rows/scores, native lifecycle, real-model
   SAE lifecycle, SQL regression, schema/ACL, warning-clean PG18, CMake/CTest,
   package identity, inventory, `py_compile`, and diff gates pass.

This is a planning-only contract. The fail-safe branch and strengthened staged
smoke belong to a separate implementation commit.

ARCH-4 slice 3a.1 closure evidence:

- Generation status, generation-cache state, index details, and the installed
  status wrapper now classify copied-metapage corruption before traversing a
  manifest, linked L0, mutation debt, object bytes, reachability, or rebuild
  workload. The bounded result marks diagnostics incomplete, reports the exact
  health reason, preserves only metapage/physical facts, and leaves unknown
  posting and debt fields null. Healthy and merely stale roots retain the prior
  path and shape.
- The staged one-page truncation gate passes at
  `/tmp/ii42-arch4-corrupt-status-final.json`: health is `corrupt`, reason is
  `segment_root_out_of_bounds`, relation size is 8,192 bytes, four debt fields
  are unknown, and query plus preload both fail closed. Explicit refresh then
  restores health and all ten expected hits. The maturity suite now binds this
  gate to its staged dylib and version SQL instead of the system installation.
- The exact final staged artifact passes independent golden parity at
  `/tmp/ii42-arch4-corrupt-golden-final.json`, preserving manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80` at `/tmp/ii42-arch4-corrupt-native-final.json`; the
  real-model lexical-first SAE lifecycle passes `15/15` at
  `/tmp/ii42-arch4-corrupt-sae-final.json`.
- Isolated SQL regression passes `1/1` under
  `/tmp/ii42-arch4-corrupt-regression-final2.9GTuEM`; schema and runtime-role
  privilege smokes pass. Warning-clean PG18, CMake/CTest `1/1` at
  `/tmp/ii42-arch4-corrupt-cmake.78lKXV`, modified-script `py_compile`, product
  inventory, hidden-symbol inspection, and diff checks pass.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `ad886d5af0533c7806b0f0596770aad7fe6be6752810c84137b1d299597b9be4`.
  Installed SQL is byte-identical at
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 3b: Payload-Health Projection Contract

Move only `ii42_am_payload_health_state` and
`ii42_am_payload_health()` into `ii42_am_meta.c/.h`. Health is an immutable
projection over one copied metapage plus the relation's physical block count;
it is not recovery, validation of the complete segment closure, or publication
authority.

Boundary rules:

- preserve exact zero initialization, status/reason precedence, expected and
  capacity byte calculations, root-deserialization result, out-of-bounds test,
  and stale/corrupt/rebuild flag interpretation;
- use the existing checked `ii42_am_relation_nblocks()` projection. The health
  function may deserialize the fixed copied root but must not load a manifest,
  linked L0, COW object, relation page beyond the metapage already read by the
  caller, mutation debt, cache state, or model/runtime state;
- expose only the fixed value result and a typed read-projection function.
  `ii42_am.c` retains call-site policy: whether corruption is diagnostic,
  fail-closed, refreshable, preloadable, or maintenance work;
- do not move payload status JSON, error construction, rebuild admission,
  relation description, query/preload/maintenance execution, metapage writers,
  root publication, locks, WAL, callbacks, or SQL entrypoints;
- the function becomes a hidden internal module symbol, never an installed C or
  SQL API. No second health implementation or compatibility wrapper remains.

Mechanical acceptance gate:

1. The health struct and function have one owner in `ii42_am_meta`; `ii42_am.c`
   has no definition or private prototype and only typed calls.
2. Inventory rejects forbidden traversal/publication/runtime dependencies in
   the health closure and rejects duplicate ownership. Object/symbol inspection
   proves the implementation resides only in `ii42_am_meta.o` and is not a
   public dylib symbol.
3. The metapage boundary and one-page corruption gates preserve every exact
   reason and recovery result. Golden parity, native `80/80`, real-model SAE
   lifecycle, SQL regression, schema/ACL, warning-clean PG18, CMake/CTest,
   artifact identity, inventory, `py_compile`, and diff gates pass.
4. Any output, reason, root/score byte, lock/WAL, lifecycle, memory, or BM25
   baseline change stops the slice. No neighboring metapage or publication
   helper may move to make the extraction easier.

This is a planning-only contract. The health projection moves in a separate
mechanical commit.

ARCH-4 slice 3b closure evidence:

- `ii42_am_meta.h` now solely owns the payload-health value result and typed
  projection declaration; `ii42_am_meta.c` owns the byte-identical function
  body. `ii42_am.c` has no duplicate type, definition, or private prototype and
  retains all policy at its 14 call sites. Object inspection shows one undefined
  reference in `ii42_am.o`, one implementation in `ii42_am_meta.o`, and no
  exported dylib symbol.
- Inventory enforces one owner and rejects manifest/segment-page, mutation-debt,
  rebuild, document-COW, runtime/model, writer/WAL, scheduler, cache, or
  publication authority in the meta module. The extraction reduces
  `ii42_am.c` to 45,933 lines without moving a callback or mutable authority.
- The final staged package at `/tmp/ii42-arch4-health-stage.IRLbkG` passes the
  complete metapage edge gate at `/tmp/ii42-arch4-health-meta.json` and the
  one-page bounded corruption/recovery gate at
  `/tmp/ii42-arch4-health-corrupt.json`. Every prior reason, fail-closed query
  and preload result, explicit refresh, and ten repaired hits remain exact.
- Independent golden parity passes at `/tmp/ii42-arch4-health-golden.json`,
  retaining manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `80/80` at `/tmp/ii42-arch4-health-native.json`; real-model
  lexical-first SAE lifecycle passes `15/15` at
  `/tmp/ii42-arch4-health-sae.json`.
- Isolated SQL regression passes `1/1` under
  `/tmp/ii42-arch4-health-regression.ggs2Em`; schema and runtime-role privilege
  smokes pass. Fresh warning-clean PG18, CMake/CTest `1/1` at
  `/tmp/ii42-arch4-health-cmake.mxiU9Q`, `py_compile`, product inventory,
  hidden-symbol inspection, and diff checks pass.
- A 40,000-document, five-trial A/B against the preceding staged artifact
  passes all 22 exactness/performance gates in both
  `/tmp/ii42-arch4-health-perf-old.json` and
  `/tmp/ii42-arch4-health-perf-new.json`. New/old median mean ratios across
  static-before, static-after, fragmented, workload-folded, impact, and static
  reference are `1.0166x`, `0.9997x`, `1.0201x`, `0.9961x`, `0.9945x`, and
  `1.0010x`; HOT_FOLD QPS is `1.0056x`. The mixed direction and bounded range
  show no systematic regression and stay inside the frozen 5% gate.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `ca7df44d63c2c5c7475ba854dd25ae50026b6b05aac52c7a09ea11e029a0acd0`.
  SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4a: Rebuild Memory Admission Contract

Begin the build authority with the dependency-closed resource-admission core,
not the relation build callback or maintenance scheduler. Move only the
rebuild-builder enum, measured-workload value, builder-name projection, and
pure standard/compact/spill/semantic-stream memory admission calculation into
new private `ii42_am_build.h` and `ii42_am_build.c` modules.

The existing owners continue to measure the workload and decide when work is
required. The explicit build path still handles the documented over-budget
fallback and `NOTICE`; the maintenance selector still decides whether to skip
an unaffordable action. Both pass immutable typed inputs to one admission
function:

- whether the relation uses semantic streaming;
- measured live-payload, identity-source, and linked-L0 mutation bytes;
- the configured rebuild-memory budget in MiB;
- the current PostgreSQL `work_mem` value in KiB.

Boundary rules:

- the build module opens no relation, heap, metapage, manifest, buffer,
  snapshot, model session, or runtime lease and acquires no lock or WAL
  resource;
- it owns no GUC or process-global state. Callers read the existing
  `ii42.maintenance_rebuild_memory_budget` and PostgreSQL `work_mem` values and
  pass them by value, so the module cannot become a second policy authority;
- convergent reachable-byte measurement, linked-L0 debt reading, conservative
  heap-size fallback, explicit-build fallback/notice, and automatic
  maintenance action selection remain in their current owners;
- overflow saturation, fixed semantic workspace, builder order, headroom,
  large-allocation guards, estimate outputs, names, and error/notice behavior
  remain unchanged. This slice does not tune any threshold or budget;
- the private header exposes only the enum, three-counter workload value, and
  two typed functions. It must not expose AM implementation state, relation
  callbacks, a generic policy interface, or an `extern` GUC;
- PGXS adds `ii42_am_build.o`. The PG-independent CMake core remains unchanged.

Mechanical acceptance gate:

1. The enum, workload value, memory estimator, and builder-name projection
   have exactly one owner in `ii42_am_build.*`; no calculation or constant copy
   remains in `ii42_am.c`.
2. Explicit CREATE INDEX/REINDEX and automatic maintenance select the same
   builders and emit the same status fields/notices at the same configured
   budgets. Focused compact, spill, over-budget, and semantic-stream gates pass.
3. The new module has no relation, meta, segment-page, runtime, lock, WAL, GUC,
   or retained-memory dependency; object/symbol inventory proves its private
   boundary.
4. Golden parity, native v3 `80/80`, real-model SAE lifecycle, schema/ACL,
   isolated regression, warning-clean PG18, CMake/CTest, artifact identity,
   inventory, modified-script `py_compile`, and `git diff --check` pass from
   fresh directories.
5. Any builder-selection, status, notice, score/order, root, lock/WAL, RSS, or
   BM25 performance change stops the slice for diagnosis.

This is a planning-only contract. The subsequent mechanical extraction is a
separate commit and may not absorb workload measurement, relation build,
publication, or maintenance dispatch.

ARCH-4 slice 4a closure evidence:

- `ii42_am_build.h` now solely owns the rebuild-builder enum and immutable
  three-counter workload value. `ii42_am_build.c` solely owns builder naming,
  overflow-saturating estimates, headroom, allocation guards, and
  standard/compact/spill/semantic-stream admission. The two callers pass the
  existing rebuild-memory budget and `work_mem` by value; relation workload
  measurement, explicit-build fallback/notice, and maintenance selection stay
  in `ii42_am.c`.
- Product inventory enforces one value/function/constant owner and rejects
  relation, metapage, segment, runtime/model, lock, WAL, GUC, allocation, or
  error-policy authority in the new module. Object inspection finds the two
  implementations only in `ii42_am_build.o`; the final dylib exports neither.
  `ii42_am.c` is reduced by 281 net lines to 45,652 without moving a callback,
  mutable state, publication, or scheduler decision.
- The focused compact and spill maintenance-builder smokes preserve both
  automatic status selection and explicit REINDEX selection. The staged
  runtime-service smoke now directly requires a 1 MiB semantic workload to
  report `rebuild_admitted=false` with builder `semantic_stream`, while bounded
  completion still converges and a later REINDEX remains valid.
- Independent golden parity passes at
  `/tmp/ii42-arch4-build-admission-golden.json`, retaining manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. The current
  native lifecycle passes `71/71` at
  `/tmp/ii42-arch4-build-admission-native.json`; the focused lexical-first SAE
  lifecycle passes `15/15` at
  `/tmp/ii42-arch4-build-admission-sae.json`.
- Isolated SQL regression passes `1/1` under
  `/tmp/ii42-arch4-build-admission-regression`; schema and runtime-role
  privilege smokes pass. Fresh warning-clean PG18, CMake/CTest `1/1` at
  `/tmp/ii42-arch4-build-admission-cmake`, modified-script `py_compile`, product
  inventory, artifact/symbol inspection, and diff checks pass.
- The 40,000-document, five-trial current artifact passes all 22 exactness and
  performance gates at
  `/tmp/ii42-arch4-build-admission-perf-new.json`. Relative to the preceding
  accepted artifact, median mean ratios for static-before, static-after,
  fragmented, workload-folded, impact, and static-reference are `0.9885x`,
  `0.9993x`, `0.9902x`, `1.0103x`, `0.9880x`, and `1.0047x`; worst QPS is
  `0.9898x` and worst p99 is `1.0793x`. Rows, order, and scores remain exact,
  and the mixed bounded movement passes the frozen performance gate.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `f184f3be07646fef22a77449ea824acea7489fff1500c6dbbea1da4de6438565`.
  SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4b: Rebuild Metapage Publication Contract

Continue the build extraction with the final full-rebuild metapage switch,
which is the smallest remaining mutable authority with a closed resource
boundary. Move only the MAIN/INIT-fork rebuild-root publication function from
`ii42_am.c` into `ii42_am_meta.c/.h` and rename it to describe that narrow
role. The caller continues to build and validate the replacement, write the
sealed bundle, own the generation barrier, and decide cache epoch, flags, and
rebuild count.

The meta publication function receives only immutable typed values:

- the already-open index relation and MAIN or INIT fork;
- the fully written and serialized `ii42_segment_read_root`;
- source type, checked `uint32` document count, cache epoch, flags, and a
  nonzero rebuild count.

Boundary rules:

- the caller must validate the replacement and ensure every immutable bundle
  page is written before calling. The meta module must not know the build
  replacement type, index payload, semantic postings, builder choice, runtime
  model, or workload policy;
- the caller retains the generation-barrier lock. The moved function acquires
  only the metapage buffer's exclusive content lock, performs the existing
  critical-section overwrite, marks the buffer dirty, emits the same full-page
  WAL record when required, releases the buffer, and flushes the publication
  LSN before returning;
- the exact MAIN/INIT fork validation, root serialization, physical
  high-water check, fixed-field zeroing, flags, cache epoch, source type,
  document count, rebuild count, and error text remain unchanged;
- no generic root writer, callback table, publication policy, lock helper, or
  compatibility path is introduced. Incremental linked-L0/COW publication,
  flag-only updates, relation truncation, page allocation, and INIT-fork page
  creation remain in their current owners;
- the function is a hidden internal module symbol, not an installed C or SQL
  API. `ii42_am_meta` retains no relation pointer, buffer, root, or backend
  state after return.

Mechanical acceptance gate:

1. The rebuild metapage switch has exactly one owner in `ii42_am_meta.*`;
   `ii42_am.c` contains one typed call and no implementation, private
   prototype, WAL call, or copied fixed-field initialization for this route.
2. Inventory rejects build-output/runtime/model dependencies and relation
   truncation, page allocation, generation-barrier, scheduler, or incremental
   publication authority in the moved closure. Object inspection proves one
   implementation and no product export.
3. CREATE INDEX, REINDEX, UNLOGGED INIT-fork recovery, explicit corruption and
   refresh, compact/spill/semantic builders, restart, and physical replay
   preserve exact roots, cache epochs, rebuild counts, errors, and durability.
4. Golden parity, native lifecycle, real-model SAE lifecycle, schema/ACL,
   isolated regression, warning-clean PG18, CMake/CTest, artifact identity,
   inventory, modified-script `py_compile`, `git diff --check`, and the frozen
   BM25 performance matrix pass from fresh directories.
5. Any root byte, lock order, WAL record/flush, score/order, status, RSS,
   lifecycle, or BM25 baseline change stops the slice for diagnosis.

This is a planning-only contract. The subsequent mechanical move is a
separate commit and may not absorb sealed-bundle construction, relation
truncation, page allocation, `ambuildempty`, generation barriers, or any
incremental publication path.

ARCH-4 slice 4b closure evidence:

- `ii42_am_meta.c/.h` now solely own the hidden
  `ii42_am_publish_rebuild_meta` implementation and typed declaration.
  `ii42_am.c` retains replacement validation, immutable-bundle writing,
  generation-barrier ownership, relation truncation/allocation, and the sole
  typed call. Object inspection finds one undefined caller in `ii42_am.o`, one
  implementation in `ii42_am_meta.o`, and no exported dylib symbol.
- Product inventory requires the exact MAIN/INIT validation, root
  serialization, physical high-water check, metapage content lock, critical
  section, full-page WAL record, and publication-LSN flush. It rejects
  replacement/build, segment-page, runtime/model, scheduler, truncation,
  allocation, generation-barrier, Generic WAL, or incremental-publication
  authority in the moved closure. The old writer name, prototype, and body are
  absent.
- Fresh empty/UNLOGGED lifecycle passes `7/7`, including crash recovery to a
  valid empty INIT generation. Independent golden parity passes at
  `/tmp/ii42-arch4-meta-publish-golden.json`, retaining manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native v3
  lifecycle passes `71/71` at
  `/tmp/ii42-arch4-meta-publish-native.json`; real-model lexical-first SAE
  lifecycle passes `15/15` at
  `/tmp/ii42-arch4-meta-publish-sae.json`.
- Physical replication, bounded payload-corruption/explicit-refresh, schema,
  runtime-role privilege, compact builder, spill builder, and isolated SQL
  regression gates pass. Fresh warning-clean PG18, CMake/CTest `1/1`, modified
  script `py_compile`, product inventory, artifact/symbol inspection, and diff
  checks pass.
- Both old and new 40,000-document, five-trial artifacts pass all 22 internal
  exactness/performance gates at
  `/tmp/ii42-arch4-meta-publish-perf-old.json` and
  `/tmp/ii42-arch4-meta-publish-perf-new-r2.json`. IDs and scores are exact;
  static-before, static-after, fragmented, workload-folded, and
  static-reference paired ratios pass every frozen threshold.
- The first 96-sample cross-binary impact p99 comparison was red and stopped
  the slice. A second interleaved run reproduced small-sample p99 variance, so
  the route was diagnosed without changing the product or gate. Native
  impact-specialized diagnostics then used the same physical setup and index,
  nine interleaved trials, and 4,096 samples per trial in
  `/tmp/ii42-arch4-meta-publish-impact-diag-old-v2.json` and
  `/tmp/ii42-arch4-meta-publish-impact-diag-new-v2.json`. New/old
  mean/p50/p95/p99/QPS ratios are `0.9944x`, `0.9985x`, `0.9900x`, `1.0598x`,
  and `1.0056x`; all frozen thresholds pass. The earlier red p99 was therefore
  not a reproducible hot-path regression.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `b367863232c187da886a927acb85addd1a4e4038b0f2d4174b3720f1cd2d3fbb`.
  Installed SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4c: Rebuild Output Ownership Contract

Do not move the complete sealed-bundle publisher next. Its current closure
still depends on AM-owned runtime-contract derivation, semantic fingerprint
validation, segment-page publication, and rebuild-output ownership. Moving it
now would require a reverse callback, a broad private-state header, or a second
contract authority.

Instead, establish the smallest prerequisite boundary by moving only the
rebuild-output value and its resource release into `ii42_am_build.c/.h`.
Rename `ii42_am_replacement` to `ii42_am_rebuild_output` and its cleanup to
`ii42_am_rebuild_output_release` so the type describes its current role rather
than an alternate index lifecycle.

Boundary rules:

- preserve every field and its exact type: source OID, TID map, document count,
  serialized-byte placeholder, semantic signature, lexical index, validity and
  SAE flags, semantic postings/fingerprints, and PostgreSQL build statistics.
  This slice neither deletes currently unused fields nor changes build output;
- preserve allocator provenance exactly. TIDs, semantic postings, and semantic
  fingerprints remain `palloc`-owned; serialized bytes remain `malloc`-owned;
  the embedded index remains `ii42_index_free`-owned when valid. Release stays
  null-safe, conditionally frees each owner, and zeroes the complete value;
- `ii42_am_build_replacement` continues to populate the value in `ii42_am.c`.
  Full and INIT-fork publishers continue to validate and consume it. The empty
  build continues to borrow its index from the existing `PG_FINALLY` cleanup
  owner and must not acquire a second release;
- the build header may include only the concrete value-type dependencies needed
  for its fields. It must not expose AM build state, relation callbacks,
  semantic runtime sessions, metapages, manifests, locks, WAL, scheduler state,
  GUCs, or generic cleanup callbacks;
- the build implementation gains only deterministic resource release. It
  opens no relation, allocates no output, retains no state, and performs no
  model, policy, publication, or error decision. The existing memory-admission
  authority remains unchanged.

Mechanical acceptance gate:

1. `ii42_am_rebuild_output` and `ii42_am_rebuild_output_release` have one owner
   in `ii42_am_build.*`; the old type/function names and duplicate cleanup body
   are absent from `ii42_am.c` and all product source.
2. Product inventory proves the exact fields, allocator-specific cleanup,
   null-safety, full zeroing, one normal release call, and the intentional
   INIT-fork borrowed-owner path. Object inspection finds one implementation
   and no exported product symbol.
3. No relation, metapage, manifest, segment-page, runtime/model, lock, WAL,
   scheduler, GUC, or publication dependency enters `ii42_am_build.c` through
   this move. Header inclusion remains acyclic and private to current AM/build
   consumers.
4. Standard, compact, spill, semantic-stream, empty INIT-fork, CREATE INDEX,
   REINDEX, failure cleanup, restart, and physical replay preserve exact output,
   ownership, errors, roots, and durability.
5. Golden parity, native and real-model SAE lifecycle, schema/ACL, isolated
   regression, warning-clean PG18, CMake/CTest, artifact identity, inventory,
   modified-script `py_compile`, `git diff --check`, and the frozen BM25
   performance matrix pass from fresh directories.

This is a planning-only contract. The type move and rename occur in a separate
mechanical commit. No builder, publisher, runtime-contract, semantic validation,
field deletion, or allocator conversion may ride with it.

ARCH-4 slice 4c closure evidence:

- `ii42_am_build.c/.h` now solely own the hidden
  `ii42_am_rebuild_output` value and
  `ii42_am_rebuild_output_release()` implementation. The value preserves every
  former field and exact type. The release preserves `pfree`, `free`, and
  `ii42_index_free` provenance, remains null-safe, and zeroes the complete
  value. The old replacement names and duplicate AM cleanup body are absent.
- `ii42_am.c` retains output construction, validation, sealed-bundle
  publication, generation-barrier ownership, and one normal release call. The
  INIT-fork empty build retains its existing `PG_FINALLY` index owner and has no
  second release. Object inspection finds one undefined caller in `ii42_am.o`,
  one private implementation in `ii42_am_build.o`, and no exported dylib
  symbol. Product inventory rejects relation, metapage, segment-page,
  runtime/model, lock, WAL, scheduler, GUC, publication, and allocation
  authority in the moved release closure.
- Fresh empty/UNLOGGED lifecycle passes `7/7`; independent golden parity
  retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native mutable
  lifecycle passes `71/71` at
  `/tmp/ii42-arch4-rebuild-output-native.json`; convergent real-model SAE
  lifecycle passes `15/15` at
  `/tmp/ii42-arch4-rebuild-output-sae.json`.
- Physical replication, including 2PC, linked-L0, maintained CRUD, REINDEX,
  layout transitions, and drop replay, passes at
  `/tmp/ii42-arch4-rebuild-output-replication.json`. Compact and spill
  builders, failed-rebuild rollback, schema, runtime-role privilege, isolated
  SQL regression, warning-clean PG18, and fresh CMake/CTest `1/1` also pass.
- The preceding and current 40,000-document, five-trial artifacts pass all 22
  internal exactness/performance gates at
  `/tmp/ii42-arch4-rebuild-output-perf-old.json` and
  `/tmp/ii42-arch4-rebuild-output-perf-new.json`. IDs, order, and scores are
  exact. Across fragmented, static, static-reference, workload-folded, and
  impact-specialized states, the worst new/old latency ratio is `1.0149x` and
  the worst QPS ratio is `0.9893x`; every frozen paired threshold passes.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `e1a95c9200c3468f6d38607528f66fa76b437384d42e240971a491d73e3ded8a`.
  Installed SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4d: Document Fingerprint Predicate Contract

The sealed-bundle publisher cannot move while it calls an AM-local fingerprint
predicate. The same fixed-width all-zero test is currently implemented three
times in the AM, segment codec, and document-COW codec. Consolidate only this
pure value predicate before changing publication authority.

Promote the segment codec's predicate to the sole internal `static inline`
definition `ii42_document_fingerprint_is_zero()` in `ii42_segments.h`.
Replace the AM, linked-L0, and document-COW copies with typed calls and delete
all three old private names. A header definition is required because
document-COW validation executes this fixed-width predicate once per decoded
lexical record; an out-of-line cross-object call is not an acceptable hot-path
boundary.

Boundary rules:

- the function accepts only a pointer to the fixed
  `II42_DOCUMENT_FINGERPRINT_BYTES` value, returns true for `NULL` or an
  all-zero value, and returns false after the first nonzero byte. It allocates
  nothing, retains no state, and raises no error;
- existing callers continue to decide whether zero is valid and preserve their
  current error text, status code, quarantine, retirement, or validation
  behavior. No caller may delegate policy to the predicate;
- document-COW callers already pass embedded fixed arrays. Adopting the
  null-safe common predicate must not add a nullable runtime path or alter any
  serialized byte, checksum, equality rule, or tree validation result;
- the segment header gains only this value-level inline definition. It
  acquires no PostgreSQL relation, AM/build, runtime/model, metapage, lock,
  WAL, scheduler, GUC, memory-context, or publication dependency;
- this slice does not move the sealed-bundle publisher, derive a runtime
  contract hash, change rebuild output, or modify lexical/semantic scoring.

Mechanical acceptance gate:

1. One source definition exists in `ii42_segments.h`, with no out-of-line
   implementation or public symbol; the old AM, linked-L0, and document-COW
   helper names and bodies are absent.
2. Inventory proves the exact fixed-width loop, null behavior, no allocation or
   policy/error authority, and typed use by all former callers. Object
   inspection finds no unresolved helper call and no exported product symbol.
3. Segment/document-COW codec tests, independent golden parity, empty/UNLOGGED,
   native mutable lifecycle, real-model SAE completion, failed rebuild,
   restart, VACUUM, and physical replay preserve exact bytes, rows, order,
   scores, errors, and durability.
4. Warning-clean PG18, fresh CMake/CTest, schema/ACL, isolated regression,
   artifact identity, inventory, modified-script `py_compile`,
   `git diff --check`, and the frozen 40K BM25/impact matrix pass.
5. Any format, root, score/order, error, lock/WAL, RSS, lifecycle, or BM25
   baseline change stops the slice for diagnosis.

This is a planning-only prerequisite contract. Its mechanical commit may only
consolidate the predicate. The later sealed-bundle publisher extraction must
separately receive an immutable contract hash and retain the existing page and
metapage publication order.

The first uncommitted mechanical attempt used one out-of-line implementation
in `ii42_segments.c`. Correctness, lifecycle, recovery, replication, package,
and all 22 internal 40K gates passed, but the frozen cross-slice gate stopped
on static-reference and impact-specialized latency. The old artifact
`e1a95c9200c3468f6d38607528f66fa76b437384d42e240971a491d73e3ded8a`
and candidate artifact
`941c95d29347df1deb0356dddd23db339cf8313ae5b34e32f75ffba12b792ddb`
were then rebuilt and measured in the same high-load window. A same-index
9-by-4,096 impact diagnostic showed candidate/baseline mean, p50, p95, p99,
and QPS ratios of approximately `0.992x`, `0.982x`, `0.987x`, `0.986x`, and
`1.008x`; the prior impact red was load and sample noise. A 7-by-32
static-reference control still placed the candidate centre approximately
`1.052x` above the baseline while same-binary control labels drifted by about
`1.043x`. Source inspection identified the per-record document-COW validation
call. The contract therefore preserves one logical definition but requires it
to remain inline; it does not weaken any performance threshold.

ARCH-4 slice 4d closure evidence:

- `ii42_segments.h` now solely defines the null-safe fixed-width
  `ii42_document_fingerprint_is_zero()` predicate as `static inline`. The AM,
  linked-L0, and document-COW callers use that definition; all three retired
  private helpers are absent. Object and dylib inspection find no unresolved
  predicate call and no exported symbol.
- Product inventory requires one inline definition, its exact null/all-zero/
  first-nonzero behavior, no policy or PostgreSQL authority, the expected
  typed call counts, and complete removal of the old names. Warning-clean PG18
  and fresh CMake/CTest `1/1` pass.
- Fresh empty/UNLOGGED lifecycle passes `7/7` at
  `/tmp/ii42-arch4-fingerprint-inline-unlogged.json`. Independent golden
  parity passes at `/tmp/ii42-arch4-fingerprint-inline-golden.json`, retaining
  manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`. Native
  mutable lifecycle passes `71/71` at
  `/tmp/ii42-arch4-fingerprint-inline-native.json`; convergent real-model SAE
  lifecycle passes `15/15` at
  `/tmp/ii42-arch4-fingerprint-inline-sae.json`.
- Physical replication passes at
  `/tmp/ii42-arch4-fingerprint-inline-replication.json`. Compact and spill
  builders, schema, runtime-role privilege, and isolated SQL regression gates
  also pass.
- The standard 40,000-document, five-trial inline artifact passes all 22
  internal exactness/performance gates at
  `/tmp/ii42-arch4-fingerprint-inline-perf.json`. IDs and scores are exact.
  After an initial cross-window stop, the old and inline artifacts were tested
  in an `inline-old-inline-old` process bracket with each static-reference
  index measured twice per process. Aggregate inline/old mean, p50, p95, p99,
  and QPS ratios are `0.9572x`, `0.9513x`, `0.9662x`, `0.9711x`, and
  `1.0459x`. Same-index 9-by-4,096 impact measurements normalized by their
  interleaved controls report `0.9638x`, `0.9659x`, `0.9846x`, `1.0063x`,
  and `1.0376x`. Every frozen threshold passes.
- Binary inspection confirms that the page-native AM scan, complete page-query
  range, document-COW validation range, and segment query-contract range have
  identical addresses and machine code in the old and inline artifacts. This
  independently validates that the earlier cross-window red result was host/
  process variance rather than an accepted query-path change.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `fb7f09aa810bd44e4e1f430ced24dc68bec7cf5cd1fd88294ef89d971c7bd49b`.
  Installed SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4e: Rebuild Sealed-Bundle Publisher Contract

The rebuild publisher can now cross the build-module boundary without a
reverse callback or broad private header. Rebuild output ownership lives in
`ii42_am_build`, metapage publication has a narrow typed API, and fingerprint
validation has one internal definition. The remaining AM-local dependency is
relation-derived runtime-contract hashing.

Move the complete `ii42_am_publish_replacement_segments()` closure to
`ii42_am_build.c/.h` and pass it an immutable
`II42_SEGMENT_CONTRACT_HASH_BYTES` value. The two AM callers derive that value
from the relation immediately before publication. The moved function retains
replacement validation, deterministic manifest/segment/L0 ids, manifest and
document-version construction, exact lexical/semantic payload assembly,
sealed-bundle writing, rebuild-metapage publication, and all cleanup.

Boundary rules:

- `ii42_am.c` retains runtime-contract derivation, MAIN/INIT fork selection,
  generation-barrier ownership, rebuild-count and cache-epoch selection,
  relation truncation, initial metapage creation, auto-preload scheduling, and
  all CREATE INDEX/REINDEX/`ambuildempty` orchestration;
- `ii42_am_build` receives an already-derived immutable hash and must not read
  relation options, model/runtime state, GUCs, or shared-runtime state. It may
  use the relation only for existing sealed page and metapage publication;
- MAIN publication still occurs after the caller holds the generation barrier,
  truncates the relation, and writes the initial metapage. INIT publication
  still occurs only after `ii42_am_write_init_page_at()` initializes the fork;
- the exact validation and error order inside the publisher, id derivation,
  query-contract build, sealed-bundle write, metapage switch, and `PG_TRY`/
  `PG_FINALLY` cleanup order remain unchanged. Moving hash derivation to the
  caller must not introduce another relation read after publication begins;
- the public build header exposes only the typed publisher declaration and
  existing rebuild-output type. It gains no lock, scheduler, model, scan,
  mutation, VACUUM, preload, or global mutable-state authority;
- do not move replacement construction, low-memory builder selection,
  relation allocation/truncation, incremental publication, or any query or
  scoring path in this slice.

Mechanical acceptance gate:

1. `ii42_am_build.c` has the sole publisher implementation; its header has one
   declaration and `ii42_am.c` has exactly the MAIN and INIT calls. The old
   static prototype/body are absent.
2. Inventory proves the immutable hash parameter/copy, exact validation/id/
   payload/write/meta/cleanup closure, caller-owned hash derivation and fork
   preparation, and forbidden authority absence. Object inspection finds one
   AM caller object, one build-module implementation, and no exported symbol.
3. Empty/UNLOGGED, golden parity, native lifecycle, real-model SAE lifecycle,
   failed rebuild, restart, VACUUM, compact/spill builders, and physical replay
   preserve exact bytes, rows, order, scores, errors, and durability.
4. Warning-clean PG18, fresh CMake/CTest, schema/ACL, isolated regression,
   package identity, inventory, modified-script `py_compile`,
   `git diff --check`, and the frozen 40K matrix pass.
5. Any root byte, error order, lock/WAL, memory owner, status, lifecycle,
   score/order, or BM25 performance change stops the slice for diagnosis.

This is a planning-only contract. The mechanical move is a separate commit
and may not absorb the surrounding rebuild orchestrator or another ARCH-4
authority boundary.

ARCH-4 slice 4e closure evidence:

- `ii42_am_build.c` now solely implements the complete rebuild sealed-bundle
  publisher and `ii42_am_build.h` exposes its typed declaration. The MAIN and
  INIT AM callers retain fork preparation, generation/truncation authority,
  derive one immutable runtime-contract hash immediately before publication,
  and pass it into the build authority. The moved closure retains validation,
  deterministic ids, lexical/semantic payload construction, sealed-bundle
  writing, metapage publication, and `PG_TRY`/`PG_FINALLY` cleanup unchanged.
- Product inventory proves the sole implementation/declaration, exactly two AM
  callers, immutable hash copy, caller setup/hash/publication order, complete
  publisher closure, and forbidden authority absence. Object inspection finds
  one unresolved AM call and one build-module definition; the final dylib does
  not export either the bundle publisher or metapage publisher. Warning-clean
  PG18 and fresh CMake/CTest `1/1` pass.
- Fresh empty/UNLOGGED lifecycle passes `7/7` at
  `/tmp/ii42-arch4-publisher-unlogged.json`. Independent golden parity passes
  at `/tmp/ii42-arch4-publisher-golden.json`, retaining manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- The first native mutable lifecycle run stopped at `70/71` in
  `/tmp/ii42-arch4-publisher-native.json`. Transaction rollback, one-record WAL
  atomicity, and eventual convergence were exact; the sole failure was a
  fault-injection accounting heuristic that observed one inactive aborted
  physical record while all `17` pending semantic records remained correctly
  represented. An independent confirmation passes `71/71` at
  `/tmp/ii42-arch4-publisher-native-confirm.json`. No test or product behavior
  was weakened to hide the first result.
- Real-model convergent SAE lifecycle passes `15/15` at
  `/tmp/ii42-arch4-publisher-sae.json`; physical replication passes at
  `/tmp/ii42-arch4-publisher-replication.json`. Compact and spill builders,
  schema, runtime-role privilege, and isolated SQL regression also pass.
- The standard 40,000-document, five-trial artifact passes all 22 internal
  exactness/performance gates at
  `/tmp/ii42-arch4-publisher-perf.json`. IDs, order, and scores are exact.
  Folded/static mean, p50, p95, p99, and QPS ratios are `1.0000x`, `0.9964x`,
  `1.0040x`, `1.0036x`, and `1.0000x`; folded-over-fragmented normalized
  ratios are `1.0028x`, `1.0054x`, `1.0020x`, `1.0023x`, and `0.9972x`.
- Source, staged, and installed dylibs are byte-identical at SHA-256
  `0b25f8f152b250bf3cf0b95393c26e5a022fed63224694abcf2815d10993307e`.
  Installed SQL remains
  `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`;
  control remains
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`.

#### ARCH-4 Slice 4f: Root Object-Byte Accounting Contract

The next extraction moves only checked v3 root/manifest object-byte accounting
from `ii42_am.c` into the existing metapage/root authority. This is a shared
dependency of status projection, explicit-build admission, diagnostics, and
maintenance policy; leaving it AM-private would force later modules to depend
back on the callback entry module.

Move these exact functions to `ii42_am_meta.c/.h` without renaming or changing
their arithmetic:

- `ii42_am_segment_object_bytes_add()` performs saturating `uint64` addition;
- `ii42_am_segment_manifest_object_bytes()` sums the manifest object,
  query-contract, term-directory, neutral-fold, impact-fold, sealed segment
  payload, active-L0, and pending-L0 bytes, and reports sealed payload bytes
  separately;
- `ii42_am_convergent_object_bytes()` deserializes the checked root, loads one
  sealed manifest, applies the same accounting, and releases the manifest in
  the existing `PG_TRY`/`PG_FINALLY` order.

Boundary rules:

- the meta module may include the existing segment-page reader needed to load
  the sealed manifest. It gains no mutation, scheduler, query, model/runtime,
  reloption, preload, VACUUM, or SQL-control authority;
- no physical range is scanned and no reachability, retired-range, recyclable
  marker, relation-capacity, or heap byte is added. This function reports the
  same logical live-object measure, not allocated or reclaimable storage;
- overflow remains saturating rather than throwing, and the existing invalid
  root/accounting errors and cleanup order remain byte-for-byte unchanged;
- all current AM callers continue through the typed private meta header. This
  slice does not move generation JSON, mutation debt, health policy, builder
  selection, or preload decisions;
- the old static definitions are deleted immediately. No wrapper, callback,
  alias, or duplicate accounting route may remain.

Mechanical acceptance gate:

1. `ii42_am_meta.c` has the sole three implementations; its header has one
   declaration for each and `ii42_am.c` has calls only. Inventory proves the
   exact accounted fields, saturation behavior, sealed-manifest lifecycle, and
   forbidden authority absence.
2. Object inspection finds AM callers and meta-module definitions but no
   exported dylib symbol. Status/index byte fields and build-memory builder
   selection are unchanged on the same fixtures.
3. Golden parity, native lifecycle, real-model SAE lifecycle, restart,
   physical replication, compact/spill builder selection, storage plateau,
   schema/ACL, and isolated regression preserve exact results and state.
4. Warning-clean PG18, fresh CMake/CTest, package identity, inventory,
   modified-script `py_compile`, `git diff --check`, and the frozen 40K matrix
   pass. Any score, root, builder, lock/WAL, lifecycle, RSS, or BM25 performance
   change stops the slice for diagnosis.

This is a planning-only contract. Its mechanical move is a separate commit and
may not absorb generation status, workload admission, or another root helper.

#### ARCH-4 Stop Gate: CSG-I121 VACUUM/COW Ownership

The first slice-4f 8-writer/8-reader real-model replay exposed one low-frequency
failure while online maintenance and `VACUUM` both changed document COW state:
query replay rejected a retirement target, one maintenance build rejected a
document-directory append patch, and final convergence stopped. The same
candidate subsequently passed three full replays, including 24 writer cycles
and eight staging-reuse cycles; the pre-4f baseline also passed 12- and
48-cycle replays. The accounting move is arithmetic-only, so this is treated as
a timing-sensitive authority defect rather than accepted as test noise or
attributed to the extraction without evidence.

Static ownership review identifies one missing edge. Online seal, compaction,
fold, reclamation, and semantic completion own the transaction-scoped
per-index maintenance lock before taking a root snapshot. Convergent `VACUUM`
instead snapshots document COW and linked L0 under its transaction writer pin,
then upgrades the writer barrier only around COW publication. It can therefore
race an online maintenance root transition between discovery and publication,
even though both paths individually revalidate their immediate build root.

The symptom matches the earlier CSG-I120 failure, but the earlier narrow repair
remains valid: it supplies the legal empty `HISTORY_BARRIER` when VACUUM has
already absorbed an entire frozen frontier. CSG-I121 addresses the deeper
ownership gap that allowed the same seal/VACUUM interleaving to recur after
that representation fix and its original repeated 8x8 qualification.

Close CSG-I121 before committing slice 4f:

1. Make convergent `VACUUM` join the same transaction-scoped per-index
   maintenance authority before it snapshots document COW or linked L0. Keep
   the existing short writer-barrier upgrade for tuple-version publication;
   do not serialize ordinary foreground writers with the whole maintenance
   pass.
2. Add a deterministic v3 gate that freezes a pending-L0 seal at the existing
   test pause, starts `VACUUM`, and proves `VACUUM` cannot enter COW discovery
   or publication until the maintenance owner releases. After both complete,
   require normal/oracle equality and a clean converged root.
3. Repeat 8-writer/8-reader real-model mutation, query, maintenance, `VACUUM`,
   and staging-reuse stress. Require every actor to finish, exact expected
   operation counts, no invalid COW/L0 transition, and zero remaining debt.
4. Re-run native, SAE, transaction/2PC, restart, replication, storage plateau,
   and BM25 performance gates. Any deadlock, foreground-write serialization,
   root/score change, or material latency regression rejects the repair.

The CSG-I121 contract and implementation are separate commits. Slice 4f remains
uncommitted until the repaired parent passes this gate, after which the exact
accounting extraction is reapplied and qualified independently.

CSG-I121 closure evidence:

- Commit `97cba0aa` makes convergent `VACUUM` acquire the transaction-scoped
  per-index maintenance authority before writer pin, root read, document-COW
  traversal, and linked-L0 discovery. The existing short writer-barrier
  upgrade still owns tuple-version publication; ordinary foreground append
  remains outside the maintenance critical section.
- The deterministic pre-fix probe at
  `/private/tmp/ii42-csg-i121-pre-fix.json` observed `VACUUM` waiting on the
  writer-barrier tag `844317250`. The fixed probe at
  `/private/tmp/ii42-csg-i121-post-fix-1x1.json` observes the maintenance tag
  `844317261`, then completes paused seal and `VACUUM` with normal/oracle
  equality and zero debt. Two independent 8-writer/8-reader, 24-cycle runs at
  `/private/tmp/ii42-csg-i121-post-fix-8x8x24.json` and
  `/private/tmp/ii42-csg-i121-post-fix-8x8x24-r2.json` complete all actors,
  query parity, eight staging-reuse cycles, and clean convergence.
- Tests that deliberately own the public session maintenance gate now issue
  `VACUUM` through that owner rather than constructing a cross-backend test
  deadlock. The full real-model mutable lifecycle passes `71/71` at
  `/private/tmp/ii42-csg-i121-native-r2.json`; SAE lifecycle passes `15/15` at
  `/private/tmp/ii42-csg-i121-sae-r2.json`; transaction/2PC/restart passes
  `21/21` at `/private/tmp/ii42-csg-i121-transaction-r2.json`; and the complete
  runtime-service product smoke passes at
  `/private/tmp/ii42-csg-i121-runtime-service.log`.
- The 140,000-row bounded retirement frontier passes `10/10` at
  `/private/tmp/ii42-csg-i121-vacuum-frontier-140k.json`. Native page/scorer
  lifecycle passes `80/80` at
  `/private/tmp/ii42-csg-i121-native-segment.json`, and physical replication
  passes prepared transactions, linked L0, maintained CRUD, layout conversion,
  and REINDEX at `/private/tmp/ii42-csg-i121-replication.json`.
- Eight staging-reuse cycles retain an exact `3192`-page plateau across all 16
  snapshots. The 40,000-document, five-trial paired matrix passes all `22`
  exactness/performance gates at
  `/private/tmp/ii42-csg-i121-perf-40k.json`; static, fragmented, folded, and
  impact-specialized top-100 IDs and scores are exact with maximum score drift
  `0.0`. Folded/static mean, p50, p95, p99, and QPS ratios are `0.9954x`,
  `0.9904x`, `1.0053x`, `1.0201x`, and `1.0046x`.

CSG-I121 is closed. Slice 4f may now be restored from its isolated worktree
state and qualified as an independent arithmetic-only extraction.

#### ARCH-4 Slice 4f Closure Evidence

- Commit `4a5909a6` moves only the three root/manifest object-byte accounting
  functions into `ii42_am_meta.c/.h`. `ii42_am.o` retains unresolved callers,
  `ii42_am_meta.o` has the sole definitions, and the final dylib exports none of
  the three private symbols. Product inventory proves the exact summed fields,
  saturating arithmetic, manifest cleanup, and forbidden-authority boundary.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4f-stage.7K9jd8`; evidence is under
  `/private/tmp/ii42-arch4-4f-evidence.MQE5bv`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `7757e45bc4ee1252d7369e1057390f68109c6ecab456d3850e78ecdd16895792`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Independent golden parity passes; empty/UNLOGGED lifecycle passes `7/7`;
  native page/scorer lifecycle passes `80/80`; mutable real-model lifecycle
  passes `71/71`; convergent SAE passes `15/15`; and transaction/2PC passes
  `21/21`. Runtime service, runtime privilege, compact/spill builders,
  non-public schema, isolated SQL regression, storage-version fail-closed
  recovery, and physical replication also pass against the same artifact.
- The 140,000-row VACUUM frontier passes `10/10`. A separate 8-writer/8-reader,
  24-cycle mutable run passes `72/72`. The same-index authority gate completes
  all 8 writers, 8 readers, 24 cycles, and 8 staging-reuse cycles; `VACUUM`
  waits on maintenance tag `844317261`, normal/oracle results match, and all 16
  steady snapshots remain at `3517` physical pages.
- The 40,000-base-document, 10,000-delta-document, five-trial matrix passes all
  `22` gates at
  `/private/tmp/ii42-arch4-4f-evidence.MQE5bv/perf-40k.json`. Static,
  fragmented, folded, and impact-specialized top-100 IDs and scores are exact,
  with maximum score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS
  ratios are `0.9979x`, `1.0002x`, `1.0000x`, `0.9996x`, and `1.0021x`.
- Warning-clean PG18, fresh CMake/CTest `1/1`, modified-script `py_compile`,
  product inventory, and `git diff --check` pass. No root format, accounting
  result, builder selection, lock/WAL order, score, lifecycle, or BM25
  performance change was observed. Slice 4f is closed.

#### ARCH-4 Slice 4g: Linked-L0 Mutation-Debt Projection Contract

The next extraction moves only the checked linked-L0 mutation-debt projection
from `ii42_am.c` into the existing metapage/root authority. Status, policy,
diagnostics, and rebuild admission all consume this same read-only projection;
leaving the type and decoder private to the AM callback file would force each
later module either to duplicate L0 accounting or to depend back on the entry
module.

Move the exact `ii42_am_convergent_mutation_debt` type and
`ii42_am_convergent_mutation_debt_read()` implementation to
`ii42_am_meta.c/.h` without changing their observable behavior:

- automatic consistency derives exact upsert and retirement counts from the
  current linked-L0 storage snapshot;
- manual consistency starts from the metapage's non-physical pending counters,
  then adds any linked-L0 records if present;
- semantic-complete and semantic-quarantine records remain excluded from
  lexical mutation counts while total records and payload bytes still report
  the full L0 snapshot;
- `uint32` upsert and retirement counters retain saturating arithmetic;
- invalid roots, invalid record kinds, snapshot load errors, and
  `PG_TRY`/`PG_FINALLY` cleanup retain the current fail-closed behavior.

Boundary rules:

- the meta module may consume the immutable consistency projection from
  `ii42_am_options.h` and the existing typed L0 storage reader. It gains no
  mutation publication, maintenance selection, scheduler, model/runtime,
  preload, VACUUM, query, SQL-control, or reloption-definition authority;
- the projection reads one checked root and one L0 snapshot. It may not rotate,
  seal, compact, reclaim, mutate the metapage, scan the heap, or acquire a
  maintenance/action lock;
- the existing callers in generation status, explicit-build admission,
  maintenance policy, maintenance execution, and index details continue
  through one typed private declaration. This slice moves none of those
  callers and changes no JSON, tuple, builder, or action-selection output;
- the old AM-local type and implementation are deleted immediately. No wrapper,
  callback, alias, or alternate debt counter may remain.

Mechanical acceptance gate:

1. `ii42_am_meta.c/.h` have the sole type and implementation; `ii42_am.c` has
   calls only. Inventory proves consistency handling, all four L0 record-kind
   cases, saturated counters, full record/byte identity, cleanup, and forbidden
   authority absence.
2. Object inspection finds AM callers and the meta-module definition but no
   exported dylib symbol. Generation status, index details, policy output,
   explicit builder choice, and automatic maintenance decisions are unchanged
   on the same fixtures.
3. Golden parity, native lifecycle, real-model SAE lifecycle, transaction/2PC,
   restart, physical replication, compact/spill builders, storage plateau,
   schema/ACL, and the frozen 40K performance matrix pass against one staged
   artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any debt, status, root, builder,
   action, lock/WAL, score, lifecycle, RSS, or BM25 performance change stops
   the slice for diagnosis.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb generation JSON, rebuild workload construction,
policy recommendation, maintenance dispatch, or another meta helper.

#### ARCH-4 Stop Gate: CSG-I122 Preload Publisher Synchronization

The slice-4g storage-plateau gate stopped before its first mutation cycle. Its
index enables `auto_preload`, so the preload worker can reserve the exact-root
`UNIFIED_WARM` slot before the smoke's explicit preload call. The explicit call
then correctly avoids a duplicate publisher and returns while the existing
slot is still `loading`; the old smoke immediately requires `resident=true`.
This first failure is a stale test synchronization assumption, not a root,
storage, or debt projection failure. Bounded waiting exposed a deeper obsolete
contract in the same unreferenced May harness: it requires the retired
`active_data_pages` and `delta_data_pages` diagnostics plus the pre-v3
`segment_reused` and `retired_tail_truncated` maintenance result. Current v3
publishes linked-L0 seals and reports typed `reused_blocks`; its fixed-live
plateau is already owned by `test_same_index_writer_concurrency_temp_pg.py`.

Close CSG-I122 before accepting slice 4g:

1. Keep the product contract unchanged: the process that owns a new slot walks
   the checked root synchronously and publishes it, while a competing caller
   neither duplicates the walk nor waits while holding relation or shared-cache
   authority.
2. Retire `test_segmented_generation_storage_bloat.py`; it has no suite,
   documentation, or package caller and no longer measures the current storage
   lifecycle. Do not fabricate aliases for retired fields or infer v3 reuse
   from old result strings.
3. Use `test_same_index_writer_concurrency_temp_pg.py` as the storage gate. Its
   current root/status contract verifies fixed-live insert/delete/VACUUM,
   bounded maintenance, exact native/oracle queries, zero final L0 debt, and a
   24-cycle physical-page and relation-byte plateau.
4. Do not add runtime waits, condition variables, sleeps, retries, alternate
   preload APIs, relaxed residency assertions, or a second plateau harness.

The planning contract and obsolete-smoke removal are separate commits. After
the removal, rerun the canonical storage-plateau gate against the same staged
binary; any current-contract failure returns to product diagnosis.

CSG-I122 closure evidence:

- Commit `e9a7aa94` removes only the orphaned May smoke. Repository search and
  product inventory find no caller, suite, package route, or second current
  storage contract; runtime code and installed artifacts are unchanged.
- `/private/tmp/ii42-arch4-4g-evidence.jmbQhy/same-index-8x8x24.json`
  completes 8 writers, 8 readers, all `288/288` model mutations, concurrent
  maintenance/VACUUM, exact normal/oracle rows, and the logical-prefix and
  VACUUM-authority gates. The fixed-live stage passes all 24 insert/delete
  cycles, drains L0 debt, and keeps every steady snapshot at `1778` physical
  pages, below the `1780`-page gate.
- The same staged artifact then passes the full 40K performance matrix. No
  runtime preload wait, duplicate publisher, legacy diagnostic alias, or second
  plateau harness was introduced. CSG-I122 is closed.

#### ARCH-4 Slice 4g Closure Evidence

- Commit `2da2cd18` moves only the linked-L0 mutation-debt value type and
  read-only projection into `ii42_am_meta.c/.h`. The five AM callers remain in
  place. `ii42_am.o` retains an unresolved caller, `ii42_am_meta.o` has the sole
  definition, and the final dylib exports no private debt symbol. Product
  inventory proves manual/automatic consistency behavior, all four valid L0
  record kinds, saturating counters, full snapshot record/byte identity,
  fail-closed cleanup, and the forbidden-authority boundary.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4g-stage.dNJLP5`; evidence is under
  `/private/tmp/ii42-arch4-4g-evidence.jmbQhy`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `918edf339c67cd9ff3c7fb538b1f8f68b860871b4b68ea62cc4cc800461a7ce3`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Independent golden parity passes; empty/UNLOGGED lifecycle passes `7/7`;
  native page/scorer lifecycle passes `80/80`; mutable real-model lifecycle
  passes `71/71`; convergent SAE passes `15/15`; and transaction/2PC passes
  `21/21`. Runtime service, runtime privilege, non-public schema, isolated SQL
  regression, compact/spill builders, storage-version fail-closed recovery,
  physical replication, and the 140,000-row VACUUM frontier `10/10` also pass
  against the same artifact.
- The canonical same-index gate completes 8 writers, 8 readers, all `288/288`
  model mutations, concurrent maintenance/VACUUM, exact normal/oracle results,
  logical-prefix and VACUUM-authority checks, and all 24 fixed-live cycles.
  Every steady snapshot remains at `1778` pages under the `1780`-page gate and
  final L0 debt is zero.
- The 40,000-base-document, 10,000-delta-document, five-trial matrix passes all
  `22` gates at
  `/private/tmp/ii42-arch4-4g-evidence.jmbQhy/perf-40k.json`. Static,
  fragmented, folded, and impact-specialized top-100 IDs and scores are exact,
  with maximum score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS
  ratios are `0.9957x`, `0.9999x`, `0.9957x`, `0.9953x`, and `1.0043x`;
  fragmented/static ratios are `0.9849x`, `0.9766x`, `1.0038x`, `1.0045x`,
  and `1.0154x`.
- Warning-clean PG18, fresh CMake/CTest `1/1` at
  `/private/tmp/ii42-arch4-4g-cmake.2aLpMX`, modified-script `py_compile`,
  product inventory, object ownership inspection, and `git diff --check` pass.
  No debt, status, root, builder, action, lock/WAL, score, lifecycle, RSS, or
  BM25 performance change was observed. Slice 4g is closed.

#### ARCH-4 Slice 4h: Metapage Rebuild-Ordinal Projection Contract

The next extraction moves only the two read-only rebuild ordinals from
`ii42_am.c` into the metapage authority. MAIN rebuild publication consumes the
next rebuild count, while explicit build orchestration consumes the next cache
epoch. Keeping either projection private to the AM entry module forces build
ownership to depend on an unrelated callback implementation detail.

Move `ii42_am_next_rebuild_count()` and `ii42_am_next_cache_epoch()` to
`ii42_am_meta.c/.h` without changing their arithmetic or error behavior:

- a zero-block relation returns `1` without attempting a metapage read;
- an existing relation is read through the sole checked metapage decoder;
- rebuild count advances by one and saturates at `UINT64_MAX`;
- cache epoch advances by one and wraps `UINT16_MAX` to `1`, preserving the
  existing nonzero epoch convention;
- relation lifetime and PostgreSQL lock ownership remain entirely caller-owned.

Boundary rules:

- the meta module gains no fork initialization, truncation, root publication,
  generation barrier, runtime-contract, preload, scheduler, builder, mutation,
  VACUUM, query, SQL-control, or reloption authority;
- the helpers may inspect relation block count and call `ii42_am_read_meta()`.
  They may not open a relation, acquire a lock, write a page, mutate the
  metapage, or silently recover from a malformed existing root;
- MAIN/INIT publisher structure, explicit builder selection, cache retirement,
  auto-preload scheduling, JSON/status output, and all callers remain in place;
- the old AM-local definitions are deleted immediately. No wrapper, alias, or
  alternate epoch arithmetic may remain.

Mechanical acceptance gate:

1. `ii42_am_meta.c/.h` have the sole two definitions and declarations;
   `ii42_am.c` has calls only. Inventory proves the empty/existing relation
   branches, exact saturation/wrap arithmetic, checked read, and forbidden
   authority absence.
2. Object inspection finds AM callers and the meta-module definitions but no
   exported dylib symbols. MAIN and INIT root bytes, rebuild count, cache epoch,
   generation status, index details, and preload identity remain unchanged.
3. Golden parity, empty/UNLOGGED, native lifecycle, mutable real-model, SAE,
   transaction/2PC, restart, physical replication, compact/spill builders,
   storage plateau, schema/ACL, and the frozen 40K performance matrix pass
   against one staged artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any root, ordinal, status, builder,
   lock/WAL, score, lifecycle, RSS, or BM25 performance change stops the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb relation initialization, rebuild publication, build
orchestration, preload scheduling, or another metapage helper.

#### ARCH-4 Slice 4h Closure Evidence

- Commit `79996f7d` moves only `ii42_am_next_rebuild_count()` and
  `ii42_am_next_cache_epoch()` into `ii42_am_meta.c/.h`. The AM retains one
  caller of each helper. Object inspection finds unresolved AM references and
  the sole meta-module definitions; the final dylib exports neither private
  symbol. Product inventory freezes the zero-block branch, checked metapage
  read, rebuild-count saturation, cache-epoch wrap, and forbidden-authority
  boundary.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4h-stage.K5o0TA`; evidence is under
  `/private/tmp/ii42-arch4-4h-evidence.0ii221`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `28846a7e2f2083849ea8a0a0922c10553d56ee1f6c79636ac71804e812d34b5a`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Golden parity, empty/UNLOGGED `7/7`, native page/scorer `80/80`, mutable
  real-model `71/71`, convergent SAE `15/15`, and transaction/2PC `21/21`
  pass. Runtime-required, runtime-privilege, non-public-schema, isolated SQL
  regression, compact/spill builders, all 12 storage-version rejection and
  `REINDEX` recovery boundaries, physical replication, and the 140,000-row
  VACUUM frontier `10/10` also pass against the same artifact.
- The canonical same-index gate passes 8 readers, 8 writers, all `288/288`
  expected mutations, concurrent maintenance/VACUUM, linked-L0 logical-prefix
  and VACUUM-authority checks. Its 24 fixed-live cycles remain at or below
  `1938` physical pages under the `1940`-page plateau limit and finish with
  exact rows and zero actionable debt.
- The 40,000-base-document, 10,000-delta-document, five-trial matrix passes all
  `22` gates at
  `/private/tmp/ii42-arch4-4h-evidence.0ii221/perf-40k.json`. Static,
  fragmented, folded, and impact-specialized top-100 IDs and scores are exact,
  with maximum score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS
  ratios are `1.0015x`, `1.0022x`, `1.0092x`, `0.9999x`, and `0.9985x`;
  fragmented/static ratios are `1.0064x`, `1.0056x`, `1.0134x`, `1.0219x`,
  and `0.9936x`.
- Warning-clean PG18, fresh CMake/CTest `1/1` at
  `/private/tmp/ii42-arch4-4h-cmake.fLgHca`, modified-script `py_compile`,
  product inventory, object ownership inspection, and `git diff --check` pass.
  No ordinal, root byte, status, builder, lock/WAL, score, lifecycle, RSS, or
  BM25 behavior change was observed. Slice 4h is closed; CSG-I114 remains open
  until the substantive build, VACUUM, scan, mutation, maintenance, semantic,
  and preload authorities are extracted.

#### ARCH-4 Slice 4i: Page Initialization And WAL Primitive Contract

The next extraction removes four raw page-write primitives from the AM entry
module before moving build or VACUUM authority. They already serve only root
bootstrap and metapage maintenance, so their correct owner is the checked
metapage/root module rather than a PostgreSQL callback implementation.

Move `ii42_am_mark_buffer_dirty_with_wal()`, `ii42_am_write_page_at()`,
`ii42_am_write_init_page_at()`, and `ii42_am_write_new_page()` to
`ii42_am_meta.c/.h` without changing their names, callers, or bytes:

- the dirty helper requires an already pinned, exclusively locked buffer and
  active critical section; it marks the buffer dirty and logs one full-page
  image only when the relation needs WAL;
- the MAIN-fork writer may replace an existing block or append exactly at the
  current block count. It rejects a skipped block, initializes the whole page,
  copies the exact caller payload, and preserves the current WAL condition;
- the INIT-fork writer appends exactly at its current block count and always
  WAL-logs the complete initialized page so UNLOGGED crash recovery can copy a
  valid empty generation back to MAIN;
- the append wrapper derives the current MAIN block count and delegates to the
  checked MAIN-fork writer. It owns no additional state or cleanup.

Boundary rules:

- caller-owned relation and generation locks remain unchanged. The module may
  acquire only the existing buffer content lock and critical section required
  to initialize one page;
- the moved closure gains no relation open/close, truncation, generation or
  writer barrier, builder selection, root publication, mutation, maintenance,
  VACUUM callback, query, preload, runtime, SQL-control, or reloption authority;
- MAIN/INIT fork choice, contiguous-write checks, `PageInit`, payload copy,
  dirty marking, WAL condition, forced INIT WAL, and error text remain exact;
- old AM-local definitions are deleted immediately. No wrapper, duplicate page
  writer, alternate WAL route, or second critical-section policy may remain.

Mechanical acceptance gate:

1. `ii42_am_meta.c/.h` have the sole four definitions and declarations;
   `ii42_am.c` retains calls only. Inventory freezes every fork, block-count,
   buffer-lock, critical-section, page-init, payload-copy, dirty, WAL, and error
   branch and rejects forbidden authority.
2. Object inspection finds AM callers and the meta-module definitions but no
   exported dylib symbols. MAIN/INIT root bytes, metapage status, rebuild
   ordinals, generation identity, and WAL behavior remain unchanged.
3. Golden parity, empty/UNLOGGED crash recovery, native and mutable lifecycle,
   transaction/2PC/restart, physical replication, storage rejection/recovery,
   concurrent VACUUM, compact/spill builders, storage plateau, schema/ACL, and
   the frozen 40K matrix pass against one staged artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any page byte, fork, WAL, lock,
   root, score, lifecycle, RSS, or BM25 performance change stops the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb generation barriers, relation truncation, rebuild
publication, builder orchestration, metapage policy, or another helper.

#### ARCH-4 Slice 4i Closure Evidence

- Commit `63d861e9` moves only
  `ii42_am_mark_buffer_dirty_with_wal()`, `ii42_am_write_page_at()`,
  `ii42_am_write_init_page_at()`, and `ii42_am_write_new_page()` into
  `ii42_am_meta.c/.h`. Object inspection finds unresolved AM calls and the
  sole meta-module definitions; the final dylib exports none of the four
  private symbols. Product inventory freezes MAIN/INIT fork selection,
  contiguous append checks, page initialization and payload copy, critical
  sections, dirty marking, conditional MAIN WAL, forced INIT WAL, exact error
  text, caller counts, and forbidden-authority boundaries.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4i-stage.PrCYVt`; evidence is under
  `/private/tmp/ii42-arch4-4i-evidence.IT2abs`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `a89550685128c3b33fc324b8b1a1f1992cd888f43da4a417e201ae460f96a0b2`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Golden parity passes with manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Empty/UNLOGGED crash recovery passes `7/7`, native page/scorer `80/80`,
  mutable real-model lifecycle `71/71`, convergent SAE `15/15`, and
  transaction/2PC/restart `21/21`.
- Runtime-required, runtime-privilege, non-public-schema, isolated SQL
  regression `1/1`, compact/spill builders, all 12 storage-version rejection
  and `REINDEX` recovery boundaries, and physical replication pass against the
  same artifact. The 140,000-row bounded VACUUM frontier passes `10/10`,
  including injected partial failure, restart, repeat VACUUM, and post-frontier
  insertion.
- The canonical same-index gate passes 8 readers, 8 writers, all `288/288`
  expected mutations, linked-L0 logical-prefix checks, and VACUUM authority.
  Its 24 fixed-live cycles remain at or below `1762` physical pages under the
  dynamically frozen `1764`-page plateau limit and finish exact with zero
  actionable debt.
- The 40,000-base-document, 10,000-delta-document, five-trial matrix passes all
  `22` gates at
  `/private/tmp/ii42-arch4-4i-evidence.IT2abs/perf-40k.json`. Static,
  fragmented, folded, and impact-specialized top-100 IDs and scores are exact,
  with maximum score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS
  ratios are `1.0080x`, `1.0079x`, `0.9796x`, `0.9902x`, and `0.9921x`;
  fragmented/static ratios are `1.0065x`, `1.0096x`, `1.0081x`, `1.0132x`,
  and `0.9936x`.
- Warning-clean PG18, fresh CMake/CTest `1/1` at
  `/private/tmp/ii42-arch4-4i-cmake.ZkHpwJ`, modified-script `py_compile`,
  product inventory, object ownership inspection, and `git diff --check` pass.
  No page byte, fork, WAL, lock, root, score, lifecycle, RSS, storage, package,
  or BM25 performance change was observed. Slice 4i is closed; CSG-I114 remains
  open until substantive build, VACUUM, scan, mutation, maintenance, semantic,
  and preload authorities are extracted.

#### ARCH-4 Slice 4j: Generation-Barrier Ownership Contract

Move the generation-identity advisory barrier from the AM callback module into
the metapage/root authority before extracting rebuild publication. The barrier
serializes full replacement, truncation, and explicit `REINDEX` around one
index OID; it is not the writer barrier used by incremental mutation and
VACUUM.

Move `II42_AM_GENERATION_BARRIER_LOCK_TAG` plus
`ii42_am_lock_generation_barrier_oid()`,
`ii42_am_unlock_generation_barrier_oid()`,
`ii42_am_lock_generation_barrier()`, and
`ii42_am_unlock_generation_barrier()` to `ii42_am_meta.c/.h` without changing
their lock identity or callers:

- the OID helpers remain file-local implementation details;
- the private header exposes only the Relation-typed acquire and release
  operations required by rebuild publication and explicit `REINDEX`;
- the advisory lock tag remains `(MyDatabaseId, 0x32534255, index_oid, 0)`;
- acquire remains blocking, transaction-owner independent, session-lock false,
  `ExclusiveLock`; release uses the same key/mode and preserves the exact
  `ii42 generation barrier is not held` failure.

Boundary rules:

- this slice does not move the writer-barrier tag or helpers, relation locks,
  truncation, page writes, metapage publication, build/rebuild orchestration,
  VACUUM, mutation, maintenance, semantic completion, query, preload, runtime,
  or SQL control;
- callers retain their existing `PG_TRY`/`PG_FINALLY` cleanup and relation
  lifetime. The barrier module retains no OID, lock token, relation, root,
  buffer, or backend-sized state;
- no wrapper, alternate advisory key, conditional acquire, lock conversion,
  timeout, retry, or second generation-barrier policy may remain in the AM;
- the generation barrier and writer barrier stay separate because they protect
  different publication scopes and use different lock tags.

Mechanical acceptance gate:

1. `ii42_am_meta.c` has the sole tag and four definitions; only the two
   Relation wrappers are declared in `ii42_am_meta.h`. `ii42_am.c` retains the
   two rebuild/reindex call pairs and no private prototype or lock-tag copy.
   Inventory freezes key fields, `ExclusiveLock`, blocking/session flags,
   release failure, caller counts, private OID helpers, and forbidden authority.
2. Object inspection finds unresolved Relation-wrapper calls in the AM, sole
   definitions in the meta object, and no exported dylib symbol. Writer-barrier
   object ownership, lock key, mode, callers, and behavior remain unchanged.
3. Golden parity, empty/UNLOGGED, native/mutable/SAE lifecycle,
   transaction/2PC/restart, physical replication, 140K VACUUM frontier,
   same-index 8x8x24 authority/plateau, storage rejection/recovery,
   compact/spill builders, schema/ACL, and the frozen 40K matrix pass against
   one staged artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any lock wait, deadlock, root,
   fork, WAL, score, lifecycle, RSS, storage, or BM25 performance change stops
   the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb rebuild publication, truncation, writer-barrier
ownership, or another helper.

#### ARCH-4 Slice 4j Closure Evidence

- Commit `21dd564a` moves the generation-barrier advisory tag and four
  acquire/release functions into `ii42_am_meta.c`. Only the Relation wrappers
  enter the private header; OID helpers remain file-local. The AM retains two
  acquire/release call pairs and no tag, prototype, wrapper, or alternate key.
  Object inspection finds unresolved wrapper calls in `ii42_am.o`, sole
  definitions in `ii42_am_meta.o`, and no dylib export. Inventory freezes the
  exact database/tag/OID key, `ExclusiveLock`, blocking and session flags,
  release failure, caller counts, private OID helpers, and forbidden authority.
  The writer-barrier tag, helpers, modes, and callers remain in the AM.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4j-stage.57IjKX`; evidence is under
  `/private/tmp/ii42-arch4-4j-evidence.iVKd5N`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `1f2829f3cb47a8a8d9fc99b814f21a2963aa12452e362066e95489c52c5fedd3`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Empty/UNLOGGED passes `7/7`, native page/scorer `80/80`, mutable real-model
  lifecycle `71/71`, convergent SAE `15/15`, and transaction/2PC/restart
  `21/21`. Physical replication, runtime-required, runtime-privilege,
  non-public-schema, isolated SQL regression `1/1`, compact/spill builders,
  and all 12 storage-version rejection/`REINDEX` recovery boundaries pass.
- The 140,000-row bounded VACUUM frontier passes `10/10`. The canonical
  same-index gate passes 8 readers, 8 writers, all `288/288` expected
  mutations, linked-L0 logical-prefix checks, and VACUUM maintenance authority.
  Its 24 fixed-live cycles remain at or below `1847` physical pages under the
  frozen `1849`-page limit and finish exact with zero actionable debt.
- The 40,000-base-document, 10,000-delta-document, five-trial matrix passes all
  `22` gates at
  `/private/tmp/ii42-arch4-4j-evidence.iVKd5N/perf-40k.json`. Static,
  fragmented, folded, and impact-specialized top-100 IDs and scores are exact,
  with maximum score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS
  ratios are `1.0014x`, `1.0008x`, `1.0029x`, `0.9903x`, and `0.9986x`;
  fragmented/static ratios are `1.0034x`, `1.0041x`, `0.9962x`, `1.0072x`,
  and `0.9967x`.
- Warning-clean PG18, fresh CMake/CTest `1/1` at
  `/private/tmp/ii42-arch4-4j-cmake.5tP9Sz`, modified-script `py_compile`,
  product inventory, object ownership inspection, and `git diff --check` pass.
  No lock wait, deadlock, page byte, fork, WAL, root, score, lifecycle, RSS,
  storage, package, or BM25 performance change was observed. Slice 4j is
  closed; the build publication transaction can now depend one-way on the
  root-owned barrier without calling back into the AM entry module.

#### ARCH-4 Slice 4k: Replacement-Relation Preparation Contract

Move the physical MAIN-fork replacement preparation from
`ii42_am_write_convergent_segment_relation()` into the build module while
preserving the existing transaction and contract-hash order. Add one private
build API:

```c
uint64 ii42_am_prepare_replacement_relation(Relation index_relation);
```

The function must execute exactly the current preparation closure:

1. derive the next rebuild count through the checked metapage projection;
2. truncate MAIN to zero blocks;
3. initialize one zeroed `ii42_am_meta_page` with the exact magic, version,
   and metapage kind;
4. append that complete metapage through the root-owned checked page writer;
5. return the previously derived nonzero rebuild count.

The caller continues to hold the generation barrier, then computes the runtime
contract hash and calls `ii42_am_publish_replacement_segments()` in the same
order as before. This preserves the important sequence
`next ordinal -> truncate/bootstrap -> current contract hash -> sealed bundle`
without a callback, precomputed hash, or reverse dependency on the AM.

Boundary rules:

- `ii42_am_prepare_replacement_relation()` requires caller-owned Relation
  lifetime and generation-barrier authority. It acquires no advisory or
  relation lock and retains no relation, root, buffer, or backend-sized state;
- build gains only full-replacement MAIN preparation. INIT-fork build,
  `ambuildempty`, heap scanning, encoder/runtime contract calculation, builder
  selection, sealed-bundle publication, preload retirement/scheduling,
  mutation, VACUUM, maintenance, query, and SQL control remain unchanged;
- the exact metapage bytes, `RelationTruncate(index_relation, 0)`, rebuild
  count, page writer, hash timing, error behavior, and caller
  `PG_TRY`/`PG_FINALLY` cleanup remain exact;
- no duplicate truncate/bootstrap helper, compatibility wrapper, callback,
  generic transaction object, or second replacement-preparation path may
  remain in the AM.

Mechanical acceptance gate:

1. `ii42_am_build.c/.h` have the sole implementation and declaration. The AM
   wrapper has exactly one typed call between generation-barrier acquisition
   and runtime-contract hashing, and retains neither direct truncate nor empty
   metapage initialization. Inventory freezes the exact order, bytes, ordinal,
   caller count, precondition boundary, and forbidden authority.
2. Object inspection finds one unresolved AM call, the sole build-object
   definition, and no exported dylib symbol. Rebuild count, cache epoch,
   contract hash, manifest/root bytes, INIT fork, and writer barrier remain
   unchanged.
3. Golden parity, empty/UNLOGGED, native/mutable/SAE lifecycle,
   transaction/2PC/restart, physical replication, 140K VACUUM frontier,
   same-index 8x8x24 authority/plateau, storage rejection/recovery,
   compact/spill builders, schema/ACL, and the frozen 40K matrix pass against
   one staged artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any ordering, lock, page byte,
   fork, WAL, root, ordinal, score, lifecycle, RSS, storage, or BM25 performance
   change stops the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb runtime-contract calculation, sealed-bundle
publication, generation-barrier scope, INIT build, or another helper.

#### ARCH-4 Slice 4k Closure Evidence

- Commit `2c2968f8` moves exactly the MAIN replacement preparation closure
  into `ii42_am_build.c`: checked next-rebuild projection, MAIN truncate,
  zeroed v3 metapage bootstrap through the root-owned page writer, and returned
  ordinal. The AM has one typed call and no duplicate empty metapage, truncate,
  ordinal, or page-write logic. It still owns the generation-barrier
  `PG_TRY`/`PG_FINALLY`, current runtime-contract hash, and sealed publication
  in exact `lock -> prepare -> hash -> publish -> unlock` order. INIT build,
  writer barrier, heap scan, builder selection, mutation, maintenance, VACUUM,
  query, preload, and SQL control are unchanged.
- Object inspection finds one unresolved call in `ii42_am.o`, the sole
  definition in `ii42_am_build.o`, and no exported dylib symbol. Inventory
  freezes the owner/declaration/caller counts, exact metapage fields, ordinal,
  truncate/page-writer sequence, AM orchestration order, and forbidden
  authority. PG18 and fresh CMake builds are warning-clean; CTest passes `1/1`;
  modified-script `py_compile`, inventory, and `git diff --check` pass.
- The fresh staged root is
  `/private/tmp/ii42-arch4-4k-stage.Yk4ZsR`; evidence is under
  `/private/tmp/ii42-arch4-4k-evidence.qInS6O`. Source, staged, and installed
  dylibs are byte-identical at SHA-256
  `25713a50aab974549609b816314e5df8fe411ee5064c98419d0a22877da9f292`.
  Control and version SQL remain byte-identical at
  `7ca220e11d5304982709e2300389394c0930a8b82c7248d4d105429d477989db`
  and `f3cc649bf34fda1eec5f5617d8b4940b77bb8ad8876a93aa6c8b69c04ca58d2f`.
- Golden parity retains manifest SHA-256
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  and runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
  Empty/UNLOGGED passes `7/7`, native page/scorer `80/80`, mutable real-model
  lifecycle `71/71`, convergent SAE `15/15`, transaction/2PC/restart `21/21`,
  and the 140,000-row VACUUM frontier `10/10`. Physical replication,
  runtime-required/privilege/restart, non-public schema, isolated SQL
  regression `1/1`, compact/spill builders, and all 12 storage-version
  rejection/explicit-`REINDEX` recovery boundaries pass.
- The canonical same-index gate passes 8 readers, 8 writers, all `288/288`
  expected mutations, linked-L0 logical-prefix checks, VACUUM maintenance
  authority, and 24 fixed-live cycles. Physical pages remain at or below
  `1666` under the frozen `1668`-page limit and finish exact with no actionable
  debt. The 40,000-base/10,000-delta five-trial matrix passes all `22` gates:
  static, fragmented, folded, impact-specialized, and static-reference top-100
  IDs/scores are exact with maximum drift `0.0`. Folded/static mean, p50, p95,
  p99, and QPS ratios are `1.0167x`, `1.0189x`, `1.0117x`, `1.0258x`, and
  `0.9836x`; fragmented/static ratios are `1.0062x`, `1.0099x`, `1.0072x`,
  `1.0067x`, and `0.9938x`. Slice 4k is closed with no observed ordering,
  lock, page byte, fork, WAL, root, ordinal, score, lifecycle, RSS, storage,
  package, or BM25 regression.

#### ARCH-4 Slice 5a: Linked-L0 XID Classification Authority Contract

The VACUUM, page-native query, semantic-quarantine, and overlay closures all
consume the same linked-L0 transaction-visibility classifier. Pending seal
shares its result-state type while retaining a separate safe-horizon audit.
The classifier and type are currently embedded in the AM entry module, so
extracting VACUUM or scan first would require a reverse call into
`ii42_am.c`. Establish the mutation module as the sole owner before either
larger authority moves.

Add `ii42_am_mutation.c/.h` and move exactly:

```c
typedef enum ii42_am_delta_xid_state {
    II42_AM_DELTA_XID_COMMITTED = 0,
    II42_AM_DELTA_XID_ABORTED,
    II42_AM_DELTA_XID_UNRESOLVED
} ii42_am_delta_xid_state;

bool ii42_am_delta_record_states(
    const TransactionId *record_xids,
    ii42_am_delta_xid_state *states_out,
    uint32 record_count
);
```

The function must preserve the current batch classification exactly:

1. reject null input/output only when `record_count > 0`;
2. identify whether any non-current normal XID requires clog inspection;
3. preserve the superuser-only
   `ii42.test_require_frozen_delta_xids` audit before acquiring the truncation
   lock;
4. hold `XactTruncationLock` shared across the complete clog-status batch and
   release it through the same `PG_TRY`/`PG_FINALLY` cleanup;
5. classify the aborted sentinel as aborted, invalid/frozen/non-normal XIDs as
   committed, current-transaction XIDs as unresolved, old-clog or committed
   XIDs as committed, aborted XIDs as aborted, and every other XID as
   unresolved;
6. return the unchanged `has_normal_xid` projection.

Boundary rules:

- mutation owns only this linked-L0 transaction-visibility primitive and its
  specialized test-audit lookup in this slice. It retains no XID array,
  snapshot, relation, root, buffer, lock, or backend-sized state after return;
- every existing classifier caller, including the page-query adapter,
  full-overlay oracle, semantic-quarantine scan, and VACUUM, continues to
  allocate and interpret its own result array. Pending seal continues to own
  its stricter safe-horizon classification while consuming the shared enum;
- no append, rotation, transaction queue, COW publication, VACUUM callback,
  semantic inference, query scoring, maintenance policy, preload, or SQL
  control may move in this slice;
- the AM-local enum, sentinel definition, and classifier are deleted
  immediately. No wrapper, callback, duplicate classifier, alternate snapshot
  rule, or reverse dependency on `ii42_am.c` may remain.

Mechanical acceptance gate:

1. `ii42_am_mutation.c/.h` have the sole enum, sentinel, implementation, and
   declaration. The four existing AM callers include the typed header and
   retain their exact call/result branches. Inventory freezes caller count,
   classification
   order, fault gate, lock scope, return value, and forbidden authority.
2. Object inspection finds one mutation-object definition, unresolved AM
   callers, and no exported dylib symbol. Query L0 visibility, overlay replay,
   seal safety, semantic quarantine, VACUUM retirement, and current-XID
   behavior remain exact.
3. Golden parity, native/mutable/SAE lifecycle, transaction/savepoint/2PC,
   restart, physical replication, 140K VACUUM frontier, same-index 8x8x24,
   storage rejection/recovery, compact/spill builders, schema/ACL, and the
   frozen 40K matrix pass against one staged artifact.
4. Warning-clean PG18, fresh CMake/CTest, inventory, modified-script
   `py_compile`, and `git diff --check` pass. Any MVCC, lock, root, WAL, score,
   lifecycle, RSS, storage, package, or BM25 performance change stops the
   slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb another mutation helper or modify an existing
caller's visibility policy. The dependency-audit result intentionally moves
this shared mutation primitive before the larger VACUUM and scan slices;
otherwise those modules could not depend one-way on mutation authority.

#### ARCH-4 Slice 5a Closure Evidence

- Commit `b3cba56b` adds `ii42_am_mutation.c/.h`, moves the enum, aborted-XID
  sentinel, frozen-XID audit, and batch classifier exactly once, and deletes
  the AM-local implementation. `ii42_am.c` retains the four typed consumers;
  pending seal retains its distinct safe-horizon audit while sharing only the
  result type. The entry module falls from `44,730` to `44,633` lines without
  moving append, rotation, COW publication, VACUUM, query, or semantic work.
- Inventory proves one mutation-object definition and one header declaration,
  the frozen caller count and branch order, shared `XactTruncationLock` scope,
  superuser-only fault gate, and absence of relation, buffer, segment, query,
  semantic, runtime, background-worker, and SPI authority. Object inspection
  finds unresolved AM callers, the sole mutation-object definition, and no
  exported classifier in the final dylib.
- A warning-clean PG18 PGXS build, fresh CMake build, CTest `1/1`, modified
  script `py_compile`, product inventory, and `git diff --check` pass. Source,
  staged, and installed artifacts are byte-identical at dylib SHA-256
  `ce0a53913ced2d28ed941653b36a5c0b85d5cff86d294177b7fbde50a64c42a0`.
  The independent golden manifest remains
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  with runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- The one-artifact staged matrix passes native page/scorer `80/80`, empty and
  UNLOGGED `7/7`, transaction/savepoint/2PC/restart `21/21`, mutable real-model
  `71/71`, SAE lifecycle `15/15`, schema/ACL, runtime-required, privilege,
  restart, compact/spill builders, all twelve storage rejection/recovery
  boundaries, isolated SQL regression `1/1`, and physical replication.
- The 140,000-document VACUUM frontier passes `10/10`. Same-index concurrency
  passes eight writers, eight readers, and all `288/288` operations; final
  semantic completion is converged with zero active/pending records and no
  runtime failure. This exercises query visibility, semantic quarantine,
  maintenance pinning, segment seal/compaction/reclamation, and VACUUM through
  the moved classifier.
- The 40,000-base/10,000-delta five-trial matrix passes all `22` gates. Static,
  fragmented, folded, impact-specialized, and static-reference top-100
  IDs/scores are exact with maximum drift `0.0`. Folded/static mean, p50, p95,
  p99, and QPS ratios are `1.0009x`, `0.9970x`, `1.0031x`, `1.0189x`, and
  `0.9991x`; fragmented/static ratios are `1.0031x`, `1.0040x`, `0.9966x`,
  `1.0005x`, and `0.9969x`. Slice 5a is closed with no observed MVCC, lock,
  root, WAL, score, lifecycle, RSS, storage, package, or BM25 regression.

#### ARCH-4 Slice 5b: Transaction-Scoped Maintenance Ownership Contract

Mutation append, VACUUM COW retirement, manual maintenance, page-reuse
preparation, and realtime maintenance probes share one relation-scoped lock
protocol. The lock tags, writer barrier, transaction pin list, and consistency
predicates are currently embedded in the AM entry module. Moving mutation or
VACUUM first would either reverse-call `ii42_am.c` or duplicate lock policy.
Establish `ii42_am_maintenance.c/.h` as the sole owner of that protocol before
moving either larger lifecycle authority.

Move exactly these existing authorities without changing names, lock modes, or
wait semantics:

- the maintenance and writer-barrier advisory lock tags;
- transaction-level try/blocking maintenance lock, session-level try/unlock,
  held-state projection, and explicit transaction-level early unlock;
- writer-barrier acquire/release with the existing caller-selected
  `LOCKMODE` and `dont_wait` behavior;
- the transaction-local pinned-index value/list, duplicate-index check,
  mode lookup, pin operation, subtransaction reparent/abort invalidation,
  present-state projection, and transaction-end clear;
- maintenance-tracking, eventual-policy, automatic-policy, and foreground
  maintenance predicates. The internal raw eventual-consistency predicate may
  remain private to this module.

The AM keeps the sole PostgreSQL transaction and subtransaction callbacks.
Those callbacks coordinate cache invalidation and deferred worker wakeup, but
must access pin state only through narrow maintenance APIs: present,
reparent/abort, and clear. The pending-background-wakeup flag remains AM-owned.
The pin operation keeps its current policy exactly: BM25 realtime acquires an
`ExclusiveLock`; SAE and non-realtime paths acquire a `ShareLock`; one index is
pinned once per top transaction; entries live in `TopTransactionContext` and
carry their originating subtransaction ID.

Boundary rules:

- transaction locks remain PostgreSQL transaction-owned and session locks
  remain explicitly session-owned. No module-local unlock is added at normal
  transaction end; PostgreSQL releases transaction locks. Early unlock remains
  only where the current no-publication paths already call it;
- aborting a subtransaction invalidates only its pin entry; committing one
  reparents it. Top-level clear drops the list reference and relies on
  `TopTransactionContext` cleanup exactly as today. The module retains no
  relation pointer, root, buffer, snapshot, model session, posting data, or
  index-sized backend state;
- maintenance may depend on immutable relation policy from
  `ii42_am_options.h`. Options, mutation, VACUUM, scan, semantic maintenance,
  preload, and the AM entry module may depend one-way on maintenance; no
  reverse include or callback into `ii42_am.c` is allowed;
- append-lock ownership, worker-slot locks, shared worker counters, scheduling,
  debt classification, candidate/action selection, COW publication, semantic
  inference, VACUUM callbacks, query scoring, preload residency, SQL wrappers,
  and transaction-callback registration do not move in this slice;
- the old lock tags, scoped-index type, pin list, predicates, prototypes, and
  implementations are deleted immediately. No wrapper, duplicate tag, second
  pin list, or alternate mode decision may remain.

Mechanical acceptance gate:

1. `ii42_am_maintenance.c/.h` have the sole moved definitions and state. The AM
   callback contains no direct pin-list access and preserves cache reset,
   precommit fault injection, deferred wakeup, and event ordering. Inventory
   freezes definition/caller counts, exact advisory tag fields, session flags,
   lock modes, allocation context, subtransaction handling, and cleanup.
2. Object inspection finds one maintenance-object definition per moved symbol,
   unresolved typed callers, and no new product export. The module has no root,
   WAL, segment, scorer, model/runtime, worker-policy, SPI, or SQL authority and
   no index-sized retained memory.
3. Current-XID/query visibility, writer serialization, maintenance busy/held
   probes, manual refresh rejection after writes, savepoint/abort/2PC, pending
   seal, semantic completion/quarantine, VACUUM COW retirement, page reuse,
   worker concurrency, restart, and physical replay remain exact.
4. Golden parity, native/mutable/SAE lifecycle, transaction and UNLOGGED gates,
   same-index 8x8 concurrency, 140K VACUUM frontier, replication, storage
   recovery, schema/ACL, isolated regression, builders, warning-clean PG18,
   fresh CMake/CTest, artifact identity, inventory, `py_compile`,
   `git diff --check`, and the frozen 40K matrix pass from one staged artifact.
5. Any lock acquisition/release change, deadlock, MVCC difference, root/WAL or
   score drift, lifecycle/RSS/storage/package change, or BM25 regression stops
   the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb append publication, maintenance scheduling, action
selection, VACUUM, semantic, scan, preload, or SQL-control code. Closing this
slice is the prerequisite for one-way mutation and VACUUM extraction; it is
not itself a new maintenance lifecycle.

#### ARCH-4 Slice 5b Closure Evidence

- Commit `a8aa33c7` adds `ii42_am_maintenance.c/.h` as the sole owner of the
  maintenance and writer-barrier lock tags, transaction/session lock helpers,
  transaction pin list, subtransaction handling, and consistency predicates.
  The AM keeps callback registration, cache reset, and deferred worker wakeup,
  and now reaches pin state only through typed present, reparent, and clear
  operations. Append locking, scheduling, action selection, VACUUM, semantic,
  scan, preload, and SQL control do not move.
- Inventory freezes the exact advisory tag fields, lock/session flags, BM25
  realtime `ExclusiveLock`, SAE/eventual `ShareLock`,
  `TopTransactionContext` ownership, subtransaction commit/abort handling,
  caller counts, and one-way options dependency. Object inspection finds
  unresolved AM callers, the sole maintenance-object definitions, no product
  export, and no root, WAL, segment, scorer, runtime, worker, SPI, or retained
  index-sized authority in the new module.
- A warning-clean PG18 PGXS build, fresh CMake build, CTest `1/1`, modified
  script `py_compile`, product inventory, and `git diff --check` pass. Source,
  staged, and installed artifacts are byte-identical at dylib SHA-256
  `5955d909d232f7c8dea62d9c404a3d8b269994595b2b29f2af352a7d245e61d0`.
  The independent golden manifest remains
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  with runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- The one-artifact staged matrix passes native page/scorer `80/80`, empty and
  UNLOGGED `7/7`, transaction/savepoint/2PC/restart `21/21`, mutable real-model
  `71/71`, SAE lifecycle `15/15`, schema/ACL, runtime-required, privilege,
  restart, compact/spill builders, all twelve storage rejection/recovery
  boundaries, isolated SQL regression `1/1`, and physical replication.
- The first native run overlapped another temporary-cluster gate and exhausted
  its bounded fixed-live-set seal attempts at the safe `xid_horizon` fence. A
  clean serial rerun passed `80/80`; the fence therefore remained fail-closed
  rather than publishing across an unsafe snapshot. Horizon-sensitive
  lifecycle gates remain serial in this qualification evidence.
- The 140,000-document VACUUM frontier passes `10/10`. Same-index concurrency
  passes eight writers, eight readers, and all `288/288` writer operations;
  a second writer enters in `0.658 ms`, maintenance defers behind the active
  writer, final linked-L0 debt is zero, semantic completion is converged, and
  the shared runtime reports no failure.
- The 40,000-base/10,000-delta five-trial matrix passes all `22` gates. Static,
  fragmented, folded, impact-specialized, and static-reference top-100
  IDs/scores are exact with maximum drift `0.0`. Folded/static mean, p50, p95,
  p99, and QPS ratios are `0.9995x`, `1.0007x`, `0.9965x`, `1.0005x`, and
  `1.0005x`; fragmented/static ratios are `1.0066x`, `1.0080x`, `1.0081x`,
  `0.9991x`, and `0.9934x`. Slice 5b is closed with no observed MVCC, lock,
  root, WAL, score, lifecycle, RSS, storage, package, or BM25 regression.

#### ARCH-4 Slice 5c: Linked-L0 Append-Lock Authority Contract

Foreground linked-L0 mutation, maintenance rotation, semantic transition, and
COW root publication all serialize their short physical frontier changes with
one advisory append lock. Its tag and helpers remain embedded in the AM entry
module after transaction-scoped maintenance ownership moved. Leaving that lock
behind would force later mutation, publication, semantic, and VACUUM modules to
reverse-call `ii42_am.c`. Extend the existing maintenance lock authority rather
than introduce a second lock module.

Move exactly:

```c
#define II42_AM_APPEND_LOCK_TAG UINT32_C(0x32534241)

void ii42_am_lock_append(Relation index_relation);
void ii42_am_unlock_append(Relation index_relation);
```

The implementation must preserve the current protocol exactly:

1. derive one PostgreSQL advisory lock tag from `MyDatabaseId`, the frozen
   append tag, and `RelationGetRelid(index_relation)`;
2. acquire a transaction-owned `ExclusiveLock` with `dont_wait=false` and no
   session ownership;
3. release that same transaction-owned lock and raise
   `ii42 append lock is not held` if release fails;
4. retain no relation pointer, lock tag, root, frontier, buffer, snapshot, or
   backend-sized state after return.

Boundary rules:

- all six lock callers and eight unlock callers continue to bracket their
  existing root revalidation, linked-L0 append/rotation, COW publication, or
  semantic-transition critical sections. No caller changes lock ordering,
  `PG_TRY`/`PG_FINALLY` cleanup, WAL flush, or publication behavior;
- maintenance owns only the append lock primitive in this slice. Linked-L0
  page layout, root validation, record encoding, sequence reservation, append,
  rotation, COW publication, semantic work, VACUUM, action selection,
  scheduling, preload, scan, and SQL control do not move;
- the AM-local tag, prototypes, and implementations are deleted immediately.
  No wrapper, duplicate tag, alternate lock mode, try-lock variant, early
  unlock policy, or reverse include of `ii42_am.c` may remain;
- mutation, semantic maintenance, VACUUM, publication, scan, preload, and the
  AM entry may depend one-way on maintenance lock authority. Maintenance may
  continue to depend only on immutable options and PostgreSQL lock APIs.

Mechanical acceptance gate:

1. `ii42_am_maintenance.c/.h` have the sole tag, implementation, and typed
   declarations. Inventory freezes exact tag fields, lock flags, caller counts,
   error text, and forbidden authority. Object inspection finds unresolved AM
   callers, sole maintenance-object definitions, and no product export.
2. Current-XID mutation visibility, writer serialization, active/pending L0
   rotation, pending seal, semantic completion/quarantine, COW publication,
   VACUUM retirement, page reuse, restart, and physical replay remain exact.
3. Warning-clean PG18, fresh CMake/CTest, golden, native, transaction,
   mutable/SAE, same-index 8x8, 140K VACUUM, storage, builders, regression,
   replication, schema/ACL, inventory, `py_compile`, `git diff --check`, and
   the frozen 40K matrix pass from one staged artifact.
4. Any lock-order or wait change, deadlock, MVCC difference, root/WAL or score
   drift, lifecycle/RSS/storage/package change, or BM25 regression stops the
   slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb any linked-L0, publication, maintenance-action,
semantic, VACUUM, scan, preload, or SQL-control implementation. Closing this
slice supplies a shared one-way serialization primitive; it does not create a
second maintenance lifecycle.

#### ARCH-4 Slice 5c Closure Evidence

- Commit `8f77f7f6` moves only the linked-L0 append advisory tag and typed
  lock/unlock helpers into `ii42_am_maintenance.c/.h`. All six lock and eight
  unlock call sites retain their existing critical sections. Linked-L0 page
  layout, root validation, append/rotation, COW publication, semantic work,
  VACUUM, action selection, scheduling, preload, scan, and SQL control do not
  move.
- Inventory freezes tag fields, transaction ownership, `ExclusiveLock`,
  blocking acquisition, exact release error, caller counts, and one-way module
  ownership. Object inspection finds unresolved AM callers and the sole
  maintenance-object definitions; the final dylib adds no public symbol.
- Warning-clean PG18 PGXS and fresh CMake builds, CTest `1/1`, modified-script
  `py_compile`, product inventory, and `git diff --check` pass. Source, staged,
  and installed artifacts are byte-identical at dylib SHA-256
  `0a31b078bb2f552ad33ef9783f00f18db2ef9c496a449fba5c0bd51ae2962782`.
  The independent golden manifest remains
  `5792543cfb259cb969136315e4072bf90f0542345aa636062878649d88414d32`
  with runtime signature `5f24130eb550dd59f88fdab3cc1c1dda`.
- The serial one-artifact matrix passes native page/scorer `80/80`, empty and
  UNLOGGED `7/7`, transaction/savepoint/2PC/restart `21/21`, mutable real-model
  `71/71`, SAE lifecycle `15/15`, runtime-required/privilege/restart,
  compact/spill builders, all twelve storage rejection/recovery boundaries,
  isolated SQL regression `1/1`, physical replication, and schema/ACL.
- The 140,000-document VACUUM frontier passes `10/10`. Same-index concurrency
  passes eight writers, eight readers, and all `288/288` writer operations; a
  second writer enters in `0.549 ms`, maintenance defers behind the active
  writer, final linked-L0 debt is zero, semantic completion converges, and the
  shared runtime reports no failure.
- The 40,000-base/10,000-delta five-trial matrix passes all `22` gates. Static,
  fragmented, folded, impact-specialized, and static-reference top-100 IDs and
  scores are exact with maximum drift `0.0`. Folded/static mean, p50, p95, p99,
  and QPS ratios are `1.0028x`, `1.0023x`, `1.0054x`, `0.9957x`, and
  `0.9972x`; fragmented/static ratios are `1.0058x`, `1.0077x`, `1.0076x`,
  `1.0099x`, and `0.9943x`. Slice 5c is closed with no observed lock-order,
  MVCC, root, WAL, score, lifecycle, RSS, storage, package, or BM25 regression.

#### ARCH-4 Slice 5d: Scan Visibility-Context Authority Contract

Every ranked, filtered, phrase, raw, bitmap, and native page-query route uses
one PostgreSQL heap-visibility context. Its value type and lifecycle remain in
the AM entry module even though they own no scorer or durable state. Establish
`ii42_am_scan.c/.h` with this exact visibility boundary before moving scan
opaque state or AM scan callbacks.

Move exactly:

- `ii42_am_visibility_ctx`;
- `ii42_am_visibility_begin()` and
  `ii42_am_visibility_begin_with_snapshot()`;
- `ii42_am_tid_visible()` and `ii42_am_tid_visible_as()`;
- `ii42_am_visibility_end()`.

The implementation must preserve the current protocol exactly:

1. the caller owns an open heap relation and a snapshot whose lifetime exceeds
   every visibility probe; `begin()` borrows `GetActiveSnapshot()` while the
   explicit form borrows the supplied snapshot;
2. the context owns one `table_index_fetch_begin()` handle and one
   `TTSOpsBufferHeapTuple` slot in the caller's current memory context;
3. each probe resets the table fetch, follows PostgreSQL's `call_again` chain,
   clears the slot after every attempt, and returns the slot TID when valid or
   the current heap TID otherwise;
4. `end()` remains null-safe and releases the slot before the table-fetch
   handle, then clears all borrowed pointers. It retains no relation, tuple,
   snapshot, result, or index-sized state after return.

Boundary rules:

- this slice moves no AM callback, scan opaque/result owner, query parser,
  page-native builder, ranking, scorer, token expansion, phrase verification,
  highlight rendering, runtime/model request, root read, linked-L0 projection,
  preload, SQL function, or test hook;
- `ii42_am_scan` depends only on PostgreSQL executor/table/snapshot APIs. It
  must not include AM options/meta/build/mutation/maintenance, segment, scorer,
  runtime, preload, SPI, WAL, worker, or SQL-control headers;
- all existing callers keep the same relation, snapshot, `PG_TRY` cleanup, and
  memory-context scope. No snapshot registration, copy, ownership transfer,
  alternate HOT-chain policy, visibility cache, or callback indirection is
  introduced;
- the AM-local type, prototypes, and implementations are deleted immediately.
  No wrapper, duplicate context, or alternate visibility route may remain.

Mechanical acceptance gate:

1. `ii42_am_scan.c/.h` have the sole type, declarations, and implementations.
   Inventory freezes definition/caller counts, table-fetch/slot construction,
   `call_again`, slot clearing, visible-TID fallback, cleanup order, and
   forbidden authority. The new PGXS object is not added to the independent
   core CMake target.
2. Object inspection finds unresolved AM callers, sole scan-object
   definitions, and no new dylib export. The scan module has no static mutable
   state and retains no backend memory between calls.
3. Golden, native, transaction, mutable/SAE, same-index 8x8, 140K VACUUM,
   empty/UNLOGGED, storage, builders, regression, replication, runtime-service,
   schema/ACL, inventory, `py_compile`, `git diff --check`, and the frozen 40K
   matrix pass from one staged artifact.
4. Any visible-TID, HOT-chain, bitmap/ordered result, score, snapshot, cleanup,
   RSS, root/WAL/lock, lifecycle, package, or BM25 performance difference stops
   the slice.

This is a planning-only contract. Its mechanical extraction is a separate
commit and may not absorb scan callbacks or any query/scorer implementation.
Closing it creates the first narrow scan authority; it does not create a second
query path.

#### ARCH-4 Stop Gate: CSG-I123 Reader-Fence And Retirement-Order Ownership

The first slice-5d same-index replay reproduced the CSG-I120/I121 corruption
signature: concurrent readers rejected an L0 retirement target and final
maintenance rejected a document-directory append patch. This is not accepted
as test noise. The exact frozen 8-writer/8-reader, 12-cycle pre-5d artifact
completed all 288 operations, while the otherwise identical slice-5d artifact
failed. A diagnostic build that disabled retired-page reuse produced no
corrupt read or COW object, but could not complete the expected reclamation
action. The extraction changes timing, but the reusable-page lifetime defect
is in the parent design.

Static review identifies the missing ownership edge. The current reuse helper
conditionally takes the relation `AccessExclusiveLock`, drains old readers,
revalidates the root, derives an arena from manifest-authenticated retired
ranges, and then releases the relation lock before any reused page is written
or the replacement root is published. A new reader can therefore acquire the
still-current old root after the drain and before publication, then observe a
retired page while maintenance overwrites it. Root revalidation at publication
cannot repair an old reader whose immutable page was already reused.

Close CSG-I123 on the pre-5d parent before restoring the scan extraction:

1. Make successful retired-page reuse transfer explicit ownership of the
   nonblocking relation reader fence to its caller. Keep that fence from the
   final root/range revalidation through every reused-page write, replacement
   root publication, WAL flush, and FSM handoff. Release it in `PG_FINALLY()`
   on success, deferral, or error.
2. Keep reuse opportunistic. Build CPU-heavy COW inputs before requesting the
   fence; if `ConditionalLockRelation()` cannot acquire it, publish through the
   existing append/FSM path without waiting. Ordinary foreground mutation and
   no-reuse maintenance must not acquire this fence.
3. Apply the ownership protocol to reclamation, compaction, neutral fold, and
   impact specialization. No caller may receive retired blocks after the
   fence is released, and no second allocator or persistent free-list
   lifecycle may be introduced.
4. Add a deterministic probe that pauses a reuse owner after the arena is
   authenticated, starts an old-root query, proves the query waits on the
   relation fence until root publication, then requires exact normal/oracle
   rows, valid COW objects, and clean convergence. Freeze the helper's lock
   transfer and every caller's unconditional cleanup in inventory.
5. Repeat the 8x8 real-model gate at 12 and 24 cycles from one staged artifact,
   plus native, mutable, SAE, transaction, 140K VACUUM, restart, replication,
   runtime-service, storage plateau, builders, schema/ACL, package, and 40K
   BM25 performance gates. Any deadlock, blocking fallback, root/score change,
   storage growth, or measurable no-reuse regression rejects the repair.

The first ownership implementation proved the deterministic fence and passed
the 8x8 12-cycle gate, but one 8x8 24-cycle run still reproduced both the L0
retirement-target and document-COW patch failures. The same binary then passed
both 1-reader/8-writer and 8-reader/1-writer 24-cycle controls. Two later
8-reader/8-writer 24-cycle repeats also completed 576/576 mutations without a
query or maintenance error. The mixed-pressure failure remains timing-sensitive
and cannot yet be dismissed. Current-root diagnostics additionally exposed a
separate storage-lifetime defect: one repeat left 30 interior-unreachable pages
and another left one page that was neither a retained retirement hint nor a
recyclable marker. This accounting gap does not explain the observed query/COW
corruption because the reuse allocator consumes only manifest-authenticated
retired ranges, but it invalidates a storage-plateau closure claim and is now
tracked independently by CSG-I124.

A forced 16-writer/8-reader replay now makes the remaining failure repeatable.
The first rejected document had immutable `born_sequence=447`, immutable
`retirement_sequence=511`, and a later visible `RETIRE` at sequence `563`.
There was no visible intervening event after the immutable retirement. Readers
observed the same state both before and after pending-L0 seal: manifest 98 plus
pending segment 81, and manifest 99 with that pending segment incorporated.
This excludes unauthenticated page reuse as the direct cause and identifies an
orphan duplicate retirement produced across VACUUM/COW/L0 publication. A
`RETIRE` record does not carry the target incarnation's born sequence or heap
TID, so the query path must not make an already-retired target a generic no-op:
the same relaxation could retire a later reused slot with the same length.

Test-only producer provenance selects the exact boundary. The failing sequence
`329` was reserved by the dead-L0-upsert branch for slot 165 and source born
sequence 203. That slot had already been materialized as born 203 and retired
at shared COW fence 248. The physical L0 `RETIRE` at 248 belonged to slot 114;
one reservation sequence intentionally retires every dead upsert in that COW
patch. A later VACUUM therefore cannot infer slot 165's retirement from the L0
record set alone and reserves it again unless append-time source validation
also checks the current immutable record.

Before CSG-I123 can close, the implementation must additionally:

6. Attribute every test retirement reservation to the immutable-COW candidate
   or L0-upsert branch and capture its source root, slot incarnation, sequence,
   and publication result. Remove this temporary provenance after the first
   failing producer boundary is proven.
7. Make the producer validate and reserve a retirement only while the current
   slot incarnation and its publication authority are protected. A stale COW
   candidate, stale L0 upsert, no-op COW patch, or raced publication must not
   leave a visible retirement record. Keep append, writer-barrier, and
   maintenance lock order acyclic; do not add query-side tolerance.
8. Pass the L0 upsert's expected born sequence into append-time validation.
   Under the existing append lock, suppress the reservation when the current
   immutable incarnation has the same born sequence and is already retired, or
   when a newer incarnation owns the slot. Add a deterministic regression with
   at least two dead L0 upserts: the first VACUUM materializes both under one
   shared retirement fence, the second VACUUM must add no duplicate retirement,
   and normal/oracle queries must remain exact before and after seal.
9. Preserve the reader fence and prove separately that every query-visible root
   is drained before a referenced page becomes recyclable. Then require
   repeated 16x8 forced-overlap and 8x8 24-cycle success from one staged
   artifact. The one-axis controls remain mandatory diagnostics but cannot
   substitute for mixed pressure.

CSG-I123 focused implementation evidence:

- Commit `f649b60d` retains the relation reader fence through reused-page
  writes and replacement-root publication, and binds a dead-L0 retirement
  reservation to its expected born sequence under the existing append lock.
  It does not change the root format, scorer, writer barrier, maintenance lock
  order, or query acceptance rules.
- The deterministic shared-retirement regression starts with two dead L0
  upserts. The first `VACUUM` changes the linked frontier from two records to
  three, the second leaves it at three, and normal/oracle results are exactly
  `stable` before the repeated `VACUUM`, after it, and after seal. The separate
  retired-page reader-fence probe proves an old-root query waits for the
  relation fence and remains exact after reclamation.
- `/private/tmp/ii42-csg-i123-16x8x48-a.json` and
  `/private/tmp/ii42-csg-i123-16x8x48-b.json` each complete `2304/2304`
  foreground mutations with 16 writers, eight readers, concurrent maintenance,
  and concurrent `VACUUM`. Both finish with zero linked-L0 debt, equal
  normal/oracle results, and no actor/runtime failure. The same installed PG18
  artifact also passes `/private/tmp/ii42-csg-i123-8x8x24.json` and the focused
  no-model gate.
- Test-only producer provenance has been removed. PG18 PGXS build, core `1/1`,
  product inventory, Python compile, and `git diff --check` pass. The corruption
  subgate is therefore satisfied. Formal CSG-I123 closure remains withheld
  until the complete staged matrix runs after CSG-I124, because the same stress
  artifacts still expose the independent retirement-accounting leak.

The CSG-I123 contract, implementation, closure evidence, and restored 5d
extraction remain separate commits. The post-CSG-I124 staged matrix now closes
this stop gate at `d0d11e0f`; slice 5d resumes from that exact parent and may
move only the previously frozen visibility-context authority.

#### ARCH-4 Stop Gate: CSG-I124 Lossless Retirement Overflow

`ii42_segment_pages_prepare_retired_ranges()` currently bounds the manifest to
64 ranges by deleting the smallest ranges from its in-memory inventory. Those
pages are no longer reachable from the new root, are no longer authenticated by
the manifest, and have not been converted to WAL-logged recyclable markers.
High-churn 8x8x24 evidence observed exactly this leak: all query and mutation
checks passed, but retired hints plus recyclable markers did not account for all
interior-unreachable blocks. Silent range deletion is not an acceptable bounded
metadata policy because ordinary maintenance can never recover those pages.

Close CSG-I124 without introducing a persistent free-list or second lifecycle:

1. Never publish a root that drops retirement evidence. Before publication,
   either coalesce exact adjacent ranges, convert excess ranges to recyclable
   markers while holding the existing reader fence, or defer the maintenance
   action without changing the current root.
2. Keep the manifest limit fixed and keep foreground mutation nonblocking.
   Expensive orphan discovery remains an explicit scrub concern; this repair
   only preserves pages retired by the current checked transition.
3. Add a deterministic overflow regression with more than 64 disjoint retired
   ranges. Require every interior-unreachable block to be either a current
   manifest retirement hint or a valid recyclable marker before and after
   restart, then prove later maintenance can reuse the blocks.
4. Repeat mixed 8x8x24 stress and the fixed-live storage plateau. Any lost
   range, unbounded manifest growth, relation-lock wait in foreground mutation,
   score/root-format change, or unexplained physical growth rejects the repair.

Selected implementation boundary:

1. Every non-reader-fenced COW path must determine whether its exact retirement
   inventory fits the 64-range manifest before its first physical page write.
   Append, replacement, and document patch use exact in-memory COW trees.
   Vocabulary growth also builds its lexicon, prefix, and term patches before
   writing pages. The term preflight uses a valid unpublished fixed-width
   catalog ref; after fit is proven, the real catalog is written, the term patch
   is rebuilt with its physical ref, and the retired ranges must remain exactly
   equal. A single-term fold may use the exact document patch plus the proven
   maximum one-leaf/one-path term-tree retirement bound. No post-write retry is
   legal.
2. An overflow returns `reader_fence_required`; it is not an ERROR and does not
   publish or leave candidate pages. The caller may retry only after obtaining
   the existing relation reader fence conditionally and revalidating the same
   root. Failure to obtain that fence defers the bounded maintenance action;
   foreground mutation must never wait for this repair.
3. A reader-fenced transition writes no retirement inventory into the next
   manifest. After descendant-root publication and WAL flush, it converts both
   newly retired ranges and unconsumed ranges from the old reuse arena into
   checked recyclable markers, then exposes those pages through the index FSM.
   The handoff range vector is dynamically sized and has explicit result
   ownership; replacing the manifest cap with another fixed handoff cap is not
   lossless.
4. All range combination uses the existing canonical block-range inventory.
   The manifest, marker pages, and FSM remain the only authorities. No overflow
   object, persistent side list, query fallback, or orphan scan is introduced.

The CSG-I124 contract, implementation, and closure evidence are separate
commits. CSG-I123 remains the corruption stop gate; neither issue may be closed
using evidence from the other.

#### ARCH-4 Slice 5d Closure Evidence

Commit `84776e01` moves only the frozen scan-visibility value type and its five
lifecycle/probe functions into `ii42_am_scan.c/.h`. Commit `369a3463` binds the
compact and spill maintenance-builder smokes to an explicit staged extension
without changing product behavior. The extraction leaves `ii42_am.c` at
44,515 lines, a reduction of 123 lines; file size is recorded only as evidence
that ownership moved rather than being copied.

- Product inventory proves one type owner, one implementation owner for each
  function, and frozen AM caller counts of `5/7/1/5/5`. It also freezes table
  fetch and slot construction, `call_again`, slot clearing, visible-TID
  fallback, cleanup order, includes, and forbidden dependencies.
- `ii42_am_scan.o` alone defines the five scan-visibility symbols. The final
  dylib has no added or removed public symbol name, and the scan object has no
  unexpected unresolved project authority.
- Normal ONNX-enabled and no-ONNX PG18 PGXS builds are warning-clean. The
  independent CMake/core target remains unchanged and passes `1/1`. The final
  local and staged dylibs are byte-identical at SHA-256
  `f71f1544fa6b719d67715a7dc85faff8c60de320b5dd3b370c8e0df664eea1b2`.
- One staged artifact passes native `80/80`, transaction `21/21`, mutable
  real-model `71/71`, SAE lifecycle `15/15`, empty/UNLOGGED `7/7`, 140,000-row
  VACUUM-frontier `10/10`, exact 8-writer/8-reader 24-cycle concurrency with
  `576/576` expected operations, golden-oracle, storage plateau, physical
  replication, runtime required/privilege/restart, schema/ACL, SQL regression
  `1/1`, and compact plus spill builder gates.
- The first native attempt exhausted its bounded `xid_horizon` retry budget
  before producing a result. This previously documented timing-safe outcome
  did not recur in the clean serial rerun, which passed `80/80`; no corruption,
  score difference, or lifecycle failure was accepted or hidden.
- The final 40,000-document matrix passes all 22 gates with maximum top-100
  score drift `0.0`. Folded/static mean, p50, p95, p99, and QPS ratios are
  `1.0165x/1.0200x/1.0229x/1.0175x/0.9837x`; fragmented/static ratios are
  `1.0177x/1.0175x/1.0331x/1.0335x/0.9826x`.
- The final 250,000-document, five-segment matrix also passes all 22 gates with
  maximum top-100 score drift `0.0`. Folded/static mean, p50, p95, p99, and
  QPS ratios are `0.9995x/1.0048x/1.0170x/0.9983x/1.0005x`;
  fragmented/static ratios are
  `1.0093x/0.9947x/1.0282x/1.0167x/0.9908x`.
- The final backend-memory gate passes from the same staged artifact. Expanding
  from 5,000 to 80,000 rows leaves II-42 memory-context growth at zero and
  index-scaled backend-private live writable growth at 917,504 bytes, below
  the 16 MiB limit. The checked root remains page/shared-buffer authority.

No snapshot ownership, HOT-chain behavior, result, score, root byte, lock/WAL
order, lifecycle, RSS, storage, package, or BM25 regression was observed.
Slice 5d is closed; `ii42_am_scan` is the sole narrow visibility authority and
no second query path exists. This qualifies the functional milestone but does
not close CSG-I114: major callback implementations and shared-preload,
semantic-maintenance, VACUUM, and scan orchestration still reside in
`ii42_am.c`.

#### ARCH-4 Slice 5e: Source-Schema Option Authority

Commit `9988e95b` moves the unchanged indexed-column count, multicolumn,
source-type validation, and textlike-source predicates from `ii42_am.c` into
`ii42_am_options.c/.h`. These functions interpret relation options and indexed
column shape; placing them beside reloption validation removes a duplicated
schema authority without changing any callback, score, lock, WAL, memory, or
error contract. The monolith is reduced from 44,515 to 44,411 lines.

- A warning-clean PG18 PGXS build links one implementation owner. Object
  inspection shows callers in `ii42_am.o`, definitions in
  `ii42_am_options.o`, and no new public dylib export.
- The product inventory and `git diff --check` pass. A DESTDIR-staged PG18
  package passes the isolated non-public-schema installation smoke, including
  fresh, pinned, and intentional non-relocatable cases.
- A Clang call-graph and repository-reference audit found no unreferenced
  `ii42_am.c` function that can safely be deleted. The 12 white-box
  `ii42_test_*` hooks are all used by the independent page-native golden smoke
  and none is installed by product SQL. Inventory now enforces exact equality
  between hook implementations and test references and rejects installed-SQL
  exposure.
- The retained `generation_cache` and `mutable_delta` names are documented
  public diagnostics for shared derived residency and the linked-L0 read view.
  They do not own storage or a second lifecycle, so cosmetic renaming would add
  compatibility debt without removing an authority.

Slice 5e is closed. The dead-code audit closes CSG-I126 and CSG-I127 without
deleting active test coverage or churning stable diagnostics. CSG-I114 remains
open for the larger callback authorities that cannot be separated by a pure
body relocation.

#### ARCH-4 Slice 5f: Policy Recommendation Authority

Commit `637415af` moves the unchanged policy-recommendation result type,
consistency-name projection, and BM25/SAE recommendation function into
`ii42_am_options.c/.h`. The policy module reads only the immutable relation
options and checked metapage/debt projections; it acquires no lock, publishes
no root or WAL, runs no inference, and owns no mutable lifecycle state.

- A warning-clean PG18 PGXS build produces one caller in `ii42_am.o`, one
  implementation in `ii42_am_options.o`, and no new public dylib export.
- Product inventory, dependency inspection, and `git diff --check` pass.
- The staged retired-layout boundary rejects policy access before explicit
  `REINDEX`, then restores native v3 CRUD and query behavior.
- The same staged artifact passes the native v3 `80/80` matrix, including
  valid BM25 recommendation output, linked-L0 debt projection, contract drift,
  CRUD, VACUUM, fold, compaction, restart, and exact page-native scoring.

Slice 5f is closed. `ii42_am.c` is now 44,273 lines. This is the medium Gate C
checkpoint for the policy portion of `ii42_am_options`; no SAE, 2PC,
replication, RSS, package, or performance matrix was rerun for this mechanical
move.

#### ARCH-4 Slice 5g: Source Build-Mode Authority

Commit `be56507b` moves the unchanged source-type-to-build-mode enum and
validator from `ii42_am.c` into `ii42_am_options.c/.h`. Build mode is a
definition-time projection of source type and indexed-column count, so it
belongs with the source-schema option authority rather than the pure build
helpers.

The first attempted placement in `ii42_am_build` was rejected before commit:
product inventory detected its `ereport()` contract and correctly refused to
let a pure build-admission module acquire definition-time validation authority.
The gate was not weakened. The final placement leaves build orchestration,
storage publication, locks, WAL, scoring, and runtime state unchanged.

- A warning-clean PG18 PGXS build links callers from `ii42_am.o` to the sole
  definition in `ii42_am_options.o`; the helper is not a public dylib export.
- Product convergence inventory and `git diff --check` pass.
- A fresh DESTDIR-staged PG18 artifact passes the isolated extension-schema
  smoke, binding the evidence to the corrected options-module placement.
- Repository references and the generated call graph remain the deletion
  authority. No function or source file is removed merely because its name
  appears old; removal requires proof that it is absent from direct calls,
  callback tables, SQL entry points, tests, build manifests, and installed
  runtime behavior.

Slice 5g is closed. `ii42_am.c` is now 44,228 lines. No score, root, lock,
WAL, memory, lifecycle, or public API behavior changed, so the full Gate D
matrix remains deferred until all accepted ARCH-4 slices settle.

#### ARCH-4 Slice 5h: Mutable Metapage Projection Authority

Commit `d4bd8949` moves the unchanged stale-flag update, manual-policy debt
accounting, and VACUUM statistics projection into `ii42_am_meta.c/.h`. These
operations mutate or project only the checked metapage. They retain the exact
append-lock, transaction pin, buffer-lock, critical-section, and WAL order;
VACUUM callbacks and maintenance scheduling remain outside the meta module.

- The inventory requires one implementation and declaration for each public
  projection, keeps the flag updater private, checks the exact maintenance-lock
  call split, and rejects scheduling, worker, model, query, segment-publication,
  build-scan, and `ambulkdelete` authority in the closure.
- A warning-clean PG18 PGXS build links the three former AM-private callers to
  `ii42_am_meta.o`; no helper is exported by the public dylib.
- The DESTDIR-staged mutable lifecycle passes all `71/71` gates, including
  BM25 realtime, SAE eventual, linked-L0 MVCC, manual debt, VACUUM tombstones,
  REINDEX, restart, semantic completion, and bounded backend state.
- `py_compile`, product inventory, symbol inspection, and `git diff --check`
  pass. The full replication, package, RSS soak, and performance matrix remains
  deferred to Gate D.

Slice 5h is closed. `ii42_am.c` is now 44,079 lines. The complete VACUUM
cluster still has dependencies on L0 append, COW publication, reader-fence,
segment-identity, work-hint, and test-fault authorities. It must not move until
those dependencies have proper one-way owners; a catch-all service table or
reverse dependency on the AM entry module is forbidden.

#### ARCH-4 Slice 5i: L0 Source-Incarnation Guard Authority

Commit `e7b1c736` moves the unchanged L0 expected-source predicate into
`ii42_am_mutation.c/.h`. The guard compares complete source state for ordinary
L0 actions and the immutable slot incarnation for RETIRE, preventing semantic
completion or quarantine state from defeating a valid VACUUM retirement while
also preventing a reused slot from inheriting it.

- Inventory requires one mutation-module owner, one typed declaration, and the
  exact append/VACUUM caller pair. It rejects relation, buffer, WAL, root,
  publication, segment-page, model, runtime, and query authority in the leaf.
- A warning-clean PG18 build, symbol ownership, private dylib inspection,
  `py_compile`, product inventory, and `git diff --check` pass.
- The staged same-index concurrency gate exits successfully. Its shared
  retirement-sequence guard and VACUUM maintenance-authority checks both pass,
  including slot reuse, duplicate-retirement prevention, query/oracle parity,
  L0 convergence, and maintenance-lock waiting.

Slice 5i is closed. `ii42_am.c` is now 44,045 lines. This removes one VACUUM
reverse dependency without introducing a generic service interface. The full
Gate D matrix remains deferred until the accepted module set settles.

#### ARCH-4 Slice 5j: L0 Rotation-Policy Authority

Commit `77370e38` moves the unchanged active-L0 record-limit projection from
`ii42_am.c` into `ii42_am_mutation.c/.h`. The helper owns only the production
default and the superuser-only test override used to force bounded L0 rotation;
it does not acquire a relation or buffer lock, publish WAL or a root, or own
shared state.

- Inventory requires one mutation-module definition, one typed declaration,
  and the single linked-L0 append caller. It also preserves the exact default,
  superuser check, and maximum-value validation of the test override.
- A warning-clean PG18 build links `ii42_am.o` as a caller and
  `ii42_am_mutation.o` as the sole definition; the helper is not a public dylib
  export. `py_compile`, product inventory, and `git diff --check` pass.
- The focused staged text-seal gate forces the record threshold, observes
  `active_l0_rotated`, and then converges through `pending_l0_sealed`.
- The first full native replay stopped in fixed-live-set churn after an
  `xid_horizon` repeated across rotation. This was not treated as a pass. An
  unchanged second replay completed all `80/80` exact page-native gates,
  including fixed-live-set churn, transaction boundaries, rotation, VACUUM,
  COW fold, compaction, restart, ordered scans, and score parity. The isolated
  first failure remains recorded for final Gate D reproducibility review.

Slice 5j is closed. `ii42_am.c` is now 43,998 lines. No obsolete implementation
was discovered during this slice: source/header files remain reachable from the
build and include graph, current test hooks remain referenced by the independent
golden smoke, and product-source `obsolete`/`retired` names describe active
cache or reclamation states. Future deletion requires correlated proof across
direct calls, callback tables, SQL entry points, tests, build manifests,
installed package behavior, and generated call graphs; naming or apparent age
alone is not deletion evidence.

#### ARCH-4 Slice 5k: Linked-L0 Writer Authority

Commit `cd5c522a` moves the complete linked-L0 page/write closure from
`ii42_am.c` into `ii42_am_mutation.c/.h`: page framing and validation, exact
metapage-root comparison, root advancement and WAL publication, active-to-
pending rotation, inline and chained record append, reusable document-slot
selection, source-incarnation validation, and hard-frontier enforcement.

The mutation module does not acquire shared-preload or worker-scheduling
authority. Instead, it returns one typed `maintenance_due` outcome. The AM
entry module retains a small `PG_FINALLY` adapter that converts this outcome
into the existing shared work hint. Both pre-error pending-frontier pressure
and successful append-to-pending conditions set the outcome, so an error still
wakes urgent maintenance after mutation lock cleanup without giving the
mutation module a reverse dependency on preload state.

- A warning-clean PG18 build links the AM wrapper as the sole caller of
  `ii42_am_mutation_append_l0_record`; the root guard, root writer, rotation,
  and append entry all have one mutation-module definition and no public dylib
  export.
- Product inventory now checks XID classification and source-incarnation
  guards as pure sub-closures while allowing the enclosing module its intended
  relation, buffer, GenericXLog, and segment-page authority. It rejects work
  hints, shared preload, scheduling, workers, SPI, model, query, runtime, and
  rebuild authority in the mutation module, and rejects page/WAL mutation in
  the AM scheduling adapter.
- The DESTDIR-staged native matrix passes `80/80`, including exact WAL/root
  publication, active/pending rotation, transaction boundaries, CRUD,
  VACUUM, compaction, COW folds, restart, and score parity.
- The staged same-index concurrency gate exits successfully, including the
  shared-retirement sequence guard, slot reuse, repeat VACUUM, and normal/
  oracle query parity.
- The same staged artifact and current P2 model pass the mutable lifecycle
  matrix `71/71`, including BM25 realtime, SAE eventual completion, MVCC,
  semantic quarantine/completion, restart, crash restart, REINDEX, and bounded
  backend state.

Slice 5k is closed. `ii42_am.c` is now 43,004 lines and the cohesive mutation
module is 1,264 lines. A transient untracked sync-conflict copy of the product
inventory appeared during the final gate and was rejected because it contained
retired reloptions; it disappeared before intervention and had no Git, build,
test, or repository reference, so no source deletion was performed. Shared
work-hint/preload ownership remains a separate authority problem and was not
hidden behind a callback table or duplicated state.

#### ARCH-4 Slice 5l: COW Publication Helper Ownership

Commit `04a8938b` removes three AM-private helper implementations that would
otherwise have been copied into the mutation module. Exact object-reference and
active-L0-frontier value comparisons now have one codec-layer owner in
`ii42_segments.c/.h`; the unchanged segment-maintenance codec error projection
now has one owner in `ii42_am_maintenance.c/.h`.

- Product inventory requires one implementation and declaration for each moved
  helper and rejects the three retired AM-private names anywhere under `src/`.
  This makes future accidental reintroduction or duplicate authority a build
  gate rather than a review convention.
- A fresh CMake build and core CTest pass `1/1`. The Homebrew PostgreSQL 18
  PGXS build is warning-clean under `-Werror`; product inventory,
  `py_compile`, private-symbol inspection, and `git diff --check` pass.
- No root, lock, WAL, page, score, memory, or API behavior changed. The old
  helper bodies were removed in the same commit, and no newly unused function,
  source file, test hook, SQL entry point, or package route was found by the
  correlated dead-code audit.

Slice 5l is closed. `ii42_am.c` is now 42,950 lines. The next accepted slice
may move the COW page/root publication into mutation authority while retaining
shared-preload retirement and automatic prewarm scheduling in a narrow AM
adapter. The full Gate D matrix remains deferred until that behavioral closure
and the remaining accepted module set settle.

#### ARCH-4 Slice 5m: COW Root Publication Authority

Commit `104fd387` moves the complete checked COW root-publication transaction
into `ii42_am_mutation.c/.h`. The mutation authority now owns append-lock
serialization, latest-root revalidation, manifest validation, pending-seal or
manifest replacement, metapage Generic WAL publication, root-LSN durability,
and the reconstructible FSM handoff. `ii42_am.c` retains only a narrow adapter
that retires obsolete shared-preload state and schedules automatic prewarm
after successful publication.

- The first warning-clean build rejected an incomplete direct-header choice
  because `ii42_segment_cow_result` is owned by `ii42_segment_pages.h`. The
  mutation interface now includes that canonical owner rather than adding a
  duplicate declaration or a convenience type.
- Product inventory requires one mutation implementation and declaration, one
  AM adapter call, the complete lock/root/WAL/FSM closure in mutation, and the
  shared-preload retirement/prewarm side effects in the adapter. It rejects
  worker, SPI, scheduler, work-hint, or preload authority in mutation and
  rejects page, WAL, FSM, or append-lock authority in the adapter.
- The Homebrew PostgreSQL 18 PGXS build is warning-clean under `-Werror`. The
  DESTDIR-staged native matrix passes `80/80`; same-index writer concurrency
  passes linked-L0 prefix, reader-fence, shared-retirement, and score-parity
  checks; the 140,000-row VACUUM-frontier matrix passes `10/10`; and the current
  P2 model mutable lifecycle passes `71/71`.
- Correlated source/header, call-graph, build, package, test, and symbol-table
  inspection found no obsolete candidate that was safe to delete. The AM
  adapter has eight active lifecycle callers, and the two retained symbols are
  private implementation symbols rather than abandoned public ABI.

Slice 5m is closed. `ii42_am.c` is now 42,809 lines and the cohesive mutation
module is 1,429 lines. Shared work-hint/preload ownership remains active product
authority and is not dead code. The next slice must map that authority as a
whole or extract an independent VACUUM closure without introducing a reverse
dependency on `ii42_am.c`.

#### ARCH-4 Slice 5n: Identity Manifest Codec Ownership

Commit `44187a57` removes the AM-private identity-manifest implementation.
`ii42_segment_manifest_build_identity` now has one typed core-codec owner in
`ii42_segments.c/.h`, and all four maintenance, reclamation, and VACUUM callers
handle its status explicitly through the existing maintenance error projection.

- The first proposed owner was the lock/pin maintenance module. Existing
  inventory correctly rejected that choice because the module may not acquire
  segment-codec authority. No rejected source change was committed; the final
  implementation follows the one-way AM-to-codec dependency directly and adds
  no module, callback, or duplicated type.
- Product inventory requires one codec implementation and declaration, exactly
  four AM callers, and zero occurrence of the retired
  `ii42_am_segment_identity_build_manifest` name under `src/`. The warning-clean
  dylib contains one private implementation symbol and no stale AM symbol.
- A fresh CMake core build and CTest pass `1/1`; the Homebrew PostgreSQL 18 PGXS
  build passes under `-Werror`. The same DESTDIR artifact passes the 140,000-row
  VACUUM-frontier matrix `10/10` and the native maintenance, CRUD, fold,
  reclamation, restart, and score-parity matrix `80/80`.
- The correlated API, source/header, call-graph, build, package, test, and
  symbol audit found no unrelated obsolete route safe to remove. Active
  work-hint, reader-fence, fault-injection, and shared-preload code remains
  reachable and is not treated as legacy because of its location in the AM.

Slice 5n is closed. `ii42_am.c` is now 42,770 lines. The VACUUM closure has one
fewer AM-private dependency, but reader-fence/reuse, work-hint scheduling, test
fault injection, and successful-publication preload side effects still cross
its boundary. Those authorities must be mapped or extracted independently
before the VACUUM entry points can move without reverse dependencies.

#### ARCH-4 Slice 5o: Unsigned Saturating-Addition Ownership

Commit `b096df42` removes two byte-identical private helpers from
`ii42_am.c` and `ii42_am_meta.c`. `ii42_u32_saturating_add` now has one typed
core owner in `ii42_core.c/.h`; the two VACUUM accounting callers and four
mutation-debt/metapage callers retain the same saturation behavior.

- The deletion was accepted only after correlated source/header, caller,
  installed SQL/API, build, package, test, and symbol inspection showed that
  the helpers had no independent lifecycle or public contract. Product
  inventory requires one core implementation and declaration, exactly two AM
  callers, exactly four metapage callers, and rejects both retired names.
- Focused core tests cover zero, ordinary addition, the exact `UINT32_MAX`
  boundary, and overflow saturation. The CMake build and CTest pass `1/1`; the
  Homebrew PostgreSQL 18 PGXS build is warning-clean under `-Werror`.
- The DESTDIR-staged native matrix passes `80/80`. Product inventory,
  private-symbol inspection, and `git diff --check` pass; the dylib contains
  only the private core implementation symbol.
- No score, root, lock, WAL, page, memory, API, or scheduler behavior changed.
  No unrelated obsolete source or route met the deletion evidence threshold.

Slice 5o is closed. `ii42_am.c` is now 42,759 lines and
`ii42_am_meta.c` is 954 lines. The remaining reader-fence, shared-preload,
work-hint, and fault-injection routes are active lifecycle authorities and must
not be removed merely to reduce AM size.

#### ARCH-4 Slice 5p: Unsigned 64-Bit Saturating Arithmetic

Commit `a770b253` gives unsigned 64-bit saturating addition and multiplication
one typed core owner. Five private arithmetic implementations are deleted from
build estimation, posting-heat accounting, and page-native telemetry. The
root object-byte API remains in the metapage module because its name and caller
surface express accounting authority, but its arithmetic delegates to core.

- Product inventory requires one core implementation and declaration for each
  primitive, freezes caller counts in AM, build, and metapage owners, and
  rejects every retired private helper name under `src/`. The rebuild-publisher
  and root-accounting closure checks now use their real authority boundaries
  rather than deleted helper positions.
- Focused core tests cover ordinary add/multiply, zero multiplication, add
  overflow, and multiply overflow. CMake/CTest pass `1/1`; the Homebrew
  PostgreSQL 18 PGXS build is warning-clean under `-Werror`.
- The DESTDIR-staged native matrix passes `80/80` at
  `/tmp/ii42-arch4-u64-native.json`. Product inventory, private-symbol
  inspection, and `git diff --check` pass; only the two private core symbols
  remain in the dylib.
- Correlated source and generated call-graph inspection confirms that direct
  scan or VACUUM extraction is not yet a one-way move. Scan reaches the
  exact-root/HOT_FOLD registry and work-hint/heat state; VACUUM reaches reader
  fencing, reuse, work hints, fault injection, and successful-publication
  preload retirement. Those active routes must first receive a cohesive
  shared-runtime owner or remain in the AM; they are not dead code.

Slice 5p is closed. `ii42_am.c` is now 42,723 lines,
`ii42_am_build.c` is 638 lines, and `ii42_am_meta.c` is 950 lines. The next
structural slice must define one narrow shared-runtime/preload boundary before
moving scan or VACUUM orchestration; a callback table, public control struct,
or temporary reverse dependency is not acceptable.

#### ARCH-4 Slice 5q: Rebuild Saturation Cleanup

Commit `6d5d91f6` removes the last independent saturating multiplication
implementation from rebuild memory admission. All six semantic-streaming and
BM25 builder estimates now call `ii42_u64_saturating_mul`; their multipliers,
inputs, saturation boundary, and admission decisions are unchanged.

- Correlated source, caller, API, build, package, test, and symbol inspection
  showed that `ii42_am_rebuild_estimate` had no contract or behavior distinct
  from the core primitive. Product inventory freezes the six build callers and
  rejects reintroduction of the retired helper.
- CMake/CTest pass `1/1`; the Homebrew PostgreSQL 18 PGXS build is warning-clean
  under `-Werror`. The DESTDIR-staged native lifecycle matrix passes `80/80` at
  `/tmp/ii42-arch4-rebuild-sat-native.json`.
- Product inventory, private-symbol inspection, and `git diff --check` pass.
  Only the core u64 add/multiply implementation symbols remain; no obsolete
  build route was deleted because every surrounding builder remains active.

Slice 5q is closed. `ii42_am_build.c` is now 624 lines. The next extraction
remains gated on a narrow shared-runtime/preload authority boundary; direct
scan or VACUUM movement would still create reverse dependencies.

#### ARCH-4 Slice 5r: Shared-State Authority Split Contract

The current `ii42_am_shared_preload_control` is not one cohesive authority. It
combines an evictable exact-root/HOT_FOLD arena with worker limits, work hints,
catalog reconciliation, semantic telemetry, and posting heat. This accidental
layout coupling blocks one-way scan, VACUUM, preload, and maintenance module
dependencies. Slice 5r will separate the private shared-memory layout before
moving any of those behaviors.

The implementation contract is:

- `ii42_am_shared_preload_control` retains only cache identity, arena usage,
  entry/hash capacity, relation-entry eviction counters, access clock, entry
  metadata, hash slots, and arena bytes.
- A private `ii42_am_scheduler_control` owns background-worker accounting,
  maintenance timing and fairness, work hints, reconciliation state, semantic
  telemetry, and posting heat. It is reconstructible scheduling state, not an
  index root or model-session owner.
- Both controls remain postmaster-owned, are initialized by the existing shmem
  startup hook under `AddinShmemInitLock`, and continue to use the same named
  LWLock. The slice may change private offsets and ABI version only; it must not
  change lock order, atomicity, GUC behavior, worker admission, cache eviction,
  status text, SQL/API shape, root bytes, scores, or lifecycle policy.
- AM callers receive separate cache-availability and scheduler-availability
  predicates. No public control struct, generic service table, callback bridge,
  raw shared-state accessor, or reverse dependency is permitted.
- After the split, cache/HOT_FOLD code can move to `ii42_am_preload.c`, while
  maintenance and semantic modules can absorb their own typed scheduler
  operations. Scan and VACUUM remain in AM until those APIs are narrow.

This is a private-state Gate B change, not mechanical relocation. Acceptance
requires warning-clean PG18 and no-ONNX builds, product inventory, exact status
shape, staged native `80/80`, worker fairness and work-hint/reconcile coverage,
shared-preload restart/standby coverage, semantic telemetry coverage, symbol
inspection, and `git diff --check`. Any changed score, status field, worker
decision, cache residency, lock order, or restart behavior rejects the slice.

Commit `38377453` implements and closes Slice 5r. The exact-root/HOT_FOLD
registry now owns only cache identity, allocation, lookup, and eviction state;
the independently versioned scheduler control owns worker admission, work
hints, reconcile fairness, semantic telemetry, and posting heat. Both controls
remain postmaster-owned and use the existing shared-preload LWLock, so lock
order and generation-status snapshot atomicity are unchanged. The private
shared-preload ABI advances from 27 to 28 and therefore retains the existing
postmaster-restart deployment boundary.

- Correlated source, caller, SQL/API, build, package, test, and symbol review
  proved `last_preload_warning_time` had no reader, writer, public contract, or
  test observation. It was removed with the private ABI change; inventory now
  rejects its return and prevents cache/scheduler fields from crossing the new
  authority boundary.
- CMake/CTest pass `1/1`; normal ONNX-enabled and no-ONNX Homebrew PostgreSQL
  18 PGXS builds are warning-clean under `-Werror`. Product inventory,
  `py_compile`, private-symbol inspection, and `git diff --check` pass.
- The DESTDIR-staged native lifecycle matrix passes `80/80` at
  `/tmp/ii42-arch4-shared-split-native80.json`. Shared-preload lifecycle and
  standby/auto-preload gates pass at
  `/tmp/ii42-arch4-shared-split-preload-lifecycle.stdout` and
  `/tmp/ii42-arch4-shared-split-standby.json`.
- The formal semantic-maintenance fairness run passes at
  `/tmp/ii42-arch4-shared-split-semantic-fairness-default.json`: all four
  indexes progress with one worker, the four-worker scenario observes three
  concurrent maintenance workers, sustained ingress makes structural
  progress, work-hint overflow remains zero, restart reconciliation converges,
  and normal/oracle parity is true for every SAE index.

Slice 5r is closed. The next structural slice may extract the cohesive
exact-root/HOT_FOLD cache authority, or move typed scheduler operations into
their maintenance owners. Scan and VACUUM remain in the AM until those APIs are
narrow; no callback table or raw shared-state accessor is permitted.

Commit `f8bfa965` closes Slice 5s, the dependency cleanup required before the
exact-root/HOT_FOLD extraction. The byte-identical
`ii42_am_generation_identity_matches()` implementation now belongs to the
metapage authority in `ii42_am_meta.c`; the two active cache callers retain the
same typed function and no alternate identity route exists. This is a Gate A
relocation: root bytes, comparison fields, error behavior, lock order, memory
ownership, status shape, and public ABI are unchanged.

- Product inventory enforces one definition in the meta authority, one header
  declaration, and no implementation in the AM entry module.
- CMake/CTest passes `1/1`; the ONNX-enabled Homebrew PostgreSQL 18 PGXS build
  is warning-clean under `-Werror`. `py_compile`, `git diff --check`, and
  private-symbol inspection pass. The dylib contains one local implementation
  symbol and no public export.
- The current DESTDIR artifact passes the checked metapage/root boundary at
  `/tmp/ii42-arch4-generation-identity-metapage.json`: valid roots retain the
  same hits before and after REINDEX, while empty relations, malformed headers,
  retired fields, invalid roots, and out-of-bounds roots remain fail-closed.

Slice 5s is closed. The next accepted Gate B slice must define a typed preload
API before moving state: shared-memory sizing/startup, exact-root and HOT_FOLD
residency, unified-warm transitions, and status snapshots must leave no direct
AM access to preload entries or control storage.

Slice 5t freezes that preload boundary before implementation:

- `ii42_am_preload.c` exclusively owns the registry/hash/control layout, arena
  allocator, exact-root identity lookup, lease refcounts, eviction, retirement,
  hugepage advice, preload GUC storage, and its private shared-memory pointer.
- Callers attach immutable bytes through a typed lease and publish through a
  typed reservation. A lease may expose only its validated payload pointer and
  length; a reservation may expose only its writable payload pointer and must
  be explicitly committed or aborted. Neither type exposes an entry pointer,
  arena offset, registry slot, control pointer, or lock.
- Unified-warm and HOT_FOLD are the only accepted generation kinds. Their
  mapped-size invariants remain `1` and at least one HOT_FOLD header,
  respectively. Payload codec validation stays with the HOT_FOLD owner; the
  preload module owns residency and immutable lease lifetime.
- Status and admission diagnostics cross the boundary through one value
  snapshot captured under the preload lock. Cache clear and obsolete-root
  retirement are typed operations. The AM must not iterate registry entries.
- The existing postmaster shmem hook remains the sole startup coordinator. It
  supplies the existing named LWLock to both preload and scheduler owners; the
  modules do not reference each other's control storage.
- Backend-local publication cleanup retains one reservation handle registered
  with the existing process-exit path. Error cleanup aborts before releasing
  the lease, preserving the current no-partial-publication guarantee.

This is a Gate B ownership move. Acceptance requires warning-clean ONNX and
no-ONNX PG18 builds, product inventory and symbol checks, staged native
lifecycle, shared-preload restart/eviction and standby/auto-preload gates,
status-shape equality, HOT_FOLD exactness, semantic fairness, and
`git diff --check`. The slice is rejected if AM still names preload control,
entry, hash-slot, arena-offset, or preload-lock storage after the move.

#### ARCH-4 Slice 5t Closure Evidence

Slice 5t is closed by implementation commit `d63389ca`. The preload module now
owns registry, hash, arena, GUC, admission, eviction, retirement, lease,
reservation, and status-snapshot state. The AM retains HOT_FOLD codec and
publication policy, but accesses residency only through typed preload APIs;
semantic runtime checks only typed preload availability. Product inventory
rejects a second layout owner, direct private-handle access, or old publisher
state.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds are warning-clean under `-Werror`;
  CMake/CTest passes `1/1`. `py_compile`, product inventory, private-symbol and
  staged-library dependency inspection, and `git diff --check` pass.
- The staged native lifecycle passes `80/80` at
  `/tmp/ii42-arch4-preload-native80-final-rerun.json`, including restart
  HOT_FOLD republication, exact query behavior, CRUD, transaction visibility,
  reclamation, and bounded query context.
- Shared preload admission, exact-root residency, eviction, and registry
  lifecycle pass at `/tmp/ii42-arch4-preload-lifecycle-final.log`.
- Primary/standby generation replacement and automatic exact-root prewarm pass
  at `/tmp/ii42-arch4-preload-standby-final.json`; every recovery-side durable
  maintenance route remains a no-op.
- Eventual semantic fairness passes at
  `/tmp/ii42-arch4-preload-semantic-fairness-final.json`: one- and four-worker
  sustained-ingress scenarios converge, four SAE indexes retain oracle parity,
  restart reconciliation converges, and the realtime BM25 route remains live.
- Preload diagnostics are one internally atomic value snapshot under the
  preload lock. Scheduler telemetry remains a separate diagnostic snapshot;
  no public contract requires cross-owner point-in-time atomicity.

This closure does not claim that HOT_FOLD codec or publication policy moved out
of the AM. It closes only preload residency authority and preserves private
shared-memory ABI version 28 because layout and behavior did not change.

#### ARCH-4 Slice 5u: HOT_FOLD Codec And View Authority Contract

Move the complete immutable HOT_FOLD representation boundary into
`ii42_am_hot_fold.c/.h`: checksum, ordered term/entry validation, lease-backed
attach/release, binary term lookup, replacement-block construction, and typed
publication through preload reservations. The module owns wire-layout
interpretation and may depend one-way on preload, segment snapshots, term-fold
codec, and PostgreSQL memory APIs.

The AM retains policy only: query-heat observation decides whether a resident
term is useful, impact specialization decides when and which term to publish,
and query execution decides when the exact hot path is eligible. Callers may
inspect immutable term/entry values through the typed view, but may not access
its preload lease, payload base, mapped size, checksum, or reservation state.
Publication still releases the prior lease before retiring the exact root,
commits only after a fully validated replacement is copied, and aborts on every
error path. No alternate cache, score path, root identity, or background
lifecycle is introduced.

This is a Gate B relocation. Acceptance requires sole codec/view ownership in
the new module, no direct preload-handle access in AM, warning-clean ONNX and
no-ONNX PG18 builds, product inventory and private-symbol checks, staged native
lifecycle `80/80` including cache clear, L0 invalidation, retirement, restart,
and HOT_FOLD republication, shared-preload lifecycle, and `git diff --check`.
Any wire byte, checksum, ordering, score, exactness, lock, lease, publication,
root-epoch, or status change rejects the slice.

#### ARCH-4 Slice 5u Closure Evidence

Slice 5u is closed by implementation commit `30f1235f`. The new HOT_FOLD
module exclusively owns checksum, header/full validation, ordered term/entry
codec, immutable lease-backed view, binary lookup, replacement-block build,
and reservation-backed publication. The AM retains query eligibility, heat
observation, impact-specialization policy, and visible-hit execution; it cannot
access view or preload-handle internals. No second cache or lifecycle exists.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds pass under `-Werror`;
  CMake/CTest passes `1/1`. Product inventory, modified-script `py_compile`,
  private-symbol inspection, line-length audit, and `git diff --check` pass.
- The staged artifact passes native lifecycle `80/80` at
  `/tmp/ii42-arch4-hot-fold-native80-rerun.json`. Cache clear reload, epoch
  binding, immediate L0 invalidation, post-convergence rebuild, retirement-aware
  exactness, restart preservation, and shared HOT_FOLD republication all pass.
- Shared preload admission, exact-root residency, eviction, and reclamation
  pass at `/tmp/ii42-arch4-hot-fold-preload-lifecycle.log`.
- The first native attempt stopped in the unrelated page-reuse setup after a
  background worker held the maintenance lock through the bounded retry window.
  It did not reach HOT_FOLD execution. An unchanged isolated rerun completed
  `80/80`; no lock contention or HOT_FOLD failure repeated.

The wire constants and serialized structs remain in the internal header so
preload can enforce the minimum mapped-size invariant. Codec interpretation and
all mutable view state have one implementation owner. `ii42_am.c` is 40,360
lines after this slice; file size remains evidence rather than the objective.

#### ARCH-4 Slice 5v: Scheduler Shared-State Authority Contract

Move the scheduler's complete reconstructible shared-state boundary into
`ii42_am_scheduler.c/.h`: shared-memory sizing/startup, worker admission and
phase accounting, launch reservations, work hints, catalog-reconcile leases,
fairness cursors, semantic telemetry, and the shared posting-heat store. The
module owns its private control layout, capacities, access clocks, and shared
lock pointer. No caller may retain or inspect the control, an array slot, or
the lock.

The boundary remains deliberately narrower than background maintenance:

- the AM retains PostgreSQL worker registration and main loops, catalog/SPI
  discovery, per-index candidate classification, bounded action dispatch,
  local query-heat aggregation, and all query/build/VACUUM behavior;
- callers exchange typed work-hint tokens, telemetry/result values, posting
  observations/summaries, worker phases, and one database-scoped scheduler
  status snapshot; JSON/text formatting remains with the existing SQL/status
  owner;
- GUC storage and assignment remain process-local AM configuration, while the
  assignment hook updates the scheduler through a typed worker-limit setter;
- shared-memory startup continues under the existing postmaster hook and
  `AddinShmemInitLock`. The scheduler receives the existing named LWLock once
  and neither owns nor calls preload or runtime-service storage;
- missing shared preload retains the current best-effort fallback behavior.
  Work hints and telemetry may be absent, while foreground BM25 remains live
  and SAE continues to fail closed at its existing runtime boundary.

This is a Gate B ownership move. Acceptance requires no scheduler layout,
slot, access-clock, or shared-lock access in `ii42_am.c`; warning-clean PG18
ONNX/no-ONNX builds; product inventory and private-symbol checks; exact status
shape; native lifecycle `80/80`; shared-preload restart/standby; semantic
fairness, work-hint overflow/reconciliation, and posting-heat/HOT_FOLD gates;
and `git diff --check`. Any worker decision, fairness order, status field,
telemetry value, heat candidate, lock order, score, root, or public ABI change
rejects the slice.

#### ARCH-4 Slice 5v Closure Evidence

Slice 5v is closed by implementation commit `33467ba1`. The scheduler module
now exclusively owns its shared-memory layout and lock, worker accounting,
launch reservations, work hints, database reconcile leases and fairness,
semantic telemetry, and posting-heat state. The AM retains worker loops,
catalog discovery, maintenance policy/action dispatch, local heat batching,
status formatting, and every query/build/VACUUM authority. Product inventory
rejects raw scheduler layout, slot-array, or shared-lock access outside the
owner.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds pass under `-Werror`;
  CMake/CTest passes `1/1`. Product inventory, modified-script `py_compile`,
  private-symbol inspection, staged dependency inspection, line-length audit,
  and `git diff --check` pass.
- The staged artifact passes native lifecycle `80/80` at
  `/tmp/ii42-arch4-scheduler-native80.json`. This includes work-hint and
  scheduler-priority coverage, posting-heat batching, HOT_FOLD restart,
  cache-clear/reload, CRUD, MVCC, reclamation, and exact root/score behavior.
- Shared-preload admission, exact-root residency, eviction, registry reuse,
  and lifecycle closure pass with the staged artifact. Primary/standby root
  replacement and automatic preload pass at
  `/tmp/ii42-arch4-scheduler-standby.json`; recovery maintenance remains a
  no-op while preload reconciliation converges.
- Eventual semantic fairness passes at
  `/tmp/ii42-arch4-scheduler-semantic-fairness.json`: one- and four-worker
  sustained-ingress scenarios converge, four SAE indexes retain oracle parity,
  restart reconciliation completes, and realtime BM25 remains live.
- The database-scoped status snapshot preserves the existing field set and
  exact-entry semantics. The shared-memory wire layout and private scheduler
  ABI version remain unchanged; only ownership moved. `ii42_am.c` is 39,150
  lines after this slice.

#### ARCH-4 Slice 5w: Reader-Fence And Page-Reuse Authority Contract

Move the complete retired-page reuse preparation boundary into
`ii42_am_reclamation.c/.h`: conditional relation reader fencing, latest-root
revalidation, physical high-watermark validation, bounded retired-range arena
construction, and transfer of an acquired fence to the caller. The module
owns no durable state; it prepares optional reuse evidence for an already
authorized COW writer.

The boundary is intentionally narrow:

- callers must already hold transaction-scoped maintenance authority when
  requesting opportunistic reuse; the explicit reader-fence form remains
  available to VACUUM/COW paths that require a fence even without reusable
  ranges;
- a successful return transfers `AccessExclusiveLock` ownership to the
  caller, which must retain it through reused-page writes and root publication
  and release it with `UnlockRelation` on every normal and error path;
- the module re-reads the current metapage under the relation fence and accepts
  only the same root id, next-segment id, manifest ref, non-decreasing published
  block watermark, and a physical relation at least that large;
- failure to acquire the conditional fence or a root mismatch returns no
  arena and changes only latency/reuse opportunity, never correctness;
- the existing superuser-only fault-injection pause moves with this authority
  through one private test-support helper. No test GUC, wait behavior, public
  SQL hook, lock order, or timeout changes;
- mutation, root publication, preload retirement, scheduler hints, VACUUM
  discovery, segment codecs, and page writes remain with their current owners.

This is a Gate A ownership relocation. Acceptance requires one implementation
of the generic fault-injection parser and one implementation of the reader
fence/reuse closure; no AM-private duplicate; warning-clean PG18 ONNX and
no-ONNX builds; product inventory and private-symbol checks; CMake/CTest;
staged native lifecycle `80/80`; deterministic reader-fence/reuse, concurrent
VACUUM, restart/reclamation, and storage-plateau coverage; and
`git diff --check`. Any root byte, page-reuse range, lock lifetime, WAL record,
score, public ABI, or fault-injection behavior change rejects the slice.

#### ARCH-4 Slice 5w Closure Evidence

Slice 5w is closed by implementation commit `ec92988f`. The private
`ii42_am_reclamation.c/.h` module now exclusively owns conditional relation
fencing, latest-root/high-watermark validation, bounded retired-range arena
preparation, and the explicit lock-transfer result. Generic superuser-only
fault-injection parsing and waits have one private owner in
`ii42_am_test_support.c/.h`. Callers still own every reused-page write, root
publication, and normal/error-path fence release; no lock lifetime, retry,
publication, or reuse policy changed.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds pass under `-Werror`;
  CMake/CTest passes `1/1`. Product inventory, modified-script `py_compile`,
  private-symbol inspection, staged dependency inspection, line-length audit,
  and `git diff --check` pass.
- The staged artifact passes native lifecycle `80/80` at
  `/tmp/ii42-arch4-reclamation-native80.json`. Fixed-live churn keeps the
  document-slot high watermark at `7` and resets the reusable cursor after
  convergence.
- Same-index writer/reclamation concurrency passes at
  `/tmp/ii42-arch4-reclamation-concurrency.json`. Linked-L0 logical-prefix,
  shared-retirement sequencing, and retired-page reader-fence probes pass;
  the deterministic query waits for the relation fence, and the independent
  second writer completes in `0.722 ms`.
- The 140,000-row bounded VACUUM frontier passes `10/10` at
  `/tmp/ii42-arch4-reclamation-vacuum-frontier.json`, including the
  65,536-record COW batch boundary, fault resume, repeat VACUUM, restart, and
  post-frontier queryability. `ii42_am.c` is 38,868 lines after this slice.

#### ARCH-4 Slice 5x: Runtime-Service Shared-Memory Authority Contract

Move runtime-service shared-memory ownership into the existing semantic
runtime boundary: control-pointer and lock storage, exact size calculation,
`ShmemInitStruct` attachment, first-creation initialization, and magic/version
validation. The AM keeps only postmaster hook composition, named tranche
request/allocation, and the ordered startup calls for runtime service,
scheduler, and preload.

The boundary is intentionally behavior-preserving:

- `ii42_runtime_service_control`, its queue/response/worker layout, magic,
  version, capacities, configured worker count, atomics, provider defaults,
  and public status shape remain byte-for-byte unchanged;
- startup receives the already selected runtime-service tranche lock while
  `AddinShmemInitLock` is held. It neither requests tranches nor initializes
  scheduler/preload state;
- the existing lazy backend attach remains available and validates the same
  named shared-memory object. Missing shared preload and ABI mismatch retain
  their current fail-closed errors;
- AM still registers maintenance/runtime workers and owns `_PG_init`; no GUC,
  worker count, restart interval, database connection, or startup order moves;
- no new shared-memory copy, process-local fallback, callback framework, or
  public C/SQL surface is introduced.

This is a Gate A ownership relocation. Acceptance requires no runtime-service
control/lock definition or direct layout access in `ii42_am.c`; one private
owner for size/startup; warning-clean PG18 ONNX and no-ONNX builds;
CMake/CTest; product inventory and private-symbol checks; staged native
lifecycle `80/80`; runtime-service required/restart/privilege coverage; shared
preload restart/standby coverage; and `git diff --check`. Any shared ABI,
worker readiness, queue admission, runtime result, error contract, startup
order, or BM25 behavior change rejects the slice.

#### ARCH-4 Slice 5x Closure Evidence

Slice 5x is closed by implementation commit `bff6d09e`. The semantic runtime
module now exclusively owns the runtime-service control pointer, tranche-lock
pointer, exact shared-memory size, named shared-memory attachment,
first-creation initialization, and magic/version validation. The AM retains
only postmaster hook composition, tranche selection, `AddinShmemInitLock`
ordering, worker registration, and the ordered calls into runtime, scheduler,
and preload startup. The runtime queue layout and public ABI are unchanged.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds pass under `-Werror`; the staged
  dylib links the expected ONNX Runtime dependency. CMake/CTest passes `1/1`.
  Product inventory, modified-script `py_compile`, private-symbol inspection,
  staged dependency inspection, and `git diff --check` pass.
- The first native lifecycle attempt reached the pre-existing fixed-live-set
  `xid_horizon` retry limit and was not accepted as a pass. An unchanged rerun
  of the same staged artifact passes `80/80` at
  `/tmp/ii42-arch4-runtime-native80-rerun.json`, including CRUD, transaction,
  crash/restart, reader-fenced reuse, VACUUM, compaction, fold, and score
  parity.
- Runtime-service required, privilege, and worker-restart smokes pass with the
  current P2 runtime model. The restart probe preserves result rows while the
  worker PID changes from `29819` to `30017`.
- Shared-preload lifecycle closure passes with exact-root residency and bounded
  registry state. Physical standby replay and exact-root auto preload pass at
  `/tmp/ii42-arch4-runtime-standby.json`, including replacement-generation
  replay and recovery-mode maintenance no-ops. `ii42_am.c` is 38,828 lines
  after this slice.

#### ARCH-4 Slice 5y: Worker-Process Scheduler Lifecycle Contract

Complete scheduler ownership of worker accounting by moving its remaining
process-local state and exit cleanup into `ii42_am_scheduler.c/.h`: current
worker phase, active-worker registration, inherited launch-reservation state,
one-time exit-hook registration, and phase-specific PostgreSQL activity
reporting. These values coordinate one worker process with the scheduler's
already-private shared counters; they are not AM query or maintenance policy.

The boundary remains deliberately narrow:

- `ii42_maintenance_worker_main` and the supervisor loop remain in the AM, as
  do connection initialization, transactions, snapshots, SPI/catalog scans,
  per-index slots, candidate ordering, action dispatch, retry, and sleep;
- the worker process explicitly adopts the launch reservation created by its
  parent before connection initialization, so an early connection failure
  still releases the reservation through `before_shmem_exit`;
- active-worker and preload/maintenance phase counters retain their current
  transition order. Normal completion and process exit remain idempotent and
  cannot decrement a counter twice;
- `pgstat_report_appname` and `pgstat_report_activity` retain the exact
  `ii42 background`, `ii42 preload`, and `ii42 maintenance` names and states;
- the scheduler shared-memory layout, ABI version, worker limit, launch timeout,
  fairness cursor, worker registration, and public status shape do not change;
- no callback table, process-local fallback, second counter owner, or new
  public C/SQL surface is introduced.

This is a Gate A ownership relocation. Acceptance requires no worker-phase,
active-registration, inherited-launch, or exit-registration state in
`ii42_am.c`; one scheduler owner for the process lifecycle; warning-clean PG18
ONNX and no-ONNX builds; product inventory and private-symbol checks;
CMake/CTest; staged native lifecycle `80/80`; shared-preload maintenance and
lifecycle coverage; eventual semantic fairness; runtime-service restart; and
`git diff --check`. Any worker count, phase count, launch reservation,
activity state, action order, root, score, public status, or ABI change rejects
the slice.

#### ARCH-4 Slice 5y Closure Evidence

Slice 5y is closed by implementation commit `3578a27d`. The scheduler module
now owns the worker process's current phase, active-registration guard,
inherited launch-reservation guard, exit-hook registration, and PostgreSQL
activity reporting. The AM retains the worker and supervisor control flow,
transactions, snapshots, SPI scans, per-index slots, action dispatch, and
retry/sleep policy. Inventory checks pin the ownership and exact call counts so
the removed process authority cannot silently return to the AM.

- PostgreSQL 18 ONNX and no-ONNX PGXS builds pass under `-Werror`; the staged
  dylib links ONNX Runtime 1.28.0. CMake/CTest passes `1/1`, product inventory,
  modified-script `py_compile`, private-symbol inspection, and
  `git diff --check` pass.
- The DESTDIR-staged native mutable lifecycle passes all current `71/71` gates
  on its first run at `/tmp/ii42-arch4-worker-native80.json`; no
  `xid_horizon` retry was needed.
- Shared-preload lifecycle closure passes with exact-root residency and all
  active, phase, and pending-launch counters returning to zero. Runtime-service
  restart preserves result rows while replacing worker PID `56552` with
  `56729`.
- Eventual semantic-maintenance fairness passes both single-worker and
  four-worker sustained-ingress scenarios, restart reconciliation, native
  oracle parity, and realtime-BM25 coexistence at
  `/tmp/ii42-arch4-worker-fairness.json`. No scheduler failure or stranded
  launch reservation is reported. `ii42_am.c` is 38,643 lines after this
  slice.

#### ARCH-4 Stop Gate: Remaining Orchestration Closure

The accepted Gate A/B extraction set stops after Slice 5y. A current-source
function-dependency audit finds that the remaining build, foreground mutation,
VACUUM, semantic completion, maintenance action, page-native query, and SQL
search implementations share one large orchestration closure. In particular,
moving the VACUUM callbacks would require exporting publication, linked-L0,
document-COW, scheduler, and test-control helpers back from the AM; moving a
worker main loop would require the scheduler module to call AM-owned preload
and maintenance orchestrators. Moving query entrypoints independently would
similarly expose cache, parser, visibility, candidate, and scorer internals.

Those dependency reversals would violate the one-way module graph and the
explicit stop condition against callback frameworks, broad private APIs, or a
second lifecycle. Small disconnected comparator or prefix helpers do not own
an authority and moving them would reduce line count without improving review
isolation. Therefore no further change may be classified as a mechanical
ARCH-4 slice.

This is not a claim that the ideal forwarding-only AM shape has been reached.
`CSG-I114` remains open for a future, separately reviewed Gate B/C redesign of
one complete orchestration boundary. The current beta close-out instead freezes
the already extracted single-owner modules and proceeds to one Gate D matrix.
Any later split must begin with a typed, one-way interface contract and may not
reuse the Gate A relocation exception.

For every extraction:

- [x] Move ownership, tests, and documentation together; do not copy logic.
- [x] Keep module state private and expose the smallest typed interface.
- [x] Document lock prerequisites, snapshot lifetime, memory-context owner,
  error cleanup, and whether an input root must be revalidated.
- [x] Reject circular includes and cross-module access to private globals.
- [x] Prefer PostgreSQL conventions and explicit C ownership over new generic
  abstractions, callback frameworks, or opaque indirection without a lifecycle
  benefit.
- [x] Delete the old implementation immediately after the moved route passes;
  no dual product path is permitted.
- [x] Delete unrelated legacy code only after call-graph, SQL/API, build,
  package, and test-reference audits all prove that it is unreachable. Keep
  active reclamation state, negative fixtures, and independently used test
  hooks even when their names contain `obsolete`, `legacy`, or `v2`.

Structural gate: `ii42_am.c` must become a reviewable entry module rather than
the implementation owner. File-size reduction is evidence, not the objective;
cohesion, one-way dependencies, and single authority are the acceptance test.

### ARCH-5: Qualification And Documentation Closure

The final clean Gate D artifact is
`/tmp/ii42-arch4-final-product-maturity-v4.json`. It binds commit
`7a2a22869bd689cceeb5b0bd3b36c3680cf6452f`, a clean source tree, package
fingerprint `5cd4631ced524fb7430be56cfb135c0168f22d6ef78430799283ca0c93df0aa1`,
and the pinned ONNX Runtime 1.26.0 release dependency. All 30 maturity steps
pass in 1,604,236 ms, including source migration, exact v3 lifecycle,
transaction/2PC, VACUUM frontier, 8x8 same-index concurrency, quarantine,
runtime-service restart, physical replication, resource soak, the full 50k
production-model lifecycle, and the 75k product-path benchmark.
The ledger commit recording this evidence is a documentation-only descendant
and does not modify or re-authorize the qualified artifact.

The benchmark artifact at
`/tmp/ii42-arch4-final-product-maturity-v4.benchmark.json` passes validation.
The semantic build encodes 75,000 documents in 597.64 seconds with average
batch size 31.997 and observed maximum 32. All 96 concurrent query clients
succeed with zero runtime failures, recovery, busy rejection, cancellation,
or liveness timeout; the source join uses TID scan rather than sequential scan.

- [x] Build with PostgreSQL 18 in normal, no-ONNX, warning-clean, and static
  analysis configurations. Run core sanitizer tests where supported.
- [x] Run focused default-SAE, BM25, transaction/savepoint/2PC, semantic
  completion/quarantine, VACUUM, linked-L0, fold, concurrent writer/reader,
  worker fairness, restart, UNLOGGED, replication, migration, DDL, and package
  gates on the staged binary.
- [x] Before every temporary-cluster staged run, verify that the PostgreSQL
  installed version SQL/control files are byte-identical to the staged package,
  or use a package-isolated script loader. `extension_control_path` alone does
  not bind version SQL and must not be accepted as staged-package evidence.
- [x] Re-run full-overlay differential and golden-oracle exactness for every
  supported query shape and lifecycle state.
- [x] Re-run backend RSS and shared-runtime ownership tests. SAE must fail
  closed without preload and must not acquire an index-sized backend cache.
- [x] Re-run 40k and 250k exact performance matrices plus write, seal,
  semantic-completion, cold-read, warm-read, p50/p95/p99, QPS, WAL-byte, index
  size, and fixed-live churn/storage-plateau gates.
- [x] Verify pure BM25 exact rows/scores and performance against the frozen
  baseline; SAE cannot tax the disabled path beyond measurement noise.
- [x] Reconcile architecture, quickstart, API, parameters, policy, operations,
  migration, testing, release-readiness, and changelog documents against the
  installed extension rather than source assumptions.
- [x] Run `py_compile`, shell syntax checks, `git diff --check`, product
  inventory, schema/ACL smoke, clean-install smoke, and source-package checks.

### Commit And Review Discipline

- ARCH-1 and each ARCH-4 extraction are independent commits.
- A commit may be contract-changing or mechanically structural, never both.
- After each commit, reopen the actual diff and run its focused gate; historical
  suite counts are not current evidence.
- If a mechanical extraction changes SQL output, scores, root bytes, lock
  order, WAL behavior, memory ownership, or performance beyond noise, revert
  that slice and diagnose before proceeding.
- Update this ledger before fixing every newly discovered issue. Do not create
  a second planning file.

### Tiered Refactor Validation

ARCH-4 validation is proportional to the authority changed. Running the full
release matrix after every source relocation wastes qualification time without
adding evidence. Conversely, describing a change as mechanical never permits
an authority change to bypass its focused lifecycle gate.

1. **Gate A: mechanical relocation.** A slice that only moves unchanged
   declarations and function bodies runs a clean PGXS build, strict compiler
   warnings, dependency and exported-symbol inspection, `git diff --check`,
   product inventory, and the smallest smoke that calls the relocated entry
   points. Review must show unchanged control flow, constants, error text,
   lock/WAL calls, and memory-context ownership. It does not run the full
   lifecycle or performance matrix.
2. **Gate B: interface or private-state boundary.** A slice that introduces a
   typed header, changes static linkage, or transfers module-owned state also
   runs the focused CRUD, restart, scan, maintenance, preload, or VACUUM gate
   for that authority.
3. **Gate C: completed authority module.** Once all mechanical slices for one
   module are assembled, run its medium cross-boundary matrix, including the
   neighboring callback and failure path. Commit the module checkpoint before
   starting another authority.
4. **Gate D: final architecture and release.** After all planned modules are
   complete, run the full staged lifecycle, transaction/2PC, VACUUM,
   concurrency, restart, replication, RSS, storage plateau, exactness,
   BM25/SAE performance, package, and PostgreSQL-version matrices once against
   the same authorized clean artifact.

Any Gate A diff that changes behavior is reclassified before testing. It must
be split into a contract commit plus a Gate B or Gate C implementation commit;
it cannot remain a mechanical relocation.

### Completion Criteria

The goal is complete only when:

1. CSG-I110 through CSG-I114 are resolved with current reproducible evidence.
2. The documented SAE default is usable and eventual-only; BM25 behavior is
   unchanged when SAE is disabled.
3. Installed product code has one v3 query/mutation/maintenance path and no
   experimental-v2 runtime fallback.
4. Module ownership matches the table above without cyclic authority, duplicate
   lifecycle logic, broad internal globals, or index-sized backend state.
5. Exact result, lifecycle, recovery, replication, memory, storage plateau,
   and performance gates pass on the staged PostgreSQL extension.
6. The public documentation describes the installed behavior and the open
   issue table is empty.

### Stop And Escalation Conditions

- Stop before implementation if eventual-only SAE is not accepted as the beta
  contract; ARCH-1 determines the rest of the work.
- Escalate any reloption whose intended v3 behavior cannot be inferred from
  current code and measurement rather than preserving it speculatively.
- Stop a module extraction if it requires a new root, scorer, scheduler,
  lifecycle, global mutable singleton, or cross-module lock inversion.
- Do not accept a refactor that passes unit tests but regresses exact native
  query results, BM25 baseline, backend RSS, WAL/recovery, or storage plateau.

## Phase Ledger

### CSG-0: Freeze Baseline And Design

- [x] Tag the clean pre-redesign `sae` commit.
  - Evidence: `ii42-pre-convergent-segments-20260730` points to
    `9cdeb73149d878e8982f06b164c46a8e85904229`.
- [x] Record the target architecture, invariants, literature, worker policy,
  migration boundary, and performance contract.
- [x] Capture the current exact core performance matrix and command lines on
  the qualification host.
- [x] Record current static index bytes, warm RSS, QPS, median, p95, and p99.
  - The final 40k and 250k paired matrices retain exact top-100 ids and scores.
    Folded/static and impact/materialized mean, p50, p95, p99, and QPS gates
    all pass. The backend-memory gate records 5k/80k physical footprint,
    first/repeat latency, RSS, malloc, and II-42 memory-context ownership.

### CSG-1: Fragmented-Posting Representation Oracle

- [x] Implement a test-only virtual-contiguous extent reader over the current
  contiguous posting payload.
- [x] Split every term deterministically into 1, 2, 4, and 8 extents.
- [x] Prove score and top-k parity with the current scorer.
- [x] Benchmark extent fanout using the same score workspace and one top-k.
- [x] Reject per-segment top-k or result-list merge implementations.

Acceptance: exact parity at every split and measured latency within the dynamic
query contract. The 250,000-document local oracle reported `1.0062x` at four
extents and `1.0013x` at eight extents with exact checksums. The final 40k and
250k observed-work matrices close the qualification gate.

### CSG-2: Exact Statistics-Neutral BM25

- [x] Factor the core lexical payload into neutral TF/document-length facts and
  an explicit global
  statistics.
- [x] Implement the exact dynamic scorer and bounded lookup-table fast path.
- [x] Compare dynamic exact scores and ranking with current materialized
  impacts across corpus-statistics changes.
- [x] Measure the neutral-path cost and identify the specialization needed to
  recover the current hot loop.
- [x] Keep semantic posting weights independent of lexical statistics.

Acceptance: exact ranking parity after insert/delete statistics changes without
rewriting unchanged postings. The core representation gate passes: base
postings plus an added-document extent reproduce the expanded index exactly.
At 250,000 documents, neutral contiguous is `1.0966x` the materialized-impact
path and neutral eight-extent scoring is `1.1051x`. Persistent neutral segment
serialization moves to CSG-3; the epoch-specialized `1.00x` path moves to
CSG-7.

### CSG-3: Native Segment And Manifest Format

- [x] Implement a standalone little-endian manifest/descriptor codec with a
  whole-manifest checksum and bounded segment count.
- [x] Reject corrupt/truncated manifests, overlapping sequence/page ranges,
  duplicate segment ids, stale impact epochs, and invalid active-L0 state.
- [x] Define the core storage version, manifest, segment descriptor, sequence
  watermark, checksums, canonical payload, and bounded term runs.
  - The existing contiguous lexical CSC can now be converted losslessly into
    a statistics-neutral canonical segment, including immutable document
    versions. Core serialization, directory attachment, and mixed-scorer
    parity pass. Native fresh builds now select v3 by default; existing legacy
    v2 indexes retain their format until explicit `REINDEX` migration.
- [x] Bind manifest, directory, and segment payloads to native page ownership.
  - The immutable page-chain envelope now validates object kind, object and
    owner-manifest identity, exact page ordinal/count/used bytes, total bytes,
    object checksum, and header checksum. PostgreSQL relation append/read I/O
    now writes complete WAL-protected contiguous chains under the relation
    extension lock. Root publication uses the WAL-protected metapage switch.
  - Manifest v4 now carries complete immutable object references for the term
    directory and both fold surfaces. A block range without exact object size,
    checksum, identity, and owner is no longer publishable.
  - A published-closure validator now proves that the manifest object, every
    derived surface, and every segment payload are owned by the same manifest,
    lie below one block high-water mark, and do not overlap.
  - Manifest v5 requires one immutable query-contract object. Its checked
    codec preserves BM25 parameters, empty-token identity, and the optional
    text vocabulary without retaining a duplicate legacy posting matrix.
  - A sealed-snapshot loader now reads and validates the published manifest
    closure, every payload chain, optional fold chains, query contract, and
    term directory. It reconstructs only scorer/query metadata and one
    directory-backed read view and attaches both linked-L0 frontiers.
  - A serialized bundle writer now appends query contract, canonical payloads,
    derived directory, and manifest in dependency order, validates the full
    closure below one high-water mark, and returns an unpublished read root.
  - The fresh-build path writes that sealed bundle under the AM
    publication lock and atomically switches one clean v3 metapage root.
    Runtime status/signature/byte accounting understand the new authority
    without loading posting payloads. Linked-L0 CRUD and default publication
    use this authority.
- [x] Define stable document-version identity and retirement semantics.
  - The core manifest separates live `visible_document_count` from the
    root-relative `document_slot_count`. One immutable tuple-version owner plus
    a separate transaction-owned retirement is persisted and applied by the
    native MVCC path. A live incarnation is never renumbered within its root.
  - Sealed-snapshot attachment now requires exactly one immutable version
    owner for every occupied document slot and reconstructs the PostgreSQL TID
    map from those records. Retirement visibility uses PostgreSQL's safe
    horizon. A fully drained retired or aborted slot may be reincarnated only
    in a descendant root under a newer owning sequence.
  - Aborted slot reservations now have one strict immutable hole state. The
    codec requires frozen identity with zero TID, length, fingerprint, and
    postings; TID lookup and scoring ignore it, and zero-residency holes become
    reusable capacity rather than permanent workspace.
  - Immutable frozen retirement records now rebuild one globally sorted,
    duplicate-free bounded retirement set without reading posting runs.
    Attachment invalidates retired TIDs and proves live count/length against
    the manifest; linked L0 retirement replay merges into the same set.
- [x] Define and validate the global term directory and bounded extent
  descriptors, including a hard eight-extents-per-term limit.
  - Appending one immutable segment now derives the next complete directory
    from the validated prior directory plus only the new payload's canonical
    runs. Exact-prefix validation prevents accidental descriptor mutation, and
    the result is byte-equivalent to a full logical rebuild without reading
    any old posting payload.
  - This flat directory remains a representation oracle. It still allocates
    and serializes all vocabulary offsets and extents on every seal; CSG-3B
    replaces that physical publication cost without changing its logical
    result.
- [x] WAL-log segment append/seal and atomic manifest publication.
  - The validated read-root contract now binds one manifest object reference,
    active and optional rotated L0 frontiers, next sequence, and published
    block high-water mark. It also owns one monotonic segment-id allocator and
    one monotonic reusable-document-slot cursor, so neither rotation nor
    foreground slot allocation scans the manifest or linked-L0 frontiers.
  - The complete root now has a fixed 200-byte little-endian codec with its
    own checksum. Metapage publication can switch one stable blob rather than
    persisting compiler-dependent C struct padding.
  - The AM metapage now reserves that fixed blob and validates v3 roots
    against the physical relation high-water mark. V3 also rejects any
    simultaneous legacy base/delta/semantic physical authority. Writer
    publication switches the checked root atomically.
  - AM payload health and backend-local cache identity now treat the checked
    root blob as the sole v3 generation identity. Sealed snapshots are freed
    before their owning memory context, while the existing v2 DSM identity is
    unchanged.
  - Fresh pure-BM25 v3 builds now persist the runtime contract digest in the
    immutable manifest, clear every legacy physical-authority field, flush the
    WAL publication LSN, and expose no partially published root.
- [x] Fail unsupported native formats with `rebuild_required`; do not add
  experimental
  compatibility translation.
  - The product migration boundary is source-table migration from
    `psql_bm25s`. The superuser-only legacy-v2 builder is retained solely as a
    differential test oracle and is not a supported storage-upgrade route.

Acceptance: restart and page-validation tests can distinguish active,
unpublished, retired, corrupt, and reclaimable pages without scanning the
relation.

### CSG-3B: Incremental Catalog And Directory Metadata

- [x] Measure current flat directory and query-contract bytes, seal CPU, and
  WAL growth against fixed changed-term batches as vocabulary grows.
  - A reproducible isolated benchmark now fixes two changed terms while growing
    the initial text vocabulary. The first cold measurements were about 9 ms at
    2,000 terms, 87 ms at 20,000 terms, and 453 ms at 100,000 terms. This proves
    the remaining seal CPU is vocabulary-shaped after sparse payload work was
    removed. After native lookup cutover and append-only mandatory seal, three
    repetitions measured 1.44/1.77/2.05 ms medians at 2k/20k/100k terms on
    the final prefix-COW build. Logical growth remained 432-460 bytes, physical
    growth 23-27 pages, and WAL growth 190,544-223,656 bytes per seal.
- [x] Replace the flat authoritative term directory with a checksummed paged
  copy-on-write map keyed by stable term id.
  - A pure core implementation now stores fixed 16-term leaves behind a
    five-level sparse radix root. Canonical little-endian objects carry
    checksums, old roots remain readable, and a three-leaf append wrote only
    three leaves plus their 15 path nodes.
  - Child refs now carry exact page-chain locator, owner, byte count, inner
    checksum, and page-envelope checksum. A bottom-up fake page-store gate
    proves that real child locators can be incorporated into parent checksums,
    the final physical root remains queryable, and no second object-id locator
    table is required.
  - A relation-owned writer now appends leaves and parents bottom-up into real
    WAL-protected page chains. An isolated PG18 probe resolves one term through
    the bounded physical radix path before and after restart. Pending seal and
    bounded compaction now path-copy only touched external leaves/root paths,
    preserve ancestor-owned children, and publish manifest v8 descendants
    without materializing the complete directory. Recursive closure and
    reader-fenced reclamation cover ancestor-owned descendants.
- [x] Store stable segment/object identity in directory extents so appends and
  bounded compaction do not renumber untouched references.
  - The core map records stable segment ids and materializes the current flat
    segment-index directory only as a differential oracle.
- [x] Move exact raw per-term document frequency from the manifest-wide array
  into the same COW term leaf. Keep only visible document count, total visible
  field length, and statistics epoch as manifest scalar statistics.
  - Raw DF is present in each core leaf. Native manifest v8 seal and
    compaction publish no manifest-wide DF array, while the differential
    accelerator reproduces the exact current frequencies.
- [x] Split the scorer/model contract from an append-only lexical term catalog;
  adding terms must update bounded catalog pages rather than rewrite all term
  strings.
  - The scorer/model contract is now a fixed 96-byte object with no vocabulary
    payload. Immutable catalog blobs own contiguous term-id ranges, and each
    COW term record carries its complete checked catalog ref.
  - Initial publication writes one all-term catalog. A pending seal writes only
    the unseen suffix catalog before its changed COW paths and manifest.
    Numeric ID indexes keep zero catalog refs.
  - Native PG18 evidence grew from 28 to 2,039 terms while retaining the exact
    initial query-contract page, object id, owner, size, and checksum. The
    2,011-term suffix occupied one 43,156-byte catalog rather than copying the
    original strings. Selective compaction retained all four catalog ranges.
- [x] Update only leaves and root paths touched by a seal, compaction, or fold,
  and compare every result with the current flat-directory oracle.
  - The first append differential test matches
    `ii42_term_directory_append_payload()` byte-for-byte and proves prior-root
    isolation, canonical codec round-trip, and corruption rejection.
  - Native pending seal and bounded replacement use the external path-copy
    writers. The flat directory is materialized only as a read accelerator or
    differential oracle, not as publication authority.
- [x] Persist one reconstructible COW lexical byte-to-id lookup for text
  indexes; numeric-id indexes must not carry it.
  - The packed 64-way core and restart-safe external patch contract are
    complete. Manifest v10 now owns the explicit root and immutable hash seed,
    validates their owner/kind/range/published closure, and rejects partial
    metadata instead of falling back to a full vocabulary scan.
  - Native relation-page writing, recursive reachability, fresh build, suffix
    path-copy, unchanged-root reuse, compaction, and term-fold inheritance now
    pass one 45-gate lifecycle. Pending seal now resolves only unique changed
    terms through this root and never materializes the complete vocabulary.
- [x] Keep mandatory pending seal independent of global reclamation inventory.
  - A controlled ablation identified exact reachability inventory as the
    remaining vocabulary-shaped cost after lookup cutover. Seal now publishes
    append-only; reader-fenced interior reuse remains available to compaction
    and fold. Native lifecycle passed 45/45 and real-model eventual SAE passed
    11/11.
- [x] Make routine interior-page reuse proportional to transition evidence.
  - Manifest v11 carries at most 64 canonical, non-overlapping retired block
    ranges. COW publication carries unused prior hints, retires the replaced
    manifest and selected segment payloads, and authenticates the resulting
    set with the manifest checksum.
  - Optional compaction and fold consume only these hints after the existing
    nonblocking old-reader fence and root/high-water revalidation. The new
    manifest itself remains append-only, so a range cannot be consumed after
    its retirement list has already been serialized.
  - Hints are advisory transition evidence, not a second storage authority or
    a complete free list. Overflow drops the smallest ranges; status reports
    both hint range and block counts. Full recursive reachability remains the
    explicit diagnostic/scrub oracle, never ordinary worker input.
  - With fixed 1,664-byte input, three cold compaction medians are now
    1.27/1.27/1.56 ms at 2k/20k/100k vocabulary, versus
    5.74/63.83/285.88 ms before the change. Every run returned the same five
    rows and reused four blocks. Native lifecycle passed 46/46 and real-model
    SAE lifecycle passed 11/11.
- [x] Keep lookup warm-path performance within the frozen baseline; permit a
  flat image only as a reconstructible accelerator, never publication
  authority.
- [x] Cover restart, checksum failure, COW cancellation, orphan reclamation,
  new-term races, and physical replication.
  - Core corruption/cancellation gates, 80-gate native lifecycle, concurrent
    new-term publication, immediate-stop recovery, reader-fenced reclamation,
    and primary/standby replay all pass on the final staged build.

Acceptance: ordinary publication reads/writes metadata proportional to changed
terms and bounded COW paths, never `O(vocabulary + extents)` or an
`O(vocabulary)` DF array, while exact query rows and the
one-directory/one-scorer contract remain unchanged.

### CSG-4: Lexical-Immediate Write Path

- [x] Replace durable "temporary delta" authority with an active first-class L0
  segment.
  - The read root is now specified as one metapage snapshot containing the
    sealed-manifest reference plus the active linked-L0 frontier. The
    immutable manifest is not rewritten for every foreground mutation.
  - The core linked-page and frontier codecs now enforce one segment identity,
    monotonic per-page sequence bounds, exact payload checksums, a hard
    1,024-page bound, and valid empty/absent states.
  - Portable checksummed logical mutation records now carry exact
    sequence/document-slot/XID/TID identity and either sorted numeric atoms or
    normalized UTF-8 lexical terms. Independent fragment frames permit one
    logical row to span physical pages without imposing a row-size limit.
    The read root atomically reserves the next sequence and either claims one
    checked reusable slot or advances the document-slot high watermark.
  - Fresh pure-BM25 v3 indexes now append text and numeric UPSERT records to
    the active linked L0. A record that fits the tail publishes the changed
    tail and read root in one generic WAL record. A non-fitting or oversized
    record is written first as one complete unreachable chain and is then
    linked to the old tail together with the new root in one generic WAL
    record. This keeps foreground work row-bounded and never republishes the
    immutable manifest.
  - The page reader follows pending then active chains, rejects cycles and
    frontier drift, validates every page and fragment, reassembles oversized
    records, and proves sequence/record/byte closure.
  - Insert-only L0 records now attach to the same global vocabulary, corpus
    statistics, score workspace, and top-k as sealed segments. New UTF-8 terms
    receive stable cache-local global IDs and numeric terms retain their IDs.
    Current-transaction records are visible only to their owning transaction;
    unresolved, aborted, rolled-back savepoint, and prepared records obey
    PostgreSQL transaction visibility. A snapshot containing unresolved
    transactions is deliberately not reusable from the backend cache.
  - UPDATE/DELETE retirement is query-exact after VACUUM, including
    idempotent repeat VACUUM. Safe pending sealing under an old snapshot,
    HOT-chain/CTID reuse, and default publication are proved.
  - Active-to-pending rotation is now an O(1), WAL-protected metapage switch.
    The old active chain becomes immutable pending, a monotonic id names the
    fresh active chain, and foreground writes continue while queries attach
    both frontiers into one read view. No manifest or payload is rewritten.
  - Manifest v6 segment descriptors now persist the immutable payload owner.
    A descendant manifest can bind and validate an ancestor-owned payload by
    exact object identity, range, byte count, and checksum. This removes the
    ownership dependency that otherwise forced every seal to copy all old
    payloads.
  - The fixed scorer/model contract retains its exact ancestor-owned object
    reference across every compatible copy-on-write publication. Vocabulary
    compatibility is proven separately by complete contiguous catalog coverage
    in the COW term records, so even a seal with new terms does not rewrite the
    contract or prior strings.
  - Pending seal publication now has one checked root transition. It consumes
    the exact monotonic manifest identity, requires a nondecreasing physical
    high-water mark, clears only pending, and preserves the concurrent active
    frontier byte-for-byte. Exact object closure permits immutable and active
    pages to interleave without blocking foreground appends.
  - The COW append writer validates the ancestor manifest/directory and exact
    descriptor prefix, appends only the pending payload, derives one
    replacement directory from old metadata plus that payload, reuses an
    unchanged ancestor query contract, and writes one descendant manifest.
    It intentionally clears optional folds so correctness never depends on a
    stale acceleration surface.
  - The worker now classifies every pending record only behind PostgreSQL's
    safe non-removable XID horizon, preserves aborted slot reservations as
    strict frozen holes, and compiles committed UPSERT/RETIRE records into one
    immutable segment without reopening ancestor posting payloads. A short
    final compare-and-publish lock requires the exact ancestor and pending
    identities while preserving concurrent active writes.
- [x] Append INSERT/UPDATE/DELETE effects in work proportional to the changed
  row.
  - INSERT and the replacement side of UPDATE append one lexical UPSERT
    immediately. Heap visibility hides DELETE and superseded tuple versions
    immediately; VACUUM records each globally dead tuple version as a compact
    transaction-owned RETIRE record without rewriting the sealed manifest or
    postings.
  - Convergent VACUUM now walks only the authoritative document COW leaves and
    the bounded active/pending L0 records. It excludes already frozen or
    unsealed retirements before appending new RETIRE records and never opens
    sealed posting payloads to enumerate TIDs.
- [x] Preserve current-transaction, savepoint, abort, and 2PC visibility.
- [x] Preserve HOT-chain, CTID-reuse, old-snapshot, and concurrent VACUUM
  semantics.
- [x] Seal L0 by bounded byte/record/age thresholds without reading old
  segments.
- [x] Publish the new manifest atomically.

Acceptance: CRUD is immediately lexical-searchable and foreground work is
independent of corpus size.

### CSG-5: Unified Virtual-Contiguous Query

- [x] Support validated zero-copy segment-local document ids through a base
  offset or immutable local-to-global map.
- [x] Resolve each query term once through a validated global-directory read
  view.
- [x] Scan lexical-neutral and semantic/direct-impact extents into one score
  workspace and one exact top-k.
- [x] Route the native AM scan through the manifest-backed read view.
  - The immutable sealed portion can now be loaded as one checked snapshot.
    It includes scorer metadata and the tuple TID map, but is intentionally not
    exposed as a complete mutable query view until active and pending linked-L0
    frontiers are attached.
  - Ordered and field-aware full scoring now dispatch through one wrapper to
    the canonical mixed read view. One posting cursor now drives legacy CSC or
    segmented extents for filtered, boolean, and phrase candidate generation.
    The isolated PG18 smoke proves sealed and insert-only L0 text/token/ID,
    public search, and native ordered rank parity through restart, including a
    2,000-term record spanning physical pages. V3 still uses a complete mixed
    ranking rather than a specialized sparse hot path, so default builders
    remain disabled until retirement, rotation, and the performance gate pass.
- [x] Apply document-version retirement without a corpus-sized query bitmap.
  - A bounded sorted retirement set is intersected with only the queried
    lexical posting extents to derive exact live DF. Live `N` and total
    document length are replayed from the same transaction-visible records;
    retired TIDs fail visibility and retired scores are cleared. Fold removes
    the temporary intersection cost.
- [x] Add exact manifest/segment/cache identity checks.
- [x] Remove the normal runtime dependency on compiled full-delta cache views.

Acceptance: fragmented native storage passes the full-overlay/full-rebuild
oracle and the bounded fanout performance matrix.

### CSG-6: Tiered Compaction

- [x] Implement geometric size classes and similar-size merge selection.
  - The core now assigns overflow-safe geometric classes from canonical
    payload bytes. The PostgreSQL writer persists them and the worker selects
    four adjacent same-class segments after first handling extent pressure.
- [x] Merge only selected segments and safe retirements.
  - A pure bounded transform now merges one contiguous descriptor range.
    It preserves tuple-version records, aborted holes, frozen retirements,
    posting kinds, and exact four-byte values while leaving every unselected
    payload unopened.
- [x] Bound segments per tier, write amplification, and free-page pressure.
  - Native tier and extent-pressure tests now enforce the eight-segment and
    64-MiB input limits. Reader-fenced reachability inventory reclaims the
    exact trailing suffix and safely reuses eligible interior pages.
- [x] Publish replacement descriptors atomically.
  - The logical directory replacement is complete: selected extents are
    removed, one replacement run set is inserted, and unchanged prefix/suffix
    descriptors are reused with remapped segment indices.
  - The page-level COW writer now loads only a selected payload range, reuses
    all other immutable payloads and the query contract, and writes one merged
    payload, replacement directory, and descendant manifest. Its generic root
    transition preserves active and pending L0 frontiers byte-for-byte.
  - The AM builds outside the append lock, then compares manifest/allocator
    identity and publishes under the short append lock while preserving
    concurrent active writes.
- [x] Reclaim replaced payloads only after a reader-safe fence.
  - Derive one exact current-root physical inventory from the manifest,
    recursive term/document COW references, term-local fold/catalog
    references, and linked active/pending L0 page chains. The disposable flat
    term accelerator is never reclamation authority.
  - Classify physical debt into two distinct classes. Pages at or above the
    checked root high-water mark are unpublished trailing garbage; holes below
    it are retired/interleaved interior debt. Never infer either class from
    object ids or generation ancestry alone.
  - A worker may truncate only trailing garbage after acquiring a nonblocking
    `AccessExclusiveLock`, re-reading the metapage/root under that fence, and
    proving the relation is still longer than the checked high-water mark.
    Foreground readers are never made to wait for cleanup.
    This path is implemented as `segment_tail_cleanup`; an active native
    reader returns `reader_fence_busy`, while the next idle pass truncates the
    exact unpublished suffix.
  - Report exact reachable and interior-unreachable blocks below the
    high-water mark. A later COW action may build an in-memory best-fit
    allocator from that exact complement only after the same nonblocking
    reader fence and current-root revalidation. Reused pages receive Generic
    WAL full-page images; a busy fence falls back to append-only allocation.
    Public status now reports physical, reachable, interior-unreachable,
    trailing-unpublished, unique-range, and observed-reference counts.
  - Test duplicated retained references, recursive ancestor-owned COW paths,
    fold/catalog reachability, non-contiguous L0 chains, trailing crash
    garbage, interleaved interior orphans, concurrent old readers, immediate
    crash recovery, restart, and unchanged rows/scores. The native gate proves
    zero reuse while an old reader holds the index and positive reuse after it
    releases the relation lock.
- [x] Prove sustained writes converge without merging the largest segment for
  every small update.
  - Thirty-two forced one-record rotations stayed within four immutable
    segments and converged to two. Ten geometric tier merges each read no more
    than eight segments or 64 MiB, while exact v2 rows/scores and restart state
    remained unchanged.
- [x] Collate each newly sealed payload term-major and persist checked
  block/extent seek and upper-bound metadata without reading older payloads.
- [x] Prove neutral lexical bounds are conservative across statistics changes
  and specialized/semantic bounds are exact for their declared contract.
  - A core representation oracle now derives bounds over stable global
    document-slot blocks. It passes all five BM25 methods, positive and
    negative query weights, lexical-neutral, lexical-impact, semantic-impact,
    and mapped-run ordering checks against exhaustive per-posting scores.
    Payload/fold codecs persist the checked records and native qualification
    covers both BM25 and SAE.
- [x] Combine folded-prefix and tail block bounds in one global pruning
  schedule and one top-k. Prove that the dynamic path never creates a
  per-segment heap, underestimates a mixed lexical/semantic score, or drops a
  candidate that the exhaustive mixed scorer would retain.

Acceptance: bytes read/written are proportional to selected segments; crash,
cancel, and retry are idempotent.

### CSG-7: Term-Local Structural And Workload Fold Surfaces

- [x] Implement bounded, decayed heavy-hitter heat accounting.
  - Foreground queries must aggregate posting-key observations in a bounded
    backend-local table and flush them only in batches. One query must not take
    the shared generation-cache lock once per term.
  - The shared table is volatile, fixed-capacity, set-associative, and
    lazily decayed. It may lose or evict heat without changing correctness.
    It stores only posting-key ids, root identity, reusable structural-cost
    observations, and aggregate scorer work; never query strings or results.
  - A worker may consume a candidate only after re-reading the current
    manifest and COW term record. Stale root identity, insufficient stable
    heat, no surface reduction, or an unfavorable rewrite-benefit ratio must
    decline the optional fold without blocking mandatory maintenance.
  - The native implementation retains the first 64 unique keys per backend,
    deliberately drops excess wide-query observations, and flushes once per
    32 successful segmented queries. One 4,096-entry, four-way shared table
    uses integer half-life decay and root-local heat. Status exposes flush,
    observation, eviction, candidate, fanout, and query-associated pruning
    counters. An isolated 70-key query gate proved zero shared flushes through
    query 31 and exactly one bounded flush at query 32.
- [x] Build persistent term-scoped folded prefixes with exact per-term coverage
  watermarks.
  - The v1 fold-bundle codec now preserves lexical TF and signed semantic
    impacts separately, records one complete-boundary coverage per term, and
    rejects mixed coverage, invalid run ordering, and corrupt bytes.
  - Native relation publication, recursive COW reachability, restart-safe read
    attachment, and exact fold-plus-tail scoring now pass. A replacement fold
    removes its covered extents from the selected term record while preserving
    raw DF; unrelated source-segment terms remain reachable.
  - Single-term workload admission now uses root-stable heat, exact current
    surface reduction, and bounded input bytes to publish through the existing
    COW fold path. Publication validates and prewarms the selected immutable
    object before switching the root.
- [x] Permit coverage only at complete immutable extent boundaries. Reject,
  split, or fold-forward any compaction replacement that would straddle an
  inherited term-local coverage watermark.
  - Core preflight resolves only leaves touched by the selected payloads and
    classifies each folded term as covered, tail, or straddling. The worker
    tries the original bounded range first, then searches only its already
    loaded subranges while retaining the original extent-pressure objective.
    It evaluates at most eight candidate subranges and stops at the smallest
    legal segment count. A range with no legal subrange defers explicitly
    before merge or page writes; corruption still fails closed.
- [x] Store complete page-chain fold references in COW term records and include
  every referenced bundle in recursive publication closure and reclamation
  reachability; do not add a second object-id locator.
  - COW leaf codec v5 now stores complete immutable object references for both
    fold surfaces. Recursive closure, relation publication, and reclamation
    traversal and exact reader-fenced reclamation now pass native restart
    qualification.
- [x] Admit the same term-local fold as mandatory structural work when hard
  extent pressure cannot be resolved by one bounded whole-segment compaction.
  This path must not depend on heat and must not rewrite unrelated terms.
  - The native worker now first attempts its existing bounded whole-segment
    compaction and then selects one pressured COW term. It advances that term's
    prior neutral fold over the smallest complete segment prefix needed to
    leave at most five source extents. One action is bounded to eight segments
    and 64 MiB, changes only the selected term leaf/root paths, and leaves
    source segments reachable for unrelated terms.
- [x] Prove the active -> sealed -> neutral-folded -> impact-specialized read
  state machine preserves exact rows, scores, ties, and TIDs at every atomic
  root transition.
  - One native sequence published a hot epoch-bound impact image, invalidated
    it immediately with a linked-L0 INSERT, retained exact neutral-plus-L0
    v2/v3 rows and scores, sealed two descendants, folded and promoted the
    changed prefix, then rebuilt the impact image at the new statistics epoch.
    A VACUUM retirement then invalidated the image, converged to a new
    retirement-aware image, and remained exact after cache clear and
    immediate-stop crash recovery.
- [x] Inherit eligible neutral folds across descendant manifest publication;
  ordinary writes must become post-coverage tail events, not global
  invalidations.
- [x] Rank fold work by measured benefit per rewritten byte and enforce
  per-index/database budgets, including measured pruning slack and avoidable
  candidate work from loose fragment bounds.
  - Admission combines root-stable heat, observed candidate/block work,
    surface reduction, exact input bytes, and geometric carry. One worker
    action is capped at one key, eight segments, and 64 MiB; fair scheduling
    provides the database-level bound without a second budget authority.
- [x] Derive heat from queries but persist only reusable term/atom folds.
  Query-result rows and query strings must not become fold authority. Use
  decay, hysteresis, and a stability estimate so a churning workload does not
  repeatedly rewrite a large posting key.
- [x] Keep independently admitted posting keys in separate immutable fold
  objects.
  - Multi-key physical bundles were rejected for the beta design: current
    one-key COW publication already passes the peak matrix, while bundling
    couples unrelated invalidation, reclamation, and rewrite cost without a
    measured benefit.
- [x] Measure physical page/object transitions and cache-local colocation
  separately from logical extent count. Extent count remains the hard safety
  bound; optional fold admission must be justified by observed read cost.
- [x] Advance a selected fold from its prior coverage rather than rewriting
  unrelated terms.
  - The core fold-forward transform accepts one prior checked bundle plus only
    the selected term's post-coverage extents and touched payload views. Its
    output matches a direct complete-prefix fold byte-for-byte without opening
    unrelated historical payloads. Native worker selection and restart-safe
    publication pass for mandatory structural pressure and optional heat-based
    admission. Physical multi-term bundling is deliberately not adopted.
- [x] Replace single-prefix attachment and COW identity with a stable major
  fold, one independently referenced minor fold, and a bounded raw tail per
  posting key.
  - COW term-map v6 persists disjoint major/minor coverage and complete object
    references. Cold attachment scores major, minor, and raw tail through the
    same global read view, excludes every covered source run exactly once, and
    includes both fold objects in reachability/reclamation. Core tests publish
    two descendant generations, serialize and reload both levels, and match
    the unfurled score oracle.
- [x] Replace single-prefix *worker advancement* with a stable major fold, one
  geometrically advanced minor fold, and a bounded raw tail per posting key.
  - Native maintenance now plans from effective major/minor coverage, loads
    only the old minor plus selected tail for a carry, and merges major plus
    minor only for promotion. COW publication preserves both independent
    intervals until one checked root switch.
- [x] Prove geometric carry and promotion: minor replacement must normally add
  at least comparable new bytes, while major promotion requires a size ratio,
  mandatory surface repair, or quiet-period workload ROI. Record bytes read
  and written per newly indexed posting under sustained updates.
  - The native gate defers a one-posting tail against a 240-byte minor, admits
    carry only after 328 new posting bytes accumulate, grows that minor to
    568 bytes, and promotes only when it is comparable to the 592-byte major.
    Promotion consumes zero extents and produces one 936-byte major.
- [x] After a quiet interval, promote and prewarm the measured hot working set
  to one major surface per key. Preserve exact rows, scores, ties, and top-k in
  fragmented, major/minor/tail, and promoted states.
  - The one-key native policy promotes a root-stable hot key when ratio or
    workload ROI passes. The writer reads and checks the new immutable fold
    before root publication, warming the exact pages in PostgreSQL shared
    buffers. The 38-gate lifecycle smoke covers every intermediate shape and
    restart.
- [x] Implement optional per-hot-term statistics-epoch impact specialization.
  - Query telemetry distinguishes an exact neutral fold that is eligible for
    specialization from raw extent fragmentation. The shared scheduler admits
    the hot key without inventing a second index or correctness authority.
    One COW leaf/root update publishes a checked impact bundle only after
    lexical tails and both linked-L0 frontiers are empty.
- [x] Select the best exact surface without gaps or duplicate scoring.
  - Cold attach builds one global read plan from disjoint major, minor, and
    post-minor raw-tail intervals. Core and native tests match the unfolded
    score oracle exactly.
- [x] Persist fold descriptors across restart and use them as a bounded shared
  cache preload list; volatile heat state is not correctness state.
- [x] Keep persistent fold state and volatile page warming separate. Build and
  validate a replacement hot fold under its old root, prefetch only within
  residual memory/I/O budgets, then publish one COW term-root update. Cache
  eviction must affect latency only.
- [x] Test the fold validity matrix for lexical UPSERT, RETIRE, semantic
  completion/quarantine, statistics-epoch change, segment compaction, restart,
  cache eviction, and contract change.
- [x] Enforce SILK-style residual-capacity admission and object-boundary
  preemption under query, semantic-freshness, L0, fanout, and free-space
  pressure.
  - Mandatory lexical/fanout work and semantic freshness run ahead of optional
    fold. Optional work executes one bounded root-revalidated object action and
    yields between actions; correctness never depends on retained heat or
    cache residency.
- [x] Keep full neutral fold as an explicit qualification/REINDEX operation,
  never a normal worker convergence requirement.
  - Normal structural and workload actions select one touched posting key and
    bounded complete extents. Static one-segment construction remains a
    benchmark/REINDEX control, not a worker action.
- [x] Demonstrate that a quiescent skewed workload converges its hot posting
  keys to the contiguous scorer baseline while cold keys remain bounded
  fragmented, and record warm/cold median, p95, p99, CPU, I/O, and bytes
  rewritten.
  - Focused local evidence now covers the hot-key half: three independent
    40,000-document PG18 runs preserved exact rows/scores, held three-extent
    p95 to a median `1.042x` static, and returned folded p50/QPS to
    `1.003x`/`1.000x` paired static. The final 40k and 250k observed-work
    matrices additionally pass every folded/static and impact/materialized
    mean, p50, p95, p99, QPS, exactness, and byte-accounting gate.
- [x] Exercise the three-state performance contract: freshly committed
  fold-plus-tail, structurally converged bounded extents, and
  workload-converged folded/specialized hot paths. Require exact rows and
    scores in all states and baseline peak performance after quiescent hot-set
    convergence.
  - The paired local matrix covers static, fragmented, neutral-folded,
    impact-specialized, and legacy materialized-impact controls. At 40k and
    250k documents, every top-100 id and score matched exactly. Impact versus
    materialized mean/p50/p95/p99/QPS ratios were
    `0.976/0.978/0.973/0.980/1.024` and
    `0.977/0.968/1.007/0.991/1.023`; every blocking gate passed.

Acceptance: writes immediately invalidate only stale specialization; the
neutral/fragmented path remains correct. A quiescent index returns to the
baseline contiguous scorer for the warmed benchmark working set and passes the
no-regression matrix without a mandatory whole-corpus fold.

### CSG-8: Eventual Semantic Completion

- [x] Store semantic-pending identities in the same L0 segment lifecycle.
  - L0 record v3 now has checksummed `SEMANTIC_COMPLETE` and
    `SEMANTIC_QUARANTINE` forms. Completion values are sorted positive
    semantic impacts; both forms bind one prior document slot to an exact
    input fingerprint. The native lexical-first lifecycle smoke proves the
    pending identity remains query-visible through INSERT and indexed UPDATE.
- [x] Append exact completion/quarantine/retirement records as segment data.
  - Manifest v7 and payload v3 now persist exact semantic transition counts
    and fixed-width completion/quarantine state. A semantic-only segment can
    reference an older root-relative slot through a sparse map, and bounded
    compaction unions maps and retains the latest transition without
    duplicating the document-version owner.
  - The native worker now appends bounded completion/quarantine batches to
    linked L0. Query overlay consumes the exact transition, and pending seal
    publishes semantic-only or mixed immutable payloads without duplicating
    the document-version owner. Fresh v3 publication, worker completion,
    UPDATE, DELETE/VACUUM retirement, seal, and cold restart pass through the
    public native route.
  - Model inference remains outside the append lock. Immediately before a
    completion or quarantine record is appended, the writer reloads the exact
    source slot through one bounded COW radix descent under that lock and
    compares the complete owner, semantic state, retirement, and residency
    record. A reused or otherwise changed incarnation is deferred without
    publishing stale model output.
- [x] Reject stale completion after update/delete while accepting a valid HOT
  successor whose indexed input fingerprint is unchanged.
  - Model output is staged before publication, then revalidated through
    PostgreSQL's visible tuple chain and the exact input fingerprint. A changed
    indexed version receives an empty terminal transition; an unchanged HOT
    successor retains the completion without emitting a new L0 UPSERT.
- [x] Recover semantic debt after an uncommitted completion append crashes.
  - A real-model gate stops PostgreSQL immediately after the completion record
    is appended but before commit. Recovery ignores that aborted transition,
    retains the original sealed debt, and a normal retry converges to the same
    semantic score with zero pending work.
- [x] Prioritize query inference, then completion, then ordinary large physical
  maintenance. Pending-L0 hard pressure, free-page safety, and hard fanout may
  preempt completion so model throughput cannot block lexical availability.
  Prove that semantic and I/O lanes may overlap only under independent
  headroom while root publication remains serialized.
- [x] Compact semantic records without re-encoding unchanged documents.

Acceptance: lexical visibility is immediate, foreground inference is zero,
completion converges fairly, and all current eventual lifecycle gates pass.

### CSG-9: Recovery, Replication, Memory, And Concurrency

- [x] Test crash before/during/after manifest publication.
- [x] Test concurrent readers, writers, compaction, fold, VACUUM, REINDEX, and
  DROP.
- [x] Test savepoints, 2PC, old snapshots, standby replay, promotion, and
  restart.
- [x] Prove shared-memory and backend-local RSS remain bounded.
  - Convergent normal query attach must use relation pages through PostgreSQL
    shared buffers plus bounded query-local state. A complete decoded sealed
    snapshot is permitted only in maintenance, scrub, and test-oracle paths.
  - SAE keeps a strict preload ownership boundary: creation, rebuild, query
    encoding, and semantic completion require the shared runtime worker pool
    and shared mutable-tail arena. Ordinary backends may never load model
    sessions or fall through to a private unified cache. The bounded
    backend-local compatibility path remains BM25-only.
  - Run the gate with multiple fresh backends and increasing document and
    vocabulary sizes; private retained bytes must not scale with index size.
- [x] Prove one hot index cannot starve another index or query inference.

Acceptance: no ambiguous manifest, stale posting, lost work, unbounded memory,
or fairness failure.

### CSG-10: Product Closure

- [x] Run BM25 and SAE CRUD/lifecycle suites.
- [x] Run exact fragmented/full-rebuild differential tests.
- [x] Run static and dynamic core performance matrices.
- [x] Run package/install/upgrade and clean-cluster tests.
- [x] Update architecture, parameters, operations, migration, testing,
  release-readiness, API, and changelog documentation.
- [x] Isolate replaced delta/replacement-generation code from every product
  path. The superuser-only `ii42.test_legacy_v2_build` route remains solely as
  the independent full-rebuild differential oracle; it is not a supported
  storage format, migration path, or maintenance fallback.
- [ ] Clear every issue below and perform a final line-level review.

Acceptance: all completion rules at the top of this document pass.

## Open Issues

| ID | Severity | Issue | Required resolution | State |
| --- | --- | --- | --- | --- |
| CSG-I1 | P0 | Current lexical postings materialize IDF/average-length-dependent impacts, so a statistics change logically stales every unchanged posting. | Statistics-neutral lexical facts and the exact dynamic scorer now preserve rows and scores across statistics changes. Optional epoch-bound impacts are never correctness authority. | Resolved |
| CSG-I2 | P0 | Current lightweight fold publishes one replacement generation and still rewrites the complete base. | Linked L0, bounded pending seal, selective segment compaction, and term-local COW folds converge without rewriting the complete base. The 32-rotation gate proves sustained bounded convergence. | Resolved |
| CSG-I3 | P0 | A segment design can double-score old and replacement tuple versions without a stable document-version invariant. | Immutable version slots, document COW ownership, frozen retirements, aborted holes, and one mixed read view pass CRUD/MVCC/restart qualification. | Resolved |
| CSG-I4 | P0 | Workload fold or explicit full fold can create gaps/duplicates if its coverage watermark is not atomic with the manifest. | Coverage is restricted to complete immutable extent boundaries and published in the same descendant COW manifest as its fold reference. Major/minor/tail and restart gates prove exact exclusion. | Resolved |
| CSG-I5 | P1 | Fragment chains may exceed the latency bound under sustained writes. | Eight-extent hard pressure triggers bounded compaction or mandatory term-local fold; 32 rotations and non-uniform high-DF pressure converge within the declared segment/byte limits. | Resolved |
| CSG-I6 | P1 | Old snapshots may require retired segments after a new manifest is published. | Both trailing truncation and interior reuse conditionally acquire relation `AccessExclusiveLock`, drain old-root readers, and revalidate the current root and relation size. A busy fence makes maintenance yield or append rather than wait. Native old-reader, reuse, and crash-restart gates pass. | Resolved |
| CSG-I7 | P1 | Impact specialization can publish against a stale statistics epoch or remain selected after an unsealed lexical L0 changes live BM25 statistics. | Publication now revalidates the exact manifest and both linked-L0 frontiers. Selection requires complete effective major/minor coverage, matching statistics epoch, and empty active/pending L0. Any mismatch immediately uses the exact neutral fallback; native INSERT, seal, fold, promotion, cache-clear, and crash-restart gates pass. | Resolved |
| CSG-I8 | P1 | Segment/term directory metadata can grow with mutation count. | Segment count and per-term extents are hard-bounded; COW term leaves update only changed paths, while geometric compaction and folds prevent mutation-count growth. | Resolved |
| CSG-I9 | P1 | Existing cache and generation identities assume one base plus one delta extent. | Runtime and shared-cache identity now bind the checked read root, manifest, linked-L0 frontiers, storage version, and contract rather than legacy base/delta ranges. | Resolved |
| CSG-I10 | P1 | Full convergence may be exact but slower than the frozen baseline. | A strictly eligible lexical-impact read image now uses the materialized sparse hot loop over the same COW authority. Neutral, semantic, mixed, weighted, stale, and active-L0 shapes fall back to the exact global scorer. Paired 40k and 250k matrices match every id/score and meet all latency/QPS gates; the 44-gate lifecycle covers retirement, invalidation, rebuild, cache clear, and restart. | Resolved |
| CSG-I11 | P0 | The first term-directory draft did not distinguish lexical-neutral facts from semantic or epoch-specialized direct impacts, which would force split query paths. | Add checksummed extent scoring kinds and a single mixed scorer/read view. | Resolved |
| CSG-I12 | P0 | The first manifest draft equated live BM25 document count with score-workspace capacity, so a retired version would require global renumbering or reject a valid read view. | Separate `visible_document_count` from root-relative `document_slot_count`; one live incarnation keeps its slot for the life of that root, while CSG-I92 permits only fully drained descendant-root reuse. | Resolved |
| CSG-I13 | P0 | Reusing a logical-document slot across UPDATE before evacuation would make old and replacement postings accumulate. Permanent non-reuse would instead make fixed-live workspace grow with history. | UPDATE first publishes a replacement and retires the old incarnation. CSG-I92 allows later numeric reuse only after the safe horizon, zero COW residency, and no active/pending reference; `born_sequence` distinguishes incarnations. | Resolved |
| CSG-I14 | P1 | The first canonical four-byte value scorer selected TF/impact storage inside every posting iteration and exceeded the local eight-extent latency target. | Hoist value-source and document-map selection to the extent boundary, leaving branch-free posting loops. | Resolved |
| CSG-I15 | P0 | Independent valid checksums did not prove that a directory term actually referenced the matching payload run, allowing a term/kind permutation to preserve byte coverage. | At attach, match every directory extent to the next canonical run by term, kind, offset, and count. | Resolved |
| CSG-I16 | P0 | A crash or failed COW build before manifest publication leaves unreachable object pages. Concurrent active-L0 appends can interleave those objects, so they are not necessarily a truncatable tail. | Walk the complete checked root closure; truncate only the revalidated unpublished suffix; and build an in-memory best-fit allocator from the exact interior complement only after the old-reader fence. Reused pages receive Generic WAL full-page images. Failed publication leaves unreachable pages for the next derived inventory, without a persistent free-list. | Resolved |
| CSG-I17 | P0 | Listing a mutable active L0 descriptor inside the immutable manifest would require rewriting the manifest for every foreground append. | Keep sealed segments in the immutable manifest; atomically snapshot the active linked-L0 identity/frontier beside the manifest reference in the metapage read root. | Resolved |
| CSG-I18 | P0 | Removing the legacy serialized index would also remove BM25 parameters and the text vocabulary needed to compile queries. | Store a checksummed manifest-owned query contract containing only global scorer and vocabulary metadata, never duplicate postings. | Resolved |
| CSG-I19 | P0 | Rebuilding only params and corpus statistics could silently score a posting against a zero document length when its tuple-version metadata was missing. | Reconstruct slot lengths from unique immutable version records at attach time and reject every posting whose global slot has no version owner. | Resolved |
| CSG-I20 | P0 | Term-directory serialization originally required its final manifest object ref, but that ref exists only after serialization and native page append. | Separate logical directory validation used during build/serialization from full published-manifest validation used at attach. | Resolved |
| CSG-I21 | P0 | A v3 metapage intentionally clears the legacy generation signature, so accepting its checked physical root alone would not detect changed BM25/tokenization reloptions. | Persist the current generation-contract digest in the immutable manifest and validate it against runtime options on every cache acquisition. | Resolved |
| CSG-I22 | P0 | Publishing v3 by default before linked-L0 CRUD exists would let foreground writes fall back into the incompatible v2 delta format. | New builds now select v3 by default and every v3 mutation publishes through the checked linked-L0 path. Legacy v2 indexes remain readable/mutable without silently changing storage, and explicit `REINDEX` migrates them to v3. The 55-gate default-v3 lifecycle proves fresh build, CRUD, rotation, convergence, restart, v2 isolation, and migration. | Resolved |
| CSG-I23 | P0 | The first page writer conflated the payload's embedded zero-range checksum with the page envelope's complete-object checksum, so a valid canonical payload could not be published. | Keep the embedded codec checksum for payload corruption and store the complete blob checksum in the descriptor/object ref used by page closure validation. | Resolved |
| CSG-I24 | P0 | The first query-capable v3 generation was reported as invalid because generation status, signature, and logical byte accounting only interpreted v2 physical fields. | Load only the checked immutable manifest for control-plane reporting and expose v3 storage, contract, segments, L0 frontiers, and bytes through the existing status API. | Resolved |
| CSG-I25 | P0 | An empty v3 index had no term-array allocation, but the mixed scorer required a non-null term pointer before applying its existing zero-document fast return. | Require term extents only for a nonempty vocabulary and cover the empty mixed scorer plus native empty-index query path. | Resolved |
| CSG-I26 | P1 | Reporting the manifest start plus the relation high-water mark as one legacy primary range implied that fragmented immutable objects and future retired pages were contiguous live storage. | Keep legacy contiguous range fields zero for v3 and report fragmentation, the exact manifest range, published high-water mark, and logical manifest-owned bytes separately. | Resolved |
| CSG-I27 | P0 | The linked-L0 page/frontier draft had no stable mutation-record contract. Reusing the v2 raw-value delta would defer tokenization to reads, reject terms absent from the immutable vocabulary, and retain a one-page row-size limit. | Store portable checksummed logical records with numeric or normalized UTF-8 atoms, frame them independently across pages, and reserve sequence plus document slot in the read root. | Resolved |
| CSG-I28 | P0 | Requiring active-L0 page count to be no greater than logical record count rejects every valid record that spans multiple physical pages. | Bound pages and records independently; traversal must prove fragment and logical-record cardinality against the published frontier. | Resolved |
| CSG-I29 | P0 | Linking fragments as they are written can expose a partial logical record after an error or crash. | Publish a complete-fit tail append with the root in one WAL record; otherwise write a complete unreachable chain first and atomically publish only its old-tail link plus the advanced root. | Resolved |
| CSG-I30 | P0 | A corrupt frontier could advertise an unbounded logical record count before page traversal and force a large cache allocation. | Bound each frontier to 131,072 logical records, bound total attach allocation, and prove exact contiguous sequence cardinality before accepting the root. | Resolved |
| CSG-I31 | P0 | A slot-only RETIRE record can hide an old TID, but leaving its contribution in BM25 `N`, average length, and per-term DF changes every live score. | Keep live `N` and length in L0 replay; intersect the bounded sorted retirement set with each queried lexical extent to subtract dead DF, then clear retired scores. | Resolved |
| CSG-I32 | P0 | The first v3 VACUUM retirement path cold-loaded complete sealed payload objects to reconstruct the TID-to-slot map, while convergent semantic completion needed a bounded durable pending frontier. Both defects came from keeping document metadata inside posting payloads. | Manifest v9 provides the authoritative document/version COW directory and semantic maintenance consumes its bounded actionable summaries. VACUUM now visits checked document COW leaves plus bounded linked-L0 records, excludes existing retirements, and opens no sealed posting payload. Native CRUD, repeat-VACUUM, compaction, restart, and score parity pass all 44 gates. | Resolved |
| CSG-I33 | P0 | Treating a complete neutral fold as normal convergence would reintroduce whole-index maintenance and contradict the product goal. | Converge only the bounded hot working set; reserve complete fold for explicit REINDEX, migration, or qualification. | Resolved |
| CSG-I34 | P1 | A heat-only fold policy can waste I/O on large cheap-to-scan terms or churn under updates. | Admission requires root-stable heat and repays exact input bytes with observed candidate/block work and surface reduction. Geometric carry, one-key/eight-segment/64-MiB action limits, cooldown, and fair worker rounds bound rewrite work. The final observed-work matrices pass. | Resolved |
| CSG-I35 | P0 | The periodic-collation literature pauses ingestion while rewriting block order, which is not acceptable for PostgreSQL foreground writes. | Active-to-pending rotation is O(1); every seal, compaction, and term fold is built outside the append lock and publishes only after a short root revalidation while the new active L0 continues accepting writes. | Resolved |
| CSG-I36 | P0 | A second active-L0 threshold can be reached while the prior pending L0 is still unsealed. The current writer treats the rotation threshold as the hard limit and rejects the first append at that soft boundary. | Soft rotation now occurs at half the hard record/page frontier. If pending is occupied, foreground appends continue into the reserved half, emit an urgent maintenance hint, and reject only at the hard bound. Native v2/v3 parity covers one pending frontier plus three active records and complete bounded convergence. | Resolved |
| CSG-I37 | P0 | Payload ownership was inferred from the current manifest, so a descendant manifest could not reuse an unchanged ancestor payload and every seal would silently become a full payload rewrite. | Persist `payload_owner_manifest_id` in each checksummed descriptor and validate the exact ancestor-owned object reference without transferring ownership. | Resolved |
| CSG-I38 | P0 | Rebuilding the complete global term directory from all segment payload views would make every seal read all historical posting bytes even after payload ownership became copy-on-write. | Validate the unchanged descriptor prefix, copy prior directory extents, and append only the new segment's ordered runs; compare against the full logical rebuild oracle. | Resolved |
| CSG-I39 | P0 | Rollback and savepoint rollback can leave a reserved slot gap; dropping the record would make the root range unrepresentable, while immediate reuse could mix transaction identities. | Persist a strict frozen aborted-hole record with no TID, length, fingerprint, semantic state, or postings. It rejects every posting reference and becomes reusable only through the same CSG-I92 zero-residency descendant-root transition. | Resolved |
| CSG-I40 | P0 | Freezing committed/aborted L0 transaction state or folding retirements before the oldest relevant PostgreSQL snapshot can change visibility for an already-running reader. | Block sealing on unresolved/prepared transactions and retain transaction identity until PostgreSQL's safe non-removable horizon proves the immutable outcome globally observable. | Resolved |
| CSG-I41 | P0 | Sealing a pending RETIRE into an immutable payload without replaying immutable retirement metadata would make its old tuple version searchable again after the L0 frontier is cleared. | Collect only frozen payload retirements into the same sorted read-side set as L0 retirements, reject duplicates/non-frozen records, invalidate TIDs, and verify manifest live statistics without reopening posting runs. | Resolved |
| CSG-I42 | P0 | Query contracts were owned only by the current manifest, forcing every pending seal to rewrite the complete vocabulary even when no query metadata changed. | Permit exact ancestor-owned query-contract references, bind the serialized owner identity during decode, and require current vocabulary compatibility. | Resolved |
| CSG-I43 | P0 | A seal that rebuilt the read root ad hoc could clear or roll back the active frontier written after rotation, even when all new objects were valid. | Centralize the root transition: require the exact monotonic manifest identity, preserve active byte-for-byte, clear only pending, and advance one append-only high-water mark. | Resolved |
| CSG-I44 | P0 | Requiring a new manifest page to begin after the latest root high-water mark would make any concurrent active-L0 append invalidate the worker build or force foreground writes to block for the full seal. | Permit physical page interleaving; publish by unique manifest identity, exact checked object closure, preserved active frontier, and a nondecreasing relation high-water mark. | Resolved |
| CSG-I45 | P0 | A seal writer that reused ancestor ownership but still serialized every payload would preserve format correctness while retaining whole-index write amplification. | Validate an exact descriptor prefix, write only the new pending payload, derive the replacement directory from prior metadata plus its runs, and reuse the query contract unless vocabulary metadata changed. | Resolved |
| CSG-I46 | P0 | A slot-only pending RETIRE did not carry the old document length, so a seal could not update global live-length statistics without reopening an ancestor payload or trusting a stale aggregate. | Store the exact retired version length in L0 record v2, validate it during mixed replay, and feed it from VACUUM's checked document-version snapshot. | Resolved |
| CSG-I47 | P0 | Manifest validation bounded raw posting DF by live document count even though query replay subtracts retirements later, so a legal retirement of a high-DF document could make an immutable manifest fail validation. | Bound raw DF by the current root's score-slot capacity, subtract checked retired posting membership at read time, and reject any corrected live DF above visible `N`. | Resolved |
| CSG-I48 | P0 | Immutable retirement payloads dropped the exact retired document length carried by L0, so later selective compaction could not reconstruct live-length changes from the selected segments alone. | Persist length in the fixed-width retirement record, validate all reserved bytes, and bump the rebuild-only segment payload format to v2. | Resolved |
| CSG-I49 | P0 | Rotation creates an immutable pending L0, but no worker path yet classifies its XIDs at PostgreSQL's safe horizon, compiles one descendant segment, and atomically clears only that exact pending frontier. A second rotation therefore reaches bounded foreground backpressure. | Build unreachable COW objects from a pending-only snapshot, defer the complete seal on unsafe transaction state, then compare the latest root under the short append lock and publish only if its ancestor and pending identities still match. Preserve concurrent active L0 byte-for-byte. | Resolved |
| CSG-I50 | P0 | A selective merge that rebuilds the complete directory or normalizes posting values would turn physical maintenance into a semantic rewrite and reopen every historical payload. | Merge only one contiguous selected range, preserve every MVCC and posting value byte, reuse unselected descriptors, and replace only selected directory extents. | Resolved |
| CSG-I51 | P0 | Reusing the pending-seal root transition for compaction would clear an unrelated immutable pending frontier, while a full-snapshot writer would reopen every payload. | Add a range-only loader, COW replacement writer, and physical-only root transition that preserves both linked L0 frontiers exactly. | Resolved |
| CSG-I52 | P0 | Pending-seal admission checked each new run independently, so two new extent kinds for one term could pass at old count seven and fail only after unreachable page writes began. | Aggregate new runs per term before COW, trigger compaction at six extents, and keep each compaction pass bounded to eight segments and 64 MiB. | Resolved |
| CSG-I53 | P1 | One hand-built four-segment merge did not prove that repeated foreground rotations converge instead of accumulating immutable fragments or rewriting the largest segment for each update. | Drive 32 one-record rotations through the native worker, require every compaction to stay within the segment/byte budget, compare every result to a v2 oracle, and restart at the converged root. | Resolved |
| CSG-I54 | P1 | `ii42_index_details` and raw generation-cache diagnostics still read legacy metapage debt counters. A linked-L0 index can therefore report zero pending writes and only sealed documents while `generation.delta` correctly reports an active record. | For convergent storage, report linked-frontier records/bytes directly, label documents as sealed-generation scope, and count exact upsert/retire debt from bounded linked-L0 record headers without reading historical posting payloads. | Resolved |
| CSG-I55 | P0 | Geometric tier tests did not prove that six hot-term extents with unlike payload sizes can converge before the hard eight-extent limit. | Build six native segments across multiple size classes, require the selector to choose only the smallest bounded contiguous range for `extent_pressure`, and compare query/restart state to v2. | Resolved |
| CSG-I56 | P0 | Optional workload fold can consume the same worker, I/O, and cache capacity needed by semantic completion, pending seal, or hard-fanout compaction, creating freshness debt or foreground p99 spikes. | The unified scheduler orders lexical safety and hard fanout ahead of semantic completion, and semantic completion ahead of optional fold. Each round performs one bounded action; query/document inference has separate bounded workers while root publication stays serialized. The multi-index fairness gate passes. | Resolved |
| CSG-I57 | P0 | Waiting for semantic inference before sealing lexical pending L0 couples foreground write capacity to model throughput. Mutating or duplicating an old document-version owner would instead violate immutable COW ownership. A single chronological completion cursor is also insufficient because one future-dated quarantine would block unrelated work. | Eventual SAE seals lexical state immediately, persists fingerprinted semantic debt, and appends guarded completion/quarantine transitions later through the same document COW authority. CRUD, HOT/stale output, crash retry, source migration, 8x8 concurrency, restart, and replication gates pass. | Resolved |
| CSG-I58 | P0 | The current COW append avoids reading old posting payloads but still allocates and serializes the complete flat `term_offsets + extents` directory on every seal, making metadata publication `O(vocabulary + extents)`. | Add CSG-3B paged COW directory leaves keyed by stable term id; rewrite only changed leaves/root paths and retain the flat form as an oracle or derived accelerator. | Resolved |
| CSG-I59 | P0 | Extending the current query contract with one unseen lexical term rewrites all prior vocabulary strings, so new-term admission is not bounded by the changed row. | Split stable scorer/model contract from append-only immutable lexical ranges referenced by COW term records. Prove stable contract identity, exact new-term lookup, restart, and compaction preservation through the native lifecycle smoke. | Resolved |
| CSG-I60 | P0 | One manifest-wide neutral-fold object and coverage sequence cannot represent independent hot-term working-set folds or safely advance one term without rewriting unrelated terms. | COW term leaves own independent major/minor fold refs and coverage. The final beta deliberately keeps one posting key per immutable fold object: multi-key bundling adds coupled invalidation/reclamation without measured benefit, while the one-key form passes peak-performance and sustained-update gates. | Resolved |
| CSG-I61 | P0 | The publication description invalidated all optional folds after every append, contradicting the exact prefix-plus-tail model and preventing convergence under sustained low-rate writes. | Descendant COW term records retain neutral major/minor refs and coverage while later events append to the exact tail. Contract changes invalidate authority; statistics changes make only epoch-specialized impacts ineligible. | Resolved |
| CSG-I62 | P0 | The current manifest serializes one raw document-frequency value per vocabulary term, so every seal remains `O(vocabulary)` even after posting and directory publication become COW. | Move exact raw DF into the corresponding COW term leaf; keep only corpus-wide scalar BM25 facts and the statistics epoch in the manifest. | Resolved |
| CSG-I63 | P1 | Treating a persistent fold and shared-cache residency as one object would either make correctness depend on volatile memory or expose a cold-cache latency cliff after publication. | The manifest-reachable checksummed fold is authority. Publication validates and prewarms the replacement before its COW switch; exact-root preload markers recover bounded warming after restart. Cache clear/eviction changes latency only, and restart/standby gates remain exact. | Resolved |
| CSG-I64 | P0 | Persisting only logical COW object ids would require a second global id-to-page map after restart; predicting one contiguous allocation would conflict with concurrent L0 appends. | Embed complete immutable page-chain refs in parent nodes and write changed objects bottom-up. The core fake store and real PG18 relation/restart gates pass without a second locator map. Publication closure is tracked separately by CSG-I65. | Resolved |
| CSG-I65 | P0 | Manifest v7 treats the term directory as one flat object whose object id and owner both equal the current manifest. A COW root has its own object identity and may retain recursively reachable ancestor-owned children, so relaxing the old check would make closure and reclamation ambiguous. | Manifest v8/v9 provide complete recursive term/document refs. One validating visitor and exact physical-range inventory now cover recursive objects, embedded fold/catalog refs, immutable payloads, and linked L0 pages without a second locator catalog. Native duplicate-ref, interior-debt, tail-cleanup, old-reader, and restart gates pass. | Resolved |
| CSG-I66 | P0 | The in-memory COW builder assumes one dense local object-id array and numeric child-before-parent ordering. A restarted descendant must path-copy from externally stored ancestor nodes without loading the complete tree or comparing unrelated owner-local ids. | Make object identity the pair `(owner_manifest_id, object_id)`, allocate new ids only within the descendant owner, validate descent by radix level/prefix/full ref/checksum, and implement an external path-copy update that reads and rewrites only touched leaves and root paths. | Resolved |
| CSG-I67 | P0 | Publishing a term-local fold by removing or rewriting its source segment descriptors would either hide unrelated cold terms or make hot-fold publication a segment-wide rewrite. | Keep fold objects standalone. Update only selected COW term leaves, exclude covered source events by the term-local sequence watermark, and include every retained standalone fold/catalog object in the exact recursive reachability inventory. Native fold, compaction, inventory, restart, and exact-score gates pass; reader-fenced physical reuse is provided by CSG-I6/I16. | Resolved |
| CSG-I68 | P1 | Bounded extents alone do not recover dynamic-pruning efficiency when each fragment has a loose term-wide upper bound. | Canonical global-slot blocks, one mixed pruning schedule, COW length bounds, active-L0 bounds, exact fallback, and query-associated block counters feed admission. Exact parity and the 40k/250k folded/static and impact/materialized matrices pass. | Resolved |
| CSG-I69 | P1 | Treating quiescent convergence as all-query static equivalence would silently require a whole-corpus fold. | Guarantee mathematical equivalence for every key, static-performance equivalence for the observed hot working set, and an explicit bounded first-hit penalty plus asynchronous fold admission for unseen cold keys. | Resolved |
| CSG-I70 | P0 | A pressured term can span large, unlike segments whose smallest legal whole-segment range exceeds the per-action byte/segment budget. Selecting only another whole-segment merge cannot guarantee the hard extent bound. | Prefer bounded whole-segment compaction, then fall back to a mandatory term-local structural fold that updates only the pressured COW leaf and exact coverage watermark. Keep the source segments for unrelated terms until normal reachability reclamation. Native worker and restart gates pass with hard action limits of eight segments and 64 MiB. | Resolved |
| CSG-I71 | P1 | Catalog publication was bounded, but pending seal still cloned complete scoring metadata, rebuilt the complete text-to-id map, and then recursively inventoried all current pages before publishing a fixed changed-term batch. | Sparse compilation now sorts only changed postings; manifest v10 owns a restart-safe COW lexical lookup; seal resolves only unique changed terms; and mandatory publication is append-only rather than paying for optional reclamation. Fixed two-term medians changed from 9.2/86.7/452.9 ms originally and 5.93/64.81/272.93 ms after lookup-only cutover to 1.29/1.51/1.93 ms at 2k/20k/100k vocabulary. Native 45/45 and real-model SAE 11/11 gates pass. WAL/byte and qualification-host measurements remain release work, not this structural defect. | Resolved |
| CSG-I72 | P0 | A term fold whose watermark cuts through a later compacted extent cannot exclude covered postings without per-posting sequence metadata, causing duplicate or missing scores. | Publish coverage only at whole-extent boundaries. COW replacement keeps fully covered runs hidden and replaces wholly post-coverage runs normally. A touched-leaf preflight now detects straddling before merge/page writes; the native worker selects a legal bounded subrange or explicitly defers, and restart parity passes. | Resolved |
| CSG-I73 | P0 | At hard manifest/extent pressure, every bounded compaction subrange may conflict with one or more inherited fold watermarks. Correctly deferring avoids corruption but can repeatedly select the same impossible action and stop convergence. | Return the first conflict term and fold-forward that exact term to the selected compaction's complete end boundary under the same eight-segment/64-MiB budget, then retry compaction in a later worker round. The worker does not requeue a blocked lexical-impact or over-budget fold, and reports the exact blocker instead of spinning. | Resolved |
| CSG-I74 | P1 | Raw segment or extent count is an incomplete proxy for read cost: colocated blocks can approach contiguous performance while a small number of distant objects can still cause cache misses and loose-bound work. A count-only policy can therefore over-fold cheap keys and ignore expensive ones. | Extent count remains the hard invariant; optional work also uses decayed heat, observed block/candidate work, exact input/posting bytes, surface reduction, and geometric rewrite gates. Fixed action caps and fair rounds provide storage/rewrite budgets. Persistent folds remain valid if telemetry is lost. | Resolved |
| CSG-I75 | P0 | Segment-local block ordinals or a non-monotonic document map make fold and tail bounds incomparable, so a query could underestimate a mixed block or require one heap per segment. | One global-slot block scheduler now sums signed lexical-neutral, lexical-impact, and semantic-impact bounds across fold, immutable tail, and linked L0, then maintains one exact heap. Core exact-score comparisons and native BM25/SAE lifecycle gates cover retirements, semantic transitions, compaction, fold-forward, restart, and visibility catch-up. | Resolved |
| CSG-I76 | P1 | Duplicating document-length extrema in every lexical term block would multiply metadata and force unrelated posting rewrites after a document change. Omitting them would make neutral-BM25 block bounds too loose or unsafe. | Document COW v2 stores conservative count/min/max summaries in the existing authoritative tree. Bulk checked traversal materializes contract-aligned block extrema once per sealed snapshot, while active/pending L0 adds transient extrema and block records. Missing optional metadata selects exact scoring; malformed checked metadata fails closed. | Resolved |
| CSG-I77 | P0 | Updating one shared heat table under the generation-cache LWLock for every queried posting key would serialize hot concurrent reads and turn an optional optimization into a foreground regression. | Each backend retains at most 64 keys and flushes once per 32 successful queries under one short lock. The 70-key batching gate and simultaneous 8-writer/8-reader/VACUUM/maintenance run pass with zero runtime failures or busy rejections. | Resolved |
| CSG-I78 | P0 | Query heat emits a maintenance hint, but the convergent candidate scan currently declares work due only for active/pending L0 or unpublished tail pages. A workload-only hint can therefore be cleared without attempting its eligible persistent fold. | The candidate scan now admits one root-stable heat-table candidate as optional debt without a vocabulary scan. A native background wakeup published the eligible COW fold with no active/pending L0 and exact v2/v3 rows and scores. | Resolved |
| CSG-I79 | P0 | The maintenance candidate records semantic, publication, fold, and debt fields, but the comparator currently ignores all of them after urgency/staleness. Optional work on one index can run before overdue semantic completion on another, contradicting the documented worker order. | The native classifier now assigns query-visibility, semantic-completion, structural, and optional action classes. Urgent/stale state precedes class; the existing database-local fairness cursor applies within each class. One worker transaction still publishes at most one bounded action. | Resolved |
| CSG-I80 | P0 | The convergent candidate path returns after checking linked-L0 and unpublished-tail debt. Once lexical sealing clears both frontiers, semantic-pending documents that remain in the authoritative document COW tree can lose their maintenance hint and never reach asynchronous inference. | Candidate classification now performs one bounded actionable-document lookup through document COW subtree summaries. A real-model background wakeup rediscovered and completed sealed semantic debt after both L0 frontiers had cleared. | Resolved |
| CSG-I81 | P0 | Creating an empty convergent `int4[]` index can retain the synthetic empty-token vocabulary while the core empty index legitimately omits exact DF arrays. Segment publication copied `doc_frequencies` unconditionally whenever `vocab_size > 0`, causing a backend SIGSEGV. | Convergent publication now materializes the empty index's mathematical all-zero DF vector and copies exact source DFs only when present. Empty numeric creation, subsequent L0 writes, exact parity, convergence, and restart pass. | Resolved |
| CSG-I82 | P0 | Public `ii42_index_maintain_due(...)` independently reimplements the legacy v2 scheduler in PL/pgSQL. It can miss convergent semantic COW debt and heat-only folds, and can order work differently from the built-in worker. | The SQL surface is now a C materialized SRF. Catalog enumeration supplies automatic-policy OIDs, while public and worker routes share the exact candidate classifier, comparator, non-blocking executor, fairness cursor, and result accounting. Native replay proves mandatory structural work precedes optional heat and that the public route later discovers and publishes the queued heat-only fold. | Resolved |
| CSG-I83 | P0 | Physical standby WAL replay can replace an `auto_preload` generation without emitting a foreground work hint. The preload worker then waits for the five-minute catalog-reconcile interval while the obsolete resident remains hot and the current generation stays cold. | Recovery-mode preload reconciliation now polls only `auto_preload` catalog metadata at the greater of five seconds and the configured preload interval. Standby replay retired the old resident and loaded the replacement generation in about seven seconds while every durable maintenance surface remained a recovery no-op. | Resolved |
| CSG-I84 | P0 | The superuser-only full-overlay oracle gate is evaluated only after opening the semantic payload. A malformed or absent payload can therefore mask the required `insufficient_privilege` rejection for an unprivileged application role. | The privilege gate now runs after source-table/RLS authorization but before semantic payload I/O or result allocation. The complete runtime-service privilege smoke passes for an ordinary application role. | Resolved |
| CSG-I85 | P0 | The low-memory SAE builder conflates retaining a streamable primary index with selecting convergent-segment output. It discards the legacy semantic payload whenever a segment source exists, even if publication still selects v2, so `CREATE INDEX ... WITH (sae=true)` can succeed with a BM25-only physical generation that fails its declared contract. | Streaming retention and convergent output are now independent. Legacy v2 publishes its complete unified semantic payload without a duplicate primary matrix; v3 consumes the checked segment source. The runtime privilege smoke and all eight real-model convergent lifecycle gates pass. | Resolved |
| CSG-I86 | P0 | Advancing one high-DF term copied its complete prior neutral fold plus a small selected tail. Repeated low-rate updates were exact but could create unbounded write amplification and cache churn, contradicting the sustained-update product goal. | COW term-map v6 and the worker preserve independently referenced major/minor coverage plus raw tail. Optional carry requires comparable new posting bytes; ratio/structural/quiet-hot promotion is explicit; publication validates and prewarms the new object. A 38-gate native lifecycle run proves small-tail deferral, geometric carry, zero-extent promotion, exact scores, restart, and 32-rotation convergence. The v2 paired matrix makes converged-vs-static performance a blocking gate. Broader mixed CRUD latency and qualification-host scale remain under CSG-10 rather than this format defect. | Resolved |
| CSG-I87 | P1 | An abnormal temporary PostgreSQL backend could leave the lifecycle smoke blocked indefinitely in `pg_ctl -m fast`, while a pre-existing output JSON remained on disk and could be mistaken for fresh passing evidence. | Remove the requested artifact before execution, bound fast shutdown to 30 seconds, and escalate failed cleanup to immediate shutdown. A failed run can no longer retain a stale green report or orphan the isolated test cluster indefinitely. | Resolved |
| CSG-I88 | P1 | Optional compaction and fold still derived an exact whole-root reachability complement before interior reuse. Fixed 1,664-byte compaction measured 5.74/63.83/285.88 ms at 2k/20k/100k vocabulary, proving global metadata cost independent of rewritten input. | Manifest v11 now authenticates at most 64 canonical retired ranges. Routine optional work consumes only these ranges after the nonblocking reader fence and root/high-water revalidation; the new manifest is append-only and full reachability remains diagnostics/scrub only. Fixed-input medians are now 1.27/1.27/1.56 ms, all result rows remain exact, native lifecycle passes 46/46, and real-model SAE lifecycle passes 11/11. | Resolved |
| CSG-I89 | P1 | The first bounded retirement producer records prior manifests and explicitly replaced segment payloads, but successful COW path-copy and pending-L0 consumption can also supersede internal COW objects or linked L0 pages. They remain safe unreachable debt, yet high churn may require an explicit scrub before relation bytes plateau. | External term/document/lexicon patches now return every old object ref they supersede, and pending seal returns every consumed linked-L0 page. The native no-failure gate reports exact hint/interior equality at both its first transition (`14/14`) and final reuse state (`118/118`); fixed-live-set UPDATE/VACUUM remains v2-query exact with complete transition evidence. | Resolved |
| CSG-I90 | P0 | Exact retirement evidence alone does not bound relation growth. In a 24-cycle, four-row UPDATE/VACUUM matrix, every cycle remained TID/score exact and final hints covered all `591` unreachable blocks in only four ranges, but the published high-water mark grew from 40 to 608 blocks because foreground L0 and mandatory seal still append. | Reader-fenced optional publication now removes handed-off ranges from the descendant manifest, WAL-marks them recyclable, and exposes them to PostgreSQL's reconstructible index FSM. Every claim verifies the physical marker and otherwise leaks safely/falls back to append. Old-reader tests force zero reuse before release and positive reuse afterward; restart and exact query gates pass. The 48-cycle high-water mark fell from `608` to `79`; the remaining three-block cadence is document-slot capacity owned by CSG-I92, not page reuse. | Resolved |
| CSG-I91 | P0 | The first 48-cycle FSM-reuse run reduced fixed-live-set growth from `40 -> 608` blocks to `40 -> 79`, but did not reach a true plateau. Every second tier compaction copied the prior historical segment unchanged: selected input grew from `1,960` to `6,476` bytes while only four rows remained live. Recyclable pages therefore work, but live segment payloads still retain VACUUM-safe retired postings and version records forever. | Query attachment now obtains tuple-version/TID/length/retirement authority from one COW document traversal; immutable payloads own postings only. Bounded selected compaction drops every event for COW-authoritative frozen retirements and emits a checked empty history barrier when nothing remains. Alternating inputs stay near `1,248 / 1,888-1,976` bytes, full native lifecycle passes 50/50, and delete-all, reinsert, convergence, and restart remain exact. Physical high-water growth is now isolated to CSG-I92 slot capacity. | Resolved |
| CSG-I92 | P0 | CSG-I91 bounded alternating compaction input at about `1,248 / 1,888-1,976` bytes and restored 48/48 native parity, but the fixed four-row relation still grew from 76 to 79 blocks at cycles 16, 31, and 46. The cadence matched one new COW document leaf per 16 permanently reserved tuple-version slots. | Document COW v3 tracks lexical, semantic, and event residency and summarizes exact reusable capacity. Seal, fold, compaction, and retirement update those counters; allocation claims a zero-residency slot under the append lock from a root-persisted monotonic cursor. Sparse mapped payloads, `born_sequence` tie keys, and root-relative patch validation prevent mixed incarnations. In 48 fixed-live cycles, slot HWM reached seven by cycle two and page HWM reached 82 by cycle four; both then plateaued with exact v2/v3 TIDs and scores. Restart, old-reader page fencing, abort/savepoint/2PC, and real-model semantic lifecycle gates pass without a corpus scan. | Resolved |
| CSG-I93 | P0 | Semantic inference runs outside the append lock. After CSG-I92, a source slot could theoretically be drained and reused between model output validation and L0 append, allowing stale output to target a new incarnation. | Completion/quarantine append now accepts the complete source COW record as a compare-and-append guard. Under the append lock it loads one current manifest and at most `II42_DOCUMENT_COW_RADIX_LEVELS + 1` COW objects, rejects any owner/state/retirement/residency mismatch, and publishes nothing stale. The bounded lookup unit gate, 11/11 real-model lifecycle gates, and 21/21 transaction/restart gates pass. | Resolved |
| CSG-I94 | P0 | The first CSG-I92 allocator excluded active and pending claims by materializing both complete linked-L0 frontiers under the append lock, making foreground UPSERT work `O(delta)` despite bounded COW slot lookup. | Read-root v3 reuses its previously zero-reserved offset 28 as a checked `reusable_document_slot_cursor`. One UPSERT loads one maintenance manifest and follows one fixed-depth document-COW path at or above the cursor, advances the cursor with its L0 append, and never loads an L0 snapshot. Rotation preserves it; maintenance resets it only after both frontiers are empty. Old v3 roots decode the former zero field safely. Status exposes the cursor, and fixed-live, real-model, transaction/restart, and 1,024-row frontier gates pass. | Resolved |
| CSG-I95 | P1 | The current-tree 40,000-document paired matrix preserved exact ids and scores and passed every folded/static gate, but one impact-specialized p95 measurement was `1.111x` the materialized path, above the `1.05x` gate. Mean, p50, p99, and QPS passed. The host simultaneously reported load `7.382/7.112/6.533` and a high-I/O disk image job, so this is not yet attributable to the scorer. | Clean 40k and 250k observed-work reruns passed all exactness and performance gates. Impact/materialized p95 ratios were `0.43578x` and `0.12583x`; folded/static p95 ratios were `0.95344x` and `0.98780x`. The noisy red sample did not reproduce. | Resolved |
| CSG-I96 | P0 | Root-relative score-slot reuse requires `born_sequence` ordering, but the first tie-break implementation selected zero-score fallback documents by scanning every score slot. Any normal query with fewer than `k` positive hits, as well as an empty or unknown-id query, therefore regressed to `O(corpus)` query work. | Snapshot attachment now builds one disposable live-slot order from checked COW `born_sequence` keys and the visible L0 overlay. Invalid TIDs and aborted holes are excluded and exact live cardinality is enforced. Block-max all-zero ranking and MVCC zero-score completion consume only the required ordered prefix. Core instrumentation proves `k=3` examines three ordered documents rather than all eight score slots; 52/52 native, 11/11 real-model SAE, 21/21 transaction/restart, and 64/256/1,024 semantic-frontier gates preserve exact v2/v3 behavior. Full-scale attach/RSS cost remains a release gate under CSG-I95/CSG-10. | Resolved |
| CSG-I97 | P0 | Promoting convergent v3 for normal builds while leaving `ambuildempty()` on generation-delta v2 would make an UNLOGGED index silently change physical format after crash recovery copies INIT over MAIN. | Initial sealed-bundle publication is fork-aware. INIT is a complete zero-document v3 generation with the same checked query/model contract as MAIN and WAL coverage for every INIT page. The seven-gate empty/UNLOGGED lifecycle proves crash restore, first post-crash mutation, and BM25/SAE contract stability. Existing legacy indexes remain the only v2 boundary and migrate explicitly by `REINDEX`. | Resolved |
| CSG-I98 | P0 | A maintenance transaction could make a replacement root buffer-visible and return success while its final Generic WAL record remained beyond the flush position. An immediate crash then recovered the previous root even though queries had remained logically exact. Session-scoped maintenance ownership also allowed another worker to chain from the not-yet-durable root. | Internal maintenance ownership is transaction-scoped and remains held until transaction end. Maintenance-only active-L0 rotations and COW manifest switches flush their final root WAL record before returning success; normal foreground L0 appends retain PostgreSQL commit semantics. A clean 21-gate transaction/MVCC/VACUUM/2PC suite recovers the exact compacted generation and zero linked-L0 debt after immediate stop. Neighboring default-v3 55/55, real-model SAE 11/11, and UNLOGGED/INIT 7/7 suites remain green. | Resolved |
| CSG-I99 | P0 | An old read root may end inside a physical linked-L0 tail page that a later writer legally extends or links to a successor chain. Requiring the physical page to end exactly at the old logical frontier reports a false page-order failure under concurrent reads and writes. | The reader now treats its root as a logical record/sequence prefix, validates and excludes complete newer suffix records, and accepts a newer tail link only after the old frontier is complete. Chain order comes from checked ordinals/links rather than block-number monotonicity. A deterministic post-root-decode pause proved same-page `1 -> 1` and successor `1 -> 8` appends: both old readers returned their exact old rows and both new readers observed the committed row. The final 8x8 writer/reader/VACUUM run retained this gate. | Resolved |
| CSG-I100 | P0 | Real-model fixed-live churn can finish with exact queries and zero L0/semantic debt while retirement ranges accumulate on the clean `no_pending` path. A 24-cycle diagnostic grew from 166 to 1,138 physical pages, retained 422 reachable pages, and reached the 64-range cap, where dropping ranges would turn no-failure churn into unbounded safe leakage. | At 16 retirement ranges, clean maintenance attempts one reader-fenced identity-manifest transition. It preserves every query object/frontier, hands the old ranges to checked FSM markers, and retains only the superseded manifest without a posting, vocabulary, document, or model scan. Linked-L0 claims individual recyclable pages; bounded multi-run COW claims restore undersized fragments and refresh FSM state. In the final 24-cycle fixed-representation real-model gate, every second-half sample was exactly 1,418 pages / 11,616,256 bytes. 288/288 mutations, 329 reader queries, 664 maintenance calls, 669 VACUUM calls, native/oracle parity, zero runtime failures, and zero busy rejections passed. | Resolved |
| CSG-I101 | P0 | The design claimed immediate exact BM25 corpus statistics after UPDATE/DELETE, but PostgreSQL's AM contract gives `aminsert` only the replacement tuple/TID and gives no index callback for DELETE. Before VACUUM, heap MVCC hides dead rows while their old length/DF contribution can still change scores and rank relative to REINDEX. Recovering the predecessor in the foreground would require a heap-wide lookup, executor-private coupling, or trigger lifecycle. | Keep foreground writes row-bounded and make the real boundary explicit: v3 status reports `retirement_statistics = vacuum_convergent`; the design distinguishes immediate row visibility from retirement-statistics convergence. A native differential gate requires the pre-VACUUM live row set to match a REINDEX oracle and, after `VACUUM (INDEX_CLEANUP ON)`, requires exact ID, rank, and score parity for UPDATE and DELETE without a whole-index query overlay. | Resolved |
| CSG-I102 | P0 | A manual v3 mutation updates stale state without changing the immutable segment root. The backend-local cache therefore matched the old root and returned before checking the current metapage stale flag, suppressing the documented stale `NOTICE` on cache hits. | Check the caller-supplied current metapage stale flag before either cache-hit or cold-attach return, emit one consistent notice, and remove the duplicate cold-only branch. Preserve old-root queryability and manual no-overlay semantics. | Resolved |
| CSG-I103 | P0 | The monolithic `ii42_integration` expected file still encodes legacy v2 byte/page sizes, one-shot delta folds, old overlay-budget behavior, unstable internal document slots, and pre-VACUUM score constants. Blindly accepting its current v3 output would hide contract regressions and make the release gate layout-dependent. | Replace obsolete assertions with v3 logical gates: staged bounded convergence, business-ID/TID joins, immediate MVCC row visibility, post-VACUUM REINDEX score/rank parity, exact linked-L0 visibility beyond legacy overlay budgets, scheduler debt observability, and the current cache-tier contract. Retain deterministic physical-layout snapshots only where they intentionally gate the format. | Resolved |
| CSG-I104 | P0 | The shared-preload lifecycle closure still treated every fresh index as a v2 DSM blob. A large v3 exact-root preload therefore reported `loading` with no DSM candidate bytes, and the test incorrectly required a 1 MiB arena admission miss even though convergent storage intentionally prewarms relation pages through PostgreSQL's buffer cache. It also expected unmarked foreground v3 reads to allocate private-cache residents and evict them under arena pressure. | The v3 gate now requires a physical payload larger than the configured arena to remain non-DSM-share-eligible, become resident through one exact-root marker, explicitly report `tier=postgres_buffer_cache`, stay queryable, and produce no DSM admission miss. Unmarked foreground v3 reads remain queryable without polluting the auto-preload registry or evicting marked roots. Separate explicit legacy-v2 fixtures retain decoded-payload admission, eviction, pinning, and rollout coverage. | Resolved |
| CSG-I105 | P0 | Convergent preload keeps relation pages in PostgreSQL shared buffers and stores only an exact-root marker in the II-42 arena, but normal query attach still called `ii42_segment_pages_load_sealed_snapshot()` and retained the complete decoded manifest, vocabulary, term/fold metadata, payload views, document metadata, tie-break order, and L0 snapshot in each backend. | The public v3 routes now use a checked root, page-native posting/document cursors, query-specific L0 projection, bounded top-k state, and exact zero-score authority. The 79-gate lifecycle is exact. At 80k versus 5k rows, the complex raw-query RSS gate measured zero index-scaled II-42 context bytes, 12 KiB malloc growth, and 272 KiB effective private-writable growth. Full snapshots remain maintenance/test oracles only. | Resolved |
| CSG-I106 | P0 | Public raw-query helpers used page-native scoring only for simple terms. Boolean, phrase, prefix, and negated query shapes still attached the complete decoded generation. | Every positive shape now uses one fixed-root progressive page-native stream. Boolean, phrase, and negation are heap-verified under the same snapshot; immutable and visible-L0 prefixes expand to exact per-term coordinates so each term keeps its own DF/IDF. The active-L0 raw-query gate, including 2,000 overlapping prefix terms, passes exact v2/v3 IDs and scores, and the complex-query backend-memory gate passes. | Resolved |
| CSG-I107 | P1 | Immutable prefix expansion is output-bounded but obtains matches by scanning the hash-COW lexicon, so cold CPU and shared-buffer reads remain `O(vocabulary)` even though backend memory is bounded. | Manifest v12 owns a checksummed ordered prefix-COW tree. Native queries range-seek it; a 20k-key zero-match unit gate loads at most tree depth. Suffix publication path-copies only affected leaves/ancestors, including the 47/48/49-child split boundary. Native query parity, restart, reclamation, bounded 100k seal, and physical replication pass. | Resolved |
| CSG-I108 | P1 | Product documentation claims convergent VACUUM deletion discovery is proportional to the dead TIDs supplied by PostgreSQL. The index AM actually receives a deadness callback, not a dead-TID list, and must visit the authoritative document-COW records plus bounded linked L0 to discover dead entries. | The product contract now states the exact `O(document slots + bounded L0)` discovery cost. VACUUM opens no posting payload, allocates no corpus bitmap, and retires records with a fixed 65,536-record COW workspace. Ordinary background convergence remains sparse and input-bounded. | Resolved |
| CSG-I109 | P0 | One large VACUUM appends one current-transaction RETIRE record per dead document. Those records cannot cross the safe-XID seal gate before the VACUUM transaction ends, so about 131k retirements can exhaust the active/pending L0 hard frontier and abort otherwise valid cleanup. Raising the frontier would make query projection unbounded. | Globally-dead versions now publish through bounded immutable document-COW patches and share one frozen L0 sequence fence. The fence remains independently sealable after a later VACUUM error. A 140,000-document fault-injection regression proves partial-batch recovery, one 96-byte fence, idempotent resume, seal, restart, and post-frontier writes; the complete v3 lifecycle remains 80/80. | Resolved |
| CSG-I110 | P0 | The design requires lexical-first eventual SAE mutation, but the public default, quickstart, policy recommendation, and reloption validator still advertise or accept SAE realtime. Fresh v3 mutation rejects that combination, so a documented index can fail on its first write. | SAE now defaults to eventual, rejects explicit realtime/manual at definition time, emits only eventual SAE policy recommendations, and shares one current contract across public docs. Default-SAE CRUD, worker completion, REINDEX, restart, transactional lifecycle, and physical replay pass staged PG18 gates. | Resolved |
| CSG-I111 | P1 | Public architecture, parameter, policy, and shared-cache documentation still describe a complete unified-delta cache as mandatory v3 query state. Current v3 public routes instead use bounded page-native context, posting cursors, query-specific L0 projection, PostgreSQL shared buffers, and exact-root markers. | Current product docs define relation pages, checked roots, page-native cursors, matching-L0 scratch, shared buffers, exact-root markers, and optional HOT_FOLD residency as the v3 memory model. Legacy complete snapshots/caches are labeled oracle-only, and static inventory rejects stale authority claims. | Resolved |
| CSG-I112 | P1 | `auto_rebuild_threshold`, `auto_rebuild_delta_bytes`, and policy sizing still inherit cache-centric v2 semantics, while v3 maintenance is primarily driven by linked-L0 frontier, semantic debt, structural pressure, and fair worker rounds. The advertised knobs may not control the behavior users infer from their names. | The inert public thresholds, decoded-overlay limits, and decoded-cache GUCs are removed. V3 exposes physical linked-L0, semantic-debt, worker-progress, and root-residency state; immediate low-rate convergence remains driven by one coalesced hint/reconciliation scheduler and one bounded action. Current staged exactness, lifecycle, concurrency, replication, and static gates pass. | Resolved |
| CSG-I113 | P1 | The hidden legacy-v2 differential oracle still participates in installed SQL dispatch and a large runtime implementation, allowing test-only architecture to influence product documentation and policy. | Independent checked golden artifacts cover numeric/text BM25, semantic scoring, MVCC, zero-score ordering, predicates, and linked-L0 overlays. Installed SQL enters one native v3 scorer; product source, symbols, strings, GUCs, status, preload, tests, and current docs contain no v2 runtime fallback. Non-v3 roots fail closed and migrate only by explicit `REINDEX`; the isolated corruption fixture remains negative evidence. | Resolved |
| CSG-I114 | P2 | `ii42_am.c` combined reloptions, shared memory, build, linked-L0 publication, maintenance scheduling, VACUUM, query execution, SQL functions, and folds in one oversized authority surface. The public design was compact, but implementation review and change isolation were not. | The qualified milestone has extracted options, SQL, checked meta/root, build, mutation, maintenance-lock, scan visibility, preload registry, scheduler and runtime-service shared state, HOT_FOLD codec/view, and reader-fence/page-reuse authorities without changing behavior. The monolith still owns the VACUUM orchestrator, background worker loops, major segment-maintenance orchestration, and query preparation/scoring glue, so modularization is not complete. Finish only cohesive behavior-preserving slices under the tiered policy, then run one final Gate D matrix. | In progress |
| CSG-I115 | P0 | The current-source 8-writer/8-reader real-model concurrency gate completed every foreground mutation and query, but concurrent VACUUM rejected a document-COW transition. The 4x4 replay proved that L0 closure rebuilt an already materialized same-sequence COW version as `L0_OWNED`; this changed ownership instead of adding retirement to the current record. Page refs, decoding, query parity, and bounded frontier replay remained valid. | L0 closure now classifies the current slot incarnation: same-version materialized records retain ownership/residency and receive retirement in place; same-version aborted placeholders become retired L0 records; stale incarnations are ignored; only unmaterialized/reusable versions become `L0_OWNED`. Final staged evidence passes 8x8 concurrency, 80 native, 11 SAE, 21 transactional, and 10 large-frontier gates. | Resolved |
| CSG-I116 | P0 | The v3 fast path in the owner-only `ii42_query(...)` exact-BM25 diagnostic entered page-native lexical preparation before checking whether an SAE index actually owns an exact-BM25 payload. A unified SAE index therefore reached an unrelated COW lexical-lookup error instead of the frozen capability rejection. | Apply the exact-BM25 capability gate before every raw diagnostic preparation path. Keep `ii42_query(...)` as the unified product route and prove BM25 exact diagnostics remain unchanged while SAE fails with the documented `feature_not_supported` error. | Resolved |
| CSG-I117 | P1 | The installed mutable-lifecycle gate still synchronized on the retired v2 `lightweight_delta_fold` and required a concurrent writer to invalidate a whole-generation swap. V3 seals one frozen pending L0 while writers append to an independent active L0, so the test neither exercised nor proved the current publication boundary. | Add a test-only pause after the checked pending-seal snapshot, append concurrently to active L0, require the frozen prefix to publish without rejecting or absorbing the new tail, then complete the tail through normal bounded maintenance. Remove the old `meta_changed`, decoded-delta, and generation-swap assertions. | Resolved |
| CSG-I118 | P1 | The mutable gate changed a BM25 tokenizer reloption and then expected ordinary `ii42_index_maintain()` to perform a hidden full-heap rebuild. That contradicts the v3 contract that incremental maintenance is bounded and contract changes require an explicit rebuild. | Require status and query to fail closed on contract drift, prove incremental maintenance does not reinterpret the old root, and recover with explicit `REINDEX` or refresh. Keep full-corpus work outside the ordinary maintenance selector. | Resolved |
| CSG-I119 | P0 | The page-native overlay is exact against the independent oracle, but worker semantic completion and explicit REINDEX can assign materially different scores to the same live rows. Mixed CRUD measured a maximum pending-to-converged delta near `54.28`; TID reuse measured a compaction-to-REINDEX delta near `41.57`. The differential localized the mismatch to semantic admission: REINDEX allocates one corpus-global semantic budget and prunes every document together, while eventual completion appends every positive atom for one document. A new row can therefore change old keep-counts, which cannot converge through bounded maintenance. | Define `document_semantic_budget_ratio` as a deterministic per-document ratio over that document's mapped lexical posting count. Full build and worker completion must apply the same impact-first, atom-id-tiebroken keep rule, then persist atom-id order. Measure the deliberate root-byte/score change against ARCH-0, require overlay-oracle, converged, restart, and REINDEX score parity, and retain bounded ordinary maintenance with no heap or whole-index scan. | Resolved |
| CSG-I120 | P0 | The first ARCH-2 current-source 8-writer/8-reader SAE replay exposed `invalid ii42 query L0 retirement target` in concurrent readers, followed by `invalid ii42 COW document-directory append patch object` during final maintenance. A minimum 1x1x1 replay fails earlier with `empty ii42 pending segment`: VACUUM has already incorporated every effective event in the frozen pending frontier into document COW, but seal rejects the resulting legal no-op payload. Native 80/80, transaction 21/21, and focused SAE 15/15 gates pass, so this is isolated to concurrent mutation/VACUUM/seal ownership rather than the retired reloptions. | A fully pre-applied frozen frontier now publishes the existing checked empty `HISTORY_BARRIER`, consuming its exact sequence range without changing root format, lock/WAL protocol, scorer, or action selection. Repeated full 8x8 replay plus native, SAE, transaction, 140k VACUUM-frontier, SQL, core, storage-plateau, and physical-replication gates pass. | Resolved |
| CSG-I121 | P0 | After the CSG-I120 empty-frontier representation fix and its repeated 8x8 qualification, the same invalid retirement-target and COW-patch symptoms recurred once during slice-4f stress. Convergent `VACUUM` still snapshots document COW and linked L0 outside the per-index maintenance authority, while seal, compaction, fold, reclamation, and semantic completion can publish a new root concurrently. Repeated candidate and pre-4f baseline runs show the defect is timing-sensitive rather than a stable accounting-arithmetic change. | Convergent `VACUUM` now acquires transaction-scoped per-index maintenance authority before root/COW/L0 discovery while retaining its short writer-barrier publication upgrade. A deterministic paused-seal probe proves the lock boundary, repeated 8x8x24 stress is exact, and native 80/80, mutable 71/71, SAE 15/15, transaction 21/21, 140k frontier 10/10, replication, runtime-service, storage-plateau, and 40k performance gates pass. | Resolved |
| CSG-I122 | P1 | The orphaned May storage-bloat smoke races `auto_preload`, then requires retired generation-page diagnostics and pre-v3 reuse result strings. Bounded waiting proves the first race, but translating the remaining assertions would restore an obsolete lifecycle. | Preserve the non-blocking single-publisher runtime contract, retire the unreferenced smoke, and qualify storage through the current same-index fixed-live native/oracle, zero-debt, 24-cycle page/byte plateau gate. The 8x8 run completes 288/288 mutations and all 24 fixed-live cycles plateau at 1,778 pages. | Resolved |
| CSG-I123 | P0 | Retired-page reuse originally released its reader fence before reused-page writes and root publication. After retaining that fence, mixed pressure exposed a second defect: one shared COW retirement sequence retired several dead L0 upserts, but only one physical `RETIRE` named the reservation slot, so later `VACUUM` could retire another already-retired same-born slot again. | Commit `f649b60d` retains reader-fence ownership and validates the expected L0 born sequence under the append lock. The post-CSG-I124 staged artifact passes deterministic double-`VACUUM`, exact 8x8x24 normal/oracle concurrency with zero debt, native 80/80, mutable 71/71, transaction 21/21, 140k VACUUM frontier 10/10, physical replication, fixed-live storage plateau, restart/reuse, and the 40k exact performance matrix. | Resolved |
| CSG-I124 | P1 | Retirement metadata was capped at 64 ranges by silently deleting the smallest excess ranges before root publication. Mixed 8x8x24 runs completed correctly but left interior-unreachable pages that were neither manifest retirement hints nor WAL-logged recyclable markers, so those pages could not return through ordinary maintenance. | Commits `b0aa6ffd` and `1d21a0e7` add side-effect-free exact retirement preflight, a conditional reader-fence retry, and dynamically sized post-publication FSM handoff of all new plus unused prior ranges. No new persistent authority or free-list lifecycle was added. Greater-than-64 accounting, restart/reuse, native 80/80, exact 8x8x24 pressure, mutable 71/71, transaction 21/21, 140k frontier 10/10, physical replication, storage plateau, and 40k performance gates pass. | Resolved |
| CSG-I125 | P1 | The final qualified lifecycle artifact predated the documentation-only milestone HEAD, so current evidence did not bind one authorized commit, source archive, staged package, runtime library, and checksum set. | Gate D now binds clean commit `7a2a22869bd689cceeb5b0bd3b36c3680cf6452f`, package fingerprint `5cd4631ced524fb7430be56cfb135c0168f22d6ef78430799283ca0c93df0aa1`, pinned ONNX Runtime 1.26.0, installed runtime identities, the psql_bm25s source package, all 30 maturity steps, the 50k lifecycle, and the 75k/96-client product benchmark. The complete artifact is `/tmp/ii42-arch4-final-product-maturity-v4.json`. | Resolved |
| CSG-I126 | P2 | The production shared library exports internal `ii42_test_*` C entry points used by superuser-created white-box smoke functions, although installed SQL exposes none of them. | All 12 hooks are used by the independent page-native golden smoke and none appears in installed SQL. Product inventory now requires exact implementation-to-test-reference equality and rejects installed exposure. A second test runtime would duplicate storage code, so the indispensable superuser-only white-box boundary remains isolated in test code. | Resolved |
| CSG-I127 | P3 | Diagnostic names such as `generation_cache` and `mutable_delta` can be mistaken for retired storage authorities even when they now describe shared derived residency and the linked-L0 read view. | Current docs explicitly define these retained public diagnostics as shared derived residency and linked-L0 read-view state, not storage or lifecycle authorities. New internals use page-native/linked-L0 terminology; stable SQL and JSON names remain unchanged to avoid cosmetic compatibility churn. | Resolved |
| CSG-I128 | P1 | The source-migration maturity smoke treated `generation.docs` as the current live row count immediately after one maintenance call. On v3 that field is explicitly scoped to the immutable `sealed_generation`, while committed CRUD is already query-visible through linked L0 and one bounded maintenance call may publish a derived view before a later call compacts it. The stale assertion rejected a healthy query-ready index with legal pending debt. | Readiness, immediate live-query parity, and sealed-generation convergence are now separate gates. The focused installed-package replay proves exact old/new CRUD scores before compaction, then records `active_l0_rotated` and `pending_l0_sealed` as two bounded phases before requiring four sealed documents and zero linked-L0/pending debt. The gate passes at `/tmp/ii42-arch4-final-product-maturity.source-migration-fixed.json` without changing product maintenance behavior. | Resolved |
| CSG-I129 | P1 | The eventual semantic-quarantine smoke held the public session-level maintenance guard on one connection, then executed `VACUUM` on a second connection. Since CSG-I121 correctly placed convergent VACUUM under the same per-index maintenance authority, the second backend waited indefinitely on the guard held deliberately by the test. | The product lock boundary remains unchanged. Test-owned VACUUM now runs through the same maintenance session that owns the reentrant advisory guard, while mutations and diagnostics stay on the writer connection. `release_guard()` also proves that ownership is fully released. Two focused installed-package runs complete in about 17 seconds and pass row-local retry/delete, REINDEX, restart recovery, final zero debt, and guard release at `/tmp/ii42-arch4-final-product-maturity-v2.semantic-quarantine-fixed-v2.json`. | Resolved |
| CSG-I130 | P1 | The 50k production-model medium gate performed one UPDATE and one DELETE, ran plain `VACUUM`, then required the sealed unified posting count to equal the heap live count. PostgreSQL may skip index cleanup under `INDEX_CLEANUP AUTO` when only two versions are dead; the observed healthy zero-delta root therefore retained exactly 50,002 physical records while the heap had 50,000 live rows. | MVCC query visibility remains independent from physical retirement, and the retirement-specific gate now requests `VACUUM (INDEX_CLEANUP ON)`. The full 50k installed-package replay reaches 50,000 sealed and posting records, zero linked-L0 and semantic debt, exact query diagnostics, and REINDEX parity at `/tmp/ii42-arch4-final-product-maturity-v3.medium-lifecycle-fixed.json`. Build and REINDEX each take about 138 seconds; bounded maintenance takes 2.19 seconds. | Resolved |
| CSG-I131 | P3 | `ii42_am_vacuum_stats.needs_refresh` is written after convergent VACUUM retirement but never read. Correctness and eventual physical reclamation are preserved by periodic reconciliation, but successful VACUUM does not immediately wake the existing maintenance path. | The dead statistic is removed. A non-empty tracked retirement now marks the existing coalesced maintenance hint and uses the existing transaction callback for post-commit worker launch; abort remains harmless because workers recheck durable debt. Product inventory passes, the staged source builds, and `/tmp/ii42-vacuum-wakeup-smoke.json` passes all 80 page-native lifecycle gates including CRUD retirement and VACUUM convergence. | Resolved |
| CSG-I132 | P2 | PostgreSQL parallel heap build, parallel VACUUM discovery, and parallel AM scan are not implemented. Large first builds and document-COW VACUUM discovery remain single-process even though ordinary incremental maintenance is bounded. | Preserve this as a post-beta scale investigation. Add parallelism only behind PostgreSQL AM contracts and the existing single-root publication authority; do not introduce another index lifecycle or weaken exact scoring. | Deferred |
| CSG-I133 | P3 | SAE reports, machine-artifact metadata, and frozen source metadata are split across three sibling `docs/research-*` roots, obscuring that they form one historical research system and leaving multiple archive entrypoints. | `docs/research-sae/` is now the single archive entrypoint with `reports`, `artifacts`, and `source` authorities. All 1,433 report files, 1,401 artifact rows, and 1,595 source rows remain present. Tracked links and canonical paths are migrated; the one changed retained report path is bound by artifact-manifest digest `50eff315e36899bed4f96560486ab68a7ac5b505bd2d557d445d70a629eaa65e`. Static inventory rejects the three retired roots and passes all layout, stage-count, digest, and Markdown-link gates. | Resolved |
| CSG-I134 | P1 | Ordinary product documentation duplicated responsibilities and mixed current page-native behavior with retired generation/v2/sidecar terminology and historical performance conclusions. This obscured the single-index lifecycle and made API, policy, and operations guidance harder to verify. | The README, documentation map, architecture, API, query, policy, runtime, memory, maintenance, migration, contribution, testing, and semantic examples now describe one BM25/SAE access method, page-native root, linked L0, eventual-only lexical-first semantic completion, shared worker ownership, and one public lifecycle. Generation-era guide filenames are retired and inventory-blocked; retained `generation_cache` SQL/GUC names are documented only as current runtime/residency diagnostics. Every root document is classified, product Markdown links pass, `py_compile`, `test_product_convergence_inventory.py`, and `git diff --check` pass. Research, performance, blog, and technical-report evidence is explicitly frozen pending deliberate refresh. | Resolved |

### CSG-I119 closure slices

1. [x] Prove that pending page-native scoring matches the independent
   full-overlay oracle and that MVCC retirement/TID reuse are not the source of
   the mismatch.
2. [x] Trace REINDEX to corpus-global `semantic_target` allocation and worker
   completion to unpruned per-row semantic transition publication. Record that
   exact global reallocation is incompatible with bounded convergent
   maintenance.
3. [x] Introduce one deterministic per-document semantic budget helper. Apply
   it to full build and completion using the same mapped lexical pair count,
   impact ordering, and atom-id tie/order rules.
4. [x] Add a focused build/completion/restart/REINDEX score differential and
   record posting counts, root bytes, and score changes against ARCH-0.
5. [x] Re-run the complete mutable, native, SAE, transaction, concurrency, and
   staged-package gates before closing the issue.

Current CSG-I119 implementation evidence:

- Full build and bounded worker completion now share one per-document keep
  count and one impact-first/atom-id ordering rule. Completion derives the
  lexical denominator from the same mapper contract without scanning another
  document or the whole index.
- The focused mixed-CRUD differential reports zero overlay-to-oracle,
  oracle-to-converged, pending-to-converged, and converged-to-REINDEX score
  delta at the frozen tolerance. Insert and update semantic visibility also
  pass in the active mutable suite.
- The remaining main-sequence REINDEX mismatch is a stale test boundary: it
  compares an index immediately after `active_l0_rotated`, while semantic
  completion still reports one exact sealed pending row, against a fully built
  REINDEX. The active gate must complete the bounded rotation, seal, semantic
  completion, and generated-L0 seal chain before comparing scores. It must not
  make eventual mutation synchronous or weaken score parity.
- Several other active red gates still encode retired v2 assumptions: a
  complete backend-local decoded cache, semantic completion in the first
  maintenance call, zero physical append after a rolled-back savepoint, and a
  fixed generation ID across VACUUM COW publication. They will be replaced by
  bounded page-native state, final convergence, MVCC visibility, and checked
  root evidence before the full CSG-I119 replay.

The first corrected mutable replay at
`/tmp/ii42-arch1-p2-mutable-2.json` passes 70/71 gates, up from 54/71. It now
passes exact converged-to-REINDEX scores, eventual phase ordering and telemetry,
semantic root pinning, rollback memory/visibility, worker batching, VACUUM/TID
reuse, preload, and zero index-sized backend state. The sole remaining red gate
is a BM25 contract-drift fixture that still expects the old `stale=true`,
zero-delta hidden-rebuild state. V3 instead retains the newly inserted row in a
bounded lexical L0, fails query closed on the contract signature, and requires
explicit REINDEX; the gate will be aligned to that already documented behavior.

The completed mutable replay at `/tmp/ii42-arch1-p2-mutable-4.json` passes
71/71 gates. The final nondeterministic subtransaction gate now bounds
MVCC-dead physical append debt by the number of failed statements while still
requiring zero visible rows, zero pending mutation memory, bounded backend RSS,
unchanged live posting count, and a query-ready checked root. This accepts no
logical leak and no index-sized backend state.

The same staged PG18 product build passes the complete closure matrix:

- `/tmp/ii42-arch1-final-native80.json`: 80/80 native v3 gates;
- `/tmp/ii42-arch1-final-sae.json`: 15/15 default/eventual SAE gates;
- `/tmp/ii42-arch1-final-txn21.json`: 21/21 transaction and recovery gates;
- `/tmp/ii42-arch1-final-concurrent-ddl.json`: 8/8 DDL/rewrite gates;
- `/tmp/ii42-arch1-final-replication.json`: physical replication lifecycle;
- staged runtime-service temporary-PG, product inventory, Python compile, and
  `git diff --check` gates.

The mutable report specifically passes
`sae_rejects_duplicate_exact_bm25_routes`,
`bm25_pending_seal_preserves_concurrent_active_l0_append`,
`bm25_contract_drift_requires_explicit_reindex`, and
`unified_delta_matches_compaction_and_reindex`. CSG-I116 through CSG-I119 are
closed without a second query, mutation, or maintenance authority.

### CSG-I105 closure slices

1. [x] Load a checked foreground context containing only the read root, bounded
   manifest, fixed query contract, and the two linked-L0 frontier descriptors.
   Reject flat term directories and never attach vocabulary, DF, payload,
   document, score-slot, or complete L0-record arrays.
2. [x] Replace one-term materialization with a checked posting cursor that opens
   one fold/extent block at a time. Query memory must not scale with a high-DF
   term.
3. [x] Score only touched document blocks with fixed scratch and a bounded top-k
   heap. Resolve candidate document records and length extrema lazily through
   document COW.
   - The immutable scorer now merges all query terms and all fold/raw runs in
     document order, retains one fixed 128-slot document block per global
     block, and uses an `O(query runs + k)` workspace.
   - A two-term 513-document probe examined 1,282 postings over two passes but
     opened only ten document blocks. A separate one-term/eight-run structural
     fold probe opened two document blocks. Both matched exact snapshot IDs and
     bit-identical float scores.
4. [x] Add query-specific linked-L0 token/admission and MVCC handling without
   rebuilding a complete dynamic vocabulary or score-slot overlay.
   - The foreground query context no longer materializes either L0 frontier.
     The isolated 64-gate v3 lifecycle suite preserved CRUD, transaction,
     rotation, restart, fold, and page-native parity.
   - One checked visitor now streams pending then active records while retaining
     at most one reconstructed record plus the format-bounded page inventory.
    A restart/oversized-record differential gate matched the maintenance
    snapshot record order, fields, payloads, pages, and bytes exactly.
    Query-specific MVCC/token projection now serves every public query shape.
5. [x] Persist or derive a bounded zero-score tie authority so exact
   `born_sequence` fallback does not scan every document slot.
6. [x] Route every ordinary v3 BM25 and SAE query shape through the page-native
   scorer. Keep full snapshots only for maintenance, scrub, diagnostics, and
   differential test oracles.
7. [x] Require exact v2/v3 parity plus increasing-corpus, increasing-DF, and
   multi-backend RSS gates before closing this item.

### CSG-I108 and CSG-I109 closure

1. [x] Document the PostgreSQL deadness-callback contract and exact
   `O(document slots + bounded L0)` discovery boundary.
2. [x] Publish globally-dead immutable and L0-owned versions through bounded
   document-COW patches without opening posting payloads or allocating a
   corpus bitmap.
3. [x] Reserve one frozen L0 sequence fence per VACUUM and keep it sealable
   after an injected error following a successful COW batch.
4. [x] Cross the 131,072-record historical L0 frontier with 140,000 documents,
   then prove repeat-VACUUM idempotence, convergence, restart, and subsequent
   insert/search/restart behavior.
5. [x] Preserve the complete PostgreSQL 18 convergent lifecycle at 80/80 gates.

## Decision Log

| Date | Decision | Reason |
| --- | --- | --- |
| 2026-08-01 | Retire globally dead document versions through bounded document-COW patches guarded by one frozen L0 sequence fence. | PostgreSQL has already established global deadness, so index cleanup is permanent even if the surrounding VACUUM command later errors. A frozen fence remains independently sealable, preserves ordering with linked L0, and prevents per-document retirement records from exhausting the bounded query frontier. |
| 2026-07-31 | Treat shared preload as a strict ownership boundary, not merely a warmup hint. | Model runtime and mutable unified state must be postmaster-owned; relation-owned v3 pages use PostgreSQL shared buffers; ordinary backends may retain only bounded workspace. A complete decoded v3 snapshot per backend defeats connection-pool safety even if its source pages were prewarmed. |
| 2026-07-31 | Reclaim accumulated retirement ranges through a thresholded identity-manifest transition on the clean maintenance path. | Real-model churn can drain all logical work before optional compaction consumes retirement metadata. A bounded root-only transition closes that storage lifecycle without adding a corpus scan, a second allocator, or foreground work. |
| 2026-07-31 | Interpret each linked-L0 frontier as a logical prefix of physically appendable pages. | Concurrent later writers may extend the old tail or link a complete chain. Old readers must validate physical suffixes without exposing them or rejecting a valid historical root. |
| 2026-07-31 | Supersede permanent score-slot uniqueness with root-relative safe reuse. | CSG-I91 bounded payload history yet fixed-live-set capacity still grew one COW leaf every 16 updates. A descendant root may reuse a numeric slot only after the removal horizon and zero active, pending, immutable, fold, retirement, and semantic references; old roots retain the old incarnation. This preserves snapshot correctness without making query workspace proportional to historical churn. |
| 2026-07-31 | Use a checked recyclable-page marker between manifest retirement and index-FSM reuse. | PostgreSQL's index FSM is reconstructible and may be stale after crash. A physical marker makes stale free hints rejectable, while lost hints cause only safe append fallback or bounded leakage. |
| 2026-07-31 | Reuse the existing relation `AccessExclusiveLock` transition fence as the Hot Standby drain point. | PostgreSQL WAL-logs relation AccessExclusive locks and recovery holds them through transaction completion. Handoff therefore needs no custom WAL resource manager or per-write relation fence when root publication and recyclable markers remain in the same transaction. |
| 2026-07-31 | Reuse interior pages only through a derived, reader-fenced COW arena. | The current manifest remains the sole authority. A nonblocking old-reader fence and root revalidation make its exact unreachable complement safe to overwrite, while append fallback preserves foreground availability and avoids a persistent free-list lifecycle. |
| 2026-07-31 | Keep mandatory pending seal append-only; reclaim interior pages only in compaction/fold or explicit maintenance. | Exact root inventory is optional storage reclamation and scales with global metadata. Coupling it to seal violated changed-set maintenance even though lookup and payload publication were bounded. |
| 2026-07-31 | Persist bounded retirement evidence from COW transitions rather than rediscovering routine garbage from a complete root scan. | Fixed-input compaction scales almost linearly with vocabulary solely because reuse preparation inventories the full current root. Manifest-authenticated hints preserve one storage authority while making normal reclamation proportional to changed objects. |
| 2026-07-30 | Keep workload fold optional; allow the same term-local representation as mandatory structural work. | Quiescent hot paths should recover peak locality, while a hard extent bound still needs a bounded escape hatch when whole-segment compaction is too large. |
| 2026-07-31 | Split an advancing fold into stable major, geometric minor, and bounded raw tail. | FIII-style grouping recovers locality without continuously rewriting the largest list. The current single-fold advance is exact but has the wrong sustained-write amplification; quiet hot keys may still collapse to one prewarmed major without a corpus-wide fold. |
| 2026-07-30 | Keep eventual semantic inference in the worker. | Foreground model inference would make writes unsuitable for dense update workloads. |
| 2026-07-30 | Use one global term directory and one score workspace. | Per-segment top-k would sacrifice the existing II-42 query advantage and complicate exactness. |
| 2026-07-30 | Make the logical global directory physically paged and copy-on-write. | A full flat-directory rewrite on every seal is still corpus-scale metadata work; stable leaves preserve one lookup while making publication proportional to changed terms. |
| 2026-07-30 | Store raw document frequency in term-directory leaves. | Per-term DF changes with touched posting keys; a manifest-wide array would retain hidden `O(vocabulary)` publication cost. |
| 2026-07-30 | Treat active, sealed, neutral-folded, and impact-specialized forms as read shapes of one logical stream. | Immediate visibility and quiescent peak performance can coexist only if every background transition is an exact COW representation change, not a second query path. |
| 2026-07-30 | Never infer missing document semantics during query execution. | Lexical-first visibility is the bounded fallback; document completion belongs to the worker, while only query encoding remains on the online model path. |
| 2026-07-30 | Fold posting keys, not query results. | Term/atom prefixes are reusable across queries, preserve additive scoring, and can be selected from bounded workload heat without creating another result-cache lifecycle. |
| 2026-07-30 | Inherit neutral term folds across ordinary writes. | Append-only events after a term-local coverage watermark form an exact tail; only contract changes or stale impact epochs require invalidation. |
| 2026-07-30 | Align term-fold coverage to immutable extent boundaries. | Exact fold-plus-tail reads need no per-posting sequence field only when no retained or replacement run straddles coverage; compaction must preserve that invariant. |
| 2026-07-30 | Store complete physical child refs inside COW directory nodes. | Logical ids alone need another global locator; bottom-up actual locators preserve bounded publication even when foreground L0 pages interleave. |
| 2026-07-30 | Use four-way geometric carry for ordinary equal-size L0 segments. | Repeated one-record rotations converged within four visible segments without touching the largest segment for every write; extent pressure remains a separate exceptional path. |
| 2026-07-30 | Compact hot-term extent pressure before the hard directory limit. | At six extents the worker merges the smallest bounded contiguous pair; unlike payload sizes do not need to wait for a tier match or force a complete-index rewrite. |
| 2026-07-30 | Separate neutral lexical facts from global BM25 statistics. | Without this, every statistics change requires whole-index impact rewriting. |
| 2026-07-30 | Treat current/experimental physical formats as rebuild-only. | The beta should not accumulate migration debt for unpublished storage formats. |
| 2026-07-30 | Keep score slots monotonic across ordinary maintenance. Superseded by CSG-I92 root-relative reuse. | Retiring a tuple version must not renumber unchanged postings or force a whole-index rewrite. |
| 2026-07-30 | Assign one score slot to one immutable heap tuple version. Superseded by CSG-I92 root-relative reuse. | Reusing a slot before reference evacuation would mix old and replacement term weights and break old-snapshot semantics. |
| 2026-07-30 | Store one tagged-by-extent four-byte value per posting. | Explicit run kinds preserve neutral lexical TF and direct semantic impacts without duplicating value arrays or branching per posting. |
| 2026-07-30 | Keep the active L0 frontier in the metapage read root, outside the immutable manifest. | Foreground O(row) appends must not republish a corpus metadata object; one metapage snapshot still binds a reader to an exact sealed and active view. |
| 2026-07-30 | Store complete object references for every manifest-owned derived surface. | A block range alone cannot prove object completeness, ownership, or checksum before recovery or attach. |
| 2026-07-30 | Persist query metadata separately from posting payloads. | Text lookup and BM25 parameters are required at read time, but retaining the legacy serialized posting matrix would duplicate index authority and storage. |
| 2026-07-30 | Reconstruct global index metadata rather than retaining the old matrix. | Segmented scoring needs params, vocabulary, document lengths, and global statistics, but posting ownership must remain exclusively in canonical payloads. |
| 2026-07-30 | Lock relation extension per immutable object, not for the complete COW bundle. | Exact object references and closure permit active-L0 pages to interleave while each object remains contiguous; the short final root switch preserves foreground concurrency. |
| 2026-07-30 | Persist the read root as a fixed checksummed blob. | Atomic publication must not depend on compiler layout, host padding, or a partially valid in-memory struct. |
| 2026-07-30 | Keep initial v3 publication behind a hidden superuser test switch. | Sealed native read parity can be qualified without exposing a storage version whose foreground CRUD path is not yet linked-L0 aware. |
| 2026-07-30 | Treat payload codec and page-envelope checksums as separate invariants. | The inner checksum detects payload-field corruption while the outer checksum proves exact immutable-object identity and page closure. |
| 2026-07-30 | Keep active text terms as normalized UTF-8 bytes until L0 sealing assigns global term ids. | Foreground mutation must make a previously unseen term searchable without rewriting immutable query metadata or moving tokenization into query attachment. |
| 2026-07-30 | Separate logical L0 mutation records from physical page fragments. | One changed row may exceed one page; corpus-independent foreground work requires a complete logical checksum and independently validated fragments rather than a hard row-size limit. |
| 2026-07-30 | Publish only complete logical L0 records. | Small records update tail and root atomically; large records become reachable only after every checked fragment exists, so readers never observe a partial record. |
| 2026-07-30 | Require contiguous logical sequence coverage inside each L0 frontier. | Exact `min..max` cardinality lets attachment prove that no logical mutation disappeared even when one record spans several pages. |
| 2026-07-30 | Keep retirement correction on the bounded dynamic path. | Query-term posting intersection gives exact live BM25 statistics without a corpus bitmap or posting rewrite; fold later removes the temporary read cost. |
| 2026-07-30 | Converge the hot working set rather than the complete corpus. | Workload fold recovers peak locality where queries pay for it while normal maintenance remains proportional to changed/index-hot data. |
| 2026-07-30 | Borrow segment durability from Tantivy, not per-segment search. | Immutable segments and similar-size merges solve ingestion; II-42 retains one global directory, score workspace, and top-k. |
| 2026-07-30 | Keep semantic completion ahead of all optional fold work. | Lexical visibility must remain immediate and document inference must remain outside foreground writes and missing-document query repair; normal query encoding remains online. |
| 2026-07-30 | Separate structural and workload convergence. | Correctness and fanout work must progress under writes; hot-term locality is a reconstructible optimization that may consume only residual capacity. |
| 2026-07-30 | Separate persistent fold from volatile warming. | Fold coverage is exact durable state; shared-cache residency is a bounded disposable latency optimization and can be prepared before the optional root switch. |
| 2026-07-30 | Track lexical visibility, semantic freshness, structural boundedness, and hot-path performance as independent progress clocks under one root. | Foreground availability and exactness must not wait for model throughput or optional locality work, while one manifest and one scorer prevent lifecycle divergence. |
| 2026-07-30 | Keep global corpus facts exact and delay only their faster read representation. | Manifest/COW/L0 state owns current BM25 facts; neutral scoring remains exact when writes invalidate an epoch-specialized hot fold. |
| 2026-07-30 | Use persistent hot folds as stable read surfaces, not as result caches. | FIII and dLSM support bounded colocation and stable hot views; query strings, result lists, and cache residency remain disposable non-authority state. |
| 2026-07-30 | Treat fold publication as a latency-only state transition. | A new COW root is selectable only after exact posting, score, tie-order, and top-k parity; workload heat may choose work but cannot authorize approximation. |
| 2026-07-30 | Track heat per posting key and act only in the worker. | HotRAP supports fine-grained hot-data retention, while II-42 must keep first-query latency free of synchronous fold or document inference. |
| 2026-07-30 | Store document-block bounds in the existing document/version COW summaries. | A separate document-block tree would duplicate update, publication, validation, recovery, and reclamation work. Subtree length summaries provide the same conservative range oracle while preserving one lifecycle. |
| 2026-07-30 | Recover static peak for the observed hot set, not every cold key. | FIII shows bounded colocation can approach contiguous performance; promising that for every unseen term would silently reintroduce a complete-corpus fold. |
| 2026-07-30 | Do not block lexical sealing on semantic inference. | Semantic freshness may lag, but model throughput must not consume the bounded active/pending frontier or stall foreground writers. |
| 2026-07-30 | Persist semantic completion as a version-state transition. | A transition can reference an older slot without duplicating or rewriting its immutable document-version owner. |
| 2026-07-30 | Make L0 rotation a short foreground-safe root switch, but keep sealing in the worker. | Crossing a hard append threshold must not perform corpus work; writes can continue in a new active chain while one immutable pending chain is sealed asynchronously. |
| 2026-07-30 | Keep immutable payload ownership with the manifest that created it. | Descendant manifests explicitly bind ancestor-owned payload object identities, allowing true copy-on-write publication instead of ownership-driven payload copying. |
| 2026-07-30 | Derive a sealing directory from prior metadata plus the new payload only. | A complete logical read directory remains atomic and query-efficient, while maintenance never reopens unchanged posting payloads merely to republish derived metadata. |
| 2026-07-30 | Preserve aborted slot reservations as immutable holes. | PostgreSQL transaction rollback must not force slot reuse, unbounded L0 retention, or a corpus-wide score-workspace renumbering. |
| 2026-07-30 | Replay immutable and linked-L0 retirements through one bounded set. | Clearing a sealed L0 frontier is safe only if its compact retirement facts remain query-visible without a corpus bitmap or historical posting scan. |
| 2026-07-30 | Treat tiered compaction as exact physical consolidation. | Compaction may change local document ids and extent layout, but it must not reinterpret posting values, remove MVCC facts, or read unselected payloads. |
| 2026-07-30 | Bound one normal compaction pass to eight segments and 64 MiB. | Pending sealing may need several resumable passes under severe fragmentation, but one worker action must never become an implicit corpus rewrite. |
| 2026-07-30 | Separate all-time exactness from working-set performance convergence. | A cold key cannot have a static contiguous first-hit cost without folding the complete corpus; the product instead guarantees bounded fragments and persistent hot-key compilation. |
| 2026-07-30 | Persist pruning metadata when a segment is already being sealed. | FIII-style local colocation and linked-block MaxScore evidence show that bounded fragments need safe block bounds as well as a fanout cap. |
| 2026-07-30 | Allow resource-separated maintenance overlap under one scheduler. | Semantic model work and immutable I/O work may use distinct headroom, but one policy authority and serialized root publication preserve a single lifecycle. |
| 2026-07-30 | Use one term-local fold mechanism for two admission classes. | Hard fanout needs a non-heat-dependent structural escape hatch when whole-segment input is too large; query heat uses the same exact representation only as an optional peak-performance optimization. |
| 2026-07-30 | Split stable query contract from immutable lexical catalog ranges. | New-term publication must be proportional to new token bytes and touched COW paths. Term records bind exact catalog refs, while one fixed scorer/model contract remains reusable across descendants. |

## Evidence Log

Add one row for every closed phase or issue.

| Date | Item | Command or artifact | Result |
| --- | --- | --- | --- |
| 2026-07-30 | CSG-0 milestone | `git show ii42-pre-convergent-segments-20260730^{}` | `9cdeb73149d878e8982f06b164c46a8e85904229` |
| 2026-07-30 | CSG-1 exactness | `ctest --test-dir build --output-on-failure` | Unit score and top-k parity passed. |
| 2026-07-30 | CSG-1 extent cost | `./build/ii42_extent_bench 250000 300 50` | Ratios for 1/2/4/8 extents: `0.9997x`, `0.9971x`, `1.0062x`, `1.0013x`; exact checksums. |
| 2026-07-30 | CSG-2 statistics change | `ctest --test-dir build --output-on-failure` | Old base postings plus a new extent exactly reproduced the expanded corpus; all five methods matched. |
| 2026-07-30 | CSG-2 neutral cost | `./build/ii42_extent_bench 250000 300 50` | Neutral contiguous `1.0966x`; neutral 8-extents `1.1051x`; impact specialization remains `1.00x`. |
| 2026-07-30 | CSG-3 format core | `ctest --test-dir build --output-on-failure` and PGXS build | Manifest round-trip and corruption/overlap/epoch validation passed in both core and extension builds. |
| 2026-07-30 | CSG-3 term directory | `ctest --test-dir build --output-on-failure`, PGXS build, and `git diff --check` | Little-endian directory round-trip passed; truncated/corrupt inputs, invalid segment/range references, and over-eight extent lists were rejected. |
| 2026-07-30 | CSG-5 zero-copy ids | `ctest --test-dir build --output-on-failure` and `./build/ii42_extent_bench 250000 300 50` | Attach-time range validation passed; impact 8-extents `1.0060x`, neutral 8-extents `1.1200x`. |
| 2026-07-30 | CSG-5 mixed read view | `ctest --test-dir build --output-on-failure` and `./build/ii42_extent_bench 250000 300 50` | Directory-driven base+new-segment replay matched a full rebuild; mixed 1/2/4/8-extents ratios to impact were `1.0826x`, `1.0917x`, `1.0880x`, and `1.0833x`, with exact checksums. |
| 2026-07-30 | CSG-3 canonical payload | `ctest --test-dir build --output-on-failure`, PGXS build, and corruption tests | Checksummed term runs, compact values, sparse slot maps, tuple-version records, and retirements round-tripped; corrupt/truncated payloads and invalid TF/impact values were rejected without mutating the prior view. |
| 2026-07-30 | CSG-3 sealed snapshot attach | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, and PG18 PGXS build | Query/scorer metadata and heap TIDs were reconstructed without posting duplication; every slot requires one immutable version owner, text lookup and segmented score parity passed, and native immutable loader assembly compiled. |
| 2026-07-30 | CSG-3 sealed bundle write | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and unpublished-directory unit case | Logical directory serialization no longer has a physical-ref cycle; the native writer emits objects in dependency order and validates one unpublished read root before AM publication. |
| 2026-07-30 | CSG-3 read-root codec | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, and PG18 PGXS build | Manifest ref, active/pending L0 frontiers, sequence, and high-water round-tripped through a fixed checksummed blob; corruption and truncation failed closed. |
| 2026-07-30 | CSG-3 metapage root read | PG18 PGXS build and `git diff --check` | V3 reads use only the checked root blob, reject relation high-water overflow, and fail closed when legacy physical fields coexist. V2 remains unchanged. |
| 2026-07-30 | CSG-5 sealed AM attach | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | Backend-local cache identity uses the checked root; sealed metadata, TIDs, and directory-backed mixed scoring attach without changing v2 DSM behavior. Nonempty linked L0 and legacy-matrix candidate paths fail closed pending native traversal. |
| 2026-07-30 | CSG-5 directory-native candidates | CMake build, PG18 PGXS build, and direct legacy-matrix reference audit | Ordered scans use canonical mixed scoring and filtered/boolean/phrase candidate generation traverses one legacy-or-segmented cursor. Remaining raw CSC reads are confined to guarded v2 sparse hot paths. |
| 2026-07-30 | CSG-I14 canonical hot loop | `./build/ii42_extent_bench 250000 300 50` | After extent-level dispatch, canonical 2/4/8-extent averages were `1.1305x`, `1.1302x`, and `1.1227x` impact baseline with exact checksums; qualification-host p95 remains open. |
| 2026-07-30 | CSG-3 page-chain envelope | `ctest --test-dir build --output-on-failure` and PGXS build | Exact page counts and terminal byte counts round-tripped; corrupt identity/checksum and malformed page boundaries failed closed. |
| 2026-07-30 | CSG-3 native object I/O | PG18 PGXS build | Relation-owned immutable chains use WAL-protected page appends, repeated ownership identity, exact range validation, and whole-object checksum verification; publication is intentionally not yet enabled. |
| 2026-07-30 | CSG-4 linked-L0 core | `ctest --test-dir build --output-on-failure` and PGXS build | Active page header/payload corruption failed closed; present, empty, absent, and over-limit frontier states were validated without adding a manifest-per-write dependency. |
| 2026-07-30 | CSG-3 read-root contract | `ctest --test-dir build --output-on-failure` and PGXS build | Manifest ownership/range, active/pending L0 ordering, next sequence, and published high-water mark validation passed; invalid cross-identity and out-of-range roots failed closed. |
| 2026-07-30 | CSG-3/5 opt-in native v3 publication | `python3 scripts/test_convergent_segment_read_smoke.py ...` | All 9 gates passed: v2/v3 text/token/ID/native-order parity, status, empty index, restart, contract drift, REINDEX recovery, fail-closed v3 mutation, and unchanged v2 mutation. |
| 2026-07-30 | Existing PG18 product regression | `python3 scripts/test_extension_regression_temp_pg.py ...` | `ii42_integration` passed against the staged library and SQL package; default v2 behavior did not change. |
| 2026-07-30 | CSG-4 L0 mutation codec | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | Numeric, UTF-8, and retirement records round-tripped with exact identity and whole-record checksums; malformed order/corruption failed closed; independent frame headers validated single- and multi-page record boundaries; read-root next-slot persistence passed. |
| 2026-07-30 | CSG-4 native L0 append | `python3 scripts/test_convergent_segment_read_smoke.py ...` | All 9 gates passed after native mutation: three text records occupied seven L0 pages, an oversized 2,000-term record crossed pages, the following small record reused its tail page, one numeric record published independently, restart preserved both frontiers, v2 mutation remained available, and query stayed fail-closed pending L0 traversal. |
| 2026-07-30 | CSG-4 checked L0 traversal | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and the 9-gate convergent smoke | Traversal proved page identity/order, cycle freedom, frame boundaries, whole-record checksums, exact sequence coverage, logical byte counts, and restart stability. |
| 2026-07-30 | CSG-4/5 insert-only L0 query overlay | `/tmp/ii42-v3-l0-overlay-smoke.json` from `scripts/test_convergent_segment_read_smoke.py` | All 9 gates passed. Sealed plus L0 text and numeric queries matched v2 scores exactly after restart; a 2,000-term cross-page record and a following tail-page append were both searchable through one mixed read view and one top-k. |
| 2026-07-30 | CSG-4/5 v3 retirement overlay | `/tmp/ii42-v3-crud-retirement-smoke.json` from `scripts/test_convergent_segment_read_smoke.py` | All 10 gates passed. Text and numeric UPDATE/DELETE plus `VACUUM (INDEX_CLEANUP ON)` matched v2 native `ORDER BY` rows and scores, retained the sealed manifest identity, and remained exact after restart. Core retirement scoring also matched a live-only full rebuild. |
| 2026-07-30 | CSG-4 transaction boundaries | `/tmp/ii42-v3-transaction-boundaries-smoke.json` from `scripts/test_convergent_segment_read_smoke.py` | All 12 gates passed. Current-transaction L0 rows were visible only to their owner; rollback, savepoint rollback, prepared-transaction invisibility, `COMMIT PREPARED`, and repeat-VACUUM idempotence were exact. |
| 2026-07-30 | CSG-4 active-to-pending rotation | `/tmp/ii42-v3-l0-rotation-smoke.json` from `scripts/test_convergent_segment_read_smoke.py` | All 13 gates passed. Ten old active records atomically became pending, a fresh active accepted two writes while pending remained unsealed, both frontiers were query-visible through one scorer, and restart preserved exact frontier identity. |
| 2026-07-30 | CSG-I37 COW payload ownership | Core unit tests, PG18 PGXS build, and `/tmp/ii42-v3-cow-ownership-smoke.json` | Manifest v6 round-tripped ancestor payload ownership, rejected future owners, and validated exact page identities. All 13 isolated lifecycle gates passed without rewriting or weakening the v2 route. |
| 2026-07-30 | CSG-I38 incremental term directory | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | The next directory built from one prior checked directory plus one new payload matched the full two-payload rebuild exactly; unchanged payload views were neither required nor read. |
| 2026-07-30 | CSG-I39 aborted slot holes | Core unit tests, PG18 PGXS build, and `git diff --check` | Strict hole records round-tripped, occupied their zero-length metadata slot, and caused posting references to fail closed. The PG18 TID-map build explicitly skips the hole flag. |
| 2026-07-30 | CSG-I41 immutable retirement replay | Core unit tests, PG18 PGXS build, `git diff --check`, and `/tmp/ii42-v3-immutable-retirement-read-smoke.json` | Frozen retirement records were globally sorted and deduplicated without reading posting runs; unsafe records failed closed. Sealed attach now validates live statistics, and all 13 isolated CRUD, MVCC, rotation, restart, and parity gates passed. |
| 2026-07-30 | CSG-I45 COW append writer | CMake build, core tests, PG18 PGXS build, and `git diff --check` | The writer preflights ancestor closure and the incremental directory before page I/O, retains all ancestor payload references, writes one pending payload plus replacement metadata, and leaves the metapage switch to a latest-root compare-and-publish step. |
| 2026-07-30 | CSG-I46 retirement statistics | Core tests, PG18 PGXS build, `git diff --check`, and `/tmp/ii42-v3-retirement-length-smoke.json` | L0 record v2 persisted exact retired-version length, mixed replay checked it against document metadata, and all 13 isolated CRUD, MVCC, restart, parity, and rotation gates passed. |
| 2026-07-30 | CSG-I47 raw/live DF contract | Core tests, PG18 PGXS build, `git diff --check`, and `/tmp/ii42-v3-raw-df-retirement-smoke.json` | Manifest validation accepted raw posting DF above live `N` only within the monotonic slot bound; mixed scoring rejected any retirement-corrected live DF above visible `N`; all isolated lifecycle gates passed. |
| 2026-07-30 | CSG-I48 self-contained retirements | Core tests, PG18 PGXS build, `git diff --check`, and isolated v3 lifecycle smoke | Segment payload v2 round-tripped exact retired length in a fixed 32-byte record, rejected nonzero reserved state, and retained rebuild-only storage semantics. |
| 2026-07-30 | CSG-I49 pending seal publication | Core tests, PG18 PGXS build, `git diff --check`, and `/tmp/ii42-v3-pending-seal-postfix.json` | The worker safely classified pending XIDs, compiled only that frontier, reused ancestor payloads, extended the vocabulary without invalidating immutable payloads, and atomically published a second sealed segment while preserving the two-record active L0. All 13 CRUD, MVCC, 2PC, rotation, restart, and query gates passed. |
| 2026-07-30 | CSG-I50 exact selective merge core | `cmake --build build --parallel 8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | Two selected payloads consolidated without opening an unselected payload path. The oracle preserved lexical-neutral, semantic-impact, lexical-impact, aborted-hole, and retirement data exactly; directory replacement reduced selected extents while keeping one global read directory. |
| 2026-07-30 | CSG-I51 bounded replacement COW | Core root-transition tests, PG18 PGXS build, and `git diff --check` | A bounded loader opens only the selected descriptor range. The writer reuses unselected payload/query-contract objects and emits only replacement payload, directory, and manifest objects; physical-only publication preserves concurrent active and immutable pending L0 frontiers exactly. |
| 2026-07-30 | CSG-I52 native selective compaction | `/tmp/ii42-v3-selective-compaction.json` from the 14-gate PG18 lifecycle smoke | Four sealed segments with 66,848 input bytes became one 66,508-byte payload. Query rows/scores, 13-document statistics, active and pending L0 frontiers were exact before/after publication and restart; CRUD, MVCC, 2PC, retirement, and v2 parity gates remained green. |
| 2026-07-30 | CSG-I64 physical COW term pages | `/tmp/ii42-cow-physical-smoke.json`, core tests, PG18 PGXS build, and `git diff --check` | Seven real relation-owned COW objects were written bottom-up. A nonempty term matched the flat-directory DF/extent oracle through one bounded radix path before and after restart. All 18 convergent v3 parity, CRUD, MVCC, rotation, compaction, and restart gates passed. |
| 2026-07-30 | CSG-I53 sustained geometric convergence | `/tmp/ii42-v3-sustained-compaction.json` from the 15-gate PG18 lifecycle smoke | Thirty-two forced rotations exercised ten bounded tier compactions. Visible immutable segments never exceeded four and converged to two; all 33 rows, exact v2 scores, pending drain, and restart identity passed. |
| 2026-07-30 | CSG-I55 non-uniform extent pressure | `/tmp/ii42-v3-extent-pressure.json` from the 16-gate PG18 lifecycle smoke | Payloads generated from 16 through 100,000 numeric atoms reached six visible segments without a same-tier four-way match. The worker selected two adjacent segments and read 960 bytes, reduced fanout to five, and preserved all seven v2 query rows/scores plus restart identity. |
| 2026-07-30 | CSG-I54 linked-L0 debt observability | `/tmp/ii42-v3-linked-debt-status.json`, core tests, and isolated `ii42_integration` | Public status, details, and raw cache diagnostics agreed on exact linked-L0 records, bytes, upserts, and retirements. CRUD exposed four upserts plus two retirements; active one-record frontiers survived compaction and restart while all 16 native gates remained green. |
| 2026-07-30 | CSG-I57 semantic L0 state codec | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | L0 record v3 round-tripped exact completion impacts and bounded quarantine state, rejected invalid impact/order/fingerprint/retry shapes, checksum corruption, and lexical/semantic decoder confusion. |
| 2026-07-30 | CSG-I57 immutable semantic transitions | `cmake --build build -j8`, `ctest --test-dir build --output-on-failure`, PG18 PGXS build, and `git diff --check` | Manifest v7 and payload v3 round-tripped fixed-width completion state. A semantic-only payload referenced an older owner slot, and selective merge produced one slot, one owner, and one latest transition while rejecting fingerprint drift. |
| 2026-07-30 | CSG-3B COW term-map core | Clean CMake build, core tests, and ASan/UBSan core tests | Fixed 16-term leaves and sparse radix paths preserved prior roots, updated only three touched leaves plus 15 nodes, matched the flat append/DF oracle exactly, and rejected checksum corruption. Physical relation publication remains open. |
| 2026-07-30 | CSG-I58/I62/I66 bounded COW maintenance | `/tmp/ii42-cow-v8-bounded-maintenance-smoke.json`, core and ASan/UBSan tests, PG18 PGXS build, `py_compile`, and `git diff --check` | Manifest v8 pending seal and selective replacement path-copied only touched externally stored COW leaves/root paths, kept raw DF in leaves, and avoided a flat-directory or manifest-wide DF rebuild. All 18 native parity, CRUD, MVCC, rotation, compaction, pressure, restart, and sustained-convergence gates passed. |
| 2026-07-30 | CSG-I59 append-only lexical catalog | `/tmp/ii42-cow-v9-lexical-catalog-smoke.json`, core plus ASan/UBSan tests, PG18 PGXS build, staged `ii42_integration`, `py_compile`, and `git diff --check` | All 20 native gates passed. A fixed 96-byte query contract retained the same physical ref and checksum while vocabulary grew from 28 to 2,039 terms. One new 2,011-term catalog was appended, unseen-term queries survived restart, and selective compaction preserved all catalog identities exactly. The codec rejects noncanonical ranges, while COW append rejects a missing descendant catalog only for an existing lexical-catalog lineage and keeps numeric vocabulary growth valid. |
| 2026-07-30 | CSG-I60/I67 bounded-tail term fold | `/tmp/ii42-convergent-fold-tail-advance-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | A term-local fold published through one COW leaf/root path, removed covered source extents from that term's read authority without changing raw DF, and preserved exact native rows/scores through restart. Advancing from the prior fold while loading only one touched tail payload matched a direct full-prefix fold byte-for-byte; all 21 isolated native lifecycle gates passed. |
| 2026-07-30 | CSG-I72 fold-aware COW compaction | `/tmp/ii42-convergent-fold-aware-compaction-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | A real fold patch followed by a two-segment replacement retained the fold, raw DF, and zero covered extents while all other terms used the replacement segment. A replacement containing the folded term on both sides of its watermark failed before publication. All 21 isolated native lifecycle gates remained green. |
| 2026-07-30 | CSG-I70 bounded structural-fold worker | `/tmp/ii42-convergent-structural-term-fold-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | With whole-segment compaction disabled only through a superuser test setting, normal maintenance selected the pressured COW term twice, consumed one complete immutable extent per action, and left five source extents. Every action stayed within one segment and 256 input bytes, a later tail sealed normally, v2/v3 rows and scores remained exact after restart, and all 23 isolated native lifecycle gates passed. |
| 2026-07-30 | CSG-I72 fold-safe compaction selection | `/tmp/ii42-convergent-fold-safe-compaction-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | A normal four-segment tier candidate crossed one inherited neutral-fold watermark. Touched-leaf preflight rejected it before merge, selected a two-segment 512-byte tail-only subrange, and published through the regular worker. Exact v2/v3 rows and scores, the concurrent active L0, and restart all passed; all 25 isolated native lifecycle gates remained green. |
| 2026-07-30 | CSG-I73 conflict-targeted fold-forward | `/tmp/ii42-convergent-fold-forward-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | A forced two-segment tier candidate had no legal compaction subrange because its first segment was folded and its second was tail. The worker retained the conflict term and boundary, advanced one 256-byte term-local tail, and then compacted the original 512-byte pair on the next pass. The active L0 remained query-visible, v2/v3 rows and scores were exact, restart parity passed, and all 27 isolated native lifecycle gates remained green. |
| 2026-07-30 | CSG-I32 manifest v9 document COW publication | `/tmp/ii42-doc-cow-manifest-v9-smoke.json`, core tests, PG18 PGXS build, and `git diff --check` | Initial publication wrote one checked document/version radix closure; ordinary append loaded only affected old slots and path-copied touched objects; fold and compaction retained the ancestor-owned root. Full attach/restart validated recursive closure and live-count parity. CRUD retirement, active/pending rotation, seal, fold, compaction, restart, and sustained writes passed all 27 isolated native gates. Semantic maintenance consumption remains open under CSG-I57. |
| 2026-07-30 | CSG-I57 lexical-first pending seal | `/tmp/ii42-lexical-first-seal-smoke.json`, core tests, PG18 PGXS build, and `git diff --check` | Convergent eventual-SAE foreground UPSERT records a model-input fingerprint. Safe pending seal no longer waits for inference: committed versions become semantic-pending in the same document COW publication and the lexical segment carries pending state. Realtime convergent-SAE writes fail closed, while all 27 BM25 v3 CRUD, MVCC, rotation, seal, fold, compaction, restart, and convergence gates remained green. Initial v3 semantic publication and worker transition consumption remain open. |
| 2026-07-30 | CSG-I57 native SAE lifecycle | `/tmp/ii42-convergent-sae-lifecycle.json` from `scripts/test_convergent_sae_lifecycle_smoke.py` | All 8 gates passed through the public native route with a real model contract. Fresh v3 build was complete; INSERT and indexed UPDATE were lexically visible before inference; worker completion changed semantic scores; DELETE was hidden immediately; VACUUM retirements sealed to three live documents; and cold restart preserved exact rows, scores, and converged status. |
| 2026-07-30 | CSG-I68/I75 global document-block bound oracle | Core unit tests, CMake build, and `ctest --test-dir build --output-on-failure` | Global document-slot blocks produced conservative lexical bounds for all five BM25 methods and both query-weight signs. Direct semantic and lexical-impact extrema bounded signed contributions, while non-monotonic mapped runs failed closed. No runtime pruning or payload-format claim is made yet. |
| 2026-07-30 | CSG-I68/I75 persistent posting-block metadata | `/tmp/ii42-convergent-segment-block-metadata.json`, fresh core and ASan/UBSan tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | Query contract v3 fixed one global block shift; segment payload v4 and fold v2 persisted canonical checked block ids, posting ranges, TF extrema, and signed-impact extrema. Corruption failed closed, attached read extents retained block slices, and all 27 native CRUD, rotation, seal, fold, compaction, extent-pressure, and restart gates passed. PGXS now rebuilds every extension object after any product-header change. The global pruning scheduler and document-block length directory remain open. |
| 2026-07-30 | CSG-I68/I75/I76 global block-max native scoring | `/tmp/ii42-convergent-blockmax-smoke-isolated.json`, `/tmp/ii42-convergent-sae-blockmax-smoke-isolated.json`, core tests, clean PG18 PGXS build, and `git diff --check` | Document COW v2 added checked subtree length summaries and bulk contract-block derivation without a second metadata tree. Linked L0 builds transient posting blocks and extends document extrema. One global scheduler combines folded prefix, immutable tail, L0, lexical-neutral, lexical-impact, semantic-impact, signed query weights, retirements, and visibility catch-up through one heap. A focused core case proved actual block skipping with exact top-k; missing optional metadata falls back to exact scoring. All 27 BM25 and 8 SAE native lifecycle gates passed. |
| 2026-07-31 | CSG-I34/I77 workload heat and native COW fold | `/tmp/ii42-convergent-posting-heat-batching.json`, `/tmp/ii42-convergent-sae-posting-heat-batching.json`, isolated `ii42_integration`, fresh core ASan/UBSan tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | A bounded backend-local table and decayed shared heavy-hitter table drive root-stable optional fold admission without correctness dependency. A 70-key query caused zero shared flushes through query 31 and one flush at query 32; its 64 retained observations plus bounded set-associative replacements were exact. A hot three-extent term published one COW fold, reduced two physical surfaces, preserved v2/v3 rows and scores, rejected stale-root heat, and survived restart. All 30 BM25, 8 real-model SAE lifecycle, and default v2 extension regression gates passed. macOS ASan/UBSan passed with leak detection disabled because Apple ASan does not support `detect_leaks=1`. |
| 2026-07-31 | CSG-I32 document-COW VACUUM | `/tmp/ii42-document-cow-vacuum-smoke.json`, core tests, clean PG18 PGXS build, and `git diff --check` | Convergent VACUUM stopped materializing sealed posting payloads. It enumerated immutable TIDs through checked document COW leaves, merged active/pending L0 retirement state, appended only new RETIRE records, and preserved v2/v3 CRUD, repeat-VACUUM idempotence, compaction, restart, and query parity across all 44 native gates. |
| 2026-07-31 | CSG-I40 safe snapshot horizon | `/tmp/ii42-old-snapshot-horizon-smoke.json`, `py_compile`, and `git diff --check` | A repeatable-read reader retained its old snapshot while a concurrent committed INSERT became visible to a new reader. Normal maintenance rotated the active L0 but returned `xid_horizon` instead of freezing the pending transaction outcome; after the old reader ended, the same pending frontier sealed and exact query parity resumed. All 45 native lifecycle gates passed. |
| 2026-07-31 | CSG-I57 stale and HOT semantic publication | `/tmp/ii42-semantic-hot-race.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | Model output is staged before publication and revalidated through PostgreSQL's visible tuple chain plus the exact input fingerprint. A concurrent indexed UPDATE during an injected post-inference pause produced `semantic_completed=0` and `semantic_retired=1`; an empty terminal transition drained the replaced immutable slot. A concurrent unindexed HOT update changed CTID without emitting an L0 UPSERT and retained the valid completion with `semantic_completed=1` and `semantic_retired=0`. Both current versions converged with zero semantic debt, and all 10 real-model lifecycle gates passed. |
| 2026-07-31 | CSG-I57 post-append crash recovery | `/tmp/ii42-semantic-crash-recovery.json`, PG18 PGXS build, `py_compile`, and `git diff --check` | PostgreSQL was stopped in immediate mode after semantic completion had appended its L0 transition but before the maintenance transaction committed. Recovery rejected the aborted transition and retained the original exact `sealed_pending=1` debt. A normal retry changed the target score from lexical-only `2.269779` to semantic-complete `60.355831`, drained pending work to zero, and all 11 real-model lifecycle gates passed. |
| 2026-07-31 | CSG-I6/I16/I65 exact reachability and trailing cleanup | `/tmp/ii42-csg6-reachability-smoke.json`, `/tmp/ii42-convergent-sae-reachability-smoke.json`, fresh core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | A root-derived inventory covered manifest-owned objects, recursive ancestor-owned term/document COW closures, embedded fold/catalog refs, payloads, semantic segments, and linked L0 pages. Exact duplicate refs deduplicated and partial overlap failed closed. A native reader forced nonblocking `reader_fence_busy`; after it exited, the worker truncated exactly the unpublished suffix without changing rows/scores, and restart retained zero tail debt. Interleaved interior orphans were counted but not reused. All 33 isolated BM25 lifecycle and 8 real-model SAE lifecycle gates passed. |
| 2026-07-31 | CSG-I6/I16 reader-fenced interior reuse | `/tmp/ii42-reuse-crash-smoke.json`, `/tmp/ii42-reuse-sae-smoke.json`, fresh core ASan/UBSan tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | The exact reachability complement fed a best-fit per-maintenance allocator. A live old reader made maintenance append with `reused_blocks=0`; after release, the next seal reused 14 interior blocks. Reused pages used Generic WAL full-page images, and an immediate-stop crash recovery preserved exact queries and reachability. All 35 isolated BM25 lifecycle and 8 real-model lexical-first SAE lifecycle gates passed. |
| 2026-07-31 | CSG-7 native hot-path matrix | `scripts/benchmark_convergent_query_states.py` and `docs/performance/data/diagnostics/convergent-query-states-2026-07-31/` | Three independent 40,000-document PG18 clusters returned identical ids and scores in static, three-segment, and workload-folded states. Three-extent p50/p95 medians were `1.034x`/`1.042x` static; folded paired-static p50/p95/QPS medians were `1.003x`/`1.004x`/`1.000x`. Every fold consumed three hot-term extents, reduced two query surfaces, and reused 49 unreachable blocks without corpus compaction. Qualification-host and mixed/cold matrices remain open. |
| 2026-07-31 | CSG-I36/I78/I81 foreground headroom and background fold | `/tmp/ii42-convergent-scheduler-headroom-smoke.json` from `scripts/test_convergent_segment_read_smoke.py` | All 36 gates passed. Empty numeric v3 creation no longer crashes; one pending L0 retained three exact active records in reserved headroom; workload heat alone woke the worker and published one exact persistent fold; restart, COW, CRUD, MVCC, reclamation, and v2 parity remained green. |
| 2026-07-31 | CSG-I80 sealed semantic debt discovery | `/tmp/ii42-convergent-sae-scheduler-smoke.json` from `scripts/test_convergent_sae_lifecycle_smoke.py` | All 8 real-model gates passed. INSERT was lexical-first, lexical sealing cleared both L0 frontiers, one background touch rediscovered the remaining semantic-pending document through the COW summary, and completion converged without foreground inference or a corpus scan. |
| 2026-07-31 | CSG-I79/I82 unified maintenance authority | `/tmp/ii42-convergent-unified-scheduler-smoke.json` and `/tmp/ii42-convergent-unified-scheduler-sae-smoke.json` | All 37 native gates and all 8 real-model SAE gates passed. With structural and heat-only debt present, public `maintain_due(1)` first rotated the mandatory frontier; after it converged, the same C entrypoint published the exact heat fold. The worker independently rediscovered heat-only and sealed semantic COW debt, preserving lexical-first visibility, CRUD, restart, and exact rows/scores. |
| 2026-07-31 | CSG-I83 standby replay preload | `scripts/test_shared_preload_standby_auto_preload.py --timeout 20` | The initial and WAL-replayed replacement generations both became shared-resident. Recovery catalog reconciliation ran twice, the replacement was visible in about seven seconds, and all explicit plus automatic durable-maintenance routes remained no-ops. |
| 2026-07-31 | CSG-I84 runtime oracle boundary | `scripts/test_runtime_service_privilege_smoke.py` | The ordinary application role received the owner/superuser boundary before semantic payload access, while owner-visible runtime service and query behavior remained valid. |
| 2026-07-31 | CSG-I85 SAE output-mode separation | `scripts/test_runtime_service_privilege_smoke.py` and `/tmp/ii42-convergent-post-output-split-sae-smoke.json` | Legacy v2 unified publication passed the runtime privilege smoke. Convergent v3 passed all eight real-model lexical-first, completion, CRUD, VACUUM, and restart gates with `all_gates_passed=true`. |
| 2026-07-31 | CSG-I86 dual-coverage COW foundation | Core unit tests, PG18 PGXS build, isolated `ii42_integration`, and `git diff --check` | COW term-map v6 persisted separate major/minor fold references and complete-extent coverage. A two-generation major-then-minor round trip removed the covered raw tail, cold-attached both exact intervals through one global scorer, included both objects in reachability, rejected a minor without a major, and matched the unfurled score oracle. Worker carry, promotion, prewarm, and sustained-write qualification remain open. |
| 2026-07-31 | CSG-I86 geometric worker closure | `/tmp/ii42-dual-fold-write-amplification-smoke.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | All 38 native gates passed. A one-posting tail left the 240-byte minor unchanged; 328 new posting bytes then carried it to 568 bytes. The 568-byte minor qualified against a 592-byte major, and zero-extent promotion produced one 936-byte major. Every intermediate state matched v2 rows/scores, survived restart, and retained the existing 32-rotation geometric segment-convergence proof. Publication read back the checked fold before its root switch. |
| 2026-07-31 | CSG-I86 converged performance gate | `scripts/benchmark_convergent_query_states.py` and `docs/performance/data/diagnostics/convergent-query-states-2026-07-31/run-v2-dual-fold.json` | The v2 40,000-document paired matrix made converged performance blocking. Static and folded top-100 ids/scores were identical. Folded/static mean, p50, p95, p99, and QPS ratios were `0.987x`, `0.980x`, `0.958x`, `1.019x`, and `1.013x`; all declared equivalence gates passed. |
| 2026-07-31 | Dual-fold SAE lifecycle regression | `/tmp/ii42-dual-fold-convergent-sae-smoke.json` from `scripts/test_convergent_sae_lifecycle_smoke.py` | All eight real-model gates passed on the same staged binary. Lexical-first INSERT/UPDATE, asynchronous semantic completion, DELETE MVCC, VACUUM retirement convergence, and cold-restart score identity remained intact. |
| 2026-07-31 | CSG-I7 epoch-bound impact specialization | `/tmp/ii42-impact-fastpath-native-smoke-v2.json`, core tests, PG18 PGXS build, `py_compile`, and `git diff --check` | All 44 native gates passed. Root 36 selected an exact impact image at coverage 89/epoch 15; active L0 immediately disabled it. Bounded neutral fold, promotion, and impact COW actions rebuilt coverage 91 at epoch 17. VACUUM retirement disabled it again; sealed retirement advanced epoch 18 and rebuilt an eligible image with one retired slot. Cache clear and immediate-stop recovery retained exact TIDs/scores and the same selected root 46 image. |
| 2026-07-31 | CSG-I10 materialized-impact peak closure | `scripts/benchmark_convergent_query_states.py`, `run-v3-impact-40k.json`, and `run-v3-impact-250k.json` | Both committed-source paired matrices passed every exactness and performance gate. Impact-specialized versus legacy materialized mean/p50/p95/p99/QPS ratios were `0.976/0.978/0.973/0.980/1.024` at 40k and `0.977/0.968/1.007/0.991/1.023` at 250k. Maximum score difference was `0.0`; the 250k run covered five actual segments created by bounded byte pressure. |
| 2026-07-31 | CSG-I71 sparse pending-seal payload | `/tmp/ii42-csg71-sparse-seal-smoke-fresh.json`, `/tmp/ii42-csg71-convergent-sae-lifecycle.json`, focused headroom replay, core tests, PG18 PGXS build, and `git diff --check` | Pending seal no longer clones the full contract or builds vocabulary-sized DF/IDF/offset arrays. A one-million-term core case built three postings and two canonical runs with allocations proportional to changed entries. The fresh native BM25 lifecycle passed 45 gates without server errors, the real-model lexical-first SAE lifecycle passed all 11 CRUD/HOT/crash/VACUUM/restart gates, and a focused reserved-headroom replay passed. Cold text catalog reverse-map reconstruction remains CSG-I71 work. |
| 2026-07-31 | CSG-I71 cold reverse-map measurement | `/tmp/ii42-csg71-cold-text-seal.json` from `scripts/benchmark_convergent_text_seal.py` | Three cold repetitions per vocabulary size gave median pending-seal times of 9.2 ms at 2,000 terms, 86.7 ms at 20,000 terms, and 452.9 ms at 100,000 terms. The monotonic vocabulary-shaped cost remains after sparse payload construction, so a manifest-owned reconstructible COW lexical lookup is required before CSG-I71 can close. |
| 2026-07-31 | CSG-I71 COW lexical-lookup core | Core unit tests, ASan/UBSan core tests, PG18 PGXS build, and `git diff --check` | A packed 64-way hash trie mapped exact normalized bytes to contiguous stable ids without one-object-per-term overhead. Three-term suffix admission retained the complete prior root, read at most one bucket per changed key, wrote only changed buckets/shared paths, distinguished colliding radix routes by full bytes, round-tripped the canonical codec, and rejected checksum corruption. A separate 100,000-term case admitted two terms while reading at most two buckets/two radix paths and writing under 512 KiB. Manifest/page publication and the native cold-seal replacement remain open. |
| 2026-07-31 | CSG-I71 restart-safe lexical lookup | Core unit tests, fresh ASan/UBSan tests, PG18 PGXS build, clang static analysis, and `git diff --check` | The COW lookup codec now embeds complete child page locators plus inner and outer checksums and prepares/binds objects bottom-up. A fake immutable page store freed the source trees, cold-loaded and recursively validated the old root, built a suffix patch by reading only touched paths, published a new owner-scoped root, retained the old root, and rejected payload/ref corruption. Manifest v10, native relation-page publication, reachability, and cold-seal cutover remain open. |
| 2026-07-31 | CSG-I71 manifest v10 lexical root | Core unit tests, manifest round-trip/corruption tests, PG18 PGXS build, and `git diff --check` | Manifest v10 appends one checked lexical-lookup root ref and immutable nonzero hash seed without moving prior fields. Validation covers flag/ref/seed agreement, kind/owner, every manifest-owned range, payload overlap, published high watermark, and the manifest checksum. Numeric or not-yet-migrated manifests retain an explicitly absent root/zero seed. Native relation-page publication and cold-seal cutover remain open. |
| 2026-07-31 | CSG-I71 native lexical-lookup publication | `/tmp/ii42-csg71-lexicon-native-fresh.json`, core tests, PG18 PGXS build, and `git diff --check` | Fresh text publication wrote a checked lookup closure into the index relation. New-term seal path-copied only suffix paths; no-term append, fold, and compaction retained the prior immutable root and seed. Recursive reachability covered lookup descendants. All 45 native CRUD, rotation, fold, compaction, reader-fence, reclamation, restart, and REINDEX gates passed. Cold worker lookup cutover and the fixed-vocabulary scaling rerun remain open. |
| 2026-07-31 | CSG-I71 bounded cold seal closure | `/tmp/ii42-csg71-bounded-text-seal-exact.json`, `/tmp/ii42-csg71-bounded-seal-lifecycle-exact.json`, and `/tmp/ii42-csg71-bounded-seal-sae-lifecycle-exact.json` | Pending seal resolves only unique changed terms through the native lookup and publishes append-only. With two changed terms, 2k/20k/100k vocabulary medians were 1.29/1.51/1.93 ms. All 45 native lifecycle gates and all 11 real-model SAE lexical-first/completion/HOT/crash/VACUUM/restart gates passed. Reader-fenced compaction reused zero blocks under an old reader and ten after release. |
| 2026-07-31 | CSG-I88 optional-reclamation scaling | `/tmp/ii42-csg88-reclamation-scaling.json` from `scripts/benchmark_convergent_reclamation.py` | Three cold repetitions compacted the same four small segments with exactly 1,664 input bytes and two changed terms per segment. Median latency grew from 5.74 ms at 2,000 existing terms to 63.83 ms at 20,000 and 285.88 ms at 100,000, while result cardinality remained exact and 11-12 interior blocks were reused. The remaining cost is whole-root reachability discovery, so routine compaction requires bounded manifest-authenticated retirement evidence. |
| 2026-07-31 | CSG-I88 bounded retirement evidence | `/tmp/ii42-csg88-retirement-scaling.json`, `/tmp/ii42-csg88-retirement-native-v3.json`, and `/tmp/ii42-csg88-retirement-sae-v2.json` | Manifest v11 carries a checked bounded retirement set, and routine optional maintenance builds its best-fit allocator directly from that set rather than a root inventory. Fixed-input medians became 1.27/1.27/1.56 ms at 2k/20k/100k vocabulary, every run retained five exact results and reused four blocks, all 46 native lifecycle gates passed, and all 11 real-model SAE lifecycle gates passed. |
| 2026-07-31 | CSG-I89 exact transition retirement | `/tmp/ii42-csg89-fixed-churn-toggle.json`, `/tmp/ii42-csg89-sae.json`, core tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | Term, document, and lexical-lookup COW patches return their exact superseded objects; pending seal also returns every consumed linked-L0 page. The strengthened native lifecycle passed 48/48 gates, including exact `14/14` and `118/118` hint/interior equality. Real-model SAE lifecycle remained 11/11. A true 24-cycle four-row UPDATE/VACUUM matrix stayed v2 TID/score exact and ended at `591/591` retired/unreachable blocks. |
| 2026-07-31 | CSG-I90 fixed-live-set growth diagnosis | `/tmp/ii42-csg89-fixed-churn-toggle.json` | Complete retirement evidence did not produce physical convergence: the published high-water mark grew from 40 to 608 blocks while the reachable live set stayed between 16 and 19 blocks. Sixteen bounded compactions each reused seven blocks, proving that optional COW reuse works but foreground linked-L0 and mandatory append-only seal remain outside the reusable-page allocator. Range capacity is not the blocker: the final exact evidence occupied only four of 64 ranges. |
| 2026-07-31 | CSG-I90/I91 page reuse and bounded history closure | `/tmp/ii42-csg91-history-barrier-final.json`, core tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | Full native lifecycle passed 50/50. Reader-fenced FSM handoff and marker validation reduced 48-cycle growth from 608 to 79 blocks. COW-authoritative attach plus bounded retirement filtering kept alternating compaction inputs near 1,248 and 1,888-1,976 bytes. Delete-all compaction emitted an empty history barrier, remained query-ready, accepted a new active-L0 row, converged, and survived immediate-stop restart. Remaining HWM steps at cycles 16/31/46 exactly match permanent document-slot allocation and are isolated to CSG-I92. |
| 2026-07-31 | CSG-I92/I94 root-relative slot reuse and bounded foreground claim | `/tmp/ii42-slot-cursor-smoke.json`, core tests, clean PG18 PGXS build, `py_compile`, and `git diff --check` | Full native lifecycle passed 52/52. Four live rows completed 48 UPDATE/VACUUM/convergence cycles with exact v2/v3 TIDs and scores. Slot HWM reached seven by cycle two and page HWM reached 82 by cycle four; both remained flat through cycle 47. The persisted cursor was zero after every converged cycle, while source inspection confirms the allocator no longer loads either complete L0 frontier. Restart preserved the current incarnations. |
| 2026-07-31 | CSG-I93 semantic compare-and-append guard | `/tmp/ii42-slot-cursor-sae-smoke.json`, `/tmp/ii42-slot-cursor-transactional.json`, and core COW lookup gates | The guarded writer compares the complete source record under the append lock through at most six immutable COW object loads. All 11 real-model lexical-first/HOT/stale/crash/VACUUM/restart gates and all 21 commit/rollback prepared, savepoint, old-snapshot, reclamation, and eventual semantic transaction gates passed. |
| 2026-07-31 | Convergent semantic frontier performance | `/tmp/ii42-slot-cursor-convergent-frontier.json` | Explicit convergent v3 runs at 64, 256, and 1,024 pending rows passed exact pending/final full-overlay oracle comparison and drained to zero. The 1,024-row run used four fixed 256-row semantic batches, survived restart, and recorded zero frontier scans. The final reusable-slot cursor was zero. Corpus-independent COW lookup work is enforced separately by the six-object core gate. |
| 2026-07-31 | CSG-I96 bounded zero-score tie order | `/tmp/ii42-tiebreak-segment-smoke.json`, `/tmp/ii42-tiebreak-sae-smoke.json`, `/tmp/ii42-tiebreak-transactional.json`, `/tmp/ii42-tiebreak-semantic-frontier.json`, and core block-max instrumentation | A generation-local live-slot order preserves `born_sequence` tie semantics without scanning retired slot history. The focused `k=3` gate examined three ordered candidates instead of the fallback path's eight score slots. Native lifecycle passed 52/52, real-model SAE lifecycle passed 11/11, transaction/savepoint/2PC/crash-restart passed 21/21, and convergent semantic frontier levels 64/256/1,024 remained oracle-exact, drained to zero, survived restart, and recorded zero frontier scans. |
| 2026-07-31 | CSG-I22/I97 default-v3 and INIT closure | `/tmp/ii42-v3-default-segment-smoke-durable-root.json` and `/tmp/ii42-v3-default-init-unlogged-durable-root.json` | Fresh builds use v3 linked-L0 while legacy fixtures remain v2 until explicit `REINDEX`. All 55 native lifecycle gates passed. Empty logged indexes were query-ready, UNLOGGED BM25/SAE crash restore retained v3 and the model contract, and first post-crash mutations used linked-L0; all seven INIT lifecycle gates passed. |
| 2026-07-31 | CSG-I98 durable maintenance root | `/tmp/ii42-v3-default-transactional-durable-root-clean.json`, `/tmp/ii42-v3-default-segment-smoke-durable-root.json`, `/tmp/ii42-v3-default-sae-lifecycle-durable-root.json`, and `/tmp/ii42-v3-default-init-unlogged-durable-root.json` | Internal maintenance locks remain transaction-scoped and successful maintenance flushes only its final physical root record. Immediate-stop recovery retained the exact compacted generation with zero active/pending L0 debt. Transaction/MVCC/VACUUM/2PC passed 21/21, native v3 passed 55/55, real-model SAE passed 11/11, and empty/UNLOGGED INIT passed 7/7 without a `pageinspect` dependency. |
| 2026-07-31 | CSG-I99 linked-L0 logical-prefix closure | `/tmp/ii42-v3-linked-prefix-lexical-1x1x1.json` and `/tmp/ii42-v3-final-linked-prefix-reclaim-8x8x12.json` | A native reader paused after root decode while a concurrent writer first extended the same physical page (`1 -> 1`) and then linked an oversized successor chain (`1 -> 8`). Each paused reader returned exactly its pre-append rows; a new reader observed the new row. Cleanup returned zero delta debt and the exact pre-fixture query set. The gate runs on BM25 eventual storage to isolate root semantics from semantic completion; the same binary then passed real-model SAE concurrency. |
| 2026-07-31 | CSG-I100 clean-state reclamation closure | `/tmp/ii42-v3-final-linked-prefix-reclaim-8x8x12.json`, `/tmp/ii42-v3-final-transactional.json`, `/tmp/ii42-v3-final-segment-read.json`, `/tmp/ii42-v3-final-sae-lifecycle.json`, and `/tmp/ii42-v3-final-empty-unlogged.json` | Reader-fenced identity publication, individual-page linked-L0 reuse, and bounded fragmented-run FSM claims closed the no-pending retirement leak. Under 8 writers, 8 readers, maintenance, and VACUUM, 288/288 mutations and 329 reads passed with native/oracle parity, zero runtime failures, and zero busy rejections. All 12 second-half samples of a 24-cycle fixed-representation real-model churn stayed at exactly 1,418 pages and 11,616,256 bytes. Fresh-source qualification passed core 1/1, transaction 21/21, native 55/55, SAE 11/11, and UNLOGGED/INIT 7/7. |
| 2026-07-31 | CSG-I101 retirement-statistics contract | `/tmp/ii42-v3-retirement-contract-smoke-v2.json` from `scripts/test_convergent_segment_read_smoke.py` | All 57 local native gates passed. The UPDATE/DELETE fixture preserved the exact live business-ID set before VACUUM while measuring maximum score drift of `0.0340861082` for `alpha` and `0.7205848694` for `omega`. After `VACUUM (INDEX_CLEANUP ON)`, both terms matched a fresh REINDEX oracle exactly in IDs, rank, and float scores. Status reported `retirement_statistics = vacuum_convergent`; no heap-wide query overlay was introduced. |
| 2026-07-31 | CSG-I102/I103 manual-cache and v3 regression closure | `/tmp/ii42-v3-regression-modernized-final/` from `scripts/test_extension_regression_temp_pg.py` | The manual stale notice now runs before either backend-cache return while preserving old-root queryability. The complete isolated PG18 integration suite passed `1/1` after replacing v2 assumptions with linked-L0 visibility, staged bounded maintenance, exact scheduler debt, business-ID results, and post-VACUUM score assertions. The expected snapshot was accepted only after every new logical gate returned true and no unexpected SQL or runtime error remained. |
| 2026-07-31 | CSG-I104 preload-tier closure | `/tmp/ii42-v3-post-outage-preload-lifecycle-final.log`, `/tmp/ii42-v3-post-outage-auto-preload.log`, `scripts/test_shared_preload_generation_cache.py`, and `/tmp/ii42-v3-post-outage-generation-rollout.log` | A 203,202,560-byte v3 root exceeded the 1 MiB II-42 arena, prewarmed through `tier=postgres_buffer_cache`, retained one eight-byte exact-root marker, and recorded zero DSM admission misses. Marked v3 roots warmed in priority order without unmarked registry pollution. Explicit test-only legacy v2 fixtures separately passed decoded-payload pressure eviction, active-reference pinning, and obsolete-generation reclamation. |
| 2026-07-31 | CSG-I105 bounded context, posting cursor, immutable scorer, and L0 stream | `/tmp/ii42-csg-i105-l0-stream.json`, fresh core build, PG18 PGXS build, `py_compile`, and scoped `git diff --check` | A foreground context loads only the checked root, bounded manifest, fixed query contract, and linked-L0 descriptors without vocabulary, DF, document, payload, score, or complete L0 arrays. A two-term probe examined 1,282 postings over two passes while opening ten fixed 128-slot document blocks; a structural-fold probe merged eight immutable runs for one term while opening two document blocks. Both used an `O(query runs + k)` workspace and matched full-snapshot top-k IDs plus float scores exactly. The common linked-L0 parser now either collects maintenance records or streams one transient record; after restart, an oversized cross-page record plus two inline records matched snapshot order, fields, payloads, page count, and bytes exactly. All 65 native lifecycle gates passed. Query-specific L0 projection, zero-score authority, full-route cutover, and multi-backend RSS closure remain open. |
| 2026-08-01 | CSG-I107 ordered prefix closure | Core prefix-COW tests and `/tmp/ii42-final-bounded-text-seal-20260801.json` | A 20k-key zero-match seek loads at most tree depth; external suffix path-copy preserves the prior root and the 47/48/49-child split regression passes. At 2k/20k/100k vocabulary, two-term seal medians were 1.44/1.77/2.05 ms with 432-460 logical bytes, 23-27 physical pages, and 190,544-223,656 WAL bytes per seal. |
| 2026-08-01 | Final lifecycle and concurrency matrix | `/tmp/ii42-final-prefix-fix-segment-read-guard-a-20260801.json`, `/tmp/ii42-final-prefix-fix-segment-read-guard-b-20260801.json`, `/tmp/ii42-final-prefix-fix-sae-lifecycle-20260801.json`, `/tmp/ii42-final-prefix-fix-transactional-20260801.json`, `/tmp/ii42-final-prefix-fix-concurrency-20260801.json`, and `/tmp/ii42-final-prefix-fix-fairness-20260801.json` | Two simultaneous native runs passed 80/80 each; real-model SAE passed 11/11; transaction/restart passed 21/21; 8 writers plus 8 readers completed all 288 mutations with zero failures/busy rejections; four SAE indexes plus realtime BM25 met fairness and restart-reconciliation gates. |
| 2026-08-01 | Final recovery, replication, and memory matrix | `/tmp/ii42-final-prefix-fix-empty-unlogged-20260801.json`, `/tmp/ii42-final-prefix-fix-replication-20260801.json`, `/tmp/ii42-final-prefix-fix-standby-preload-20260801.json`, and `/tmp/ii42-final-prefix-fix-backend-rss-20260801.json` | Empty/UNLOGGED passed 7/7; primary/standby replay covered L0, semantic completion, 2PC, CRUD, REINDEX, and BM25/SAE conversion; standby exact-root preload converged. The 5k-to-80k backend delta retained zero II-42 context bytes, 12 KiB malloc, and 5.625 MiB private-live writable memory, below the 16 MiB gate, with shared-root markers present. |
| 2026-08-01 | Final package and migration closure | `/tmp/ii42-final-prefix-fix-regression-20260801`, `/tmp/ii42-final-prefix-fix-source-migration-20260801.json`, `scripts/test_extension_schema_smoke.py`, and `scripts/test_product_convergence_inventory.py` | Isolated PG18 regression passed 1/1; staged `psql_bm25s` source-table migration retained side-by-side scores, CRUD parity, rollback, and source rows; non-public schema and product inventory gates passed. |
| 2026-08-01 | CSG-I123 post-overflow corruption closure | Commit `1d21a0e7`, `/private/tmp/ii42-csg-i124-model-8x8x24-v2.json`, `/private/tmp/ii42-csg-i124-segment-smoke-v2-rerun.json`, `/private/tmp/ii42-csg-i124-mutable.json`, `/private/tmp/ii42-csg-i124-transaction.json`, `/private/tmp/ii42-csg-i124-vacuum-frontier.json`, and `/private/tmp/ii42-csg-i124-replication.json` | The staged artifact retained reader-fence ownership and same-born retirement identity under mixed reuse pressure. Exact real-model 8x8x24 concurrency, native 80/80, mutable 71/71, transaction 21/21, 140k frontier 10/10, restart/reuse, fixed-live storage plateau, and physical replication all passed with zero unexplained debt. |
| 2026-08-01 | CSG-I124 lossless retirement overflow closure | Commits `b0aa6ffd` and `1d21a0e7`, `/private/tmp/ii42-csg-i124-segment-smoke-v2-rerun.json`, `/private/tmp/ii42-csg-i124-model-8x8x24-v2.json`, and `/private/tmp/ii42-csg-i124-query-states-40k.json` | Exact in-memory preflight prevents side effects before a non-fenced overflow; the conditional fenced retry publishes once and hands every new plus unused prior range to checked FSM markers through a dynamically sized result. Greater-than-64 accounting, restart/reuse, 24-cycle storage plateau, all 22 exact 40k query/performance gates, and the neighboring lifecycle matrices passed without a new persistent authority. |
