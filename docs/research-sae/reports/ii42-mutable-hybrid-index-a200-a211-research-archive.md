# II-42 A200 Mutable Hybrid Index Plan

Date: 2026-06-10

## Purpose

This document defines the mutable maintenance direction for the II-42
BM25+SAE product path.

The goal is not a second model-specific scheduler and not a sidecar semantic
service. The goal is a real maintainable PostgreSQL index:

```text
table DML
    -> ii42 index AM
    -> lexical BM25 postings + semantic SAE atom postings
    -> same consistency policy
    -> same stale/debt accounting
    -> same maintenance workers
    -> one query path and one final ranking surface
```

The product target remains:

```text
string query -> built-in text-to-atoms encoder -> unified BM25+SAE sparse index
```

The important A200 decision is that semantic mutable maintenance must be an
extension of the existing ii42/BM25 index lifecycle, not a parallel lifecycle.

## Issue #18 Alignment: Shared Delta Generations

GitHub issue #18 predates SAE productization, but it is directly on the critical
path for model-backed indexes. The issue exposed a weakness in the existing
eventual-consistency BM25 path:

- the active/base generation can be shared and resident;
- foreground writes can record only pending counters or compact delta records;
- query-time delta overlay remains bounded and can still depend on backend-local
  materialization;
- a large shared-preload deployment can therefore have a healthy resident base
  generation while new rows are not searchable until background catch-up crosses
  `auto_rebuild_threshold` or `auto_rebuild_delta_bytes`.

The production mitigation was to lower thresholds so background catch-up runs
sooner. That is useful, but it is not the final design. The correct product
direction is:

```text
base generation + shared lexical delta generation + shared semantic delta
generation + tombstone generation
    -> statement/scan leases
    -> one query accumulator
    -> background fold into the next base generation
```

This means SAE support should not introduce another sidecar freshness model.
Instead, the BM25 delta issue and the SAE mutable index work should converge on
one generation model:

- base and delta generations are identified by database OID, index OID,
  relfilenode/locator, base generation id/cache epoch, delta page range, record
  counts, and byte counts;
- foreground queries in shared-preload deployments attach resident base and
  resident delta generations through leases; they should not rebuild large
  deltas per backend;
- if a matching shared delta generation is not available, shared-preload mode
  prefers a documented base-only eventual read plus worker wakeup over a
  backend-local memory amplification path;
- standalone deployments without shared-preload may keep a local bounded delta
  fallback;
- maintenance folds base+delta+tombstone into a new base generation and retires
  old generations only after readers drain.

For model-backed indexes, lexical BM25 delta, semantic SAE delta, pending
semantic encode debt, and tombstones are different sources inside the same
physical lifecycle. They should share scheduling, leases, GC, status, and
promotion semantics.

## Core Decision

Model-backed BM25+SAE indexes should use the same index strategies that normal
ii42 BM25 indexes already use:

- same `CREATE INDEX USING ii42` surface;
- same `consistency` options;
- same stale/debt metadata;
- same maintenance worker limit and priority rules;
- same `auto_preload` direction;
- same query-time tolerance for eventual freshness debt;
- same operator status and maintenance entrypoints where possible.

SAE adds semantic encoding and atom postings, but those are index payload
features, not a separate scheduling system.

## Current Implementation Status

Current milestone: `A210 native semantic lifecycle`

Implemented on 2026-06-08:

- `ii42_model_mutable_maintenance_contract()`;
- `ii42_model_mutable_maintenance_status(index_name regclass)`;
- `ii42_model_mutable_overlay_create_table(schema_name, table_name)`;
- `ii42_model_mutable_delta_upsert(...)`;
- `ii42_model_mutable_tombstone(...)`;
- `ii42_model_mutable_overlay_status(overlay_table, index_name)`;
- `ii42_model_semantic_debt_create_table(schema_name, table_name)`;
- `ii42_model_semantic_debt_record(...)`;
- `ii42_model_semantic_debt_resolve(...)`;
- `ii42_model_semantic_debt_status(debt_table, index_name)`;
- `ii42_model_semantic_maintenance_plan(index_name)`;
- `ii42_model_semantic_debt_maintain(index_name, max_rows)`;
- `ii42_model_mutable_query_atoms(...)`;
- `ii42_model_mutable_query_text(...)`;
- `semantic_overlay_table`, `semantic_debt_table`,
  `semantic_maintenance_max_rows`, `auto_rebuild_semantic_pending_docs`, and
  `auto_rebuild_semantic_delta_bytes` ii42 index reloptions;
- A150 smoke coverage for contract/status, configured reloptions, base-only
  query, delta-only query, base+delta merge, tombstone skip, text-query overlay
  parity, semantic debt recording, semantic debt resolution, and debt status
  aggregation, direct semantic debt maintenance, and `ii42_index_maintain_due`
  semantic debt consumption;
- A200e smoke coverage for semantic maintenance threshold planning,
  reloption-derived worker batch sizing, WAL-backed overlay/debt table checks,
  delete/VACUUM safety, and failure-safe encode handling.
- A200f smoke coverage for configured foreground semantic-debt recording through
  index reloptions, mutable compaction planning, and retired-generation
  overlay/debt finalization that refuses to clear the active generation.

A200f deliberately did not change physical index pages. It used relation-owned
SQL overlay/debt tables to validate mutable query semantics, debt accounting,
status aggregation, worker-facing semantic debt consumption, configured
foreground DML bridge behavior, and retired-generation cleanup before native
semantic page storage.

A200g added the first native semantic page lifecycle primitive. It stores the
compact semantic delta payload inside the ii42 index relation, mirrors A210
semantic-delta publishes into that native segment, and opportunistically reuses
or truncates the native semantic tail when a later publish shrinks or clears the
payload and no reader holds the old segment.

A200g also has a narrow native semantic debt log page primitive. It stores
opaque debt-log bytes in dedicated index-AM pages with status, validation,
load/store, clear, and restart-crash recovery coverage. This is a storage
primitive for the direct ingestion path; the SQL debt ledger remains the
authoritative source for row locking, encoder invocation, and debt resolution.

A210 now adds a first-class native semantic tombstone page segment. Delete and
replace tombstones produced by the bounded native append/merge path are stored
as a separate native page segment, exposed through status/validation/load/clear
APIs, crash-safe smoke coverage, and `ii42_generation_cache_state_json(...)`.
Foreground native mutable queries prefer that segment over legacy compact
payload metadata and fall back to metadata only for older native bundles.

A210 also connects those native page primitives to the mutable lifecycle:
native-only semantic maintenance can consume the native debt-log snapshot,
build empty-tail or append/merge semantic payloads, store first-class tombstone
pages, and fold a converged native semantic tail into a new EATMH001 active
generation without going through backend-local query fallback. SQL still owns
encoder invocation and failure handling, while direct native page apply uses
C/SPI helpers for row preflight and final batch debt-state transitions. Manual,
SQL scheduler, and C worker routes now share the same per-index maintenance
lock.

Verified A200e/A200f properties:

- no independent SAE scheduler is introduced;
- `eventual` is recommended for large model-backed indexes but is not a hidden
  default;
- lexical and semantic postings share one whole-index consistency policy;
- existing `ii42_index_details(...)` remains the lexical debt source;
- semantic debt can be attached through `semantic_debt_table` and reports
  pending encode/replace/delete rows, failed rows, pending bytes, and latest
  encode error;
- base generation and semantic delta overlay can be queried through one
  model-backed ranking surface;
- configured mutable query wrappers resolve `semantic_overlay_table` from index
  reloptions;
- pending semantic debt can be consumed with `FOR UPDATE SKIP LOCKED` by the
  same due-helper/worker lifecycle;
- the C background worker falls back to the SQL due helper when no lexical
  maintenance candidate is maintained, so semantic-only debt can converge
  without a separate scheduler;
- tombstones suppress base candidates before final top-k ranking;
- shared-preload mutable queries no longer treat a materialized semantic delta
  row as sufficient by itself. If the reusable semantic delta is not currently
  shared-resident, the foreground query stays on the base+tombstone eventual
  path and wakes maintenance instead of parsing, copying, or building a
  backend-local semantic delta payload.
- semantic maintenance thresholds are index reloptions and are reported through
  `ii42_model_semantic_maintenance_plan(...)`;
- pending semantic debt is always eligible for convergence, while threshold
  pressure separately marks urgent/compaction due state;
- failed semantic encoding resolves debt to `failed` without publishing a delta;
- overlay and debt tables created by the helpers are WAL-backed permanent tables;
- delete tombstones survive `VACUUM` until future compaction removes them;
- foreground DML-facing code can record semantic debt through
  `ii42_model_semantic_debt_record_index(...)` without hard-coding the ledger
  table;
- foreground index-AM inserts and changed updates now call the configured
  semantic debt bridge automatically for text-like model-backed indexes. The
  current bridge records encode debt into the SQL ledger only; it deliberately
  defers native debt-log page synchronization until post-commit maintenance so
  non-MVCC native side pages cannot outlive a rolled-back heap transaction. The
  A210 smoke now drives both
  insert-origin and changed-update encode debt rows through
  `ii42_index_maintain_due(...)` and verifies the resulting semantic deltas plus
  clean native debt-log mirrors. The same fixture verifies that non-indexed
  updates do not create semantic debt. Ordinary eventual INSERT/changed-UPDATE
  semantic debt therefore has an automatic convergence proof without expanding
  maintenance pressure for unrelated updates. The fixture also queries the
  affected rows before semantic convergence, proving that eventual mode keeps
  INSERT, indexed UPDATE, and non-indexed HOT-style UPDATE results SQL-visible
  through the ordinary BM25/SRF path while semantic maintenance catches up. The
  native DML fixture now also mirrors the SQL overlay rollback contract: a
  transaction-local native INSERT records pending debt inside the transaction,
  but rollback leaves no heap row, SQL ledger row, SQL overlay/generation bridge
  row, native debt-log counter drift, or shared-runtime maintenance work.
  Direct native debt ownership remains a later step.
- heap deletes follow PostgreSQL index-AM semantics: there is no per-row DELETE
  statement callback, so ii42 records semantic delete debt from the
  `ambulkdelete`/VACUUM exact-delete callback. The AM batches confirmed-dead
  heap TIDs through `ii42_model_semantic_debt_record_index_batch(...)`, so one
  VACUUM pass records all semantic delete debt in the SQL ledger while native
  debt-log sync remains a maintenance-owned repair step. Those rows use the
  same heap-TID document key as foreground DML inserts and are mirrored into
  native debt-log pages only after the SQL-visible operation is safely outside
  the foreground callback. The A210 smoke now drives those VACUUM-origin
  delete rows through `ii42_index_maintain_due(...)` and verifies they converge
  into tombstones without a manual resolve path. It also checks that deleted
  rows are not returned by the SQL SRF path before VACUUM records semantic
  delete debt and remain hidden after tombstone convergence.
- `ii42_model_mutable_compaction_plan(...)` reports active-generation compaction
  readiness and makes clear that rebuilt generation publish is still required;
- `ii42_model_mutable_compaction_finalize(...)` can remove retired-generation
  overlay/debt rows after a generation switch, and refuses to clear active rows.

A210 issue #18 closure properties:

- foreground hot shared-preload text, field-aware, id-array, and raw queries
  offload uncovered delta debt instead of building backend-local mini-indexes;
- shared lexical delta and tombstone generations can be published, attached,
  queried, and later folded into a new base generation;
- `ii42_index_maintain_due(...)` and the C worker rank shared-resident repair,
  lexical/tombstone publish, covered-delta fold, and hot debt consistently;
- cold indexes do not consume shared-holder slots only because they have
  publishable delta or semantic bridge rows;
- pure lexical indexes avoid semantic bridge planning, status, deployment, and
  SPI maintenance paths;
- standby readers can load replicated shared text-delta and tombstone layers
  while maintenance entrypoints remain recovery no-ops.

