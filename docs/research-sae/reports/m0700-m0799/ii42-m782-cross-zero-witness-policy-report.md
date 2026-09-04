# M782 Cross-Zero Witness Policy

M782 tests a finite deterministic policy grid over M781 witnesses:
top100 crossing, total scale, tail displacement share, and entropy
shift.  The policy is selected on dev and replayed once on test.

## Selected Policy

| max_cross | max_scale | min_tail | max_entropy | score |
| ---: | ---: | ---: | ---: | --- |
| 0 | 0.04 | 0.6 | 0 | tail_scale |

## Test Result

| Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 0 | 6.3 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.000001 |

## Decision

M782 found no clean cross-zero witness policy. Stop deterministic bundle-policy tuning.
