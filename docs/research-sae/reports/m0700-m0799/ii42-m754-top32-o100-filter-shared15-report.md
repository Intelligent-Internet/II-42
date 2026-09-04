# M754 Top32 Global Proposal O100 Filter Shared15

This selector relaxes exact M654 best-row imitation to selecting any
native replay attempt accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 8544 | 286 | 0.912323 | 521.278457 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 271 | 268 | 33 | 16 | +0.000000 | +0.000164 | +0.000322 | -0.000451 | -0.000059 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001374 | +0.001711 | +0.000291 | +0.000043 | +0.000000 |

## Decision

Accepted-region selector did not find a strict-gate selector. The M654 coordinate oracle remains non-deployable with current features.
