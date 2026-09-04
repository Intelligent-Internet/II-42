# M1570 Composite Latent Posting Contract

## Objective

M1565 closes coarse corpus routes at practical candidate budgets. M1566-M1569
close exact lexical compression because the useful source either loses dense
membership or requires high-DF scans. M1570 changes the posting vocabulary:

> Can one low-DF composite semantic key per document preserve dense-neighbor
> access through a single inverted posting namespace?

This is a qrels-free source-capacity audit before any encoder, adapter, query
selector, lexical residual, or native-index integration.

## Literature Mechanism

The source follows the [Inverted Multi-Index](https://www.robots.ox.ac.uk/~vilem/cvpr2012.pdf):
product quantization creates a much finer posting partition than one global
coarse codebook. The audit also respects the negative result in
[Revisiting the Inverted Indices for Billion-Scale ANN](https://arxiv.org/abs/1802.02422):
deep descriptors may be too entangled for a product code, so source capacity
must be measured rather than assumed.

## Frozen Construction

- official FiQA and the exact M1565 frozen BGE basis;
- uncentered second-moment rotation, preserving every dense dot product;
- greedily variance-balanced orthogonal split into two 384-dimensional
  subspaces;
- one independent Euclidean K-means codebook with 256 cells per subspace;
- one composite `(left_cell, right_cell)` posting per document;
- no qrels, queries, BM25, exact terms, ANN graph, or external index in source
  construction;
- deterministic query key score equal to the sum of the two subspace centroid
  dot products;
- exact dense reranking only after candidates are frozen.

The fixed candidate budgets are 1,000, 1,256, and 2,048. Every posting read,
empty composite-cell probe, and unique candidate is counted.

## Diagnostic Oracle

A qrels-free cell oracle orders whole non-empty composite cells by the best
exact-dense rank of any member for the current query. It may diagnose source
capacity but cannot pass the deployable gate by itself.

All codebooks, assignments, and candidate surfaces are persisted before qrels
are loaded.

## Gates

Integrity requires:

- rotated dense score maximum absolute error `<=1e-5`;
- exactly one composite posting per document;
- deterministic candidate counts equal each fixed budget;
- mean posting reads equal at most 3% of corpus size at budget 1,256;
- p95 composite-cell probes at budget 1,256 `<=4096`.

The primary deterministic budget-1,256 row must reach:

- O@10 `>=0.98`, O@100 `>=0.95`, and O@256 `>=0.90`;
- Recall@100 `>=0.72` and candidate upper bound `>=0.89`;
- at budget 1,000, at least `+0.05` O@100 and `+0.08` O@256 over the frozen
  M1565 route1000 source.

A restricted training bridge exists only if the deterministic 1,256 row
reaches O@100 `>=0.90` and O@256 `>=0.82`, while the cell oracle reaches
O@100 `>=0.98` and O@256 `>=0.95` under the same source and cost.

## Decisions

- Primary gate passes: replicate the unchanged source on NFCorpus and SciFact.
- Only the training bridge passes: authorize one retrieval-trained composite
  codebook, not a text encoder or lexical stage.
- Neither passes: stop product/composite semantic keys and do not search
  codebook counts, splits, rotations, or probe budgets.

No source may be promoted from FiQA alone.
