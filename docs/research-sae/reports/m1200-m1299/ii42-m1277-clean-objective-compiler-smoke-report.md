# M1277 Clean-Objective Compiler Smoke

M1277 trains a qrels-free candidate-row model to predict the sparse clean
target surface from M1276.  This is still a target/harm frontier test, not a
native replay.

The key change from earlier fixed-topK selectors is variable-budget selection:
the model chooses up to three atoms per query using a train-fold threshold.

## Run

- Surface: hard-row smoke
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Output JSON:
  `runs/m1277_clean_objective_compiler_smoke_v1/m1277_clean_objective_compiler_smoke.json`
- Output detail:
  `runs/m1277_clean_objective_compiler_smoke_v1/m1277_clean_objective_compiler_smoke.md`

## Macro Frontier

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | HarmQuery | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_variable_budget_top3` | 0.5576 | 0.3712 | 0.0092 | 0.3620 | 0.3004 | 0.0082 | 1.342 |
| `source_abs_top8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 | 0.3951 | 0.0123 | 4.716 |

## Interpretation

This is the first useful signal after the M1272-M1276 route-control sequence.

What improved:

- precision roughly doubled: `0.1850 -> 0.3712`
- target/harm gap improved: `0.1789 -> 0.3620`
- predicted atom budget dropped from `4.716` to `1.342`
- harm query rate stayed low

What regressed:

- target recall dropped from `0.9770` to `0.5576`
- any-hit rate dropped from `0.3951` to `0.3004`

This is a plausible generated-posting compiler shape: sparse and safer, but
not yet proven useful for native retrieval.  The next question is whether the
lost coverage is acceptable when replayed through the native scorer.

## Decision

Promote to bounded native replay on the same hard-row smoke only.

Replay constraints:

1. Use the exact LODO model/threshold setup from M1277.
2. Compare against source_abs/equal signed_sum baseline.
3. Test two impact geometries only:
   - selected signed_sum
   - selected uniform_l1
4. Stop if Recall/CUB losses dominate or if macro score is below the existing
   source_abs route.
