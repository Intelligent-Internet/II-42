# M1249 Low-Action Reserve Source

## Goal

M1248 showed that all-actions top8 misses are mostly low-action, single-action
tail atoms.  M1249 tests whether a small low-action reserve can improve the
target/harm frontier before native replay.

## Runs

- Smoke: `runs/m1249_low_action_reserve_source_smoke_v1/`
- Full shared15: `runs/m1249_low_action_reserve_source_v1/`

## Full Shared15 Result

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| all_actions_top8 | 0.7793 | 0.2138 | 0.0298 | 0.1840 | 7.975 |
| all_actions_top12 | 0.8781 | 0.1627 | 0.0298 | 0.1328 | 11.808 |
| all_actions_top16 | 0.9608 | 0.1393 | 0.0289 | 0.1105 | 15.085 |
| top8_plus_low_reserve2 | 0.8508 | 0.1875 | 0.0298 | 0.1577 | 9.928 |
| top8_plus_low_reserve4 | 0.9155 | 0.1697 | 0.0295 | 0.1401 | 11.806 |
| top8_plus_low_reserve8 | 0.9898 | 0.1442 | 0.0283 | 0.1159 | 15.013 |

Low-action reserve improves target recall over same-size generic all-actions
budgets:

- reserve4 vs top12: recall +0.0375, precision +0.0070, harm slightly lower.
- reserve8 vs top16: recall +0.0290, precision +0.0049, harm lower.

## Interpretation

This is a real source-frontier improvement.  The M1248 low-action miss pattern
is actionable at the target/harm level.

However, this is not yet a retrieval improvement.  It only proves that the
source can recover more CUB target atoms with bounded harm.

## Decision

Run a bounded native replay for the low-action reserve shape.
