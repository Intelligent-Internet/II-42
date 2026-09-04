# M1236 Action-Conditioned Added Atom Report

M1236 tested whether the CUB-specific added-atom teacher should keep
`high/mid/low` action identity instead of collapsing every proposal to one
atom ID.  This was an observability audit only; no native replay was launched.

## Inputs

- Source labels: M1224 CUB-specific target/harm atom rows.
- Correction inherited from M1235: this is an added-atom problem, not existing
  query-atom weight movement.
- Evaluation: leave-one-dataset-out over shared15.
- Output:
  `runs/m1236_action_conditioned_added_atom_v1/m1236_action_conditioned_added_atom.json`

## Full Shared15 Result

Source atom baseline:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.7793 | 0.2138 | 0.0298 | 0.1840 | 7.975 |

Best action-conditioned variants:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `row_only_model_logistic_top12_atom` | 0.8709 | 0.1667 | 0.0292 | 0.1375 | 11.430 |
| `context_interact_model_logistic_top12_atom` | 0.8512 | 0.1648 | 0.0294 | 0.1354 | 11.302 |
| `row_only_model_logistic_top8_atom` | 0.7418 | 0.2071 | 0.0290 | 0.1781 | 7.835 |
| `context_interact_model_logistic_top8_atom` | 0.7210 | 0.2024 | 0.0295 | 0.1728 | 7.796 |

## Interpretation

The smoke result had a weak positive hint, but full shared15 rejects the shape.
Action-conditioned top12 variants recover more target atoms, but they do it by
adding too many low-precision atoms.  The separation gap falls from `0.1840` to
`0.1375`, so this is not a deployable selector/generator improvement.

Pair-level action rows are also worse than projected atom rows, which means the
action identity is not the missing safe-selection signal.  It mostly creates
more proposal degrees of freedom without enough harm separation.

## Decision

Stop M1236 here.

- Do not run native replay for this shape.
- Do not train an action-conditioned compiler from this feature set.
- Preserve the result as evidence that added-atom target coverage can be raised
  only by sacrificing precision unless proposal construction changes.

The next useful direction should not be another threshold/grid/classifier over
the same target/harm surface.  The remaining bottleneck is proposal source and
teacher construction: useful CUB target atoms are real, but current observable
features cannot safely distinguish target from harm at query time.
