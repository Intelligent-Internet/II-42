# M754 Top16 Global Proposal O100 Filter Shared15

This selector relaxes exact M654 best-row imitation to selecting any
native replay attempt accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 4272 | 167 | 0.902392 | 476.747594 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 271 | 10 | 1 | 0 | +0.000000 | -0.000006 | -0.000010 | +0.000000 | +0.000000 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.000829 | +0.001402 | +0.000226 | +0.000063 | +0.000000 |

## Decision

Accepted-region selector did not find a strict-gate selector. The M654 coordinate oracle remains non-deployable with current features.
