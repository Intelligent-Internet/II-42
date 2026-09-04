# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.693647 | 0.537218 | 0.479138 | 0.375236 |
| `alternate` | 0.694788 | 0.506047 | 0.451343 | 0.353098 |
| `oracle` | 0.698005 | 0.558551 | 0.490956 | 0.390299 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.166667 |
| `positive_top100_delta_rate` | 0.040000 |
| `negative_top100_delta_rate` | 0.026667 |
| `alt_win_with_recall_gain_rate` | 0.033333 |
| `alt_win_rank_only_rate` | 0.133333 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 415 |
| `base_top_relevant` | 405 |
| `negative_demotions` | 1422 |
| `negative_promotions` | 1412 |
| `positive_demotions` | 5 |
| `positive_promotions` | 15 |
| `positive_rank_improvements` | 140 |
| `positive_rank_worsenings` | 345 |
| `top100_relevant_delta` | 10 |
| `top10_relevant_delta` | -8 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.670400 |
| `union_top_score_delta_std` | 0.664320 |
| `demoted_margin_mean` | 0.631520 |
| `base_alt_cutoff_delta` | 0.623040 |
| `top100_churn` | 0.584960 |
| `top100_jaccard` | 0.584960 |
| `union_top_score_delta_mean` | 0.535680 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.786240 |
| `positive_rank_delta_std` | 0.709760 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.026155 | -0.035123 | -0.023216 | 0.100000 | 0.000000 |
| `fiqa` | 30 | +0.000000 | -0.004817 | -0.007741 | -0.003950 | 0.200000 | 0.000000 |
| `nfcorpus` | 30 | +0.007370 | -0.039907 | -0.012866 | -0.005295 | 0.200000 | 0.133333 |
| `scidocs` | 30 | -0.001667 | -0.022567 | -0.017490 | -0.017065 | 0.266667 | 0.066667 |
| `scifact` | 30 | +0.000000 | -0.062407 | -0.065752 | -0.061163 | 0.066667 | 0.000000 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
