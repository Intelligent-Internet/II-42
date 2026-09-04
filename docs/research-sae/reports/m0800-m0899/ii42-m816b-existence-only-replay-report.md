# M816B Existence-Guarded Pairwise Replay

M816B combines a query-level safe-existence model with the M815
pairwise top-bundle selector and top-bundle abstainer.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.2538 | 0 | 57 | +0.000170 | +0.000435 | +0.000041 | +0.000000 | +0.000022 | +0.000000 | +0.000624 | 0.300 | 0.421 |
| seed7642 | 0.8584 | 1 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000 | 0.000 |
| seed7643 | 0.8567 | 1 | 2 | +0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000001 | 0.010 | 1.000 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | climate-fever | 8 | 0.250 | +0.000613 | +0.004907 |
| original | cqadupstack | 6 | 0.500 | +0.000590 | +0.003537 |
| original | dbpedia-entity | 10 | 0.600 | +0.007767 | +0.077673 |
| original | fever | 2 | 0.000 | +0.000000 | +0.000000 |
| original | fiqa | 5 | 0.200 | +0.000021 | +0.000103 |
| original | msmarco | 3 | 1.000 | +0.006548 | +0.019645 |
| original | nfcorpus | 6 | 0.667 | -0.006433 | -0.038600 |
| original | quora | 2 | 0.000 | +0.000000 | +0.000000 |
| original | scidocs | 2 | 1.000 | +0.052738 | +0.105477 |
| original | scifact | 1 | 0.000 | +0.000000 | +0.000000 |
| original | trec-covid | 8 | 0.375 | +0.000311 | +0.002490 |
| original | webis-touche2020 | 4 | 0.000 | -0.001522 | -0.006088 |
| seed7643 | trec-covid | 2 | 1.000 | +0.000069 | +0.000138 |

## Decision

M816B improves selector safety but remains partial. Keep the existence guard, then inspect remaining failed task/query modes.