Current implementation target: A200g native semantic pages now store a
self-describing semantic-delta bundle: compact EATMH002 query payload bytes plus
the base generation id and exact overlay signature needed to prove currentness.
They are a queryable semantic-delta source for cold/non-shared paths and for hot
shared-preload paths that explicitly set
`allow_native_semantic_hot_fallback = true`. The default hot shared-preload
policy remains conservative: prefer shared resident semantic deltas, otherwise
serve the base eventual result and wake maintenance rather than silently parsing
native semantic bytes in every backend. Native pages also have a structural
validator, so status and foreground fallback only treat the native payload as
query-eligible when the metapage and every referenced semantic payload page are
consistent. `ii42_generation_cache_state_json(...)` exposes those validation
results under `native_semantic_delta`, allowing maintenance dashboards and
external schedulers to see whether native semantic storage is absent, valid, or
repairable. The semantic maintenance plan now treats missing/invalid native
pages for a current materialized semantic generation as republish debt, and
treats native orphan pages with no SQL overlay rows as clear debt. The C worker
comparator and SQL due helper also prioritize those native mirror repair/clear
tasks immediately after shared-resident semantic repair and before ordinary
lexical fold/rebuild debt, because native mirror convergence is cheap and keeps
model-backed indexes on the reusable semantic-delta path. Native-bundle
metadata now also lets foreground mutable queries prove that native pages are
current even if the generation-table semantic-delta row has been deleted or
corrupted, while still comparing against the live SQL overlay signature before
using the bytes. Native debt counters are now mirrored into the index metapage by
the SQL ledger record/resolve/maintain paths, so operator and scheduler state can
observe semantic pressure without querying only the side ledger. Native debt log
pages add a second native debt surface for opaque encoded debt records, but they
do not yet replace the SQL ledger for worker ingestion. Native page storage,
tombstone visibility, append/merge, delete-all tails, and tail-fold lifecycle
are now closed. Bounded worker-facing record selection is now native
debt-log-driven. The next product-completeness step is embedded index-AM
semantic posting and encoder integration: final index-AM semantic debt
ownership, query-time encoding/failure accounting inside index-AM, single-call
semantic posting storage, and the final single-call base/native/tombstone fold
merge.
Maintenance-time encoder failure accounting already resolves failed debt rows
through the same guarded C/SPI accounting primitive used by the native
debt-log-driven worker path. Row matching, locked preflight, and the final
`encoded`/`cleared`/`failed` SQL debt-state transition are now C/SPI-owned
batch helpers, but the SQL ledger remains authoritative.
A200/A210 status surfaces now make that boundary explicit: SQL overlay/debt
ledgers are attached, native semantic page storage is available as a
materialized semantic-delta mirror, native semantic tombstones are available as
a native visibility segment, native semantic debt counters are attached as a
mirror, lifecycle completion is reported separately, and direct C/index-AM
posting ingestion is still deliberately reported as pending.
The latest slice adds `ii42_model_mutable_semantic_delta_publish_native(...)`,
`ii42_model_semantic_debt_maintain_native(...)`, and the explicit
`native_semantic_delta_maintenance` reloption. Together they materialize the
same compact EATMH002 delta directly into native pages, delete the reusable
generation-table semantic delta row, and prove that debt maintenance can leave
native pages as the only current foreground semantic source. The default worker
route remains the generation-table bridge, but an index can now opt in to
native-only semantic materialization through the same maintenance worker entry
point. The public manual wrapper,
`ii42_model_semantic_debt_maintain(...)`, follows the same reloption so manual
operator repair cannot diverge from the C worker or SQL due-helper route. This
still leaves SQL overlay/debt ingestion as the upstream source of records;
native direct ingestion is only attached for a guarded empty-tail, encode-only,
full-batch apply path that can build a compact EATMH002 payload plus a native
doc map, store it in native pages, resolve the SQL debt rows, and let foreground
query translate native `doc_ord` values back to document keys without a
generation-table semantic-delta row. Foreground mutable queries now also reuse
the same payload that passed native page, bundle, checksum, and EATMH validation
instead of reloading and extracting native bytes again inside the dynamic query.
That keeps lifecycle status and query execution on the same proven payload
surface while native pages remain a materialized semantic-delta source. Manual
and automatic preload now
also walk the native semantic page segment through the same structural
validation hook, report the warmed page count in manual preload output, and
avoid leaving the first foreground native-semantic query to discover cold page
I/O. Lightweight lexical/tombstone folds now perform the same post-fold native
semantic validation/prewarm step and expose the warmed native semantic page count
plus before/after preservation diagnostics in the maintenance result, so fold
convergence actively preserves the semantic tail instead of merely avoiding
metadata damage. The A150 model-backed smoke now also proves the stricter
native-only case: after semantic delta materialization deletes the generation-table
row, a lexical lightweight fold keeps `native_semantic_delta_pages` as the
foreground mutable query source and returns the same semantic top-k. Fold
readiness now also treats native semantic repair/clear debt as a first-class
blocker: status and worker-visible planning do not advertise a lightweight fold
while the semantic mirror needs repair or deletion. The reported
`current_fold_strategy`, `target_fold_strategy`, and next implementation step all
point at native semantic repair/clear first, matching the worker comparator
priority. Native storage smoke now also covers the segment-boundary case direct
ingestion will rely on: after a native semantic segment is appended at the
relation tail, later lexical delta records relocate behind that semantic tail,
can span multiple delta pages, native semantic repair/republish can rewrite its
payload while that lexical debt is still pending, and the later fold still
produces a clean lexical base without corrupting the native semantic payload.
SQL maintenance readiness now also mirrors the C lightweight-fold side-page
validator. The status surface exposes
`native_side_pages_valid_for_lightweight_fold`, a
`native_side_page_validation_blocker`, and per-segment validation reasons for
the native semantic delta, native debt-log, and native tombstone side pages
inside `lightweight_multi_delta_fold_payload_state`. A scheduler can therefore
see the same structural blocker that would make the C fold helper refuse a
fold, instead of learning that only after attempting the fold.
The direct C try-maintain path now uses that same boundary. A structurally
invalid native semantic delta, native debt log, or native tombstone segment
returns an explicit non-progress `lightweight_delta_fold` result with
`native_side_page_validation_blocker=...` before lexical online rebuild can
consume the maintenance pass. Ordinary counter-only or incomplete lexical
payload debt still falls through to the heap-backed rebuild path, because those
states are lexical payload availability problems rather than native lifecycle
corruption.
Native tail fold apply now also has the same migration seam as the posting
writer. `ii42_model_semantic_native_tail_fold_apply_locked(...)` is a C/SPI
entrypoint for callers that already hold the per-index maintenance lock. It
delegates to the existing strict fold plan/apply/manifest flow, preserves the
direct `semantic_native_tail_fold` result shape, and attaches post-fold manifest
status plus wrapper diagnostics. Native-only semantic maintenance now uses that
locked entrypoint when the tail is fold-ready, while
`single_call_c_index_am_tail_fold_attached=false` remains explicit until the
final C/index-AM fold implementation replaces the C/SPI seam. Plan/apply/status
surfaces now also carry
`single_call_c_index_am_tail_fold_blocker=
single_call_c_index_am_tail_fold_not_attached`,
`tail_fold_contract_gap=single_call_c_index_am_tail_fold_not_attached`, and
`final_tail_fold_migration_target=
single_c_index_am_base_native_tombstone_fold` so the remaining gap is visible
without reading the implementation.

The current A210 slice also splits payload construction into
`ii42_model_semantic_native_ingestion_build_payload_from_encoded(...)`. The
existing SQL encoder loop now hands validated atom rows to that helper. The
append/merge path has the same boundary through
`ii42_model_semantic_native_ingestion_merge_build_payload_from_encoded(...)`,
which validates a pre-encoded batch against the native debt-log manifest before
rebuilding the merged native payload and tombstone map. Empty-tail direct apply
now also has
`ii42_model_semantic_native_ingestion_apply_empty_tail_from_encoded(...)`, which
accepts a pre-encoded full native-debt batch, stores the compact EATMH002 tail
in native semantic pages, resolves the consumed SQL debt rows, and synchronizes
native counters without rerunning the SQL row-by-row encoder. A future C/worker
encoder can therefore hand atom rows directly to the safe empty-tail native page
writer. Append/merge direct apply now has the matching
`ii42_model_semantic_native_ingestion_apply_merge_from_encoded(...)` route: it
accepts a bounded native debt-log batch plus pre-encoded rows, rebuilds the
merged EATMH002 native tail through the same C compact builder, stores native
tombstones, resolves SQL debt, and syncs native counters without invoking the
SQL row-by-row encoder. The final debt-row transition for these native page
apply paths now goes through
`ii42_model_semantic_native_ingestion_resolve_batch(...)`, backed by the
`c_semantic_debt_resolve_batch` SPI updater, so empty-tail and append/merge
apply no longer loop over consumed debt rows in PL/pgSQL. The remaining
direct-ingestion gap is query-time encoder execution/failure accounting and
final index-AM posting-storage integration; maintenance-time failure
accounting is attached through the current debt worker path, but this still
does not claim that the SQL debt ledger has stopped being authoritative.
Status surfaces now expose this as
`native_semantic_encoded_worker_data_plane_attached`, keeping the completed
encoded native-page data plane separate from the unfinished direct C/index-AM
posting ingestion path.
`ii42_model_semantic_native_posting_writer_plan(...)` now names that unfinished
boundary directly. It checks the current native batch, C/SPI preflight,
encoded-handoff validator, compact payload builders, native page stores, and
query-time encoder status in one non-mutating contract. The plan can report
that the present C primitives are attached while still exposing
`single_call_c_index_am_posting_writer_attached=false`; this gives the final
C/index-AM writer a stable replacement target without letting lifecycle status
over-claim completion.
`ii42_model_semantic_native_posting_writer_apply(...)` is the current mutating
single-call C/SPI entrypoint over those primitives. It takes the same
maintenance lock as the worker path, delegates to native semantic debt
maintenance, and returns before/after writer plans. This narrows the remaining
migration target to replacing that C/SPI body with one C/index-AM writer call,
not changing worker or operator-facing semantics.
The ordinary C/SPI entrypoint now also has the same relation-owner guard and
SPI cleanup boundary as the pre-encoded writer path. Passing a non-ii42 index is
rejected before SQL orchestration starts, and C wrapper errors do not rely on
transaction cleanup to unwind SPI state.
`ii42_model_semantic_native_posting_writer_apply_locked(...)` is the
worker-owned-lock variant of the same contract. The C maintenance worker already
serializes an index with the per-index maintenance lock, so native-only semantic
maintenance uses this helper to expose the posting-writer lifecycle without
calling the public wrapper and deadlocking on the same lock. This keeps manual,
worker, and future C/index-AM posting-writer routes aligned around one JSON
contract while the current data plane still runs over SQL/C primitives.
Direct C try-maintain now preserves the same scheduler ordering under lexical
rebuild pressure. When native semantic repair/clear, direct apply, tail fold, or
publish/delete work is due, `ii42_index_try_maintain(...)` lets the native
posting-writer path run before the online rebuild and returns the detailed
lock-owned writer result if it makes progress. If that priority semantic work is
attempted but blocked, try-maintain reports `mode=semantic_delta_priority`
instead of rewriting the base payload. Lower-priority semantic debt still yields
to lexical rebuilds, so this does not turn every semantic tail into a rebuild
blocker.
The global scheduler now treats native tail fold as part of that native
lifecycle band too: both `ii42_index_maintain_due(...)` and the C worker
comparator rank tail fold after direct native apply and before lexical
shared-delta publish, tombstone publish, or covered lexical fold. That keeps
native semantic compaction from being starved by unrelated lexical fold work
without changing the fallback rule for ordinary semantic debt.
The encoded empty-tail and append/merge apply results now also expose
`worker_handoff_contract = A210.encoded_native_page_apply.v1` plus the
native-debt batch manifest hash, resolve-lock summary, and resolve-batch summary
inside the native execution profile. That is the stable replacement boundary for
moving the remaining encoder/posting writer into C/index-AM code without
changing worker or operator-facing JSON.
`ii42_model_semantic_native_posting_writer_apply_from_encoded(...)` now routes
that same pre-encoded worker stream through the posting-writer seam. It takes
the normal maintenance lock, derives the bounded native debt-log batch, tries
the empty-tail and append/merge encoded native page helpers, releases the lock,
and returns the writer-level handoff contract. The final C/index-AM writer is
still intentionally marked unattached, but worker input no longer has to bypass
the posting-writer boundary to exercise native semantic page storage.
The encoded handoff contract is now batch-manifest-bound: every encoded row
emitted by `ii42_model_semantic_native_ingestion_encode_batch(...)` carries the
native debt-log batch manifest hash, and the C handoff validator rejects
otherwise well-formed stale rows whose manifest does not match the current
bounded batch. This prevents the future out-of-process or worker-owned encoder
from accidentally applying atoms for a previous native record stream.
The native tail-fold manifest is now fingerprint-addressable as well. Plan and
apply paths both include `manifest_sha256` with the explicit
`A210.native_tail_fold_manifest_without_manifest_sha256` scope, and the
execution profile reports that same value. This keeps the fold input contract
stable while the remaining fold-builder and posting-ingestion work migrates
from the current SQL/C bridge toward the final C/index-AM implementation.
After a successful fold, `ii42_model_semantic_native_tail_fold_manifest_status`
validates the persisted manifest against the active base payload. This matters
because the native semantic/tombstone/debt-log side pages are cleared after
fold apply; the folded base generation itself must carry enough verifiable
metadata for operators and future C maintenance code to prove payload identity
without re-reading the retired native tail.
`ii42_model_mutable_maintenance_status(...)` now mirrors that manifest status
under the base segment, maintenance view, and shared-holder readiness view, so
the main lifecycle endpoint reports whether the folded native base manifest is
present, valid, and why validation failed if it is not.

Semantic maintenance now also repairs a stale native semantic debt-log snapshot
before consuming semantic debt. The SQL ledger remains authoritative, but a
worker no longer treats valid-but-outdated native debt-log pages as a no-op: the
default generation-table bridge and the `native_semantic_delta_maintenance`
route both resynchronize the native log from the ledger before processing debt.
If the repair finds no pending semantic debt, it still reports a successful
`native_semantic_debt_log_repaired` maintenance action so workers and operators
can distinguish useful repair from a true no-op. The C worker comparator and
SQL `ii42_index_maintain_due(...)` helper both rank native semantic delta
repair/clear and native debt-log repair in the same native-repair priority
bucket. The SQL due-helper fallback now calls the public semantic maintenance
wrapper instead of bypassing it, so external scheduler calls take the same
per-index maintenance lock as C workers and still honor the
`native_semantic_delta_maintenance` route. Fallback execution therefore cannot
race a background worker or silently materialize a generation-table semantic
delta for an index that opted into native-only materialization.
Foreground SQL touch is also only a wakeup signal while a transaction is
active: `ii42_index_touch_maintenance()` records the offload pressure
immediately but defers the background worker launch until commit. This is
required for native semantic storage because the page append can be physically
visible before the SQL overlay/debt rows that describe it are committed; a
worker launched too early could otherwise treat the native side pages as stale
or unrelated tail state.
The A172 runtime-service smoke now asserts this boundary directly: an in-flight
transactional touch must return `mode=background_wakeup_deferred`, must not
advance the shared runtime success counter before commit, and only the
post-commit worker may consume the semantic debt through the shared runtime.
The same smoke now also guards the native posting-writer contract while real
native semantic debt is pending: the current C primitives and encoded native
data-plane must be attached and ready, but `final_c_index_am_posting_writer`
must remain explicitly unattached with
`writer_contract_gap=final_c_index_am_posting_writer_not_attached`. After
maintenance applies the encoded native page batch, the lock-owned apply result
must preserve the same current/target orchestration boundary. This prevents
tests from accidentally treating the current C/SPI bridge as the final
C/index-AM writer.
The smoke also exercises the public/manual
`ii42_model_semantic_native_posting_writer_apply(...)` entrypoint after a native
semantic tail already exists. That path must take and release the normal
maintenance lock, use the shared runtime service for encoding, append/merge the
encoded batch into native semantic pages, leave generation-table and SQL-overlay
semantic deltas empty, and keep the same final-writer gap visible.
The public pre-encoded wrapper is covered as a strict handoff boundary too: if
encoded rows were produced for an older native debt-log manifest and the pending
batch changes before apply, `ii42_model_semantic_native_posting_writer_apply_from_encoded(...)`
must reject the stale rows with `native_record_stream_inconsistent`, leave debt
and native side pages intact, avoid backend-local ONNX, release the maintenance
lock on the error path, and allow a later ordinary public writer pass to
converge.
The ordinary public native posting writer now has the same non-blocking
lock-contention proof: when another backend already owns the per-index
maintenance lock, `ii42_model_semantic_native_posting_writer_apply(...)` returns
`maintenance_lock_busy`, leaves SQL debt, native debt-log state, generation-table
deltas, SQL overlay rows, shared-runtime counters, and backend-local ONNX state
unchanged, then converges through append/merge after the lock is released.
`ii42_model_semantic_debt_maintain_native(...)` remains only the explicit
force-native operator/testing entrypoint, and it takes the same lock.

## Consistency Semantics

The model-backed index must not silently change PostgreSQL index semantics.
`eventual` should be recommended for large BM25+SAE workloads, but it should not
be made the hidden default without a separate explicit decision.

### `manual`

`manual` means the index can accumulate freshness debt until an explicit
maintenance call or rebuild.

Recommended use:

- offline builds;
- controlled test fixtures;
- bulk load followed by explicit publish;
- deployments where operators want full control over semantic refresh timing.

Behavior:

- foreground DML marks stale and records lightweight pending-write/delete
  counters, but does not record exact semantic encode debt;
- explicit `ii42_index_maintain(...)`, `ii42_index_refresh(...)`, or
  `REINDEX INDEX` rebuilds the index when the operator chooses;
- query uses the last valid index state plus any allowed visible delta;
- no automatic semantic rebuild is required;
- status must clearly expose lexical and semantic debt.

### `eventual`

`eventual` is the recommended production mode for large knowledge-base and RAG
workloads.

Behavior:

- foreground DML should stay lightweight;
- BM25 lexical delta can follow the existing eventual delta path;
- semantic documents that need encoding are recorded as semantic debt;
- background workers encode/build/compact when thresholds and worker slots allow;
- queries keep using the last valid base plus visible delta;
- maintenance failure never invalidates the previous generation.

Important semantic point:

If a newly inserted row has BM25 delta but no encoded SAE atoms yet, the index is
freshness-debt state. That is acceptable only under `eventual` or `manual`.
Status and diagnostics must show that semantic coverage is not caught up.

### `realtime`

`realtime` is only correct if foreground writes can complete every required
index component:

- BM25 token postings;
- text normalization/tokenization;
- model encoder execution;
- SAE atom postings;
- metadata/stat updates.

That is likely too expensive for real model-backed indexes. If supported, it
should be explicitly documented as high-cost and should fail loudly when the
model runtime cannot execute synchronously. It must not degrade into "BM25 is
fresh but SAE is eventual" while still claiming realtime hybrid semantics.

