# II-42 M366 Dense Coordinate Posting Report

## Goal

M366 asks a more basic dense-only question:

If atom postings are allowed to be signed dense coordinates, how close can we get
to dense retrieval without learning any SAE basis?

This completely removes BM25, qrels, scorer features, admission objectives, and
learned atom training. It keeps the largest signed dense coordinates and scores
with sparse coordinate cosine.

## Setup

- Input: `/Volumes/Betty/Tmp/psql_bm25s_sae_m80/neutral-stage-a-v0`
- Docs / queries / eval queries: 4096 / 128 / 23
- Embedding dim: 768
- Seed: 362
- Gate: exact dense top-k overlap

## Results

| Active dense coordinates | Exact O@10 | Exact O@20 | Exact O@100 |
| ---: | ---: | ---: | ---: |
| 16 | 0.1304 | 0.1152 | 0.1513 |
| 32 | 0.2391 | 0.2587 | 0.2517 |
| 64 | 0.4000 | 0.4087 | 0.4091 |
| 96 | 0.5304 | 0.4978 | 0.5178 |
| 128 | 0.5826 | 0.5783 | 0.6022 |
| 192 | 0.6913 | 0.6761 | 0.7157 |
| 256 | 0.7957 | 0.7435 | 0.7804 |
| 384 | 0.8609 | 0.8522 | 0.8700 |
| 512 | 0.9565 | 0.9391 | 0.9396 |
| 768 | 1.0000 | 1.0000 | 1.0000 |

## Interpretation

This is the strongest dense-only result so far.

At active-k 384, simple signed dense-coordinate postings already exceed the
M363 promotion threshold:

- Exact O@10: 0.8609
- Exact O@100: 0.8700

At active-k 512, it is very close to exact dense:

- Exact O@10: 0.9565
- Exact O@100: 0.9396

This changes the route:

- atom posting form is not the blocker;
- learned SAE atoms are currently weaker than raw dense-coordinate postings;
- dense-coordinate postings are the first practical dense-only baseline that
  actually approaches dense;
- future learned atoms must beat this baseline, not M320/M362.

## Product Implication

If storage and posting fanout are acceptable, signed dense-coordinate postings
could be a direct dense-only product baseline:

- one atom per dense coordinate and sign;
- query/doc retain top-k absolute dense coordinates;
- score with signed coordinate cosine or equivalent weighted dot;
- no BM25 required.

This is not yet optimized for index size or query-time cost, but it is already
quality-competitive with dense on the local gate.

## Next Step

M367 should test rotated-coordinate postings:

1. Apply random orthogonal / Hadamard-style rotations before top-k.
2. Compare k128, k192, k256, k384 against raw coordinates.
3. Measure posting fanout and score quality together.
4. If rotation improves low-k preservation, use it as the new dense-only atom
   baseline.

The learned SAE route should pause until it can beat dense-coordinate postings.
