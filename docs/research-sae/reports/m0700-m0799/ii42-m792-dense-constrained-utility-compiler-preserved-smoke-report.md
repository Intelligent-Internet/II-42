# M792 Dense-Constrained Utility Compiler Preserved Smoke

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
| dev | 1 | 0 | -0.000003 | +0.000000 | +0.000000 | +0.000000 | +0.000036 | -0.000857 | +0.000015 |
| test | 0 | 0 | +0.000010 | +0.000000 | +0.000000 | +0.000000 | -0.000002 | -0.000714 | +0.000009 |

## Decision

M792 learns some utility but still spends dense overlap. Keep the target construction, but strengthen preservation before scaling.
