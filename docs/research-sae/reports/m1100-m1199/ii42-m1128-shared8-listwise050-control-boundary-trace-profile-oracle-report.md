# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.679811 | 0.641111 | 0.552024 | 0.431187 |
| `alternate` | 0.699476 | 0.593659 | 0.519704 | 0.406667 |
| `oracle` | 0.697786 | 0.657343 | 0.564586 | 0.448447 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 210.000000 |
| `alt_win_rate` | 0.223810 |
| `positive_top100_delta_rate` | 0.123810 |
| `negative_top100_delta_rate` | 0.052381 |
| `alt_win_with_recall_gain_rate` | 0.100000 |
| `alt_win_rank_only_rate` | 0.123810 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 1558 |
| `base_top_relevant` | 1479 |
| `negative_demotions` | 2444 |
| `negative_promotions` | 2365 |
| `positive_demotions` | 114 |
| `positive_promotions` | 193 |
| `positive_rank_improvements` | 2109 |
| `positive_rank_worsenings` | 2385 |
| `top100_relevant_delta` | 79 |
| `top10_relevant_delta` | -15 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `union_top_score_delta_std` | 0.683592 |
| `promoted_margin_mean` | 0.682809 |
| `union_top_score_delta_mean` | 0.609581 |
| `demoted_margin_mean` | 0.599595 |
| `base_alt_cutoff_delta` | 0.583736 |
| `top100_churn` | 0.580081 |
| `top100_jaccard` | 0.574142 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.847083 |
| `positive_rank_delta_std` | 0.791672 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.060628 | -0.055714 | -0.060628 | 0.000000 | 0.000000 |
| `cqadupstack` | 30 | +0.066172 | -0.034007 | -0.010351 | +0.007712 | 0.133333 | 0.166667 |
| `fiqa` | 30 | +0.036111 | -0.084392 | -0.046309 | -0.064329 | 0.266667 | 0.100000 |
| `nfcorpus` | 30 | -0.003734 | -0.033431 | -0.013937 | -0.013380 | 0.200000 | 0.066667 |
| `scidocs` | 30 | +0.013333 | -0.050635 | -0.026394 | -0.018193 | 0.266667 | 0.066667 |
| `scifact` | 30 | +0.000000 | -0.040501 | -0.045679 | -0.048827 | 0.100000 | 0.000000 |
| `trec-covid` | 15 | +0.015257 | +0.000000 | -0.022661 | +0.041933 | 0.666667 | 0.600000 |
| `webis-touche2020` | 15 | +0.036292 | -0.057143 | -0.033045 | +0.010075 | 0.533333 | 0.333333 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
