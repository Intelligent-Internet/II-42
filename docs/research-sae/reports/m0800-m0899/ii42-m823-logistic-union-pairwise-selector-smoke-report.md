# M823 Union-Pool Pairwise Selector Smoke

M823 trains a within-query pairwise winner selector over the M821
`mixed_direction_old_plus_expanded` union pool.

## Replay

| Held-out | Pairs | Winner Queries | Train Clean Threshold | Threshold | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility | Oracle Upgrades | Negative Tasks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | 158 | 6 | 0 | 0.544012 | 1 | +0.000076 | 0 | +0.000071 | 1 | +0.000077 | 1 | trec-covid |
| seed7642 | 142 | 7 | 0 | 0.767516 | 1 | +0.000010 | 0 | -0.000005 | 1 | +0.000278 | 1 | climate-fever,webis-touche2020 |
| seed7643 | 120 | 5 | 1 | 0.563727 | 0 | +0.000004 | 0 | +0.000014 | 1 | +0.000019 | 3 | nfcorpus |

## Decision

M823 still leaves a clean oracle gap. Pairwise objective alone does not make the union pool deployable; add richer interaction features or pause this route.
