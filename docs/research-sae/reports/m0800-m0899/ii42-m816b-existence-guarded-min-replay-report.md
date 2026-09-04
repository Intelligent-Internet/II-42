# M816B Existence-Guarded Pairwise Replay

M816B combines a query-level safe-existence model with the M815
pairwise top-bundle selector and top-bundle abstainer.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.2944 | 0 | 38 | +0.000113 | +0.000393 | +0.000041 | +0.000000 | +0.000022 | +0.000000 | +0.000525 | 0.200 | 0.553 |
| seed7642 | 0.3142 | 0 | 24 | +0.000024 | +0.000053 | +0.000018 | +0.000000 | +0.000001 | +0.000000 | +0.000081 | 0.126 | 0.583 |
| seed7643 | 0.4166 | 0 | 20 | +0.000012 | -0.000261 | +0.000000 | +0.000000 | +0.000022 | +0.000000 | -0.000237 | 0.102 | 0.650 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | climate-fever | 3 | 0.667 | +0.002096 | +0.006288 |
| original | cqadupstack | 3 | 0.667 | +0.001280 | +0.003840 |
| original | dbpedia-entity | 5 | 0.800 | +0.009558 | +0.047792 |
| original | fever | 1 | 0.000 | +0.000000 | +0.000000 |
| original | fiqa | 3 | 0.333 | +0.000034 | +0.000103 |
| original | msmarco | 3 | 1.000 | +0.006548 | +0.019645 |
| original | nfcorpus | 6 | 0.667 | -0.006433 | -0.038600 |
| original | quora | 1 | 0.000 | +0.000000 | +0.000000 |
| original | scidocs | 2 | 1.000 | +0.052738 | +0.105477 |
| original | trec-covid | 8 | 0.375 | +0.000311 | +0.002490 |
| original | webis-touche2020 | 3 | 0.000 | -0.001608 | -0.004823 |
| seed7642 | climate-fever | 6 | 0.500 | +0.000801 | +0.004805 |
| seed7642 | dbpedia-entity | 4 | 0.750 | +0.003581 | +0.014323 |
| seed7642 | fiqa | 1 | 0.000 | -0.000313 | -0.000313 |
| seed7642 | msmarco | 1 | 1.000 | +0.000534 | +0.000534 |
| seed7642 | nfcorpus | 3 | 0.667 | +0.000806 | +0.002418 |
| seed7642 | trec-covid | 6 | 0.500 | -0.000097 | -0.000584 |
| seed7642 | webis-touche2020 | 3 | 0.667 | +0.000251 | +0.000752 |
| seed7643 | climate-fever | 1 | 1.000 | +0.002924 | +0.002924 |
| seed7643 | cqadupstack | 1 | 1.000 | +0.000337 | +0.000337 |
| seed7643 | dbpedia-entity | 2 | 0.500 | +0.001493 | +0.002987 |
| seed7643 | msmarco | 2 | 1.000 | +0.000592 | +0.001183 |
| seed7643 | nfcorpus | 6 | 0.333 | -0.013056 | -0.078333 |
| seed7643 | trec-covid | 6 | 0.667 | +0.000661 | +0.003968 |
| seed7643 | webis-touche2020 | 2 | 1.000 | +0.001299 | +0.002598 |

## Decision

M816B improves selector safety but remains partial. Keep the existence guard, then inspect remaining failed task/query modes.
