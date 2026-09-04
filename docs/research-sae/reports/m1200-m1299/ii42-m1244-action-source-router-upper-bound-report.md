# M1244 Action-Source Router Upper Bound Report

M1244 tested whether the missing structure is not just variable budget, but
action-source routing.  The question: if we choose the right action source
(`high`, `mid`, or `low`) before selecting added atoms, does the selector become
much cleaner?

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Baseline: all-action `source_abs` topK.
- Action policies:
  - fixed `high`, `mid`, `low`.
  - oracle `winner_action` from the CUB action teacher.
  - oracle `best_action` by target/harm overlap.
- Full output:
  `runs/m1244_action_source_router_upper_bound_v1/m1244_action_source_router_upper_bound.json`

## Baseline

`all_actions_source_abs_top8`:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.7793 | 0.2138 | 0.0298 | 0.1840 | 7.975 |

## Full Shared15 Result

Key policies:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `winner_action_fixed_top8` | 1.0000 | 0.9667 | 0.0474 | 0.9193 | 2.263 |
| `winner_action_fixed_top4` | 0.5613 | 0.9677 | 0.0470 | 0.9207 | 1.269 |
| `winner_action_oracle_safe_plus1` | 0.9905 | 0.9315 | 0.0054 | 0.9260 | 2.326 |
| `oracle_best_action_fixed_top8` | 1.0000 | 0.3257 | 0.0204 | 0.3053 | 6.717 |
| `fixed_action_high_fixed_top8` | 0.7360 | 0.2420 | 0.0275 | 0.2144 | 6.654 |
| `fixed_action_low_fixed_top8` | 0.3375 | 0.0949 | 0.0331 | 0.0619 | 7.778 |

## Interpretation

This is a stronger upper-bound signal than M1241.  The all-action source mixes
too many atoms from non-winning actions.  If the correct action source is known,
the target atom set becomes almost trivial to recover.

The fixed action sources are not enough:

- `fixed_action_high`/`mid` are better than `low`, but still below all-action
  recall.
- `fixed_action_low` is clearly bad on shared15.

The key bottleneck is now action-source routing.  `winner_action_fixed_top8`
has perfect target recall with very high precision.  This means the next
deployable question is whether the winner/action source can be predicted from
query-time features without qrels.

## Decision

Promote action-source routing as the next main line.

- Do not continue budget-only predictors as the primary route.
- Build M1245 learned action-router observability with LODO validation.
- If action routing is learnable, replay predicted-action topK natively.
- If action routing is not learnable, this oracle upper bound remains
  non-deployable and we need a different query-conditioned supervision source.
