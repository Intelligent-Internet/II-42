# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 381.000000 |
| `alt_win_rate` | 0.110236 |
| `positive_top100_delta_rate` | 0.023622 |
| `negative_top100_delta_rate` | 0.034121 |
| `alt_win_with_recall_gain_rate` | 0.018373 |
| `alt_win_rank_only_rate` | 0.091864 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 776 |
| `base_top_relevant` | 791 |
| `negative_demotions` | 3052 |
| `negative_promotions` | 3067 |
| `positive_demotions` | 24 |
| `positive_promotions` | 9 |
| `positive_rank_improvements` | 127 |
| `positive_rank_worsenings` | 497 |
| `top100_relevant_delta` | -15 |
| `top10_relevant_delta` | -11 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `union_top_score_delta_std` | 0.724119 |
| `demoted_margin_mean` | 0.719729 |
| `top100_churn` | 0.685279 |
| `top100_jaccard` | 0.676851 |
| `promoted_margin_mean` | 0.671653 |
| `base_alt_cutoff_delta` | 0.649810 |
| `union_top_score_delta_mean` | 0.552114 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.800042 |
| `positive_rank_delta_std` | 0.661645 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa` | 194 | +0.000630 | -0.001865 | -0.002168 | +0.000882 | 0.113402 | 0.025773 |
| `nfcorpus` | 97 | -0.002457 | +0.013025 | -0.000261 | -0.000253 | 0.082474 | 0.010309 |
| `scifact` | 90 | +0.033333 | -0.004617 | -0.005727 | -0.004584 | 0.133333 | 0.033333 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
