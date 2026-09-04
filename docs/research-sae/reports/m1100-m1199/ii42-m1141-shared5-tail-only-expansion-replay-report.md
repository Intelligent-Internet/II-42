# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.717658 | 0.578506 | 0.515893 | 0.411211 |
| `tail` | 0.724981 | 0.598489 | 0.526144 | 0.423195 |
| `protect5_then_tail` | 0.724981 | 0.578843 | 0.522491 | 0.416203 |
| `protect10_then_tail` | 0.731529 | 0.579530 | 0.515893 | 0.415003 |
| `protect15_then_tail` | 0.731529 | 0.578945 | 0.515893 | 0.413781 |
| `protect20_then_tail` | 0.731529 | 0.578506 | 0.515893 | 0.413328 |
| `protect25_then_tail` | 0.731529 | 0.578506 | 0.515893 | 0.412885 |
| `protect30_then_tail` | 0.731660 | 0.578506 | 0.515893 | 0.412530 |
| `protect40_then_tail` | 0.725141 | 0.578506 | 0.515893 | 0.412092 |
| `protect50_then_tail` | 0.725630 | 0.578506 | 0.515893 | 0.412181 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.007324 | +0.019982 | +0.010251 | +0.011984 |
| `protect5_then_tail` | +0.007324 | +0.000337 | +0.006598 | +0.004992 |
| `protect10_then_tail` | +0.013871 | +0.001024 | +0.000000 | +0.003792 |
| `protect15_then_tail` | +0.013871 | +0.000439 | +0.000000 | +0.002570 |
| `protect20_then_tail` | +0.013871 | +0.000000 | +0.000000 | +0.002117 |
| `protect25_then_tail` | +0.013871 | +0.000000 | +0.000000 | +0.001674 |
| `protect30_then_tail` | +0.014002 | +0.000000 | +0.000000 | +0.001318 |
| `protect40_then_tail` | +0.007484 | +0.000000 | +0.000000 | +0.000881 |
| `protect50_then_tail` | +0.007972 | +0.000000 | +0.000000 | +0.000970 |

## `protect5_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | -0.033333 | -0.003914 | -0.010994 | -0.003914 |
| `fiqa` | +0.040873 | +0.007338 | +0.016618 | +0.010403 |
| `nfcorpus` | +0.009078 | -0.000415 | +0.015789 | +0.007248 |
| `scidocs` | +0.013333 | -0.001324 | +0.004742 | +0.009219 |
| `scifact` | +0.006667 | +0.000000 | +0.006834 | +0.002003 |

## Query-Level Classes

- `clean_gain`: 42
- `damage`: 27
- `mixed`: 1
- `unchanged`: 80

## Protect Underfill

For the selected policy, `1` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

If the selected protected-tail policy improves all macro metrics, promote it to the next validation route. It should still be replayed on another split/seed or a native path before becoming a training target.
