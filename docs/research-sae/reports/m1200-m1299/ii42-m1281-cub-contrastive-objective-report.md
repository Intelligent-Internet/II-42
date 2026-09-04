# M1281 CUB-Contrastive Objective Audit

## Question

M1280 rejected direct source composition.  M1281 changes the training objective
instead: train one held-out model for CUB-specific target atoms and another for
CUB-harm atoms, then score candidates with:

`target_prob - lambda * harm_prob`

This tests whether making CUB-harm an explicit objective term creates a
cleaner source before native replay.

## Result

Hard-row smoke surface:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Top frontier:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `contrast_h0.25_top3` | 0.797235 | 0.252924 | 0.010234 | 0.242690 | 2.815 |
| `contrast_h0.5_top3` | 0.788018 | 0.250733 | 0.010264 | 0.240469 | 2.807 |
| `source_abs_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |

Per-fold shape:

| Heldout | Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | `source_abs_top3` | 0.652174 | 0.155709 | 0.000000 | 0.155709 |
| `cqadupstack` | `contrast_h0.25_top3` | 0.652174 | 0.156250 | 0.000000 | 0.156250 |
| `scidocs` | `source_abs_top3` | 0.915789 | 0.346614 | 0.027888 | 0.318725 |
| `scidocs` | `contrast_h0.25_top3` | 0.915789 | 0.346614 | 0.027888 | 0.318725 |
| `webis-touche2020` | `source_abs_top3` | 0.773585 | 0.282759 | 0.000000 | 0.282759 |
| `webis-touche2020` | `contrast_h0.25_top3` | 0.773585 | 0.282759 | 0.000000 | 0.282759 |

## Interpretation

M1281 is weakly positive as a diagnostic, but not strong enough to replay.

The best CUB-contrastive objective improves the aggregate gap by only about
`+0.00035`, and the per-fold view shows almost no real movement.  `h0.25`
barely changes `cqadupstack` and leaves `scidocs` and `webis-touche2020`
unchanged.  Stronger harm penalties trade recall away.

This means CUB-harm must remain part of the future objective, but this simple
two-head contrastive formulation does not create a deployable source.

## Decision

Do not run native replay for M1281.

Retain the lesson:

- explicit CUB-harm supervision is the right kind of ingredient;
- current candidate rows/features still do not expose enough additional
  information for a new source;
- the next branch must change the source/interface, not only the scalar loss.

## Artifacts

- Script: `scripts/audit_m1281_cub_contrastive_objective.py`
- JSON: `runs/m1281_cub_contrastive_objective_smoke_v1/m1281_cub_contrastive_objective.json`
- Markdown: `runs/m1281_cub_contrastive_objective_smoke_v1/m1281_cub_contrastive_objective.md`
