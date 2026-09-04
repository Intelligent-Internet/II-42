# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.020833 | -0.024466 | -0.019481 | 0.533333 | 0.125000 |
| `fiqa` | 30 | +0.000000 | -0.009952 | -0.011140 | -0.008775 | 0.433333 | 0.153846 |
| `nfcorpus` | 30 | +0.006027 | -0.022870 | -0.009872 | -0.003997 | 0.400000 | 0.250000 |
| `scidocs` | 30 | -0.008333 | -0.021602 | -0.009529 | -0.007618 | 0.466667 | 0.357143 |
| `scifact` | 30 | +0.000000 | -0.069352 | -0.071474 | -0.068107 | 0.333333 | 0.000000 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.000461 | -0.028922 | -0.025296 | -0.021596 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
