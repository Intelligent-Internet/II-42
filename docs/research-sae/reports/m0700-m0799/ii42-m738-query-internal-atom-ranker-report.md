# M738 Query-Internal Atom Ranker

M738 trains a within-query pairwise atom ranker from expanded M736B
single-atom replay rows, then replays fixed top-k policies through
the native unified-posting scorer.

## Rank Audit

| Split | Positive atoms | Mean rank | Hit@1 | Hit@2 | Hit@3 | Hit@5 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `train` | 66 | 2.818 | 0.333333 | 0.545455 | 0.681818 | 0.909091 |
| `holdout` | 9 | 4.111 | 0.222222 | 0.222222 | 0.444444 | 0.777778 |

## Native Replay Summary

| Surface | Split | Rows | Productive | Safe | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pairwise_damage_penalty_top1_s0.005` | `full` | 200 | 0.120000 | 0.590000 | +0.000000 | +0.000500 | -0.000025 | +0.000214 | -0.001981 | -0.000450 | -0.000117 | 0 |
| `pairwise_damage_penalty_top1_s0.005` | `holdout` | 29 | 0.068966 | 0.655172 | +0.000000 | +0.000398 | +0.000209 | +0.000000 | +0.000000 | +0.001034 | +0.000000 | 1 |
| `pairwise_damage_penalty_top2_s0.005` | `full` | 200 | 0.095000 | 0.545000 | +0.000119 | +0.000457 | -0.000515 | +0.000048 | -0.000981 | -0.000850 | +0.000449 | 0 |
| `pairwise_damage_penalty_top2_s0.005` | `holdout` | 29 | 0.068966 | 0.620690 | +0.000000 | -0.000272 | -0.000169 | +0.000000 | +0.000000 | +0.000345 | +0.000135 | 0 |
| `pairwise_top1_s0.005` | `full` | 200 | 0.120000 | 0.560000 | +0.000000 | +0.000499 | -0.000511 | +0.000214 | -0.001981 | -0.000750 | +0.000020 | 0 |
| `pairwise_top1_s0.005` | `holdout` | 29 | 0.068966 | 0.586207 | +0.000000 | +0.000398 | +0.000209 | +0.000000 | +0.000000 | +0.000690 | -0.000404 | 0 |
| `pairwise_top2_s0.005` | `full` | 200 | 0.090000 | 0.520000 | +0.001119 | +0.000515 | -0.000515 | +0.000048 | -0.000981 | -0.001300 | +0.000586 | 0 |
| `pairwise_top2_s0.005` | `holdout` | 29 | 0.068966 | 0.551724 | +0.000000 | -0.000272 | -0.000169 | +0.000000 | +0.000000 | -0.000345 | +0.000269 | 0 |

## Decision

M738 found a deployable holdout policy: pairwise_damage_penalty_top1_s0.005. Expand the same fixed policy to a larger shared15 slice.
