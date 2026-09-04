# M792 Dense-Constrained Utility Compiler Smoke

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
| dev | 0 | 2 | +0.001240 | +0.000580 | -0.000076 | -0.000623 | -0.000232 | -0.009143 | +0.001580 |
| test | 0 | 2 | -0.000151 | -0.000014 | -0.000175 | -0.000060 | -0.000446 | -0.006857 | -0.000400 |

## Decision

M792 does not generalize the clean utility target through this small compiler. Do not scale this architecture.
