# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.024947 | -0.019376 | -0.024947 | 0.566667 | 0.058824 |
| `cqadupstack` | 30 | -0.003714 | -0.002525 | -0.021727 | -0.002495 | 0.600000 | 0.111111 |
| `fiqa` | 30 | +0.004762 | -0.033595 | -0.006996 | -0.020829 | 0.566667 | 0.294118 |
| `nfcorpus` | 30 | -0.001649 | -0.019567 | -0.010672 | -0.003273 | 0.766667 | 0.304348 |
| `scidocs` | 30 | +0.005000 | -0.019074 | -0.006481 | -0.002996 | 0.600000 | 0.277778 |
| `scifact` | 30 | +0.000000 | -0.019192 | -0.050770 | -0.039995 | 0.700000 | 0.142857 |
| `trec-covid` | 15 | +0.007555 | -0.033333 | -0.037079 | +0.017152 | 0.666667 | 0.700000 |
| `webis-touche2020` | 15 | +0.044396 | -0.022222 | -0.027169 | +0.026595 | 0.866667 | 0.538462 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.007044 | -0.021807 | -0.022534 | -0.006348 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
