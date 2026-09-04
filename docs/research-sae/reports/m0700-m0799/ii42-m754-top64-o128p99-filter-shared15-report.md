# M754 Top64 Global Proposal O128p99 Filter Shared15

This selector relaxes exact M654 best-row imitation to selecting any
native replay attempt accepted by the strict metric gate.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | ---: | ---: | ---: | ---: | ---: |
| `context` | `trained` | 17088 | 778 | 0.846277 | 572.334488 |

## Test

| Feature set | Queries | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `context` | 271 | 93 | 16 | 5 | +0.000000 | +0.000151 | +0.000137 | +0.000000 | -0.000061 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001395 | +0.001711 | +0.000291 | +0.000050 | +0.000000 |

## Decision

Accepted-region selector did not find a strict-gate selector. The M654 coordinate oracle remains non-deployable with current features.
