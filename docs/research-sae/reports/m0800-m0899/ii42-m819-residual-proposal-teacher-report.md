# M819 Residual Proposal Teacher Audit

M819 audits whether residual harmful selected queries already contain
a safe alternative in the same generated-bundle proposal pool.

## Replay Ceiling

| Held-out | Selected | Safe Rate | Replaceable | No Safe | Baseline Clean | Baseline Utility | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 3 | 1.000 | 0 | 0 | 1 | +0.000076 | 1 | +0.000076 |
| seed7642 | 3 | 1.000 | 0 | 0 | 1 | +0.000012 | 1 | +0.000012 |
| seed7643 | 8 | 0.875 | 0 | 1 | 0 | +0.000007 | 1 | +0.000021 |

## Residual Actions

| Held-out | Task | Query | Action | Selected Safe | Selected Utility | Best Safe Utility |
| --- | --- | --- | --- | ---: | ---: | ---: |
| seed7643 | nfcorpus | PLAIN-660 | drop_no_safe | 0 | -0.000981 | +0.000000 |

## Decision

M819 found partial same-pool replacement gains, but not a common clean ceiling. Proposal coverage and top-bundle teacher both need inspection before scaling.
