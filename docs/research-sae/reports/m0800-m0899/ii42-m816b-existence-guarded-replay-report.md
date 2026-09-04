# M816B Existence-Guarded Pairwise Replay

M816B combines a query-level safe-existence model with the M815
pairwise top-bundle selector and top-bundle abstainer.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.1385 | 0 | 40 | +0.000104 | +0.000393 | +0.000041 | +0.000000 | +0.000022 | +0.000000 | +0.000516 | 0.211 | 0.500 |
| seed7642 | 0.2547 | 0 | 16 | +0.000009 | +0.000053 | +0.000018 | +0.000000 | +0.000001 | +0.000000 | +0.000066 | 0.084 | 0.688 |
| seed7643 | 0.2390 | 0 | 22 | +0.000051 | -0.000026 | +0.000000 | +0.000000 | +0.000022 | +0.000000 | +0.000036 | 0.112 | 0.636 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | climate-fever | 5 | 0.400 | +0.001234 | +0.006169 |
| original | cqadupstack | 3 | 0.667 | +0.001280 | +0.003840 |
| original | dbpedia-entity | 6 | 0.667 | +0.007965 | +0.047792 |
| original | fever | 1 | 0.000 | +0.000000 | +0.000000 |
| original | fiqa | 3 | 0.333 | +0.000034 | +0.000103 |
| original | msmarco | 3 | 1.000 | +0.006548 | +0.019645 |
| original | nfcorpus | 4 | 0.750 | -0.009915 | -0.039660 |
| original | quora | 1 | 0.000 | +0.000000 | +0.000000 |
| original | scidocs | 2 | 1.000 | +0.052738 | +0.105477 |
| original | trec-covid | 8 | 0.375 | +0.000311 | +0.002490 |
| original | webis-touche2020 | 4 | 0.000 | -0.001522 | -0.006088 |
| seed7642 | climate-fever | 3 | 0.667 | +0.000191 | +0.000573 |
| seed7642 | dbpedia-entity | 3 | 0.667 | +0.004606 | +0.013817 |
| seed7642 | msmarco | 1 | 1.000 | +0.000534 | +0.000534 |
| seed7642 | nfcorpus | 2 | 1.000 | +0.001291 | +0.002583 |
| seed7642 | trec-covid | 4 | 0.500 | -0.000072 | -0.000288 |
| seed7642 | webis-touche2020 | 3 | 0.667 | +0.000251 | +0.000752 |
| seed7643 | climate-fever | 1 | 1.000 | +0.002924 | +0.002924 |
| seed7643 | cqadupstack | 1 | 1.000 | +0.000337 | +0.000337 |
| seed7643 | dbpedia-entity | 3 | 0.667 | +0.004170 | +0.012509 |
| seed7643 | fiqa | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | msmarco | 2 | 1.000 | +0.000592 | +0.001183 |
| seed7643 | nfcorpus | 6 | 0.333 | -0.002238 | -0.013431 |
| seed7643 | scidocs | 1 | 1.000 | +0.000413 | +0.000413 |
| seed7643 | trec-covid | 6 | 0.667 | +0.000661 | +0.003968 |
| seed7643 | webis-touche2020 | 1 | 1.000 | +0.001924 | +0.001924 |

## Held-out Oracle Threshold Ceiling

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Mean Utility |
| --- | ---: | ---: | ---: |
| original | 1 | 0.4626 | +0.000147 |
| seed7642 | 1 | 0.2945 | +0.000080 |
| seed7643 | 1 | 0.6531 | +0.000000 |

## Decision

M816B improves selector safety but remains partial. Keep the existence guard, then inspect remaining failed task/query modes.
