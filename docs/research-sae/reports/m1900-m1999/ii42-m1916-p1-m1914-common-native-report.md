# M1916 P1 versus M1914 Common Native Matrix

Decision: **retain_p1_first_stage_do_not_start_m1914_stage2**

All rows use the same PostgreSQL normalized-postings backend, native
qrels, candidate-k 1000, and no BM25 or dataset-specific tuning.
This is a four-row full-corpus comparison, not a complete BEIR15
generalization claim. Quality macro values are dataset-weighted;
latency percentiles are query-weighted.

## Per-dataset Quality

| Dataset | Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | p1 | 0.463089 | 0.401005 | 0.770400 | 0.552099 | 0.544830 | 0.922898 |
| fiqa | m1914 | 0.358606 | 0.300585 | 0.665214 | 0.444843 | 0.350833 | 0.860618 |
| arguana | p1 | 0.398682 | 0.276755 | 0.990007 | 0.274830 | 0.610043 | 0.999286 |
| arguana | m1914 | 0.404141 | 0.282061 | 0.988580 | 0.280458 | 0.535675 | 0.999286 |
| nfcorpus | p1 | 0.272408 | 0.118020 | 0.266389 | 0.465003 | 0.380743 | 0.558879 |
| nfcorpus | m1914 | 0.339391 | 0.164808 | 0.292702 | 0.559041 | 0.318266 | 0.587925 |
| scifact | p1 | 0.643097 | 0.614619 | 0.897667 | 0.623941 | 0.438000 | 0.986667 |
| scifact | m1914 | 0.708670 | 0.667947 | 0.954333 | 0.678083 | 0.401767 | 0.993333 |

## M1914 Minus P1

| Dataset | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | dO@100 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | -0.104482 | -0.100421 | -0.105186 | -0.107256 | -0.193997 | -0.062280 |
| arguana | +0.005459 | +0.005306 | -0.001428 | +0.005628 | -0.074368 | +0.000000 |
| nfcorpus | +0.066983 | +0.046788 | +0.026313 | +0.094038 | -0.062477 | +0.029046 |
| scifact | +0.065573 | +0.053328 | +0.056667 | +0.054141 | -0.036233 | +0.006667 |

## Macro And Cost

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB | Storage | p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| p1 | 0.444319 | 0.352600 | 0.731116 | 0.478968 | 0.493404 | 0.866933 | 737665024 | 681.584 ms |
| m1914 | 0.452702 | 0.353850 | 0.725207 | 0.490606 | 0.401635 | 0.860291 | 1076133888 | 89.735 ms |
| M1914-P1 | +0.008383 | +0.001250 | -0.005908 | +0.011638 | -0.091769 | -0.006642 | - | - |

- Storage ratio M1914/P1: `1.459`
- Indexed p95 ratio M1914/P1: `0.132`
- Severe row harms: `['fiqa']`
- M1914 existing-BMP maximum metric delta: `0`

## Gate

- candidate_upper_bound_floor: `False`
- map_floor: `True`
- mrr_floor: `True`
- ndcg_floor: `True`
- recall_floor: `False`
- m1914_reference_parity: `True`
- no_severe_row_harm: `False`
- normalized_storage_ratio_le_1p60: `True`
- indexed_p95_ratio_le_1p60: `True`

## Interpretation

P1 remains the frozen first-stage product baseline. M1914 is not
authorized as its replacement for second-stage training because
the candidate/Recall floors and row-safety gate fail. Its much
lower indexed latency and positive NFCorpus/Scifact rows remain
useful learned-sparse control evidence, not a promotion result.
