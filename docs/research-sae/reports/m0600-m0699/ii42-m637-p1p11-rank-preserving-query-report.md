# M637 / P1.11 Rank-Preserving Query Compiler Report

Status: `canary_failed_with_rank_signal`

## Goal

M637 tested whether the M636 failure could be fixed by freezing document
postings and training only the query-side generated-posting compiler.  The
design follows the older query-side correction lesson: preserve dense top-k
geometry first, then apply retrieval constraints.

This stage used no BM25, fixed alpha, learned gate, dataset id, document id,
query id, or post-hoc reranker.

## Runs

| Run | Change | Gate | Selected trained checkpoint |
| --- | --- | --- | --- |
| `m637_p1p11_query_smoke_seed6371` | Dense-rank preservation + qrels promotion | failed | no |
| `m637_p1p11_query_boundary_seed6371` | Adds top100 boundary loss | failed | no |

Both runs used `FiQA2018, ArguAna` and compared against frozen M549 query/doc
postings.

## Dev Signal

| Run | dO@100 | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| A: rank-preserving | -0.000208 | +0.001223 | +0.001836 | +0.000000 | +0.000902 |
| B: boundary loss | -0.000208 | +0.001542 | +0.002183 | +0.000000 | +0.001249 |

The useful signal is that query-side training can improve ranking metrics while
keeping dense overlap inside the guard.  This is materially better than M636,
where the trained compiler broke dense overlap by about `-0.01234`.

## Test Macro

The selected model falls back to epoch0 because no trained checkpoint met the
Recall gate.  Therefore test metrics equal frozen M549:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.975391 | 1.000000 | 0.499620 | 0.400499 | 0.924674 | 0.475701 |
| `m549_frozen` | 0.975391 | 0.996484 | 0.499644 | 0.400598 | 0.924674 | 0.475766 |
| `m637_query` | 0.975391 | 0.996484 | 0.499644 | 0.400598 | 0.924674 | 0.475766 |

## Diagnosis

M637 proves that the query-side, rank-preserving route is safer than the M636
two-sided generated-posting compiler.  It can move MAP/NDCG/MRR without
destroying dense top100 geometry.

It does not yet move Recall@100.  Adding explicit top100 boundary loss improved
the ranking deltas but still did not push new positives across the top100
threshold on the dev gate.  This means the next bottleneck is not generic
ranking shape; it is the top100 boundary crossing signal.

## Stop / Continue Decision

Do not expand M637 to shared15 yet.

Keep the route alive because it has a clean positive rank signal and avoids the
M636 geometry collapse.  The next probe should be smaller and more diagnostic:

1. Count baseline top100 misses that are present in top1000 per query.
2. Train only on queries with boundary-positive cases.
3. Evaluate boundary crossing directly, not only macro Recall.
4. If boundary crossing improves without O@100 loss, then rerun the normal
   smoke gate.

The next stage should be M638: boundary-positive mining and query-side recall
crossing audit.
