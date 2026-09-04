# M1245 Learned Action Router

## Goal

M1244 showed a strong action-source oracle upper bound.  M1245 tests whether
that oracle can be approximated from query-time observable features before
launching a native replay or deeper training line.

This is an observability audit, not a deployment experiment.

## Inputs

- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Source: M1244 action rows (`high`, `mid`, `low`)
- Validation: leave-one-dataset-out
- Feature groups:
  - `action_curve`: per-action score curve features
  - `action_curve_native`: action curve plus native rank/context features
- Output:
  - `runs/m1245_learned_action_router_smoke_v1/m1245_learned_action_router.json`
  - `runs/m1245_learned_action_router_smoke_v1/m1245_learned_action_router.md`

## Result

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | HarmQuery | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `all_actions_top8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 | 0.3855 | 0.0120 | 4.602 |
| `all_actions_top4` | 0.8940 | 0.2277 | 0.0082 | 0.2195 | 0.3815 | 0.0120 | 3.422 |
| `router_action_curve_top4` | 0.3779 | 0.3083 | 0.0113 | 0.2970 | 0.1365 | 0.0040 | 1.068 |
| `router_action_curve_top8` | 0.3825 | 0.2767 | 0.0100 | 0.2667 | 0.1365 | 0.0040 | 1.205 |
| `router_action_curve_native_top8` | 0.1429 | 0.1360 | 0.0000 | 0.1360 | 0.0402 | 0.0000 | 0.916 |

The learned router improves precision/gap by becoming highly conservative, but
it gives up most target coverage.  It does not approximate the M1244 oracle
or even the all-action smoke baseline.

## Fold Diagnostics

| Dataset | Group | Accuracy | BalancedAccuracy |
| --- | --- | ---: | ---: |
| `cqadupstack` | `action_curve` | 0.4700 | 0.2181 |
| `cqadupstack` | `action_curve_native` | 0.4600 | 0.2025 |
| `scidocs` | `action_curve` | 0.4600 | 0.2822 |
| `scidocs` | `action_curve_native` | 0.4300 | 0.2240 |
| `webis-touche2020` | `action_curve` | 0.5918 | 0.2702 |
| `webis-touche2020` | `action_curve_native` | 0.5306 | 0.2643 |

Balanced accuracy is too low for action-source routing.  Adding native
features did not help; it reduced recall further.

## Interpretation

M1245 confirms the review-level diagnosis: the current bottleneck is not
whether useful movement exists.  The bottleneck is safe query-time selection.

M1244 remains valuable as an upper bound and as evidence that action-source
routing is structurally meaningful.  M1245 says the current observable feature
set is not enough to deploy that routing.

## Decision

Stop M1245 at smoke.

Do not run full shared15 and do not deepen this classifier with the same
feature family.  A bigger model or more threshold tuning would likely only
repeat the same target/harm inseparability failure.

The next line should change the proposal/source construction so that harm
separation is built into the candidate source, then run the same small-to-large
gate:

1. target/harm separability audit
2. smoke native replay
3. LODO
4. full shared15 only if the smoke gate is clean
