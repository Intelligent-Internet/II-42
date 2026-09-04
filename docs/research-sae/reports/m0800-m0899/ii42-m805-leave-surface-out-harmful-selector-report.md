# M805 Leave-Surface-Out Harmful Selector

M805 trains harmful-aware generated-bundle selectors on two surfaces
and evaluates the selected threshold on the held-out surface.  This
tests whether the M803 clean point is robust or surface-specific.

## Best Clean Per Held-Out Surface

| Held-out | Score | Model | Lambda | Gate | Clean | Applied | dMAP | dNDCG | dCUB | dO@100 | Utility |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | score_mean_margin | logistic | 2.0 | 1 | 1 | 20.0 | +0.000155 | +0.000118 | +0.000000 | +0.000000 | +0.000286 |
| seed7642 | score_mean_margin | logistic | 2.0 | 1 | 1 | 16.0 | +0.000133 | +0.000113 | +0.000000 | +0.000000 | +0.000271 |
| seed7643 | conservative_margin | logistic | 0.0 | 1 | 1 | 5.0 | +0.000007 | +0.000000 | +0.000050 | +0.000000 | +0.000032 |

## Top Cases

| Held-out | Score | Model | Lambda | Gate | Clean | Applied | dMAP | dNDCG | Utility | Negative Tasks |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | score_mean_margin | logistic | 2.0 | 1 | 1 | 20.0 | +0.000155 | +0.000118 | +0.000286 | None |
| seed7642 | score_mean_margin | logistic | 2.0 | 1 | 1 | 16.0 | +0.000133 | +0.000113 | +0.000271 | None |
| original | margin_bundle | logistic | 5.0 | 1 | 1 | 44.0 | +0.000082 | +0.000066 | +0.000161 | None |
| original | margin_bundle | logistic | 3.0 | 1 | 1 | 28.0 | +0.000080 | +0.000066 | +0.000159 | None |
| seed7642 | conservative_margin | hgb | 0.0 | 1 | 1 | 32.0 | +0.000041 | +0.000072 | +0.000148 | None |
| original | conservative_margin | logistic | 5.0 | 1 | 1 | 42.0 | +0.000067 | +0.000066 | +0.000146 | None |
| original | score_mean_margin | logistic | 3.0 | 1 | 1 | 27.0 | +0.000066 | +0.000066 | +0.000145 | None |
| original | score_mean_margin | logistic | 5.0 | 1 | 1 | 37.0 | +0.000066 | +0.000066 | +0.000145 | None |
| original | score_mean_margin | logistic | 0.5 | 1 | 1 | 5.0 | +0.000092 | +0.000052 | +0.000144 | None |
| original | score_mean_margin | logistic | 0.0 | 1 | 1 | 4.0 | +0.000012 | +0.000122 | +0.000140 | None |
| original | conservative_margin | logistic | 0.0 | 1 | 1 | 3.0 | +0.000012 | +0.000122 | +0.000134 | None |
| seed7643 | conservative_margin | logistic | 0.0 | 1 | 1 | 5.0 | +0.000007 | +0.000000 | +0.000032 | None |
| seed7642 | margin_bundle | logistic | 0.5 | 1 | 1 | 5.0 | +0.000000 | +0.000024 | +0.000024 | None |
| seed7643 | margin_bundle | logistic | 0.0 | 1 | 1 | 3.0 | +0.000003 | +0.000000 | +0.000022 | None |
| seed7643 | conservative_margin | logistic | 0.5 | 1 | 1 | 1.0 | +0.000001 | +0.000000 | +0.000020 | None |
| original | margin_bundle | logistic | 0.5 | 1 | 1 | 3.0 | +0.000010 | +0.000000 | +0.000010 | None |
| original | margin_bundle | logistic | 1.0 | 1 | 1 | 13.0 | +0.000010 | +0.000000 | +0.000010 | None |
| seed7642 | margin_bundle | logistic | 1.0 | 1 | 1 | 8.0 | +0.000003 | +0.000000 | +0.000003 | None |
| original | margin_bundle | logistic | 0.0 | 1 | 1 | 0.0 | +0.000000 | +0.000000 | +0.000000 | None |
| seed7642 | conservative_margin | hgb | 0.5 | 1 | 0 | 88.0 | +0.000167 | +0.000426 | +0.000652 | seed7642:climate-fever;dbpedia-entity |

## Decision

M805 found useful clean leave-surface-out points only for some surfaces. Keep the route, but add worst-surface abstention before broader replay.
