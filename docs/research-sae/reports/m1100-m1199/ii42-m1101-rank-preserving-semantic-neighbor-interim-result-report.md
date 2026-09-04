# ii42 M1100 Semantic-Neighbor Posting Result

Verdict: `fail`

Result scope: `partial`

## M1100 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.521808 | 0.505866 | 0.429390 | 0.330480 |
| atom_only | 0.260634 | 0.195435 | 0.168678 | 0.134189 |
| unified_scale_0.5 | 0.524486 | 0.516631 | 0.442546 | 0.339516 |
| unified_0.5_minus_lexical | 0.002678 | 0.010764 | 0.013156 | 0.009036 |
| atom_only_minus_lexical | -0.261173 | -0.310431 | -0.260712 | -0.196291 |

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
| atom_only | -0.001685 | 0.013005 | -0.000767 | -0.012519 |
| unified_scale_0.5 | -0.022676 | -0.009893 | -0.006788 | -0.013894 |

## Gate

| Check | Value |
| --- | ---: |
| `beats_lexical_recall` | `True` |
| `beats_lexical_map` | `True` |
| `no_mrr_regression_vs_lexical` | `True` |
| `no_ndcg_regression_vs_lexical` | `True` |
| `beats_m1050_metric_count` | `0` |
| `atom_only_improves_metric_count` | `1` |

## Per-Dataset M1100 Unified 0.5 Deltas Versus Lexical

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 0.005166 | 0.004696 | 0.002184 | 0.002942 |
| scifact | 0.000000 | 0.017297 | 0.024969 | 0.015598 |