## Physical Model

The mutable hybrid index should be represented as one logical ii42 index with
multiple internal posting sources.

```text
ii42 index relation
    meta page
        consistency
        model/runtime/scoring ids
        active base generation/segment
        pending lexical debt
        pending semantic debt
        tombstone/deletion debt
        stale flags
    base segment
        immutable BM25 postings
        immutable SAE atom postings
        doc identity map
        blockmax / upper-bound metadata
    delta segment
        append-only BM25 postings
        append-only SAE atom postings when available
        pending encode doc refs when not available
    tombstone segment
        deleted/replaced doc identities
```

The current read-only EATMH generation is a useful base-segment prototype. A200
should evolve it into an index-owned segment rather than keeping it forever as a
fully detached sidecar.

## Term Namespace

BM25 tokens and SAE atoms should share a single sparse-posting abstraction but
must keep explicit namespaces.

Recommended representation:

```text
term_key = (source_kind, term_id)

source_kind:
    lexical_bm25
    semantic_sae
```

This avoids accidental collisions while allowing the query path to traverse both
types through the same accumulator and blockmax machinery.

## Document Identity

The current read-only generation uses dense `doc_ord` ordering. Mutable indexes
need a stable identity layer.

Recommended layers:

- `doc_key`: stable logical document key when configured, otherwise heap TID
  generation identity;
- `doc_seq`: monotonically assigned internal document sequence;
- `base_doc_ord`: compact ordinal inside a base segment;
- `delta_doc_seq`: global sequence id for delta documents;
- `heap_tid`: current heap tuple reference for visibility checks.

Queries should never assume that a base `doc_ord` alone is stable across
compactions. The final result path should map candidates back through the
document identity map and then apply heap/MVCC visibility.

SQL-facing SRF helpers that expose `ctid` must also respect PostgreSQL HOT
chains. Native index scans can return the root heap TID because the executor
performs heap/MVCC recheck, but SRF APIs bypass that executor path and are
commonly joined with `table.ctid = hit.ctid`. Those SRFs therefore resolve the
currently visible tuple TID before returning a hit, so non-indexed HOT updates
do not disappear from SQL-visible search results.

## Write Path

Foreground writes should follow the existing ii42 index AM flow as much as
possible.

### Insert

1. Existing index AM extracts lexical fields.
2. Lexical postings follow the configured consistency policy.
3. If the index has a model configuration:
   - `manual`: mark stale for explicit maintenance, without queuing semantic
     encode debt;
   - `eventual`: record semantic encode debt;
   - `realtime`: require synchronous semantic posting ownership; until the
     final index-AM semantic writer is attached, fail fast before queuing debt,
     invoking the runtime, or returning a BM25-fresh/SAE-stale result.
4. Update pending counters:
   - lexical delta records/bytes;
   - semantic pending docs;
   - semantic pending bytes estimate;
   - stale flags when thresholds are crossed.
5. Wake the same existing maintenance machinery when policy says it is due.

### Update

Updates are delete + insert from index-maintenance perspective.

Important optimization:

- if indexed lexical fields and model input text are unchanged, skip semantic
  encode debt;
- if only non-indexed fields change, do not mark semantic postings stale;
- if text changes, tombstone old identity and add new semantic debt.

### Delete

Delete should not rewrite base postings immediately.

1. Record tombstone/deletion debt.
2. Update pending delete counters.
3. Query-time visibility/tombstone filters skip deleted candidates.
4. Compaction removes old postings when worker rebuilds the segment.

## Query Path

The query path should not be Python-style late fusion. It should be one sparse
retrieval path:

```text
query text
    -> lexical query terms
    -> query SAE atoms
    -> unified query term list
    -> base postings + delta postings
    -> one accumulator
    -> one score
    -> top-k
```

Expected score components:

- BM25 contribution;
- SAE atom impact contribution;
- source-specific normalization from scoring artifact;
- query-level BM25/SAE scale if the promoted model supplies one;
- freshness diagnostics.

The SQL-visible result should expose enough diagnostics to distinguish:

- lexical-only hit;
- semantic-only hit;
- mixed hit;
- base hit;
- delta hit;
- skipped tombstone;
- semantic freshness debt.

## Maintenance Path

A200 should reuse the current ii42 maintenance worker design.

The worker should:

1. discover due ii42 indexes using the existing stale/debt mechanism;
2. prioritize hot/stale/debt indexes with the existing worker policy;
3. acquire one index maintenance lock;
4. load current base segment and pending delta/tombstone state;
5. encode semantic pending documents if needed;
6. build or compact the next base segment;
7. validate generation/segment metadata;
8. publish the new active segment atomically;
9. preserve the previous active segment until no backend references it;
10. clear or reduce debt counters;
11. release the worker slot.

No separate SAE scheduler should be introduced. If semantic encoding needs
batching or runtime-specific behavior, that should be a phase inside the same
maintenance worker pipeline.

## Debt And Thresholds

Existing BM25 debt metrics should stay, and semantic-specific debt should be
added as additional fields.

Recommended metadata:

- `pending_write_tuples`;
- `pending_delete_tuples`;
- `delta_record_count`;
- `delta_bytes`;
- `pending_semantic_encode_tuples`;
- `pending_semantic_encode_bytes_estimate`;
- `semantic_delta_bytes`;
- `semantic_tombstone_tuples`;
- `semantic_stale`;
- `last_semantic_generation_id`;
- `last_semantic_encode_error`;

Recommended thresholds:

- reuse existing `auto_rebuild_delta_bytes`;
- add `auto_rebuild_semantic_delta_bytes`;
- add `auto_rebuild_semantic_pending_docs`;
- keep global worker launch/timer/limit semantics unchanged.

If these new thresholds are omitted, they should default to values derived from
existing rebuild thresholds rather than inventing a separate policy.

## Encoding Timing

Semantic encoding is the only new expensive phase.

Recommended policy:

- build time: encode all rows as part of normal index build;
- `eventual` insert/update: queue semantic debt, do not block foreground writes;
- `manual` insert/update: mark stale only; do not queue semantic debt;
- `realtime`: synchronous encode only if explicitly configured and runtime is
  ready;
- compaction: batch encode pending docs before building the new segment.

The encoder runtime must be model-swappable. The mutable maintenance path should
call the same model runtime boundary used by:

- `ii42_model_encode_text(...)`;
- runtime-backed generation build workers;
- future promoted ONNX artifact execution.

## WAL, MVCC, And Crash Recovery

This is where A200 must be stricter than the current SQL-side generation
prototype.

Required properties:

- foreground DML writes must be crash-safe;
- semantic debt records must survive restart;
- a partially built new segment must not replace the previous active segment;
- publishing a new active segment must be atomic;
- old active segments must remain readable by existing backends;
- tombstones must be respected until compaction removes dead postings;
- uncommitted writers must not cause maintenance to publish an invalid snapshot.

This aligns with the current BM25 maintenance design where stale indexes remain
queryable and workers catch up asynchronously.

## Relation To Current A190 Generation Builder

A190d is useful but not the final mutable model.

Current A190d:

- creates a persistent staging atom table;
- processes source rows in chunks;
- publishes a read-only generation;
- uses job-level advisory locking to skip duplicate workers;
- records batch-control, failure, and cleanup policy metadata;
- supports explicit cleanup.

A200 should reuse the lessons:

- chunked work;
- explicit lifecycle state;
- failure-safe publish;
- cleanup metadata.

But A200 should move from a SQL-side generation builder toward index-owned
mutable state and the existing ii42 maintenance worker.

## API And Reloptions

Model-backed mutable indexes should keep using the normal ii42 reloption
surface.

Existing options to reuse:

- `consistency`;
- `auto_preload`;
- existing rebuild thresholds;
- existing maintenance worker/timer GUCs;
- model metadata reloptions:
  - `model`;
  - `model_path`;
  - `atom_space`;
  - `scoring_profile`.

Potential new reloptions:

- `semantic_consistency` should be avoided unless absolutely necessary, because
  it splits the mental model. Prefer one `consistency` for the whole hybrid
  index.
- `semantic_encode_policy` may be useful only for explicit realtime/manual
  edge cases.
- `auto_rebuild_semantic_pending_docs`;
- `auto_rebuild_semantic_delta_bytes`;
- `semantic_delta_max_rows`;
- `semantic_tombstone_ratio`.

Recommendation:

Start with minimal semantic thresholds and inherit as much as possible from
existing BM25 options.

## Status And Diagnostics

The status API must make freshness visible.

Required fields:

- lexical freshness status;
- semantic freshness status;
- pending lexical writes/deletes;
- pending semantic encode docs;
- pending semantic delta bytes;
- tombstone count/ratio;
- active base generation/segment id;
- latest maintenance worker attempt;
- latest encode error;
- latest publish error;
- whether query results include delta overlay.

The user-facing message should be clear:

```text
index is queryable, semantic postings are 4,210 docs behind
```

instead of a vague stale flag.

## Testing Plan

### Unit / Smoke

- create model-backed index with `manual`, insert rows, verify debt counters;
- create model-backed index with `eventual`, insert rows, verify worker due
  state;
- update indexed text, verify old identity tombstone and new semantic debt;
- update non-indexed column, verify no semantic debt;
- delete row, verify tombstone and query skip;
- crash-safe publish simulation: failed segment does not replace old segment.

### Query Correctness

- base-only query;
- delta-only query;
- base + delta query;
- tombstoned base doc skipped;
- semantic pending doc appears only under documented eventual semantics;
- BM25-only hit, SAE-only hit, mixed hit diagnostics.

### Maintenance

- worker compacts semantic delta into new base;
- worker respects existing global worker limit;
- hot/stale priority is preserved;
- failed encode marks error but keeps old base active;
- cleanup removes old segments only after safe reference release.

### Performance

- foreground insert/update latency under `eventual`;
- query latency with growing semantic delta;
- query latency before/after compaction;
- maintenance throughput;
- memory resident size;
- preload latency;
- WAL volume.

## Milestone Breakdown

### A200a: Design And Metadata

Status: implemented.

Completed:

- finalized the A200 design;
- added `ii42_model_mutable_maintenance_contract()`;
- added `ii42_model_mutable_maintenance_status(index)`;
- added smoke tests for no separate scheduler, no hidden default, unified
  consistency, and lexical-debt source wiring.

Still deferred to native semantic query/fold integration:

- physical semantic debt counters in index metadata;
- semantic debt updates from index AM DML paths;
- default promotion of native-only semantic materialization for all
  model-backed indexes;
- native index-AM ingestion of worker-processed semantic debt.

### A200b: Prototype Delta Overlay

Status: implemented.

- use the existing read-only generation as base;
- add a small semantic delta overlay table/payload;
- query base + delta through one model-backed query function;
- validate scoring and tombstone semantics.

Implemented SQL surface:

- `ii42_model_mutable_overlay_create_table(...)`;
- `ii42_model_mutable_delta_upsert(...)`;
- `ii42_model_mutable_tombstone(...)`;
- `ii42_model_mutable_overlay_status(...)`;
- `ii42_model_mutable_query_atoms(...)`;
- `ii42_model_mutable_query_text(...)`.

A200b is intentionally an overlay prototype. It is safe for correctness
validation and operator diagnostics, but it does not yet make foreground DML
write semantic debt into native ii42 index metadata.

### A200c: Semantic Debt Ledger

Status: implemented.

- add `semantic_overlay_table` and `semantic_debt_table` reloptions;
- create an index-owned semantic debt ledger keyed by
  `(index_oid, generation_id, doc_key)`;
- record pending `encode`, `replace`, and `delete` debt without foreground
  semantic encoding;
- resolve debt to `encoded`, `failed`, or `cleared`;
- expose debt through `ii42_model_mutable_maintenance_status(index)`;
- let configured mutable query wrappers resolve the overlay table from index
  reloptions.

A200c is the SQL and control-plane contract for semantic debt. It still does
not mutate native index pages from foreground DML and does not launch workers by
itself.

### A200d: Maintenance Worker Integration

Status: implemented as worker-facing SQL overlay/debt processing.

- process pending semantic debt through `ii42_model_semantic_debt_maintain`;
- use `FOR UPDATE SKIP LOCKED` so parallel workers skip rows already claimed by
  another worker;
- encode pending documents through the configured model runtime;
- publish encoded rows into the SQL semantic delta overlay;
- turn delete debt into tombstones;
- resolve debt to `encoded`, `cleared`, or `failed`;
- call semantic maintain from `ii42_index_maintain_due`;
- let the C background worker fall back to the SQL due helper for semantic-only
  debt when lexical C maintenance did not maintain an index.

A200d still does not make foreground DML hooks write semantic debt directly into
native index pages. A200f adds the configured SQL bridge; physical page
integration remains future native-storage work.

### A200e: Production Hardening

Status: implemented as worker-facing hardening for the SQL overlay/debt path.

- add semantic maintenance reloptions:
  `semantic_maintenance_max_rows`,
  `auto_rebuild_semantic_pending_docs`, and
  `auto_rebuild_semantic_delta_bytes`;
- expose `ii42_model_semantic_maintenance_plan(index)` with due, urgent,
  compaction, batch-size, threshold, WAL-backed table, and failure-safety
  diagnostics;
- make `ii42_model_semantic_debt_maintain(index)` derive batch size from the
  index plan when no explicit `max_rows` is passed;
- keep `ii42_index_maintain_due(...)` on the same ii42 lifecycle while using the
  configured semantic batch plan;
- add smoke coverage for threshold due, direct maintain with derived batch size,
  failure-safe encode handling, delete tombstones, and `VACUUM` safety.

A200e is still not native semantic page storage. It hardens the contract that
the native implementation must preserve.

### A200f: Configured Bridge And Compaction Finalizer

- add a configured foreground debt bridge:
  `ii42_model_semantic_debt_record_index(...)`;
- add `ii42_model_mutable_compaction_plan(...)` to expose whether semantic debt
  has converged and whether overlay/tombstones require a rebuilt generation;
- add `ii42_model_mutable_compaction_finalize(...)` to clear retired-generation
  SQL overlay/debt rows only after the active generation has moved forward;
- keep active-generation compaction conservative: SQL helpers do not pretend to
  merge base+delta into native pages.

### Next: Native Semantic Page Storage And Compaction

- move semantic debt authority from the SQL ledger plus native mirror into
  native index-AM records without changing the public debt-maintenance
  contract;
- compact SQL overlay/debt into native index-AM storage or a durable shared
  delta/base generation publish path;
- move VACUUM-derived delete debt from SQL-authoritative rows to
  native-authoritative tombstone records once the native authority path is in
  place;
- add crash, WAL, long-running backend cache, and large-corpus latency tests.

### A210: Shared Delta Generation Convergence

Status: A210 shared-holder convergence is implemented for the production
issue #18 path: lexical/text delta, tombstone delta, raw phrase/boolean
queries, field-aware queries, masked-query tombstone/wakeup behavior,
semantic generation-table publication, shared-resident semantic repair,
compact/spill full-fold publish, and SQL/C worker priority alignment are all
covered by focused smoke tests. A200g/A210 add native semantic payload,
debt-log, and tombstone page primitives with store/load/status/clear APIs and
rebuild/crash lifecycle smoke. Mutable semantic queries can already use native
semantic pages in fallback/non-hot paths, and the status surface now reports
the effective foreground semantic delta query source plus native tombstone
segment state. The remaining design work is making native pages the default
source where appropriate and extending lightweight multi-delta fold beyond
payload-complete lexical/tombstone deltas into semantic-tail compaction.
Maintenance status also reports the current fold strategy and the target
lightweight fold candidate state when shared lexical/tombstone deltas are
query-covered but threshold policy still wants convergence; payload-complete
durable deltas now execute through the lightweight fold path before falling back
to heap-backed online rebuild for counter-only, stale, corrupt, or incomplete
delta states. The status surface also exposes the exact lightweight-fold
blocker, so operators and schedulers can distinguish safe fold candidates from
counter-only or incomplete payload debt.
Covered shared lexical deltas are now also tested on both sides of the policy
boundary: they stay query-complete and not due while below fold thresholds, and
become maintain-due and converge to a clean folded base when the threshold is
lowered.

