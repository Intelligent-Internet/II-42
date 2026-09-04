# M764 Top16 Rank-Trace Feedback Audit

M764 adds qrels-free query-local rank-trace feedback on top of the
M758 deterministic context policy.

## Top Guards

| Rank | Guard | Test Neg Tasks | Gate | Applied | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | `trace_position_keep_at_128 le 0.476562` | None | 1 | 64 | +0.000000 | +0.000369 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 2 | `trace_position_keep_at_256 le 0.34375` | cqadupstack | 1 | 75 | +0.000000 | +0.000367 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 3 | `trace_position_keep_at_256 le 0.339844` | cqadupstack | 1 | 72 | +0.000000 | +0.000367 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 4 | `trace_position_keep_at_256 le 0.335938` | cqadupstack | 1 | 72 | +0.000000 | +0.000367 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 5 | `trace_position_keep_at_256 le 0.351562` | cqadupstack | 1 | 80 | +0.000000 | +0.000365 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 6 | `trace_position_keep_at_256 le 0.347656` | cqadupstack | 1 | 79 | +0.000000 | +0.000365 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 7 | `trace_position_keep_at_256 le 0.359375` | cqadupstack | 1 | 84 | +0.000000 | +0.000364 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 8 | `trace_position_keep_at_256 le 0.355469` | cqadupstack | 1 | 82 | +0.000000 | +0.000364 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| 9 | `trace_position_keep_at_256 le 0.332031` | trec-covid | 1 | 65 | +0.000000 | +0.000368 | +0.000268 | +0.000185 | +0.000004 | +0.000000 |
| 10 | `trace_position_keep_at_256 le 0.328125` | climate-fever, trec-covid | 1 | 63 | +0.000000 | +0.000347 | +0.000268 | +0.000185 | +0.000004 | +0.000000 |
| 11 | `trace_position_keep_at_256 le 0.324219` | climate-fever, trec-covid | 1 | 63 | +0.000000 | +0.000347 | +0.000268 | +0.000185 | +0.000004 | +0.000000 |
| 12 | `trace_position_keep_at_100 le 0.53` | fiqa | 1 | 64 | +0.000000 | +0.000278 | +0.000228 | +0.000185 | +0.000004 | +0.000000 |
| 13 | `trace_mean_drop_at_256 ge 0.820312` | climate-fever, trec-covid | 1 | 54 | +0.000000 | +0.000163 | +0.000224 | +0.000000 | +0.000004 | +0.000000 |
| 14 | `trace_mean_abs_at_256 ge 1.60156` | climate-fever, trec-covid | 1 | 54 | +0.000000 | +0.000163 | +0.000224 | +0.000000 | +0.000004 | +0.000000 |
| 15 | `trace_mean_abs_at_256 ge 1.60938` | climate-fever, trec-covid | 1 | 54 | +0.000000 | +0.000163 | +0.000224 | +0.000000 | +0.000004 | +0.000000 |

## Decision

M764 found a rank-trace guard with no test negative tasks.
