# M739 Query-Internal Utility Ranker

M739 removes the M735B productive selector dependency and trains a
pairwise atom ranker directly from native replay utility deltas.

## Rank Audit

| Split | Positive atoms | Mean rank | Hit@1 | Hit@2 | Hit@3 | Hit@5 | Utility corr |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `train` | 126 | 5.317 | 0.087302 | 0.166667 | 0.230159 | 0.492063 | 0.302878 |
| `holdout` | 32 | 5.562 | 0.093750 | 0.156250 | 0.187500 | 0.406250 | 0.245303 |

## Native Replay Summary

| Surface | Split | Rows | Productive | Safe | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `utility_risk_penalty_top1_s0.005` | `full` | 240 | 0.066667 | 0.912500 | -0.000005 | +0.000001 | -0.000028 | +0.000000 | -0.000072 | +0.000125 | +0.000358 | 0 |
| `utility_risk_penalty_top1_s0.005` | `holdout` | 40 | 0.075000 | 0.925000 | +0.000000 | +0.000080 | +0.000126 | +0.000000 | +0.000000 | -0.000250 | +0.000098 | 0 |
| `utility_top1_s0.005` | `full` | 240 | 0.058333 | 0.916667 | +0.000000 | -0.000040 | -0.000049 | +0.000000 | -0.000904 | +0.000042 | +0.000293 | 0 |
| `utility_top1_s0.005` | `holdout` | 40 | 0.075000 | 0.975000 | +0.000000 | -0.000051 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000098 | 0 |
| `utility_top2_s0.005` | `full` | 240 | 0.062500 | 0.850000 | +0.000015 | -0.000175 | -0.000571 | +0.000000 | -0.000866 | -0.000083 | +0.000342 | 0 |
| `utility_top2_s0.005` | `holdout` | 40 | 0.125000 | 0.900000 | +0.000000 | +0.000038 | +0.000126 | +0.000000 | +0.000000 | -0.000500 | +0.000195 | 0 |

## Decision

M739 learned some query-internal utility ordering, but no native policy passed the strict gate. Redesign utility labels or add stronger preservation constraints before expanding.
