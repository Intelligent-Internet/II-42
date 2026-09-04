# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa` | 194 | +0.004639 | +0.000320 | -0.000966 | -0.000343 | 0.170103 | 0.121212 |
| `nfcorpus` | 97 | +0.000644 | +0.005948 | +0.000805 | -0.000667 | 0.020619 | 0.500000 |
| `scifact` | 90 | +0.022222 | +0.013619 | +0.006439 | +0.013314 | 0.244444 | 0.272727 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.009169 | +0.006629 | +0.002092 | +0.004101 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
