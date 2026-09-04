# M824 Rich-Feature Union Pairwise Selector Smoke

M824 reruns the M823 pairwise selector with M798-style interaction
features on the expanded candidates.

## Replay

| Held-out | Pairs | Winner Queries | Rich Rows | Train Clean Threshold | Threshold | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility | Oracle Upgrades | Negative Tasks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | 158 | 6 | 21 | 0 | 0.952887 | 1 | +0.000076 | 1 | +0.000076 | 1 | +0.000077 | 1 | none |
| seed7642 | 142 | 7 | 55 | 1 | 0.799832 | 1 | +0.000010 | 0 | -0.000012 | 1 | +0.000278 | 1 | webis-touche2020 |
| seed7643 | 120 | 5 | 30 | 1 | 0.878867 | 0 | +0.000004 | 0 | +0.000008 | 1 | +0.000019 | 3 | nfcorpus,trec-covid |

## Decision

M824 still leaves a clean oracle gap. Rich M798-style features were not enough for this small leave-surface selector.
