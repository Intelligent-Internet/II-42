# M781 Bundle Oracle Observability

M781 audits whether the M779 strict-positive bundle oracle is visible
from qrels-free native/bundle features before more selector training.

## Counts

| Split | Rows | Positives | Positive Queries | Positive Signatures |
| --- | ---: | ---: | ---: | ---: |
| dev | 10413 | 318 | 105 | 13 |
| test | 10569 | 332 | 118 | 13 |

## Feature Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| cross_rate_at_100 | 0.907132 | 0.903732 | -1 |
| tail_abs_share_at_100 | 0.638940 | 0.616826 | +1 |
| total_scale | 0.607292 | 0.614821 | -1 |
| scale_1 | 0.550527 | 0.577145 | -1 |
| entropy_delta_at_100 | 0.567101 | 0.554857 | -1 |
| scale_0 | 0.545162 | 0.536719 | -1 |
| direction_0 | 0.530067 | 0.520931 | +1 |
| direction_1 | 0.530067 | 0.520931 | +1 |
| dim_0 | 0.518215 | 0.511841 | -1 |
| accepted_train_count | 0.520585 | 0.502033 | +1 |
| dim_1 | 0.524078 | 0.496173 | +1 |
| rr_min_delta_at_100 | 0.509625 | 0.494367 | +1 |

## Best Test Feature Separation

| Feature | AUC(abs) | Effect | Positive Mean | Negative Mean |
| --- | ---: | ---: | ---: | ---: |
| cross_rate_at_100 | 0.903732 | -1.325 | 0 | 0.0129901 |
| tail_abs_share_at_100 | 0.616826 | +0.472 | 0.408684 | 0.380515 |
| total_scale | 0.614821 | -0.461 | 0.0500602 | 0.0571456 |
| scale_1 | 0.577145 | -0.288 | 0.0254819 | 0.0301465 |
| entropy_delta_at_100 | 0.554857 | -0.132 | -5.01414e-06 | -1.7665e-06 |
| scale_0 | 0.536719 | -0.152 | 0.0245783 | 0.0269991 |
| direction_0 | 0.520931 | +0.157 | -0.76506 | -0.848784 |
| direction_1 | 0.520931 | +0.157 | -0.76506 | -0.848784 |
| dim_0 | 0.511841 | -0.051 | 235.238 | 245.475 |
| rr_min_delta_at_100 | 0.505633 | -0.138 | -0.0300246 | -0.0227645 |
| dim_1 | 0.503827 | -0.024 | 565.56 | 570.065 |
| accepted_train_count | 0.502033 | +0.008 | 2361.04 | 2358.85 |

## Positive Overlap

- dev positive queries: 105
- test positive queries: 118
- query intersection: 36
- dev positive signatures: 13
- test positive signatures: 13
- signature intersection: 13

## Decision

M781 finds enough qrels-free observability to justify one bounded interaction-witness selector smoke.
