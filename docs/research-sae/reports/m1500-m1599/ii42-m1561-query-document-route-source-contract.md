# M1561 Query-Document Route Source Contract

## Objective

M1560A improved document-document neighborhood co-location but reduced query
candidate access.  M1561 therefore changes the supervised object from a
document graph to a qrels-free query-document bipartite graph.

The product target remains one encoder, one posting map, and one II42 inverted
index.  Dense vectors are an offline teacher only.  This stage does not train
a text head; it first asks whether query-conditioned route membership has
enough held-out capacity to justify training.

## Frozen Surface

- frozen `BAAI/bge-base-en-v1.5` embeddings;
- document-only spherical k-means from M1542;
- two route postings per document;
- 15% exact unique candidate union and at most 30.1% posting reads;
- exact dense reranking over touched documents;
- NFCorpus first, with three deterministic 50/50 query splits;
- qrels are loaded only after every split-specific posting source is frozen.

The centroid-dual all-query row must reproduce M1542 exactly before the new
source is interpreted.

## Source Construction

For each split, use only the calibration-query text and frozen dense teacher:

1. assign every query to its nearest document centroid route;
2. take the frozen dense top256 documents for that query;
3. add reciprocal-log-rank mass from the query route to each top256 document;
4. retain the document primary centroid route;
5. choose as its secondary route the non-primary query route with greatest
   accumulated mass, falling back to the second centroid route when no
   calibration edge exists.

Posting order is source-aware but query-independent.  A document-route edge
observed in the calibration graph is ordered by accumulated teacher mass,
with centroid affinity as the deterministic fallback.  The index still stores
ordinary `(route, document, impact)` posting entries.

## Required Decomposition

Each held-out split compares:

- `centroid_dual`: frozen M1542 source;
- `train_bipartite_dual`: built from the other half of queries;
- `all_query_bipartite_dual`: diagnostic source that also sees held-out query
  dense edges.

Every source is evaluated with the deterministic centroid query router and the
M1560 dense-top100 route-cover oracle.  The all-query source is not deployable;
it separates fixed-degree source capacity from held-out transfer.

Report train and held-out direct route-edge coverage at query-route depths 1
and 4, route-load skew, O@10/O@100/O@256, CUB, Recall@100, NDCG@10, MAP@100,
MRR@20, union, reads, and probes.

## Gates

### Representational Capacity

The all-query source must, averaged across the three held-out folds:

- improve centroid-routed O@100 over centroid dual by at least `0.10`;
- improve oracle O@100 by at least `0.05`;
- avoid reducing O@10 or O@256 by more than `0.01`;
- obey the union and read limits.

If this gate fails, fixed degree-two centroid-route postings cannot express the
query access teacher under this construction.  Stop before more data or
training.

### Held-Out Transfer

The train-only source must:

- improve mean held-out centroid-routed O@100 by at least `0.03`;
- improve every split by at least `0.01`;
- avoid reducing mean O@10 or O@256 by more than `0.01`;
- improve held-out direct route-edge coverage@4;
- obey the union and read limits.

This is only a source-signal gate.  Text compiler training is not authorized
until a later corpus-derived synthetic-query source also transfers.

## Decisions

- Capacity and transfer pass: expand to SciFact and FiQA, then replace
  benchmark calibration queries with corpus-derived synthetic queries.
- Capacity passes but transfer fails: do not train a router or selector; the
  next bounded question is query-generation coverage.
- Capacity fails: stop this route vocabulary/source and revisit posting
  factorization or document fanout under a new contract.
- Any qrels dependency, dataset-specific threshold, larger budget, or hidden
  ANN/BM25 runtime invalidates the result.
