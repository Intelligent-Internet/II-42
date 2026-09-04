# M1276 Generated-Objective Surface Audit

M1276 is the first step after stopping post-hoc gates and fixed transforms. It
checks whether a support-safe generated-posting objective has enough target
signal after clean/sign-consistency constraints, before training a compiler or
running native replay.

## Run

- Surface: hard-row smoke
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Output JSON:
  `runs/m1276_generated_objective_surface_smoke_v1/m1276_generated_objective_surface.json`
- Output detail:
  `runs/m1276_generated_objective_surface_smoke_v1/m1276_generated_objective_surface.md`

## Target Retention

| Heldout | TargetCount | Clean | CleanSigned | CleanConsistent |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 69 | 1.0000 | 1.0000 | 1.0000 |
| `scidocs` | 95 | 1.0000 | 1.0000 | 1.0000 |
| `webis-touche2020` | 53 | 1.0000 | 1.0000 | 1.0000 |

The clean/sign constraints do not destroy the hard-row target surface.  The
target atoms are query-local clean in this teacher view.

## Source Frontier

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `oracle_clean_consistent_target_top8` | 1.0000 | 1.0000 | 0.0000 | 1.0000 | 0.871 |
| `oracle_clean_signed_target_top8` | 1.0000 | 1.0000 | 0.0000 | 1.0000 | 0.871 |
| `oracle_clean_target_top8` | 1.0000 | 1.0000 | 0.0000 | 1.0000 | 0.871 |
| `source_abs_top8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |
| `signed_positive_top8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |

This is not a deployable result because oracle labels are used.  Its value is
that it reframes the next training problem:

> The target is not "rank top8 atoms better"; it is "predict a sparse
> variable-budget set, usually zero or one atom per query."

## Decision

M1276 passes the objective-surface gate.

The next valid step is a small qrels-free compiler smoke:

1. Train on candidate rows with clean target labels.
2. Use variable-budget selection, not fixed top8.
3. Evaluate target recall, precision, harm precision, and predicted count.
4. Stop if the model only reproduces the old top8 coverage/precision tradeoff.

Do not run native replay from oracle clean targets.
