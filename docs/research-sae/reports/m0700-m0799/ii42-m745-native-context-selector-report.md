# M745 Native-Context Accepted-Region Selector

M743 relaxes M742 from selecting the exact M654 best row to selecting
any native replay attempt that was accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 3240 | 357 | 0.968751 | 395.998271 |
| `context_group` | `trained` | 3240 | 357 | 0.981874 | 231.722947 |
| `context_teacher` | `trained` | 3240 | 357 | 0.974046 | 1068.028807 |
| `context_teacher_group` | `trained` | 3240 | 357 | 0.983018 | 1278.957862 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 25 | 25 | 12 | 2 | +0.000000 | +0.001306 | +0.000940 | +0.000000 | +0.000222 | +0.000000 | 1 |
| `context_group` | 25 | 24 | 11 | 2 | +0.000000 | +0.000178 | +0.000000 | +0.000000 | -0.000084 | +0.000000 | 0 |
| `context_teacher` | 25 | 24 | 10 | 2 | +0.000000 | +0.000050 | +0.000000 | +0.000000 | +0.000317 | +0.000000 | 1 |
| `context_teacher_group` | 25 | 25 | 12 | 0 | +0.000000 | +0.000116 | +0.000000 | +0.000000 | -0.000004 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

## Decision

Accepted-region selector found held-out top100 or NDCG movement. Expand to the next broader native replay surface.
