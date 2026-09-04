# M816B Existence-Guarded Pairwise Replay

M816B combines a query-level safe-existence model with the M815
pairwise top-bundle selector and top-bundle abstainer.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.5000 | 1 | 3 | +0.000004 | +0.000069 | +0.000000 | +0.000000 | +0.000006 | +0.000000 | +0.000076 | 0.016 | 1.000 |
| seed7642 | 0.5000 | 1 | 3 | +0.000012 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000012 | 0.016 | 1.000 |
| seed7643 | 0.5000 | 0 | 8 | +0.000006 | +0.000005 | +0.000000 | +0.000000 | -0.000008 | +0.000000 | +0.000007 | 0.041 | 0.875 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | msmarco | 2 | 1.000 | +0.009803 | +0.019606 |
| original | trec-covid | 1 | 1.000 | +0.000865 | +0.000865 |
| seed7642 | climate-fever | 2 | 1.000 | +0.000286 | +0.000573 |
| seed7642 | webis-touche2020 | 1 | 1.000 | +0.002568 | +0.002568 |
| seed7643 | msmarco | 2 | 1.000 | +0.000592 | +0.001183 |
| seed7643 | nfcorpus | 3 | 0.667 | -0.000304 | -0.000912 |
| seed7643 | trec-covid | 3 | 1.000 | +0.000534 | +0.001603 |

## Held-out Oracle Threshold Ceiling

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Mean Utility |
| --- | ---: | ---: | ---: |
| original | 1 | 0.4626 | +0.000147 |
| seed7642 | 1 | 0.2945 | +0.000080 |
| seed7643 | 1 | 0.6531 | +0.000000 |

## Decision

M816B improves selector safety but remains partial. Keep the existence guard, then inspect remaining failed task/query modes.
