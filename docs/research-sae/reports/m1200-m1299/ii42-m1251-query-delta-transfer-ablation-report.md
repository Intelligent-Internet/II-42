# M1251 Query-Delta Transfer Ablation

## Goal

M1250 found a macro-positive native replay from all-actions top8 with
query-time signed deltas.  M1251 isolates whether the signal comes from:

- `signed_sum`: summed signed deltas across action sources
- `max_action`: signed delta from the strongest action source
- `mean_teacher`: M1225-style mean teacher delta

This decides whether the next native policy candidate can be deployable without
teacher/oracle deltas.

## Runs

- Smoke: `runs/m1251_query_delta_transfer_ablation_smoke_v1/`
- Full shared15: `runs/m1251_query_delta_transfer_ablation_v1/`

## Full Shared15 Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1251_signed_sum_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |
| `m1251_mean_teacher_s1` | 7.697 | 0 | +0.001653 | +0.002902 | +0.002442 | +0.002807 | +0.000017 | +0.027485 |
| `m1251_signed_sum_s0.5` | 7.975 | 0 | +0.001448 | +0.001783 | +0.001633 | +0.001728 | +0.000236 | +0.019547 |
| `m1251_max_action_s1` | 7.975 | 1 | +0.001608 | +0.002630 | +0.002219 | +0.002377 | -0.000188 | +0.020239 |
| `m1251_max_action_s0.5` | 7.975 | 1 | +0.001298 | +0.001117 | +0.000516 | +0.000646 | -0.000224 | +0.006349 |
| `m1251_mean_teacher_s0.5` | 7.697 | 1 | +0.001369 | +0.001056 | +0.000515 | +0.000646 | -0.000239 | +0.006112 |

## Interpretation

The strongest result is `signed_sum_s1`, and it is deployable in principle
because it uses query-time action deltas instead of oracle or teacher deltas.

`mean_teacher_s1` is also safe, but weaker.  This matters because it shows the
new signal is not just the old M1225 mean-teacher replay.  The query-time
signed delta carries useful score geometry.

`max_action` is unsafe due to CUB loss.  The cross-action signed sum appears to
be the important shape.

## Decision

Promote `all-actions top8 + signed_sum query-time delta, scale=1.0` as the next
native policy candidate.

The next step should be a stability audit before larger engineering promotion:

1. per-dataset delta table for `signed_sum_s1`;
2. row-level negative localization where any individual dataset regresses;
3. compare against current P1 baseline, BM25, and dense on the same native
   matrix once stable.
