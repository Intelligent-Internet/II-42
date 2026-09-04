# ii42 M348 Broad8 Result Diagnostic

## Summary

M348 is a no-GPU diagnostic over completed broad8 result JSONs. It compares scorer quality against candidate upper bound and checks whether M346A gains come from true residual recovery or from small top-rank reshuffling.

## Aggregate

|   Run |    R@100 |   MRR@20 |  NDCG@10 |  MAP@100 | UB NDCG Gap | UB MAP Gap |
| ----- | -------- | -------- | -------- | -------- | ----------- | ---------- |
|  m344 | 0.686372 | 0.375673 | 0.366046 | 0.296308 |    0.563955 |   0.592540 |
|  m346 | 0.683902 | 0.378038 | 0.368333 | 0.299527 |    0.561667 |   0.589322 |
| m347a | 0.682834 | 0.376239 | 0.364822 | 0.298607 |    0.565178 |   0.590241 |
| m347b | 0.682729 | 0.377569 | 0.367955 | 0.299346 |    0.562045 |   0.589503 |

## Dataset Gap Ranking

|          Dataset |     dNDCG |      dMAP | NDCG Gap |  MAP Gap | Top20 Rate |                                                                        Tags |
| ---------------- | --------- | --------- | -------- | -------- | ---------- | --------------------------------------------------------------------------- |
| webis-touche2020 |  0.017922 |  0.003944 | 0.661632 | 0.675505 |      0.933 |                                             top100_recall_gap, top_rank_gap |
|          arguana |  0.002381 |  0.006434 | 0.599539 | 0.729348 |      0.905 |                                                                top_rank_gap |
|          scidocs |  0.000431 |  0.000180 | 0.585895 | 0.546782 |      0.597 | candidate_ceiling, top100_recall_gap, top_rank_gap, many_queries_ranked_low |
|             fiqa |  0.005626 |  0.011544 | 0.578612 | 0.599816 |      0.701 |                    top100_recall_gap, top_rank_gap, many_queries_ranked_low |
|      cqadupstack |  0.002707 |  0.001724 | 0.558828 | 0.582478 |      0.627 |                    top100_recall_gap, top_rank_gap, many_queries_ranked_low |
|         nfcorpus | -0.002213 | -0.000442 | 0.512562 | 0.399554 |      0.753 |                          candidate_ceiling, top100_recall_gap, top_rank_gap |
|       trec-covid | -0.032708 | -0.001289 | 0.428550 | 0.171702 |      0.933 |                      candidate_ceiling, top_rank_gap, regressed_vs_baseline |
|          scifact |  0.003350 |  0.004884 | 0.364083 | 0.405778 |      0.816 |                                                                             |

## Diagnostic Category Totals

- `bm25_supported_relevant_suppressed`: 1972
- `qrel_admitted_but_ranked_low`: 1409

## Interpretation

M346A remains the best completed broad8 variant, but the gap to candidate upper bound is still dominated by ranking quality. The main residual is not another binary admission loss; it is that many qrel positives already in the pool are not promoted high enough, especially on datasets with large top-rank gaps.
