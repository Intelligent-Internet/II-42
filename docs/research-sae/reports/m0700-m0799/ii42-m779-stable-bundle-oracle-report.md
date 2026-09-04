# M779 Stable Bundle Oracle Smoke

M779 tests whether same-direction coordinate pairs can restore
oracle ceiling after the single-coordinate row-delta route stopped.

## Bundle Config

- singles: 10
- bundles: 13

## Label Counts

| Split | Rows | Strict Positive | Utility Positive |
| --- | ---: | ---: | ---: |
| dev | 10413 | 318 | 1450 |
| test | 10569 | 332 | 1498 |

## Test Oracle Results

| Oracle | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| query_strict_positive | 1 | 1 | 51.7 | +0.000596 | +0.001129 | +0.000201 | +0.000135 | +0.000000 | +0.001833 |
| query_utility_positive | 0 | 0 | 91.7 | +0.002228 | +0.002928 | +0.001544 | +0.000414 | -0.003985 | +0.005671 |

## Decision

M779 bundle oracle is strong enough to justify selector replay.
