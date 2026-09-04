# M789 Generated Delta Query Safety

This audit decomposes M788 frontier configs into per-query deltas.
The key question is whether aggregate-unsafe boundary movement hides
individual clean query-level positives that a hard gate could learn.

## Config Summary

| Model | TopN | Scale | Selected | Utility+ | Clean Utility+ | Boundary+ | Clean Boundary+ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| logistic | 4 | 0.50 | 11 | 6 | 6 | 0 | 0 |
| logistic | 4 | 1.00 | 173 | 61 | 59 | 0 | 0 |
| logistic | 16 | 1.00 | 173 | 60 | 58 | 0 | 0 |
| logistic | 8 | 1.00 | 173 | 60 | 58 | 0 | 0 |
| logistic | 2 | 1.00 | 173 | 61 | 58 | 0 | 0 |
| logistic | 4 | 0.50 | 43 | 22 | 18 | 0 | 0 |
| logistic | 8 | 0.50 | 43 | 22 | 18 | 0 | 0 |
| hgb | 4 | 2.00 | 87 | 42 | 14 | 1 | 0 |
| hgb | 2 | 0.25 | 0 | 0 | 0 | 0 | 0 |
| hgb | 16 | 2.00 | 87 | 42 | 14 | 1 | 0 |
| hgb | 8 | 2.00 | 87 | 42 | 14 | 1 | 0 |
| hgb | 2 | 2.00 | 87 | 42 | 14 | 1 | 0 |
| hgb | 8 | 1.00 | 185 | 70 | 61 | 0 | 0 |
| hgb | 16 | 1.00 | 185 | 70 | 61 | 0 | 0 |

## Decision

M789 found boundary positives, but none are query-clean. Stop row-mixed generated deltas for boundary recovery; switch to a model-level compiler objective with dense constraints.
