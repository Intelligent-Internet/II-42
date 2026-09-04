# M792 Dense-Constrained Utility Compiler Conservative Smoke

M792 trains a small query-output residual compiler on clean utility
targets from M790/M791. It is not a boundary-recovery claim.

## Target Counts

- Train examples: 210
- Train target examples: 29
- Dev examples: 70
- Dev target examples: 11

## Metrics

| Split | Gate | Neg Tasks | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dev | 1 | 0 | -0.000004 | +0.000000 | +0.000000 | +0.000000 | +0.000075 | -0.000857 | +0.000033 |
| test | 0 | 0 | +0.000011 | +0.000000 | +0.000000 | +0.000000 | -0.000002 | -0.000571 | +0.000010 |

## Decision

M792 learns some utility but still spends dense overlap. Keep the target construction, but strengthen preservation before scaling.
