# M790 Vector Delta Target Oracle Smoke

M790 directly constructs a query-vector delta target from qrels
positives outside P1 top100 and hard top100 false positives. It
tests whether a model-level compiler has a clean target before any
training is attempted.

## Summary

| Mode | Eligible | Clean Boundary | Clean Utility | Unsafe Boundary | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| support_masked | 24 | 0 | 8 | 9 | +0.013644 | +0.014357 | +0.084012 | +0.005291 | +0.009406 | -0.035833 | +0.033763 |
| full_vector | 24 | 0 | 8 | 9 | +0.013644 | +0.014357 | +0.084012 | +0.005291 | +0.009406 | -0.035833 | +0.033763 |

## Decision

M790 found clean vector-level ranking targets but no clean boundary targets. Treat this as ranking-side supervision only; do not claim first-stage boundary recovery.
