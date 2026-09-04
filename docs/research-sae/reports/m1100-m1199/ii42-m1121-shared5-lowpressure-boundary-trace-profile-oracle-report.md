# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `stop_selector_interface`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.715565 | 0.540206 | 0.485608 | 0.386185 |
| `alternate` | 0.711254 | 0.516566 | 0.458126 | 0.361981 |
| `oracle` | 0.720417 | 0.559975 | 0.500846 | 0.398561 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.160000 |
| `positive_top100_delta_rate` | 0.026667 |
| `negative_top100_delta_rate` | 0.033333 |
| `alt_win_with_recall_gain_rate` | 0.020000 |
| `alt_win_rank_only_rate` | 0.140000 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 401 |
| `base_top_relevant` | 405 |
| `negative_demotions` | 1257 |
| `negative_promotions` | 1261 |
| `positive_demotions` | 9 |
| `positive_promotions` | 5 |
| `positive_rank_improvements` | 89 |
| `positive_rank_worsenings` | 305 |
| `top100_relevant_delta` | -4 |
| `top10_relevant_delta` | -11 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.614087 |
| `union_top_score_delta_std` | 0.574405 |
| `top100_churn` | 0.572255 |
| `top100_jaccard` | 0.564484 |
| `demoted_margin_mean` | 0.548280 |
| `base_alt_cutoff_delta` | 0.534061 |
| `union_top_score_delta_mean` | 0.523810 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.887235 |
| `positive_rank_delta_std` | 0.642361 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | -0.033333 | -0.008820 | -0.023479 | -0.007497 | 0.100000 | 0.000000 |
| `fiqa` | 30 | +0.016667 | -0.013558 | +0.004484 | -0.007246 | 0.366667 | 0.033333 |
| `nfcorpus` | 30 | -0.003218 | -0.017130 | -0.016158 | -0.006203 | 0.100000 | 0.066667 |
| `scidocs` | 30 | -0.001667 | -0.000513 | -0.024628 | -0.020274 | 0.233333 | 0.033333 |
| `scifact` | 30 | +0.000000 | -0.078175 | -0.077628 | -0.079799 | 0.000000 | 0.000000 |

## Interpretation

The oracle remains large, but deployable boundary trace features do not separate alternate wins strongly enough. More selector micro-tuning on the same observable interface is not justified.
