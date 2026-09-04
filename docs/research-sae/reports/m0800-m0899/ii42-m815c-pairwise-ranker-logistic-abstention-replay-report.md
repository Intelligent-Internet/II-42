# M815C Pairwise Ranker Abstention Replay

M815C keeps the M815 pairwise ranker but adds a second global
abstention classifier trained on whether the selected top bundle is
safe-positive under native replay labels.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.9504 | 0 | 6 | +0.000019 | +0.000035 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000054 | 0.032 | 0.833 |
| seed7642 | 0.9192 | 0 | 3 | -0.000023 | +0.000000 | +0.000000 | +0.000000 | +0.000008 | +0.000000 | -0.000018 | 0.016 | 0.667 |
| seed7643 | 0.9290 | 0 | 6 | +0.000021 | +0.000000 | +0.000000 | +0.000000 | -0.000008 | +0.000000 | +0.000017 | 0.031 | 0.833 |

## Train Abstention Summary

| Held-out | Clean Threshold | Train Mean Utility | Train Sel Rate | Train Safe Rate |
| --- | ---: | ---: | ---: | ---: |
| original | 1 | +0.000046 | 0.028 | 0.909 |
| seed7642 | 1 | +0.000056 | 0.005 | 1.000 |
| seed7643 | 1 | +0.000086 | 0.028 | 0.818 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | dbpedia-entity | 2 | 1.000 | +0.009026 | +0.018053 |
| original | nfcorpus | 1 | 1.000 | +0.001352 | +0.001352 |
| original | trec-covid | 2 | 1.000 | +0.000059 | +0.000118 |
| original | webis-touche2020 | 1 | 0.000 | -0.004823 | -0.004823 |
| seed7642 | climate-fever | 1 | 0.000 | -0.007576 | -0.007576 |
| seed7642 | nfcorpus | 1 | 1.000 | +0.001352 | +0.001352 |
| seed7642 | trec-covid | 1 | 1.000 | +0.001236 | +0.001236 |
| seed7643 | cqadupstack | 1 | 1.000 | +0.003772 | +0.003772 |
| seed7643 | nfcorpus | 3 | 0.667 | +0.000224 | +0.000672 |
| seed7643 | trec-covid | 1 | 1.000 | +0.000067 | +0.000067 |
| seed7643 | webis-touche2020 | 1 | 1.000 | +0.000225 | +0.000225 |

## Decision

M815C improves abstention but still has only partial held-out replay gains. Inspect failed surfaces before scaling.
