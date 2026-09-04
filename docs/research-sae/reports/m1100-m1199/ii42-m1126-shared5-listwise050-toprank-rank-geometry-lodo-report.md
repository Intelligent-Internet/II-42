# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.032176 | -0.032781 | -0.030661 | 0.666667 | 0.050000 |
| `fiqa` | 30 | -0.011111 | -0.014716 | -0.015149 | -0.006349 | 0.400000 | 0.333333 |
| `nfcorpus` | 30 | +0.009871 | -0.011481 | -0.004862 | -0.006928 | 0.566667 | 0.529412 |
| `scidocs` | 30 | +0.013333 | -0.043280 | -0.025087 | -0.016705 | 0.366667 | 0.272727 |
| `scifact` | 30 | +0.000000 | -0.041349 | -0.039297 | -0.037671 | 0.633333 | 0.105263 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.002419 | -0.028601 | -0.023435 | -0.019663 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
