# M1730 Bounded Residual Block Contract

## Question

M1727 proves that frozen balanced postings expose useful candidates at
`0.179909x` reads. M1728 proves that one-hop key-hit/logit scores cannot rank
them. M1730 asks the next structural question before any model is trained:

> Can one physical posting index retain the M1727 candidate gain by decoding a
> compact residual payload for only a bounded set of geometrically selected
> posting blocks?

This is not ANN plus BM25. Lexical and semantic postings remain the access
path. Compact forward payloads are read only for documents admitted through
those postings.

## Frozen Surface

- M1600 10,000-query/88,992-document train pool for representation fitting;
- disjoint 1,000-query/8,988-document validation pool for every gate;
- frozen M1600 8x512 source and exact M1727A-v2 expansion policy;
- exact frozen BM25 top256 candidates;
- no qrels, dataset identity, learned selector, score alpha, or search grid.

## Fixed Compact Representation

The M1600 code combination is retained as the first-order document
reconstruction. Fit one global rank-96 PCA basis to 20,000 deterministic
training residuals:

```text
document = first_order_code_reconstruction + residual
residual ~= PCA96(INT8 coefficients)
```

The document forward payload contains:

- 96 INT8 residual coefficients;
- eight FP16 first-order impacts;
- one FP16 reconstructed norm.

The representation is fixed at 114 bytes/document. Quantization scales use
the train-pool 99.9th absolute percentile and cannot be tuned after validation.

The exact full vector over the same candidate set is the integrity control.

## Fixed Block Layout

Each of the 4,096 frozen semantic posting lists is partitioned independently:

- maximum block size: 16 posting entries;
- deterministic spherical clustering when a list needs more than one block;
- five Lloyd iterations;
- one 96-dimensional INT8 summary centroid plus FP16 radius per block.

The global summary PCA basis is fitted independently on the same 20,000 train
documents. The radius includes projection and quantization error:

```text
upper(q, block) = dot(q_projected, centroid_INT8) + block_radius
```

Because query/document vectors are unit normalized, this is a safe full-space
upper bound. BM25-only documents are decoded directly. Semantic blocks are
opened in descending upper-bound order until the next bound cannot enter the
current exact top256.

## Separate Measurements

M1730 reports four validation surfaces:

1. exact full-vector score over every M1727 candidate;
2. compact residual score over every M1727 candidate;
3. exact full-vector score after safe block pruning;
4. compact residual score over the documents decoded by safe block pruning.

This separates compact-score capacity from block-selectivity capacity.

Report O@10/O@100/O@256, exact integrity, candidate and decoded document
counts, summary scans, opened blocks, decoded posting entries, p95 costs,
block count, bytes/document, residual reconstruction cosine, and assignment
parity with M1727.

## Gates

All conditions are conjunctive:

- exact candidate control reproduces M1727A-v2 O@10/O@100/O@256;
- safe-block exact score matches the exact candidate control at all depths;
- compact all-candidate and compact block scores each retain at least 95% of
  the corresponding exact O@10/O@100/O@256;
- mean summary scans `<=256` per query;
- mean decoded unique documents `<=512`, p95 `<=1,024`;
- total compact payload plus block-summary storage `<=256` bytes/document;
- M1727 mean posting reads remain `<=0.18x`, max DF `<=0.02`;
- M1600 key-assignment parity is exactly 1.0.

## Stop

- Compact-score failure closes 96-byte residual scoring before engine work.
- Safe-bound cost failure closes this block organization even if exactness
  passes.
- Combined failure cannot be repaired with another rank, block size, payload
  width, quantile, selector, or score normalization.
- Only a conjunctive pass authorizes one native/full-corpus prototype and
  later text-to-residual compiler training.
