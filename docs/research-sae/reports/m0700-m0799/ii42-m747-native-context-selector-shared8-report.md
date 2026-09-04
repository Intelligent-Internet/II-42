# M747 Native-Context Accepted-Region Selector Shared8

M743 relaxes M742 from selecting the exact M654 best row to selecting
any native replay attempt that was accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 6000 | 688 | 0.959731 | 475.238986 |
| `context_group` | `trained` | 6000 | 688 | 0.972630 | 418.103355 |
| `context_teacher` | `trained` | 6000 | 688 | 0.967157 | 1638.130646 |
| `context_teacher_group` | `trained` | 6000 | 688 | 0.974746 | 2302.205441 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 49 | 26 | 12 | 1 | +0.000000 | +0.001046 | +0.000769 | +0.001020 | +0.000612 | +0.000000 | 1 |
| `context_group` | 49 | 46 | 22 | 0 | +0.000000 | +0.000720 | +0.000560 | +0.000000 | +0.000229 | +0.000000 | 1 |
| `context_teacher` | 49 | 39 | 21 | 0 | +0.000000 | +0.001009 | +0.000420 | +0.000000 | +0.000517 | +0.000000 | 1 |
| `context_teacher_group` | 49 | 17 | 8 | 0 | +0.000000 | +0.000763 | +0.000420 | +0.000000 | +0.000492 | +0.000000 | 1 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001603 | +0.003861 | +0.010204 | +0.000360 | +0.000000 |

## Decision

Accepted-region selector found held-out top100 or NDCG movement. Expand to the next broader native replay surface.
