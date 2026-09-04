# ii42 M1100 Semantic-Neighbor Posting Result

Verdict: `fail`

Result scope: `partial`

## M1100 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.521808 | 0.505866 | 0.429390 | 0.330480 |
| atom_only | 0.255611 | 0.197038 | 0.180415 | 0.148577 |
| unified_scale_0.5 | 0.550452 | 0.517595 | 0.442635 | 0.341521 |
| unified_0.5_minus_lexical | 0.028644 | 0.011729 | 0.013245 | 0.011041 |
| atom_only_minus_lexical | -0.266197 | -0.308828 | -0.248975 | -0.181903 |

## M1050 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.521808 | 0.505866 | 0.429390 | 0.330480 |
| atom_only | 0.262320 | 0.182430 | 0.169444 | 0.146708 |
| unified_scale_0.5 | 0.547163 | 0.526524 | 0.449334 | 0.353411 |
| unified_0.5_minus_lexical | 0.025355 | 0.020657 | 0.019944 | 0.022931 |
| atom_only_minus_lexical | -0.259488 | -0.323437 | -0.259945 | -0.183772 |

## M1100 Minus M1050

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| atom_only | -0.006709 | 0.014608 | 0.010970 | 0.001869 |
| unified_scale_0.5 | 0.003290 | -0.008928 | -0.006699 | -0.011890 |

## Gate

| Check | Value |
| --- | ---: |
| `beats_lexical_recall` | `True` |
| `beats_lexical_map` | `True` |
| `no_mrr_regression_vs_lexical` | `True` |
| `no_ndcg_regression_vs_lexical` | `True` |
| `beats_m1050_metric_count` | `1` |
| `atom_only_improves_metric_count` | `3` |

## Per-Dataset M1100 Unified 0.5 Deltas Versus Lexical

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 0.013970 | 0.006916 | 0.004017 | 0.004760 |
| scifact | 0.044444 | 0.016911 | 0.023181 | 0.017803 |
