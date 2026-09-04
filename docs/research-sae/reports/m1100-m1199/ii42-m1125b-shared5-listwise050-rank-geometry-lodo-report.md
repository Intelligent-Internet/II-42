# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.031045 | -0.024170 | -0.031045 | 0.633333 | 0.052632 |
| `fiqa` | 30 | +0.036111 | -0.038750 | -0.029094 | -0.035818 | 0.600000 | 0.333333 |
| `nfcorpus` | 30 | +0.001472 | -0.002249 | -0.003453 | -0.000666 | 0.166667 | 0.400000 |
| `scidocs` | 30 | -0.008333 | -0.037377 | -0.026149 | -0.020234 | 0.500000 | 0.133333 |
| `scifact` | 30 | +0.000000 | -0.029841 | -0.036329 | -0.029356 | 0.666667 | 0.100000 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.005850 | -0.027852 | -0.023839 | -0.023424 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
