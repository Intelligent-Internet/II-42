# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `stop_selector_interface`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.721448 | 0.577975 | 0.510147 | 0.404917 |
| `alternate` | 0.738764 | 0.529030 | 0.477573 | 0.372550 |
| `oracle` | 0.740375 | 0.593621 | 0.519713 | 0.414634 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.226667 |
| `positive_top100_delta_rate` | 0.073333 |
| `negative_top100_delta_rate` | 0.026667 |
| `alt_win_with_recall_gain_rate` | 0.060000 |
| `alt_win_rank_only_rate` | 0.166667 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 444 |
| `base_top_relevant` | 437 |
| `negative_demotions` | 1762 |
| `negative_promotions` | 1755 |
| `positive_demotions` | 9 |
| `positive_promotions` | 16 |
| `positive_rank_improvements` | 246 |
| `positive_rank_worsenings` | 403 |
| `top100_relevant_delta` | 7 |
| `top10_relevant_delta` | -6 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `union_top_score_delta_std` | 0.586460 |
| `promoted_margin_mean` | 0.562880 |
| `demoted_margin_mean` | 0.549442 |
| `union_top_score_delta_mean` | 0.538540 |
| `base_alt_cutoff_delta` | 0.537525 |
| `top100_jaccard` | 0.513565 |
| `top100_churn` | 0.505325 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.757987 |
| `positive_rank_delta_std` | 0.698403 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.034160 | -0.034501 | -0.032645 | 0.066667 | 0.000000 |
| `fiqa` | 30 | +0.055556 | -0.028727 | -0.029471 | -0.029542 | 0.300000 | 0.066667 |
| `nfcorpus` | 30 | +0.011024 | -0.048639 | -0.012232 | -0.009831 | 0.366667 | 0.200000 |
| `scidocs` | 30 | +0.020000 | -0.058519 | -0.033674 | -0.028468 | 0.333333 | 0.100000 |
| `scifact` | 30 | +0.000000 | -0.074683 | -0.052990 | -0.061348 | 0.066667 | 0.000000 |

## Interpretation

The oracle remains large, but deployable boundary trace features do not separate alternate wins strongly enough. More selector micro-tuning on the same observable interface is not justified.
