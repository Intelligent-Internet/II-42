# ii42 M1114 Boundary Trace Profile Oracle Audit

Verdict: `stop_selector_interface`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.686834 | 0.616704 | 0.534653 | 0.411558 |
| `alternate` | 0.692879 | 0.598705 | 0.513353 | 0.403228 |
| `oracle` | 0.694470 | 0.640724 | 0.551001 | 0.433207 |

## Heldout Movement Rates

| Metric | Value |
| --- | ---: |
| `query_count` | 210.000000 |
| `alt_win_rate` | 0.242857 |
| `positive_top100_delta_rate` | 0.119048 |
| `negative_top100_delta_rate` | 0.052381 |
| `alt_win_with_recall_gain_rate` | 0.100000 |
| `alt_win_rank_only_rate` | 0.142857 |

## Heldout Movement Sum

| Metric | Value |
| --- | ---: |
| `alt_top_relevant` | 1472 |
| `base_top_relevant` | 1421 |
| `negative_demotions` | 2417 |
| `negative_promotions` | 2366 |
| `positive_demotions` | 94 |
| `positive_promotions` | 145 |
| `positive_rank_improvements` | 1907 |
| `positive_rank_worsenings` | 2387 |
| `top100_relevant_delta` | 51 |
| `top10_relevant_delta` | -19 |

## Deployable Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `union_top_score_delta_std` | 0.603897 |
| `promoted_margin_mean` | 0.580404 |
| `base_alt_cutoff_delta` | 0.563325 |
| `demoted_margin_mean` | 0.548773 |
| `union_top_score_delta_mean` | 0.541004 |
| `top100_churn` | 0.512209 |
| `top100_jaccard` | 0.506043 |

## Label-Derived Trace Feature AUC

| Feature | AUC |
| --- | ---: |
| `positive_rank_delta_mean` | 0.831669 |
| `positive_rank_delta_std` | 0.713651 |

## Per Dataset Heldout Summary

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Win Rate | Pos Top100 Delta Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.001875 | -0.010034 | -0.002734 | 0.100000 | 0.000000 |
| `cqadupstack` | 30 | -0.003714 | -0.005859 | -0.038269 | -0.011194 | 0.066667 | 0.000000 |
| `fiqa` | 30 | +0.004762 | -0.009150 | +0.003772 | -0.005435 | 0.300000 | 0.033333 |
| `nfcorpus` | 30 | -0.001329 | -0.052900 | -0.020133 | -0.005032 | 0.266667 | 0.166667 |
| `scidocs` | 30 | +0.018333 | +0.005519 | -0.002867 | -0.007675 | 0.300000 | 0.166667 |
| `scifact` | 30 | +0.000000 | -0.033954 | -0.056529 | -0.046412 | 0.133333 | 0.000000 |
| `trec-covid` | 15 | +0.004126 | -0.033333 | -0.022920 | +0.013997 | 0.600000 | 0.466667 |
| `webis-touche2020` | 15 | +0.044396 | -0.022222 | -0.027169 | +0.026335 | 0.466667 | 0.466667 |

## Interpretation

The oracle remains large, but deployable boundary trace features do not separate alternate wins strongly enough. More selector micro-tuning on the same observable interface is not justified.
