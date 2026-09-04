# M816B Existence-Guarded Pairwise Replay

M816B combines a query-level safe-existence model with the M815
pairwise top-bundle selector and top-bundle abstainer.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.6000 | 1 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000 | 0.000 |
| seed7642 | 0.6000 | 1 | 1 | +0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000001 | 0.005 | 1.000 |
| seed7643 | 0.6000 | 1 | 3 | +0.000001 | +0.000005 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000006 | 0.015 | 1.000 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| seed7642 | climate-fever | 1 | 1.000 | +0.000198 | +0.000198 |
| seed7643 | nfcorpus | 1 | 1.000 | +0.000007 | +0.000007 |
| seed7643 | trec-covid | 2 | 1.000 | +0.000768 | +0.001536 |

## Held-out Oracle Threshold Ceiling

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Mean Utility |
| --- | ---: | ---: | ---: |
| original | 1 | 0.4626 | +0.000147 |
| seed7642 | 1 | 0.2945 | +0.000080 |
| seed7643 | 1 | 0.6531 | +0.000000 |

## Decision

M816B improves selector safety but remains partial. Keep the existence guard, then inspect remaining failed task/query modes.
