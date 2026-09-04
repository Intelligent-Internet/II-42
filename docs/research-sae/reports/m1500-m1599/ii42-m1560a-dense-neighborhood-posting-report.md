# M1560A Dense-Neighborhood Posting Report

## Conclusion

M1560A rejects document-document dense-neighborhood replication as the next
unified-posting source.  The corrected graph construction improves exact
top-32 document-edge co-location, but reduces query-to-document dense
candidate access under both the deployable centroid router and the diagnostic
dense-neighbor oracle.

This is a source-level stop, not a rejection of the unified inverted-index
product.  It shows that document graph preservation is not equivalent to
query candidate access.  The next source must train document route membership
from a qrels-free query-document bipartite graph and validate on held-out
queries before any text compiler is trained.

## Surface

- Dataset: NFCorpus canary, 2,063 documents and 100 queries.
- Dense root: frozen `BAAI/bge-base-en-v1.5`.
- Index budget: two route postings per document, 15% unique candidate union.
- Source comparison: M1542 centroid dual versus graph dual.
- Query comparison: centroid routing versus dense-top100 route-cover oracle.
- Qrels usage: evaluation only after postings and candidates were frozen.
- Corrected ClearML task: `a163de34fc0548439f36447be75290bc`.

The M1542 centroid-dual parity delta is exactly `0.0`, so the comparison is
on the intended frozen surface.

## Result

| Surface | Edge coverage@32 | Query policy | O@100 | O@256 | Recall@100 | NDCG@10 | Reads |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| centroid dual | 0.614836 | centroid | 0.6443 | 0.490156 | 0.355283 | 0.455250 | 0.1730x |
| graph dual | 0.672882 | centroid | 0.6293 | 0.481563 | 0.350124 | 0.451372 | 0.1686x |
| centroid dual | 0.614836 | dense-neighbor oracle | 0.7344 | 0.526094 | 0.332412 | 0.434107 | 0.1686x |
| graph dual | 0.672882 | dense-neighbor oracle | 0.7080 | 0.506367 | 0.353979 | 0.446407 | 0.1650x |

The graph source increases document-edge coverage by `+0.058047`, while its
O@100 changes are `-0.0150` under centroid routing and `-0.0264` under oracle
routing.  Both the codebook and query-router gates fail.  The predeclared
fanout-four extension is therefore unauthorized.

## Implementation Correction

The first runtime smoke selected a secondary route by mean neighbor
similarity.  That did not implement the contract, which specifies the largest
cosine-weighted neighbor mass.  The formal run above orders secondary routes
by total mass and uses mean similarity only as a tie-breaker.  The earlier
runtime smoke is excluded from interpretation.

## Decision

Stop this source before full-three-dataset expansion, fanout extension, or
text-to-route training.  More epochs cannot repair a codebook whose oracle
query access is already worse than the frozen baseline.

Proceed to a separately versioned capacity test:

1. build route supervision from frozen dense query-document edges without
   qrels;
2. construct document postings using only training-query edges;
3. evaluate candidate access on held-out queries;
4. require a material O@100 gain before testing synthetic-query scaling or a
   deployable text compiler.
