# M1544 Clustered Coordinate-Block Contract

## Question

M1541 showed that full signed-coordinate accumulation selects an excellent 8%
candidate set but requires a full posting union. M1542 showed that global
corpus routes provide honest access but lose too much dense neighborhood.
M1544 tests the missing access structure between them:

> Can Seismic-style corpus-geometric blocks inside each signed coordinate
> posting list preserve accumulated dense evidence under bounded reads?

This is an index-mechanism audit. It does not train a selector, projection,
reranker, or output head.

## Frozen Construction

- frozen BGE pooled document/query vectors;
- M1541 document-only PCA rotation, 128 active signed document coordinates,
  and 256-dimensional INT8 tail scorer;
- fit the same document-only spherical route codebook as M1542;
- partition each signed coordinate posting by its document route assignment;
- chunk each route-conditioned posting group at 16 entries;
- store a normalized full-vector centroid and maximum anchor impact per block;
- qrels remain evaluation-only.

## Query Access

For every active signed query coordinate:

1. enumerate matching coordinate blocks;
2. score each block by
   `max(0, query dot block_centroid) * max_anchor_contribution`;
3. visit blocks in descending score;
4. stop when either the unique union reaches 15% or total posting reads reach
   30% of the corpus;
5. rerank the admitted union with the frozen tail256 INT8 score.

Duplicate document reads across blocks count toward posting-read cost. Block
summary scans, posting reads, unique union, and rerank count are reported
separately.

## Gate

The 15% tail row passes a dataset only when:

- Recall@100, NDCG@10, and CUB are each at least 95% of exact BGE;
- exact-dense candidate upper overlap@100 is at least 0.90;
- raw union is at most 0.151;
- posting reads are at most 0.301.

Scale requires two of three dataset passes and no row below 90% of exact BGE
Recall or NDCG.

## Decisions

- **Pass:** validate the same block access in the native unified index, then
  add lexical evidence without changing the semantic source.
- **Admission pass, tail fail:** permit one deterministic sketch-capacity
  check.
- **Fail:** close the pooled-dense inverted-posting equivalence route. Keep
  M1542 only as an efficiency approximation and use a proper ANN index where
  dense-equivalent recall is required.

