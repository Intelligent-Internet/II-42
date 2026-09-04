# M791 Protected Projection Delta Oracle Smoke

M791 projects vector-level qrels deltas away from protected P1/dense
top-support document spans, then repeats the M790 clean-target audit.

## Summary

| Mode | Protected | Eligible | Clean Boundary | Clean Utility | Unsafe Boundary | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| support_masked | 8 | 27 | 0 | 9 | 13 | +0.009540 | +0.002682 | +0.100315 | +0.004080 | +0.017991 | -0.045556 | +0.022034 |
| support_masked | 16 | 27 | 0 | 11 | 11 | +0.007148 | -0.001504 | +0.092197 | +0.001852 | +0.017271 | -0.034815 | +0.014650 |
| support_masked | 32 | 27 | 0 | 9 | 12 | +0.006071 | +0.000000 | +0.099246 | +0.000000 | +0.016207 | -0.031481 | +0.014175 |
| support_masked | 64 | 27 | 0 | 9 | 9 | +0.004668 | +0.000000 | +0.082456 | +0.000000 | +0.015332 | -0.017778 | +0.012333 |
| full_vector | 8 | 27 | 0 | 9 | 13 | +0.009540 | +0.002682 | +0.100315 | +0.004080 | +0.017991 | -0.045556 | +0.022034 |
| full_vector | 16 | 27 | 0 | 11 | 11 | +0.007148 | -0.001504 | +0.092197 | +0.001852 | +0.017271 | -0.034815 | +0.014650 |
| full_vector | 32 | 27 | 0 | 9 | 12 | +0.006071 | +0.000000 | +0.099246 | +0.000000 | +0.016207 | -0.031481 | +0.014175 |
| full_vector | 64 | 27 | 0 | 9 | 9 | +0.004668 | +0.000000 | +0.082456 | +0.000000 | +0.015332 | -0.017778 | +0.012333 |

## Decision

M791 still only finds clean ranking targets. Protected projection reduces damage but does not solve first-stage boundary recovery.