Goal: fix the pre-existing eventual-consistency delta gap while adding native
SAE support, so BM25 and SAE do not diverge into two mutable-index designs.

Core tasks:

1. Define a delta generation identity:
   - database OID;
   - index OID;
   - relation locator / relfilenode identity;
   - base active generation id and cache epoch;
   - delta start block and page count;
   - lexical delta record and byte counts;
   - semantic delta record and byte counts;
   - tombstone record count;
   - pending semantic encode debt count.
2. Extend the shared generation registry to distinguish:
   - immutable base generation;
   - lexical delta generation;
   - semantic delta generation;
   - tombstone generation;
   - optional combined hybrid delta generation.
3. Add lease semantics for `base + matching delta` attachment:
   - statement/SRF/scan leases pin all attached generations together;
   - generation identity is rechecked after acquiring leases;
   - GC retires old base/delta only after refs drain.
4. Move shared-preload delta reads away from backend-local materialization:
   - if matching shared delta exists, attach it;
   - if no matching shared delta exists, wake maintenance/materialization;
   - avoid per-backend large delta rebuilds in shared-preload mode.
5. Add a maintenance phase that materializes eligible deltas:
   - lexical delta mini-index from existing delta pages;
   - semantic delta generation from encoded semantic debt through the current
     generation-table bridge until native semantic pages are wired into query
     and fold paths;
   - tombstone generation from delete/replace debt;
   - no full base rebuild required when delta is small and within policy.
6. Add a fold/compaction phase:
   - build next base from old base + shared deltas + tombstones;
   - publish by metapage switch;
   - preserve previous base/delta until readers drain;
   - clear only the debt that was actually folded.
7. Keep threshold semantics explicit:
   - `query_overlay_max_*` remains foreground query budget only;
   - `auto_rebuild_threshold` and `auto_rebuild_delta_bytes` remain catch-up
     pressure;
   - semantic thresholds derive from lexical thresholds unless configured;
   - `auto_preload > 0` may preload base and eligible shared deltas, but should
     not force an immediate full rebuild.

Acceptance criteria:

- A newly inserted eventual document is either represented in a reusable shared
  delta generation or visibly counted as freshness debt with a worker wakeup.
- Connection-pooled queries do not each build their own large delta overlay.
- arXiv/PubMed-scale shared-preload indexes can keep base resident and converge
  through background materialized deltas or full fold, without blocking
  foreground writes.
- A model-backed index exposes lexical debt, semantic encode debt, semantic
  delta debt, and tombstone debt through one status surface.
- Crash/restart tests prove that a partial delta materialization never replaces
  the last valid base/delta pair.
- Standby can attach replayed base/delta generations but does not run
  primary-side maintenance.

Phase 1/2 implementation:

- shared-preload/eventual textlike indexes no longer build backend-local text
  delta mini-indexes for foreground queries when there is pending/delta debt;
- the offload guard is tied to the current query's attached shared-preload base
  entry, so backend-local cache entries keep the existing bounded local overlay
  behavior;
- query/write touches are allowed to wake a bounded dynamic maintenance worker
  even when the shared-preload supervisor is enabled;
- dynamic maintenance worker launches now reserve against the shared
  maintenance-worker limit before calling PostgreSQL background-worker
  registration. The reservation is released when the worker claims a real
  maintenance slot, fails to claim one, or registration fails, and stale
  reservations are pruned after a short timeout. This keeps foreground touches
  agile while preventing a burst of pooled backends from all registering
  workers before `active_maintenance_workers` catches up. The dynamic worker
  process also registers an early shared-memory exit hook before
  opening the database connection, so a connection-init failure releases the
  pending launch immediately instead of waiting for the timeout prune.
  The legacy
  `ii42_generation_cache_state(...)` string and structured
  `ii42_generation_cache_state_json(...)` surface expose
  `pending_maintenance_worker_launches` so operators can verify that touch
  wakeups are reserving capacity instead of flooding background-worker
  registration. The shared-preload delta wakeup smoke asserts both the legacy
  string and structured JSON counter so future scheduler changes cannot
  silently remove this operator signal;
- hot resident indexes with uncovered eventual debt are considered
  maintenance-due even below the large rebuild thresholds, regardless of
  `auto_preload`; `auto_preload` is only a proactive warmup/priority hint;
- `ii42_index_maintain_due(max_indexes)` now orders SQL-scheduled maintenance
  by a shared-holder-first priority surface: semantic shared-resident republish
  pressure, hot semantic threshold pressure, stale lexical debt, hot
  shared-preload candidates, materialized-delta publish/delete pressure,
  lexical debt records, semantic debt records, lexical/semantic debt bytes,
  preload priority, relation size, and OID. It also filters out indexes with no
  lexical or semantic maintenance pressure before attempting work. This keeps
  restart/eviction repair for hot resident semantic generations ahead of
  ordinary lexical catch-up, so foreground queries can keep offloading to the
  shared holder instead of rebuilding backend-local semantic payloads. The C
  worker's semantic due probe exposes the same
  `shared_resident_publish_due` priority bit and sorts it before urgent,
  stale, hot, auto-preload-active, semantic-delta publish/delete, debt-size,
  and preload-priority criteria, so built-in background workers and external
  SQL schedulers converge on the same repair-first order.
  `ii42_model_semantic_maintenance_plan(...)` now exposes a
  `maintenance_priority` object with `shared_resident_repair`,
  `matches_c_worker_comparator`, and `matches_sql_due_helper`, so tests and
  operators can verify that shared-holder repair did not drift back to a
  stale-first scheduler. The aggregate
  `ii42_model_mutable_maintenance_status(...)` also mirrors these priority
  bits under `shared_holder_readiness`, so operators can read one compact status
  surface without manually drilling into the semantic plan;
- `ii42_generation_cache_state(...)` exposes
  `foreground_delta_offload_wakeups`, a shared counter for foreground
  text-delta deferrals that were offloaded to the maintenance path, and
  `foreground_delta_local_builds`, a shared counter for actual backend-local
  delta overlay builds. The A210 smoke uses both counters to prove that hot
  shared-holder resident queries offload uncovered debt instead of building
  private overlays.
  `ii42_generation_cache_state_json(...)` mirrors the same cache/debt/shared
  holder state as structured JSON, so operators and external schedulers do not
  need to parse the legacy diagnostic string to decide whether a hot index is
  base-resident, has a shared delta/tombstone layer, or is only offloading
  foreground debt. The JSON surface also exposes
  `shared_holder_effective.lexical_delta_covered`,
  `shared_holder_effective.tombstone_delta_covered`,
  `shared_holder_effective.lexical_delta_publish_due`, and
  `shared_holder_effective.tombstone_delta_publish_due`, so schedulers can
  distinguish covered debt from work that still needs a background publish.
  These publish-due fields require the base generation to be shared-holder
  resident. A cold eventual index with small pending debt is not promoted to
  shared-holder publish work solely because the holder is absent;
- raw phrase/boolean query preparation now uses the same covered-delta rule as
  the token path for scalar text/varchar indexes. When the shared-holder base is
  resident, a current shared text delta is prepared through the existing raw
  verifier against the delta mini-index and merged back into the base result
  state, while a current shared tombstone delta filters both base and delta
  hits. The nested delta verifier suppresses recursive delta merging, so
  phrase/boolean semantics stay on the existing verifier path without building a
  second backend-local large overlay;
- masked queries no longer bypass the whole shared-holder delta path. Because
  `weight_mask` is sized to the leased base generation, fresh delta doc ids
  cannot be safely merged with invented mask weights. Token, field-aware, and
  raw masked queries therefore apply resident shared tombstones, skip unmaskable
  write-delta hits, and wake maintenance so the writes converge into a new base
  generation that can be masked explicitly;
- `ii42_index_maintain_due(...)` now consumes the same structured state. A
  shared-holder index with pending lexical counters but current matching
  text-delta/tombstone generations is not selected merely because the base has
  unfoldered debt; stale/full-fold pressure and uncovered publish-due work still
  enter the normal priority queue. This prevents external schedulers from
  repeatedly waking no-op maintenance for hot indexes that are already query
  complete through the shared holder. It also matches the C worker by not
  selecting cold nonresident debt unless stale or rebuild thresholds make the
  index due through the normal maintenance path;
- foreground cache attach now uses the same lexical due predicate before waking
  background maintenance. Raw pending counters are not enough to disturb a hot
  shared holder once matching shared text-delta/tombstone generations are
  current; uncovered debt, stale/corrupt state, and threshold-driven fold
  pressure still wake the worker path;
- foreground cache attach also skips the pending-maintenance wait/overlay probe
  for textlike eventual indexes whose pending write/delete counters are already
  covered by current shared text-delta/tombstone generations. This removes an
  avoidable per-backend lock probe from the hot query path while preserving the
  existing local overlay fallback for non-shared or uncovered debt. The covered
  shortcut now explicitly requires the current base generation itself to be
  shared-resident; a matching shared delta without a resident base is not enough
  to claim query completeness or skip the normal maintenance/overlay path;
- field-aware multicolumn text indexes use the same shared-holder contract as
  ordinary token queries. Counter-only debt from a resident field-aware base
  wakes maintenance instead of forcing per-backend delta work, and once shared
  text-delta or tombstone generations are current, weighted field-aware queries
  attach those layers without incrementing foreground offload wakeups;
- A172 now locks that contract with a connection-pool-style field-aware smoke:
  a different backend first sees the resident base as an eventual base-only read
  while uncovered append debt is still waiting for shared publication, proves
  `foreground_delta_local_builds` does not increase, then runs maintenance to
  publish the shared text delta and proves the same weighted field-aware query
  sees the fresh row through the shared holder without a private overlay build;
- `ii42_model_mutable_maintenance_status(...)` now mirrors lexical and tombstone
  shared-holder readiness from `ii42_generation_cache_state_json(...)` under
  `shared_holder_readiness`. The model-backed operator surface therefore shows
  `lexical_delta_covered`, `tombstone_delta_covered`,
  `lexical_delta_publish_due`, `tombstone_delta_publish_due`,
  `covered_delta_query_complete`, `covered_delta_fold_due`, and
  `foreground_delta_offload_wakeups`, and
  `pending_maintenance_worker_launches` without forcing schedulers to combine
  two status APIs manually. `covered_delta_query_complete` means hot queries
  are complete through the shared holder; `covered_delta_fold_due` means that
  same query-complete state has accumulated enough covered debt to warrant a
  background fold and is selected by `ii42_index_maintain_due(...)`;
- the shared-preload registry now records a generation kind and reports
  base/text-delta/semantic-delta/tombstone entry counts;
- append-only text deltas can now be materialized by maintenance into a
  reusable shared `TEXT_DELTA` mini-index generation instead of forcing an
  immediate full base rebuild;
- tombstone deltas can now be materialized by maintenance into a reusable shared
  raw doc-id generation. Query paths attach this generation as a negative layer
  and filter base/delta hits without rebuilding the base generation;
- foreground queries that hold a shared-preload base generation first try to
  attach matching shared text-delta and tombstone generations and only fall
  back to worker wakeup/base-only eventual reads when no matching shared delta
  is resident;
- manual preload and shared-delta maintenance results now report
  `shared_text_delta_published` and `shared_tombstone_delta_published`
  separately. This keeps the operator surface aligned with the physical
  lifecycle: a hot index can show that append-only text deltas and delete
  tombstones are both resident instead of collapsing all delta work into one
  ambiguous `shared_delta_published` flag;
- counter-only debt with `pending_write_tuples` or `pending_delete_tuples` but
  no concrete delta records now also records a foreground offload and wakes
  maintenance from text and field-aware query paths. This covers the original
  issue #18 arXiv shape where the resident base was healthy but fresh rows were
  represented only as pending maintenance debt. The structured
  `ii42_generation_cache_state_json(...)` surface exposes this shape as
  `debt.counter_only` and
  `shared_holder_effective.needs_background_convergence`; once a matching
  shared text-delta or tombstone layer is current, the same JSON surface marks
  the debt as covered instead of repeatedly asking schedulers to publish it;
- manual `ii42_generation_cache_preload(index)` and auto-preload workers now
  treat resident hot indexes as more than base-cache warmup. After the base
  generation is shared-resident, the same preload path also attempts to publish
  eligible shared text-delta and tombstone layers. For model-backed indexes it
  also attempts the SQL/generation-table semantic delta publish/delete bridge
  when the overlay signature is not current, and republishes a durable-current
  compact semantic delta when restart or eviction left it outside the shared
  holder. This lets explicit operations prewarm and startup/timer warmup make
  the shared holder useful before the first foreground query has to discover
  the delta tail. The semantic bridge is best-effort during preload: SQL bridge
  errors are reported in the preload diagnostic but do not break
  base/text/tombstone shared-preload residency;
- standby and recovery behavior is now explicit: `ii42_index_refresh(...)`,
  `ii42_index_maintain(...)`, `ii42_index_try_maintain(...)`, and
  `ii42_index_maintain_due(...)` return or behave as no-ops while PostgreSQL is
  in recovery. The shared-preload supervisor can still launch preload-only
  workers on a standby so replicated immutable generations can be warmed into
  the local shared holder, but no online/full rebuild or semantic debt
  maintenance is attempted outside the primary;
- auto-preload candidate ordering now uses the same semantic plan surface as
  maintenance. Within the same `auto_preload` priority, a durable-current
  semantic delta that only needs shared-holder republish is handled before
  generic large-generation warmup, followed by urgent semantic pressure,
  ordinary semantic maintenance pressure, relation size, and OID. This keeps
  startup/timer warmup aligned with issue #18 shared-holder convergence instead
  of treating semantic repair as a side effect of catalog-size ordering;
- shared delta identity is stricter than base identity: base generations ignore
  mutable delta counters, while text-delta/tombstone generations must match the
  delta page range, record count, byte count, and pending counters they were
  built from;
- model-backed semantic overlay rows can now be published by maintenance or
  operators into a reusable A210 semantic delta generation. Mutable semantic
  queries compare the overlay signature against the published generation
  metadata and attach the materialized generation by id when it matches,
  avoiding per-query `bytea` generation rebuilds on the hot path. Publication
  also attempts `ii42_sae_generation_publish_shared(...)`, so shared-preload
  deployments can place the compact semantic delta bytes in the shared holder
  before the first pooled backend query touches them. This is the
  SQL/generation-table bridge toward native shared semantic delta storage, not
  the final index-AM page format;
- the C `ii42_index_try_maintain(...)` path now attempts the bounded SQL
  semantic debt bridge before falling through to online/full base maintenance.
  A touch/worker wakeup can therefore encode pending semantic debt, publish the
  reusable semantic delta generation, and return `reason=semantic_delta` without
  rebuilding the immutable base when only SAE delta work is due;
