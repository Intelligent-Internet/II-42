# M772 Accept Selector Smoke

M772 trains global accept selectors on dev surfaces and replays one
dev-selected threshold on the matching test surfaces.

## Test Results

| Model | Label | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hgb | query_utility_positive | 1 | 0 | 6.0 | +0.000028 | +0.000073 | +0.000000 | +0.000014 | +0.000000 | +0.000107 |
| hgb | query_strict_positive | 0 | 0 | 10.3 | +0.000034 | +0.000062 | +0.000000 | +0.000011 | +0.000000 | +0.000102 |
| logistic | query_strict_positive | 0 | 0 | 36.0 | +0.000028 | +0.000068 | +0.000000 | +0.000011 | +0.000000 | +0.000102 |
| logistic | query_utility_positive | 0 | 0 | 4.0 | -0.000005 | +0.000059 | +0.000000 | +0.000000 | +0.000000 | +0.000054 |

## Decision

M772 found no trained selector that passes multi-surface test; online accept needs stronger features or native feedback.
