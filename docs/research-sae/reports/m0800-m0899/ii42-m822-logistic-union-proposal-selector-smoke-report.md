# M822 Union-Proposal Selector Smoke

M822 trains one leave-surface-out selector over the M821
`mixed_direction_old_plus_expanded` union pool.

## Replay

| Held-out | Train Clean Threshold | Threshold | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility | Oracle Upgrades | Negative Tasks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | 0 | inf | 1 | +0.000076 | 1 | +0.000076 | 1 | +0.000077 | 1 | none |
| seed7642 | 0 | 0.584916 | 1 | +0.000012 | 0 | -0.000003 | 1 | +0.000280 | 1 | webis-touche2020 |
| seed7643 | 1 | inf | 0 | +0.000007 | 0 | +0.000007 | 1 | +0.000059 | 6 | nfcorpus |

## Decision

M822 keeps a clean oracle ceiling, but the learned selector did not recover it.  The next step is better selector features or a pairwise/listwise selector, not more proposal expansion.