- background candidate discovery now probes the semantic maintenance plan for
  model-backed indexes with configured overlay/debt tables. Semantic-only debt
  enters the same hot/stale/debt priority queue as lexical debt, instead of only
  being handled by a fallback SQL sweep after no C candidates exist;
- when a mutable semantic query finds delta rows but no current materialized
  semantic delta generation, it still uses the correctness-preserving local
  fallback for that query, but it now also calls the shared maintenance touch
  path. That keeps foreground backends non-blocking while nudging the shared
  holder to publish the reusable semantic delta for later pooled connections;
- `ii42_model_mutable_semantic_delta_status(index)` reports whether the
  materialized semantic delta generation exists, still matches the current
  overlay signature, needs publish, or can be deleted. The aggregate
  maintenance status embeds this bridge state and a `shared_holder_readiness`
  summary so schedulers and operators can tell whether the hot holder avoids
  backend-local semantic delta rebuilds without manually reading generation
  metadata;
- `ii42_sae_generation_cache_state()` now exposes `loads`,
  `revision_checks`, `stale_reloads`, `serialized_bytes`,
  `query_resident_bytes`, `metadata_bytes`, and `shared_resident` for parsed
  SAE/evidence generations. The A210 smoke checks materialized semantic delta
  first-load and second-query hit behavior. In shared-preload deployments,
  compact `EATMH002` by-id generations can now attach raw resident bytes from
  the shared arena. The normal parsed-cache path reports
  `shared_resident = true` when it uses or publishes those bytes, while
  `ii42_evidence_atom_query_by_id(...)` can now bypass the parsed cache
  entirely and traverse the resident compact bytes through a validated
  offset-view. `query_resident_bytes` remains useful for parsed-cache sizing,
  while `metadata_bytes = 0` is the compact-generation target;
- A210 adds `EATMH002` compact evidence-atom generations through
  `ii42_evidence_atom_build_compact_generation(...)` and
  `ii42_evidence_atom_build_compact_generation_from_table(...)`. Compact
  generations deliberately omit document-id and atom-name strings, keep only
  fixed-width query-hot arrays, and are query-compatible with `EATMH001`.
  A210 semantic delta publication and foreground local fallback now use this
  compact builder, so reusable semantic delta generations no longer carry
  query-cold metadata and shared-preload by-id queries can traverse the
  compact resident bytes without first rebuilding backend-local arrays;
- `ii42_sae_generation_publish_shared(generation_table, generation_id,
  payload_kind)` gives maintenance and operator code an explicit way to publish
  a compact generation-table row into the shared resident holder. Semantic
  delta publication calls it after upsert. Shared-preload tests now verify that
  an explicitly published compact generation can be queried directly from the
  resident bytes on the first read without populating the backend parsed cache;
- `ii42_sae_generation_shared_resident(generation_table, generation_id,
  payload_kind)` gives status and operator code a cheap attach/release probe
  for generation-table shared residency. `ii42_model_mutable_semantic_delta_status`
  now reports `shared_resident_holder_available`, `shared_resident_current`,
  and `shared_resident_publish_due`, so schedulers can distinguish
  "materialized in the generation table" from "already useful in the shared
  holder". When a durable compact semantic delta is current but missing from
  the shared holder after restart or eviction, `ii42_model_semantic_maintenance_plan`
  returns `semantic_delta_shared_resident_publish_due`, and
  `ii42_model_semantic_debt_maintain` calls
  `ii42_model_mutable_semantic_delta_publish_shared(...)` to publish the
  existing generation row without rebuilding it. Manual and auto-preload use
  the same shared publish path, so hot startup prewarm can repair the shared
  resident holder before any pooled foreground query runs;
- `ii42_model_mutable_query_atoms(...)` and
  `ii42_model_mutable_query_text(...)` now accept
  `allow_local_semantic_delta_fallback`. Its default `NULL` value uses
  the C-level `ii42_index_shared_preload_resident(index)` helper as an
  automatic policy: resident shared-holder indexes return a base+tombstone
  eventual read, wake maintenance, and avoid building backend-local semantic
  delta payloads before the reusable materialized generation is published.
  SQL overlay delta rows are loaded into backend temp storage only after the
  query has chosen a shared-resident materialized delta or explicit local/debug
  fallback, and tombstone rows are copied only when the overlay signature shows
  delete debt is actually present. For shared-resident materialized semantic
  deltas, foreground queries score directly from the shared generation and only
  map returned doc ordinals back to overlay identities, avoiding a full backend
  copy of delta atom arrays. The expensive full overlay signature is now also
  delayed until a matching materialized semantic delta exists; unpublished or
  shared-holder-miss eventual reads use a counts-only overlay probe and worker
  wakeup, leaving full `delta_postings`/byte accounting to maintenance/status
  APIs; standalone eventual indexes keep the correctness-preserving local
  fallback. Callers can still pass explicit `true`/`false` for debugging or
  controlled experiments. Realtime model-backed mutable queries do not use that
  escape hatch implicitly: if semantic debt is pending, or overlay rows are not
  represented by a current materialized semantic delta, status reports
  `semantic_delta_generation_not_materialized_for_realtime` and the query fails
  fast instead of silently returning an eventual base-only view. The configured
  text-query entrypoint is covered by the same guard after runtime encoding, so
  SQL callers cannot bypass realtime semantics by using string input instead of
  explicit atom arrays. Aggregate maintenance status uses the same ordering:
  pending semantic debt blocks realtime query readiness before semantic-delta
  materialization is considered, and reports
  `pending_semantic_debt_for_realtime` with
  `realtime_pending_semantic_rows`. The configured text-query entrypoint now
  runs that aggregate readiness preflight before runtime encoding, so a known
  impossible realtime query does not consume shared runtime/model work first.
  The lower-level atom-query path performs the same pending-debt preflight before
  overlay relation access, so realtime semantic debt cannot fall through into
  local overlay scans or temp-table setup. Realtime semantic DML is now guarded
  at the index-AM debt-recording boundary as well: if a model-backed index has
  semantic overlay/debt reloptions and the semantic input changes, the write
  fails until a synchronous semantic posting writer is attached. Eventual
  remains the recommended mutable policy for deferred semantic maintenance. A172
  now repeats the DML and text-query realtime guards in the runtime-enabled ONNX
  fixture and verifies the fail-fast path leaves shared-runtime counters and
  backend-local ONNX cache state unchanged;
- semantic shared-holder readiness now separates three foreground states:
  `semantic_delta_query_complete` means the current semantic delta is usable by
  the foreground query, `semantic_delta_foreground_offload_active` means a
  shared-preload resident index is deliberately serving base+tombstone and
  waking maintenance until the semantic delta is shared-resident, and
  `avoids_backend_local_semantic_delta_rebuild` means the pooled backend will
  not build a private semantic delta payload even if semantic freshness is still
  pending;
- the shared-preload smoke now opens several fresh backend connections in the
  resident-miss state and verifies that none of them builds a private semantic
  delta cache or temp delta table. Connection-pooled traffic therefore remains
  base+tombstone eventual plus maintenance wakeup until the shared holder is
  repaired;
- the shared-preload smoke also covers the raw complex-query gap directly:
  a phrase query must find a newly inserted `"shared fresh"` document through
  the shared text-delta mini-index, and a phrase query for `"shared doomed"`
  must not return a document deleted through the shared tombstone delta;
- the same smoke verifies that masked token/raw queries no longer return before
  the shared-holder path: in a covered text+tombstone state they apply the
  tombstone layer and increment the foreground offload counter to request base
  convergence for unmaskable write-delta docs;
- `ii42_model_mutable_semantic_delta_status(...)` is also counts-first. It only
  computes the full atom-array overlay signature when an existing materialized
  semantic delta must be compared for exact currentness. Unpublished deltas and
  empty overlays expose a lightweight incomplete signature with a
  `signature_skipped_reason`, so scheduler/status polling does not become a
  hidden backend-local atom-array scan. Generation byte size is recorded in
  metadata at upsert time and status reads that metadata value instead of
  detoasting the durable bytea payload;
- aggregate mutable maintenance and compaction status now reuse
  `ii42_model_semantic_maintenance_plan(...)` sub-status instead of separately
  rescanning debt, overlay, and semantic delta state. Status polling remains a
  backend-light observation path rather than another source of per-backend
  overlay/signature work;
- semantic delta lifecycle cleanup is part of the same due/worker path:
  when the SQL overlay has no delta rows but an old materialized semantic
  generation still exists, `semantic_delta_generation_delete_due` is selected
  by `ii42_index_maintain_due(...)` and cleared through
  `ii42_model_mutable_semantic_delta_publish(...)`. Generation-specific
  shared-resident probes return `false` after delete instead of surfacing a
  missing-row error, so cleanup checks stay idempotent;
- failed semantic delta publication preserves the last valid materialized
  generation. The smoke injects a corrupt overlay row, verifies publication
  fails without deleting or replacing the current compact semantic delta, then
  removes the bad row and republishes the valid shared-resident generation;
- worker-facing semantic delta publication failure now stays in the semantic
  bridge lane. If no lexical/shared-delta maintenance is due, the C maintenance
  path returns `reason=semantic_delta_error` or
  `reason=semantic_delta_no_progress` instead of falling through to an online
  base rebuild. The SQL due helper returns that attempted/error result rather
  than re-entering the same failing PL/pgSQL publish path in the same cycle.
  The background worker fallback also treats these attempted/no-progress
  semantic results as non-progress and only uses the SQL fallback when the C
  candidate scan found no due index at all. A hot index with a bad semantic
  delta therefore does not retry the same bridge failure twice or report a
  successful worker cycle until a real shared-holder publish, semantic debt
  encode, or lexical fold has happened;
- realtime mutable model preflight is now a narrow observation path.
  `ii42_model_mutable_realtime_readiness(...)` reads only reloptions, semantic
  debt counters, and semantic-delta currentness. It intentionally does not read
  deployment/runtime/query-encoder status and does not require a model session.
  It also short-circuits before semantic-delta status when eventual policy or
  pending semantic debt already determines readiness. `ii42_model_mutable_query_text(...)`
  uses that helper before runtime encoding, and aggregate mutable status embeds
  the same helper result under `consistency.realtime_readiness` so operator
  status and product query fail-fast behavior cannot drift.
- online/native side-page race results are also classified as non-progress:
  `reason=meta_changed` from lightweight fold and
  `reason=native_semantic_meta_changed` from heap-backed online publish are
  filtered by both the C background worker counter and
  `ii42_index_maintain_due(...)`. A worker or external scheduler therefore
  retries them in a later cycle instead of counting a skipped publish as an
  index-converging maintenance pass;
- obsolete shared entries are retired by generation kind, so replacing a
  text-delta generation does not leak old resident mini-indexes or retire the
  current base incorrectly;
- a shared-preload smoke verifies first and second append visibility plus a
  mixed append/delete tail: new rows become searchable through shared text delta
  generations, tombstones become resident as shared negative generations, and
  the base remains resident with rebuild count unchanged;
- the same smoke now includes a small `field_aware = true` multicolumn index and
  verifies counter-only offload, shared text-delta visibility, shared tombstone
  filtering, and no extra foreground wakeup after the matching shared layers are
  resident. This keeps the original issue #18 field-weighted arXiv/pubmed query
  shape covered, not only the single-column token helper path;
- the same smoke then forces a full rebuild/fold by lowering
  `auto_rebuild_threshold`, verifies pending write/delete debt is cleared,
  appended rows are preserved, deleted rows are not resurrected, and old shared
  text-delta/tombstone entries are retired from the shared registry;
- the same shared-preload smoke also builds a small model-backed index,
  publishes a compact semantic delta generation, restarts the temporary
  PostgreSQL cluster to clear shared memory, verifies the durable generation is
  current but not resident, and proves
  `ii42_model_semantic_debt_maintain(...)` takes the
  `semantic_delta_publish_shared` path to republish it without rebuilding the
  generation row. A companion cold-index regression keeps the inverse case
  honest: a model-backed index that has not loaded its base generation into the
  shared holder can materialize a durable semantic delta, but that delta is not
  marked shared-resident publish-due and is not selected by maintain-due as
  shared-holder repair work;

The current shared delta implementation is intentionally narrow:

- append-only lexical/text deltas are supported;
- tombstone/delete deltas are supported as shared negative doc-id generations;
- semantic SAE deltas have a reusable materialized generation-table bridge, so
  maintenance can publish the SQL overlay once and foreground queries can reuse
  it instead of rebuilding a delta payload per backend/query. Shared-resident
  semantic repair is a hot-index operation: it only becomes maintain-due after
  the corresponding ii42 base generation is already shared-holder resident.
  Auto-preload candidate ordering follows the same rule: it does not compute
  semantic maintenance plans for cold model-backed indexes solely to sort the
  preload queue. The single-index auto-preload attempt follows the same guard:
  semantic delta publication is attempted only after the base is confirmed
  resident in shared memory, so cold preload attempts do not spend a worker
  cycle materializing a generation-table semantic payload that pooled backends
  cannot reuse through the holder. The mutable semantic bridge's explicit
  shared-publish helper is gated the same way, so operators cannot accidentally
  promote cold semantic deltas into shared memory through the product wrapper;
- compact `EATMH002` raw bytes can be resident in the shared arena and leased
  through a generation-table keyed identity. Normal parsed-cache attach remains
  available as the fallback, but `ii42_evidence_atom_query_by_id(...)` now has
  a direct offset-view path that traverses resident doc vectors, impact heads,
  and start arrays without rebuilding backend-local arrays;
- generic generation-table resident bytes now have a conservative pressure
  policy: when a new semantic/evidence blob cannot be published because the
  shared arena is full, the publisher may evict unreferenced generic blobs from
  any generation table in the current database and retry. This does not evict
  BM25 base/text-delta/tombstone resident entries, and it does not affect
  durability because generation-table rows remain the source of truth. A 1 MiB
  shared-holder smoke verifies that a newer compact evidence generation can
  replace an older unreferenced generic blob from another generation table and
  still answer directly from resident bytes;
- the pressure policy is asymmetric in favor of hot relation-backed indexes:
  BM25 base/text-delta/tombstone preload may also evict unreferenced generic
  model/evidence blobs when the shared arena would otherwise reject the
  relation-owned generation. This keeps issue #18 workloads resident in the
  shared holder instead of letting optional model blob cache entries force
  backend-local BM25 materialization. The 1 MiB smoke verifies this by
  publishing a large generic evidence blob first, then preloading a BM25 index
  and checking that BM25 becomes resident while the generic blob is evicted;
- `ii42_generation_cache_state(...)` and
  `ii42_generation_cache_state_json(...)` expose generic shared-resident blob
  pressure separately from relation-backed resident entries:
  `generic_blob_entries`, `generic_blob_ready_entries`,
  `generic_blob_refcounted_entries`, `generic_blob_bytes`, and
  `generic_blob_unref_bytes`. This gives operators a direct signal when model
  blobs are consuming shared arena capacity that hot BM25 generations may need;
- full fold/compaction is verified through the existing base rebuild path;
- standard, compact, and spill rebuilds all keep the shared-holder contract:
  after a full fold publishes a clean base generation, maintenance attempts to
  publish that rebuilt base into the shared holder immediately. The compact and
  spill paths reuse the serialized replacement bytes already produced by the
  builder instead of forcing the next pooled backend to read and parse the same
  generation from disk. The A210 smoke forces a low-memory spill rebuild and
  requires `shared_preload_published=true` plus a resident base afterward;
