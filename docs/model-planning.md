# Model Planning

Status: active model follow-up plan subordinate to the
[Product Roadmap](product-roadmap.md), the current engineering planning authority.

Updated: 2026-09-03

This document owns the detailed follow-up work removed from the Beta 1 model
technical reports ([English](technical-report-ii42-model.md),
[Traditional Chinese](technical-report-ii42-model-zh.md)). The reports retain model
design, frozen evaluation results, limitations, and broad future directions.
This plan is neither a list of shipped improvements nor a new model release
contract. It does not authorize a model replacement, index rebuild, deployment,
or change to default approximation policy.

## Baseline and Scope

The current package binds P2.2 ABI-v2 through
[`packaging/milestone-model.json`](../packaging/milestone-model.json) and pins
ONNX Runtime through [`packaging/onnxruntime.version`](../packaging/onnxruntime.version).
The full BEIR15/MTEB10 matrices in the reports remain P2.1 / b1.125 evidence;
they must not be relabeled as a current-package rerun. NFCorpus-bound lexical
vocabulary and calibration limit transfer claims for the bundled checkout.

Existing full-text windows, compact postings, explicit impact quantization,
semantic accelerators, and background convergence are the starting point,
not features to reimplement from the old report's checklist. Before opening a
work item, compare it with the current
[storage design](convergent-segmented-index.md),
[query semantics](query-semantics.md), and
[validation workflow](testing-and-validation.md).

## 1. Bind the Evidence Before Further Optimization

- Bind each comparison to source commit, package fingerprint, model manifest,
  tokenizer, calibration, dataset/qrels identity, index options, and query
  shape. Record exact and approximate serving modes separately.
- Complete the dense baseline's PPLX revision, embedding SHA-256, and
  VectorChord build/index manifest. Preserve provenance for the 13 reused
  BEIR rows rather than presenting them as one fresh run.
- Produce a separately labeled current-P2.2 evaluation. Begin with bounded
  differential checks, then expand to the full matrices before making new
  full-surface quality claims. Preserve the P2.1 baseline unchanged.
- Report real pooled P50/P95/P99 alongside per-dataset values. Keep encoder,
  lookup, total query, cold-start, and warm-query timings separate; include
  the dense query encoder in any end-to-end speed comparison.
- Measure 1/4/8/16 concurrency where host capacity permits, with QPS, CPU,
  RSS/shared residency, disk/I/O, cache state, and tail latency. Include
  read-only and mixed-read/write runs with maintenance active.

Deliverable: an identity-bound baseline with explicit measurement coverage and
gaps, not an inferred SLO from historical single-worker timings.

## 2. Reduce High-DF Posting Cost Without Regressing Serving

High DF is a hypothesis to measure, not an explanation for every slow query.
Start with representative high-DF and ordinary queries on MS MARCO and
qualified product corpora. Isolate compiler, filtering, traversal, cache misses,
I/O, and background publication before changing an algorithm.

- Profile query-atom DF, posting/block touches, skip and upper-bound
  effectiveness, candidate counts, shard fanout where applicable, and cache
  behavior. Compare BM25-only and unified BM25-plus-semantic paths.
- Evaluate incremental improvements to existing compact posting layouts,
  block skipping/upper-bound pruning, locality, and artifact sharing or
  deduplication. Measure memory and disk as well as query latency.
- One candidate is a derived term-local exact-bound directory under the existing
  accelerator generation: merge signed bounds into global block order and
  intersect allowed blocks before opening posting payloads. Validate it against
  current packed-reference page reuse and forward-bound structures before
  adding a new representation. Canonical postings remain score authority and
  the existing exact path remains the fallback. This is a proposal, not shipped
  functionality.
- Treat impact quantization and semantic-support/budget changes as explicit
  quality/cost experiments. Compare with existing `f32`/`u8` and alpha modes;
  do not silently turn approximate profiles into generic defaults.
- Start locally on small indexes and increase scale only after behavior is
  understood. Record root, accelerator, and serving-baseline identities,
  publication intervals, bytes rewritten, and resource use under continuous
  writes. Reuse compatible serving artifacts while bounded background work
  converges; do not introduce a serving-baseline expiry or force foreground
  rebuilds because delta exists.
- Prefer changes that reuse current roots. Require evidence and a separate
  migration decision before any full rebuild or incompatible format change.

Deliverable: a narrow measured change with an explained mechanism and
before/after behavior, not a stack of threshold adjustments. Current
maintenance and accelerator designs remain authoritative; historical research
plans are not reopened automatically.

## 3. Improve Quality and Generalization

- Evaluate independent corpora and authorized real-query samples excluded
  from policy selection. Keep a held-out set separate from calibration and
  label any reused selection surface.
- Quantify the bundled NFCorpus vocabulary/calibration boundary before
  proposing broader calibration or a replacement checkpoint. Keep one global
  policy unless a separately reviewed product contract requires otherwise.
- Measure P2.2 full-text windows on long documents against the historical
  6,000-character preparation and 512-token P2.1 limits. Record chunk/window
  aggregation, input length, inference cost, and relevance separately.
- Target the remaining NDCG/MAP/MRR gap without hiding losses in Recall@100
  or CUB@1000. Report downstream reranker gains separately from single-index
  first-stage retrieval; do not add an unreported ANN or late-fusion route.

Deliverable: independent quality evidence tied to the exact model and input
policy, with any model change versioned and qualified separately.

## Acceptance and Stop Conditions

- Exact execution optimizations preserve ranked identities and scores within
  the declared numerical contract. Filtered queries must be compared against
  the complete filtered top-k reference, not just predicate membership.
- Approximate experiments declare their trade-off in advance. Track all five
  metrics and per-dataset changes; preserve Recall@100 within a predeclared
  statistical tolerance and the CUB@1000 advantage, or document any departure
  for explicit review. Do not trade away head ranking silently.
- Set workload-specific latency/resource limits before measurement. Require
  warm-query stability while maintenance converges, no major foreground
  blocking, and no unbounded rewrite, memory, or disk growth. Stop and retain
  evidence if quality, visibility, or latency regresses materially.
- Reproduce locked model/package artifacts from their manifests. Validate
  rebuilt indexes by logical identities, scores, options, and integrity;
  identical physical index bytes are not assumed across hosts or build orders.
- A new default, model contract, or physical format needs its own review and
  qualification. Passing a local microbenchmark does not close a full-matrix
  or deployment gate.

No implementation or new benchmark is recorded as completed by this document.
As work lands, link its evidence here and update the release reports only when
new measurements justify a new report edition.
