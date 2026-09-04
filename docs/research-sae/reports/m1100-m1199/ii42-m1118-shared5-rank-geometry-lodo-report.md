# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.027407 | -0.021245 | -0.027682 | 0.500000 | 0.066667 |
| `fiqa` | 30 | +0.000000 | -0.002449 | -0.013874 | +0.000795 | 0.500000 | 0.400000 |
| `nfcorpus` | 30 | +0.000521 | -0.000859 | -0.001311 | -0.001679 | 0.366667 | 0.181818 |
| `scidocs` | 30 | -0.005000 | +0.027778 | +0.002976 | +0.002155 | 0.300000 | 0.333333 |
| `scifact` | 30 | +0.000000 | -0.053942 | -0.051204 | -0.052591 | 0.533333 | 0.062500 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.000896 | -0.011376 | -0.016931 | -0.015800 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
