# M1565 Fixed-Budget Route Frontier Contract

## Objective

M1564 showed that the unchanged nearest-two corpus routes preserve FiQA dense
top100 almost exactly at a 15% candidate union. That surface touches 8,646 of
57,638 documents, so it does not establish a practical inverted-index access
path.

M1565 measures the same semantic posting source at fixed absolute candidate
budgets. It changes no route assignment, score, codebook, or query policy.
The result decides whether the semantic substrate can carry M1549 lexical
rescue inside one inverted index.

HI2 (`arXiv:2210.05521`) is the architecture basis: embedding-cluster lists
and salient-term lists share one inverted index. M1565 tests the cluster-list
half before any lexical residual or selector training is allowed.

## Frozen Surface

- official FiQA: 57,638 documents and 648 qrel queries;
- frozen `BAAI/bge-base-en-v1.5` embeddings;
- spherical k-means with `ceil(document_count / 16)` routes;
- nearest-two centroid assignments, identical to the M1564 control;
- fixed candidate budgets `256,512,1000,2048` plus the 15% parity anchor;
- exact dense reranking only over documents reached through route postings;
- deterministic centroid traversal and route-cover oracle;
- no lexical candidates, qrels, labels, training, or parameter selection.

The qrels-free embeddings, codebook, assignments, and IDs must be persisted as
a reusable basis artifact before qrels are loaded.

## Integrity Gate

- official root identity must be exact;
- the 15% anchor must reproduce every reported M1564 nearest-two control
  metric with maximum absolute delta at most `1e-9`;
- candidate unions may not exceed their declared absolute budget;
- the 1,000-candidate path may read at most 5% of the corpus postings.

## Capacity Gate At 1,000 Candidates

The deterministic route path must satisfy:

- O@10 at least `0.99`;
- O@100 at least `0.95`;
- O@256 at least `0.90`;
- Recall@100 at least 98% of exact dense;
- NDCG@10, MAP@100, and MRR@20 each at least 99.5% of exact dense.

The route-cover oracle independently requires O@100 at least `0.98` and
O@256 at least `0.95`.

## Decisions

1. Deterministic and oracle pass: authorize M1566 single-index lexical
   residual replay without ANN or a second BM25 index.
2. Oracle passes but deterministic fails: authorize one HI2-style supervised
   cluster-selector distillation stage. Do not change document assignments.
3. The gate passes only at 2,048 candidates: retain as high-cost diagnostic,
   not a product or training target.
4. Oracle fails at both 1,000 and 2,048: stop this corpus-route source.

Do not tune cluster count, assignment fanout, candidate budgets, or thresholds
after observing this result.
