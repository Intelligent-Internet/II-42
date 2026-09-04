# M743 Accepted-Region Selector

M743 relaxes M742 from selecting the exact M654 best row to selecting
any native replay attempt that was accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `native` | `trained` | 3240 | 357 | 0.825157 | 0.404054 |
| `native_group` | `trained` | 3240 | 357 | 0.848450 | 0.335783 |
| `teacher` | `trained` | 3240 | 357 | 0.860197 | 759.498031 |
| `teacher_group` | `trained` | 3240 | 357 | 0.870287 | 859.056305 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `native` | 25 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `native_group` | 25 | 2 | 1 | 0 | +0.000209 | +0.000182 | +0.000000 | +0.000000 | +0.000209 | -0.000400 | 0 |
| `teacher` | 25 | 4 | 1 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `teacher_group` | 25 | 2 | 2 | 0 | +0.000000 | +0.000002 | +0.000000 | +0.000000 | +0.000065 | +0.000000 | 1 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

## Decision

M743 found only a tiny safe accepted-region signal. It is useful as evidence that accepted attempts are partially learnable, but it is not a retrieval-boundary breakthrough.
