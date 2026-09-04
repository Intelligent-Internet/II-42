# M771 Online Accept Ceiling

M771 estimates the ceiling for a native online accept/fallback
policy by comparing current guards with qrels-derived oracle
selection inside the same M758 proposal pool.

## Test Aggregate

| Policy | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m758_all | 0 | 0 | 226.7 | -0.000552 | -0.000328 | -0.000554 | +0.000098 | +0.000000 | -0.000942 |
| m764_guard | 0 | 0 | 57.7 | +0.000102 | +0.000113 | +0.000123 | +0.000028 | +0.000000 | +0.000253 |
| m768_guard | 1 | 1 | 6.0 | +0.000005 | +0.000000 | +0.000000 | +0.000005 | +0.000000 | +0.000007 |
| strict_safe_oracle | 1 | 1 | 203.0 | +0.000261 | +0.000227 | +0.000123 | +0.000021 | +0.000000 | +0.000523 |
| strict_positive_oracle | 1 | 1 | 26.0 | +0.000261 | +0.000227 | +0.000123 | +0.000016 | +0.000000 | +0.000520 |
| utility_positive_oracle | 1 | 1 | 29.0 | +0.000259 | +0.000227 | +0.000123 | +0.000100 | +0.000000 | +0.000560 |

## Decision

M771 shows a real oracle accept ceiling; train or engineer a richer accept/fallback selector before changing proposals.
