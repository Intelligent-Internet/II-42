# M1660 Official BMP Engine Contract

## Purpose

M1640 proved that the frozen OpenSearch learned-sparse representation is
useful, but its Python contiguous-block proxy could not satisfy traversal and
metadata-work gates simultaneously. The proxy omitted the core mechanisms of
BMP: quantized impacts, compressed range maxima, SIMD aggregation, and the
hybrid block-forward index.

M1660 tests the official BMP implementation before any larger model training.
It isolates the engineering question:

> Can the already validated learned-sparse output execute as exact top-100
> retrieval in a real single inverted index with practical cost?

This is not a model comparison and does not authorize changing the sparse
checkpoint.

## Frozen Inputs

- BMP source: official `pisa-engine/BMP` repository at commit
  `c0a17ffc` (`bmp==0.2`, Apache-2.0).
- Dataset: complete official FiQA surface used by M1640, 57,638 documents and
  648 evaluated queries.
- Representation: cached output of
  `opensearch-project/opensearch-neural-sparse-encoding-v2-distill`, revision
  `269e6638b2c4f648996691f6d751495285d8f330`.
- Retrieval: `k=100`, `alpha=1.0`, `beta=1.0`.
- BMP shape: block size 16 with compressed ranges. No block-size search.
- Document order: source order. BP ordering is reported as missing rather than
  simulated or tuned.

## Locked Quantization

BMP stores document impacts as unsigned 8-bit values and normalizes each
query to a maximum impact of 32. M1660 uses one corpus-wide document scale:

```text
doc_u8 = clip(round(doc_float / global_doc_max * 255), 1, 255)
query_u32 = ceil(query_float / query_max * 32)
```

Zero entries remain absent. There is no percentile clipping, per-term scale,
query-dependent document scale, pruning, threshold, or quality-selected
quantizer.

Quality is reported twice:

1. the existing float sparse M1640 reference;
2. exhaustive retrieval over the exact quantized vectors used by BMP.

This separates 8-bit representation loss from engine exactness.

## Measurements

The benchmark must record:

- build wall time and peak resident memory where available;
- serialized index bytes and bytes per document;
- index load wall time;
- query latency mean, p50, p95, and p99 after five warm-up queries;
- exhaustive quantized query latency on the same host;
- BMP top-100 set parity and score parity against exhaustive quantized scores;
- NDCG@10, MAP@100, Recall@100, MRR@20, and CUB@1000 for float reference,
  quantized exhaustive, and BMP;
- document/query nonzeros and quantization scale.

Ties are audited separately. Exactness is score equivalence plus equality of
all documents strictly above the top-100 tie boundary; arbitrary ordering
inside an equal-score boundary is not treated as an engine error.

## Gate

The official engine passes only if all conditions hold:

- all 648 queries return results without failure;
- top-100 strict-boundary parity is `1.0`;
- returned scores match exhaustive integer dot products exactly;
- BMP quality equals quantized-exhaustive quality up to tie ordering;
- quantized Recall@100 retains at least 98% of the float sparse reference;
- BMP p95 latency is lower than exhaustive quantized p95 latency;
- serialized index size and latency are finite and explicitly reported.

The 98% requirement is a representation gate, not an engine approximation
allowance. BMP itself must remain exact.

## Decisions

- **Pass:** authorize one larger, standard learned-sparse training reproduction
  with BMP cost tracked as the deployment surface.
- **Quantization fail, engine pass:** retain BMP, redesign training-time impact
  quantization; do not mutate traversal.
- **Exactness fail:** stop and diagnose the binding/index before model work.
- **Cost fail:** test official BP document ordering once. Do not run block-size
  or threshold grids.
- **BP cost fail:** close BMP for the current product target and return to the
  native PostgreSQL unified-posting engine contract.

## Literature Basis

- BMP (`2405.01117`) reports exact dynamic pruning for learned sparse
  retrieval and identifies block size 16, BP ordering, compressed range
  maxima, and hybrid forward verification as the relevant system design.
- Dense2Sparse probabilistic expansion control (`2402.17535`) motivates
  measuring expansion and access cost rather than quality alone.
- The OpenSearch inference-free sparse method (`2411.04403`) provides the
  frozen single-index representation and dense-plus-sparse training teacher.
- Vocabulary Transfer (`2607.00004`) supports full vocabulary-aligned sparse
  training rather than another pooled dense output head.

No qrels may influence quantization, index parameters, or selection. They are
evaluation-only.
