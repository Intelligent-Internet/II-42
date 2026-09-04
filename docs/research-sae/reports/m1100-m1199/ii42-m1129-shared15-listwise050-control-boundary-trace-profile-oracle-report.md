# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.790912 | 0.757817 | 0.683758 | 0.598094 |
| `alternate` | 0.790550 | 0.726437 | 0.656471 | 0.573328 |
| `oracle` | 0.794473 | 0.771216 | 0.691661 | 0.609521 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 403.000000 |
| `alt_win_rate` | 0.156328 |
| `positive_top100_delta_rate` | 0.052109 |
| `negative_top100_delta_rate` | 0.049628 |
| `alt_win_with_recall_gain_rate` | 0.042184 |
| `alt_win_rank_only_rate` | 0.114144 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 3118 |
| `base_top_relevant` | 3074 |
| `negative_demotions` | 3954 |
| `negative_promotions` | 3910 |
| `positive_demotions` | 153 |
| `positive_promotions` | 197 |
| `positive_rank_improvements` | 2840 |
| `positive_rank_worsenings` | 3612 |
| `top100_relevant_delta` | 44 |
| `top10_relevant_delta` | -41 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.685294 |
| `union_top_score_delta_std` | 0.649393 |
| `base_alt_cutoff_delta` | 0.616480 |
| `demoted_margin_mean` | 0.602148 |
| `top100_churn` | 0.548739 |
| `top100_jaccard` | 0.545868 |
| `union_top_score_delta_mean` | 0.524603 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.856559 |
| `positive_rank_delta_std` | 0.775584 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.006771 | -0.001090 | -0.004784 | 0.133333 | 0.000000 |
| `climate-fever` | 30 | +0.002778 | -0.084786 | -0.024197 | -0.033979 | 0.200000 | 0.033333 |
| `cqadupstack` | 30 | -0.000936 | +0.003386 | -0.006123 | -0.006892 | 0.133333 | 0.033333 |
| `dbpedia-entity` | 30 | -0.005089 | +0.016667 | -0.036685 | -0.037560 | 0.200000 | 0.000000 |
| `fever` | 30 | +0.000000 | -0.044444 | -0.033333 | -0.044444 | 0.000000 | 0.000000 |
| `fiqa` | 30 | -0.016667 | -0.078889 | -0.047780 | -0.058857 | 0.200000 | 0.033333 |
| `hotpotqa` | 30 | +0.000000 | +0.000000 | -0.014408 | -0.015621 | 0.066667 | 0.000000 |
| `msmarco` | 13 | +0.009960 | +0.000000 | -0.011674 | +0.003992 | 0.230769 | 0.153846 |
| `nfcorpus` | 30 | -0.000019 | -0.081209 | -0.035312 | -0.017869 | 0.100000 | 0.100000 |
| `nq` | 30 | +0.000000 | -0.028206 | -0.031674 | -0.044308 | 0.033333 | 0.000000 |
| `quora` | 30 | +0.000000 | +0.035238 | +0.008910 | +0.010308 | 0.100000 | 0.000000 |
| `scidocs` | 30 | +0.000000 | -0.046611 | -0.046195 | -0.023275 | 0.266667 | 0.000000 |
| `scifact` | 30 | +0.000000 | -0.082024 | -0.069190 | -0.069609 | 0.066667 | 0.000000 |
| `trec-covid` | 15 | +0.003606 | -0.018889 | -0.030917 | +0.023577 | 0.533333 | 0.600000 |
| `webis-touche2020` | 15 | +0.017893 | -0.028889 | -0.017942 | +0.001371 | 0.466667 | 0.266667 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