- lightweight multi-delta fold without scanning the heap is implemented for the
  first safe slice: payload-complete durable lexical/tombstone deltas. Current
  A210 convergence can publish shared text/tombstone deltas, marks query-covered
  fold pressure as a machine-readable lightweight fold candidate, and folds
  eligible payload-complete debt into a clean base generation through the
  existing tail-generation publisher. Counter-only debt, stale/corrupt payloads,
  and incomplete delta states still fall back to the heap-backed online rebuild
  path. This is deliberate: counter-only debt can represent either an
  unchanged-key update or an oversized inserted value that has no delta payload,
  so clearing it without a heap scan would be unsafe. Native semantic payload
  pages are also covered by a lifecycle regression here: a payload-complete
  lexical/tombstone lightweight fold must preserve any existing native semantic
  segment, keep it structurally valid, and leave it loadable after the clean base
  generation is published. The model-backed smoke extends that from raw payload
  preservation to the actual native-only semantic query path: when the
  generation-table semantic-delta row has been removed, the fold still preserves
  `native_semantic_delta_pages` as the current foreground semantic source.
  Native semantic repair/clear debt now blocks lightweight-fold readiness in the
  aggregate maintenance status, so repair of the semantic mirror stays ahead of
  lexical fold convergence. The maintenance and shared-holder readiness surfaces
  now report `native_semantic_repair_or_clear` as the active strategy during that
  state instead of implying a full rebuild fallback or fold attempt. The same
  surfaces now expose `lightweight_multi_delta_fold_payload_state`, including
  pending totals, durable delta-record counts, payload completeness,
  counter-only debt, incomplete payload state, and whether lexical rebuild is
  due. Counter-only and incomplete-payload states report
  `current_fold_strategy = full_rebuild_fallback` plus
  `next_implementation_step = run_heap_backed_online_rebuild`, making the
  safe fallback visible instead of looking like no fold work exists. A210 also
  treats native semantic payload, debt-log, and tombstone metapage fields as part
  of the online publish concurrency contract: heap-backed online swap and
  compatible lexical-tail carry are no longer allowed to publish across native
  side-page drift. The fold result still reports the preservation diagnostics,
  but they are no longer merely advisory: if a fold would return
  `native_semantic_preserved=false`, `native_semantic_debt_log_preserved=false`,
  or `native_semantic_tombstone_preserved=false`, the helper raises an error
  instead of continuing with a partially broken lifecycle. Preservation is now
  checked against native start block, data pages, generation, record count, and
  byte length, so a same-size side-page rewrite cannot slip through the
  lightweight fold lifecycle as a false positive. The native page smoke now also
  forces a semantic-tail-first layout, grows lexical delta
  records behind that tail until the delta spans multiple pages, then folds those
  records while preserving the native semantic payload. The same smoke rewrites
  the native semantic payload while those lexical delta records are still
  pending, proving that native mirror repair does not disturb lexical debt before
  fold convergence.
- A200g native semantic payload pages are durable index-AM storage primitives
  and a fallback/non-hot mutable semantic query source. They currently prove
  page layout, metapage lifecycle, normal restart persistence, immediate-stop
  store/shrink/clear crash recovery, `VACUUM (INDEX_CLEANUP ON)` preservation,
  full-rebuild clearing, `REINDEX INDEX` clearing, A210 semantic-publish
  mirroring of compact EATMH002 delta bytes into native pages, native-payload
  query fallback, and non-blocking native tail reuse/truncation after shrink or
  clear. Published native pages now contain a self-describing bundle with
  payload bytes and currentness metadata, so a
  fallback query can verify the native bundle against the current overlay even
  when the generation-table row is missing. The hot shared-preload default still
  prefers shared-holder offload/repair over backend-local native parsing until
  policy explicitly promotes native pages as the default semantic source. A
  native-only current bundle is the exception: if the generation-table
  materialization is absent, stale, or metadata-current but payload-invalid while
  the native bundle can prove currentness, the hot path now uses native pages by
  default instead of dropping semantic delta recall while waiting for an
  impossible shared-resident publish. That state is also not marked as
  `shared_resident_publish_due`, so maintenance does not repeatedly attempt to
  publish a missing or invalid generation-table row. Native mirror repair/clear
  now participates in the same priority comparator as shared-resident repair,
  lexical/tombstone delta publication, and lightweight fold pressure, so broken
  or orphaned native semantic mirrors converge before generic debt-heavy
  rebuilds. The native mirror health check is four-layer:
  page/metapage validation proves the index-AM segment is readable, bundle
  metadata decoding proves the mirror can prove currentness, the SHA-256
  payload checksum proves the bytes are the published payload, and EATMH parser
  validation proves the embedded semantic generation is query-usable. A
  malformed bundle, checksum mismatch, or invalid embedded payload keeps
  generation-table materialization authoritative and becomes repairable native
  mirror debt instead of a status/query exception. The aggregate mutable
  maintenance status mirrors these page/bundle/checksum/payload fields under
  `native_semantic_lifecycle`, so the scheduler-facing lifecycle view no longer
  has to infer semantic mirror readiness from structural page validity alone.
  The shared-preload lifecycle smoke now covers the three repair modes separately:
  orphan native pages are cleared after the semantic overlay retires, missing
  native pages are republished from a current generation-table semantic delta,
  and malformed native bundles are detected as invalid mirror debt and repaired
  by the same semantic publish path. Aggregate maintenance status now mirrors
  native semantic source/policy fields such as query source, native-query use,
  shared-resident publish due, foreground offload, and generation-table/native
  currentness, so the lifecycle view can distinguish native-only currentness
  from normal shared-resident semantic repair. The retired-generation
  compaction finalizer now also participates in this lifecycle: after it clears
  retired SQL overlay/debt rows, it asks the existing semantic-delta
  publish/delete path to clear an orphan native semantic tail when the current
  active generation has no semantic overlay and resynchronizes the native
  semantic debt counter mirror after retired debt rows disappear. This prevents
  a completed generation handoff from leaving stale native semantic pages or
  stale native debt counters waiting for a later generic maintenance tick.
  Direct generation deletion now uses the same lifecycle boundary:
  `ii42_model_generation_delete(...)` removes the deterministic semantic-delta
  generation, clears native semantic tail pages, and resynchronizes native debt
  counters before deleting the base generation, so manual/operator generation
  cleanup cannot leave stale semantic materialization behind. Base generation
  replacement is also covered: `ii42_model_generation_upsert(...)` invalidates
  the previous semantic-delta generation, clears native semantic tail pages, and
  resynchronizes native debt counters after a new base payload is published,
  which keeps all file/table/job generation builders from accidentally reusing
  semantic materialization from the replaced base.
  `ii42_model_semantic_debt_maintain(...)` now mirrors the same convergence in
  its top-level result: it reports whether native semantic page publication was
  attempted, whether the native tail is present or cleared afterward, and which
  materialization source was used. Workers and operators therefore no longer
  have to parse nested semantic-publish JSON just to decide whether debt
  processing advanced the native mirror lifecycle. Manual
  `ii42_generation_cache_preload(...)` and auto-preload workers now also prewarm
  structurally valid native semantic payload pages alongside the hot base and
  lexical/tombstone delta layers.
- A200g native semantic debt log pages are now a second durable index-AM
  semantic storage primitive. They use their own page kind and metapage segment,
  expose `ii42_index_native_semantic_debt_log_status/validate/store/load/clear`,
  preserve stored bytes across normal and immediate-stop restart, coexist with
  native semantic payload pages and lexical/tombstone delta tails, survive
  semantic-payload clear, remain valid through payload-complete lightweight
  lexical/tombstone folds and semantic-tail relocation, and are cleared by
  refresh/reindex lifecycle. The C storage primitive remains opaque, but
  `ii42_model_semantic_debt_sync_native_counters(...)` now writes a deterministic
  SQL-ledger snapshot into those pages whenever pending semantic debt changes,
  and clears it when pending debt reaches zero. SQL overlay/debt rows are still
  authoritative for worker selection and semantic debt maintenance; the native
  debt log now follows the real ledger lifecycle without claiming direct
  index-AM semantic ingestion is already attached. Manual preload, auto-preload,
  and lightweight folds now also validate/prewarm structurally valid native
  debt-log pages. Lightweight folds also refuse structurally invalid native
  debt-log tails and report whether the debt-log segment was preserved, so
  semantic payload pages and maintenance-facing debt-log pages share the same
  native tail lifecycle hooks.
  `ii42_model_semantic_debt_native_log_snapshot(...)` and
  `ii42_model_semantic_debt_native_log_consistency(...)` expose the exact
  SQL-ledger snapshot contract and verify that native debt-log bytes match it.
  `ii42_model_semantic_native_ingestion_plan(...)` now exposes the next-step
  readiness boundary in one machine-readable surface: whether the native debt-log
  record stream is present and consistent, whether the current worker route can
  consume it, whether SQL ledger rows are still authoritative, whether the
  native storage/fold lifecycle is complete, and whether direct native semantic
  posting ingestion is attached. A210 now also exposes
  `ii42_model_semantic_native_ingestion_apply_empty_tail(...)`, a deliberately
  narrow direct apply helper for the safe empty-tail encode-only case. It
  refuses partial batches, delete/replace records, non-empty native semantic
  tails, and non-empty SQL overlay deltas, then stores a compact native semantic
  payload and doc map directly in index pages. The ingestion plan, batch plan,
  and payload plan now expose this as a separate `empty_tail` capability with
  its own ready/blocker fields. Native-route semantic debt maintenance now
  tries this safe empty-tail direct apply before falling back to the SQL overlay
  plus native-publish path, so the capability is part of the normal
  worker/manual lifecycle rather than a standalone debug helper. General
  append/merge lifecycle apply is now attached for bounded native batches, and the
  native-tail fold path can compact a converged native semantic tail back into
  the active base generation. `ii42_evidence_atom_dump_docs(...)` exposes the
  parsed EATMH001/EATMH002 document vectors as SQL arrays, and
  `ii42_evidence_atom_dump_docs_with_ids(...)` exposes active-base document ids
  so fold planning can preserve identity without scanning the heap.
  `ii42_model_semantic_native_ingestion_merge_payload_plan(...)` now uses that
  primitive to build a merged EATMH002 payload from an existing native semantic
  tail plus bounded encode/delete/replace debt. SQL still performs the
  existing-tail/tombstone visibility staging, but final merged doc-map
  construction, impact-head rebuild, and compact EATMH002 serialization now go
  through the same C native-ingestion builder used by empty-tail apply. Delete
  and replace records produce tombstones keyed by doc key, heap TID, and base
  doc ordinal.
  `ii42_model_semantic_native_ingestion_apply_merge(...)` stores the rebuilt
  payload and first-class tombstone segment through one C native tail-store
  primitive, then resolves the consumed SQL debt rows. This keeps semantic
  delta and tombstone metapage publication under one maintenance lock and one
  status surface. Delete-all batches now publish a valid empty compact EATMH002
  semantic bundle plus native tombstone pages instead of forcing a fallback or
  a separate native clear; a later encode can append/merge from that
  tombstone-only empty tail. The empty-tail payload planner/apply path now uses
  the C
  native-ingestion builder directly after encoding, while the merge payload
  builder uses SQL for visibility staging and the same C builder for final
  compact payload/doc-map construction. Both paths avoid PL/pgSQL temp tables;
  remaining work is migrating append/merge visibility and final direct
  index-AM posting ingestion into C.
  Foreground native mutable queries load those tombstones into the same skip
  table used by SQL overlay tombstones, so deleted or replaced base identities
  are suppressed before final ranking. The native-route
  `ii42_model_semantic_debt_maintain(...)`
  path now tries safe
  empty-tail apply first and then tombstone-aware append/merge apply before
  falling back to the SQL overlay plus native-publish bridge.
  The public semantic maintenance wrapper now takes the same per-index
  maintenance lock as the C worker before it enters the SQL/native lifecycle.
  Manual calls and external schedulers therefore deduplicate with background
  workers instead of racing direct native apply, append/merge, or tail-fold
  publication for the same index.
  `ii42_model_semantic_native_tail_fold_plan(...)` and
  `ii42_model_semantic_native_tail_fold(...)` add the matching heap-free fold
  step: they refuse pending/failed semantic debt, validate native payload and
  tombstone pages, suppress replaced base document ids, build a new EATMH001
  active generation with preserved document ids, publish it through the normal
  model generation table, delete consumed overlay/debt rows, clear native
  semantic/tombstone/debt-log segments, and resynchronize native counters.
  Both plan and apply now emit `A210.native_tail_fold_manifest.v1`, a stable
  input/output manifest over the base generation hash, native tail bundle/payload
  hash, native doc-map/tombstone hashes, debt-log snapshot hash, row counts, and
  folded payload hash. The manifest is also stored in the folded active
  generation metadata. Fold payload construction has moved off PL/pgSQL temp
  tables onto a deterministic SQL CTE plus C EATMH001 generation-builder bridge;
  the manifest is the parity contract for moving the remaining base/native/
  tombstone merge into index-AM C without changing fold semantics.
  Shared-resident EATMH001 folded active bases can now serve by-id evidence
  queries directly from resident bytes, so doc-id-preserving folds no longer
  force backend-local cache parsing just to expose document ids.
- the built-in C worker candidate comparator now carries the same shared-holder
  convergence signals as the SQL due helper: semantic shared-resident repair,
  lexical delta publish due, tombstone delta publish due, and covered-delta
  fold due. This keeps the internal timer/touch worker from treating cheap
  shared-delta publication as generic stale/debt work and makes hot resident
  generations useful before falling back to full catch-up;
- uncovered hot shared-holder debt is keyed by actual base residency, not by
  `auto_preload`. A manually warmed or query-warmed `auto_preload = 0` index is
  still due for worker convergence when its lexical/tombstone debt is not
  covered by a matching shared delta generation.
- field-aware shared-holder regression now covers both counter-only and real
  delta tails before shared publication. A foreground weighted field query must
  record a maintenance offload and must not increment backend-local delta build
  counters while the base generation is resident in the shared holder. This
  directly guards the arXiv/pubmed shape from issue #18: hot multicolumn
  searches should stay agile and let maintenance publish reusable shared tails
  instead of materializing per-backend field-aware mini-indexes.
- `int4[]` id/atom indexes now use the same A210 shared lexical delta
  mechanics. The internal `TEXT_DELTA` shared generation kind remains the
  serialized lexical mini-index container, but it can now be built from either
  text tokens or already-tokenized integer ids. `ii42_query_ids(...)` merges a
  current resident id-delta mini-index into base results and otherwise records a
  shared-holder offload/wakeup instead of building a backend-local overlay for a
  hot shared-resident base. This keeps the product SAE atom path aligned with
  the BM25 text path while native semantic pages remain future work;
- global maintenance scans only enter semantic planning for indexes that have
  the semantic bridge reloptions configured. Pure BM25, multicolumn field-aware,
  and id/atom lexical indexes still participate in shared-holder lexical delta,
  tombstone, and fold scheduling, but they no longer call
  `ii42_model_semantic_maintenance_plan(...)` or semantic debt maintenance just
  because the global due helper is ranking all ii42 indexes. This keeps the
  touch/timer path cheap for the common lexical case while preserving semantic
  priority for model-backed indexes. The C semantic-debt maintain helper has
  the same reloptions guard at its own entry, and the C try-maintain semantic
  helper now applies that guard before `SPI_connect()` too. Direct helper calls,
  try-maintain, and future fallback paths all keep pure lexical indexes out of
  the SQL semantic bridge;
