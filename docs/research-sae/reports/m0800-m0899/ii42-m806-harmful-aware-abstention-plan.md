# M806 Harmful-Aware Abstention Plan

## Context

M802 built a reusable generated-bundle feature table from the existing
M754/M766/M767 surfaces.  M803 then trained a harmful-aware selector with
separate positive and harmful heads.  M805 replayed that selector in a
leave-surface-out setting to test whether the clean M803 point was surface
specific.

## Evidence

M803 found a strict-clean but very small operating point:

| Score | Model | Lambda | Applied | dMAP | dNDCG | dCUB | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| margin_bundle | logistic | 0.5 | 3.0 | +0.000012 | +0.000078 | +0.000041 | +0.000000 |

M805 is more important because the threshold is selected without seeing the
held-out surface.  It found clean leave-surface-out points on all three
surfaces, but the useful magnitude is uneven:

| Held-out | Score | Model | Lambda | Applied | dMAP | dNDCG | dCUB | dO@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | score_mean_margin | logistic | 2.0 | 20.0 | +0.000155 | +0.000118 | +0.000000 | +0.000000 |
| seed7642 | score_mean_margin | logistic | 2.0 | 16.0 | +0.000133 | +0.000113 | +0.000000 | +0.000000 |
| seed7643 | conservative_margin | logistic | 0.0 | 5.0 | +0.000007 | +0.000000 | +0.000050 | +0.000000 |

The signal is therefore real, but not ready for broad replay as a single
global threshold.  Seed7643 behaves like a worst-surface stress case: the
selector can stay clean only by applying very few moves.

## Interpretation

This round changes the diagnosis from "generated-bundle repair is not
learnable" to "generated-bundle repair needs abstention before promotion."
The interaction features and harmful head can identify useful safe moves, but
the decision boundary is still too surface-sensitive.

The route should continue only through a stricter abstention/calibration
branch.  It should not become another broad threshold or model swap loop.

## Next Step

Run M806.1 worst-surface abstention:

1. Train the same logistic harmful-aware selector family.
2. Add an abstention head or rule using inference-available uncertainty
   features: margin spread, harmful probability, positive probability,
   score-family disagreement, and cross-zero move size.
3. Select thresholds against the worst validation surface, not macro utility.
4. Accept only if every held-out surface is clean and at least two surfaces
   preserve non-trivial utility.

## Stop Conditions

Stop this generated-bundle repair line if:

1. Worst-surface abstention keeps only trivial coverage on all surfaces.
2. Utility comes from one surface while another surface requires near-zero
   application to stay clean.
3. Broader replay loses dense-overlap, CUB, MAP, NDCG, or MRR floors.
4. The best clean result requires per-surface or dataset-specific thresholds.

## Decision

Keep the route for one abstention-focused iteration.  Do not promote M803 or
M805 directly to broader replay.
