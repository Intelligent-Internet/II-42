# M791 Protected Projection Delta Oracle Shared15

M791 projects vector-level qrels deltas away from protected P1/dense
top-support document spans, then repeats the M790 clean-target audit.

## Summary

| Mode | Protected | Eligible | Clean Boundary | Clean Utility | Unsafe Boundary | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| support_masked | 64 | 65 | 0 | 26 | 20 | +0.005379 | +0.000000 | +0.070415 | +0.000769 | +0.017447 | -0.015538 | +0.014257 |
| support_masked | 128 | 65 | 0 | 18 | 11 | +0.002209 | +0.000000 | +0.042625 | +0.000000 | +0.016023 | -0.002000 | +0.010220 |
| full_vector | 64 | 65 | 0 | 26 | 20 | +0.005379 | +0.000000 | +0.070415 | +0.000769 | +0.017447 | -0.015538 | +0.014257 |
| full_vector | 128 | 65 | 0 | 18 | 11 | +0.002209 | +0.000000 | +0.042625 | +0.000000 | +0.016023 | -0.002000 | +0.010220 |

## Decision

M791 still only finds clean ranking targets. Protected projection reduces damage but does not solve first-stage boundary recovery.
