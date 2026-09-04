# Archived Product Roadmap Through v0.2.5

Status: closed historical engineering plan.

This file preserves the detailed implementation and rollout ledger that led
to the qualified v0.2.5 three-environment state. It is evidence, not the
current planning authority. Current work is tracked in
[`docs/product-roadmap.md`](../../product-roadmap.md).

## Archive Guide

Preserve the [release evidence](#historical-release-evidence),
[correctness audit](#closed-audit-csg-beta-correctness-1),
[planner-native implementation](#completed-plan-csg-beta-planner-native-1),
and [Commons query ledger](#closed-plan-csg-beta-commons-query-1). These bind
engineering decisions and measurements to their original revisions.

Unchecked tasks and the deferred-work section are historical dispositions,
not a second live backlog. The candidate, host roles, binaries, temporary
resources, and rollout commands below are not current operating instructions.
Use [Convergent Segmented Index](../../convergent-segmented-index.md) for the
implemented architecture and the [archive index](README.md) for related evidence.

## Historical Release Candidate: v0.2.5-rc1

The candidate recorded for this rollout was tag `v0.2.5-rc1`, source commit
`cd77b2977c983761f55d3d0dccfd037e04a59104`, with API 29 and ONNX Runtime
1.29.0. The recorded Shadow package and both database nodes used the same
candidate binary. At this point the remaining work was bounded rollout
qualification, not new query or storage research.

The generic product default remains `f32` block64 with
`semantic_alpha_mass = 1.0`. The current Commons rollout deliberately uses
`u8` with `semantic_alpha_mass = 0.50` for all nine product roots. That is an
explicit approximate deployment policy, not a hidden II42 default or a
dataset-name rule in the extension.

Current closure gates:

- [ ] Rebuild all nine Commons roots with the current writer while preserving
  keys, `INCLUDE` columns, predicates, field-aware options, and preload
  priorities.
- [ ] Verify 9/9 valid, ready, query-ready roots and the expected runtime,
  block64, U8, and alpha contracts without a heavyweight full-root status scan.
- [ ] Pass primary and standby natural/explicit query smoke, representative
  arXiv and PubMed filtered queries, replication catch-up, cancellation, RSS,
  and regression-level latency checks.
- [ ] Rebuild or remove remaining benchmark/calibration roots separately;
  preserve their declared purpose rather than silently applying the Commons
  approximate profile.
- [ ] Restore normal maintenance/preload convergence, remove temporary rollout
  storage only after qualification, and record the final package, catalog,
  root, runtime, and replication evidence.

Do not restart a healthy rebuild, treat a completed heap scan as publication,
or reopen speculative performance tuning during this closure. The release is
complete only after publication, swap, catalog validation, query qualification,
and standby replay all pass.

Everything below this section is retained evidence or deferred post-beta work.
Unchecked boxes in a historical section are not a second active plan.

## Historical Release Evidence

- [x] Freeze a clean candidate commit containing the bundled milestone model
  contract, package integration, and current product documentation.
- [x] Build an immutable staged package and verify its binary, SQL, control,
  model checkout, source-migration package, and provenance fingerprint.
- [x] Run the complete product maturity suite on that exact package, including
  exact lifecycle, 2PC, VACUUM, concurrent CRUD, restart, replication, RSS,
  storage plateau, 50k model lifecycle, and the current benchmark gate.
- [x] Record the clean commit and package fingerprint in release evidence. Do
  not reuse the qualification of an older binary.

The historical ORT 1.26 comparison package is bound to clean commit
`f1553ad3f0daa65c2ae1bb22695290b0a6652bba`. Its archive SHA-256 is
`cf9b9789d1878c085d9cd1e8008cee8148db9f94617438ba752fcf7e27d8be66`,
and the maturity preflight/postflight fingerprint is
`bb8918fcb6dfa16ed1b3078c4e721b5ff268128440d2542a93658726f30771c4`.
The clean package passed all 41 lifecycle, package-binding, and benchmark
steps, including the 50,000-document production-model gate and the
75,000-document, 96-client product-path benchmark, in
`/Volumes/Betty/Tmp/ii42-product-maturity-f1553ad3-full.json`. All 96 clients
completed, with semantic query p50 621 ms and p95 973 ms, zero runtime
failures, zero busy rejections, and zero batch-parity mismatches. This closes
the immutable-package and local maturity gates. It does not substitute for the
Commons full-root matrix, host package binding, or CQ-7/CQ-8 release gates.

ORT 1.26 remains historical comparison evidence. The active beta candidate
now pins ONNX Runtime 1.29.0 as its sole CPU runtime on Linux x64, Linux
aarch64, and macOS arm64. The candidate must pass a fresh immutable-package
suite and Shadow full-root qualification; evidence collected with 1.26 is not
silently promoted. Official 1.29 packages may include POSIX telemetry, so II42
disables it by default before runtime initialization. The Spark CUDA 13/SM121
runtime requires a separately built and checksum-qualified 1.29 aarch64 GPU
artifact because upstream does not publish that platform combination.

The Linux x86_64 PostgreSQL 18 current-only package built from `852e28f1`
completed its bounded Shadow closure on 2026-08-26. Its package SHA-256 is
`8c36c8a84a23313f8120e8b32227bdcd1fbd0308367cd0969fd7dc9f1d8829c0`;
the installed binary reports API 29 and linked ORT 1.29.0. All 14 Shadow root
generations remained unchanged, and pre/post ArXiv query signatures were
identical. Scope v2-v5 and accelerator policy 5-6 query readers are removed.
See
[Shadow Current-Only ORT 1.29 Closure](../../performance/reports/csg-beta-current-only-ort129-shadow-2026-08-26.md).
This closes package identity and reader cleanup only; CQ-4/CQ-7 latency,
concurrency, cancellation, and full-root RSS gates remain open.

## Closed Audit: CSG-BETA-CORRECTNESS-1

Status: completed on 2026-08-26 without changing product query policy.

The audit froze the query routes and performance results at source baseline
`810374f7`, then reviewed the current root, mutation, maintenance, runtime,
preload, failure, and package boundaries. No route threshold, cache size,
scorer, format, or publication policy changed. The only new regression closes
the combined structured-filter and `REPEATABLE READ` metadata-visibility gap.

Current source passes build/core tests, the complete Python suite, extension
regression, convergent BM25/SAE lifecycle, transactional/2PC, concurrent CRUD,
DDL/rewrite, VACUUM frontier, crash/restart, runtime cancellation/restart,
shared preload, corruption fail-closed, schema/privilege, and physical
replication gates. The evidence summary and local artifact paths are recorded in
`analysis/reports/beta-correctness-stability-audit-2026-08-26.md`.

This is a source-level correctness and stability closure, not beta release
qualification. A clean immutable ORT 1.29 package and stable full-root Shadow
qualification remain separate release evidence. The stopped Commons
performance plan below remains closed; this audit does not reopen performance
micro-tuning.

### Deferred Next-Version Item: QDIR-RSS-1

- [ ] Qualify and, only if necessary, reduce query-local semantic accelerator
  directory memory on a production-sized root. Directory v10 currently loads
  one `uint32` forward-row offset for each document plus each forward chunk,
  or approximately `4 * (document_count + forward_chunk_count)` bytes per
  concurrent query. The allocation is bounded and released at query cleanup;
  it is not a leak. The open risk is aggregate backend RSS under full-root
  concurrency. Measure exact same-root result parity, cancellation cleanup,
  backend RSS/PSS plateaus, and concurrent throughput before changing the
  representation. If the cost is material, prefer a shared root-identity-keyed
  directory or bounded page-local/lazy offsets inside the existing accelerator
  lifecycle. Do not create another artifact authority, retain index-sized
  backend state, or reopen route-threshold tuning.

## Completed Plan: CSG-BETA-PLANNER-NATIVE-1

Status: engineering implementation and isolated local/Elm qualification
completed on 2026-08-26. The implementation is included in the active release
candidate; current full-root promotion is tracked only by the rc1 closure
gates above.

### Objective And Architecture

Restore natural PostgreSQL ranked SQL without adding another root, index,
publication artifact, worker, or reclamation lifecycle:

1. BM25-only indexes keep the existing ordinary ordered `IndexScan`.
2. All SAE ranking, including single-column, multicolumn field-aware, and
   filtered top-k, uses one planner-visible `CustomPath`/`CustomScan` over the
   existing II42 root and scorer.

The product query shape is:

```sql
SELECT d.*
FROM documents AS d
WHERE d.publish_date >= DATE '2024-01-01'
  AND d.categories && ARRAY['cs.LG']
ORDER BY ii42_query(
    'documents_search_idx'::regclass,
    'graph neural networks'
) DESC
LIMIT 20;
```

PostgreSQL evaluates the ordinary relation predicate as a child plan that
projects only visible TIDs. The custom executor encodes once, normalizes that
membership, and invokes the same exact page-native filtered scorer. Structured
JSON remains a compatibility route, not the primary SQL interface.

An ordinary semantic `IndexScan` was tested and rejected: PostgreSQL also
evaluates its ordering expression in the scan projection, which would require
row re-encoding or unsafe backend score state. BM25 remains safe because its
ordering operator has a real row-local scalar definition.

### Invariants

- One root, COW/L0 frontier, accelerator publication, worker scheduler, WAL
  authority, retirement path, and reclamation lifecycle.
- One query encoding per scan; no candidate-row model invocation.
- Statement-snapshot predicate membership, visibility, and heap fetches.
- Exact subset top-k, stable score/tie ordering, and fail-closed unsupported
  plans.
- No application-side post-filtering and no serialized SQL predicate strings.
- No BM25 route, result, hot latency, lifecycle, or storage regression.
- The isolated implementation phase made no deployment, restart, rebuild, or
  configuration change on Shadow.

### Completed Local Evidence

- [x] Prove and reject the unsafe ordinary semantic `IndexScan` design.
- [x] Implement scalar `ii42_query` markers that fail closed unless
  the II42 custom executor owns the query.
- [x] Implement single-column and field-aware execution with explicit weights,
  prepared parameters, generic plans, aliases, and `LIMIT` handling.
- [x] Push ordinary PostgreSQL relation predicates through a cheapest child
  plan that projects only `ctid`; do not parse or serialize SQL.
- [x] Preserve relation/index dependencies, permissions, statement snapshots,
  `ctid`, `tableoid`, rescan, cleanup, and child executor ownership.
- [x] Reject unsupported joins, row-dependent queries, secondary sort keys,
  ascending order, unbounded queries, row locking, `WITH TIES`, RLS, and
  non-SAE indexes rather than changing semantics.
- [x] Add `EXPLAIN` route identity and actual allowed/hit counters.
- [x] Pass staged PG18 runtime, app-role, field-aware, CRUD/eventual lifecycle,
  and exact function-oracle checks on the milestone model.
- [x] Pass a 4,096-document selectivity ladder. Allowed rows and scorer work
  moved together at 4, 49, 496, 2,096, and 4,096 documents; natural SQL p50
  remained about 23.9--30.1 ms and every TID/order/`real` score matched the
  current function route.
- [x] Pass the complete local static and staged candidate gates: PG18 build,
  CMake unit tests, `438 passed, 1 skipped` Python contracts, convergence
  inventory, schema regression, 71/71 unified lifecycle, 64-client mixed
  planner/function concurrency, cancellation recovery, restart, and
  `git diff --check`.
- [x] Preserve the BM25-only route. The unchanged 20,000-document benchmark
  retained ordinary index execution with roughly 25.5--29.6 ms hot p50 across
  scalar and field-aware cases.
- [x] Pass an isolated Elm staged smoke using the current ORT 1.29 runtime,
  including natural SQL, exact function-oracle parity, field-aware execution,
  64-client mixed concurrency, cancellation recovery, and lifecycle cleanup.
- [x] Pass the Elm 20,000-document selectivity ladder. At 20, 200, 2,000,
  10,000, and 20,000 allowed rows, natural SQL p50 was approximately 77.6,
  75.2, 80.0, 99.5, and 123.4 ms. Scored blocks increased with membership
  from 20 to 1,250, and all result TIDs and score bytes matched the explicit
  `ii42_query` oracle.
- [x] Quantify query-local membership memory. The front-end canonical TID set
  uses 8 bytes per allowed row. End-to-end trace memory on Elm increased from
  about 0.79 MiB at 20 allowed rows to 1.21 MiB at 20,000, an incremental
  slope of about 21 bytes per allowed row including scorer bitmap and scratch.

### Historical Rollout Gates

These were the pre-rc1 promotion conditions. The active rc1 closure at the top
of this file supersedes their environment snapshot.

- [x] Publish natural SQL, field-aware, prepared-query, fail-closed shape, and
  JSON-compatibility documentation.
- [ ] Before production promotion, run the same matrix on one full Commons
  root and measure broad-filter concurrency RSS. The current exact child plan
  materializes membership in query-local memory; extrapolating the measured
  incremental slope to a nearly unfiltered 7.8-million-row predicate is about
  160 MiB per active query and therefore requires an explicit concurrency
  gate. This is bounded statement memory, not a leak.
- [ ] Validate cold and warm p50/p95 against the current full-root explicit
  function route before changing application defaults. Do not deploy this
  branch to Shadow until that rollout gate is deliberately opened.

The implementation phase is complete because local and Elm matrices are exact,
physical scorer work decreases with selective filters, BM25 behavior is
unchanged, and no lifecycle split was introduced. Full-root promotion remains
closed until its RSS and latency gates pass. Stop rather than add a second
index/filter authority if either rollout gate fails.

## Closed Plan: CSG-BETA-COMMONS-QUERY-1

Status: stopped without completion on 2026-08-26 by product decision.

No item below is active work. Completed items and their evidence are retained;
unchecked items remain unachieved rather than waived. The authoritative
closing inventory, performance matrix, rejected branches, retained research
candidate, and multi-host deployment snapshot are recorded in
`docs/performance/reports/csg-beta-commons-query-1-closure-2026-08-26.md`.

The 2026-08-26 Shadow physical-cost ledger supersedes earlier route-threshold
experiments. The running current root is healthy and exact for the qualified
date-category subset, but the current contract-10 matrix still fails two
latency rows: unfiltered PubMed is about 443 ms against a 300 ms gate, and
date-category is about 500--531 ms against a 500 ms gate. Date-journal,
partial-date, and broad-date pass. The complete evidence and phase-separated
costs are recorded in
`docs/performance/reports/cq3-shadow-physical-cost-ledger-2026-08-26.md`.

No further route ratio, cardinality threshold, cache size, or scorer parameter
change is admitted. The directory-only max-query-term checkpoint oracle has
now failed on current FIQA, 500K PubMed, and full PubMed roots: its maximum
physical saving is only 0.75--1.25% of forward bytes and about 0.11--0.12% of
term work. That selective branch is closed rather than implemented. Broad
search may proceed only through the exact essential-term decomposition already
validated on 500K and full PubMed. Any replacement selective representation
and the broad executor both require forced same-root physical A/B before an
automatic product policy changes.

### Objective

Qualify the current unified II42 index for every query shape used by Commons.
Filtered search must compute exact top-k inside the predicate-defined subset,
remain bounded on selective and broad predicates, and preserve the existing
single-root lifecycle. The work ends with one current-format PubMed index under
the stable product name and a reproducible Shadow qualification matrix.

This plan does not permit application-side post-filtering, a parallel metadata
index lifecycle, query-specific approximation, or a second semantic scorer.
Existing PostgreSQL indexes and the scope metadata published in the II42 root
may be used, but II42 remains the only ranking authority.

### Core Goal And Beta Closure

The stopped goal used a deliberately narrower core release definition than the
original three-host rollout:

> Produce one current-only II42 beta candidate whose package, catalog, roots,
> query behavior, and lifecycle are all qualified on Shadow. Remove
> intermediate II42 format compatibility only after that Shadow gate passes.
> Elm and macOS then rebuild directly from the cleaned package and do not block
> the Shadow source cleanup.

Shadow is beta-ready only when all of the following are true. A completed
heap scan, a ready candidate, or a passing local suite is not sufficient on
its own.

- [x] `CQ-3E` completes publication on the package-bound full PubMed root. The
  combined finalizer must accept both memory reports, prove zero swap and the
  32 GiB process-family RSS ceiling, verify the candidate is present, valid,
  and ready, and pass the tracked five-query runtime-migration fixture. The
  same-input C oracle remains the exact streaming-writer authority.
- [ ] Shadow is installed from the exact qualified ORT 1.29 0.2.5 package.
  Its unsupported beta catalog and roots are recreated from source through the
  reviewed current-only plan; no adjacent II42 upgrade script is retained.
  Every declared product root must use the current writer, scope v6 where
  scope exists, accelerator policy 7, and one current generation.
- [ ] `CQ-4`, `CQ-5`, and `CQ-7` pass on those stable Shadow roots: cold,
  first-admitted, warm, preload, restart, planner/statistics, unfiltered,
  selective-filter, broad-filter, cancellation, concurrency, exact
  membership, latency, physical-work, and RSS evidence must all be recorded.
- [ ] Stable product names pass the Commons matrix before promotion. Keep one
  rollback root until final evidence is written; no candidate or maintenance
  task may remain ambiguous at closure.
- [x] Remove scope v2-v5 readers and fixtures and accelerator policies 5-6.
  Preserve only current BM25/SAE modes and the direct `psql_bm25s` to II42
  rebuild boundary.
- [ ] Remove the synthetic fixed-32-extent compatibility contract after its
  separate current-leaf audit. A legitimate variable-width current leaf whose
  actual maximum is 32 must remain valid.
- [ ] Build a clean current-only package and rerun codec, mutable lifecycle,
  CRUD/2PC/VACUUM, restart, physical replication, cancellation/RSS, package
  binding, and Shadow Commons smoke gates against that exact artifact.

When these gates pass, record **Shadow beta-ready** and end architecture work
for this milestone. Elm and macOS rollout, documentation refresh, and removal
of per-host temporary artifacts remain bounded release operations; they are
not permission to reopen query architecture or add another format. Any new
failure must map to one of the gates above or be deferred explicitly.

### Unfinished Critical Path (Frozen)

1. Install the qualified package on Shadow, recreate the current catalog and
   roots, and verify all generation identities before any product swap.
2. Run the `CQ-4`, `CQ-5`, and `CQ-7` Shadow matrices and promote stable names
   only if correctness, bounded work, latency, cancellation, and RSS pass.
3. Remove intermediate format compatibility, build the current-only package,
   and repeat the full release and Shadow smoke gates.
4. Rebuild Elm and macOS directly from that cleaned package, update release
   documentation, and remove host-local transition artifacts.

The work stops immediately before promotion on any exact-result mismatch,
memory-limit violation, swap use, timeout, unbounded physical work, stale
generation, package/catalog mismatch, failed cancellation, or unexplained
active maintenance state. Do not compensate by weakening exact filtered
top-k or adding another persistent lifecycle.

### Observed Shadow Baseline

The following measurements were collected on AWS Shadow with II42 0.2.4,
current semantic accelerators, and warm query state unless noted otherwise.
They are routing evidence for this plan, not release qualification.

| Query shape | Observed latency | Result |
| --- | ---: | --- |
| arXiv unfiltered field-aware top-50 | about 132 ms p50 | usable |
| arXiv date plus category | about 259 ms | exact, usable |
| arXiv broad date range | about 776 ms | exact, bounded |
| arXiv Stanford organization | about 4.7 s | too slow |
| arXiv broad organization | over 15 s | timeout |
| PubMed candidate unfiltered field-aware top-50 | about 225 ms p50 | usable |
| PubMed date plus journal | about 445 ms p50 | exact, usable |
| PubMed broad date range | about 662 ms | exact, bounded |
| PubMed partial-date interval | over 30 s | timeout |
| PubMed date plus category | over 20 s | timeout |
| TX complete policy candidate union | about 119 ms | usable |
| WA complete policy candidate union | about 200 ms | usable |
| CA complete policy candidate union, warm | about 227 ms | usable |
| ordinary chunk-in-document | about 57-151 ms warm | exact, usable |
| CA document with about 39k chunks | over 20 s | timeout |

Cold observations ranged from several seconds to more than 20 seconds even
when the resident fold and accelerator reported ready. The 112 GiB shared arena
was almost full and could not admit the roughly 24 GiB CA chunk index. The
Commons stable PubMed index name was absent; only
`data_pubmed__title_abstract__current_next_idx` existed. The relevant Commons
tables also had no current PostgreSQL `ANALYZE` statistics.

### Frozen Execution Record

#### CQ-0: Freeze the benchmark and route evidence

- [x] Add one repeatable, read-only Commons query matrix runner. It must record
  extension/catalog identity, index generation, accelerator/source-manifest
  identity, residency, query route telemetry, hit count, violations, latency,
  timeout, and active backend cleanup.
- [x] Cover arXiv, PubMed, CA/TX/WA policy documents, policy chunks, and system
  chunks using the exact Commons field names, field weights, filter JSON, and
  candidate limits.
- [ ] Separate first admitted query, cold-after-restart, and warm steady-state
  measurements. Never report a warmed retry as cold performance.
- [ ] Preserve a SQL oracle for every filter and compare membership before
  optimizing the scorer.

Completion gate: the runner reproduces every baseline row above without
leaving active sessions and can identify native scope, SQL residual, forward
row, and posting/block-intersection routes.

#### CQ-1: Make filter planning observable and compositional

- [x] Expose per-query telemetry for scope predicates resolved natively,
  predicates delegated to an SQL residual, allowed-document cardinality,
  selected scoring route, blocks considered/skipped, and forward rows read.
- [x] Preserve each supported scope bitmap when another predicate needs SQL
  residual evaluation. One unsupported predicate must not discard useful date,
  equality, or overlap scope information.
- [x] Make the two PubMed partial-date ranges intersect as native ordered scope
  constraints before any residual check.
- [x] Prove the filter plan is independent of JSON key order and returns the
  same allowed set as the SQL oracle.

Completion gate: partial-date planning produces a bounded intersection and the
telemetry explains every fallback. No ranking behavior changes in this step.

#### CQ-2: Repair array and text filter candidate construction

- [x] Replace the PubMed category path that currently expands
  `unnest(categories)` across the heap. Prefer current-root scope metadata; use
  an indexable SQL residual only when native scope cannot represent the exact
  operation.
- [x] Apply the same rule to arXiv organizations and PubMed journal/category
  `ilike_any`: candidate generation must use a native scope or an expression
  that matches the deployed PostgreSQL index, followed by an exact residual.
- [x] Keep array element semantics exact. Concatenated text may generate
  candidates, but it cannot be the final membership authority when it changes
  element boundaries.
- [x] Add selective, medium, broad, empty, NULL, and Unicode cases. Include
  multiple patterns and combinations with date filters.

Completion gate: no category or organization benchmark times out, no false
positive reaches ranking, and a broad pattern has an explicit bounded route
rather than a heap-wide element expansion.

Shadow evidence on the candidate binary: broad arXiv organization filtering
resolved natively at about 930-940 ms warm with 873,163 allowed documents.
PubMed date plus category stopped timing out and completed at about 1.29-1.31
seconds warm with 51,033 exact allowed documents. Its remaining 94 MB of
forward-row reads belongs to CQ-3 route selection, not filter construction.

A colder diagnostic run exposed a second candidate-construction boundary that
the earlier warm panel had hidden. PubMed `categories ILIKE '%cancer%'` has
4,958,998 distinct scope values, above the 4,194,304 exact-comparison limit, so
the v2 scope delegated that predicate to SQL and the complete query took
5.09 seconds. The raw query trace is preserved under
`docs/performance/data/raw/commons-query-readiness-2026-08-21/`.

Scope v4 now publishes a compact ASCII-trigram block filter inside the existing
scope child object. PostgreSQL 18 UTF-8 ILIKE first lowercases complete strings,
and Shadow's libc `C.UTF-8` collation allows Unicode values such as the Kelvin
sign to match ASCII. To preserve a no-false-negative contract, any dictionary
block containing a non-ASCII value is saturated and cannot be skipped; an
ASCII-only block is pruned only when the active PostgreSQL collation folds the
complete ASCII alphabet to the expected lowercase bytes. PostgreSQL
`texticlike` remains the membership authority, unsupported patterns retain the
exact fallback, and no second artifact or lifecycle was introduced. The query
trace records total values, values examined after pruning, considered/skipped
blocks, and actual comparisons. Scope v2 and the intermediate v3 remain
readable but are maintenance-stale, so the normal worker republishes v4 without
semantic inference. Unit compatibility, a forced Unicode scope case, and the
complete staged PostgreSQL lifecycle passed all 52 gates. Shadow latency and
skip-rate qualification remain open until that republish completes.

The first Shadow v4 publication showed that saturating every dictionary block
containing any non-ASCII value was exact but ineffective on PubMed: only 596 of
19,372 gram blocks could be skipped for the date-plus-category case. Scope v5
therefore builds pruning grams from PostgreSQL `lower()` under the included
column's actual collation while retaining the original dictionary bytes and
`texticlike` as the sole membership authority. ASCII values keep the existing
fast path; non-ASCII values receive only a metadata-local folded copy, so this
does not add an artifact, scorer, or lifecycle. Scope v2 through v4 remain
query-readable but maintenance-stale. A fresh staged package passed all 53
convergent lifecycle gates, including the Kelvin-sign Unicode fold case, and
the physical replication suite passed prepared rollback/commit, linked-L0,
eventual completion, UPDATE/DELETE, REINDEX, and DROP. Shadow v5 skip-rate,
exactness, and latency qualification remain open.

#### CQ-3: Select an exact scoring route from measured work

- [x] Keep direct forward-row scoring for genuinely small allowed sets.
- [x] For medium and broad exact sets, intersect the allowed bitmap during the
  existing posting/block-major candidate traversal so excluded blocks and
  documents are not decoded or scored.
- [x] Base route selection on explicit physical work estimates such as allowed density,
  touched chunks, query posting bounds, and forward rows. Do not add a learned
  gate or an unexplained threshold grid. Directory v7 removes the linear
  planning scan, but full-root evidence shows that its term-work count omits
  the transpose executor's fixed byte and page-I/O floor. Directory v10 adds
  exact block-local row offsets. Automatic bounded admission is now restricted
  to cases where direct has already lost the publication-derived byte
  comparison, transpose exceeds the existing 64 MiB statement-work budget,
  and the published bound stream is physically smaller than transpose. This
  rule contains no corpus identity, document-count cutoff, or fitted ratio.
- [x] Preserve exact subset top-k. Candidate exhaustion must use a deterministic
  exact continuation or zero-score completion, never global top-k post-filter.
- [x] Bound statement memory and release all statement-local bitmaps, row
  buffers, and readers on success, error, and timeout.
- [ ] Qualify the filter-aware exact BMP route on the full current-format
  PubMed root. Promote it only if exact subset top-k remains bit-identical,
  selective filters perform less bound/posting work than unfiltered search,
  broad filters stay bounded, and repeated queries hold a stable RSS plateau.

Completion gate: selective filters do less ranking work than unfiltered search;
broad filters remain bounded; repeated mixed queries show stable RSS and no
backend-local index-sized state.

##### Query-path audit, 2026-08-22

A line-level audit found that the exact-BMP representation and forced
qualification path are ahead of the normal product router. The implementation
must close the following items before CQ-3 through CQ-8 can complete:

- [ ] `CQ-3A`: replace the normal filtered route's retained-cap heuristic with
  one measured planner. The planner must compare ranked-prefix, forward-row,
  transpose, and exact-BMP work from root/query metadata, select one authority,
  and include failed probe work in the final trace. The AM and page-level
  ranked-prefix probes must not independently repeat the same global ranking.
- [x] `CQ-6A`: keep generation-bound immutable-root TID lookup usable
  while linked L0 exists. Shadowed immutable TIDs must be rejected and only the
  bounded L0 projection may add replacement TIDs. A small mutation must not
  trigger materialization, hashing, and a full document-record scan.
- [x] `CQ-6B`: score a converged immutable root with exact BMP while a bounded
  L0 projection is present, then merge L0 scores and suppress
  shadowed/tombstoned root documents. Read-your-writes and filtered subset
  semantics remain exact. The derived semantic accelerator intentionally stays
  in `exact_fallback_l0` until maintenance republishes it; linked-L0 safety must
  not be misreported as derived-accelerator eligibility.
- [x] `CQ-3B`: replace unconditional corpus-sized residual state with two exact
  executors selected from query-local physical cost. Dense accumulation is
  eligible only when its complete scratch fits the existing 64 MiB statement
  budget and `postings + documents` is no greater than the merge estimate;
  otherwise a bounded document-slot merge remains authoritative. Large roots
  never disable the accelerator merely because a dense score array does not
  fit. The remaining gate is full-root latency, memory, cancellation, and RSS
  qualification of this adaptive executor, not a third residual algorithm.
- [ ] `CQ-3C`: load accelerator term metadata and document references lazily
  enough to prune before materializing a complete high-DF term. Query telemetry
  must count all owned accelerator indexes, bitmaps, and residual scratch.
- [x] `CQ-3E`: qualify the bounded full-rebuild publisher at full-root scale.
  The implementation now publishes page-addressable lexical and semantic terms
  directly from bounded streams under the existing root, WAL, retirement, and
  reclamation authority. It no longer materializes all postings or allocates a
  second sortable payload copy. The package-bound 7.79M-document PubMed rebuild
  passed the full-root process-family memory and runtime-migration gates below.
- [x] `CQ-3D`: make filtered zero-score completion bounded by `k`, including
  linked-L0 queries. It must not allocate, load, and sort every allowed
  document merely to fill a short result.
- [x] `CQ-3F`: qualify filtered-accelerator cancellation safety. The
  implementation now binds outer directory/readers, helper top-k state, and
  generic block-major/fallback scratch to one PostgreSQL statement-context
  error/reset owner. This closes the discovered non-local-exit leak without
  suppressing cancellation or changing route thresholds. Focused contracts,
  native units, and the staged 64-gate lifecycle pass. The same backend now
  survives 12 repeated full-root bounded-route cancellations at 10 ms and is
  reusable after every cancellation. RSS rises once from 53.9 MiB to about
  76.0 MiB and then remains flat through the final sample. The
  `scripts/qualify_filtered_accelerator_cancel_rss.py` full-root gate requires
  a live Linux PostgreSQL root and a representative filtered query, so it is a
  qualification tool rather than a self-contained maturity-suite smoke.
- [x] `CQ-3G`: make forward-route telemetry measure physical work rather than
  planner estimates. Direct rows no longer pre-seed `chunk_reads` and then
  count the same chunk again; transpose counts each chunk when it is actually
  scored. Filtered top-k and transpose score scratch are included in the
  statement-memory peak so CQ-3A cannot select a route from understated cost.
- [x] `CQ-3H`: make every page-native scoring fallback cancellation-safe, not
  only the derived accelerator. The audit found that exact-BMP, ordered-block,
  and exact residual-merge helpers owned libc scratch in function-local
  variables while calling page readers and `CHECK_FOR_INTERRUPTS()`. The local
  implementation now places that scratch under statement-context owners and
  leaves scoring algorithms and route thresholds unchanged. The PG18 build,
  focused tests, inventory gate, and staged lifecycle pass. Existing full-root
  evidence covers accelerator, exact-BMP, filtered exact streaming, and
  term-at-a-time. A four-atom, two-field full-root query independently selected
  ordered-block scoring, considered 60,781 blocks, and survived twelve 5 ms
  cancellations in one reusable backend. RSS reached an approximately
  34.6 MiB plateau. This closes the final fallback owner.
- [x] `CQ-7A`: account for or reuse every visibility retry. Repeated rank-limit
  attempts must be visible in trace work and must not silently repeat an entire
  global scorer under stale/dead tuple pressure.
- [x] `CQ-8A`: remove the unreachable filtered resident-fold branch and any
  source-contract tests that preserve dead routing rather than product
  behavior.

The completed implementation slices are `CQ-6A`, `CQ-6B`, `CQ-3B`, `CQ-3D`,
`CQ-3F`, `CQ-3G`, `CQ-3H`, `CQ-7A`, and `CQ-8A`. `CQ-3F` and `CQ-3H` are
closed by the repeated full-root cancellation and RSS evidence below. The
bounded `CQ-3E`
implementation and its full-root rebuild RSS qualification are complete. The
remaining critical order is: qualify the normal `CQ-3A`
route in the application matrix; use the owner matrix to decide whether
`CQ-3C` needs another format change; then run `CQ-4`, `CQ-5`, and `CQ-7`.
`CQ-3E` is an independent build/publication slice
and must not disturb query qualification. Each slice requires focused
source-contract tests plus staged PostgreSQL behavior evidence. A forced test
route cannot substitute for the normal-route product gate.

`CQ-3E` is split at the actual ownership boundary instead of being treated as
one allocator change. The first source-compatible slice introduced a
term/document-sorted semantic reader. The full rebuild source transposes
document-major output through PostgreSQL external `tuplesort` into a `BufFile`
bounded by `work_mem`. The second slice merges that stream with the lexical
term index and writes page-addressable fold groups directly, eliminating the
corpus semantic source array, the second sortable copy, and the complete
combined payload. Incremental L0 publication retains the bounded
arbitrary-order array API. A byte-for-byte fold oracle plus malformed and
short-stream tests protect both interfaces. Full-root RSS qualification,
rather than another publication rewrite, is now the remaining `CQ-3E` gate.

The permanent Shadow qualification root contains 7,791,171 copied PubMed
documents. Its isolated build selected the bounded builder because the
92,761,227,264-byte spill estimate exceeds the 34,359,738,368-byte rebuild
budget. At the 2026-08-22 21:03 EDT checkpoint, heap progress was 11.03% and
backend RSS was about 8.55 GiB. This is healthy builder-phase evidence only.
The root uses the earlier qualification candidate, so it is authoritative for
the `CQ-3A`, `CQ-3B`, and `CQ-3C` query matrix, not for the new bounded
publisher. `CQ-3E` requires a separate full-root rebuild with the streamed
publisher after this query root completes; it must not interrupt or replace the
in-progress same-root query qualification.

`CQ-3D` was closed independently on 2026-08-22 before the remaining route
planner work because it removed an unconditional `O(|allowed|)` completion
cost without changing positive scoring or route selection. Zero-score
completion now asks the same-root COW directory for the earliest matching live
records through a predicate-aware best-first cursor, keeps at most `k`
immutable and `k` linked-L0 candidates, suppresses L0-shadowed immutable
records, and sorts at most `2k` candidates. It no longer allocates, loads, or
sorts the complete allowed set. The staged convergent lifecycle passed 85/85
gates. Its active-L0 `k=3` exactness gate examined six COW records, reached a
heap peak of six, and added exactly three deterministic zero-score documents;
the real-model SAE lifecycle remained 57/57.

The first `CQ-3A` authority/accounting slice passed on 2026-08-22. A
structured-filter ranked-prefix probe is now owned by the AM layer for that
statement; if it cannot prove the subset top-k, the page scorer receives an
explicit already-attempted marker and cannot repeat the same global prefix.
Failed probe work remains separate from the final scorer's route flags but is
reported as probe attempts, documents, postings, and peak memory in
`ii42_query_trace_internal()`. The staged real-model lifecycle passed 58/58
gates. Its exact structured-filter case recorded one failed prefix probe over
three documents and five postings, then selected exact semantic BMP without a
second accelerator attempt. `CQ-3A` remains open for measured selection among
forward rows, transpose, and exact BMP.

The second `CQ-3A` audit established that document-major forward rows and the
per-chunk sparse/dense transpose are two physical layouts of the same retained,
int8-quantized contributions. Serialization validates their equivalence; the
transpose is not a sampled candidate view. Therefore a transpose result shorter
than `k` cannot become more complete by rescoring the same chunk through
forward rows. That duplicate full pass has been removed. The staged real-model
lifecycle passed 58/58 gates, including exact structured-filter membership,
CRUD, worker convergence, restart, and accelerator publication. The remaining
router still uses `retained_document_cap`, which describes publication content
rather than query I/O or posting work; it remains explicitly unqualified until
a measured planner replaces it and compares actual forward, transpose, and BMP
costs.

The same audit found that both forward layouts calculated posting work but the
page layer discarded it, leaving only logical bytes and document counts in the
trace. `accelerator_forward_postings_examined` is now required telemetry for
both layouts. Route promotion must compare this work with BMP posting/ref work;
latency or bytes alone are insufficient evidence for a stable planner. The
updated staged package passed 58/58 real-model lifecycle gates and emitted the
new field through the live PostgreSQL JSON trace; full-root Shadow probes remain
the authority for nonzero forward/transpose measurements.

The current normal filtered call order is now an explicit `CQ-3A` constraint.
It attempts the lossy forward accelerator before loading page-query term plans
and returns immediately when direct-row or transpose scoring succeeds. The
exact-BMP scorer therefore cannot compete for the same normal statement; a
forced exact-BMP A/B proves representation correctness but cannot qualify the
product router. Full-root qualification must measure one immutable root under
ranked-prefix, direct-row, transpose, and exact-BMP execution, including failed
probe work and the complete query-memory owner breakdown. Only those same-root
measurements may define the planner boundary. Reordering the routes without
that matrix would replace one publication-content heuristic with another.

A third `CQ-3A` audit closed the remaining prefix-observability gap without
changing route selection or scores. Structured JSON filters already assign the
ranked-prefix authority to the AM layer and mark that work as attempted before
the page scorer runs. Direct document-ID and TID filter sets instead run their
prefix inside the page scorer; failed probes were previously discarded from
the final trace, while successful probes appeared as a generic accelerator
route. Page-native stats now retain each probe's document, posting-equivalent,
and peak-memory work, and identify a successful probe as `ranked_prefix`.
Visibility retries aggregate those counters rather than hiding repeated work.
The staged real-model lifecycle passed 63/63 gates. Its direct TID case used one
prefix probe over three documents, 14 posting-equivalent operations, and
867,818 bytes of probe memory; the structured case recorded one failed probe
before selecting exact semantic BMP. This makes the full-root planner matrix
auditable, but it does not close `CQ-3A`: measured route selection and Shadow
qualification are still required.

A fourth `CQ-3A` line-level audit found that visibility retries accumulated
physical work correctly but OR-combined their route flags. A query whose early
attempt used one scorer and whose final visible top-k required another could
therefore report the earlier route as `query_route`. The search trace now keeps
the final successful attempt separately for route naming, while cumulative
documents/postings remain additive and query-memory ownership remains the peak
of sequential attempts. This is an observability correction only: it changes
neither route selection nor scores. Full-root route qualification must use this
final-attempt authority rather than infer the chosen scorer from historical
attempt flags.

`CQ-6A` passed on 2026-08-22. The immutable TID directory is now keyed and
rekeyed by manifest authority rather than linked-L0 bytes. Root entries
shadowed by L0 are rejected while replacement TIDs come from the bounded
projection. The fresh staged real-model lifecycle passed 57/57 gates,
including old-TID rejection, replacement-TID visibility, and unchanged-root
TID lookup under linked L0; isolated SQL regression remained 1/1. Scope lookup
remains disabled under L0 until it has equivalent shadow-safe semantics.

`CQ-6B` passed on 2026-08-22. The mutable query path now keeps the lossy
derived accelerator fail-closed because its retained scores are bound to the
published corpus statistics, but it may use the root-native exact BMP scorer.
Root offers reject immutable documents shadowed or retired by the
snapshot-visible L0 projection, then the bounded projection contributes its
replacement and inserted documents through the existing exact merge. The
superuser exactness probe now builds the same projection and attaches the same
L0 overlay to its independent oracle instead of requiring sealed storage. A
fresh staged real-model lifecycle passed 59/59 gates. In the immediate INSERT
state, exact BMP was attempted and used without fallback, examined five BMP
postings, and returned the same four document IDs and score bits as the exact
page fallback. This changes no root format or lifecycle and does not claim
that a stale lossy accelerator is safe under L0.

The post-`CQ-6B` source audit confirms that `CQ-3B` remains a real product
boundary. The default accumulated residual path still allocates and later
scans `float[document_count]`; above the fixed workspace limit it abandons the
accelerator. A bounded weighted summary exists, but only behind an unqualified
test switch, so it cannot be promoted without rank-quality and error-bound
evidence. The same audit confirms `CQ-8A`: the late filtered resident-fold
call is unreachable because the resident-fold implementation rejects every
non-NULL filter. It should be removed as dead routing after its own focused
contract and lifecycle slice rather than preserved by source-shape tests.

`CQ-8A` passed on 2026-08-22. The shared resident fold is term-major and has
always rejected a non-NULL filter, yet the fixed-root filtered path attempted
it after resolving membership. That unreachable second call and its
source-shape test have been removed; the earlier resident-fold attempt is now
explicitly restricted to unfiltered queries. Filtered search continues through
the same page-native exact scorer and no compatibility or replacement route
was introduced. The helper itself is now unfiltered-only: its unreachable
filter parameter and filtered scorer branch were removed as well.

`CQ-3B` has prior bounded-algorithm evidence, but not yet the required scale
evidence. The fixed 648-query FIQA run in
[semantic-accelerator-bounded-execution.md](../../performance/reports/semantic-accelerator-bounded-execution.md)
shows that a 14,400-counter weighted Space-Saving summary keeps about 1.05 MiB
of corpus-independent state, with mean exact O@100 0.998688 and unchanged
NDCG@10, Recall@100, and MRR@20. Its minimum O@100 is still 0.81 and its p50 is
about 111 ms versus 61 ms for exact packed scoring, so it remains a large-root
qualification candidate rather than a new default. The current Shadow root
must compare accumulated and summarized residual routes with the same query
set before this item can close; a small-corpus metric match does not justify a
silent approximation on PubMed-scale residual competition.

The source audit also identifies one exact bounded alternative that must be
tested before accepting the approximate summary. Every residual posting run is
ordered by document slot, and the current accumulated path visits terms and
runs in a deterministic order. A document-at-a-time min-heap over those runs
can therefore form each document's complete residual score in the same
contribution order, offer that score directly to the existing bounded top-k,
and discard it before advancing. This removes both
`float[document_count]` and the final corpus scan while retaining exact
candidate selection, with state proportional to residual run count plus
candidate capacity. Reusing the existing linear `next_document` cursor scan is
not acceptable at scale because it changes the removed memory cost into
`O(distinct_documents * residual_runs)` comparisons. Promotion requires a
bit-identical small-root oracle, full-root work/RSS telemetry, and latency no
worse than the accumulated implementation where that implementation fits. If
the exact merge is slower or cannot preserve float/tie behavior, the bounded
summary remains experimental rather than becoming a silent product default.

The first exact-merge implementation passed the staged real-model lifecycle
twice without changing CRUD, REINDEX, restart, filtered membership, or default
accelerator hit IDs. It preserves term/run contribution order through a
document-slot min-heap, retains the existing 64 MiB query-shape budget, and
reports its cursor heap, decoded blocks, term plans, and bounded top-k as
residual scratch. The fixture used about 0.86 MiB instead of the old 0.13 MiB,
which is an expected small-root regression from fixed cursor blocks; unlike the
old array, it does not scale with corpus document count. `CQ-3B` remains open
until same-root Shadow A/B proves bit-identical candidate behavior and an
acceptable latency/work tradeoff on production-sized residual runs.

`CQ-7A` passed on 2026-08-22. Every page-native and shared-resident scorer call
now increments a saturating `visibility_rank_attempts` counter. Physical work
from each page-native attempt was already accumulated; the trace now also
combines attempts spent by a failed ranked-prefix proof with attempts in the
final subset scorer. This makes MVCC-driven rank-limit expansion explicit
instead of reporting only the last attempt. The fresh staged real-model
lifecycle passed 60/60 gates; its structured-filter probe reported two
visibility attempts, exactly one failed ranked-prefix attempt plus one final
exact subset-scoring attempt.

The `CQ-3C` ownership audit found that the previous trace counted forward I/O
and examined document references but omitted the memory retained by fully
deserialized query-term indexes, document bitmaps, and residual accumulators.
The query path first reported deserialized indexes, membership state, forward
reader scratch, and residual scratch, but the follow-up source audit found one
missing owner: the accelerator core's query arrays, block/ref directories,
visited bitmap, ranked-cluster buffer, candidate heap, and result buffers. It
now reports that state separately as candidate scratch and includes all five
components in the statement-level memory estimate. This is an observability
prerequisite, not a lazy-loading claim:
full-root evidence must first identify whether term-index document references,
block scratch, or residual state dominates before changing materialization.
The fresh staged real-model lifecycle passed 61/61 gates. Its bounded
accelerator probe now reconciles 867,818 bytes as 3,800 bytes of owned indexes,
1 byte of membership state, 901 bytes of candidate scratch, 100 bytes of
forward scratch, and 863,016 bytes of exact residual-merge scratch. The fresh
staged real-model lifecycle passed all 62 gates. The small fixture establishes
the counter contract;
the production-sized Shadow root remains the required scale measurement.
Visibility retries are sequential, so their work counters accumulate but their
accelerator workspaces do not. Statement telemetry therefore retains the peak
accelerator attempt and copies its five owner components together instead of
adding released workspaces or mixing components from different attempts.

The subsequent `CQ-3C` line-level audit identified the exact materialization
boundary. Each query term is one page-chain object containing its header,
cluster directory, complete document-reference array, and summaries. The
current loader reads and verifies the complete object, then allocates and
copies all four arrays before block scoring begins. The serialized order also
places document references before summaries, so the block bounds needed for
pruning are not available as a compact leading range. Consequently every
accelerated high-DF query term pays complete document-reference I/O and memory
before any cluster or block can be rejected.

The storage layer already supports checked range reads: every selected page is
validated against the immutable object identity and its payload checksum, and
the generation page-validation bitmap safely reuses that result. Therefore
`CQ-3C` does not require another sidecar, root, worker, or lifecycle. If the
full-root owner matrix confirms term materialization as the dominant cost, the
correct product change is a new sole-authority term layout under the existing
accelerator directory: compact cluster bounds and summaries first, followed by
page-addressable document-reference ranges. Query execution must load the
compact metadata first and fetch document references only for opened blocks.
Publication, WAL, retirement, compaction, and rebuild remain owned by the same
accelerator root. The current in-progress Shadow build is evidence for the
existing layout and must not be restarted; format work remains gated on that
measurement so a structural migration is not justified by a small fixture.

The same full-root run exposed a separate publication-memory boundary,
`CQ-3E`, before query qualification completed. The admission estimator reported
95,835,652,096 bytes against a 34,359,738,368-byte rebuild budget and correctly
warned that the explicit build would continue. The temporary-backed semantic
stream kept the encoding/drain phase restartable, but the final handoff still
called `ii42_am_semantic_build_segment_source()` to materialize a corpus-wide
posting array and `ii42_segment_payload_attach_semantic()` to allocate and sort
another payload representation. On the 8.2-million-document isolated PubMed
root, the backend was observed at about 113.4 GiB RSS during semantic drain,
then released most temporary state and entered `segment publish` at about
50.1 GiB RSS. Host swap grew from about 36 GiB to 66 GiB. This matches the
estimator and source ownership; it is not evidence of an unexplained leak, but
it violates the intended corpus-independent builder-memory contract.

The current build must finish unchanged so its root remains usable for the
CQ-3A/C query matrix. The corrective implementation must reuse the existing
temporary semantic term order and write the sole-authority segment pages in one
bounded publication pass. It may retain term-local sort and page buffers, but
must not retain corpus-wide postings or create a side artifact, lifecycle, or
second authority. Acceptance requires bit-identical small-root bytes/scores,
successful CRUD/REINDEX/restart/replication lifecycle gates, a full-root peak
RSS below the configured explicit-build ceiling, and no second source-table or
semantic-inference pass. The final peak, index size, and build completion state
remain open until the current Shadow process exits.

The full-root qualification audit also found an operational identity mismatch
before the guarded A/B could execute. The isolated data directory and running
postmaster are owned by `admin`, while the older pending guard names `postgres`
as the OS account for `pg_ctl`. The mismatch cannot alter the in-progress root,
but that guard will fail closed before restarting the isolated server. The A/B
runner now validates the data-directory owner against `--postgres-user` during
argument validation, before any stop or namespace mount. The current root will
be qualified with the frozen latest runner and the correct `admin` identity
after the older guard exits; changing a live guard is explicitly avoided.

The first corrected-owner run exposed a second fail-closed runner defect before
executing SQL. The isolated postmaster inherited the runner's captured standard
output and error pipes, so `pg_ctl start` had already reached a healthy server
while Python waited indefinitely for pipe EOF. The root was unchanged and the
isolated server was stopped normally. Namespace starts now pass a data-directory
log file to `pg_ctl -l`, preventing the detached postmaster from retaining the
runner's pipes. This is qualification-tool isolation, not a PostgreSQL or index
lifecycle change.

The next setup attempt correctly restarted both variants but then rejected
query encoding because the original isolated postmaster's required
`shared_preload_libraries=ii42` startup contract was not represented by the
runner. The runner now accepts repeated, tokenized `--server-option` arguments,
records them in its manifest, and applies the identical option vector to both
binary variants. The hidden maintenance freeze is appended only during timed
measurements and removed for the requested final server. This keeps runtime
availability, shared-memory sizing, and other postmaster settings explicit
instead of relying on a different cluster's persistent configuration.

That startup fix exposed a separate catalog identity boundary. This isolated
database records an absolute staged library path in `pg_proc.probin` and the AM
handler, while shared preload uses the installed `$libdir` target. Replacing
only the preload target either loads II42 twice and rejects duplicate GUC
registration or leaves the query handler on the wrong binary, invalidating the
A/B. The runner now bind-mounts each variant over every declared library target
inside the same private namespace, verifies every namespaced target hash after
start, records all host hashes, and rejects any host-path change afterward.
This preserves the catalog unchanged while making the tested query hot loop
unambiguous.

The first true handler invocation then rejected the nominal baseline's neutral
fold format. Source identity showed that the full root was written by the
catalog's staged binary, not the older system-library copy named by the pending
guard. Qualification must therefore freeze that exact writer hash as baseline;
using a convenient installed binary is not equivalent evidence. The failed
run also showed that unconditional `leave-running` cleanup can start normal
maintenance after a measurement failure. The runner now leaves the isolated
cluster stopped on every failed A/B and can keep maintenance frozen on a
successful final variant for the normal-route follow-up. A failed diagnostic
can no longer publish a successor root during cleanup.

The next same-root run exposed a more fundamental surface-authority failure.
The 8.2-million-row PubMed qualification heap had been created as `UNLOGGED`,
so its II42 index was unlogged as well. An earlier emergency `immediate` stop,
used to prevent a failed diagnostic from running normal maintenance, caused
PostgreSQL crash recovery to reset both relations. Catalog statistics still
reported about 8.2 million rows, but the heap count, generation document count,
segment count, and posting-record count were all zero; the index relation was a
32 KiB empty shell and both tested binaries correctly returned no hits. This
invalidates that root as query evidence. It does not indicate scorer parity or
performance.

Restart-qualified A/B surfaces must therefore use a permanent heap and index.
The runner now requires an explicit qualified index and rejects the run before
setup when the heap or index is not permanent, the catalog index is not
valid/ready/live, or the II42 generation has no documents, immutable segments,
or posting records. The fixture repeats this fail-closed assertion and stores
its evidence tables as permanent relations. Qualification cleanup uses only a
normal fast shutdown and leaves every failed run stopped; destructive shutdown
is not a diagnostic cleanup mechanism. A new logged PubMed root is required
before CQ-3A/C route or latency conclusions can resume.
The manifest records the guarded postmaster PID file as well as its before and
after PID, so a later qualification step never has to infer the protected
cluster from process state or an older shell guard.

The replacement qualification surface is now being rebuilt from the permanent
production PubMed heap into a permanent isolated heap and II42 index. Binary
COPY, index publication, and the later A/B all use the same isolated PostgreSQL
18 cluster and the exact writer binary recorded for that root. The acceptance
sequence is deliberately strict: nonzero source count, nonzero generation
documents/segments/postings, clean fast restart with the same root identity,
then baseline/candidate measurement with maintenance frozen. No query-path
conclusion is valid before those checks pass.

The permanent binary COPY completed with 7,791,171 source rows. The earlier
8.2-million figure was a progress estimate derived from stale production
statistics, not an acceptance cardinality. Qualification binds to the copied
heap's counted cardinality and the resulting generation identity; it must not
reject or silently pad a valid root to match that estimate. The logged runner
entered `ANALYZE` after COPY and will start the II42 build only after the
permanent heap count and persistence checks finish. This is the only active
full-root build and must not be restarted or duplicated.

The build entered `building index` at 2026-08-22 23:57 UTC. At 3.67% heap
progress the backend RSS was about 2.85 GiB and the backend remained active
without a wait event. This remains early-build evidence, not a publication
peak or a query-path qualification result.

The accompanying source audit narrowed the remaining code work at that point.
`CQ-3B` no
longer contains the old corpus-sized score array: the product default now uses
the document-slot min-heap merge and bounded top-k accumulator. Its open state
means scale qualification, float/tie equivalence, latency, and RSS remain to be
proved; it is not permission to add another residual algorithm. `CQ-3C` is a
different boundary: the current term loader reads the complete immutable term
object and deserializes its complete document-reference array before pruning.
The block-major scorer also derives each cluster's block from the first and
last document reference, so merely range-reading the existing summaries would
still require document-reference I/O for every cluster before ranking. A lazy
sole-authority layout must carry the cluster's block identity in compact
metadata, alongside its summary bound, and load the cluster's document range
only after that block survives pruning. Existing checked range reads are
sufficient for the opened range; no second root or side artifact is needed.
`CQ-3A` then still ran a bounded global prefix probe before selecting direct-row
or transpose forward scoring, while exact BMP could not compete in the normal
filtered route. The later scale-ladder evidence below measured and replaced
that ordering; this paragraph records the pre-measurement boundary rather than
the current product route.

The pre-fix publication audit fixed the implementation boundary for `CQ-3E`.
Semantic inference, field-budget selection, lexical transpose, and unified-row
merge already end in bounded `BufFile` streams. The corpus-sized ownership is
introduced only afterward: `ii42_am_semantic_build_segment_source()` expands
the semantic rows into one `ii42_segment_semantic_posting[]`, transfers that
array through `ii42_am_rebuild_output`, and
`ii42_segment_payload_attach_semantic()` then sorts a second copy while building
the complete in-memory segment payload. The full rebuild additionally retains
the corpus lexical index, TID map, fingerprints, document versions, and payload
arrays until that attachment finishes. This is the observed publication peak;
it is not a semantic-runtime leak or a reason to replace the earlier bounded
builder.

The corrective boundary is therefore the full-rebuild handoff and page writer,
not the incremental mutation attachment API. Full rebuild must transfer the
existing document-major temporary streams and their checked row offsets to a
bounded publisher, construct and serialize one partition at a time, and release
each partition before advancing. Manifest statistics, query contract, document
identity, fingerprints, term authority, WAL, root publication, retirement, and
reclamation remain unchanged. Incremental linked-L0 publication may continue to
use `ii42_segment_payload_attach_semantic()` because its input is admitted by
the existing mutation budget and is not corpus-sized. A compliant solution
must not rescan the heap, rerun inference, create a side artifact, or retain all
partition payloads merely to feed the final document/term directories.

A subsequent pre-fix line-level ownership audit made that boundary more
precise. The initial-fold page writer already serialized bounded 64 MiB term
groups, but its API still received one complete combined
`ii42_segment_payload`. Before the
first page write, `ii42_segment_payload_build_lexical()` duplicates every
lexical posting and `ii42_segment_payload_attach_semantic_sorted_reader()`
allocates replacement run, index, value, and document-map arrays for every
lexical and semantic posting. Applying the existing contiguous partition helper
after this point would only partition an already materialized corpus payload and
would not reduce peak RSS. The full-rebuild source must instead merge the
existing term-major lexical index and sorted semantic `BufFile` directly into
one bounded fold group at a time. The same pass must accumulate lexical and
semantic residency for the document directory before releasing each group.

The document COW authority is a separate `O(document_count)` bound. It must
retain one version/state/residency record per document, but it must not derive
those residency counts by rescanning a complete in-memory posting payload. Its
memory ceiling and the fold-group serialization ceiling must be reported
separately. The first implementation slice will therefore add a streaming fold
source plus a byte-equivalence oracle while preserving the existing payload API
for bounded incremental publication. Only after that oracle passes may the
full-rebuild caller stop constructing the complete combined payload.

That stream primitive now passes its standalone oracle. It merges the existing
lexical term-major index and sorted semantic reader while retaining at most one
pending term and one target-sized group. The test compares canonical run order,
document slots, values, serialized bytes, size, and checksum with the current
combined-payload representation; all are identical. A forced small target
proves that groups split only between complete terms, and duplicate semantic
postings fail closed.

The second implementation slice now connects that source to the existing
initial-fold page writer. Fresh rebuild publication no longer calls either
`ii42_segment_payload_build_lexical()` or
`ii42_segment_payload_attach_semantic_sorted_reader()`. Instead, one producer
bundle is serialized and released at a time under the same neutral-fold object,
term-directory, manifest, WAL, root-publication, retirement, and reclamation
authority. When the producer reports end-of-stream, the corpus lexical index,
TID map, semantic `BufFile`, and fingerprint source are released before the
document COW directory is built. Incremental linked-L0 publication retains the
bounded payload API and is intentionally unchanged.

This removes the `O(total posting count)` replacement payload from publication,
but it is not yet a complete peak-memory closure. A single high-DF term cannot
be split across fold objects, so the posting-side bound is the larger of one
complete term and the nominal 64 MiB fold target. The document authority also
retains one `ii42_document_cow_record` per document while constructing its COW
tree. Full-root qualification must therefore report the posting-group peak and
the `O(document_count)` document-directory peak separately, verify that source
release occurs before COW construction, and show bounded RSS across failure,
restart, and repeated publication. Until that evidence exists, `CQ-3E` remains
open.

The full-root post-build gate is rank authoritative but separates two
different contracts. The same-input C oracle requires the combined-payload and
streaming writers to produce identical runs, document slots, values, bytes,
size, and checksum. The full-root fixture is instead an ORT 1.26 to ORT 1.29
runtime-migration gate because it necessarily re-encodes every document. It
requires exact top-100 membership, at most one rank of movement, at most 0.2%
relative score drift, and two independently bit-identical ranked replays of
each root. A membership-only digest cannot close `CQ-3E`, and runtime drift
cannot be misclassified as a writer-format regression. The validated fixture
is staged on Shadow as `/data/ii42-builds/cq3e-postbuild-equivalence.sql` with
SHA-256
`43856b33f556319e9974d49ef8cd7421065486518a868096447e8596ced6b372`;
it must not run until the candidate root has completed publication.

The wired slice passes a forced PostgreSQL 18 PGXS rebuild, the standalone C
byte oracle, all 300 Python product contracts, a fresh staged real-model SAE
lifecycle with 63/63 gates, and the native page lifecycle with 85/85 gates.
Those lifecycles cover fresh build, immediate
lexical visibility, eventual semantic completion, filtered exact membership,
linked L0, UPDATE/DELETE/VACUUM, `REINDEX`, crash recovery, cold restart, and
bounded repeated-query memory. This is sufficient to retain the implementation
slice, but not to infer the peak of an eight-million-document publication; the
full-root RSS phase evidence remains the closing authority.

A 2026-08-23 line-level publication-memory audit found no unmatched owner or
error-path leak in the bounded implementation. The semantic accelerator
transpose is owned by `BufFile`; tuplesort, tuple slots, scope locks, produced
term/forward bytes, per-chunk arrays, manifests, reuse arenas, and source
buffers all have symmetric normal/error cleanup. Forward publication retains
only one checked chunk, and the 8 MiB serialized target plus the fixed 64 MiB
allowance bounds its temporary decoded/copy buffers. Selected-term retention is
bounded by `vocab_size * 64`, which is covered by the estimator's vocabulary
allowance.

The audit did identify the exact remaining initial-publication peak. The
caller-owned `ii42_document_cow_record[document_count]` stays live while
`ii42_document_cow_tree_build()` copies those records into leaf objects and
holds the complete radix tree until object publication finishes. This is two
simultaneous `O(document_count)` authorities during COW construction, not an
unbounded leak. The posting side is separately bounded by the larger of one
complete high-DF term and the 64 MiB fold target. `CQ-3E` must report these two
phases independently on the current streamed publisher. A COW-format rewrite
is justified only if that measured peak violates the rebuild budget; otherwise
the current single-root representation remains simpler and correct.

The same audit found an independent maintenance-admission undercount. The
semantic accelerator workspace estimator reserved 40 bytes per document, but
the TID reverse-directory phase simultaneously owns source offsets/positions,
TIDs, sortable pairs, key/slot arrays, and the 12-byte-per-entry serialized
directory. The conservative floor is now 72 bytes per document and its native
overflow test remains fail-closed. Scope publication is not hidden in that
constant: its peak depends on distinct values, array fanout, dictionary bytes,
and postings.

That data-dependent scope gap is now closed without another fixed multiplier.
The PostgreSQL builder writes one random-access `tuplesort`, makes a first pass
to count distinct values, deduplicated postings, and dictionary bytes, then
rescans the same sort and streams directly into the one final v5 scope object.
It no longer retains per-value document arrays or a second serialized-values
authority. Before allocating that object, the codec computes its exact byte
size and compares it with the rebuild budget remaining after the bounded
accelerator workspace estimate. An oversized scope returns
`accelerator_scope_memory_budget`, leaves the exact root queryable, and follows
the same reconciliation retry policy as the general accelerator-memory block.
The reported maintenance estimate includes the exact required scope bytes.

Bounded generation readiness now reports `scope_bytes` from the accelerator
directory summary. This reads fixed root metadata rather than the scope object
or relation payload, so status remains proportional to generation metadata.
It provides the previous-generation artifact size needed for a measured
republish admission policy and for full-root evidence without invoking the
heavy audit surface.

Two representation-preserving peak reductions accompany that correction. The
TID builder now sorts its pair array first, allocates key/slot arrays only for
the verified visible count, and releases pairs before allocating serialized
output. Scope construction now retains only the current distinct value while
both passes preserve the prior C-collation ordering, per-document array
deduplication, Unicode gram folding, postings, scope bytes, and root authority.
Both scope and accelerator tuplesort descriptors are released explicitly on
normal and error cleanup rather than relying on a surrounding worker memory
context.

The streamed scope slice passes the native byte oracle, PostgreSQL 18 PGXS
build, all 308 Python contracts, product-convergence inventory, and a fresh
staged real-model lifecycle with 64/64 gates. The lifecycle produced current
scope v5, a 2,050-byte final scope object, exact filtered membership through
CRUD/savepoint/abort, eventual semantic completion, cold restart, and bounded
storage/backend RSS. A one-byte-short codec budget is rejected before
allocation while preserving the exact required size. These checks close the
scope materialization and admission defect; they do not replace the required
production-sized `CQ-3E` phase/RSS evidence.

A cancellation/error-path review then found that ordinary `ii42_status`
failures reached the scope and accelerator cleanup labels, but a PostgreSQL
`ERROR` or statement cancellation could jump over libc-owned build
temporaries. Scope values, column descriptors, and accelerator-only scratch
now use PostgreSQL memory contexts, so transaction/error cleanup owns them.
Corpus-sized arrays use the context-owned huge-allocation API, preserving the
explicit maintenance-budget authority instead of introducing `palloc`'s
unrelated one-gigabyte per-allocation ceiling.
The scope writer's portable malloc-backed output remains unchanged, but its
second pass is protected by `PG_FINALLY` and always releases an unfinished
buffer. Durable source/output buffers remain under the existing accelerator
source cleanup. This closes retry-time worker RSS retention without changing
serialized bytes, sorting, scope membership, or publication authority.

The permanent Shadow query-qualification root reached 340,792 of 1,414,848
heap blocks (24.09%) while healthy, with no build-log error. Its builder RSS
was 19,367,108 KiB after 8,739 seconds and remained below the 32 GiB admission
budget. It intentionally
uses the earlier query candidate and can close only the `CQ-3A`/`CQ-3C` owner
matrix. It cannot qualify the newer streamed publisher's `CQ-3E` RSS gate, so
the two evidence tracks must not be conflated or used to restart one another.

A later live heap-scan audit found that this RSS was not solely publication
working set. `ii42_runtime_accelerator_targets()` reparsed the unchanged JSON
accelerator configuration for every scheduler/capacity call in the caller's
long-lived build context. The retained JSON and URL allocations cost roughly
10 KiB per document and produced linear RSS growth before publication. The
parser now has one bounded process cache under `TopMemoryContext`, invalidated
by the exact GUC text, while each caller still receives and releases its own URL
copies. Empty configuration and parse errors reset the cache, so reload and
failure semantics do not leave stale targets.

The scale ladder separates that leak from the remaining publication peak. An
old-binary 57,638-document FIQA rebuild reached 131.6 MiB of used PostgreSQL
memory during heap scan and 834.0 MiB peak RSS after publication. With the
cache fix, a deliberately cancelled 13,000-15,000-document FIQA build stayed
at or below 54.5 MiB RSS and a sampled PostgreSQL context owned 14.7 MiB; this
proves that the parser-related linear accumulation is gone, but is not a
full-run peak comparison. A complete 2,000-document current-format field-aware
rebuild was byte/score exact before and after the change, passed the same
query-ready/contract/status checks, and peaked at 478.2 MiB during final
publication. That separate finalization peak remains attributable to root/COW
construction rather than accelerator-target parsing.

The cache slice passes warning-clean PostgreSQL 18 and CMake builds, the full
325-test Python suite, and a fresh staged real-model lifecycle with 70/70 gates
covering CREATE, CRUD, VACUUM, `REINDEX`, semantic completion, shared preload,
rollback, failed rebuild, restart, and crash restart. It is therefore retained
as a correctness and rebuild-efficiency fix. The in-progress Shadow root still
uses the older leaky binary and remains useful only as query qualification; it
cannot close the patched build-RSS gate. `CQ-3E` still requires one full-root
run with the patched binary and phase-labelled heap-scan, stream publication,
COW publication, cancellation, and final RSS evidence.

A fresh line-level query audit also fixes the interpretation of that matrix.
The normal filtered path first gives the semantic accelerator one bounded
ranked-prefix opportunity, then chooses direct or transposed forward scoring
from its current retained-cap heuristic. Exact filtered BMP is reached only
when the accelerator route is not used. Both paths enforce the same allowed
document membership, but they own different physical work: forward scoring
owns active chunks, decoded forward bytes, rows, and postings, while BMP owns
allowed blocks/superblocks, references, postings, and compact score scratch.
Consequently an allowed-document count or a route latency alone cannot select
the product route. The permanent same-root `auto`/`direct`/`transpose`/forced
exact-BMP matrix must compare these owned counters, bit/tie equality, bounded
statement memory, and repeated backend RSS before `CQ-3A` or `CQ-3C` changes.

The active decision ledger is now explicit:

- `CQ-3A`: no route-order or threshold change before the permanent same-root
  matrix reports prefix, forward, transpose, and exact-BMP work.
- `CQ-3B`: retain only the measured dense-or-merge exact executor. Do not add a
  third algorithm or promote the approximate summary. Close only with
  full-root latency, statement-memory, cancellation, and RSS evidence.
- `CQ-3C`: no format migration unless the same-root owner matrix shows complete
  term-reference materialization is material. If required, publish block
  identity and signed summary bounds in leading compact cluster metadata, then
  range-read only opened reference spans from the same object authority.
- `CQ-3E`: proceed independently with the bounded full-rebuild publisher, but
  do not alter or restart the in-progress permanent qualification root.
- `CQ-4`: the local restart/preload design is complete enough for
  qualification; do not add another cache or warm artifact. Close the item
  only with production restart, eviction/re-admission, concurrent first-query,
  latency, and RSS evidence after the current root is available.
- `CQ-5`: the matrix and targeted statistics tooling are complete. Shadow's
  temporary `autovacuum = off` and missing statistics remain an environment
  blocker, but `ANALYZE` must wait until the isolated root build releases host
  I/O. Statistics may change PostgreSQL predicate plans, never II42 scorer
  correctness or route admission.

The route-scale v2 contract now audits the automatic forward choice directly
against the same publication-derived byte estimates used by the planner. It
distinguishes `forward_rows`, `forward_transpose`, and `forward_bound`, rejects
missing estimates or ambiguous executor flags, and records an aggregate pass
only when at least one forward decision was observed and every applicable
decision matches the product formula. This fixes the previous bounded-route
trace label and makes the full-root matrix self-checking; it does not alter the
route formula, scoring, root bytes, or any threshold.

The same-root runner now closes a diagnostic execution gap without changing
the product planner. The candidate binary exposes one hidden
`auto`/`direct`/`transpose` forward-route control. Forced modes bypass the
ranked-prefix probe and select one forward layout when available; `auto`
preserves the normal route. After the exact baseline/candidate comparison, the
runner executes all three candidate modes against the same filter sets and
records separate traces and overlap with the forced exact candidate. A staged
real-model lifecycle passed 64/64 gates, including bit-identical direct versus
transpose results and reset-safe route traces. These controls produce the
missing owner matrix; forced results still cannot promote a product route by
themselves.

The qualification evidence contract also treats the normal-route probe as an
immutable experiment input. The runner must resolve it as a regular file before
stopping PostgreSQL and must record its path and SHA-256 beside setup, run, and
comparison SQL in the manifest. A missing, replaced, or unbound probe cannot be
used to close the `CQ-3A` owner matrix. This is a harness-integrity correction;
it does not change route selection or scoring behavior.

Each same-root qualification run also owns one empty output directory. The
runner rejects pre-existing files before reading the installed binary or
stopping PostgreSQL, so a retry cannot mix prior logs, hashes, or failure state
with the new manifest. Operators must use a new run directory rather than
overwriting qualification evidence in place.

The full-root fixture applies its 60-second timeout to each measured query, not
to an entire multi-query procedural loop. Setup installs benchmark-only capture
functions, and `psql` `\gexec` invokes one function call per filter and attempt.
This preserves bounded failure for one pathological query while allowing the
complete 40-run exact panel and 20-run diagnostic panel to exceed 60 seconds in
aggregate. A fixture-level timeout can no longer reject otherwise qualified
per-query latency solely because the matrix contains repeated measurements.

Architectural decision: filtering must be a first-class input to the scorer,
not a post-filter over a global ranked prefix. The current exact-BMP product
slice implements that interface with root-stable document membership, allowed
b16/superblock sets, adaptive fine-block membership, exact signed bounds, and
kth-score block pruning. It remains under the existing root, accelerator,
worker, WAL, retirement, and reclamation authority. The remaining CQ-3 work is
full-root physical qualification and route promotion, not another persistent
filter artifact or lifecycle. The small-set forward scorer and the ordered-
range ranked-prefix proof remain specialized routes; neither is the general
medium/broad-filter solution.

Interim Shadow evidence: the CA document scope stress case exposed an
insufficient transpose followed by an unnecessary exact posting scan. Route
selection now uses the accelerator publication's retained-document cap, and
exact direct-row exhaustion uses the same-root COW born-prefix cursor for
deterministic zero-score completion. Warm latency fell from about 6.23 seconds
to about 593 ms, forward bytes fell from 131 MB to 54.7 MB, and all 50 results
remained inside the 38,983-document scope. Medium PubMed filters and broad
range construction remain open; CQ-3 is therefore not complete.

Rejected Shadow experiment: replacing each sparse chunk's contiguous offset
directory read with per-row offset-pair reads reduced the nominal byte range
but turned sequential I/O into thousands of tiny reads. Both measured queries
timed out at 60 seconds and one backend terminated with `SIGSEGV`, after which
PostgreSQL completed crash recovery on the prior binary. This route is not in
the product. Further CQ-3 work must preserve batched contiguous reads or prove
an exact ranked prefix before constructing the full scope bitmap.

The range-only ranked-prefix slice now supplies that proof for expensive
ordered predicates. It estimates the next power-of-two prefix from the observed
match rate, but returns only after the prefix contains `k` actual predicate
matches; other predicate shapes retain the previous 128-result probe and scope
route. On Shadow PubMed, the partial-date case moved from a 30-60 second timeout
to an 809 ms warm p50 with zero violations. The admitted query read 35,040
forward rows and 324 MB instead of the earlier 67,749 rows and 577 MB. The
date-plus-category case remained on `forward_rows` at about 1.34 seconds, so
medium mixed filters remain open. The first partial-date query after restart
still took 59.9 seconds despite the resident marker; that is CQ-4 evidence, not
a qualified cold result.

A subsequent exact-BMP audit confirmed that the remaining medium-filter gap is
structural rather than a cache-size problem. The PubMed date-plus-category
filter admits 51,033 of roughly 8.2 million documents, but those documents span
41,380 sixteen-document blocks and 16,747 256-document superblocks. The query
contains 84 active runs. Exact block pruning opens only about 3,540 blocks and
examines about 83,000 semantic postings, yet the current page-native authority
must stream roughly 19.5 million fine-bound records to prove those blocks. This
metadata work dominates the scorer even after the data is warm.

Two bounded fixes are retained. Fixed-root filtered searches may now reuse the
same-generation shared document-length projection; a root or manifest mismatch
still fails closed to the pinned-root reader. Filtered BMP also bypasses its
coarse hierarchy and streams fine bounds once, eliminating about 15.3 million
redundant super-ref reads. On Shadow this reduced the warm exact-BMP p50 from
about 6.61 seconds to 6.35 seconds, with bit-identical top-50 TIDs, document
IDs, and scores. A staged package passed all 70 unified lifecycle gates. The
small gain proves that route thresholds, prewarm, and larger statement caches
cannot close CQ-3 by themselves.

The next CQ-3 slice is therefore a same-root complete fine-bound query layout,
not another scorer or lifecycle. Scope membership, sign-aware score bounds,
and posting ranges must be published under the existing accelerator/root
generation and retired by the existing worker and reclamation protocol. The
filtered scorer must intersect its allowed-block bitmap before decoding bound
or posting payloads, retain forward rows for genuinely small sets, and preserve
an exact deterministic continuation. No backend-local index-sized state or
independently maintained side artifact is permitted.

The first representation oracle now passes that admission gate on Shadow.
For the 51,033-document PubMed date-plus-category filter, only 1,651,658 of
19,505,054 query-term fine references belong to allowed b16 blocks. A layout
that can address those references from the allowed-block set before decoding
therefore removes 91.53% of bound reads, an 11.81x reduction. The diagnostic
does not alter routing, scoring, or publication; all three attempts returned
the same top-50 identities and scores with zero predicate violations. The
second locality audit narrows the required representation. The same query
has 2,185,173 query-term super references, of which 792,821 intersect the
filter's allowed superblocks (36.28%), while only 8.47% of the fine references
intersect allowed 16-document blocks. Coarse superblock pruning alone can
therefore remove at most 2.76x of the bound directory work and cannot realize
the 11.81x fine-reference opportunity.

This justifies a same-generation, query-term-selective fine-block directory,
not a block-major scan over every term in an allowed block. The preferred
shape is adaptive block membership plus rank/select into the existing exact
packed references: dense high-DF runs use a bitset, sparse runs retain a
compact ordered representation. A selected child must expose its exact signed
bound and impact offset without decoding preceding children. Approximate
accelerator terms remain ineligible as exact authority, impacts are not
duplicated, and publication, validation, retirement, and reclamation stay
under the pinned root and existing worker lifecycle.

The first product-format implementation and medium-corpus check now pass. The
packed semantic format emits a dense fine-block membership bitset only when its
bytes do not exceed the existing compact-reference count; sparse terms retain
the ordered reference stream. The filtered exact scorer intersects that bitset
with the statement-local allowed-block bitmap, uses rank/select to address the
matching exact signed bounds, and leaves record scoring unchanged. A 57,638
document FIQA root built successfully as a 183 MiB index. On one 128-atom
query, a dispersed 0.62% filter read 132 matching bounds and completed warm in
18.1--19.4 ms. Ten-percent, fifty-percent, and full-corpus filtered exact runs
completed in 16.4--19.6 ms, with every run reporting complete top-k. The
existing forward scorer remains better for the smallest subset at 5.5--7.7 ms,
so the result supports route specialization rather than replacing the small-set
path. This closes the representation and local execution oracle, but not CQ-3:
the new format still requires a Shadow-scale I/O and latency qualification
before it can become the broad-filter product route.

The first Shadow physical A/B also passes. A fresh 500,000-document PubMed
field-aware root occupied 2.68 GiB and was tested with one fixed 45-atom query
and dispersed filters admitting 2,893, 47,371, and 236,787 documents. The
direct-flat exact route originally discarded its already bounded selected-
superblock address cache, so exact record scoring repeated page-native
super-reference searches for every competitive block. Retaining that cache
does not change membership, bounds, impacts, or record scoring; its capacity
remains bounded by semantic runs times allowed superblocks under the existing
64 MiB statement work limit.

Warm p50 latency fell from 1,076.9/2,777.6/1,807.8 ms to
610.7/1,134.7/973.0 ms for the 0.6%/10%/50% filters, respectively. All 150
top-50 ranks, document ordinals, and double-precision scores were identical,
as were scored/skipped blocks and examined postings. Thirty consecutive 10%
filter queries in one Shadow backend held RSS between approximately 323 and
327 MiB rather than growing with query count. The staged lifecycle passed
56/56 gates. This validates the physical direction at medium scale, but full
production PubMed publication, route selection, and the Commons matrix remain
open before CQ-3 can close.

The full-corpus admission audit found one remaining statement-memory mismatch
before that publication completed. An 8.2-million-document score array consumes
about 31 MiB even when a selective filter admits only a small set of b16
blocks. Combined with the bounded selected-superblock address cache, that would
exceed the fixed 64 MiB query-work limit and silently discard the cache that
made the 500,000-document A/B faster. Filtered exact BMP now stores scores only
for the admitted b16 blocks, using the already required block-to-compact-block
map. The compact form is selected only when its score array plus map is smaller
than the corpus-wide score array, so broad filters do not pay an offset-table
penalty. On the 57,638-document FIQA root, a dispersed 0.6% filter reduced
statement work from 750,518 to 539,626 bytes; 10% and 50% filters retained their
previous 918,622/918,630-byte workspaces. All 60 compared top-20 ranks and
double-precision score bits matched the prior exact route, and the complete
convergent SAE lifecycle passed 56/56 gates. This is an admission fix, not the
full-scale CQ-3 latency result; the production-sized Shadow root remains the
required physical qualification.

The full-root same-generation A/B is now guarded by a reproducible isolated
runner. Baseline and candidate postmasters see their requested library through
a private mount namespace; the host `$libdir` is never replaced. The runner
refuses to stop a cluster with an active `CREATE INDEX`, verifies the isolated
data directory, port, and namespaced binary hash after every start, and records
the main Shadow postmaster PID plus host-library hash before and after the run.
It restores the isolated baseline on success or failure and writes a hashed
evidence manifest. A live Shadow dry run resolved the two frozen binaries and
all three SQL artifacts without touching either PostgreSQL instance. Execution
remains gated on completion of the full PubMed root.

The A/B now fails closed rather than treating its comparison log as acceptance.
It includes an unfiltered exact-BMP control under the same query and root,
requires bit-identical baseline/candidate ranks and score bytes, requires the
dispersed 0.6% filter to reduce both bound and posting work, and rejects an
unbounded repeated-query PostgreSQL memory-context or Linux backend-RSS range.
Every filtered control must remain on the exact BMP route without fallback,
stay within the 64 MiB statement-work limit, and avoid considering or scoring
blocks and documents outside its allowed subset. Normal-route latency SLOs
remain a separate CQ-7 gate so forced exact-path diagnostics cannot substitute
for application qualification.
The application-level Commons matrix remains separate CQ-7 evidence.

CQ-3 development now uses a scale ladder rather than waiting for every idea to
reach the production root. A current-format 20,000-document root rejects
incorrect routes and records per-query physical counters; a 100,000-document
root is admitted only when the first step is exact and predicts less work; the
existing 500,000-document PubMed evidence remains the medium-scale anchor.
These roots compare exact-BMP, automatic, direct-row, and transpose routes on
the same 0.6%, 10%, 50%, and unfiltered sets. They report rank and score-bit
equality, postings, bounds, bytes, documents, chunks, and statement memory.
Only ratios and work scaling are portable from macOS; absolute latency, Linux
RSS, cold page-cache behavior, fragmentation, and final route promotion remain
the authority of the permanent Shadow root.

The first scale-ladder run found a concrete planner defect on that 20,000-row
root. The automatic route attempted global ranked-prefix before loading the
same-root forward directory. At 10% selectivity this made automatic p50 10.5 to
14.5 times slower than forced transpose; at 50% it was 3.7 to 4.8 times slower.
The prefix and transpose paths examined a similar number of query postings, but
the failed prefix additionally read about 10 to 12 MiB of candidate forward
rows while transpose read only 0.7 to 1.1 MiB. The planner now loads the forward
directory and allowed-chunk counts first. It admits prefix only when the worst
planned oversampled candidate sequence, multiplied by the root's retained-row
cap, is below the selected direct-row or transpose lower-bound work.

A same-root candidate rerun over three queries kept the root generation,
contract, posting authority, accelerator source, ranks, and score bits stable.
Direct and transpose were bit-identical in every filtered case. Automatic 10%
filter p50 improved from a 116.5 ms median to 7.9 ms, or 13.56 times; the 50%
median improved from 68.2 ms to 14.3 ms, or 4.78 times. The 0.6% direct-row and
unfiltered controls showed no material route change. Automatic latency was
within about 2% of the best forced forward route, with zero rejected prefix
work. A 100,000-row rebuild adds no new decision evidence for this defect, so
the existing 500,000-row anchor and permanent Shadow root remain the next scale
gates. This closes the wrong call order, not all of `CQ-3A`: exact-BMP admission,
full-root Linux latency/RSS, and application-route promotion remain open.

The same scale ladder then found an independent structured-filter call-order
defect above the page planner. The AM layer attempted a global ranked prefix
before resolving a native scope bitmap from the same root. On a 2,000-document
current-format root with `pmid` included as scope metadata, 0.6%, 10%, and 50%
range filters therefore took 54.3, 52.3, and 28.5 ms p50. The equivalent
precomputed-TID routes took 16.7, 17.6, and 18.3 ms. The 10% prefix path read
about 1.80 MiB of forward data while the filtered transpose read about
0.19 MiB. This was not useful filter work: the native scope already represented
the exact allowed set.

Structured filters now resolve the same-root scope first. Any resolved scope
skips the AM prefix. A fully resolved scope enters the physical-cost page
planner immediately; a partial scope first intersects the SQL residual and then
lets that planner compare direct rows, transpose, or its own bounded prefix
against the final allowed set. Only a missing scope keeps the early AM prefix,
and only when the corpus is at least eight times larger than the first probe's
candidate budget. That budget follows the configured accelerator candidate
multiplier. Smaller roots complete the predicate directly because a global
probe cannot eliminate enough corpus work to repay its ranking cost.
On the identical root and binary pair, p50
fell to 16.2, 15.7, and 15.9 ms, improvements of 3.35, 3.32, and 1.80 times.
Structured and equivalent-TID rows plus score bits became identical. Against
forced exact BMP over three queries, O@50 stayed 1.0 for the 0.6% filter,
improved from mean 0.9333/minimum 0.86 to 1.0/1.0 at 10%, and remained mean
0.9933/minimum 0.98 at 50%. This removes both wasted work and the global-prefix
candidate bias; it is not a quality-for-speed exchange.

The partial-scope decision was then measured on a 20,000-document current-root
index with a production accelerator, `pmid` scope metadata, and three query
texts. The old AM prefix examined 3,749 documents and 9,711 postings and held
about 1.94 MiB before the final two to 165 allowed documents were known. Its
structured p50 was 62 to 83 ms, 3.1 to 4.2 times the equivalent-TID route. The
residual-first route removed that work and reduced p50 to 20 to 42 ms while
preserving every row and score bit. The remaining structured overhead tracks
the PostgreSQL predicate itself: 0.49, 2.60, 10.77, and 21.01 ms as the scoped
range grows from 120 to 20,000 rows. It is not hidden global ranking work.

The same root also proved that a no-scope prefix is uneconomic on a small
corpus. The `study` case fell from 82.0 to 38.8 ms and the `cancer` case from
61.5 to 38.5 ms after the physical-leverage gate skipped the global probe.
Large roots still retain the fallback when the corpus is at least eight times
larger than the configured first-probe candidate budget. Prefix candidate
matching now sorts TID references and uses binary lookup, replacing the prior
quadratic scan without changing duplicate-TID semantics.

The fresh staged package passes the PostgreSQL 18 build, C unit tests, all 324
Python tests, product inventory, and 66/66 real-model lifecycle gates. The
lifecycle proves zero AM-prefix attempts for fully or partially resolved native
scope and for uneconomic small-root no-scope predicates. CRUD, worker
convergence, VACUUM, restart, crash recovery, and scope storage/RSS plateau
remain green. The production-size Shadow root is still required for Linux
latency, cold cache, fragmented-root RSS, and final route promotion.

The scale ladder then isolated a separate residual-execution regression rather
than hiding it behind an exact-BMP route change. Commit `fbbaa6b2` correctly
bounded large-root memory by replacing the former dense residual accumulator
with a k-way document-slot merge, but applying that merge to every root changed
medium-root work from `O(postings + documents)` to approximately
`O(postings * log(residual runs))`. On the same current binary and FIQA root,
graph/protein queries therefore took about 144/117 ms instead of the historical
15 ms accelerator range. This was not a representation, filter, or cache
failure.

The query executor now estimates both exact strategies from already-open term
plans. It admits the dense accumulator only when its score array, posting
windows, bounded top-k heap, and result arrays together fit the existing 64 MiB
statement budget and its estimated work is no greater than the merge. The
bounded merge remains the only large-root fallback. Cancellation owns both
strategies through the same statement-context cleanup, and trace telemetry
records the selected strategy plus residual postings, documents, and scratch.

Same-binary current-format evidence supports the split. FIQA graph/protein p50
fell from about 144/117 ms to 14.9/14.3 ms, an approximately 9.7x/8.2x recovery,
using only about 465 KiB of residual scratch; the no-residual cancer query kept
its original term-at-a-time route. On the 20,000-document field-aware root,
unfiltered automatic p50 was 15-16 ms versus 18-20 ms for forced exact BMP,
while automatic 0.6%, 10%, and 50% filters were about 6.6-9.2, 7.6-8.7, and
13.5-14.7 ms. Across both roots, automatic overlap@50 against exact BMP stayed
between 0.98 and 1.0, all tested roots retained identical generation and
authority identities, and selective filters performed less ranking work than
unfiltered search. These measurements qualify the local cost decision, not the
permanent Shadow root's cold-cache, cancellation-RSS, or Linux promotion gates.
A clean PostgreSQL 18 build, C unit suite, all 324 Python contracts, and the
staged real-model lifecycle pass with 67/67 gates. The macOS cancellation-RSS
controller is intentionally not treated as evidence because it reads Linux
`/proc`; that gate remains attached to the Shadow run.

The expanded 20,000-document selectivity ladder found one more shared cost
defect below route selection. A dispersed filter containing only 20 documents
returned fewer than `k` positive scores, so every forward and exact-BMP route
entered filtered zero-score completion. The existing born-order COW cursor had
no filter summary with which to prune subtrees and therefore examined nearly
the complete document authority before finding all allowed zero-score rows.
This made the 0.1% filter slower than filters containing 200 or 400 documents,
even though the scorer itself read only 87 KiB of forward data.

Filtered zero completion now compares two exact physical costs before reading
documents. Born-prefix work is estimated as `needed * corpus / eligible`; the
alternative owns every record in each COW score block touched by the allowed
bitmap. When the latter is lower, one query-local document reader loads only
those blocks and keeps the earliest `needed` candidates by the existing
born-sequence/document-slot ordering. The general born-prefix cursor remains
the fallback. Both paths use the same immutable COW root, L0 shadow check,
liveness contract, interruption points, and bounded `2 * needed` candidate
storage; no artifact or lifecycle was added.

On the same root and old/new binaries, all six auto/exact-BMP result vectors
across three queries retained identical document ordinals and floating-point
score bits. The automatic p50 fell from about 17.2/22.0/20.0 ms to
5.75/5.84/5.77 ms, a 3.0x--3.8x improvement; exact-BMP improved by about
1.8x--1.9x. The new path read 20 document blocks and 2,464 COW records instead
of following born order across the root. Filters from 1% through 33% required
no zero completion and retained their existing route and latency range. The
no-match control also selected the intended opposite routes: 20 allowed rows
used block scan, while 200 allowed rows with `k=50` retained born-prefix and
examined 5,008 records rather than opening roughly 12,800 block records. The
query trace and scale runner now expose positive count, zero additions, COW
objects/records, heap peak, and document-block reads so this cost cannot be
hidden behind scorer latency again. A fresh staged real-model lifecycle passes
70/70 gates. This closes the local sparse-filter completion defect; Shadow
still owns the production-scale query and cancellation/RSS gates.

Two follow-up scale-ladder candidates were rejected rather than retained as
new heuristics. Raising the exact row-payload threshold from 32 to 64 reduced
the dispersed 1% direct-row byte count from about 1.71 MiB to 0.153 MiB, but
did not improve p50 and made some queries slightly slower: hundreds of tiny
payload reads replaced a few contiguous 8 KiB windows. Moving the
direct-versus-transpose boundary from the publication cap to that threshold
also changed the 1% route without a stable latency gain across three queries.
These results establish that byte count and allowed density alone cannot choose
the route. The existing contiguous reader and planner remain authoritative;
the next planner change requires term-local operation and posting estimates on
the medium and permanent roots, not another density threshold.

The next exact zero-completion slice removes already-ranked and L0-shadowed
documents before admitting a COW score block. This uses only the statement's
existing allowed set, positive result, and bounded L0 projection; liveness and
birth order remain owned by the same immutable COW records. On the unchanged
57,638-document FIQA root, a dispersed 0.1% filter touched 57 allowed rows.
The three queries reduced zero-completion block reads from 57 to 49, 14, and
30, and COW records from 7,296 to 6,272, 1,792, and 3,840. Automatic p50 was
unchanged for the first query and improved by 1.67x and 1.23x for the other
two. A 0.6% control required no completion and retained its physical work.
Old/new binaries produced bit-identical top-50 document ordinals and score
bytes for automatic and exact-BMP routes across all three queries. This is an
exact admission reduction, not a new artifact or routing threshold.

The permanent 7.79-million-document Shadow root then exposed a qualification
harness defect before any query A/B ran. The current folded v3 authority is
intentionally represented by a nonempty fragmented page-native manifest with
`segment_count=0`; the runner and SQL fixture incorrectly required at least
one immutable segment. Qualification now accepts either a nonempty segmented
root or a folded sole-authority root with a checked manifest, physical block
range, published high watermark, and posting records. It still requires a
permanent heap and index, valid/ready/live catalog state, a valid generation,
and nonzero sealed documents. Focused tests cover both legal root forms and
reject a synthetic root with neither authority. This is a harness-integrity
repair, not query-performance evidence; the same-root Shadow A/B remains the
promotion gate.

The first corrected run preserved all four top-50 result vectors and score
bytes, then found a second fixture-contract mismatch. The stable query trace
exports the final `query_route` and accelerator fallback state; the fixture
instead read two fields available only on a separate debug surface and treated
ordinary page-native/lexical `postings_examined` as proof of a semantic BMP
fallback. The exact gate now requires final route `semantic_bmp`, direct-flat
filtered execution, no accelerator fallback, bounded query memory, and block
and document work contained by the allowed subset. General posting work remains
reported but is no longer required to be zero. This preserves the physical
correctness contract without coupling qualification to an unrelated trace
schema.

The next gate exposed a related cross-route accounting error rather than a
query regression. It compared only semantic-BMP posting operations even though
the unfiltered control legitimately selected term-at-a-time execution. On the
full root the 0.6% filtered route examined 104,767 semantic and 1,487,958
page-native/lexical postings, while unfiltered examined 1,837 semantic and
52,839,212 page-native postings. Total posting work therefore fell by about
33 times, but the semantic-only comparison inverted that result. Qualification
now compares the additive semantic plus page-native posting work and retains
the independent bound-reference reduction gate.

The first full-root normal-route matrix then isolated a real planner cost. The
0.6% filter correctly selected direct rows at about 0.53 seconds, 50% completed
through one ranked prefix at about 0.99 seconds, and unfiltered used the normal
accelerator at about 0.18 seconds. The 10% filter, however, attempted two
prefixes before transposed scoring and took 2.61 seconds versus 1.77 seconds for
forced transpose. The prefix planner had sized its final sample for exactly
`k` expected matches, which has a high probability of falling short and
duplicating subset work. It now targets `2k` expected matches and issues one
probe at that power-of-two size. The existing same-root physical-cost gate
still rejects a probe whose worst candidate work is not below direct or
transpose, and any unsuccessful probe still falls back to the exact subset
path. This is a statistically motivated scheduling change, not a selectivity
threshold or result approximation.

The permanent 7,791,171-document Shadow root validates that scheduling change.
The 10% automatic route changed from two prefix probes followed by transpose
at 2,605 ms p50 to one successful prefix probe at 1,171 ms p50, a 2.23x
improvement. Prefix posting work fell from about 12.02 million to 6.01 million,
and the 50-result probe completed from 68,053 prefix documents without subset
fallback. The controls remained materially stable: 0.6% direct rows measured
542 ms versus 531 ms, 50% ranked prefix measured 1,022 ms versus 995 ms, and
unfiltered accelerator search measured 182 ms versus 184 ms. The small 20,000
document and 57,638-document FIQA roots retained their prior direct, transpose,
and unfiltered routes because the existing cost gate rejected prefix work.
The same-root exact matrix retained identical index bytes, root identity,
top-50 document ordinals, and score bits for every filter. Twelve repeated
cancellations left the backend reusable and its RSS plateaued at roughly
35--37 MiB after warm-up. This closes the duplicate-probe planner defect; it
does not replace the remaining application-level Commons and cold-start gates.

A second full-root boundary audit found two remaining routing defects. A
physically dispersed 3.125% filter contained 243,493 documents and sat almost
exactly on the former direct-row threshold. Direct rows took 1,925 ms and read
about 1.73 GiB, while transpose took 1,632 ms and read about 729 MiB. The old
threshold depended on the unrelated retained-candidate cap and selected direct.
The router now compares direct work, scaled by the published 2,048-document
forward chunk and its 32-row exact-read limit, against the documents in active
transpose chunks. Equal estimated work favors transpose because its access is
contiguous.

The same audit separated random and query-correlated filters before paying for
the full 1,024-result rescue. Capped rescues first request 128 global results.
They return immediately when those contain the filtered top-k, expand only
when the observed matches project at least `2k` matches in the final prefix,
and otherwise continue with the cheaper exact forward route. On the immutable
7,791,171-document root, the random 3.125% case changed from 2,740 ms to
1,946 ms p50, a 29.0% reduction, and selected transpose after one sample. A
query-correlated 472,064-document cancer-title filter changed from 1,122 ms to
484 ms p50, a 56.9% reduction, and completed from the sample. Both returned 50
results with 0.98 overlap against the forced exact route. The standard controls
remained stable at 498 ms for 0.6%, 1,168 ms for 10%, 1,028 ms for 50%, and
181 ms unfiltered. Twelve 10 ms cancellations left the backend reusable and
passed the 64 MiB tail-plateau allowance. Candidate SHA-256 is
`e9e91753db61dbba5196eaaf12ba1cdb21028f054ad34e554c6dd8873d0a4169`;
the final normal-route log is
`/data/ii42-builds/cq3-sampled-prefix2-final-normal-auto.log`.

#### CQ-4: Close cold-start and residency gaps

- [x] Define accelerator-ready separately from query-warm. Auto-preload must
  warm the pages and directories required by the admitted query route, or
  expose that warming is incomplete.
- [x] Measure the actual shared projections needed to retain every Commons
  business index without evicting the runtime safety margin. Reject arena
  sizing based on complete relation bytes when large roots use bounded query
  metadata and TID projections rather than resident folds.
- [ ] Admit the CA chunk index or prove a bounded disk-backed route. Large index
  state must remain shared; no PostgreSQL backend may acquire index-sized RSS.
- [ ] Test restart, preload completion, eviction, re-admission, and concurrent
  first queries. A cold query cannot silently become a 20-second prewarm job.

Completion gate: the first admitted query has a declared latency bound, warm
performance returns after restart without manual queries, and shared plus
backend RSS reaches a stable plateau.

The startup scheduler previously allowed auto-preload to acquire the same
per-index maintenance lock before a format-stale semantic accelerator was
republished. On Shadow PubMed this spent more than three minutes constructing
the reverse TID residency for a derived generation that maintenance would then
replace. Auto-preload now detects an eligible stale accelerator, marks the
normal maintenance hint, and yields without warming; healthy current roots
retain preload-first behavior. Build, focused scheduler contracts, and the
staged 52/52 lifecycle pass. Shadow restart-order qualification remains open;
the change will not interrupt the v4 publication already in progress.

The first query-metadata warm slice is now implemented locally. Bounded preload
prioritizes the current accelerator directory, scope header/column metadata,
and the complete v4 gram filter before spending the remaining budget on the
existing sequential or root walk. It uses checked object ranges from the same
published root and does not add an artifact or worker. The former one-byte warm
entry is now a versioned, root-bound marker reporting pages warmed and whether
filter-pruning metadata was complete; root successors retire rather than rekey
it. Runtime and manual-preload telemetry distinguish marker residency from
query-metadata warmth. macOS build, C unit tests, 50 focused contracts, product
inventory, and a staged shared-preload lifecycle closure pass. Shadow cold and
concurrent first-query qualification remains required before closing CQ-4.

A current-format 183 MiB FIQA root now also passes the isolated restart and
concurrent-first-query check. With `auto_preload = 100`, a fast PostgreSQL
restart restored a valid same-root warm marker, `query_metadata_warm = true`,
and 22 warmed query-metadata pages before any application query. Eight
simultaneous first queries all completed in 0.65--0.69 seconds. The shared
runtime reported eight successes, zero failures, two model-session loads, six
session-cache hits, a maximum queue depth of six, and no backend model loading;
subsequent queries stabilized at 26--37 ms. This proves the local scheduler and
shared-runtime contract, but not the production-size page-cache bound or the
production-size concurrent-first-query bound.

The live Shadow projection audit also closes the arena-sizing question. All
nine Commons business indexes report a resident same-root marker under the
existing 112 GiB arena. Large ArXiv, PubMed, CA-chunk, WA-chunk, and TX-chunk
roots retain bounded query metadata/TID projections; only the small TX-title
and system-chunk roots use resident folds. Global arena use is approximately
245 MiB with 21 ready entries and zero relation evictions, even though the
complete index relations total far more than the arena. The host has 248 GiB
of physical memory and was already using swap during the isolated full-root
build, so increasing the arena to 144/160/176 GiB has no evidence-backed
benefit and would reduce the operational safety margin.

The Commons matrix now records generation readiness and query warmth as
separate gates. Generation qualification still requires a healthy current
accelerator and complete forward view. Query-metadata qualification accepts a
current resident fold or a valid same-root metadata marker. Full query warmth
requires a current resident fold; large disk-backed roots are reported as
`metadata_only`, not warm, even when their pruning directories and TID
projection are ready. The evidence includes marker validity, warmed-page count,
resident/loading state, and shared TID projection count. An incomplete warm
state is therefore visible without being misreported as an accelerator failure.
Qualification now fails closed unless every requested preload has at least its
same-root query metadata ready before the matrix starts. The first serial query
or complete first concurrent wave must also finish within the declared two-
second cold-query ceiling; warm p50/p95 retain their case-specific bounds. This
prevents a nominally passing matrix from using its first application query as a
hidden prewarm job. A qualification environment that lacks a current catalog,
generation, query-metadata projection, planner health, or plan capture does not
execute ranked cases at all; `--plans-only` remains the non-warming diagnostic
surface. Full resident-fold warmth remains optional for large roots.
The matrix records contract version 6, and promotion/finalization reject older
evidence that predates the cold-readiness, restart-binding, and generated-
column schema gates even if it contains a stale `passed=true` value. Contract
version 6 additionally requires distinct first and warm waves and makes a
requested serial rank oracle fail closed when its SQL payload is absent. It
also rejects qualification while maintenance workers are disabled, the
semantic accelerator is forced off, or a diagnostic filtered forward route is
forced instead of the product `auto` route. Release
qualification records the
postmaster start time and requires each c1/c4/c8 panel to begin inside an
explicit `--require-restart-within-seconds` window capped at ten minutes. A
large root whose bounded query metadata cannot become ready inside that window
fails CQ-4 rather than weakening the restart evidence. Contract v3 also fixes
the complete eighteen-case Commons surface; an `--only` diagnostic or a
partial c1/c4/c8 panel cannot qualify a candidate or finalize the stable name.
This closes the observability and release-gate contract, not the remaining
production restart, eviction, and concurrent-first-query qualification.

The superuser-only query trace also reports the page-native scorer's bounded
`memory_bytes` estimate. Commons qualification can therefore distinguish
shared residency from statement-local query work and reject any route whose
backend workspace grows with total index size. This is telemetry only; it does
not alter allocation, scoring, or the public search result contract.

A subsequent bounded-preload audit found and closed one independent CQ-4
implementation defect. On a large compact initial fold, the preloader first
warmed semantic query metadata and then allowed the sequential fold pass to
consume the complete configured page budget again. One invocation could
therefore warm almost twice `ii42.prewarm_max_bytes`. The sequential pass now
receives only the pages remaining after metadata warming. A fragmented root
also no longer performs its unconditional meta-page read when semantic
metadata has already exhausted the budget. This preserves the single marker
and single preload lifecycle while making the configured budget a hard
per-invocation bound. Production restart and concurrent first-query evidence
remain required; this code correction alone does not close CQ-4.

The same audit closed the corresponding re-admission defect. Automatic preload
deliberately retains an incomplete same-root marker when the configured budget
cannot warm all required query metadata; this prevents an unbounded retry loop
and keeps the incomplete state observable. Previously that marker also made an
explicit `ii42_index_preload()` a no-op after an operator increased the budget.
The manual entrypoint now retires only an incomplete unified-warm marker and
retries under the current budget. Complete markers and the automatic scheduler
remain unchanged, so re-admission adds neither a second cache nor background
churn. The staged shared-preload closure calls the manual entrypoint twice on
the same oversized root and requires both attempts to consume the declared
128-page budget; an incomplete marker can no longer suppress the second
operator request.

The complete correction passes the PostgreSQL 18 PGXS build, C unit tests,
all 311 Python contracts, product-convergence inventory, and a fresh staged
real-model lifecycle with 64/64 gates. The staged binary, installed SQL, and
control file are byte-identical to their source build artifacts. These local
checks close the implementation defects; the production restart, eviction,
first-query, latency, and RSS observations remain CQ-4's release evidence.

The current combined route and lifecycle candidate also passes a fresh staged
real-model run with 70/70 gates. The lifecycle index explicitly opts into
`auto_preload = 100`; indexes with the default value of zero are not required
to recover a shared accelerator proactively. The test encodes one fixed query
before restart and separately verifies three contracts: the cold index is
immediately queryable with identical ranked document IDs, the accelerator-
disabled page-native route preserves rows and scores exactly, and the opted-in
shared route returns to the same route and product scores within five seconds.
It also retains CRUD, crash recovery, VACUUM, TID-filter exactness, scope
storage/backend-RSS plateau coverage, and a dynamic no-scope structured-filter
probe that must return its complete allowed set without ranked-prefix work.
This closes the local restart-contract ambiguity without weakening the
remaining production eviction, re-admission, CA-chunk, and cold-page-cache
gates.

A production-scale isolated restart on the permanent 7.79-million-document
PubMed root found and closed a separate first-query authority defect. The
ranked-prefix accelerator needs a same-generation document-length projection
to score lexical residuals exactly. Previously the first query backend built
that approximately 31 MiB shared projection lazily. Seven other simultaneous
backends observed the reservation as loading and fell back to document-major
transpose scoring. The first cold wave therefore performed about 357 million
posting visits: one 6,011,014-posting prefix probe plus seven 50,182,000-
posting forward fallbacks.

The preload worker now publishes the document-length projection before it
publishes the unified query-warm marker. Product query backends are attach-only;
the superuser test probe remains able to build the projection explicitly. Two
fresh isolated restarts reached a valid full-root warm marker, complete query
metadata, and two document-length entries before any ranked query. The second
restart reported readiness within 0.9 seconds of the first status poll. Eight
simultaneous first queries then all selected `ranked_prefix`, recorded 90 query
terms, 584 directory terms, 42 matched terms, and zero fallback. Their aggregate
work was 48,088,112 posting visits, an 86.5% reduction and approximately 7.4x
less work than the defective wave. The repeated panel measured 255.0 ms p50 and
286.4 ms p95; all 50 document ordinals and score bits matched the retained
same-root candidate exactly.

A 10 ms cancellation left no active backend or loading reservation, and the
next exact query remained on `ranked_prefix` at 249.7 ms. Twenty sequential
queries in one backend measured 217.3 ms p50 and 226.6 ms p95. Backend RSS rose
while shared pages were first touched, then plateaued at approximately
316.8 MiB; the final twelve samples changed by only 16 KiB. Shared arena use
remained 133.5 MiB with no reusable debt. The production Shadow postmaster was
not restarted or otherwise modified during this isolated qualification.

This closes the query-backend projection race and the corresponding hidden
index work. It does not yet close all of CQ-4: explicit eviction/re-admission,
the CA chunk stress root, and a controlled cold OS page-cache panel remain
release gates. A future cold-page improvement must remain worker-owned; it
must not reintroduce synchronous query-side projection construction.

The shared-residency recovery qualifier now makes the eviction/re-admission
contract reproducible without weakening that remaining production gate. It
requires an explicitly acknowledged isolated PostgreSQL port, superuser
access, no active index build, and no other active client before clearing the
single shared runtime. It accepts either a current resident fold or a valid
same-root page-metadata marker, but a semantic index must also produce its
query trace. An isolated 5,000-document resident-fold smoke cleared three
shared entries, observed the fold disappear, and then saw the existing worker
restore the same generation in 0.94 seconds. Eight simultaneous clients
preserved every rank and float4 score bit; first-query maximum latency was
14.6 ms, warm p95 was 9.8 ms, and all eight backend RSS/memory tails
plateaued. This validates the qualifier and worker-owned recovery mechanism.
It does not simulate OS page-cache eviction and does not replace the pending
production-size SAE, CA-chunk, or controlled cold-page panels.

The qualifier now fails closed unless the target itself is observed outside
the ready state immediately after the cache clear and returns through the
worker-owned path. A positive global clear count alone is not proof that the
named index was evicted. Semantic recovery also requires every concurrent
first and warm query to retain the baseline `query_route`, in addition to the
existing rank, float-score-bit, generation, latency, and RSS checks. Commit
`d9af99ed` records these gates.

The controlled cold-page companion is also reproducible without using the
host-wide Linux `drop_caches` interface. It requires an explicitly acknowledged
isolated PostgreSQL port, superuser access, no active index build or other
active client, a valid II42 generation, and ready shared query metadata. It
checkpoints that isolated instance, resolves only the target relation's main-
fork segment files, verifies their physical byte total against PostgreSQL, and
applies `POSIX_FADV_DONTNEED` only to those files. Batched `mincore` observations
record relation-specific residency before and after the operation. A Shadow
smoke reduced a 6,971,392-byte relation from 1,702/1,702 resident pages to
0/1,702 without changing its generation. The evidence is
`/data/ii42-builds/cq4-cache-smoke.json`, with SHA-256
`c8671d46868a054f25ced7aff7831863589402f7f60d1b199aa36c18cf42738b`.
The fixture used a small resident fold, so it validates the eviction mechanism
and safety guards only. The pending production panel must use a metadata-only
large root and immediately run the existing distinct first/warm query waves;
the eviction artifact alone is not latency or exactness evidence.

Commit `35d9424a` closes a publication-race gap in that companion. After
`POSIX_FADV_DONTNEED`, it reopens the isolated catalog and requires the same
generation identity, relation OID/path/size, valid generation, and ready query
metadata before measuring the final cold residency. The post-check residency,
not the unverified instant after `fadvise`, is now the starting state handed to
the query panel. Any concurrent maintenance publication therefore invalidates
the artifact rather than mixing two roots.

The original small smoke also failed to exercise PostgreSQL's segmented
relation filenames. The collector interpreted `.1` as logical segment 2 and
would reject every normal relation larger than one gigabyte as incomplete.
Commit `ab95ab7a` maps the base file to segment 0 and suffix `.N` to segment N,
while retaining the gap check; a base plus `.1` and `.2` fixture now passes and
a base plus `.2` fixture still fails. This removes a deterministic blocker from
the production PubMed cold-page panel.

The production restart also exposed two small roots whose initial fold still
contained semantic BMP subformat v1 while the only product reader accepts v2.
Their shallow readiness state remained healthy, but deep fold attachment failed
and prevented the preload scheduler from reaching lower-priority roots. Raw
page inspection confirmed intact page envelopes and checksums; this was stale
publication format, not page reuse corruption. Rebuilding the 18,981-document
TX title root and 1,381-document system-chunk root with the current binary took
23.5 and 7.3 seconds. Both then passed complete closure audit and entered shared
resident state. No v1 reader was added. The fold catalog collector also now
materializes a minor-only neutral fold instead of leaving its counted slot
uninitialized, and attachment errors retain the failing term, run, object
reference, and stage. This keeps obsolete or malformed folds fail-closed and
actionable without adding a compatibility lifecycle.

A later Shadow restart exposed one more preload-order defect on the 765,095-
document CA field-aware semantic root. After an operator raised
`ii42.prewarm_max_bytes`, a complete same-root unified marker incorrectly made
resident-fold admission appear complete. Commit `deddf2d5` makes preload
admission kind-specific, so the existing worker admitted the 174.7 MiB fold
without rebuilding the index. The same restart then showed that successful
fold publication returned before publishing the document-length projection
required by exact semantic-accelerator residual scoring. Product queries fell
back from `semantic_accelerator` to `semantic_bmp` at approximately 733 ms.

Commit `f9e7d345` keeps that projection restart-ready for semantic roots even
when a resident fold is admitted. It adds no cache or worker and does not alter
root bytes, query scores, or WAL authority. The isolated real-model lifecycle
passes 70/70 gates. On Shadow, the worker restored the fold and the sixth
document-length projection before any manual preload; the first query finished
in 1.07 seconds and five warm queries stabilized at 451--454 ms on
`semantic_accelerator` with no fallback. The current ORT 1.29 binary SHA-256 is
`f9110fbb1048d1759aa05fc8abb327b90a5dae2d7a63d2c68454379fd6d555cc`.
Evidence is retained under
`/data/ii42-runs/cq4-resident-projection-f9e7d345-20260825`.

The independent CA policy union remains above its 300 ms application gate:
six stable attempts measured 685.9 ms first and 688.4 ms warm p50. That case
serially searches the document and chunk indexes and is a CQ-7 application
cost, not evidence that preload recovery failed. CQ-4 therefore retains its
remaining CA-chunk stress and controlled cold-page acceptance gates.

#### CQ-5: Restore PostgreSQL metadata planning health

- [x] Run targeted `ANALYZE` on the Commons filter columns and confirm normal
  autovacuum/analyze policy will refresh them after ingestion.
- [x] Record plans for selective and broad date, category, organization,
  journal, and document-id predicates before and after statistics refresh.
- [x] Treat statistics as planner support, not as a substitute for a bounded
  II42 filter route. Qualification must remain correct after statistics age.

Completion gate: production tables have current statistics, expected metadata
indexes are selected where appropriate, and performance does not depend on a
one-off manual planner setting.

A read-only Shadow audit found that this is an active product blocker rather
than a precautionary gate. `data_arxiv`, `data_pubmed`, and the CA/WA policy
tables report `reltuples = -1`, no analyze timestamps, and no `pg_stats` rows
for the Commons filter columns; only the much smaller TX policy table retains
statistics. The main instance also has `autovacuum = off` in
`postgresql.auto.conf`. The expected B-tree, GIN, and trigram indexes exist,
but plans therefore depend on default cardinality estimates. Do not restore
autovacuum or start a large `ANALYZE` while the isolated full PubMed root build
is consuming the same host I/O. After that build finishes, remove the temporary
override, refresh only the declared Commons filter columns first, and record
the selective and broad plans before changing any scorer threshold.

Elm provides the healthy comparison point: autovacuum and `track_counts` are
enabled, and `pg_stats` contains the declared arXiv and PubMed date, category,
organization, and journal columns. Its selective plans use the expected
B-tree, GIN, and trigram indexes; broad date predicates may correctly choose a
sequential scan. Shadow's stale estimates still find several selective indexes,
but PubMed date-plus-journal does not form the same bitmap intersection. macOS
`ii_dev` has no Commons schema and is therefore not a planner-health surface
for this gate.

The Commons matrix now records `EXPLAIN (FORMAT JSON)` for every declared
filter predicate before running ranked queries. It preserves the complete plan
and a compact node/index/estimate summary, so targeted statistics refreshes can
be compared without executing the predicate. A read-only Elm run captured the
expected selective B-tree, GIN/trigram, and bitmap plans while broad arXiv and
PubMed date predicates correctly selected sequential scans. It also failed
closed on the PubMed partial-date case because Elm has not yet installed the
generated `publish_date_start_bound`, `publish_date_end_bound`, and
`publish_date_has_day` columns. This is deployment schema drift, not a slow
planner shape, and must be resolved before Elm can qualify.

The statistics refresh is also versioned and fail-closed. It requires normal
`autovacuum` and `track_counts`, verifies every declared Commons filter column,
and analyzes only those heap columns. It does not alter planner switches or
scan II42 relations. An Elm smoke stopped on the first missing generated column
before any `ANALYZE`, confirming that schema preparation must precede the
statistics and matrix gates.

The release matrix and statistics refresh also verify that the three PubMed
partial-date columns have the expected types, are stored generated columns,
and retain the canonical generation expressions. This prevents an idempotent
`ADD COLUMN IF NOT EXISTS` deployment from silently accepting a same-named but
incompatible column. Matrix contract v4 makes this schema check mandatory for
promotion and post-swap finalization.

The Commons matrix now also fails planner qualification when autovacuum or
`track_counts` is disabled, a declared filter table has unknown cardinality,
or any declared filter column lacks persistent `pg_stats`. The
`last_analyze`/`last_autoanalyze` activity timestamps remain diagnostic only:
PostgreSQL can reset them at restart while retaining `pg_class.reltuples` and
the `pg_statistic` catalog used by the planner. Requiring those timestamps
would therefore make a healthy restart fail CQ-5. The plan capture remains
descriptive rather than requiring one brittle node or index name: broad
predicates may legitimately select a sequential scan. This turns CQ-5 into an
environment gate without confusing planner support with scorer correctness.

For the required before/after audit, `--plans-only` captures the same catalog,
generation, statistics, residency, and `EXPLAIN` payloads without executing a
ranked query. It reports capture success separately but always keeps matrix
`passed=false`; promotion accepts only `mode=qualification`. This allows the
stale-statistics baseline to be preserved without warming query pages or
turning diagnostic evidence into release evidence.

The historical contract-v4 Shadow before snapshot is stored at
`/data/ii42-builds/cq5-shadow-plans-before-v4.json` with SHA-256
`47321ad2df97821f7f53af0c0810ff0cf3121c5c078cc63e126d679cd51c6ff1`.
It captured all ten declared planner cases, passed the canonical generated-
column schema contract, and failed planner qualification for the expected
pre-refresh reasons: `autovacuum = off`, unknown table cardinalities, missing
analyze evidence, and missing `pg_stats` rows. No ranked query was executed.

After the isolated build released host I/O, Shadow restored `autovacuum=on`,
kept `track_counts=on`, and ran targeted `ANALYZE` only for the declared
Commons filter columns. The contract-v5 plans-only capture passed planner
health for all ten declared cases. In particular, PubMed date plus journal now
uses a bitmap intersection of the date B-tree and journal trigram index;
selective organization and document-id predicates also select their expected
indexes. Broad predicates remain free to choose wide bitmap or sequential
plans. The after snapshot is
`/data/ii42-builds/cq5-shadow-plans-after-v5.json` with SHA-256
`dad6fb5bd38e2a91e1a80b6dc7be4246eb4cd1d322d5c5699251541d3d6d740b`.
This closes statistics refresh and plan capture, but not the remaining aged-
statistics and ranked-query qualification gate.

A later Shadow postmaster restart reset all six required tables'
`last_analyze` activity timestamps while retaining nonnegative `reltuples`,
every required `pg_stats` row, and the same ten healthy planner shapes. The
corrected contract-v5 plans-only capture remains planner-, schema-, and
catalog-qualified at
`/data/ii42-builds/cq5-shadow-plans-postrestart-v5.json`, with SHA-256
`1f39c48ec57c58745924dc63c505837402aac49ff1e0f4df2624b93e7c73e9b7`.
This proved restart-safe planner qualification. At that point the separate
aged-statistics observation and ranked-query gates still remained.

The contract-v6 aged-statistics capture was taken approximately 56,730 seconds
after the restarted Shadow postmaster began serving. Without another
statistics refresh or any session planner override, catalog, generated-column
schema, all nine index generations, query metadata, runtime policy, persistent
statistics, and all ten planner cases remained qualified. The artifact is
`/data/ii42-builds/cq5-shadow-plans-aged-v6.json`, with SHA-256
`65e48e53daaef16ce6dbfc66153edc0d467f8e517f1749838d24bb9f569b8b1a`.
It is deliberately `mode=plans_only`, records no ranked case, and keeps the
matrix-level `passed=false`. This closes CQ-5's statistics-age and planner
support gate without turning statistics into scorer authority. Ranked latency
and result exactness remain CQ-7 requirements on the newly published root.

#### CQ-6: Prove lifecycle correctness for filter scope

- [x] Extend CRUD tests so inserts, updates, deletes, aborted transactions,
  savepoints, and 2PC change filter membership under the correct MVCC snapshot.
- [x] Verify lexical-first visibility and eventual semantic completion for new
  rows without creating a second filter lifecycle.
- [x] Verify compaction, fold, VACUUM, restart, replication, and REINDEX retain
  scope metadata and accelerator/source-manifest identity.
- [x] Stress mixed CRUD plus filtered queries until relation bytes, retired
  ranges, shared residency, backend RSS, and query latency converge.

Completion gate: exact filter results survive the complete mutable lifecycle
and storage/RSS plateau tests without orphan pages or a manual repair step.

The staged convergent SAE lifecycle exercises filter membership inside the
writing transaction, across a metadata UPDATE rolled back to a savepoint, and
after the whole transaction aborts. It passed all 53 gates together with the
existing INSERT/UPDATE/DELETE, semantic completion, VACUUM, REINDEX, immediate
crash, and cold-restart checks. The physical replication smoke now also passes
scope membership across prepared rollback/commit, linked-L0 replay, eventual
completion, UPDATE/DELETE, REINDEX, and index DROP. That run exposed and closed
an accelerator scope-publication lock inversion: the derived publisher now
conditionally admits the heap lock and defers behind DDL rather than deadlocking
while holding the index lock. Fold identity and mixed-CRUD storage/RSS plateau
qualification remain open.

The extended lifecycle stress exposed an independent convergence defect in
scope-less SAE indexes. Accelerator freshness had started requiring a current
scope object even when the index had no `INCLUDE` columns and therefore had no
scope to publish. Every maintenance pass consequently republished the same
derived accelerator; a 17-row fixture grew from about 300 KiB to 23 MiB before
the bounded test stopped. Freshness now checks the scope format only when the
index actually has scope columns. A post-convergence maintenance probe must
return without another accelerator publication. The fresh staged package
passed the 70/70 base lifecycle and the 73/73 extended closure with 500 stable
queries, 30 mixed CRUD cycles, four readers, four writers, 20 writer cycles,
zero first-attempt visibility misses, and 492/492 successful runtime requests.
The same run keeps the strict VACUUM-to-REINDEX physical-shape gate, but permits
bounded VACUUM retries when PostgreSQL's first cleanup is held behind an older
visibility horizon.

The scope-specific closure now repeats twelve metadata-only UPDATE/VACUUM
cycles against a fixed live set after REINDEX, structural convergence, and a
cold restart. Each settled query uses current scope metadata, each accelerator
source manifest advances, and every VACUUM returns the sealed generation to
three live documents. Relation size stayed exactly 2,367,488 bytes and the
physical high-watermark stayed at 289 blocks for all twelve cycles. Backend RSS
rose from about 66 MiB while warming, then plateaued at about 70 MiB over the
final six cycles. The staged lifecycle passed 55/55 gates. Together with the
existing replication scope checks, this closes CQ-6 without a second filter
lifecycle or a manual repair step.

The forward-bound publication work later exposed a CQ-6 regression that the
earlier fixture could not cover. Forward-bound shards were referenced by the
accelerator directory and used by queries, but neither retirement nor exact
reachability enumerated those child objects. Each accelerator replacement
therefore left roughly 900 blocks outside reclamation authority. The lifecycle
now inventories every forward-bound shard in both closures, and the product
contract requires the two authority paths to own the same accelerator children.
An isolated PostgreSQL 18 run on Shadow passed 70/70 gates and twelve repeated
scope UPDATE/VACUUM cycles. Relation size stayed exactly 39,280,640 bytes,
physical size stayed at 4,795 blocks, and every settled root returned to one
retired range containing one block. Backend RSS warmed from 91,885,568 bytes to
100,212,736 bytes and then remained unchanged for the final eleven observations.
The evidence is `/data/ii42-builds/cq6-forward-bound-lifecycle-12cycle.json`
with SHA-256
`b156a03ef57183e21a4da25c8fa45bc8650e48d88bec1934a5d573fca6836b98`.

The old-snapshot seal-fence gate now tolerates only bounded `lock_busy`
contention from the active maintenance supervisor. It still requires the old
reader to hide the newly committed TID, a new reader to see it, sealing to wait
for the safe XID horizon, and the pending frontier to converge into a primary
segment after the reader releases its snapshot. Three consecutive isolated
PostgreSQL 18 runs passed all 85 lifecycle gates. This removes a test-owner
race without weakening the MVCC or convergence contract.

#### CQ-7: Qualify Shadow and perform the PubMed swap

- [ ] Run the full matrix at concurrency 1, 4, and 8 with cold and warm panels.
- [ ] Require zero predicate violations and symmetric-difference zero against
  the SQL allowed-set oracle for sampled and boundary cases.
- [ ] Require the normal-route trace to record the measured-work decision and
  reject a medium or broad forward scan when the same-root exact block-major
  estimate is lower. A qualified selective route must use filtered exact BMP
  or a ranked prefix that completes its exact proof; route admission must not
  depend on allowed-document count alone.
- [ ] Require warm p50 at or below 300 ms for ordinary unfiltered queries,
  500 ms for selective filters, 1 s for broad filters, and 2 s for the explicit
  large-document stress case. No qualified query may time out; p95 must remain
  bounded under the declared concurrency.
- [ ] Build an immutable staged package and verify source, binary, installed
  SQL, model, and index generation identities together.
- [ ] Only after every gate passes, publish the current PubMed candidate under
  `data_pubmed__title_abstract__field_aware_bm25_idx`, run the actual Commons
  CTEs, and retain a rollback name until the post-swap smoke succeeds.

Completion gate: Commons uses only stable product index names and every
application query shape passes exactness, latency, lifecycle, restart, and
concurrency gates on the same binary and index generation.

The read-only Commons matrix runner now supports explicit concurrency waves.
At concurrency greater than one, each wave launches independent PostgreSQL
sessions, preserves every backend-local II42 trace and exact membership/rank
oracle, treats the complete first wave as concurrent first-query evidence, and
summarizes later waves separately as warm load. Concurrency one retains the
previous output contract. A case passes only when warm p50 meets its declared
SLO and warm p95 remains within twice that bound. This makes the required 1/4/8
matrix reproducible; it does not qualify Shadow until the full-root run is
executed.

A scorer-only 1/4/8 concurrency diagnostic on the immutable 7.79-million-
document root confirms that the sampled-prefix router is stable under load,
but does not close the Commons matrix. At concurrency eight, p50 was 805 ms for
the 0.6% direct-row filter, 1,832 ms for the 10% ranked prefix, 1,182 ms for the
50% ranked prefix, and 288 ms unfiltered. Every request returned 50 results and
kept the same route. The artifact is
`/data/ii42-builds/cq3-sampled-prefix2-concurrency.json` with SHA-256
`db621be27930e7a836bac9c01ca09dfa370870354373966c5d833da822001599`.
This is useful route-cost evidence, not a release matrix: it omits SQL predicate
construction, cold-start panels, and the five named Commons oracles. It also
shows that ordinary prefix tuning cannot close the remaining broad-filter SLO;
both 10% and 50% routes still examine about 6.01 million query postings. The
next broad-filter improvement must reduce that traversal or prove a cheaper
same-root block-major route rather than adding another selectivity threshold.

A same-root filter-aware prefix candidate now rejects documents outside the
allowed set before reading their forward rows. It does not add a publication
artifact, change the route thresholds, or alter direct, transpose, exact-BMP,
or unfiltered execution. On the permanent 7.79-million-document PubMed root,
the 10% filter reduced forward bytes from 589,407,818 to 81,989,146 and forward
row reads from 68,053 to 6,679. Warm p50 fell from 1,175.890 ms to 554.284 ms,
a 52.9% reduction. The 50% filter reduced forward bytes from 206,440,074 to
115,582,466, but p50 fell only from 1,046.261 ms to 985.409 ms because both
routes still examined 6,011,014 prefix postings. The 0.6% direct route and the
unfiltered accelerator remained statistically unchanged. The complete forced
exact comparison returned the same 50 document ordinals and score bits for all
four filters, retained an identical generation and relation size, passed the
memory plateau gates, and left the production postmaster and host binaries
unchanged. The evidence is in
`/data/ii42-builds/cq3-full-filter-aware-ab`; the candidate binary SHA-256 is
`6d47852a5e080aa1e5b254dcb46811d8bb2181012cc0c156278ed699c4ff701b`.

The candidate therefore closes the erroneous forward-row work in ranked-prefix
execution and is retained. It does not close the broad-filter SLO. The next
slice must reduce the fixed prefix posting traversal or select a cheaper
same-root physical route from measured term/block work; another allowed-count
threshold is not sufficient.

A follow-up candidate applied the allowed filter while accumulating residual
scores. It was rejected: because the residual heap retained its global capacity,
it became populated entirely by allowed documents and expanded final forward
row reads instead of reducing them. On the 10% filter, p50 regressed from about
554 ms to 1,064 ms and forward row reads rose from 6,679 to 65,734. This proves
that per-posting membership checks are not sufficient; a future block-aware
route must jointly bound decode work and the allowed residual heap.

The retained follow-up instead samples the existing filter-aware prefix before
allocating the full residual heap. For `k=50`, 64 is the minimum power-of-two
sample. Sample support still controls expansion and the exact subset scorer is
still the final fallback. A same-root automatic-product A/B on the permanent
7.79-million-document PubMed root compared all 200 ranked TIDs and score bytes
with zero mismatches. The 10% filter reduced p50 from 540.628 ms to 474.056 ms
and p95 from 642.736 ms to 522.829 ms. Forward bytes fell from 81,989,146 to
10,705,764 and prefix documents from 68,165 to 6,734. The 50% filter reduced
p50 from 977.561 ms to 938.099 ms, p95 from 1,017.541 ms to 962.670 ms, forward
bytes from 115,582,466 to 50,728,444, and prefix documents from 19,003 to 6,730.
The selective direct-row and unfiltered controls retained identical physical
work and changed only within run noise. The medium 500,000-document root also
kept identical results, routes and physical counters. Evidence is in
`/data/ii42-builds/cq3-full-product-sample64-final` and
`/data/ii42-builds/cq3-medium-product-sample64`; candidate SHA-256 is
`2171e0920d99ff00eb5a60ea1f996d5bfcda492becc8c2c4ac23d5f6fdd20286`.

This closes the avoidable prefix oversampling but not the fixed 6,011,014
posting traversal. The next structural CQ-3 slice must make allowed membership
available before block/posting decode and must size the allowed residual heap
from the same cost model. Threshold-only and contribution-level filters are
stopped by the evidence above.

A subsequent scale-ladder audit isolated one avoidable case before that format
work: an indexed SQL predicate whose complete allowed set is smaller than the
fixed first ranked-prefix traversal. The AM now probes at most 65,537 TIDs under
the caller's MVCC snapshot. A complete result of at most 65,536 documents enters
the existing direct/transpose cost planner; an incomplete probe is discarded
and the bounded ranked-prefix route remains authoritative. This is not a score
or selectivity threshold: both branches remain exact, the probe has a fixed
memory/work ceiling, and broad predicates retain the existing route.

On the current-format 500,000-document PubMed root, warm 0.6% filters scored
2,847 documents in 16 to 19 ms and 10% filters scored 47,446 documents in 74 to
84 ms. Both avoided ranked-prefix work. The 50% filter stopped after 65,537
probe rows and retained one ranked-prefix attempt at 195 to 231 ms. On the
7.79-million-document root, the dispersed 0.6% case scored 46,748 documents in
about 437 to 487 ms with zero prefix attempts, versus about 7.6 seconds and
18,033,042 prefix postings on the superseded call order. That is about a 16 to
17 times latency reduction. The 10% and 50% controls exceeded the bounded
probe and retained ranked-prefix, so the change does not turn broad filters
into an unbounded forward scan.

Query trace now reports `filter_probe_attempted`, `filter_probe_complete`, and
`filter_probe_rows`. Together with ranked-prefix and forward counters, these
make the routing cost observable instead of inferring it from latency. This
closes the known missing-scope selective-filter call-order defect. It does not
close all of `CQ-3A`: the 65,536 ceiling still requires the production Commons
matrix, and scope-backed block elimination remains the structural route for
larger selective subsets.

The 47,446-document medium subset and 46,748-document full subset were also
compared with an explicit `tid[]` under the same binary and MVCC snapshot. Both
returned 50 rows with zero rank, TID, document-ordinal, or score differences.
Twenty followed by fifty forced 2 ms cancellations left each backend usable;
the final 25 cancellation samples had a zero-KiB RSS range after the initial
allocator plateau. This qualifies the new probe's exactness and cancellation
cleanup in isolation, but not yet concurrent production latency.

The 500,000-document root now also has a bounded c1/c4/c8 concurrency panel
using indexed `pmid` ranges and independent sessions. The 0.6% case retained
`forward_rows` with zero prefix attempts: p50 was 31.1, 34.5, and 33.5 ms at
c1, c4, and c8, versus 71.0, 69.8, and 69.5 ms for unfiltered accelerator
search. The 10% case retained `forward_transpose` at 154.7, 179.2, and 188.5
ms. The 50% case stopped at row 65,537 and retained ranked-prefix at 219.1,
251.9, and 270.9 ms. Every request kept one stable route and returned 50 hits.
The corresponding indexed metadata probes alone measured 0.28 ms for 3,000
rows, 4.36 ms for 50,000 rows, and 7.23 ms to prove the 65,537-row cap. Thus
the remaining 10% cost is the measured 3,285,170-posting transpose scorer, not
predicate construction. This qualifies bounded concurrency on the medium
root; it does not replace the named Commons production matrix or solve the
larger allowed-block elimination problem.

An intermediate full-root audit treated transpose scratch as one int8 lane per
active document and query term. That candidate improved the observed route, but
a later line-level audit showed that this was not the executor's allocation
shape: page-native transpose holds one chunk of scores, scales, and dense codes
at a time, and reads only the current query terms' sparse ranges. The reported
187--220 MiB value was therefore conceptual work, not live statement memory.
It is retained here as historical A/B evidence but is superseded by the v6/v7
measured cost models below.

A same-binary full-root A/B over three query shapes returned all 150 top-50
document ordinals and score bits identically. Automatic p50 changed from
458.1 to 308.2 ms for `cancer`, 427.7 to 328.0 ms for `graph`, and 469.9 to
326.2 ms for `protein`, a 23.3--32.7% reduction from the recorded panels.
Independent randomized pgbench waves reduced average latency from 502.9 to
335.2 ms at c1, 633.1 to 545.5 ms at c4, and 697.1 to 545.6 ms at c8, with no
failed requests. After excluding the first transaction in each session, warm
p50 changed from 466.4 to 348.5 ms at c1, 701.5 to 580.8 ms at c4, and 643.8
to 510.2 ms at c8; warm p95 changed from 508.9 to 359.4 ms, 908.9 to 596.9 ms,
and 753.9 to 554.3 ms respectively. The concurrent p50 values are improved but
still do not close the 500 ms CQ-7 gate. The 500,000-document 5,000-row
controls remained on transpose at 19--21 ms because their estimated lane
streams were only 22--27 MiB. Ten- and fifty-percent full-root filters still
exceeded the bounded TID probe and retained ranked-prefix execution.

The direct-row audit then separated two previously coupled physical
crossovers. That checkpoint used a 128-row-per-forward-chunk density model,
while an already-selected direct route may read exact row payloads through 256
selected rows per chunk. On the same full root this reduced forward bytes from
242,894,024 to 63,920,544, or 73.7%, without changing the selected route. The
three-query p50 panel changed from 358.0/360.7/362.2 ms to
266.5/289.0/290.3 ms, a 19.9--25.6% reduction, with all 150 top-50 document
ordinals and score bits identical. A new c1/c4/c8 panel measured warm server
p50 at 339.8/474.0/463.4 ms and p95 at 349.3/495.8/499.4 ms. Relative to the
preceding planner binary, c4 p50 improved 18.4% and c8 p50 improved 9.2%.

Keeping these crossovers separate is required by the medium-root control. If
the 256-row I/O crossover is reused as the route-density estimate, the
500,000-document 10% filter incorrectly changes from transpose at about 44 ms
to direct rows at about 155 ms. With the split thresholds, the 0.6%, 10%, and
50% medium controls retain their established routes at 17.3, 44.2, and 74.4
ms p50. This is a measured scale crossover rather than a result-dependent or
dataset-specific policy. Evidence is in the isolated Shadow database under
the `exact256_route128` label and in
`/data/ii42-builds/cq3-exact256-route128-concurrency.json`.

A denser scale sweep subsequently showed that the 128-row density proxy still
underestimated direct-row CPU work. On the same 500,000-document root, three
query shapes and ten filter densities were measured under one binary with
forced direct and transpose controls. At 2.5% and 5% selectivity, automatic
direct-row p50 was 49.20 and 87.95 milliseconds, while transpose was 33.19 and
50.70 milliseconds. The intermediate planner used
`allowed rows * retained cap`, moved 1%, 2.5%, and 5% filters to transpose, and
reduced p50 by 9.1%, 33.4%, and 42.0%; 0.1% and 0.5% filters remained direct.
All tested top-50 ordinals and score bits were identical. That retained-cap
proxy was useful but not physical: complete forward rows can contain more or
fewer postings than the candidate-retention cap. It is superseded by the v6
publication count below without changing the independent 256-row exact-data
read threshold.

The subsequent current-writer rebuild exposed an independent remote-runtime
scheduler defect. URL port parsing released its temporary port string before
checking the `strtol` end pointer. The resulting use-after-free rejected valid
numeric endpoints without making a socket call, counted each rejection as a
connect failure, and eventually placed every healthy remote service in
backoff. A same-host isolated A/B built the same 20,000-document table against
the same 21 services. The old binary took 245.71 seconds, completed only 32
remote batches, recorded 231 false connect failures, and left all 21 services
in backoff. The corrected binary took 26.94 seconds, a 9.12x improvement,
completed all 5,000 batches remotely, and recorded zero connect or request
failures. The change only makes port validation precede deallocation; it does
not alter model identity, runtime precision, posting bytes, query scoring, or
publication authority. The full Shadow rebuild remains the production-scale
throughput and final-root qualification gate.

The retained-cap route candidate also completed the permanent-root product
matrix rather than relying only on the medium-root crossover. On the same
7,791,171-document root, automatic p50 was 173.99 milliseconds for the 0.6%
direct-row filter, 224.00 and 445.20 milliseconds for the 10% and 50%
ranked-prefix filters, and 187.14 milliseconds for the unfiltered accelerator.
All 200 top-50 document ordinals and score bytes matched the preceding product
candidate. This confirms that the 33.4% and 42.0% medium-root improvements do
not move the production-sized broad-filter routes or alter ranking.

Repeated five-millisecond cancellation on that root covers automatic direct
rows, forced transpose, forced exact BMP, filtered exact streaming, and
unfiltered term-at-a-time scoring. Each route survived twelve cancellations in
one backend, remained reusable, and reached a stable RSS plateau after the
allocator high-water mark. A final production-sized four-atom, two-field query
then independently selected ordered-block scoring. It considered 60,781
blocks, scored 27,603, skipped 33,178, and used 3,533,305 bytes of traced query
memory. Twelve more five-millisecond cancellations in one backend reached a
34,607,104-byte RSS plateau and left the backend reusable. Evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-ordered-route.txt`,
SHA-256
`3b4c4496ce0522c1a5ced164ea041801274cc9fc09d6100ea8208721630a9e2c`,
and
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-ordered-cancel-rss.txt`,
SHA-256
`1fb75ba3dd8981dde812fb88614074f2c253950bb3a417d58f62c0ab9afd0c7d`.
This closes `CQ-3F` and `CQ-3H`; cancellation ownership no longer depends on
the selected exact scorer.

Deleting the exact filtered scorer's bounded super-reference cache was also
tested and rejected. It saved about 17 MiB of statement memory but repeated
page-native record searches made the same exact route 3.16 times slower while
changing zero results. A future direct block-to-record locator would therefore
require a same-root format change; removing the cache is not a valid query-only
optimization. These experiments qualify the new complete-probe route planner,
but they do not replace the named Commons production matrix or the remaining
scope-backed block-elimination work.

The v6 scale ladder first measured both forward executors from their serialized
layout. Publication records each forward chunk's complete posting count during
the existing validation pass, so direct work is the chunk posting count scaled
by selected rows. That estimate tracked observed row postings to about one
percent on the 20,000- and 500,000-document roots. The first query-aware
transpose planner, however, reopened every active forward chunk to inspect
query-term ranges. It read 19--23 MiB of planning metadata per 500,000-document
query and would have grown linearly with root chunk count. That planner was a
useful oracle but is not the product route.

Directory v7 instead accumulates one model-vocabulary-sized transpose work
table while the existing forward artifacts are validated. Sparse terms add
`end - start`; dense terms add one chunk document lane. Query planning scales
only its query terms by active-chunk document coverage, then charges one
additional lane operation per active document for transpose initialization and
reduction. Direct rows retain the per-chunk posting estimate. Scratch remains
bounded by the largest active chunk plus fixed workspace, and the table is
stored under the same accelerator directory, root, WAL, retirement, and
reclamation authority.

On the 500,000-document root, the fixed table is 1,276,592 bytes versus
19--23 MiB for the v6 query scan, a median 93.4% planning-byte reduction. A
maintenance-only publication produced the v7 directory in 81.3 seconds with a
260 MiB workspace estimate and no semantic inference or primary-root rebuild.
The first v7 panel exposed one missing physical term: transpose must initialize
one score lane per active document even when query postings are sparse. Adding
that work changed the incorrect 1% graph-query choice back to direct rows and
reduced the fixed-order panel's maximum route regret from 17.3% to 4.2%.

The deterministic route-order-balanced panel used three query shapes and
0.5%, 1%, 2.5%, 5%, and 10% filters. On the 20,000-document root, automatic
routing stayed within 3.1% of the faster forced route. On the 500,000-document
root it stayed within 1.8%. All 30 direct-versus-transpose comparisons returned
bit-identical top-50 ordinals and score bytes, and both root identities remained
stable. This is retained as scale-ladder evidence, not as route qualification.

The 7,791,171-document root then rejected promotion of the global 5:4 factor.
At 2.5% selectivity, the cancer and protein queries correctly selected direct
rows at about 1.30 seconds, but the graph query selected transpose at about
1.55 seconds while forced direct took about 1.31 seconds, an approximately 18%
regret. Transpose read roughly 702--730 MiB for all three query shapes while
direct read about 155 MiB. The fixed term-work table sees query-term lanes but
does not represent the executor's query-independent per-chunk metadata, scale,
and dense-term byte floor. Another global constant would only fit this root and
is therefore rejected. A future forward planner may use publication-time
physical byte/page statistics; it must not delay the exact block-pruning work
below.

The exact BMP route establishes the more important structural result. On the
same full root it decoded only about 12,000--185,000 semantic postings where the
forward routes examined tens of millions. Its latency remained 9--17 seconds
because exact signed bounds were spread across immutable term runs. A
query-local packed-reference page window reduced physical bound-read calls by
more than 99% and improved current-format exact latency by about 4--9% on the
20,000-, 500,000-, and 7,791,171-document roots, with identical exact results.
The full root still copied roughly 21 million five-byte references from
13,000--16,000 fragmented relation pages, so larger windows and threshold
tuning are now stopped.

The next admitted CQ-3 representation is one same-root, worker-published,
term-local compact bound directory. It must merge exact fine bounds across
immutable runs into global block order, preserve sign-aware min/max bounds, and
let the allowed-block bitmap reject entries before canonical posting payloads
are opened. Publication must use bounded streaming/external merge under the
existing accelerator root, WAL, retirement, reclamation, and generation
authority; it must not run semantic inference or create a second lifecycle.
The current segment BMP remains the exact fallback when that derived directory
is absent or stale. The representation is promoted only if it is bit-identical
and materially beats the best forward route on the full root. Otherwise it is
rejected and planner micro-tuning remains stopped.

The full-root result also narrows that representation requirement. Compacting
the current exact-bound references alone is insufficient: the exact route still
performed roughly 0.57--0.88 million canonical record reads after pruning most
blocks. The next oracle must therefore target the score authority used by the
product forward executors, not only the exact fallback. It derives query-
independent, sign-aware per-term block bounds from the existing quantized
forward rows, intersects the filter before row decode, and scores competitive
blocks through the unchanged forward-row scorer. Direct and transpose output
remain the oracle; exact BMP remains a fallback and diagnostic authority.

This work follows a strict representation gate rather than another route-
threshold loop:

1. Run a read-only `b8`/`b16` forward-bound oracle on the 20,000-document
   synthetic root and the 57,638-document FIQA root before any publication
   change. Report bound entries and bytes, allowed and
   competitive blocks, rows, row bytes, and postings. The projected exact
   format uses a delta-varint block identifier plus a conservative float32
   positive maximum per term/block; query weights remain nonnegative, while
   signed document contributions are bounded by that positive maximum.
2. Require the pruned result to contain the complete direct/transpose top-k and
   require at least a 50% reduction in row/posting work for the 2.5% filter.
   Bound metadata plus selected row bytes must be lower than the measured
   direct-row bytes.
3. Only after that gate passes, publish the chosen projection under the current
   accelerator root and worker lifecycle. The product format may use adaptive
   fine blocks or a hierarchy when `b16` is too loose; it must not add another
   root, worker, WAL stream, or score authority.
4. Qualify the resulting format on the full root for exact output, broad and
   selective latency, concurrency, cancellation, RSS, restart, and repeated
   accelerator publication.

If the forward-bound oracle fails the reduction or storage gate, CQ-3 stops at
v7 and does not resume global factor, threshold, cache-window, or exact-bound
micro-tuning. The next research boundary would then be a different posting
layout, not another planner constant.

The read-only oracle produced a deliberately mixed result. On the synthetic
20,000-document root, both `b8` and `b16` retained 100% of allowed row and
posting work for all three query shapes at 2.5% selectivity. This is a useful
hard-case rejection: block bounds cannot replace direct rows globally, and a
product route must decline the new executor when projected metadata plus
selected rows do not beat direct-row bytes.

On the 57,638-document FIQA root, `b8` retained the complete quantized
forward-score top-50 for all three query shapes while reducing 2.5%-filter
posting work to 30.0%, 34.4%, and 31.7%, and row bytes to 29.6%, 34.0%, and
31.4%. Projected query-term bound bytes plus selected row bytes remained below
the measured direct-row bytes in every case. `b16` retained 53.5--61.9% of
posting work and failed the 50% reduction gate for all three queries, so it is
rejected. These results admit only an exact `b8` executor with a measured
direct-row fallback; they do not yet qualify its total publication size or
full-root latency. Raw evidence is retained in
`docs/performance/data/raw/commons-query-readiness-2026-08-21/`
`cq3-forward-bound-20k-v1.json` and `cq3-forward-bound-fiqa-v1.json`.

The same scan also measured total projected publication size rather than only
query-local metadata. `b8` needs 1,727,608 entries and 8,877,312 bytes on the
20,000-document root, and 4,042,413 entries and 20,391,215 bytes on FIQA. This
is 49.7% and 60.8% of the corresponding forward artifacts, but only about 4.1%
and 8.9% of the complete 208 MiB and 218 MiB index relations. `b16` is smaller
but already failed the work-reduction gate. The `b8` size is therefore admitted
for a product prototype, not yet as a default; publication RSS and full-root
index growth remain explicit rejection gates.

The admitted executor is not allowed to use the oracle's final kth score.
It must accumulate query-local block upper bounds, process allowed blocks in
descending bound order, score documents with the unchanged forward scorer,
and stop only when the next conservative bound is below the live kth score.
This makes the pruning exact rather than result-informed. The publication
gate additionally requires total bound bytes and build RSS to remain bounded;
query-local savings alone are insufficient.

The first same-root `b8` product implementation now passes the small-to-medium
representation gate. Accelerator directory v8 owns fixed eight-document
forward-bound shards beneath the existing accelerator root; normal accelerator
maintenance can republish them from current forward-score authority without
semantic inference or a primary-index rebuild. Query execution intersects the
allowed bitmap before row decode, visits candidate blocks in descending
conservative-bound order, and scores documents with the unchanged quantized
forward-row scorer. Direct rows and transpose remain exact comparison routes,
and an absent, stale, or over-budget bound projection falls back rather than
creating a second score authority.

Across three query shapes and three filter densities on both the 20,000-
document synthetic root and the 57,638-document FIQA root, all 18 forced
direct-versus-bound comparisons returned bit-identical top-50 document ordinals
and score bytes. The measured executor, rather than the earlier rank oracle,
reduced scored rows as follows:

- on the 20,000-document root, 124 to 99--110 rows, 2,000 to 259--584 rows,
  and 10,000 to 292--584 rows for the 0.6%, 10%, and 50% filters;
- on FIQA, 358 to 274--296 rows, 5,763 to 388--777 rows, and 28,819 to
  419--642 rows for the same filter ladder.

The corrected trace also reports bound bytes and entries, candidate and scored
blocks, fallback, and budget exhaustion. For example, the FIQA 50% cancer
query read 86,338 bound bytes, considered 5,523 blocks, scored 106 blocks and
419 documents, and examined 37,129 row postings instead of direct rows'
2,542,415 postings. This is a real scorer-work reduction, not a route-planner
estimate.

This result does not yet promote `b8` to automatic routing. On small and medium
roots, transpose can still be faster because its contiguous term lanes cost
less than reading bound metadata and scattered competitive forward rows. The
FIQA example took about 9.5 ms through `b8` versus 8.9 ms through transpose,
despite scoring 98.5% fewer documents. Directory v7 is therefore frozen as the
retained planner correction, global-factor and threshold tuning remains
stopped, and the next decision is a forced-route full-root A/B. Promotion
requires bit-exact results plus lower decoded work, latency, and concurrent RSS
than the best existing route. If full-root savings do not offset bound and
random-row costs, `b8` is rejected rather than tuned by another global
constant. A coarse-to-fine bound hierarchy is considered only if full-root
telemetry specifically identifies bound metadata as the remaining dominant
cost.

The first full-root publication attempt rejected a generic global tuplesort
for the bound stream. Although each term's block identifiers are naturally
produced in increasing order, that implementation expanded every
`(term, block, maximum)` entry into a PostgreSQL heap tuple and sorted the
complete set again. After 25 minutes it had written more than 200 GiB of
temporary I/O without completing, so the attempt was cancelled without
publishing a root. This is a writer defect rather than an inherent query cost.

The replacement writer appends each already ordered term stream to a logical
tape, then concatenates those streams into the existing shard publisher. It
does not rescan the corpus, add an artifact, or change the v8 format. Logical
tape extent preallocation is disabled because hundreds of sparse term tapes
otherwise reserve sort-sized extents. On the current-format 2,000-document
root, accelerator publication completed in 11.8 seconds. Existing forward
publication used about 276 MiB of temporary space, while the bound tape and
final bound stream used about 4.2 MiB and 0.87 MiB. All nine forced
direct-versus-bound comparisons remained score-bit identical; at 50%
selectivity the bound executor scored 184--344 rows instead of 1,000. This
qualifies the bounded writer for full-root testing but does not yet qualify
automatic routing.

The forced-route full-root panel then separated the retained planner result
from the scorer result. At 0.6% selectivity, direct rows remained correct at
about 553 ms while the bounded route took about 562 ms: its mandatory 158 MiB
bound stream exceeded direct rows' roughly 63 MiB, so automatic execution must
decline it. At 10% selectivity, the bounded route completed in about 1.01 s
versus transpose's 1.71 s, scored 8,324 documents, and examined about 2.29
million row postings. At 50%, it completed in about 1.45 s versus transpose's
2.47 s, scored 6,081 documents, and examined about 1.67 million row postings.
All forced direct, transpose, and bounded results remained score-bit identical.
This is a material 40--41% full-root latency reduction with lower actual scorer
work, not merely a better executor prediction.

Directory v9 therefore publishes physical route statistics during the existing
accelerator validation pass: direct row payload bytes by chunk, transpose fixed
bytes by chunk, transpose query-term bytes, and forward-bound bytes by term.
Automatic admission compares these publication-derived quantities and retains
only the existing transpose scratch-memory safety gate. The old global 5:4
factor is removed rather than retuned. Runtime telemetry reports estimated and
actual bytes separately because selected row work is query dependent. Bound
automatic admission was initially conditioned on mandatory query-term metadata
being below both forward alternatives, subject to medium-root qualification.
Promotion requires an automatic route to reproduce the forced-route exactness,
decoded-work, latency, cancellation, concurrency, and RSS results. If it only
improves route prediction without reducing actual work, planner tuning stops and
the next boundary is a new posting/block layout.

The current-format 57,638-document FIQA qualification rejected that initial
automatic bound admission before another full-root run. All 27 filtered
direct/transpose/bound comparisons remained score-bit identical, but automatic
execution selected the bounded route for every 0.6%, 10%, and 50% case. For the
0.6% cancer query, the 86 KiB bound estimate omitted competitive forward-row
reads: bounded execution read 4.42 MiB and took about 10.0 ms, while direct read
2.23 MiB at 9.2 ms and transpose read 3.13 MiB at 8.7 ms. At 10%, transpose was
about 9.7--10.0 ms while bound ranged from 11.5--14.3 ms. At 50%, transpose
remained about 10.4--10.7 ms while bound ranged from 11.2--12.2 ms.

The failure is structural rather than another cost-factor error: the probe
knows block upper bounds, but its descending block order can revisit forward
chunks and repeatedly load row metadata. Mandatory bound bytes therefore do not
predict total scorer bytes. Automatic bound promotion is withdrawn; the v9
physical statistics and direct/transpose route remain, and bound remains a
same-root forced diagnostic. The next scorer experiment must make filter/block
intersection and competitive-row locality first-class, then demonstrate lower
actual decoded bytes and latency. No additional global ratio or threshold is
accepted as a substitute.

Directory v10 closes the identified competitive-row locality defect without a
second artifact or lifecycle. The existing forward artifact now publishes one
row-data offset per document under the same accelerator directory. A bounded
executor can therefore score an aligned eight-document block with one
contiguous object-range read instead of reopening and validating each row.
Versions v5--v9 remain retirement-only formats; v10 is the sole current
directory format.

The 57,638-document FIQA rerun kept all 27 filtered route comparisons
score-bit identical. Relative to the previous bounded implementation, actual
bounded forward bytes fell by 87.9--97.9% and latency improved in eight of nine
rows. Bound nevertheless remained about 0.2--1.4 ms slower than the best direct
or transpose route in most rows, so small-root automatic admission remains
rejected. Raw evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/fiqa-v10-block.json`,
SHA-256
`2e7d785735e5943a35703d82232a6cc45563a20e52d5e7686a57a83b3d35b9da`.

The same-binary 500,000-document scale point then established that the result
is not confined to the full root. For 10% filters, bounded p50 averaged 70.6 ms
versus transpose's 125.9 ms; for 50%, it averaged 95.2 ms versus 166.0 ms. At
0.6%, direct rows remained the safer aggregate choice at 35.7 ms versus bound's
41.8 ms. All 27 comparisons stayed score-bit identical. Raw evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-500k-v10-block.json`,
SHA-256
`8ab1c8766bd5839130d5ca21e455e09469b9ee2fe59b5a028551d7a44dab5233`.

On the immutable 7,791,171-document PubMed root, v10 publication completed
without inference or a primary-root rebuild. The accelerator directory grew
from 1,672,704 to 35,466,672 bytes, source manifest 2454 remained the published
authority, and the root stayed stable through the query panel. Bounded p50
averaged 892.8 ms at 10% versus transpose's 1,707.9 ms, and 1,363.4 ms at 50%
versus 2,392.0 ms. Actual bounded forward reads averaged about 120 MiB versus
about 687 MiB for transpose. At 0.6%, bound also averaged 501.6 ms versus direct
rows' 584.2 ms. All 27 direct, transpose, and bound results remained score-bit
identical. Raw evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-block.json`,
SHA-256
`3dbfd664d8ebbc85ee15aa96d3cc21d6e2cb80d35d7c938679e1e61941c464fa`.

Full-root runtime safety also passes. Twelve 10 ms cancellations in one backend
reached an RSS plateau and left the backend reusable. A separate warm p500
bounded panel returned identical digests at concurrency 1, 4, and 8; per-query
p50 was 1,267, 1,405, and 1,411 ms respectively. Evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-bound-cancel-rss.txt`,
SHA-256
`47db3d85a325845aa79e9f5764f9f82b6081a36ff0778b1cfc4247b972bfcfc7`,
and
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-bound-concurrency.json`,
SHA-256
`9061dde64885294b66c506d31b0a2d559614687b6f938967ce2c7bc15f7e6d98`.

The v10 bounded executor is therefore retained as a qualified same-root
capability. Automatic admission now uses the existing 64 MiB statement-work
budget as a resource boundary rather than fitting a new corpus threshold. It
preserves direct whenever direct wins the publication-derived byte estimate;
otherwise it selects bound only when transpose exceeds that budget and the
published bound stream is smaller. On the immutable full PubMed root, the
normal route kept all three 0.6% cases on direct and selected bound for all six
10%/50% cases. All nine automatic results were score-bit identical to direct.
Across the three queries, automatic p50 was 735.5--1,020.1 ms at 10% versus
transpose's 1,620.5--1,782.9 ms, and 1,232.0--1,468.9 ms at 50% versus
transpose's 2,221.1--2,499.1 ms. Raw evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-auto-policy.json`,
SHA-256
`ad010fb0f5c70a37b9beda583a533b45837021329a4ec1c9837d2a58eff90ad1`.

A second same-binary panel forced all four routes for one representative query.
At 0.6%, automatic remained direct; at 10% and 50%, automatic and forced bound
used the same route, scored the same blocks, and returned identical result
digests and score bits. Automatic/forced-bound p50 was 730.6/738.5 ms at 10%
and 1,223.5/1,231.0 ms at 50%. The root identity remained stable and the
isolated postmaster was stopped without touching Shadow's main service. Raw
evidence is
`docs/performance/data/raw/filtered-bound-v10-2026-08-23/pubmed-full-v10-auto-bound-forced.json`,
SHA-256
`663089317e43e58add5619b9761a21f849dbfe8bdb88bf99ecdb5346c5a1a8d9`.
This closes the current planner adjustment: no document-count threshold,
cost-ratio tuning, or further bound-route micro-optimization is planned before
the CQ-7 application matrix.

A subsequent 500,000-document representation audit tested whether b16/b64
coarsening or adaptive term membership plus rank-select could reduce the
mandatory bound stream without changing scores. All levels retained top-50
containment, but the exact b8 total fell by only 6--7% for dispersed 0.6%,
2.5%, and 10% filters. Only a contiguous 2.5% filter reached a material 41%
reduction. Of 45 query terms, only four selected dense membership, while 38
remained sparse; dispersed allowed blocks also touched nearly every 4 KiB
bound window. b16 and b64 saved metadata but admitted enough extra forward-row
work to lose overall. The scale ladder therefore rejects both coarsening and a
rank-select-only format change before a full-root rebuild. The retained v10
executor remains unchanged. Any future CQ-3 work must first demonstrate lower
decoded sparse/high-DF term entries and physical pages under the same-root
oracle, not another route, threshold, cache, or block-size variation. The
method and evidence are recorded in
`docs/performance/reports/filtered-forward-bound-addressability-2026-08-25.md`.

The next read-only audit isolated a more promising structural decomposition.
Instead of opening every semantic term's bound stream, it sorts terms by their
sign-safe global contribution cap, accumulates block-local bounds only for an
essential prefix, and represents every omitted term by one conservative suffix
cap. Across six 500,000-document queries and three full-PubMed queries, the
mandatory-plus-essential partial scores produced a conservative kth lower
bound without using the final reference kth as an input. Conservative projected
work fell by 23.4--48.3% on 500K and 36.7--58.3% on the full root, with every
derived lower bound safe and zero missed competitive blocks.

This admits one exact MaxScore-style product experiment, not another tuning
loop. The audit still used a complete pass to compare every prefix point and
consumed roughly 7.6--7.9 GiB RSS on the full root, so it is not a deployable
query path. The implementation gate is to integrate one forced
essential/residual proof into the current exact BMP executor with bounded
block-sized memory. All survivors must still use the unchanged score authority.
The forced `N` is only an executor-capability probe; no query- or corpus-specific
value may become a product constant. Small, FIQA, 500K, and full-root
same-binary forced A/B must report exact score bits and physical refs, pages,
bytes, postings, latency, cancellation, concurrency, and RSS before automatic
admission is considered. If a complete prepass is still required or measured
page savings disappear, the route is rejected without changing `N`, ratios,
thresholds, caches, or block sizes. The evidence and execution contract are in
`docs/performance/reports/semantic-essential-term-maxscore-oracle-2026-08-25.md`.

A live four-session, two-wave smoke against Shadow's small system-chunk scope
completed all eight requests with zero predicate violations and no residual
benchmark sessions. Every backend selected `accelerator_block_major`; warm p50
was 362.8 ms and warm p95 was 672.9 ms against the 500/1000 ms gates. The host
was concurrently building the isolated PubMed root, so this validates harness
semantics and bounded concurrency only, not the final performance panel.

Rank-oracle verification is intentionally restricted to concurrency one. The
exact tid[] oracle performs additional ranking work and may warm pages; running
it inside a concurrent latency wave would both multiply database load and
contaminate the measured cache state. CQ-7 therefore uses one serial exactness
pass and a separate bounded-concurrency pass over the same cases.

The matrix's row-level predicate check now treats SQL `NULL` as a violation.
Commit `70ac94e0` changed the result guard from a truth-only count to
`(<violation expression>) IS NOT FALSE`; a row is accepted only when every
required predicate is conclusively satisfied. This closes a fail-open case in
which a returned row with a null date, category, journal, organization, or
scope value could previously evade the concurrent predicate check. The SQL
allowed-set and rank oracles retain their existing PostgreSQL predicate
semantics, and the focused matrix suite passes.

The serial exactness pass now requires five named shapes rather than accepting
one arbitrary oracle: arXiv date-plus-category, PubMed date-plus-journal,
PubMed date-plus-category, the large CA document scope, and the system document
scope. These cover selective mixed predicates, the prior PubMed blocker, and
document-local boundaries without constructing million-TID broad-date oracles.
Promotion rejects an old matrix that lacks current catalog, generation,
planner, plan-capture, or query-metadata qualification. Finalization requires
the same serial oracle set under the stable name and binds its matrix generation
and extension version to the qualified package and live stable generation.
The three pre-swap matrices must also identify the same complete nine-index
Commons generation map; sharing only the PubMed candidate generation is not
sufficient evidence when another business index changes between concurrency
panels. Promotion and finalization re-read the bounded internal generation
status for all nine live indexes immediately before the catalog action. The
live guard requires current scope metadata as well as a ready, forward-complete
accelerator, and rejects evidence whose generation map has since drifted.

Stable-name publication now has a dedicated fail-closed promotion tool rather
than reusing the general rebuild runner. Promotion requires passing matrix
evidence for concurrency 1, 4, and 8 on one candidate generation, including a
serial rank oracle. One transaction renames the former stable index to an
explicit rollback name and the candidate to the stable name while preserving
both OIDs. Final cleanup requires a second passing matrix through the stable
name; rollback remains a separate atomic operation. The tool records matrix
hashes plus before/after catalog and generation identities. It must not be run
until the full-root and complete Commons gates above pass.

Shadow's first PubMed publication has no former stable index. The promotion
tool therefore supports an explicit `--bootstrap` mode only when both the
stable and rollback names are absent. It applies the same package, c1/c4/c8,
serial-oracle, complete-generation-set, live-generation, and transactional
identity gates, then renames only the candidate to the stable name. A
bootstrap finalization still requires a second passing stable-name matrix and
records the final generation, but has no synthetic rollback index to drop.
Ordinary promotion continues to require and retain the former stable index;
the missing-stable case never silently selects bootstrap behavior.

Promotion and finalization also require passing product-maturity evidence for
the installed staged package. The package fingerprint covers the complete
locked milestone-model bundle in addition to the extension binary, current
install SQL, control file, and bundled ONNX Runtime. Immediately before the
catalog transaction, the promotion tool re-hashes staged and installed
artifacts, requires the matrix extension version to match `BUILD-INFO`, and
records the source commit, package fingerprint, model manifest, and model id.
Rollback remains available without package evidence so a broken release can be
reverted even when the candidate package is no longer readable.

Matrix contract v7 now fails closed when the measured environment disables
maintenance or forces either hidden diagnostic query route. It records and
requires a positive maintenance-worker limit,
`ii42.test_disable_semantic_accelerator = off`, and
`ii42.test_filtered_forward_route = auto`. It also records at least two waves,
and promotion verifies that every case contains separate concurrency-sized
first and warm panels. Contract v7 additionally requires every query shape to
emit a canonical hit signature and requires that signature to remain identical
across every first/warm wave and client. The policy-union and ordinary chunk
scope cases now expose the same evidence as native filtered cases, and chunk
scope violations use null-safe identity. Promotion rejects v6 evidence rather
than accepting a matrix that did not gate repeated result stability. Promotion
and finalization also re-read the live generation and require current scope
metadata immediately before each catalog action. Diagnostic isolation remains
valid for the same-root binary A/B harness, but its output cannot substitute
for a normal runtime qualification matrix. Commit `dcabdba1` owns this contract
change. Commit `c9026c8c` makes promotion and finalization independently verify
that each case's stability attempt count equals its first plus warm panels and
that the stability gate passed; a top-level `passed=true` cannot substitute for
the v7 evidence structure.

Clean immutable macOS and Linux candidate packages now bind commit
`db03e4d1b5cd48d66996dd80c0b0aaf30c31f0f4`, ONNX Runtime 1.26.0, and model
manifest SHA-256
`419e3521eff91bdca149d7014dc71a5cd9538d6904854849056f4f327dd30364`.
The macOS archive SHA-256 is
`02475319346263b0b5b51cc29831472c351ea6769aeb5c336824e8435a8436bc`; the
Linux archive SHA-256 is
`d770d817d64907ba036caa5d7988ca1bfbb8fb8f92f623913354b4f7f54f8e94`.
Both archives pass integrity and bundled-runtime inspection. The Linux staged
library/control pair also passes the runtime-required and runtime-privilege
smokes without modifying Elm's installed files. The installed Elm and macOS
binary/SQL boundaries intentionally remain older and therefore fail the full
package-binding preflight. These artifacts are release candidates, not
deployment qualification; installation, catalog recreation, complete maturity
evidence, and the Commons matrix remain CQ-7/CQ-8 gates.

The Linux candidate binary has now completed the four staged lifecycle surfaces
without changing Elm's installed extension: mutable SAE passed 56/56 gates,
shared-preload closure passed, physical replication passed through prepared
transactions, linked L0, maintained CRUD, REINDEX, type conversion, and DROP,
and standby auto-preload passed generation replacement and exact preload. The
first replication attempt compared a primary score captured before maintenance
authority was acquired with a newer accelerator generation replayed on the
standby. The controller now pins primary maintenance authority first and requires
the standby to replay that exact generation before comparing results. The strict
score comparison then passed without tolerance. This closes the candidate's
staged lifecycle controller evidence, but it does not replace installed-package
binding, full Shadow matrices, or post-install host qualification.

The matching macOS candidate package has completed the same four staged
lifecycle surfaces without changing the installed Homebrew extension. Mutable
SAE passed 56/56 gates, shared-preload closure completed, physical replication
replayed the exact primary generation before strict score comparison, and
standby auto-preload completed generation replacement plus exact preload. This
closes detached macOS candidate-lifecycle evidence only. The installed macOS
catalogs, package binding, and query matrices remain CQ-8 requirements.

A newer clean Linux package now binds commit
`9f86046d956546521b146c4a2d5e1c98f9783824`, ONNX Runtime 1.26.0, PostgreSQL
18.4, and the same milestone-model manifest. Its archive SHA-256 is
`7c326a1a236974dfe81a150bf9bcae2eb8473b7a899c0a55bc22ab4b4cc65962`.
The isolated real-model lifecycle passed 73/73 gates; evidence is
`/data/ii42-builds/ii42-9f86046d-staged-lifecycle.json`, SHA-256
`eb4b9f59dbf4fa94bfbf47c8b28db512fd4e6d333bb15fcd55378febfc651dc6`.
Physical replication then passed prepared commit and rollback, linked-L0
replay, eventual semantic completion, maintained CRUD, REINDEX, same-relation
BM25/SAE type conversion, and DROP. Its evidence is
`/data/ii42-builds/ii42-9f86046d-staged-replication.json`, SHA-256
`d47d6be50e80c029eef44651a0bc6e6204831c3f7f77c589f3db71b24ee48b03`.

The same package also passed the shared-preload lifecycle closure. An oversized
80,000-document exact root correctly used bounded PostgreSQL prewarm rather
than index-sized shared state, while an admissible resident root invalidated
its old fold after generation replacement and republished the current fold.
The log is
`/data/ii42-builds/ii42-9f86046d-staged-shared-preload.log`, SHA-256
`c705ebc7a3eb8a0fd85f5b1934c52095cf6b6b1aa86ced19a499ebb30e8023ce`.
The runtime-service restart smoke killed worker PID 2445732 and required a new
PID generation, 2445781, in the same slot before accepting recovery; degraded
and recovered product-query rows remained identical. Its evidence is
`/data/ii42-builds/ii42-9f86046d-staged-runtime-restart.json`, SHA-256
`e6e2058404e6486f9cf1563df0996085f982a4c9cfc29aaab8356f44e2eaaae4`.
These results qualify the detached package lifecycle, not its installed-host
binding. Shadow full-root equivalence and Commons matrices, followed by
controlled Shadow, Elm, and macOS installation, remain mandatory.

The follow-up immutable Linux package binds clean commit
`83415f93af684af97fa5131058f1ef0412a7087c`, ONNX Runtime 1.26.0, and the
same milestone-model manifest. Its archive SHA-256 is
`779b5904c5f1d94618387f08276ad5fa071c09bb110ddba7ccaa00000aca09f5`.
It completed all 40 non-benchmark product-maturity steps inside an isolated
mount namespace, with matching package preflight and postflight fingerprint
`4e2e35cc82f4fd21bd86db8cbd9cf096872fe6f40e8795c577b0e57308ea728d`.
The report is
`/data/ii42-builds/ii42-83415f93-namespace-maturity-smoke.json`, SHA-256
`e6bc32729c9fe0de2178656ae6fd2b920d847df52cd82dc5648c67bdb63397fc`.
This package adds a portable Linux backend-memory ownership gate: writable
private mappings remain the hard ownership boundary, while PSS/RSS are
reported as observational physical footprint and allocator-zone metrics are
explicitly unavailable rather than reported as zero. The package passed the
50,000-document lifecycle, concurrent CRUD and DDL, 2PC, VACUUM, crash and
runtime-worker restart, physical replication, shared preload, a 1,000-iteration
ONNX Runtime soak, and the memory gate. The suite deliberately skipped the
throughput benchmark, so this closes the detached Linux package and memory-gate
blocker only. It does not close the in-progress Shadow full-root build,
five-query runtime-migration gate, CQ-4 cold/warm matrix, CQ-7 host matrix, or
CQ-8 rollout.

The current Linux x86-64 beta package binds clean commit
`9a28a2e7db32f866f72b2ef9e105c807c433b212`, ONNX Runtime 1.29.0, and the
same milestone-model manifest. Its archive SHA-256 is
`0937d1523b4557f31542686ba148e157e1ec5bd9d827b7500e3b4406c5e51726`.
All 41 non-benchmark package-maturity steps passed in an isolated mount
namespace, including convergent BM25+SAE lifecycle, 2PC, VACUUM, crash,
physical replication, shared preload and standby warmup, runtime backpressure
and restart, an ONNX Runtime soak, the production model lifecycle, concurrent
DDL, and the medium mutable lifecycle. Package preflight and postflight both
record fingerprint
`77997c56023adfd8514423fcad900d5d27f61c24797c7734cd7425ae707e2811`.
The report is
`/data/ii42-builds/ii42-9a28a2e7-ort129-namespace-maturity-smoke.json`,
SHA-256
`959be98b184a81bd1997808f8f1ed51a77949a8af5ccf409b8e35545ab8eb51e`.
This closes the detached Linux ORT 1.29 package gate only; the package remains
unqualified for host rollout until the active Shadow full-root and Commons
matrices pass.

The first full-root CQ-3E attempt with that package stopped at 818,296 of
1,414,848 heap blocks (57.836%) with `ii42 accelerator failover failed`.
The builder remained near 630 MiB RSS with no swap, OOM, kernel fault, or
postmaster failure. Runtime telemetry instead showed several remote requests
reaching their five-minute deadline while the single local document lane was
temporarily full. The asynchronous failover path treated one failed local
enqueue as permanent after the remote deadline, even though the queue and
workers were healthy and recoverable. It also allowed the expired remote
deadline to override the retry backoff in `request_ready()`, creating a busy
polling risk.

The repair gives each backend-local request a separate, bounded local-fallback
deadline. After the remote deadline, the request becomes local-only and retries
queue admission every 100 ms for at most 60 seconds; cancellation remains
immediate, and terminal failure remains bounded. A dedicated temp-PostgreSQL
probe now stalls at least 40 concurrent remote batches against the 31-slot
local document capacity, then requires the 96-document index to finish through
local fallback with no response slots left in use. This regression passes
locally. CQ-3E remains open until the repair passes the immutable ORT 1.29
package suite and a fresh full-root exact-result/RSS run on Shadow.

The full-rebuild RSS qualifier now also fails closed on publication state.
Previously a successful `psql` exit could leave the report marked completed
when the requested `--index` name was absent, invalid, or not ready. Commit
`31a19ad7` records the SQL return code separately, captures the final catalog
state, and returns failure for every incomplete publication. The focused
qualification suite passed 52 tests with one platform-specific skip. The
already-running Shadow process predates this harness correction, so its final
operator gate must still inspect `present`, `indisvalid`, and `indisready`
explicitly before running the postbuild runtime-migration fixture.

The qualifier now also makes the documented full-rebuild memory contract
fail-closed. Commit `a350613c` records the configured and observed memory
values, defaults to a 32 GiB backend peak-RSS ceiling and zero process swap,
and rejects missing RSS or swap telemetry. Its complete Python suite passed
391 tests with one platform-specific skip, and the product-convergence
inventory passed. The active Shadow process also predates this harness slice;
its final operator gate must apply the same limits to the recorded phase peaks
instead of treating an older report without `memory_limits` as qualified.
Neither harness correction closes `CQ-3E` before publication, catalog state,
runtime-migration stability, and full phase peaks are all observed.

The backend-only memory gate understated rebuild ownership because document
encoding runs in postmaster-owned II42 runtime workers. Commit `c355bce1`
extends the qualifier to sample the complete isolated PostgreSQL process
family and adds an attach-only observer for builds already in flight. The
first attach-only Shadow run was monitored at
`/data/ii42-builds/cq3e-full-ort129-family-memory.json`. Its first attached
sample observed 15,035,322,368 bytes of family PSS, a 26,701,451,264-byte
runtime-worker high-water mark, and zero process swap. This was below the
32 GiB gate, but remained provisional because the observer attached
3,441.729 seconds after the backend started.

A subsequent process-image audit rejected that run before publication. The
PostgreSQL family had mapped II42 binary SHA-256
`c23f8ef8be4624db18bc8e7159b826d4027eab904812e627b5c30b132c1f52c7`,
while the report's manually supplied package path had SHA-256
`d377c1b408b746f7104b3baaaf9d41f58ad2e6d8053268a36acd9cd2d640e759`.
The original qualifier hashed the argument but did not prove which object was
mapped. The run was cancelled cleanly at 165,207 of 1,414,848 heap blocks and
its evidence was retained under
`/data/ii42-builds/rejected-cq3e-ort129-binary-mismatch-20260824T1325Z`.
It is not CQ-3E evidence.

Commit `bf4b14a5` makes that boundary fail closed. Before starting the rebuild,
the qualifier now obtains its own PostgreSQL backend PID, reads the loaded
extension from `/proc/<pid>/maps`, hashes the mapped file, and requires it to
match the qualified artifact. The finalizer independently requires that
binding evidence. Shadow's isolated install tree was atomically replaced with
the exact package bytes and restarted; catalog calls, preload, and application
backends now map the `d377...` binary with ORT 1.29. The replacement full-root
run writes
`/data/ii42-builds/cq3e-full-ort129-bound-rss.json` and its process-family
report to
`/data/ii42-builds/cq3e-full-ort129-bound-family-memory.json`. The report
records `qualified=true` for the binary binding, and the family observer began
0.981 seconds after the build backend. The complete Python suite passed 405
tests with one platform-specific skip, and the convergence inventory passed.
CQ-3E remains open until this run completes publication, both memory reports
pass, current catalog state is present, and the postbuild runtime-migration
fixture passes.

Commit `eb8302f2` adds the fail-closed CQ-3E finalizer used after that build.
It independently revalidates both memory reports, binds their application,
candidate-index, phase, and completion-time identities, re-reads baseline and
candidate catalog state, and only then executes the tracked five-query
rank/score equivalence SQL. It writes one combined qualification JSON and
refuses to run equivalence when any evidence is incomplete. The deployed
finalizer SHA-256, including the mapped-binary requirement, is
`b087a4b0b9004ba78a2c62ec9ade379767cb2bb3e12b67165fb08c2a5674a89c`;
the local and Shadow equivalence SQL SHA-256 is
`f74874deda4fd0238224f646a894018fc5f337a69b9c799d6f40788e5effb53f`.
The complete Python suite passed 401 tests with one platform-specific skip.

The finalizer now treats the migration program and observation window as
qualification authorities rather than descriptive metadata. It requires the
migration SQL to match the tracked five-query fixture SHA-256 above, binds the
build and process-family reports to one PostgreSQL PID and backend start,
requires identical host, port, database, user, and memory limits, and rejects
a process-family observer that attached more than five seconds after backend
start. Completion coverage ends only after the observer has covered the final
recorded build sample; it does not incorrectly require the independent
observer wrapper to write its report after the build wrapper exits. Embedded
process-family peaks are included when the build report's memory summary is
recomputed. The finalizer SHA-256 is
`1ea8036f2b87f624613dc53df953b94124780c29f786e0438d3af23786642911`,
and the migration SQL SHA-256 is
`43856b33f556319e9974d49ef8cd7421065486518a868096447e8596ced6b372`.
The complete Python suite passes 431 tests with one platform-specific skip.

The package-bound rebuild completed with both reports qualified, zero swap,
and a maximum observed process-family footprint of 28,925,432,832 bytes under
the 32 GiB limit. Candidate
`bench.pubmed_full_cq3e_streamed_idx` is present, valid, ready, and
43,016,495,104 bytes. The original bit-exact cross-root fixture correctly
exposed deterministic score differences: both roots replayed identically and
all five top-100 memberships matched, but the ORT 1.26 baseline and ORT 1.29
candidate differed by at most one rank and 0.1335% relative score. The writer
byte oracle remained exact, so the corrected migration fixture retained exact
membership and replay requirements while bounding the intentional runtime
transition. Combined report
`/data/ii42-builds/cq3e-full-ort129-bound-final-qualification-v3.json`
completed with `qualified=true`, no precheck or qualification errors, and
five passing rows. This closes `CQ-3E`; it does not install the product catalog
or close the remaining Shadow query/lifecycle matrices.

The next Shadow CQ-7 slice isolated two independent PubMed costs without a
rebuild. Scope format v6 stores a no-false-negative 32-bit gram mask in the
existing reserved bytes of each value entry. The shared worker republished the
scope object in the current root without document inference while queries
continued to use the exact v5 object. On the 8,214,026-document PubMed root,
the `publish_date + category ILIKE` case reduced exact value comparisons from
2,796,310 to 370,259, an 86.8% reduction. All eight measured allowed sets
contained exactly 51,033 documents, predicate violations remained zero, and
every top-50 TID and score bit matched the exact `tid[]` rank oracle. Warm p50
fell from about 579.6 ms on v5 to 464.5 ms on v6. Scope relation bytes did not
grow because the mask reused reserved storage.

That republish also exposed a separate maintenance cost. The PubMed scope
publication took about 41 minutes and peaked near 42.45 GiB of temporary
external-sort files before atomic publication and complete reclamation. It did
not block reads, invalidate the prior scope, or re-encode documents, but scope
publication latency and temporary storage remain an explicit CQ-5 maintenance
cost rather than a query-path regression.

Commit `d7147107` then extended the existing bounded page-native prewarm pass.
After warming required accelerator directories, gram filters, and semantic
forward heads, it uses only the remaining `ii42.prewarm_max_bytes` budget for
the scope value-entry table and dictionary. It creates no new artifact,
readiness state, or worker. On Shadow, the PubMed warm marker increased from
15,154 pages, about 118 MiB, to 49,086 pages, about 383 MiB. A guarded
relation-local eviction first proved that page-cache residency fell from
46,210,416,640 bytes to zero while relation bytes, generation identity, and
accelerator identity remained unchanged.

The controlled post-eviction query separates the remaining cold cost. The
first exact filtered attempt still exceeded the 35-second client timeout.
After those data pages had been faulted in, server latency was 432.0 ms for the
first completed attempt and 424.4 ms warm p50; exact filter membership, result
stability, and the `tid[]` rank oracle all passed. The settled route scores
51,033 rows and reads 14,500,417 forward postings, about 64.7 MiB of selected
row data. Therefore scope parsing and metadata prewarm are no longer the cold
blocker. The blocker is synchronous, dispersed forward-row page access across
the large relation.

The next bounded CQ-7 experiment must improve the existing forward-row I/O
submission depth or use PostgreSQL's batched read stream for the already known
sorted page set, then repeat the same guarded eviction and exact oracle. It
must not add a persistent artifact, change route thresholds, weaken settled
semantic exactness, or treat a warmed retry as cold evidence. Separately,
foreground eventual-SAE behavior remains lexical-first: committed rows are
immediately searchable while semantic score and rank may temporarily differ
until worker completion. That freshness contract is not a substitute for the
settled-root cold-page performance gate.

Commit `23f45d00` fixed the first prefetch admission defect. Bounded metadata
prewarm made all forward headers resident, but the old prefetch helper used
"header I/O was initiated" as a proxy for row-payload residency and returned
before submitting any payload reads. The repaired helper always derives the
selected row ranges before prefetching their payload pages. On the same
guarded PubMed eviction, the first exact query completed in 5,550 ms instead
of exceeding 35 seconds. All eight attempts retained 51,033 allowed documents,
zero predicate violations, stable top-50 score bits, and the exact `tid[]`
rank oracle. Warm attempts were 451-464 ms. This is a real cold-path recovery,
but it misses the 2,000 ms first-query gate and adds work to the settled warm
path, so it is not the final CQ-7 promotion result.

Full-root forced-route evidence localizes the remaining physical boundary.
The direct document-major route completed warm in about 470 ms while reading
51,033 rows, 14,500,417 postings, and 64.7 MiB of selected row data. The
forward-bound route returned bit-identical results and reduced scored rows to
1,462 and row postings to 435,554, but it decoded 29,412,347 bound entries and
147.1 MiB of bound metadata, taking about 1,528 ms. Forced transpose exceeded
the 30-second statement timeout. Therefore neither a route threshold change
nor deeper payload prefetch is the next structural fix. A future bound path
must eliminate irrelevant bound ranges before decoding them, using a compact
same-root skip directory aligned with allowed document blocks. Until that
representation has a small-root byte/work oracle, keep direct rows as the
exact warm default and do not enlarge the current experiment into another
route-tuning loop.

The Shadow deployment also exposed a package-integrity failure in the ad hoc
candidate build: a later validation command relinked the extension against the
host's default ORT 1.26 after the initial ORT 1.29 build. PostgreSQL failed
closed at startup, the prior qualified binary was restored, and the candidate
was rebuilt and verified against `VERS_1.29.0` before redeployment. The regular
Makefile now rejects any `pkg-config` ONNX Runtime version that differs from
`packaging/onnxruntime.version`; release scripts retain their existing staged
artifact and runtime checks.

#### CQ-8: Roll out and remove transitional state

- [ ] Repeat the staged-package and query matrix gates on Elm and local macOS.
- [ ] Update bootstrap and operational documentation to the qualified filter,
  residency, statistics, and preload contract.
- [ ] After Shadow alone uses the current generation and passes its complete
  matrix, remove obsolete format branches and compatibility code and qualify a
  cleaned current-only package. Keep only the direct `psql_bm25s` to II42
  migration boundary. Elm and macOS then rebuild directly with that package;
  they do not delay the Shadow source cleanup.
- [ ] Remove per-host candidate names and temporary run artifacts after each
  host's stable-name matrix passes.
- [ ] Record the package fingerprint, per-host generation identities, complete
  performance matrix, and cleanup evidence in release evidence.

Completion gate: Shadow, Elm, and macOS run one current product path with no
intermediate index-format compatibility or unpublished candidate artifacts.

The Commons matrix contract is now version 10 and makes the product catalog,
C-module authority, and target PostgreSQL runtime identity explicit. The
current install SQL publishes `ii42_catalog_v1` through an extension-owned
internal function; the matrix and destructive rebuild runner must observe that
exact value. Matching `extversion` values are insufficient because II42 ships
one current install catalog and no beta-to-beta upgrade chain. This catalog
marker prevents a newer binary or install SQL file from being mistaken for a
current database catalog.

The same contract makes both the product
catalog's C-module authority and the target PostgreSQL runtime identity
explicit. It schema-qualifies the installed extension's ONNX Runtime probes and
requires the host to report the repository pin, API 29 and ORT 1.29.0. This
prevents release evidence from authorizing a catalog that still has an ORT 1.26
binary mapped. It also enumerates every II42 extension-owned C
function through `pg_depend`, records the distinct `pg_proc.probin` values,
and qualifies the catalog only when the sole value is `$libdir/ii42`. This
rejects stale absolute staging paths and mixed old/new module bindings before
any ranked query runs. The check complements rather than replaces CQ-3E's
Linux `/proc/<pid>/maps` byte binding: an isolated package-bound build may use
an absolute staging path for rebuild evidence, but it cannot qualify as a
deployed Commons catalog. Elm and the local Homebrew catalog currently satisfy
the module-path shape; their installed package bytes and SQL catalog still
require the separate CQ-8 rollout gates below. The atomic
promotion/finalization tool now requires the same contract version, and a
cross-tool regression prevents the matrix producer and promotion authority from
drifting apart again. The rebuild catalog reconciliation enforces the same
single-module authority before any destructive index action, so an old absolute
path cannot survive until the post-rebuild matrix. Promotion also re-reads the
repository's sole ORT pin and requires the immutable maturity report's
`BUILD-INFO` to name that exact version with bundled pkglibdir linkage; a
historical passing ORT 1.26 report cannot authorize the ORT 1.29 candidate.

A refreshed read-only rollout inventory on 2026-08-24 narrows the remaining
host work. Elm runs PostgreSQL 18.6 with a 96 GiB shared runtime and one
maintenance worker. Its `ii_demo` and `ii_dev` catalogs use the correct
`$libdir/ii42` module authority, but both still report API 26, ORT 1.26.0, and
no `ii42_query_trace_internal()` catalog object. Elm owns twelve II42 indexes:
one demo root plus eleven Commons roots. The Commons set still includes the
arXiv `legacy`, CA `current_next`, and PubMed `scope_next` transition names, so
it requires one coherent ORT 1.29 package/catalog replacement and reviewed
current-root rebuild rather than an in-place beta upgrade.

The current Homebrew PostgreSQL 18.6 product postmaster uses a 1 GiB shared
runtime and four maintenance workers. Its declared `ii_dev` and `postgres`
catalogs also have the correct `$libdir/ii42` authority but still report API 26,
ORT 1.26.0, and no trace object. `ii_dev` has no II42 index to migrate. The
nineteen II42 indexes in `postgres` are the previously declared research roots
and remain outside CQ-8 automatic rebuild scope. The current installed macOS
binary and SQL SHA-256 values are
`c2d5b4a45a02b20a850d97efb61fc76eb1fc3e6f93f64770a8f3b8bd79d0435e`
and
`021ab608b4ef614e789d4ff533861a499cb989fee3981a153ee27f95609429f4`;
neither authorizes the pinned ORT 1.29 rollout.

The refreshed Shadow production boundary is narrower than Elm's. Its
PostgreSQL 18.4 `ii_dev` catalog already has `$libdir/ii42`, but was created
from the intermediate install SQL SHA-256
`3af61761e7f7d63189fbd83cd53c867662ef37d352a910a0898bd6ad6e94304b`,
not the current package SQL
`021ab608b4ef614e789d4ff533861a499cb989fee3981a153ee27f95609429f4`.
It has `ii42_query_trace_internal()` but lacks the current catalog marker. The
postmaster has a 112 GiB shared runtime
and four maintenance workers. However, the mapped product binary SHA-256 is
`cb13ce0a2ace90a2880522c07ea22e8f3cb70aa8689a4e766182249133393770`
and still reports API 26 and ORT 1.26.0, so it is not the package-qualified
`d377...` candidate used by the isolated full-root run. Shadow owns nine
Commons roots plus five research/diagnostic roots. PubMed exists only under
`data_pubmed__title_abstract__current_next_idx`; there is no prior stable name.
After CQ-3E passes, Shadow therefore needs one coherent ORT 1.29 package
install, current-only catalog recreation, restart, reviewed current-root
rebuild, the complete same-generation matrix, and bootstrap promotion of that
PubMed root. It must not receive an intermediate catalog repair or beta upgrade
script.

The reviewed current-only transition is now catalog-derived rather than an
operator-written DDL list. A read-only inventory of Shadow `ii_dev` records all
14 live II42 roots: nine Commons roots and five benchmark/calibration roots.
Six roots contain `INCLUDE` scope columns and 13 are partial indexes. The prior
rebuild SQL retained neither topology, so using it would have silently removed
the metadata required by Commons filtered top-k. The rebuild runner now emits a
versioned plan directly from the live PG18 catalog, preserves key, INCLUDE,
predicate, and current SAE reloptions, and compares every planned definition
with the live catalog before destructive work. `--refresh-extension` refuses a
partial or stale plan. It drops the enumerated indexes and recreates the
extension in one transaction with restrictive `DROP EXTENSION`; the new
catalog identity is checked before commit, so a stale installed SQL package or
an unplanned external dependency rolls the entire transition back. A local
PG18 scratch proof reproduced a multicolumn field-aware index with two INCLUDE
columns and a partial predicate, then confirmed that an old catalog missing
`ii42_catalog_v1` leaves both original indexes and the extension intact after
the rejected refresh. This closes the orchestration design gap, not CQ-8
deployment: Shadow still requires the current immutable package and a fresh
inventory immediately before rollout.

The immutable Linux release evidence is still present at
`/data/ii42-builds/ii42-9a28a2e7-ort129-namespace-maturity-smoke.json` with
SHA-256
`959be98b184a81bd1997808f8f1ed51a77949a8af5ccf409b8e35545ab8eb51e`.
It records a passing clean `9a28a2e7` package, stable fingerprint
`77997c56023adfd8514423fcad900d5d27f61c24797c7734cd7425ae707e2811`,
ORT 1.29.0 bundled in the PostgreSQL pkglibdir, and a passing milestone-model
binding. Every staged artifact remains present. Promotion correctly cannot use
the report before installation because Shadow production does not yet contain
the report's installed `libonnxruntime.so.1.29.0`. Installing the exact package
and restarting must make every installed artifact identity match before the
matrix or promotion gate runs; the gate must not be relaxed or regenerated
against the old host files.

The current Linux release candidate supersedes that packaging evidence at
clean commit `98bb9661492f90976ab82a2c4d554b22b93c6c02`. Its immutable
PostgreSQL 18 x86-64 archive is
`ii42-v0.2.4-linux-x86_64-pg18.zip`, SHA-256
`bfd9e23b7bf5ba1f0f0792269260fe7307610d99e6d0512e532dbcb60a5a22e3`.
The staged extension and install SQL SHA-256 values are
`640cbaefda82dbb71133231b1f408d71d106d15bfa937c341bee95389319dc72`
and
`a755eb5d40bd87360a26a2a3c913b74cd9d6e393e540eb76a387a23c51eccdee`.
The package records a clean source tree, bundled ORT 1.29.0, and milestone
model manifest
`419e3521eff91bdca149d7014dc71a5cd9538d6904854849056f4f327dd30364`.

The complete Linux product-maturity suite passed 41/41 steps from a private
mount namespace in 3,314,645 ms. Its report is
`/home/leask/ii42-builds/ii42-98bb9661-ort129-namespace-maturity-smoke.json`,
SHA-256
`1b0c62db61c89902564ba09bb20e9b05cd982a3e2352927d9821bc1abf7f3845`.
Preflight and postflight retained package fingerprint
`d27cd86b45c95a1c4e53bf753e55d3082fe75ad55dfe4d2a874ac7387f5d3fa7`;
all staged and installed binary, SQL, ORT, and model identities matched. The
namespace exited without leaving package bind mounts on Elm. This closes the
current Linux package and lifecycle gate, but does not replace the running
full-root CQ-3E publication, process-family memory, cancellation, and exact
result gates. The CQ-3E build remains bound to its recorded `9a28a2e7`
artifact; no C source changed between that build and `98bb9661`, while the
later commits tighten qualification, migration, and test contracts.

A fresh read-only product-catalog inventory on 2026-08-24 records the exact
remaining CQ-8 boundary:

| Host and catalog | Runtime API | Current trace | Current catalog contract | II42 roots | Required action |
| --- | ---: | --- | --- | ---: | --- |
| Shadow `ii_dev` | 26 | present | absent | 14 | Install the qualified ORT 1.29 package and recreate the declared product catalog and roots. |
| Elm `ii_dev` | 26 | absent | absent | 12 | Recreate from the same qualified package only after Shadow promotion passes. |
| macOS `ii_dev` | 29 | absent | absent | 0 | Recreate the empty product catalog from the installed current package. |
| macOS `postgres` | 29 | absent | absent | 19 | Retain as research data outside automatic CQ-8 migration unless explicitly promoted to product scope. |

The Shadow staging directory now contains the exact `98bb9661` Linux archive
and its complete maturity report under
`/data/ii42-builds/releases/98bb9661/`; their SHA-256 values match the
qualified artifacts above. They are staged only. The production binary,
catalog, and indexes remain unchanged while CQ-3E runs in its isolated
PostgreSQL instance.

The same staged runner produced a read-only Shadow transition rehearsal at
`/data/ii42-builds/releases/98bb9661/cq8-shadow-inventory-preflight.json`,
SHA-256
`e020d36d8e23bd0f2a74bf0d994dc3901cce52ead239f12e77980b9a9ad20d82`.
It found 14/14 roots valid, ready, and live; all 14 map to
`reindex_current_sae`. Six roots retain INCLUDE columns and 13 retain partial
predicates, including arXiv's three filter columns and PubMed's seven. This
proves that the catalog-derived transition preserves current topology. It is
rehearsal evidence only: the destructive rollout must regenerate and compare
a fresh inventory immediately before package/catalog replacement.

Equivalent read-only rehearsals bound Elm and macOS. Elm's inventory is
`/home/leask/ii42-builds/ii42-release-98bb9661/cq8-elm-inventory-preflight.json`,
SHA-256
`1009d8159bbed8ed19cf44ad496c1a2fc8513d670347c43726c97d3da43e0a61`.
All 12 roots are valid, ready, and live and map to `reindex_current_sae`, but
three are transition artifacts: the arXiv `legacy`, CA `current_next`, and
PubMed `scope_next` roots. CQ-8 must retain nine stable product names, carry
forward the reviewed scope/filter topology, and remove those three transition
roots after the stable-name matrix passes. It must not blindly rebuild all 12
names as independent product roots.

The table/key comparison also fixes Elm's target topology without inventing
an II42-specific migration rule. Relative to the qualified Shadow shape, Elm
must add `organizations` to the arXiv INCLUDE set, the corresponding document
ID to each CA/TX/WA chunk root, and the three generated date-bound columns to
PubMed's existing four-column scope. Commons already owns these definitions in
`scripts/prepare_commons_ii42_filter_schema.sql` and
`scripts/prepare_commons_ii42_scope_indexes.sql`; its focused schema-contract
suite passes 5/5. CQ-8 must run that application schema preparation before
building current II42 candidates, validate them, and then promote the nine
stable names. The generic rebuild runner remains responsible for preserving
catalog topology, not for synthesizing application columns.

The macOS inventory is
`/Volumes/Betty/Tmp/ii42-cq8-macos-inventory-98bb9661.json`, SHA-256
`63ca28fe359c44bcc7b9fa3c3d9875a9d11d5291c3611a8620b5fe2938c55929`.
The `ii_dev` product catalog contains no roots; all 19 valid roots are research
indexes in `postgres`. Recreate the empty `ii_dev` product catalog from the
current package, but keep the research roots outside automatic CQ-8 migration.

The destructive rebuild runner now also reads the sole pinned runtime version
from `packaging/onnxruntime.version` and requires both the matching compile-time
ORT API and the matching linked runtime probe before touching a semantic index.
Elm and local macOS currently report API 26 and ORT 1.26.0, so the runner
correctly rejects them until the qualified ORT 1.29 package and current catalog
are installed. An explicit environment override is available only for a
separately authorized package boundary; an unavailable or empty pin fails
closed. The complete Python suite passes 416 tests with one platform-specific
skip, and the convergence inventory passes.

A current read-only Elm audit confirms that this gate is still necessary.
The installed binary can read the accelerator summaries for the TX title, WA
title, and system-chunk indexes, but rejects the summaries for arXiv, PubMed,
CA title/chunks, TX chunks, and WA chunks as an invalid serialized format.
An arXiv product query still completed, while the PubMed unfiltered probe did
not complete within a five-second diagnostic timeout. The matrix environment
snapshot intentionally fails closed on this mixed-format state. Elm also lacks
the catalog entry for `ii42_query_trace_internal()` even though the installed
same-version SQL file declares it. Replacing SQL and the binary under an
unchanged extension version does not update PostgreSQL's extension catalog and
is not a valid staged deployment. Do not add a permanent parser fallback, an
ad hoc function, or a beta-to-beta `ALTER EXTENSION` path merely to make status
green. After Shadow qualifies the current root, preserve the source tables,
remove the affected indexes, recreate the extension from one coherent current
package, and rebuild the six roots under the same product lifecycle. Then
repeat the complete matrix before removing transition support.

A second live boundary audit on 2026-08-22 binds that diagnosis to exact
artifacts. Elm's installed `ii42.so` SHA-256 is `27ad2f346b4c81982c32424d2b024e7c38bdb6af3039da942133974876f71606`,
while the staged Linux candidate is `40430b19437a7a5b7b497217a7efb3ee451a4a0ed4b4b5024e626414b45fbecd`.
Its installed SQL is `fa6d2a4c639b88485f8cf91c12aaf6a160d01ee819e5b55b3604c324f6b2c8b5`,
not the candidate SQL `3af61761e7f7d63189fbd83cd53c867662ef37d352a910a0898bd6ad6e94304b`;
the catalog still lacks `ii42_query_trace_internal()`. Elm has twelve II42
indexes, including the PubMed `scope_next`, arXiv `legacy`, and CA-title
`current_next` candidates that must be resolved after qualification rather
than retained as product names.

The local Homebrew boundary is independently stale: installed `ii42.dylib`
SHA-256 `ecb921d7511d85c93bf374f34193098a3f3622bb4e826db2fefb897aaf5481ec`
differs from candidate `cdd13068401c5acbd2fc85a833d391710e9c42d6bc9f184ae221c67f12ad498c`,
and its installed/candidate SQL hashes match the same old/current pair above.
Both local product catalogs lack the trace function. The nineteen II42 indexes
in local `postgres` remain research data and are outside automatic CQ-8
migration; only declared product catalogs are recreated from the qualified
package.

The catalog audit also bounds the remaining host work. Shadow `ii_dev` owns the
current trace function and is the full-root qualification surface. Elm is the
mixed-format deployment described above. On macOS, the product-facing
`ii_dev` and `postgres` catalogs also report version 0.2.4 without the current
trace object and therefore require the same clean catalog recreation before
local lifecycle qualification. Numerous older local benchmark databases retain
0.2.0--0.2.3 research catalogs; they are historical fixtures, not product
deployments, and must not be bulk-mutated as part of CQ-8. The release cleanup
targets the declared product databases and current package only.

The contract-v4 Elm preflight is retained at
`/tmp/cq8-elm-plans-preflight-v4.json` with SHA-256
`bbe56822529fa79128969ebbed10917ca45a29c143e8f61afab279a2bd40b93c`.
It fails closed for the expected deployment reasons: the generated PubMed
partial-date columns are absent, the catalog and mixed index generations are
not current, query metadata is incomplete, and the missing columns prevent a
complete planner capture. This is preflight evidence only; it does not qualify
Elm or justify an intermediate compatibility path.

The transition-removal inventory is deliberately narrow:

| State | Current authority | CQ-8 action |
| --- | --- | --- |
| Scope codec v2-v5 readers | Removed in `852e28f1`; `ii42_scope_header_deserialize()` accepts current v6 only | Complete after Shadow proved all six scoped product roots were v6/current. An older root now fails closed and must be rebuilt directly by the current package. |
| Accelerator policies 5 and 6 | Removed in `852e28f1`; policy 7 is the sole queryable and publication-current policy | Complete after all nine Shadow product accelerators proved ready under policy 7. An older accelerator is stale and must be republished by the current worker. |
| Fixed 32-extent term directory | Current writers support up to 64 extents and compact each COW leaf to its actual maximum; readers still accept the former fixed 32-extent shape | Remove only the synthetic fixed-width compatibility contract after every Shadow product root has been rebuilt by the current writer. Preserve variable-width current leaves, including a legitimate current leaf whose actual maximum happens to be 32. |
| Candidate and rollback roots | The dedicated promotion tool owns atomic stable/rollback naming | Remove candidate names and temporary run artifacts only after the stable-name matrix passes; retain one rollback root until finalization evidence is recorded. |
| Product SQL catalogs | One coherent current install SQL and binary define the product catalog | Recreate the declared product catalogs from the qualified package. Do not add beta-to-beta upgrade SQL or parser fallback for catalog drift. |
| Direct migration boundary | Existing `psql_bm25s` data may be rebuilt as a current II42 index | Preserve this one supported migration boundary and its documentation; it does not imply support for intermediate II42 formats. |
| BM25-only versus SAE generation contracts | Lexical and unified contracts are both current product modes | Retain both. They select `sae = false` or `sae = true`; they are not format-compatibility debt. |

A source-level serialized-format inventory found no additional intermediate
decoder families beyond this table. Runtime-service, storage-v3, COW-object,
manifest, BMP, and forward-stream version constants identify their sole current
formats rather than backward-readable branches. The 32-extent item is narrower
than a runtime decoder: its legacy constant is consumed only by synthetic
fixtures. The current term-COW v8 reader deliberately accepts each compact
leaf's actual maximum from 1 through 64, so a current leaf whose maximum is 32
must remain valid after the legacy fixture and constant are removed.

Generation readiness and the Commons matrix now expose `scope_present`,
`scope_version`, and `scope_current` from the fixed-size scope header under the
pinned accelerator root. Scope-less indexes are current by definition; indexes
with scope metadata qualify only when the header is current v6. Older v2-v5
objects are unsupported and fail closed; they require a direct rebuild by the
current package. This read is bounded to one header and does not scan the
relation or create a second lifecycle.

The current-only removal was performed only after Shadow recorded the installed
package fingerprint, stable generation identity, current scope version, and
accelerator freshness for every declared product root. The cleaned package
then passed build, codec tests, 71/71 isolated mutable lifecycle gates, restart,
and Shadow query smoke. Elm and macOS migrate by rebuilding directly with this
package; no intermediate reader remains solely for their transition. Complete
physical-replication, cancellation, and full-root concurrency RSS qualification
remain release gates rather than compatibility readers.

Elm also predates the current Commons PubMed filter schema. Shadow already has
the three generated partial-date columns, their two B-tree indexes, and a
current candidate root that includes all three values. Elm has only the raw
`publish_date`. CQ-8 must therefore run the reviewed Commons schema preparation
before rebuilding its current PubMed root. Do not emulate missing application
columns inside II42 or add an intermediate parser branch; the generated columns
are the PostgreSQL predicate and planner authority, while same-root scope
metadata is their retrieval projection.

The Commons schema preparation is now independently deployable from its II42
candidate builds. One idempotent script adds all three stored PubMed bounds in
a single `ALTER TABLE`, then builds only the two planner B-trees concurrently;
the scope-index script includes that step before starting its II42 candidates.
Candidate names are build-time identities and never application contracts. An
isolated PostgreSQL 18 smoke validated year-only, month-only,
and exact-day values plus a repeated idempotent execution, and the complete
Commons library suite passed. This closes the deployment-tooling gap, not the
Elm host gate: the production-size heap rewrite still requires a declared
maintenance window after Shadow qualification.

A clean detached macOS package smoke at commit `2a3bebaf` validated the former
1.26 release-identity gate against real artifacts. Installing its
checksum-locked archive under `/tmp` and selecting only its pkg-config file
produced a valid historical 0.2.4 package without changing the system runtime.
All fourteen staged and installed milestone-model
files matched, including manifest SHA-256
`419e3521eff91bdca149d7014dc71a5cd9538d6904854849056f4f327dd30364`.
The complete binding deliberately remained red because the local installed
`ii42.dylib` and install SQL predate that package. CQ-8 must replace those files
from one qualified package and recreate the product catalogs; it must not relax
the runtime pin or treat the matching model alone as a completed deployment.

The current macOS beta candidate now binds clean commit
`74588c23667012521d74f74de64ac751ac45b17f`, ONNX Runtime 1.29.0, and the
same milestone-model manifest. The release builder accepts both the canonical
II42-installed runtime license and Homebrew's formula-root license, then
rewrites and verifies the staged extension dependency as
`@loader_path/libonnxruntime.1.dylib`. The extension no longer resolves the
Homebrew ORT absolute path. The archive SHA-256 is
`623088e01df66774282519cfe2198f982f80006bae053f230edaa36611700673`;
the staged extension and versioned ORT SHA-256 values are
`bb61111435893f64ba68e7f0b1c07b9e13c1261bfc6419a6e4109d8cd942fd57`
and `70568a2fdf7be410976244cc113276d0345c5e04835154fc7e447be5ecacc46b`.

The package is installed on the Homebrew PostgreSQL 18.6 host with matching
binary, SQL, control, model, and versioned ORT bytes. Its complete immutable
package maturity suite passed 42/42 steps from the clean detached worktree in
`/Volumes/Betty/Tmp/ii42-product-maturity-74588c23-ort129-full.json`, SHA-256
`dd267bc485707d1d2e39c3b1a9e134e5c6f0b61ef26d7aaa8dc981b4e138ee06`.
Preflight and postflight retained fingerprint
`5374e1afce08a29259bd805e5d2ed8446c6e2f6014f4a46410024ca75c3699a5`.
The suite covered convergent BM25+SAE lifecycle, migration, 2PC, VACUUM,
concurrent CRUD and DDL, crash and runtime-worker restart, physical
replication, shared preload and standby warmup, a 1,000-iteration ORT resource
soak, the 50,000-document mutable lifecycle, and the 75,000-document,
96-client product benchmark. All 96 clients succeeded with zero runtime
failure or busy rejection; semantic query p50 was 575.9 ms and p95 was
978.1 ms. The benchmark report SHA-256 is
`2346604f72815581f991ff0f32fe133425adaa68ba6f9be443c0a976146956c7`.
This closes the immutable and installed-artifact macOS ORT 1.29 package gate.
It does not close CQ-8 catalog recreation, product-index rollout, or the
macOS Commons matrix, and it does not authorize Shadow promotion before CQ-3E
and the Shadow Commons gates pass.

### Shadow stable-name closure, 2026-08-26

Shadow's sole 90 GB PubMed root was promoted in place from
`data_pubmed__title_abstract__current_next_idx` to the stable product name
`data_pubmed__title_abstract__field_aware_bm25_idx`. No index rebuild or root
rewrite occurred. The old and new top-20 query signatures matched, catalog
valid/ready bits remained true, and the physical standby replayed the rename.
All nine Shadow Commons roots now use stable product names; candidate fallback
is rejected by the qualification runner and by Commons application startup.

### Stop and Escalation Conditions

- Stop before swap on any filter mismatch, timeout, unbounded RSS, catalog and
  binary mismatch, stale accelerator, incomplete semantic convergence, or
  active maintenance/rebuild ambiguity.
- Do not compensate for a slow exact route by weakening membership, reducing
  candidate competition, or moving post-filtering into Commons.
- Do not create a second persistent filter artifact or worker. If current-root
  scope metadata cannot support a required predicate, document the format gap
  and extend the single publication lifecycle deliberately.
- Do not use `ANALYZE`, larger shared memory, or page-cache warming to hide an
  algorithm that still performs heap-wide array expansion or linear forward
  scoring for broad filters.
- Any format change must first pass a small local lifecycle fixture, then a
  medium corpus, then a Shadow side-by-side candidate before a large rebuild.

## Deferred Post-Beta Work

### CSG-I114: remaining access-method orchestration

`src/ii42_am.c` still owns a large, cohesive orchestration closure spanning
VACUUM, semantic completion, maintenance dispatch, and query preparation.
Further extraction is optional for beta correctness but valuable for review
isolation. Start only with a typed one-way authority boundary; do not add a
second root, scheduler, scorer, lifecycle, callback framework, or broad private
API merely to reduce line count.

### CSG-I132: measured parallel scale work

PostgreSQL parallel heap build, parallel VACUUM discovery, and parallel AM scan
remain deferred. Evaluate them only after million-scale profiling shows that
single-process build or discovery, bounded maintenance, or the current shared
runtime no longer meets the target. Any implementation must retain one checked
manifest publication authority and exact page-native scoring.

## Non-Goals

- Do not add a separate retirement or statistics worker. VACUUM publication,
  coalesced hints, periodic reconciliation, and existing maintenance already
  provide gradual convergence.
- Do not re-encode completed semantic documents during ordinary maintenance.
  Seal, compaction, fold, specialization, and reclamation transform index data;
  only pending changed fingerprints require inference.
- Do not turn post-beta parallelism or modularization into a beta correctness
  dependency without new measurements.
