# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.023119 | -0.025793 | -0.021969 | 0.366667 | 0.000000 |
| `fiqa` | 30 | +0.016667 | -0.012685 | +0.005182 | -0.006663 | 0.633333 | 0.421053 |
| `nfcorpus` | 30 | -0.003595 | -0.001852 | -0.009492 | -0.002428 | 0.200000 | 0.166667 |
| `scidocs` | 30 | -0.008333 | -0.024958 | -0.024054 | -0.015311 | 0.533333 | 0.062500 |
| `scifact` | 30 | +0.000000 | -0.059841 | -0.063865 | -0.061466 | 0.300000 | 0.000000 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.000948 | -0.024491 | -0.023604 | -0.021568 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