- operator status follows the same split. `ii42_model_mutable_maintenance_status`
  returns a lightweight `semantic_bridge_unconfigured` semantic plan for pure
  lexical indexes while still exposing lexical shared-holder readiness, pending
  worker launches, covered-delta state, and fold pressure. This avoids making
  dashboards or external schedulers more expensive than the C worker and SQL
  due-helper paths they observe. The same status path now skips full model
  deployment/readiness checks when neither model nor generation reloptions are
  configured, so BM25-only indexes do not parse runtime/scoring/model checkout
  state merely to report shared-holder lexical debt. For model-backed indexes,
  the same status path now folds native semantic debt-log snapshot consistency
  into the semantic debt object and marks
  `native_semantic_debt_log_repair_due` as a fold blocker/repair strategy when
  the native log no longer matches the authoritative SQL ledger;

The native shared-resident semantic payload work does not reuse the current BM25
shared-preload entry identity by overloading meta fields. That arena is keyed by
`Relation + relfilenode/locator + ii42 metapage identity` and is correct for
BM25 base/text-delta/tombstone generations. SAE generation-table payloads are
keyed by `(generation_table_oid, generation_id, revision)` and may not have a
one-to-one physical relation identity. A210 now adds a generation-keyed shared
resident raw-byte API with:

- fixed-width `EATMH002` payload validation before publication;
- key fields `database_oid`, `generation_table_oid`, `generation_id_hash`,
  `revision_hash`, payload kind, and serialized byte length;
- an attach API returning a leased resident byte view, not raw backend
  pointers;
- refcounted leases and stale retirement equivalent to BM25 shared-preload
  entries;
- proactive retirement on generation-table upsert/delete, so a durable row
  update or removal does not leave unreachable old revision bytes occupying
  the shared arena;
- fallback to backend-local parsed cache when shared memory is unavailable or
  the resident generation is stale;
- explicit generation-table publish into the shared holder through
  `ii42_sae_generation_publish_shared(...)`;
- explicit holder availability through
  `ii42_sae_shared_resident_available()`;
- explicit generation-table shared residency probe through
  `ii42_sae_generation_shared_resident(...)`;
- experimental UBMX `block_max` by-id queries now try the same generation-table
  shared-resident attach before reading the durable row, so unified BM25+SAE
  research payloads follow the A210 holder contract instead of always pulling a
  backend-local bytea copy from SQL storage;
- maintenance-level resident-miss repair through
  `ii42_model_mutable_semantic_delta_publish_shared(...)`, gated to indexes
  whose base generation is already shared-resident;
- direct `EATMH002` by-id query traversal over the leased resident byte view,
  using safe little-endian accessors instead of unaligned struct casts.

Native semantic delta pages are now stored in the index itself, so model-backed
foreground queries can compose base + semantic-delta native state without
depending on the SQL generation-table bridge as the query source. Semantic debt
counters are also mirrored into the metapage and resynchronized during
record/resolve/maintain plus generation upsert/delete/finalize lifecycle
operations, so debt pressure has a native index-AM status surface while the SQL
ledger remains authoritative. The native lifecycle now also has a fold-back
step: once native semantic debt has converged, the native tail and tombstone
segments can be folded into the active base generation and retired. The main
remaining A210 engineering gap is therefore narrower than before: move direct
semantic query-time encoding/failure accounting and final posting
storage into a native C/index-AM ingestion path, while preserving the
SQL-visible contract, worker scheduling, and maintenance-time failure
accounting that already converged.

