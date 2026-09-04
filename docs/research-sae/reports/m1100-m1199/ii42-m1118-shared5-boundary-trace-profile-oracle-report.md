# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.699701 | 0.544098 | 0.482291 | 0.382090 |
| `alternate` | 0.698991 | 0.515337 | 0.456286 | 0.357673 |
| `oracle` | 0.701472 | 0.564178 | 0.492709 | 0.393989 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.133333 |
| `positive_top100_delta_rate` | 0.020000 |
| `negative_top100_delta_rate` | 0.013333 |
| `alt_win_with_recall_gain_rate` | 0.013333 |
| `alt_win_rank_only_rate` | 0.120000 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 399 |
| `base_top_relevant` | 398 |
| `negative_demotions` | 1173 |
| `negative_promotions` | 1172 |
| `positive_demotions` | 3 |
| `positive_promotions` | 4 |
| `positive_rank_improvements` | 77 |
| `positive_rank_worsenings` | 266 |
| `top100_relevant_delta` | 1 |
| `top10_relevant_delta` | -8 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.670769 |
| `demoted_margin_mean` | 0.647308 |
| `base_alt_cutoff_delta` | 0.623846 |
| `top100_churn` | 0.620385 |
| `top100_jaccard` | 0.612885 |
| `union_top_score_delta_std` | 0.603846 |
| `union_top_score_delta_mean` | 0.544615 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.824231 |
| `positive_rank_delta_std` | 0.705577 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.004006 | -0.002466 | -0.004364 | 0.100000 | 0.000000 |
| `fiqa` | 30 | +0.000000 | -0.059671 | -0.048660 | -0.044289 | 0.200000 | 0.000000 |
| `nfcorpus` | 30 | +0.001447 | -0.033134 | -0.015781 | -0.007341 | 0.100000 | 0.066667 |
| `scidocs` | 30 | -0.005000 | +0.009725 | -0.009603 | -0.010721 | 0.233333 | 0.033333 |
| `scifact` | 30 | +0.000000 | -0.056720 | -0.053514 | -0.055369 | 0.033333 | 0.000000 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
