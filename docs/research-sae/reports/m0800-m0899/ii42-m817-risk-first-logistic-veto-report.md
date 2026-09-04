# M817 Risk-First Top-Bundle Veto

M817 keeps the M816 existence-guarded selector but adds a separate
risk-first top-bundle veto.  The veto is trained on selected top
bundles only and uses no task identity.

## Held-out Replay

| Held-out | Base Thr | Risk Cap | Clean | Applied | Vetoed | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.500 | 0.0353 | 1 | 3 | 0 | +0.000004 | +0.000069 | +0.000000 | +0.000000 | +0.000006 | +0.000000 | +0.000076 | 1.000 |
| seed7642 | 0.500 | 0.0157 | 1 | 3 | 0 | +0.000012 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000012 | 1.000 |
| seed7643 | 0.500 | 0.0091 | 0 | 7 | 1 | +0.000006 | +0.000005 | +0.000000 | +0.000000 | -0.000008 | +0.000000 | +0.000007 | 0.857 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | msmarco | 2 | 1.000 | +0.009803 | +0.019606 |
| original | trec-covid | 1 | 1.000 | +0.000865 | +0.000865 |
| seed7642 | climate-fever | 2 | 1.000 | +0.000286 | +0.000573 |
| seed7642 | webis-touche2020 | 1 | 1.000 | +0.002568 | +0.002568 |
| seed7643 | msmarco | 2 | 1.000 | +0.000592 | +0.001183 |
| seed7643 | nfcorpus | 3 | 0.667 | -0.000304 | -0.000912 |
| seed7643 | trec-covid | 2 | 1.000 | +0.000768 | +0.001536 |

## Decision

M817 partially improves harmful-row control, but does not pass the common useful clean gate. Inspect remaining failures before scaling.
