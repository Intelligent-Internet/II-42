# M1562 Protected Query-Residual Route Contract

## Objective

M1561A showed that query-document edges fit their construction queries but
fail held-out access because they replace the second centroid edge for about
94% of documents.  M1562 tests one causal repair: preserve both semantic
centroid routes and add one query-derived residual route.

This remains a unified inverted index.  Every document publishes three route
postings into one map; there is no ANN or BM25 runtime path.

## Equal-Cost Comparison

At the same three posting edges per document, compare:

1. `centroid_triple`: the three nearest document centroids;
2. `protected_query_triple`: the two nearest centroids plus the strongest
   non-base route from the qrels-free query-document top256 graph, falling
   back to the third centroid when no teacher edge exists.

Also retain M1542 centroid dual as the frozen parity and product-cost anchor.
All sources use an exact 15% unique candidate union.  Every duplicate posting
read is counted; triple-source reads must remain at or below 45.1%.

## Evaluation

- NFCorpus first;
- three fixed 50/50 query splits;
- train-only and all-query diagnostic sources;
- deterministic centroid query routing and dense-top100 route-cover oracle;
- exact dense reranking over touched candidates;
- qrels only after sources and candidate policies are frozen.

Report O@10/O@100/O@256, direct route-edge coverage, CUB, Recall@100,
NDCG@10, MAP@100, MRR@20, route load, union, reads, probes, and storage.

## Gates

All-query capacity passes only if protected query triple, relative to centroid
triple, has:

- mean centroid-routed O@100 gain at least `0.05`;
- mean oracle O@100 gain at least `0.03`;
- O@10 and O@256 deltas no worse than `-0.01`;
- valid union and read costs.

Held-out transfer passes only if the train-only protected source has:

- mean centroid-routed O@100 gain at least `0.02`;
- no negative O@100 split;
- O@10 and O@256 deltas no worse than `-0.005`;
- positive held-out direct route-edge coverage@4 gain;
- O@100 no lower than centroid dual;
- valid union and read costs.

## Stop Conditions

- If all-query capacity fails, stop query-bipartite route supervision.
- If capacity passes but held-out transfer fails, do not train a compiler;
  first test whether corpus-derived query scale can close the coverage gap.
- Do not sweep fanout, teacher depth, route count, budget, or thresholds.
- Do not expand datasets unless both capacity and transfer pass.
