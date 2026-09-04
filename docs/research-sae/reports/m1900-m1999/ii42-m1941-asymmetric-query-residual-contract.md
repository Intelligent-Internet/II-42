# II-42 M1941 Asymmetric Query-Residual Contract

Date: 2026-07-13

## Motivation

M1940 changed a shared query/document encoder. At step 256 it improved overall
pairwise, top1, MRR, lexical-correct harm, and posting support, but BM25-error
rescue fell by 0.007519. The model became safer and smaller rather than more
complementary.

This creates one causal question: did shared parameter updates damage the
frozen document posting geometry needed by the residual objective?

M1941 changes the architecture, not a loss coefficient. It trains only an
asymmetric query encoder while every document posting and document score basis
remains frozen at M1914.

## Literature Basis

- Unified LSR reports that document term weighting is the largest quality
  contributor and that a non-expanding asymmetric query encoder can preserve
  effectiveness while reducing latency.
- BM25 Query Augmentation Learned End-to-End demonstrates transferable
  query-only lexical augmentation with document-frequency-aware cost control.
- CLEAR motivates stronger semantic pressure only when lexical retrieval
  fails.
- DF-FLOPS motivates direct corpus document-frequency measurement.

References:

- https://arxiv.org/abs/2303.13416
- https://arxiv.org/abs/2305.14087
- https://arxiv.org/abs/2004.13969
- https://arxiv.org/abs/2505.15070

## Controlled Difference

M1941 reuses exactly the M1940:

- 2,048/512 query-disjoint public rows;
- fixed BM25 candidate universe;
- six source/residual strata;
- candidate construction;
- residual and selective-PPLX objectives;
- trainable query parameter set;
- learning rate, steps, seed family, and checkpoint gate.

Only the forward geometry changes:

```text
query -> trainable M1914 query path
document -> frozen M1914 document path
score -> sparse dot product in the same vocabulary/index
```

There is still one physical posting index. The product artifact is the frozen
document encoder/index plus a query adapter in one checkpoint package, not an
ANN sidecar or a reranker.

## Additional Cost Gate

Document bytes, document nnz, and document maxDF must remain exactly at the
frozen baseline. Query support stays at Top50. In addition to all M1940 gates,
the document-DF-weighted query impact may rise by at most 10% and mean query
support by at most 20%.

## Decision

- Pass: authorize native regeneration using frozen M1914 documents and the
  selected query adapter.
- Fail with no rescue movement: query-only residual observability is closed on
  this public source.
- Fail through query DF load: the residual is expressible but violates native
  traversal cost; do not hide it with post-hoc pruning.
- Do not follow failure with weight, threshold, or step sweeps.
