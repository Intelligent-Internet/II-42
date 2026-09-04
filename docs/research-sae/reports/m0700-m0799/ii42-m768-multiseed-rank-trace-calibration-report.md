# M768 Multi-Seed Rank-Trace Calibration

M768 searches one rank-trace guard across multiple dev surfaces and
replays it on all matching test surfaces.

## Top Candidates

| Rank | Guard | Test All Clean | Mean Utility | Min Utility | Total Applied | Test Negative Surfaces |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 1 | `trace_spearman_at_256 ge 0.999933` | 1 | +0.000007 | +0.000004 | 18 | None |
| 2 | `trace_position_keep_at_128 ge 0.867188` | 1 | +0.000001 | +0.000000 | 7 | None |
| 3 | `trace_position_keep_at_100 le 0.31` | 1 | +0.000047 | +0.000000 | 3 | None |
| 4 | `trace_mean_drop_at_20 ge 0.45` | 1 | +0.000047 | +0.000000 | 2 | None |
| 5 | `trace_position_keep_at_50 le 0.36` | 1 | +0.000047 | +0.000000 | 2 | None |
| 6 | `trace_position_keep_at_50 le 0.4` | 1 | +0.000047 | +0.000000 | 2 | None |
| 7 | `trace_spearman_at_10 le 0.962091` | 1 | +0.000023 | +0.000000 | 2 | None |
| 8 | `trace_p95_abs_at_10 ge 2` | 1 | +0.000023 | +0.000000 | 1 | None |
| 9 | `trace_spearman_at_10 le 0.951515` | 1 | +0.000023 | +0.000000 | 1 | None |
| 10 | `trace_spearman_at_100 ge 0.999928` | 1 | +0.000000 | +0.000000 | 11 | None |
| 11 | `trace_position_keep_at_100 ge 0.92` | 1 | +0.000000 | +0.000000 | 8 | None |
| 12 | `trace_p95_abs_at_256 le 1.25` | 1 | +0.000000 | +0.000000 | 8 | None |
| 13 | `trace_mean_drop_at_100 le 0.04` | 1 | +0.000000 | +0.000000 | 7 | None |
| 14 | `trace_mean_abs_at_100 le 0.08` | 1 | +0.000000 | +0.000000 | 7 | None |
| 15 | `trace_spearman_at_100 ge 0.999952` | 1 | +0.000000 | +0.000000 | 7 | None |

## Best Test Surface Details

| Surface | Gate | Negative Tasks | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 1 | None | 4 | +0.000004 | +0.000000 | +0.000000 | +0.000008 | +0.000000 |
| seed7642 | 1 | None | 9 | +0.000005 | +0.000000 | +0.000000 | +0.000008 | +0.000000 |
| seed7643 | 1 | None | 5 | +0.000004 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

## Decision

M768 multi-seed guard passes but collapses to diagnostic gain.
