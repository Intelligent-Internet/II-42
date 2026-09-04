# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.712690 | 0.543900 | 0.494837 | 0.391132 |
| `alternate` | 0.727767 | 0.519171 | 0.468535 | 0.368264 |
| `oracle` | 0.730476 | 0.564989 | 0.506067 | 0.403197 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.220000 |
| `positive_top100_delta_rate` | 0.053333 |
| `negative_top100_delta_rate` | 0.033333 |
| `alt_win_with_recall_gain_rate` | 0.046667 |
| `alt_win_rank_only_rate` | 0.173333 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 452 |
| `base_top_relevant` | 446 |
| `negative_demotions` | 1670 |
| `negative_promotions` | 1664 |
| `positive_demotions` | 8 |
| `positive_promotions` | 14 |
| `positive_rank_improvements` | 221 |
| `positive_rank_worsenings` | 396 |
| `top100_relevant_delta` | 6 |
| `top10_relevant_delta` | -9 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.624061 |
| `union_top_score_delta_std` | 0.558923 |
| `top100_churn` | 0.555556 |
| `top100_jaccard` | 0.555556 |
| `demoted_margin_mean` | 0.550894 |
| `union_top_score_delta_mean` | 0.529397 |
| `base_alt_cutoff_delta` | 0.519037 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.854701 |
| `positive_rank_delta_std` | 0.743849 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.057643 | -0.063091 | -0.057643 | 0.100000 | 0.000000 |
| `fiqa` | 30 | +0.078571 | -0.029592 | -0.027397 | -0.026391 | 0.233333 | 0.100000 |
| `nfcorpus` | 30 | +0.005145 | -0.036667 | -0.015153 | -0.004621 | 0.233333 | 0.166667 |
| `scidocs` | 30 | -0.008333 | +0.023354 | -0.001156 | -0.004792 | 0.433333 | 0.000000 |
| `scifact` | 30 | +0.000000 | -0.023095 | -0.024713 | -0.020896 | 0.100000 | 0.000000 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
