# M1261 Movement-Aware Objective Source

## Question

M1260 showed that hard-vetoed large movement is net risky and does not contain
a supported positive exception bucket.  M1261 asks whether the same movement
signal can still be used as an objective/source label:

- reward accepted low-tail atoms
- penalize rejected low-tail atoms

This is a target/harm separability audit only.  It does not run native replay.

## Runs

Smoke:

- `runs/m1261_movement_objective_source_smoke_v1/`
- rejected tail count was only `1`, so smoke was not decision-grade.

Full `shared15`:

- JSON:
  `runs/m1261_movement_objective_source_v1/m1261_movement_objective_source.json`
- Markdown:
  `runs/m1261_movement_objective_source_v1/m1261_movement_objective_source.md`
- query count: `1342`
- tail count: `1319`
- accepted tails: `1154`
- rejected tails: `165`

## Target/Harm Frontier

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | HarmQuery | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `reserve0_top8` | 0.7793 | 0.2138 | 0.0298 | 0.1840 | 0.2928 | 0.0432 | 7.975 |
| `constrained_top8_tail` | 0.8093 | 0.2004 | 0.0294 | 0.1710 | 0.3092 | 0.0447 | 8.835 |
| `reserve1_top8_tail` | 0.8154 | 0.1992 | 0.0298 | 0.1694 | 0.3130 | 0.0455 | 8.958 |
| `rejected_tail_only` | 0.0061 | 0.1091 | 0.0545 | 0.0545 | 0.0134 | 0.0067 | 0.123 |
| `all_tail_only` | 0.0361 | 0.0804 | 0.0296 | 0.0508 | 0.0790 | 0.0291 | 0.983 |
| `accepted_tail_only` | 0.0300 | 0.0763 | 0.0260 | 0.0503 | 0.0656 | 0.0224 | 0.860 |

## Interpretation

Movement acceptance is not a clean objective label.

Accepted tail atoms have lower harm precision than rejected tail atoms, but
they also have lower target precision.  The target-harm gap is not better:

- accepted tail gap: `0.0503`
- rejected tail gap: `0.0545`
- all tail gap: `0.0508`

At full source level, constrained reserve improves target recall over reserve0,
but weakens the target/harm gap:

- `reserve0_top8` gap: `0.1840`
- `constrained_top8_tail` gap: `0.1710`
- `reserve1_top8_tail` gap: `0.1694`

So the movement constraint is useful for native macro score, but it does not
produce a clean training target.

## Decision

Do not use accepted-vs-rejected movement tails as the next objective label.

Stop this movement-objective branch.

The retained evidence is:

- native movement context is useful for diagnosis and bounded replay;
- hard rejected movement is net risky;
- current movement features cannot safely recover exceptions;
- accepted movement is not a cleaner target/harm label.

## Next Step

Move away from movement splits.

M1262 should test a different source/objective construction:

- keep CUB-specific train-fold contrast as the teacher;
- stop using contrast only as an allowed-atom filter;
- use contrast strength directly in source scoring;
- compare target/harm frontier before native replay.

The hypothesis is:

> M1251/M1255 select atoms mostly by query-time signed magnitude.  That improves
> native macro, but it does not fully price train-fold harm risk.  A
> contrast-weighted source may keep the useful recall expansion while restoring
> target/harm gap.

## Artifacts

- Script: `scripts/audit_m1261_movement_objective_source.py`
- Smoke JSON:
  `runs/m1261_movement_objective_source_smoke_v1/m1261_movement_objective_source.json`
- Full JSON:
  `runs/m1261_movement_objective_source_v1/m1261_movement_objective_source.json`
