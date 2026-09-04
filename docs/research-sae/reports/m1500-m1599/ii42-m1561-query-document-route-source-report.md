# M1561 Query-Document Route Source Report

## Decision

**Stop degree-two bipartite route replacement before scale or training.**

M1561A proves that query-document teacher edges contain a strong transductive
head signal, but replacing the second semantic centroid route with that edge
does not preserve broad dense candidate access and does not transfer to
held-out queries.

This is not evidence for a deeper loss or a larger text model.  The failure is
already present in the frozen posting source before any model is trained.

## Surface

- Dataset: NFCorpus, 2,063 documents and 100 queries.
- Three deterministic 50/50 query splits.
- Dense teacher: frozen `BAAI/bge-base-en-v1.5` top256.
- Two posting edges per document and 15% unique candidate union.
- Exact dense reranking over touched candidates.
- Qrels loaded only after each posting source was frozen.
- ClearML task: `fb3eb190147541bf97226b8b168f7a7b`.
- M1542 parity maximum absolute delta: `0.0`.

## Primary Result

| Source | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| centroid dual | 0.848667 | 0.639133 | 0.486536 | 0.328144 | 0.441950 | 0.1723x |
| train bipartite dual | 0.814000 | 0.591133 | 0.449297 | 0.324268 | 0.439925 | 0.1620x |
| all-query bipartite dual | 0.939333 | 0.663333 | 0.498672 | 0.355843 | 0.449119 | 0.1663x |

The all-query diagnostic sees every evaluation query and improves O@10 by
`+0.090667`, but O@100 by only `+0.024200`.  Its oracle O@100 gain is only
`+0.034333`.  It therefore fails the representational-capacity gate even
before held-out transfer is considered.

The train-only source is negative on every held-out split:

| Split | O@100 delta | O@10 delta | O@256 delta |
| ---: | ---: | ---: | ---: |
| 1561 | -0.0370 | -0.0380 | -0.0340 |
| 2561 | -0.0630 | -0.0300 | -0.0463 |
| 3561 | -0.0440 | -0.0360 | -0.0315 |
| mean | -0.0480 | -0.0347 | -0.0372 |

## Failure Mechanics

The source does fit its construction queries.  Train-query direct route-edge
coverage@4 rises by `+0.1292`, `+0.1454`, and `+0.1492` across the splits.
The same index loses held-out coverage by `-0.0324`, `-0.0522`, and `-0.0438`.
This is source over-specialization, not insufficient optimization.

The replacement is also much less local than intended:

- train-only sources replace the second centroid route for `93.9%–94.5%` of
  documents;
- the all-query source replaces it for `98.35%` of documents;
- normalized route-load entropy falls from `0.9287` to `0.9042–0.9157`;
- maximum posting-list load increases from `168` to `197–267` for train-only
  sources.

The all-query source improves direct held-out route coverage and O@10 because
it concentrates known head neighbors into hot routes.  At the fixed union
budget, that concentration does not recover the wider top100/top256
neighborhood.  The route-cover oracle cannot repair it, confirming that the
problem is source factorization rather than the deterministic query router.

## Consequences

- Do not run SciFact/FiQA or shared15 for this source.
- Do not train a text-to-route compiler against these labels.
- Do not tune split seeds, teacher depth, thresholds, or route weights.
- Do not interpret the small qrels gains of the all-query diagnostic as a
  deployable result; it directly consumes evaluation-query dense edges.

## Next Bounded Probe

Run one causal factorization test under a new contract:

1. retain both M1542 centroid routes unchanged;
2. add the strongest query-derived route as a third residual posting edge;
3. compare against an equal-cost centroid-top3 source;
4. retain the same 15% unique union and count all reads;
5. repeat the transductive-capacity and three-split held-out gates.

This tests the evidence-backed hypothesis that M1561 failed by replacing a
general semantic edge, not that query-document posting supervision has no
value.  If the protected residual does not beat centroid top3, stop this
teacher-source family rather than beginning model training.
