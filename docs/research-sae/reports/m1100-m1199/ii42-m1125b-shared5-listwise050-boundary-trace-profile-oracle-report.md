# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `proceed_rank_geometry_audit`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.715489 | 0.562618 | 0.498005 | 0.391700 |
| `alternate` | 0.725439 | 0.521758 | 0.470026 | 0.362709 |
| `oracle` | 0.725143 | 0.569454 | 0.504507 | 0.397966 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 150.000000 |
| `alt_win_rate` | 0.166667 |
| `positive_top100_delta_rate` | 0.066667 |
| `negative_top100_delta_rate` | 0.040000 |
| `alt_win_with_recall_gain_rate` | 0.040000 |
| `alt_win_rank_only_rate` | 0.126667 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 424 |
| `base_top_relevant` | 417 |
| `negative_demotions` | 1779 |
| `negative_promotions` | 1772 |
| `positive_demotions` | 9 |
| `positive_promotions` | 16 |
| `positive_rank_improvements` | 237 |
| `positive_rank_worsenings` | 390 |
| `top100_relevant_delta` | 7 |
| `top10_relevant_delta` | -6 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `promoted_margin_mean` | 0.671520 |
| `union_top_score_delta_std` | 0.615360 |
| `demoted_margin_mean` | 0.611520 |
| `base_alt_cutoff_delta` | 0.553280 |
| `union_top_score_delta_mean` | 0.528000 |
| `top100_churn` | 0.515200 |
| `top100_jaccard` | 0.515200 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.856960 |
| `positive_rank_delta_std` | 0.666240 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.022434 | -0.017392 | -0.022434 | 0.066667 | 0.000000 |
| `fiqa` | 30 | +0.036111 | -0.041528 | -0.029206 | -0.036440 | 0.233333 | 0.100000 |
| `nfcorpus` | 30 | +0.001972 | -0.055001 | -0.016730 | -0.007544 | 0.200000 | 0.133333 |
| `scidocs` | 30 | +0.011667 | -0.038830 | -0.027933 | -0.032515 | 0.266667 | 0.100000 |
| `scifact` | 30 | +0.000000 | -0.046508 | -0.048631 | -0.046023 | 0.066667 | 0.000000 |

## Interpretation

The oracle is mostly rank-quality movement rather than simple top100 relevant promotion. A deployable next step must model query-local rank geometry, not just Recall@100 boundary crossing.