A200g/A210 have started this transition with native payload pages, native debt
counter mirrors, native debt-log pages, native tombstone pages, tail lifecycle
maintenance, C append/merge builders, delete-all empty compact tails, and
heap-free native-tail fold into the active base generation. The remaining native
work is narrower now: consume the validated native debt-log record stream inside
the index-AM semantic posting ingestion path without routing general record
query-time encoder invocation/failure accounting and final posting storage
through PL/pgSQL.
`ii42_model_semantic_debt_native_log_records(...)` is the current SQL-visible
staging contract for that first step: it expands the native page payload only
after proving it still matches the authoritative SQL ledger.
The native-only semantic maintenance wrapper now uses that validated record
stream to choose pending rows and provide encode text, while still locking and
resolving the matching SQL ledger row. This moves worker input selection and
encode input onto native pages without claiming direct native semantic posting
ingestion is authoritative yet.
The append/merge and tail-fold SQL surfaces now also return
`native_execution_profile`, a stable machine-readable profile for rows, bytes,
append/merge payload builder route, tail-fold payload builder route, target, and the
next C/index-AM migration step. That profile is the observability contract for
replacing the remaining SQL-led storage loops without changing the surrounding
lifecycle API.
`ii42_index_native_semantic_tail_status(...)` now provides the matching C-level
read-only lifecycle status for all native semantic side segments in one call:
semantic delta, semantic debt-log, and semantic tombstone present/valid/reason
state plus aggregate pages, bytes, and records. The semantic delta status JSON
includes this line, so worker/operator diagnostics no longer need to stitch
three single-segment C status calls together while the direct C posting
ingestion path is still being migrated.
`ii42_index_native_semantic_sidecar_state(...)` now exposes the same native
sidecar metapage and validator state as structured JSONB directly from C. The
native debt-log consistency helper consumes this C JSON state instead of going
through the broad `ii42_generation_cache_state_json(...)` regex surface, so the
remaining direct-ingestion path has a smaller, typed status contract for native
delta/debt-log/tombstone presence, validity, page counts, bytes, and record
counts.
`ii42_index_native_semantic_debt_log_payload_state(...)` adds the matching
C-level payload identity contract for the debt-log segment. It validates and
loads the native debt-log pages, checks the metapage did not change during the
read, returns payload bytes/checksum, and embeds the loaded snapshot as JSONB.
`ii42_model_semantic_debt_native_log_consistency(...)` now consumes this C
payload-state helper rather than issuing a separate SQL bytea load. This still
keeps the SQL ledger authoritative, but it removes another SQL-led payload
read/parse boundary before the eventual direct C/index-AM semantic posting
ingestion path.
`ii42_index_native_semantic_tail_payload_load(...)` now adds the matching
fold-input read surface. It loads native semantic delta and tombstone payloads
as one `II42NST1` C bundle, verifies that the ii42 metapage is unchanged across
the read, and lets native tail-fold plan/apply consume one bundle instead of
issuing separate delta and tombstone loads. The native debt-log remains outside
that bundle by design: it is a fold consistency gate, not fold payload input.
Native tail-fold manifests and execution profiles now include the bundle bytes
and checksum as the fold-input identity, plus the split delta/tombstone payload
hashes. This is the no-regression contract for moving the remaining fold merge
implementation into C without changing the surrounding SQL lifecycle surface.
`ii42_evidence_atom_profile(...)` now adds a C-parser profile for the
base/native EATMH payload inputs. Tail-fold plan/apply record the base
generation profile, native compact-tail profile, and the
`c_evidence_atom_profile` route in the fold manifest and execution profile.
`ii42_evidence_atom_fold_generation(...)` now moves the fold merge/build loop
itself into C: SQL projects native doc-map/tombstone JSONB into typed arrays,
then C performs base suppression, native replacement precedence, final doc-key
ordering, impact-head rebuild, and EATMH001 payload encoding. This leaves SQL
as the lifecycle and JSONB projection layer while removing the previous SQL CTE
fold merge as the active path.
Base-generation replacement and native tail-fold cleanup now use the same C
native-tail clear primitive to drop semantic delta, debt-log, and tombstone
segments under one maintenance lock. That closes the cleanup side of the native
lifecycle; the remaining C migration is direct native semantic posting
ingestion from the validated native debt-log stream.
Native tail-fold planning and apply also gate on the deterministic native
debt-log snapshot consistency check. A fold is not allowed just because SQL
semantic debt counters are zero; stale or inconsistent native debt-log pages now
block the fold until the normal native counter/log sync path clears or repairs
them. This keeps the semantic delta, tombstone, debt-log, and active-base
generation lifecycle as one multi-delta contract.
`ii42_model_semantic_native_ingestion_batch(index, max_rows)` now packages the
same validated stream into a deterministic bounded batch: sorted records, row
counts, operation counts, input-byte totals, input-completeness checks, current
worker source, and a non-mutating data-plane contract. Direct storage is handled
by the stricter empty-tail apply helper, not by the batch-plan probe itself.
This keeps the future C ingestion path stable while preserving the current
SQL-ledger authority boundary for general append/merge cases.
The batch plan now also emits
`A210.native_ingestion_batch_manifest.v1`, a stable manifest over row ordinals,
doc keys, operations, input hashes, byte totals, and native debt-log record
counts. The manifest is hashed separately from the full debug records so a
future C/index-AM posting-ingestion path can verify that it is consuming the
same native debt-log snapshot as the SQL control plane without depending on
volatile timestamps or SQL staging state.
`ii42_model_semantic_native_posting_writer_plan(...)` is the current final
writer gate on top of that manifest. It keeps the current C/SPI orchestration
visible as `c_spi_native_posting_writer_over_c_primitives`, records the target
`single_c_index_am_posting_writer`, exposes native record stream authority,
current worker input source, lifecycle route, and posting materialization target,
and embeds the writer plan in aggregate mutable maintenance status so
scheduler/operator code can make one decision without traversing batch, payload,
handoff, and query-encoder plans separately.
The matching mutating entrypoint,
`ii42_model_semantic_native_posting_writer_apply(...)`, already exercises that
single-call C/SPI route, takes the per-index maintenance lock in the C wrapper,
delegates to the lock-owned writer bridge used by background workers, returns
the same before/after plan contract, and now also reports SQL-ledger authority
scope, the C/SPI preflight resolver, native stream authority, consumed batch
manifest hash, and actual semantic posting materialization route at the top
level while keeping
`single_call_c_index_am_posting_writer_attached=false`.
The lock-owned bridge now tries the encoded worker data-plane first: it encodes
the selected debt-log batch into `A210.native_ingestion_encoded_records.v1`,
attempts empty-tail or append/merge native page apply under the already-held
maintenance lock, and only falls back to the older internal debt-maintain loop
when the encoded path is blocked or fails. This narrows the remaining mutable
index gap to the final C/index-AM writer without changing the current SQL-ledger
authority boundary.
That readiness is now visible before mutation. The writer plan and aggregate
maintenance status expose `encoded_native_data_plane_apply_ready`, the exact
blocker, and `lock_owned_encoded_data_plane_first_attached` separately from the
final `writer_ready` flag, so scheduling can prefer the encoded native-page path
without over-claiming that final C/index-AM posting ingestion is attached.
The native ingestion plan, bounded batch, authority-handoff plan, and
posting-writer readiness surfaces now also carry the C/SPI query-time encoder
seams:
`c_spi_query_time_encoder_invocation_attached=true` means product text-query
helpers route normal query text encoding through `ii42_model_encode_text_c(...)`,
and `c_spi_query_time_encoder_failure_accounting_attached=true` means status
probes use the C/SPI non-mutating failure helper. Both still delegate to the
existing SQL/runtime encoder, so they are intentionally distinct from the final
embedded index-AM encoder flags, which remain false until query text encoding is
owned fully by the index-AM/worker boundary.
The aggregate mutable maintenance status now mirrors these replaceable seams at
the scheduler-facing level. It reports the C/SPI query failure-accounting
entrypoint, the current C/SPI posting writer, the lock-owned worker writer, and
the lock-owned encoded writer directly under both `segments.delta_segment` and
`maintenance` where appropriate. This keeps issue #18-style scheduling decisions
on one summary surface while still preserving the nested plans as the detailed
source of truth.
The encoded native-page apply decision is now isolated behind
`ii42_model_semantic_native_posting_writer_apply_from_encoded_locked(...)`.
The lock-owned worker/manual writer bridge delegates to that helper after
encoding the selected batch, and the helper chooses empty-tail or append/merge
native page apply without taking or releasing the per-index maintenance lock.
This makes the current pre-encoded SQL/C-primitive writer a single replaceable
seam for the future C/index-AM writer.
The public pre-encoded writer wrapper now uses the same seam in strict mode:
`ii42_model_semantic_native_posting_writer_apply_from_encoded(...)` still owns
the public C maintenance-lock acquire/release and strict bad-input ERROR
semantics, but it delegates the actual encoded native-page mutation to
`ii42_model_semantic_native_posting_writer_apply_from_encoded_locked(...,
raise_on_error=true)`. Worker/manual paths keep the non-strict wrapper so they
can fall back to internal debt maintenance without changing the data-plane
contract.
The public native maintenance route now uses the same lock-owned writer seam.
When `native_semantic_delta_maintenance=true`,
`ii42_model_semantic_debt_maintain(...)` and the SQL
`ii42_index_maintain_due(...)` helper call
`ii42_model_semantic_native_posting_writer_apply_locked(...)` after taking the
per-index maintenance lock, rather than calling the older internal native debt
loop directly. `ii42_model_semantic_debt_maintain_native(...)` follows the same
path as an explicit operator/testing override. The wrapper flattens
`maintain_result` back into the existing `semantic_debt_maintain` JSON shape,
then adds writer diagnostics for contract version, current orchestration,
target orchestration, and encoded data-plane attempt/use. The lock-owned writer
also repairs stale native semantic debt-log snapshots before batch selection;
if that repair leaves no pending rows, the result still reports a maintained
repair-only convergence instead of a misleading `no_pending` no-op.
The public posting-writer wrappers now also validate native page state after the
mutation step. Ordinary and pre-encoded writer calls attach a
`c_wrapper_native_page_validation` object sourced from
`ii42_generation_cache_state_json(...)`, carrying post-apply native semantic
delta, native debt-log, and native tombstone present/valid state. This makes the
public seam prove that native side-page lifecycle state is structurally readable
after apply, while still keeping `single_call_c_index_am_posting_writer_attached`
false until the final index-AM writer body replaces C/SPI orchestration.
The same public and lock-owned writer result contracts now expose the final
writer gap directly through `final_c_index_am_posting_writer_attached=false`,
`final_c_index_am_posting_writer_blocker=single_call_c_index_am_posting_writer_not_attached`,
`writer_contract_gap=final_c_index_am_posting_writer_not_attached`, and
`final_writer_migration_target=single_c_index_am_posting_writer`. This prevents
operators, tests, and future agents from treating the C/SPI bridge as the final
index-AM writer while still allowing the bridge to maintain native side pages,
semantic debt, tombstones, and fold state safely.
`ii42_model_mutable_maintenance_status(...)` also mirrors the same gap into the
delta-segment and maintenance aggregate status, so scheduler/operator code can
read one status document without separately expanding the raw writer plan.
The query encoder status uses the same explicit boundary shape: product text
queries name `shared_runtime_service` as the model holder, report that
backend-local model sessions are not allowed for product queries, and keep the
embedded index-AM query encoder gap visible until that seam is actually moved.
The lock-owned worker/manual posting-writer seam now emits the equivalent
`lock_owned_native_page_validation` object after mutation. This matters because
background workers, `ii42_index_maintain_due(...)`, and direct
`ii42_index_try_maintain(...)` priority semantic maintenance can call
`ii42_model_semantic_native_posting_writer_apply_locked(...)` without going
through the public C wrapper. Those paths now prove that native semantic delta,
native debt-log, and native tombstone pages are readable and valid at the seam
where the final C/index-AM writer will attach.
`ii42_model_semantic_native_ingestion_encoded_handoff_profile(...)` now moves
the pre-encoded worker handoff validation into C as well. Empty-tail and
append/merge page-apply builders both consume that single profile for
expected/encoded/delete/replace/invalid/unmatched/duplicate/missing row counts
before building EATMH002 payload arrays. This keeps the SQL error contract and
lifecycle orchestration intact while giving the future C/index-AM posting
writer the same encoded-record validation boundary as the current native-page
apply helpers.
`ii42_model_semantic_native_ingestion_encode_batch(...)` now exposes the
preceding SQL encoder output stream as
`A210.native_ingestion_encoder_batch.v1`. It takes the bounded native debt-log
batch, invokes the configured encoder, returns encoded rows plus encode-failure
diagnostics, and lets `ii42_model_semantic_native_ingestion_build_payload(...)`
enter the same `_from_encoded` payload builder used by worker-provided atoms.
This keeps SQL responsible for encoder invocation/failure accounting for now,
but makes the handoff into final C/index-AM posting storage a stable data-plane
contract instead of an implicit loop inside payload construction.
`ii42_index_native_semantic_debt_log_batch_state(...)` now owns the bounded
native debt-log record selection for that batch contract. It reads the native
debt-log payload from index pages, parses the snapshot in C, applies
`max_rows`, emits full records plus manifest records, and computes
operation/input statistics.
`ii42_model_semantic_native_ingestion_batch(...)` now consumes this C batch
state instead of re-aggregating `ii42_model_semantic_debt_native_log_records`
in PL/pgSQL. The mutating native-only maintenance fallback now consumes the same
C-selected batch state, so planning and mutation share the same bounded native
debt-log snapshot. It still locks/resolves the SQL ledger row and still invokes
the encoder through the SQL control plane until the direct C/index-AM ingestion
path owns encoder invocation, failure accounting, and final posting storage.
The fallback reloads that C batch state immediately before row locking/apply,
after any direct empty-tail, append/merge, or tail-fold attempts, so the
mutating SQL fallback does not consume a stale pre-attempt snapshot.
Row resolution is now guarded as well: semantic debt resolve can match optional
`expected_*` row identity fields, and native direct/fallback consumers pass the
operation, input hash, base doc ordinal, or exact debt id they consumed before
marking SQL debt encoded, cleared, or failed.
Direct native page apply now also performs a two-phase SQL-ledger preflight:
`ii42_model_semantic_native_ingestion_resolve_preflight(...)` first validates
every pending debt row named by the native debt-log batch without taking row
locks, then repeats the check with row locks immediately before empty-tail or
append/merge storage writes native semantic pages. A stale or concurrently
consumed batch therefore returns a blocker before modifying native semantic
storage, while expensive payload building does not hold debt-row locks and the
later guarded resolve remains the final row-accounting transition.
The row-match and optional `FOR UPDATE` part of that preflight now runs through
`ii42_model_semantic_native_ingestion_resolve_preflight_c(...)`, a C/SPI helper
that returns the same JSON contract plus a `c_semantic_debt_resolve_preflight`
resolver marker. The follow-up
`ii42_model_semantic_native_ingestion_resolve_batch_c(...)` helper now performs
the final batch `encoded`/`cleared` transition with the same row identity
contract and returns `resolver = c_semantic_debt_resolve_batch`. SQL still
extracts model options and remains the authority boundary, but direct native
page apply no longer performs row-by-row debt transition loops in PL/pgSQL.
The worker-facing single-row transition also delegates the guarded update and
native counter sync to `ii42_model_semantic_debt_resolve_c(...)`, so fallback
maintenance, failed encodes, and cleanup paths share the same C/SPI
row-accounting primitive. SQL still decides when encoder work failed or which
rows to consume; it no longer owns the state update itself.
The foreground touch path deliberately does not become a foreground semantic
maintainer. It records pending work and, inside a transaction, arms a
commit-time wakeup. Worker-side maintenance then reopens the index after
commit, reloads the current bounded native debt-log batch, and decides whether
semantic repair, publish, fold, or no-op is valid under the normal per-index
maintenance lock.
`ii42_model_semantic_native_ingestion_authority_handoff_plan(...)` now packages
the remaining debt-authority handoff as a non-mutating readiness gate. It
combines the native debt-log batch state with the C/SPI SQL-ledger preflight and
reports whether native selection, SQL preflight, and native page/fold lifecycle
preconditions are ready before final C/index-AM posting storage is attached.
When `native_semantic_delta_maintenance=true` and the native debt log is
consistent, the native debt-log batch state is now reported as the worker
selection authority. SQL ledger rows still provide resolve preflight, repair,
and row-lock authority, so the handoff is precise rather than a loose ownership
flip. The profile now distinguishes maintenance-time encoder failure accounting,
which is attached through the debt worker path, from query-time text encoding,
which is attached at the SQL product-helper boundary through
`ii42_model_query_text(...)`, `ii42_model_mutable_query_text(...)`, and
`ii42_model_try_encode_text(...)`. The normal encode path is also exposed through
`ii42_model_encode_text_c(...)`; the non-mutating failure boundary is exposed
through `ii42_model_try_encode_text_c(...)`, a stricter C/SPI wrapper that
validates the index/owner seam while preserving the same JSON failure-accounting
contract.
The handoff plan now mirrors both C/SPI query encoder flags and reports them as
blockers, so worker scheduling can distinguish "C/SPI query encoder seam is
attached" from "embedded index-AM query encoder is still pending" without
expanding the lower-level batch JSON.
The product query holder is explicitly the shared runtime service:
`ii42_model_query_encoder_status(...)` reports that runtime service use is
required, backend-local ONNX fallback is not attached, and any such fallback is
blocked by the `product_query_requires_shared_runtime_service` contract. The
A170/A210 smoke clears the backend-local ONNX cache before a product text query
against an ONNX-backed index without a runtime service and verifies the rejected
query does not create a backend-local model session.
The A172 runtime-service smoke also records ONNX-backed semantic encode debt,
runs semantic debt maintenance, and verifies that maintenance advances the
shared runtime-service success counter while the foreground backend-local ONNX
cache remains empty. The same runtime-enabled fixture flips the index to
`realtime` and proves semantic INSERT/UPDATE fail before queueing debt or
touching either the shared runtime service or a backend-local ONNX session. After
it records pending semantic debt, it also proves the configured mutable text
query fails on realtime freshness before query encoding can reach the shared
runtime. The native DML probe now repeats the fail-fast boundary for
`native_semantic_delta_maintenance=true`, proving native realtime INSERT/UPDATE
also fail without heap changes, semantic debt/native debt-log drift,
shared-runtime work, or backend-local ONNX. It then holds the native maintenance
lock around a real eventual INSERT, flips the same index to `realtime`, and
proves the configured text query fails on pending-debt freshness before query
encoding can reach the shared runtime. It then releases the lock and repeats the
same maintenance check through
`ii42_index_maintain_due(...)`, proving the automatic due-helper entrypoint also
resolves semantic debt through the shared runtime boundary. It also holds the
per-index maintenance lock from a second backend, proves direct semantic
maintenance returns `maintenance_lock_busy` without calling the shared runtime
or creating a backend-local ONNX session, then releases the lock and verifies
the same debt converges through the shared runtime service. It also repeats the
lock-busy scenario for the automatic path: `ii42_index_maintain_due(...)` emits
no progress while the lock is held, `ii42_index_touch_maintenance()` accepts the
non-blocking wakeup without encoding, and the same debt converges through the
background worker after the lock is released. The same smoke now records
another semantic debt row, commits a
`ii42_index_touch_maintenance()` wakeup, and waits for the background worker to
fold that debt through the shared runtime service. This covers the direct
maintenance encode path, the due-helper path, and the committed touch/worker
automatic maintenance path, not only the query path. It also performs real
eventual `INSERT`, indexed-text `UPDATE`, and `DELETE` plus `VACUUM` operations
against the model-backed table, verifies that non-indexed updates do not record
semantic debt, mutable overlay rows, or shared-runtime model calls, verifies
that `ii42_aminsert` records semantic encode debt and `ii42_ambulkdelete`
records semantic delete debt, and waits for the post-commit/post-VACUUM
background worker to materialize DML deltas and tombstones through the same
shared-runtime boundary. The probe also queries those maintained DML rows:
eventual INSERT and indexed UPDATE must be visible as delta hits, while the
post-VACUUM DELETE tombstone must suppress the updated doc from the same mutable
text query. This ties CRUD lifecycle coverage to SQL query semantics rather than
only debt and overlay counters. The same probe also inserts a model-backed row
inside an aborted transaction and verifies that the transactional semantic debt, heap
row, and mutable overlay residue all disappear after rollback without sending
that aborted row to the shared runtime service. It now also creates an
independent `native_semantic_delta_maintenance=true` model-backed index in the
same shared-runtime fixture, records semantic encode debt, and verifies that
both the explicit semantic-debt maintainer and `ii42_index_try_maintain(...)`
write native semantic pages directly, leave no generation-table semantic-delta
row or SQL overlay row, consume the native debt-log snapshot, increment the
shared runtime-service success counter, and keep the foreground backend-local
ONNX cache empty. It also runs mutable text queries after both native-page
maintenance entrypoints and verifies that the native delta docs are visible as
delta hits. The same runtime fixture now exercises real DML on that native index
too: eventual INSERT and indexed UPDATE are encoded by the background worker into
native pages and returned by mutable text query as native delta hits, while
DELETE plus VACUUM records index-AM delete debt, clears the current native delta,
and suppresses the updated doc from query results without generation-table or
SQL-overlay fallback. This closes the shared-holder proof for the native page
route as well as the SQL bridge route across the operator, automatic
maintenance, and real DML entrypoints. The embedded index-AM query encoder and
final C posting ingestion remain unattached, so future writer work has a precise
blocker surface instead of a loose TODO.
Aggregate mutable maintenance status embeds the same profile and mirrors the
current handoff readiness/blocker in both the delta segment and maintenance
sections. It defers full SQL-ledger preflight until a model generation exists,
reporting `model_generation_unavailable` for early indexes.
`ii42_model_semantic_native_ingestion_payload_plan(index, max_rows, head_size)`
then proves the next piece of that path without mutating the index: it encodes
the native batch directly, hands the encoded doc/atom rows to
`ii42_evidence_atom_build_compact_generation_from_docs(...)`, validates the
result, reports payload bytes/checksum, and emits the doc-ordinal map needed to
translate native delta hits back to `doc_key`/heap identity without a SQL
overlay. SQL still owns per-row encoder invocation and failure detection, but
the empty-tail path now performs doc-key deduplication, stable ordering,
impact-head construction, compact `EATMH002` serialization, and doc-map
construction in C. The guarded empty-tail native apply uses the same C builder,
so planning and application share one payload/doc-map contract. Append/merge now
projects existing native maps, tombstones, mutation records, and encoded rows
into typed arrays, then calls
`ii42_evidence_atom_merge_compact_generation(...)`; C owns tombstone precedence,
existing-tail suppression, new-row precedence, doc-key deduplication, final
payload serialization, merged doc-map emission, and merged tombstone emission.
The EATMH doc-vector dump helpers, merge payload plan, tombstone-aware merge
apply, and native-tail fold now close the safe
append/delete/replace/fold visibility case: SQL can reconstruct existing compact
semantic payload rows, combine them with a bounded debt batch deterministically,
persist delete/replace tombstones in native tombstone pages, store the rebuilt
compact payload in native semantic pages, represent delete-all tails as valid
empty compact semantic bundles, resolve that batch, fold the converged tail into
a doc-id-preserving active base generation, clear the retired native segments,
route worker/manual maintenance through that lifecycle, and query the folded
active generation from shared resident memory while preserving document ids.
Append/merge visibility reconstruction is now a C helper boundary fed by typed
arrays; the active tail-fold route also reports
`fold_visibility_merge_route = c_evidence_atom_fold_generation` and
`legacy_sql_cte_fold_builder_active = false`. The older SQL/CTE fold builder is
kept as a legacy parity helper, not as the active maintenance path. The
append/merge SQL wrapper now uses the same
`ii42_model_semantic_native_ingestion_encode_batch(...)` boundary as the
empty-tail writer, then passes the pre-encoded row stream into the C merge
builder. That removes the second row-by-row encoder loop from merge payload
construction and leaves one encoder contract for the future worker/index-AM
writer path. The writer readiness profile now reports
`encoder_batch_contract_attached=true` and includes that helper in
`current_c_primitives_attached`, so this dependency is visible to smoke tests
instead of being an implied SQL detail. The encoded writer entrypoint also owns
its per-index maintenance lock in the C wrapper now; SQL apply helpers only see
the lock state and the C `PG_TRY/PG_CATCH` cleanup releases the advisory lock on
normal return or ERROR. The lock-owned helper boundary now also verifies that
the current backend already holds that maintenance lock before running locked
posting-writer, pre-encoded posting-writer, or tail-fold code. The direct
native tail-fold mutator has the same guard, so SQL callers cannot compact
active base/native side pages without maintenance ownership. The lower-level
native ingestion apply helpers and semantic-debt internal maintain helper are
also lock-owned mutators now; plan/status functions remain the inspection
surface, while public maintain/writer wrappers or C workers own mutation. The
raw C native semantic payload/tail/tombstone page primitives now also acquire
the same per-index maintenance lock when called outside an existing maintenance
scope, so direct diagnostic page writes cannot race normal lifecycle
maintenance. The native storage smoke now holds that lock from a second backend
and proves direct delta, tail, and tombstone page mutators fail without changing
existing bytes. Native debt counter/debt-log mirror primitives remain separate
because they are part of DML debt bookkeeping as well as maintenance. Native
semantic page storage and lightweight multi-delta fold are therefore implemented
and regression-covered at the lifecycle level. Automatic
`ii42_index_try_maintain(...)` also proves the worker-facing native route can
converge shared-runtime semantic debt without backend-local model ownership. The
public native posting writer now also proves lock-busy no-progress semantics
from a competing backend before converging the same debt through append/merge
after the lock is released. The
next implementation slice is replacing the remaining C/SPI lock-owned SQL writer
bridge with direct native C posting ingestion, not basic tombstone visibility,
delete-all lifecycle correctness, doc-id correctness, fold lifecycle
correctness, append/merge visibility semantics, merge wrapper encoder contract
drift, shared-runtime native-page holder proof, or locked-helper
maintenance-lock cleanup.

## Non-Goals For A200

- Do not introduce an independent SAE scheduler.
- Do not force `eventual` as hidden default yet.
- Do not make realtime silently degrade to partial hybrid indexing.
- Do not claim dense-removal product readiness from maintenance work alone.
- Do not optimize model training here; this is an index maintenance design.

## Open Questions

- Should semantic pending docs be query-visible as BM25-only candidates under
  `eventual`, or should hybrid query explicitly mark them as semantically
  missing?
- What is the first acceptable semantic lag budget for production RAG
  workloads?
- Should base segment and delta segment share one physical relation from the
  first native implementation, or can A200b use a relation-owned sidecar while
  preserving the single scheduler/worker semantics?
- How much semantic encode batching can run inside PostgreSQL workers before an
  external controlled builder becomes operationally safer?
- Resolved for A210: hot shared-preload resident indexes should not build
  backend-local text or semantic delta materializations. They attach matching
  shared deltas when available, otherwise return a bounded eventual read and
  wake maintenance. Backend-local materialization remains allowed for
  standalone/debug paths and non-shared resident cache entries.
- Resolved for A210 scheduling: `auto_preload > 0` is a warmup and priority
  hint, not the eligibility condition for shared delta convergence. Any hot
  shared-resident base with uncovered debt can wake/enter maintenance. Proactive
  auto-preload may publish eligible shared deltas, but correctness and catch-up
  pressure are keyed to actual holder residency plus uncovered debt.
