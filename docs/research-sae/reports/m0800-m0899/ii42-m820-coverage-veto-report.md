# M820 Coverage Veto

M820 applies a separate safe-existence floor to M816 floor-selected
queries.  It targets no-safe residuals such as seed7643/nfcorpus
PLAIN-660 without changing the ranker or proposal pool.

## Held-out Replay

| Held-out | Existence Floor | Clean | Applied | Vetoed | dMAP | dNDCG | dMRR | dCUB | Utility | Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.6649 | 1 | 3 | 0 | +0.000004 | +0.000069 | +0.000000 | +0.000006 | +0.000076 | 1.000 |
| seed7642 | 0.6987 | 1 | 2 | 1 | +0.000010 | +0.000000 | +0.000000 | +0.000000 | +0.000010 | 1.000 |
| seed7643 | 0.6849 | 0 | 8 | 0 | +0.000006 | +0.000005 | +0.000000 | -0.000008 | +0.000007 | 0.875 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | msmarco | 2 | 1.000 | +0.009803 | +0.019606 |
| original | trec-covid | 1 | 1.000 | +0.000865 | +0.000865 |
| seed7642 | climate-fever | 1 | 1.000 | +0.000198 | +0.000198 |
| seed7642 | webis-touche2020 | 1 | 1.000 | +0.002568 | +0.002568 |
| seed7643 | msmarco | 2 | 1.000 | +0.000592 | +0.001183 |
| seed7643 | nfcorpus | 3 | 0.667 | -0.000304 | -0.000912 |
| seed7643 | trec-covid | 3 | 1.000 | +0.000534 | +0.001603 |

## Decision

M820 did not pass clean replay. Query-level coverage veto cannot reliably remove no-safe residuals.
