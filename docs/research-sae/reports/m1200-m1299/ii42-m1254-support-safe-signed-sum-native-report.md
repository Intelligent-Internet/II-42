# M1254 Support-Safe Signed-Sum Native Replay

## Goal

M1253 showed that `signed_sum_s1` is macro-positive but cannot be protected by
simple qrels-free harm buckets.  M1254 therefore tests source construction, not
another guard:

- `all_actions`: M1251 reference signed-sum source
- `source_abs`: M1224 CUB-specific source-abs source, using query-local signed
  deltas
- `source_x_delta`: source-abs score multiplied by signed-delta magnitude
- `intersection`: atoms selected by both `all_actions` and `source_abs`

The intended question is whether support-aware source construction can retain
the signed-sum gain while reducing harmful rows.

## Runs

Smoke v1:

- `runs/m1254_support_safe_signed_sum_native_smoke_v1/`

Smoke v2 with source-overlap instrumentation:

- JSON:
  `runs/m1254_support_safe_signed_sum_native_smoke_v2/m1254_support_safe_signed_sum_native.json`
- Markdown:
  `runs/m1254_support_safe_signed_sum_native_smoke_v2/m1254_support_safe_signed_sum_native.md`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

## Smoke Result

All source variants produced identical native metrics.

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1254_all_actions_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1254_source_abs_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1254_source_x_delta_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1254_intersection_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |

The selection-overlap audit explains why.  For all three held-out smoke
datasets, `source_abs`, `source_x_delta`, and `intersection` are exact matches
to `all_actions`:

| Dataset | Source | ExactMatch | RefOnly | CandOnly |
| --- | --- | ---: | ---: | ---: |
| `cqadupstack` | `source_abs` | 1.0000 | 0.0000 | 0.0000 |
| `cqadupstack` | `source_x_delta` | 1.0000 | 0.0000 | 0.0000 |
| `cqadupstack` | `intersection` | 1.0000 | 0.0000 | 0.0000 |
| `scidocs` | `source_abs` | 1.0000 | 0.0000 | 0.0000 |
| `scidocs` | `source_x_delta` | 1.0000 | 0.0000 | 0.0000 |
| `scidocs` | `intersection` | 1.0000 | 0.0000 | 0.0000 |
| `webis-touche2020` | `source_abs` | 1.0000 | 0.0000 | 0.0000 |
| `webis-touche2020` | `source_x_delta` | 1.0000 | 0.0000 | 0.0000 |
| `webis-touche2020` | `intersection` | 1.0000 | 0.0000 | 0.0000 |

## Decision

Stop this M1254 branch before full replay.

This is not a negative result for signed-sum itself.  It says the proposed
support-aware source construction does not actually change the selected atoms
under the current CUB-allowed/top8 setup.

## Follow-Up Finding

While checking why M1254 did not create a new source, I found a structural issue
in the prior low-reserve line: M1250 `low_reserve=0` still adds one low-tail atom
because `low_tail_predictions()` appends before testing the `top_n` limit.

That means the M1250 best row labeled `top8_low0_s1` was effectively
`top8 + 1 low-tail atom`, not pure top8.  This likely explains why M1250 was
slightly better than the later pure M1251 signed-sum run.

The next useful step is not another guard.  It is a corrected low-reserve
frontier:

- true `reserve0`
- true `reserve1`
- reserve sizes `2`, `4`, `8`
- same native full `shared15` replay

If corrected `reserve1` is clean and still beats pure signed-sum, it becomes the
next concrete source-construction improvement.
