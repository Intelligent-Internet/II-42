# M741 Boundary-Supervised Coordinate Selector

M741 tests whether M654 accepted coordinate updates are learnable
from coordinate-candidate rows before running a full native compiler.

## Training Summary

| Feature set | Status | Rows | Positive rows | Train AUC |
| --- | --- | ---: | ---: | ---: |
| `conservative` | `trained` | 2592 | 11 | 0.955268 |
| `oracle_feature` | `trained` | 2592 | 11 | 0.957733 |

## Eval Summary

| Feature set | Queries | Accepted queries | Hit accepted | Hit best | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `conservative` | 25 | 11 | 0.000000 | 0.080000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `oracle_feature` | 25 | 11 | 0.000000 | 0.080000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

## Oracle Delta

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

## Decision

M741 cannot reproduce accepted M654 coordinates even with oracle features. Stop this coordinate-selector route and redesign the coordinate candidate generation itself.
