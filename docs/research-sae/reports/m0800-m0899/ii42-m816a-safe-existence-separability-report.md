# M816A Safe-Existence Separability

M816A tests whether each query has any safe-positive generated
bundle, using only aggregate features over that query candidate set.

## Leave-Surface-Out Results

| Held-out | Model | AUC | AP | Pos Rate | P@5% | P@10% | P@20% | Train Q | Eval Q |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | hgb | 0.8762 | 0.6771 | 0.216 | 0.900 | 0.789 | 0.632 | 391 | 190 |
| original | logistic | 0.8168 | 0.5465 | 0.216 | 0.600 | 0.632 | 0.526 | 391 | 190 |
| seed7642 | hgb | 0.8754 | 0.7144 | 0.258 | 0.800 | 0.842 | 0.789 | 398 | 190 |
| seed7642 | logistic | 0.8055 | 0.5950 | 0.258 | 0.700 | 0.684 | 0.579 | 398 | 190 |
| seed7643 | hgb | 0.8761 | 0.7373 | 0.204 | 1.000 | 0.750 | 0.667 | 393 | 196 |
| seed7643 | logistic | 0.8027 | 0.5989 | 0.204 | 0.900 | 0.700 | 0.487 | 393 | 196 |

## Common Model Check

| Model | Min AUC | Mean AUC | Min AP | Mean AP | Min P@10% | Mean P@10% |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hgb | 0.8754 | 0.8759 | 0.6771 | 0.7096 | 0.750 | 0.794 |
| logistic | 0.8027 | 0.8083 | 0.5465 | 0.5801 | 0.632 | 0.672 |

## Decision

M816A found robust safe-existence separability. Proceed to M816B and use query-level existence probability as an abstention guard.
