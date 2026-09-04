# M800 Interaction Selector Threshold Oracle

M800 keeps M799 interaction-aware selector scores fixed and sweeps
test thresholds as an oracle diagnostic.  This is not deployable; it
separates calibration failure from score-ordering failure.

| Score | Model | BestAny Gate | BestAny Clean | BestAny Applied | BestAny dMAP | BestAny dNDCG | BestAny Utility | Clean Gate | Clean Clean | Clean Applied | Clean dMAP | Clean dNDCG | Clean Utility |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| score_mean_margin | logistic | 1 | 1 | 1.3 | +0.000005 | +0.000000 | +0.000005 | 1 | 1 | 1.3 | +0.000005 | +0.000000 | +0.000005 |
| score_mean_margin | hgb | 0 | 0 | 77.3 | +0.000084 | +0.000111 | +0.000212 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |
| margin_bundle | logistic | 1 | 0 | 60.3 | +0.000063 | +0.000148 | +0.000239 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |
| margin_bundle | hgb | 1 | 1 | 4.0 | +0.000010 | +0.000023 | +0.000046 | 1 | 1 | 4.0 | +0.000010 | +0.000023 | +0.000046 |
| conservative_margin | logistic | 1 | 1 | 1.3 | +0.000005 | +0.000000 | +0.000005 | 1 | 1 | 1.3 | +0.000005 | +0.000000 | +0.000005 |
| conservative_margin | hgb | 1 | 0 | 63.3 | +0.000147 | +0.000441 | +0.000648 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |

## Decision

M800 found only tiny clean operating points. Interaction features help ranking but still need a stronger safety objective.
