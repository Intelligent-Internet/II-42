# P2.2 Unified Mutable Posting Design

Date: 2026-07-22
Status: final-v2 implementation and target-host qualification complete

This design supersedes the earlier counter-only semantic maintenance plan.
Historical native-semantic and fallback experiments remain archived under
`docs/research-sae/reports/ii42-mutable-hybrid-index-a200-a211-research-archive.md`.

## Product Invariant

P2 has one physical and logical index lifecycle:

```text
one ii42 relation
    -> one active unified base generation
    -> one relation-owned unified delta segment
    -> one tombstone stream inside that delta
    -> one manifest, generation, lock, scheduler, and compaction path
```

Lexical and semantic atoms are internal namespaces in one sorted posting map.
They are not independently visible indexes or independently maintained deltas.
With `sae=false`, the same lifecycle remains in force, but the compiler emits no
semantic atoms.

## Implemented Closure

The segmented relation and BM25 delta machinery already provide WAL-backed
delta pages, tombstones, tail handoff, cache invalidation, and generation
publication. P2.2 now stores a complete versioned lexical-plus-semantic atom
map in that existing delta for each `sae=true` write. Query, maintenance,
compaction, crash recovery, and physical replication consume the same
relation-owned records. The implementation adds no segment, sidecar, journal,
scheduler, generation, or query API.

## Unified Delta Record

The existing outer record remains authoritative:

```text
heap_tid + value_bytes_len + value_bytes
```

An invalid `heap_tid` remains a tombstone whose value is the old `doc_ord`.
For a valid `heap_tid` on `sae=true`, `value_bytes` is a versioned unified atom
payload:

```text
magic + format_version + flags + atom_count
repeated(atom_id:uint32, impact:float4)
```

Atoms must be sorted, unique, finite, positive, and within the generation atom
space. The payload contains the final lexical and semantic atoms produced by
the same P2 compiler. It is bounded by atom count rather than source-text size,
so long source text does not inherit the old one-page raw-text delta limit.

For `sae=false`, valid records retain the current exact lexical source payload.
The outer page format, tombstones, manifest counters, and generation ownership
are identical in both modes.

## Mutation State Machine

Each mutation follows one state machine:

```text
pending -> compiled -> visible -> compacted
```

- `pending`: the transaction owns source values, but no partial posting is
  query-visible.
- `compiled`: one complete lexical-plus-semantic atom map has been produced and
  appended to the relation delta.
- `visible`: PostgreSQL MVCC makes the referenced heap TID visible. Aborted or
  rolled-back records remain unreachable through heap visibility.
- `compacted`: one publication atomically replaces base, delta, tombstones, and
  global metadata with a new generation.

Realtime writes must compile before returning. A compiler failure fails the
write; it must never commit a lexical-only result. Eventual policy may continue
serving the previous base while a transaction is pending, but once the heap
mutation commits its delta record must already be complete. Eventual policy is
therefore a compaction policy, not permission to publish half an index.

## Exact Query Contract

Every model-backed query evaluates one logical surface:

```text
active unified base + visible unified delta - tombstones
```

Base and delta use the same query atoms and exact sparse dot product. The
executor filters heap-invisible TIDs, removes tombstoned ordinals, deduplicates
updated rows by visibility, and performs one global top-k merge. Delta rows use
stable ordinals after the active base for diagnostics only; physical TID and
MVCC determine result identity.

The result must match a full `REINDEX` for the same committed heap snapshot.
Document identity and rank are exact; score comparison permits absolute error
`1e-4` and relative error `1e-6` because foreground single-row ONNX execution
and bulk-build ONNX execution may differ at float32 accumulation precision.
Query must not invoke an external BM25 index, ANN index, fusion step, or
benchmark-only evaluator.

## Compaction And Concurrent Tail

Online maintenance builds a replacement from a heap snapshot while foreground
writers append to the same delta segment. At publication it reuses the existing
tail-handoff fence:

1. Rows already represented in the replacement are remapped or discarded.
2. Concurrent complete records and tombstones are copied as opaque, versioned
   records after the new base.
3. One metapage switch publishes the new base and carried tail together.
4. Retired pages are reclaimed only after the existing reader-safety fence.

There is no semantic side generation. Corrupt, incompatible, or unremappable
records fail closed and require repair rather than falling back to a split
lexical/semantic state.

## Full-Text Compiler

The model's `max_length` is a window limit, not a product input limit. The
compiler tokenizes the full text and creates deterministic overlapping windows
with BOS/EOS per window. Lexical atoms are compiled once from the full text.
Semantic window outputs are aggregated into one atom map before lexical and
semantic calibration is applied.

The first exact-compatible aggregation candidate is dimension-wise maximum.
Length-normalized sum and bounded top-chunk aggregation are research variants
and cannot become defaults without the native qrels gate. Inputs that fit one
window must remain byte-for-byte equivalent at the atom/weight interface.

Every result reports full token count and window count. A configured hard
safety limit produces an explicit error; silent truncation is forbidden. SQL
query remains an immediate single-text request. Internal window batching must
not wait for unrelated queries.

## Proof Gates

Implementation is accepted only after all of the following pass:

- randomized committed/aborted/savepoint CRUD traces against full `REINDEX`;
- exact top-k, score, TID, tombstone, and generation-signature parity;
- crash/restart, rollback, concurrent maintenance, and tail-handoff tests;
- one-window output parity and long-tail marker retrieval tests;
- staged-package PG18 maturity qualification;
- native held-out qrels evaluation and exact-query latency profiling.

Lossy pruning, quantization, candidate caps, and approximate bounds remain
isolated until this exact lifecycle is proven.

## P2.2 Qualification Status

The unified lifecycle and full-text compiler pass all 48 mutable lifecycle
gates, including randomized CRUD-to-`REINDEX` parity, rollback, restart,
long-tail retrieval, explicit oversized-input failure, and short/single/batch
runtime parity.

The final Enlightenment exact-query closure preserves rank, CTID, document
identity, and scores across each 1,200-row comparison. Deferring visibility,
prefetching the document-start table, and copying each candidate document's
contiguous atom/impact slice once reduce native full-query p50/p95 from
`95.90/112.91 ms` to `50.79/58.83 ms`, or `47.0%/47.9%`. Four independent
final-v2 windows pass the 20% gate with zero identity and score mismatch.

The pinned ONNX Runtime 1.26.0 package also passes the complete package-bound
maturity suite. Preflight and postflight fingerprints are identical, all staged
and installed artifacts match, and primary/standby physical replication
preserves ranked semantic results across build, update, maintenance, and
`REINDEX`.

This closes lifecycle, packaging, replication, full-text, rollback/reinstall,
and exact-latency gates. Full native SciFact, Arguana, SciDocs, and TREC-COVID
rows improve NDCG@10, MAP@100, Recall@100, MRR@20, and CUB over BM25 with one
frozen model. RLS-aware global ranking and globally ranked partition parents
remain fail-closed. The one-variable budget-0.75 lossy canary failed its
quality, storage, and latency gates; no lossy variant is part of final-v3.
