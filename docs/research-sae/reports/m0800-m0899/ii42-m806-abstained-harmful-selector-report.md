# M806 Abstained Harmful Selector

M806 repeats the M805 leave-surface-out protocol, but adds an
explicit harmful-head abstention cap: selected rows must satisfy
`score >= threshold` and `p_harmful <= cap`.  The threshold and cap
are selected only on the non-held-out dev surfaces.

## Best Clean Per Held-Out Surface

| Held-out | Score | Model | Lambda | Cap | Gate | Clean | Applied | dMAP | dNDCG | dCUB | dO@100 | Utility |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | margin_bundle | logistic | 3.0 | 0.3328 | 1 | 1 | 26.0 | +0.000080 | +0.000066 | +0.000000 | +0.000000 | +0.000159 |
| seed7642 | score_mean_margin | logistic | 2.0 | 0.2315 | 1 | 1 | 15.0 | +0.000133 | +0.000113 | +0.000000 | +0.000000 | +0.000271 |
| seed7643 | margin_bundle | logistic | 0.0 | 0.3556 | 1 | 1 | 31.0 | +0.000038 | +0.000235 | +0.000123 | +0.000000 | +0.000334 |

## Top Cases

| Held-out | Score | Model | Lambda | Cap | Gate | Clean | Applied | dMAP | dNDCG | Utility | Negative Tasks |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| seed7643 | margin_bundle | logistic | 0.0 | 0.3556 | 1 | 1 | 31.0 | +0.000038 | +0.000235 | +0.000334 | None |
| seed7642 | score_mean_margin | logistic | 2.0 | 0.2315 | 1 | 1 | 15.0 | +0.000133 | +0.000113 | +0.000271 | None |
| original | margin_bundle | logistic | 3.0 | 0.3328 | 1 | 1 | 26.0 | +0.000080 | +0.000066 | +0.000159 | None |
| original | margin_bundle | logistic | 5.0 | 0.2572 | 1 | 1 | 43.0 | +0.000078 | +0.000066 | +0.000157 | None |
| original | conservative_margin | logistic | 5.0 | 0.3026 | 1 | 1 | 42.0 | +0.000067 | +0.000066 | +0.000146 | None |
| original | score_mean_margin | logistic | 2.0 | 0.3362 | 1 | 1 | 16.0 | +0.000066 | +0.000066 | +0.000145 | None |
| original | score_mean_margin | logistic | 3.0 | 0.2504 | 1 | 1 | 27.0 | +0.000066 | +0.000066 | +0.000145 | None |
| original | score_mean_margin | logistic | 5.0 | 0.2504 | 1 | 1 | 37.0 | +0.000066 | +0.000066 | +0.000145 | None |
| seed7642 | margin_bundle | logistic | 0.5 | 0.7747 | 1 | 1 | 5.0 | +0.000000 | +0.000024 | +0.000024 | None |
| original | margin_bundle | logistic | 0.0 | 0.7780 | 1 | 1 | 10.0 | +0.000015 | +0.000000 | +0.000015 | None |
| original | margin_bundle | logistic | 0.5 | 0.7780 | 1 | 1 | 3.0 | +0.000010 | +0.000000 | +0.000010 | None |
| original | margin_bundle | logistic | 1.0 | 0.7780 | 1 | 1 | 13.0 | +0.000010 | +0.000000 | +0.000010 | None |
| seed7642 | margin_bundle | logistic | 1.0 | 0.4648 | 1 | 1 | 9.0 | +0.000003 | +0.000000 | +0.000003 | None |
| seed7643 | margin_bundle | logistic | 5.0 | 0.2766 | 1 | 0 | 34.0 | +0.000070 | +0.000301 | +0.000446 | seed7643:msmarco |
| seed7643 | margin_bundle | logistic | 2.0 | 0.3556 | 1 | 0 | 27.0 | +0.000020 | +0.000235 | +0.000316 | seed7643:cqadupstack;msmarco |
| seed7643 | margin_bundle | logistic | 3.0 | 0.2766 | 1 | 0 | 28.0 | +0.000003 | +0.000235 | +0.000299 | seed7643:climate-fever;msmarco |
| seed7642 | margin_bundle | logistic | 2.0 | 0.4648 | 1 | 0 | 40.0 | +0.000131 | +0.000113 | +0.000269 | seed7642:scidocs |
| seed7642 | margin_bundle | logistic | 3.0 | 0.3176 | 1 | 0 | 50.0 | +0.000131 | +0.000113 | +0.000269 | seed7642:scidocs |
| seed7642 | score_mean_margin | logistic | 3.0 | 0.2315 | 1 | 0 | 19.0 | +0.000130 | +0.000113 | +0.000268 | seed7642:dbpedia-entity |
| seed7642 | score_mean_margin | logistic | 5.0 | 0.1579 | 1 | 0 | 31.0 | +0.000129 | +0.000113 | +0.000266 | seed7642:dbpedia-entity |

## Common Config Check

| Score | Model | Lambda | All Clean | Min Utility | Mean Utility | Applied Sum |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| margin_bundle | logistic | 2.0 | 0 | +0.000159 | +0.000248 | 107.0 |
| margin_bundle | logistic | 3.0 | 0 | +0.000159 | +0.000242 | 104.0 |
| margin_bundle | logistic | 5.0 | 0 | +0.000157 | +0.000287 | 129.0 |
| score_mean_margin | logistic | 3.0 | 0 | +0.000142 | +0.000185 | 72.0 |
| score_mean_margin | logistic | 5.0 | 0 | +0.000142 | +0.000184 | 94.0 |
| margin_bundle | logistic | 0.0 | 0 | +0.000015 | +0.000146 | 50.0 |
| margin_bundle | logistic | 0.5 | 0 | +0.000010 | +0.000116 | 44.0 |
| margin_bundle | logistic | 1.0 | 0 | +0.000003 | +0.000104 | 58.0 |
| score_mean_margin | logistic | 2.0 | 0 | -0.000003 | +0.000138 | 46.0 |
| conservative_margin | logistic | 1.0 | 0 | -0.000076 | -0.000035 | 123.0 |
| conservative_margin | logistic | 5.0 | 0 | -0.000095 | +0.000058 | 197.0 |
| score_mean_margin | logistic | 1.0 | 0 | -0.000099 | -0.000046 | 59.0 |

## Decision

M806 found useful clean abstained points for every held-out surface, but not under one common config. Abstention is real; the next step is score-family mixture/gating, not direct broader replay.
