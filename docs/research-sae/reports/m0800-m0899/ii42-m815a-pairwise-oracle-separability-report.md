# M815A Pairwise Oracle Separability

M815A tests whether M813 safe-oracle winners are separable from
rejected generated bundles using only inference-available features.

## Leave-Surface-Out Results

| Held-out | Model | Pair AUC | Winner Recall | Random Top1 | Safe Pred Rate | Pred Utility | Oracle Utility | Train Pairs | Test Pairs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | logistic | 0.8045 | 0.4634 | 0.1576 | 0.7073 | +0.008164 | +0.011867 | 1138 | 576 |
| original | hgb | 0.8854 | 0.6585 | 0.1576 | 0.8293 | +0.011246 | +0.011867 | 1138 | 576 |
| seed7642 | logistic | 0.8860 | 0.4490 | 0.1463 | 0.5714 | +0.005081 | +0.006324 | 1186 | 754 |
| seed7642 | hgb | 0.9237 | 0.5306 | 0.1463 | 0.6531 | +0.003366 | +0.006324 | 1186 | 754 |
| seed7643 | logistic | 0.8864 | 0.3000 | 0.1562 | 0.6250 | +0.003779 | +0.005429 | 1032 | 552 |
| seed7643 | hgb | 0.9637 | 0.6000 | 0.1562 | 0.8000 | +0.004791 | +0.005429 | 1032 | 552 |

## Common Model Check

| Model | Min AUC | Mean AUC | Mean Winner Recall | Mean Random | Min Top1 Lift |
| --- | ---: | ---: | ---: | ---: | ---: |
| hgb | 0.8854 | 0.9243 | 0.5964 | 0.1534 | +0.3843 |
| logistic | 0.8045 | 0.8590 | 0.4041 | 0.1534 | +0.1438 |

## Decision

M815A found robust pairwise separability. Proceed to M815B pairwise winner selector replay.
