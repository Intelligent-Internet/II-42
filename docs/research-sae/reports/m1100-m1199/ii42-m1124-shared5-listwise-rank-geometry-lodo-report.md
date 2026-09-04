# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.051348 | -0.056768 | -0.051348 | 0.566667 | 0.058824 |
| `fiqa` | 30 | +0.045238 | -0.024854 | -0.021819 | -0.017319 | 0.600000 | 0.277778 |
| `nfcorpus` | 30 | +0.003613 | +0.002222 | -0.001702 | -0.001722 | 0.633333 | 0.315789 |
| `scidocs` | 30 | -0.008333 | -0.001831 | -0.009955 | -0.007734 | 0.533333 | 0.375000 |
| `scifact` | 30 | +0.000000 | -0.023095 | -0.031161 | -0.022285 | 0.766667 | 0.086957 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.008103 | -0.019781 | -0.024281 | -0.020081 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
