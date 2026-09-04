# M752 Full-Query O128p99 Filter Shared15

This selector relaxes exact M654 best-row imitation to selecting any
native replay attempt accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 7920 | 1086 | 0.954525 | 487.936628 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 271 | 14 | 11 | 1 | +0.000000 | +0.000070 | +0.000471 | +0.001845 | +0.000023 | +0.000000 | 1 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.000380 | +0.000732 | +0.001845 | +0.000123 | +0.000000 |

## Decision

Accepted-region selector found held-out top100 or NDCG movement. Expand to the next broader native replay surface.
