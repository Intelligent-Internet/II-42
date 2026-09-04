# M742 Scale-Aware Coordinate Selector

M742 trains on concrete coordinate x scale native replay attempts and
chooses a dev threshold before evaluating held-out boundary queries.

## Training

| Feature set | Status | Rows | Positive rows | AUC | Threshold |
| --- | --- | ---: | ---: | ---: | ---: |
| `conservative` | `trained` | 3240 | 11 | 0.735128 | -2.323219 |
| `oracle_feature` | `trained` | 3240 | 11 | 0.838425 | 2733.614346 |

## Test

| Feature set | Queries | Applied | Hit rate | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `conservative` | 25 | 6 | 0.000000 | +0.000000 | +0.000004 | +0.000000 | +0.000000 | +0.000000 | -0.000800 | 0 |
| `oracle_feature` | 25 | 2 | 0.000000 | +0.000000 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0 |

## M654 Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

## Decision

M742 did not produce a gate-passing held-out selector. The M654 oracle depends on native replay interactions not captured by current coordinate/scale features.
