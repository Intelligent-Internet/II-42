# M1563 SOAR Spilled-Route Contract

## Objective

M1542 showed that two centroid assignments are useful but not dense-equivalent.
M1562 showed that adding a third nearest-centroid assignment is redundant and
can reduce access at a fixed union budget.  This matches the failure analyzed
by SOAR: nearest and second-nearest VQ residuals have correlated quantization
errors, so naive spilling gives little independent recovery.

M1563 tests the corpus-only SOAR assignment from `arXiv:2404.00774` as two
ordinary route postings per document.  The product target remains one encoder,
one posting map, and one II42 inverted index.

## Assignment

Train the frozen M1542 spherical k-means codebook.  For normalized document
`x`, primary centroid `c`, primary residual `r = x - c`, and candidate spilled
centroid `c'` with residual `r' = x - c'`, choose the non-primary route that
minimizes:

```text
L(r', r) = ||r'||^2 + lambda * ||proj_r(r')||^2
```

Use the paper's fixed `lambda = 1`.  Do not tune lambda, centroid count,
fanout, teacher depth, or budget.  Compare against M1542 naive centroid top2
with exactly the same codebook, storage fanout, query router, and 15% candidate
union.

## Diagnostics

Before qrels evaluation, use frozen dense top100 query-document pairs to report:

- Pearson correlation between primary and spilled quantized score errors;
- mean best centroid rank across the two assignments;
- fraction of dense top100 pairs whose best route ranks in top1/top4/top8;
- route-load entropy and maximum posting load.

These query diagnostics do not affect construction.  Qrels are loaded only
after assignments and query policies are frozen.

## Retrieval Surface

- NFCorpus first;
- exact M1542 centroid-dual parity;
- deterministic centroid query routing;
- dense-top100 route-cover oracle for codebook decomposition;
- exact dense reranking over touched candidates;
- O@10/O@100/O@256, CUB, Recall@100, NDCG@10, MAP@100, MRR@20;
- exact union, posting reads, probes, storage, and route load.

## Canary Gate

SOAR passes NFCorpus only when, relative to naive centroid dual:

- primary/spilled error correlation decreases by at least `20%`;
- mean best centroid rank decreases by at least `10%`;
- centroid-routed O@100 improves by at least `0.05`;
- route-cover oracle O@100 improves by at least `0.03`;
- O@10 and O@256 do not decrease by more than `0.01`;
- union is at most `0.151` and reads at most `0.301`.

## Decisions

- If every geometry and access check passes, expand unchanged to SciFact and
  FiQA.
- If residual decorrelation passes but access fails, stop before training: the
  index traversal cannot realize the geometric benefit.
- If decorrelation itself fails, stop this source immediately.
- No qrels, BM25, benchmark query fitting, selector, or ANN runtime is allowed
  in construction or product inference.
