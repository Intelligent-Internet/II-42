# ii42 M1100 Semantic-Neighbor Posting Result

Verdict: `fail`

Result scope: `final`

## M1100 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 |
| atom_only | 0.202978 | 0.109502 | 0.102217 | 0.084136 |
| unified_scale_0.5 | 0.512057 | 0.392427 | 0.331480 | 0.261103 |
| unified_0.5_minus_lexical | 0.027516 | -0.000408 | 0.006244 | 0.003049 |
| atom_only_minus_lexical | -0.281562 | -0.283333 | -0.223019 | -0.173919 |

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
| atom_only | 0.013078 | 0.006161 | 0.006490 | 0.001004 |
| unified_scale_0.5 | 0.008168 | -0.013221 | -0.006147 | -0.009072 |

## Gate

| Check | Value |
| --- | ---: |
| `beats_lexical_recall` | `True` |
| `beats_lexical_map` | `True` |
| `no_mrr_regression_vs_lexical` | `False` |
| `no_ndcg_regression_vs_lexical` | `True` |
| `beats_m1050_metric_count` | `1` |
| `atom_only_improves_metric_count` | `4` |

## Per-Dataset M1100 Unified 0.5 Deltas Versus Lexical

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fiqa | 0.026432 | -0.012078 | -0.000487 | -0.004634 |
| nfcorpus | 0.013970 | 0.006916 | 0.004017 | 0.004760 |
| scifact | 0.044444 | 0.016911 | 0.023181 | 0.017803 |
