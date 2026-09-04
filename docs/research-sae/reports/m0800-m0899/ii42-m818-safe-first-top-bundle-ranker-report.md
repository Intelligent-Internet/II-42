# M818 Safe-First Top-Bundle Ranker

M818 changes the top-bundle teacher: safe-positive bundles must
outrank unsafe or neutral bundles.  This tests whether harmful
top-bundle selection can be fixed at the ranker objective level.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.9015 | 1 | 1 | +0.000005 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000005 | 1.000 |
| seed7642 | 0.8642 | 0 | 8 | +0.000008 | +0.000000 | +0.000000 | +0.000000 | -0.000014 | +0.000000 | +0.000001 | 0.250 |
| seed7643 | 0.9011 | 1 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000 |

## Held-out Oracle Threshold Ceiling

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Mean Utility |
| --- | ---: | ---: | ---: |
| original | 1 | 0.8871 | +0.000005 |
| seed7642 | 1 | 0.9467 | +0.000000 |
| seed7643 | 0 | 0.6965 | +0.000390 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | dbpedia-entity | 1 | 1.000 | +0.001474 | +0.001474 |
| seed7642 | climate-fever | 1 | 1.000 | +0.000198 | +0.000198 |
| seed7642 | cqadupstack | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7642 | fever | 2 | 0.000 | +0.000000 | +0.000000 |
| seed7642 | nfcorpus | 1 | 0.000 | -0.000165 | -0.000165 |
| seed7642 | quora | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7642 | trec-covid | 1 | 0.000 | -0.002205 | -0.002205 |
| seed7642 | webis-touche2020 | 1 | 1.000 | +0.002568 | +0.002568 |

## Decision

M818 produced partial positive replay but did not pass the common clean gate. Inspect whether safe-first ranking improves the nfcorpus failure before scaling.
