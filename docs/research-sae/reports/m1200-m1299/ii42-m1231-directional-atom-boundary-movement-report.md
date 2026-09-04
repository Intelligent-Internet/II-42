# M1231 Directional Atom Boundary Movement

## Purpose

M1230 showed that static atom-doc overlap/fanout did not improve target/harm
separation. M1231 tested the next richer observable shape: for each candidate
atom, estimate whether the atom's signed source delta would lift native
boundary/tail documents rather than already-protected top documents.

This is an observability audit only. It is not a replay policy and does not
modify native results.

## Inputs

- Base surface: shared15 native P1/BM25 rows.
- Teacher surface: M1224 CUB-specific target/harm atom labels.
- Candidate source: existing retrieval-conditioned atom candidate rows.
- Added features: directional score-lift aggregates over top, boundary, tail,
  P1-only tail, BM25-only tail, and shared tail document windows.
- Full run: `runs/m1231_directional_atom_boundary_movement_v1/m1231_directional_atom_boundary_movement.json`
- Smoke run: `runs/m1231_directional_atom_boundary_movement_smoke_v1/m1231_directional_atom_boundary_movement.json`

## Smoke Result

The three-row smoke (`cqadupstack`, `scidocs`, `webis-touche2020`) already did
not justify expansion.

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| source top8 | 0.976959 | 0.184991 | 0.006108 | 0.178883 |
| movement top8 | 0.976959 | 0.184991 | 0.006108 | 0.178883 |
| movement-only top8 | 0.976959 | 0.184991 | 0.006108 | 0.178883 |
| source top3 | 0.797235 | 0.252555 | 0.010219 | 0.242336 |
| movement top3 | 0.778802 | 0.246715 | 0.010219 | 0.236496 |

Movement features were either identical to the source model at top8 or weaker
at top3.

## Full Shared15 Result

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| source top8 | 0.756812 | 0.207625 | 0.029808 | 0.177817 |
| movement top8 | 0.705722 | 0.193609 | 0.029340 | 0.164268 |
| movement-only top8 | 0.694142 | 0.190432 | 0.029714 | 0.160718 |
| source top3 | 0.320504 | 0.233789 | 0.031304 | 0.202484 |
| movement top3 | 0.306540 | 0.223602 | 0.030807 | 0.192795 |

The richer directional movement features reduced target recall and precision.
They also reduced the precision-minus-harm gap, which was the primary go/no-go
signal for this audit.

## Decision

Stop this feature shape. Do not run M1232 bounded replay from M1231.

Reason:

- The review constraint for this phase is target/harm separability first, then
  native replay only if separability improves.
- M1231 fails that gate on both smoke and full shared15.
- The failure is not a training-depth question because this audit tests
  whether the input shape makes target/harm more observable before training a
  deployable policy.

## Implication

M1228 remains the useful positive signal: query-level native rank context makes
safe/harm visibility much stronger. M1230 and M1231 show that pushing that
context down into the current atom-candidate feature shape does not solve the
deployable selector problem.

The next branch should change candidate/proposal construction so harm
separation is present before training. It should not add another gate on top of
the same candidate rows.

## Next Candidate Direction

The next useful experiment should be proposal-source redesign, not replay:

1. Build candidate atoms from contrastive teacher-positive contexts and
   explicitly exclude atoms that co-occur with observed harm boundaries.
2. Audit source coverage and harm contamination before any training.
3. Only if target/harm separability improves, run a bounded native replay.

Stop condition for the next branch: if the redesigned proposal source cannot
improve target recall and the precision-minus-harm gap over the source model,
do not train a selector and do not run native replay.
