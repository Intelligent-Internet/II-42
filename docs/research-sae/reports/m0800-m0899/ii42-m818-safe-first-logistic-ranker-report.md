# M818 Safe-First Top-Bundle Ranker

M818 changes the top-bundle teacher: safe-positive bundles must
outrank unsafe or neutral bundles.  This tests whether harmful
top-bundle selection can be fixed at the ranker objective level.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.9182 | 0 | 2 | +0.000007 | -0.000022 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.000014 | 0.500 |
| seed7642 | 0.9556 | 1 | 1 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000 |
| seed7643 | 0.8432 | 0 | 13 | -0.000000 | +0.000235 | +0.000000 | +0.000000 | +0.000129 | +0.000000 | +0.000299 | 0.308 |

## Held-out Oracle Threshold Ceiling

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Mean Utility |
| --- | ---: | ---: | ---: |
| original | 0 | 0.9404 | -0.000022 |
| seed7642 | 1 | 0.9557 | +0.000000 |
| seed7643 | 1 | 0.8852 | +0.000308 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | dbpedia-entity | 2 | 0.500 | -0.001959 | -0.003919 |
| seed7642 | fever | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | climate-fever | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | cqadupstack | 3 | 0.667 | -0.000311 | -0.000933 |
| seed7643 | msmarco | 2 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | nfcorpus | 1 | 1.000 | +0.082284 | +0.082284 |
| seed7643 | quora | 3 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | trec-covid | 1 | 1.000 | +0.000871 | +0.000871 |
| seed7643 | webis-touche2020 | 2 | 0.000 | -0.000633 | -0.001265 |

## Decision

M818 produced partial positive replay but did not pass the common clean gate. Inspect whether safe-first ranking improves the nfcorpus failure before scaling.
