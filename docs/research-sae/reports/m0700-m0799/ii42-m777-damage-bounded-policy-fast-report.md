# M777 Damage-Bounded Policy Fast Smoke

M777 searches one qrels-free damage bound before deterministic
native-context best-row selection.  Thresholds are chosen on dev
surfaces and replayed unchanged on test surfaces.

## Top Test Policies

| Rank | Score | Bound | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | `margin_bundle` | `trace_mean_abs_at_100 le 0.5` | 1 | 1 | 1.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 2 | `margin_bundle` | `None` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 3 | `margin_bundle` | `cross_rate_at_100 le 0` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 4 | `margin_bundle` | `cross_rate_at_100 le 0.01` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 5 | `margin_bundle` | `cross_rate_at_100 le 0.02` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 6 | `margin_bundle` | `trace_mean_abs_at_100 le 1.67` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 7 | `margin_bundle` | `tail_abs_share_at_100 le 0.45` | 1 | 1 | 0.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 8 | `margin_bundle` | `trace_mean_abs_at_100 le 0.5` | 1 | 1 | 1.3 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 9 | `margin_bundle` | `trace_mean_abs_at_100 le 0.5` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 10 | `margin_bundle` | `rr_min_delta_at_100 ge -0.00320513` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 11 | `margin_bundle` | `rr_min_delta_at_100 ge -0.00320513` | 1 | 1 | 1.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 12 | `margin_bundle` | `rr_min_delta_at_100 ge -0.00320513` | 1 | 1 | 0.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 13 | `margin_bundle` | `tail_abs_share_at_100 le 0.351351` | 1 | 1 | 0.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 14 | `margin_bundle` | `entropy_delta_at_100 le -5.97379e-06` | 1 | 1 | 0.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |
| 15 | `margin_bundle` | `entropy_delta_at_100 le 3.11098e-08` | 1 | 1 | 0.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | None |

## Decision

M777 found only diagnostic-scale clean damage-bounded policy; do not promote without a new proposal surface.
