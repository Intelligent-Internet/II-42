# M797 Generated Selector Threshold Oracle

M797 keeps the M796 trained selectors fixed, then sweeps thresholds
on the test split as an oracle diagnostic.  This is not a deployable
result; it separates threshold transfer failure from score-ordering
failure.

| Score | Model | BestAny Gate | BestAny Clean | BestAny Applied | BestAny dMAP | BestAny dNDCG | BestAny Utility | Clean Gate | Clean Clean | Clean Applied | Clean dMAP | Clean dNDCG | Clean Utility |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| score_mean_margin | logistic | 0 | 0 | 98.7 | +0.000139 | +0.000454 | +0.000605 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |
| score_mean_margin | hgb | 1 | 1 | 1.7 | +0.000014 | +0.000015 | +0.000031 | 1 | 1 | 1.7 | +0.000014 | +0.000015 | +0.000031 |
| margin_bundle | logistic | 0 | 0 | 134.7 | +0.000130 | +0.000550 | +0.000701 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |
| margin_bundle | hgb | 1 | 1 | 3.3 | +0.000001 | +0.000031 | +0.000032 | 1 | 1 | 3.3 | +0.000001 | +0.000031 | +0.000032 |
| conservative_margin | logistic | 0 | 0 | 104.3 | +0.000110 | +0.000564 | +0.000670 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |
| conservative_margin | hgb | 1 | 0 | 66.3 | +0.000167 | +0.000201 | +0.000401 | 0 | 0 | 0.0 | +0.000000 | +0.000000 | +0.000000 |

## Decision

M797 found only tiny clean test operating points. Selector ordering is weak; richer observable features are still needed.
