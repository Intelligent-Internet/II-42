# ii42 M1116 Rank-Geometry Leave-Dataset-Out Validation

Verdict: `fail_lodo_instability`

## LODO Selector Delta Versus Base

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 30 | +0.000000 | -0.046389 | -0.044717 | -0.046389 | 0.366667 | 0.000000 |
| `cqadupstack` | 30 | +0.032839 | -0.033333 | +0.000254 | +0.010997 | 0.466667 | 0.214286 |
| `fiqa` | 30 | +0.025000 | -0.036111 | -0.017822 | -0.034231 | 0.433333 | 0.461538 |
| `nfcorpus` | 30 | -0.000793 | -0.000097 | -0.001517 | +0.000185 | 0.333333 | 0.400000 |
| `scidocs` | 30 | +0.013333 | -0.050595 | -0.013960 | -0.008305 | 0.433333 | 0.307692 |
| `scifact` | 30 | +0.000000 | -0.025541 | -0.033889 | -0.032478 | 0.333333 | 0.100000 |
| `trec-covid` | 15 | +0.017075 | +0.000000 | -0.018067 | +0.052766 | 0.733333 | 0.727273 |
| `webis-touche2020` | 15 | +0.036292 | -0.057143 | -0.033045 | +0.010173 | 0.800000 | 0.583333 |

## Macro Delta

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.015468 | -0.031151 | -0.020346 | -0.005910 |

## Interpretation

The selector does not survive leave-dataset-out pressure cleanly. Treat M1115 as a local signal, not a scalable policy.
