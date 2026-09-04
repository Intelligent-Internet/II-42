# M815B Pairwise Winner Selector Replay

M815B trains a pairwise oracle-winner model leave-surface-out,
chooses one global threshold on training surfaces, and replays the
selected generated bundles through the native evaluator.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.9766 | 0 | 11 | -0.000088 | -0.000100 | -0.000308 | +0.000000 | +0.000006 | +0.000000 | -0.000247 | 0.058 | 0.182 |
| seed7642 | 0.9724 | 1 | 3 | +0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000001 | 0.016 | 0.333 |
| seed7643 | 0.9552 | 1 | 6 | +0.000016 | +0.000015 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000031 | 0.031 | 0.167 |

## Threshold Training Summary

| Held-out | Train Clean Threshold | Train Mean Utility | Train Sel Rate | Train Safe Rate | Train Oracle Hit |
| --- | ---: | ---: | ---: | ---: | ---: |
| original | 0 | +0.000023 | 0.049 | 0.368 | 0.189 |
| seed7642 | 1 | +0.000002 | 0.020 | 0.375 | 0.178 |
| seed7643 | 1 | +0.000002 | 0.020 | 0.125 | 0.160 |

## Decision

M815B recovered positive utility on some held-out surfaces but did not pass the common clean replay gate. Add calibrated abstention or richer oracle-explanation features before scaling.
