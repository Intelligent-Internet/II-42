# ii42 M1100 Semantic-Neighbor Posting Result

Verdict: `fail`

Result scope: `final`

## M1100 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 |
| atom_only | 0.188965 | 0.112393 | 0.097265 | 0.079317 |
| unified_scale_0.5 | 0.498775 | 0.397779 | 0.330974 | 0.258977 |
| unified_0.5_minus_lexical | 0.014234 | 0.004944 | 0.005738 | 0.000923 |
| atom_only_minus_lexical | -0.295576 | -0.280442 | -0.227971 | -0.178738 |

## M1050 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 |
| atom_only | 0.189900 | 0.103341 | 0.095727 | 0.083132 |
| unified_scale_0.5 | 0.503889 | 0.405647 | 0.337627 | 0.270175 |
| unified_0.5_minus_lexical | 0.019348 | 0.012812 | 0.012391 | 0.012121 |
| atom_only_minus_lexical | -0.294640 | -0.289494 | -0.229509 | -0.174922 |

## M1100 Minus M1050

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| atom_only | -0.000936 | 0.009052 | 0.001538 | -0.003815 |
| unified_scale_0.5 | -0.005115 | -0.007868 | -0.006653 | -0.011198 |

## Gate

| Check | Value |
| --- | ---: |
| `beats_lexical_recall` | `True` |
| `beats_lexical_map` | `True` |
| `no_mrr_regression_vs_lexical` | `True` |
| `no_ndcg_regression_vs_lexical` | `True` |
| `beats_m1050_metric_count` | `0` |
| `atom_only_improves_metric_count` | `2` |

## Per-Dataset M1100 Unified 0.5 Deltas Versus Lexical

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fiqa | 0.025344 | -0.000651 | -0.001394 | -0.006878 |
| nfcorpus | 0.005166 | 0.004696 | 0.002184 | 0.002942 |
| scifact | 0.000000 | 0.017297 | 0.024969 | 0.015598 |
