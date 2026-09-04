# M735D Tiny Bundle Native Smoke

M735D evaluates the atom rows selected by M735C as one-atom tiny
bundles.  Each selected atom row was already produced by native DB
replay in M735A; this report aggregates those native effects.

- Status: `evaluated`

## Native Smoke Summary

| Split | Rows | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `full` | 79 | +0.000000 | +0.000044 | +0.001241 | +0.000000 | -0.002483 | -0.000127 | -0.000049 | 0 |
| `holdout` | 10 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.000391 | 0 |

## Decision

M735D failed tiny bundle smoke.  Do not expand; redesign the atom teacher/interface or stop this route.
