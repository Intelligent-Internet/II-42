# M790 Vector Delta Target Oracle Shared15

M790 directly constructs a query-vector delta target from qrels
positives outside P1 top100 and hard top100 false positives. It
tests whether a model-level compiler has a clean target before any
training is attempted.

## Summary

| Mode | Eligible | Clean Boundary | Clean Utility | Unsafe Boundary | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| support_masked | 65 | 0 | 31 | 19 | +0.012280 | +0.010694 | +0.053783 | +0.002851 | +0.022229 | -0.036769 | +0.034659 |
| full_vector | 65 | 0 | 31 | 19 | +0.012280 | +0.010694 | +0.053783 | +0.002851 | +0.022229 | -0.036769 | +0.034659 |

## Decision

M790 found clean vector-level ranking targets but no clean boundary targets. Treat this as ranking-side supervision only; do not claim first-stage boundary recovery.
