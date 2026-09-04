# M1571 Overlapping Cosine-LSH Posting Contract

## Objective

M1565 and M1570 reject mutually exclusive centroid/product cells as
dense-equivalent candidate sources. M1571 tests the final evidence-backed
single-hop source:

> Can overlapping random-hyperplane bucket postings recover dense neighbors
> at bounded inverted-index reads because collision probability is directly
> monotonic in cosine similarity?

This is a qrels-free source-capacity audit. It is not a hash grid, learned
router, ANN graph, or external index.

## Mathematical Basis

[Charikar's random-hyperplane LSH](https://www.cs.princeton.edu/courses/archive/spr05/cos598E/bib/p380-charikar.pdf)
gives bit collision probability `1 - angle(q,d)/pi`. Multiple tables create
overlapping corpus partitions, avoiding the one-cell commitment that failed
M1565/M1570. Low-margin bit flips provide the fixed multi-probe order.

## Frozen Construction

- official FiQA and the exact M1565 frozen normalized BGE basis;
- 16 independent tables, matching the approximately 15 postings per document
  already admitted in M1568;
- 12 orthonormal random hyperplanes per table, derived analytically from
  `ceil(log2(57638 / 16)) = 12` bits for about 16 documents per bucket;
- fixed seed 1571;
- one bucket posting per document per table;
- query probes exact, Hamming-1, and Hamming-2 buckets globally by summed
  absolute hyperplane margin;
- fixed candidate budgets 1,000, 1,256, and 2,048;
- exact dense reranking only after the posting candidate surface is frozen;
- no BM25, exact terms, qrels, threshold tuning, or fallback candidates.

Every duplicate posting read, bucket probe, and unique candidate is counted.
The final bucket may be decoded only until the fixed unique budget is reached.

## Gates

Integrity requires:

- exactly 16 postings per document;
- maximum bucket DF ratio `<=0.01`;
- exact unique candidate count at all three budgets;
- at budget 1,256, mean reads `<=0.10x`, p95 reads `<=0.30x`, and p95 bucket
  probes `<=512`.

The deterministic budget-1,256 row must reach:

- O@10 `>=0.98`, O@100 `>=0.95`, and O@256 `>=0.90`;
- Recall@100 `>=0.72` and candidate upper bound `>=0.89`;
- at budget 1,000, at least `+0.05` O@100 and `+0.08` O@256 over frozen
  M1565 route1000.

## Decisions

- All gates pass: replicate the unchanged source on NFCorpus and SciFact.
- Any capacity or cost gate fails: stop random-hyperplane LSH and close the
  single-hop semantic posting source branch.

Do not change table count, bit count, radius, seed, probe order, or budgets
after observing the result. A failure does not authorize learned hashing.
