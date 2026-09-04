# M749 Native-Context Accepted-Region Selector Shared15

M743 relaxes M742 from selecting the exact M654 best row to selecting
any native replay attempt that was accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 7920 | 1086 | 0.954525 | 487.936628 |
| `context_group` | `trained` | 7920 | 1086 | 0.966975 | 446.859262 |
| `context_teacher` | `trained` | 7920 | 1086 | 0.960189 | 1530.416245 |
| `context_teacher_group` | `trained` | 7920 | 1086 | 0.968272 | 1462.777690 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 67 | 17 | 11 | 1 | +0.000125 | +0.000830 | +0.000239 | +0.000000 | +0.000693 | -0.000149 | 0 |
| `context_group` | 67 | 60 | 30 | 2 | +0.000125 | +0.000938 | -0.000092 | +0.000000 | +0.000903 | -0.000149 | 0 |
| `context_teacher` | 67 | 9 | 8 | 2 | +0.000000 | +0.000006 | +0.000895 | +0.000000 | +0.000344 | +0.000000 | 1 |
| `context_teacher_group` | 67 | 42 | 25 | 3 | +0.000000 | +0.000713 | -0.000151 | +0.000000 | +0.000903 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001535 | +0.002960 | +0.007463 | +0.000497 | +0.000000 |

## Decision

Accepted-region selector found held-out top100 or NDCG movement. Expand to the next broader native replay surface.
