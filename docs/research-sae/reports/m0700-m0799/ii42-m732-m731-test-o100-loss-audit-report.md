# M732 M731 Test O@100 Loss Audit

Status: `broad_test_o100_instability`

M732 inspects the M731 test split where low-rank oracle
reconstruction failed dense overlap@100 despite the per-query ridge
oracle passing.  It compares exact dense top100 membership under P1,
ridge oracle, and PCA oracle reconstructions.

## Rank Summary

| Rank | Q | Loss Q | Gain Q | Lost docs | Net dO@100 | Mean rec L2 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `r128` | 54 | 18 | 18 | 20 | -0.020000 | 0.011842 |
| `r64` | 54 | 17 | 16 | 19 | -0.030000 | 0.012777 |

## Dataset Summary

| Source/Dataset | Q | Loss Q | Gain Q | Lost docs | Net dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `r128:arguana` | 15 | 5 | 6 | 5 | 0.010000 |
| `r128:cqadupstack` | 15 | 2 | 4 | 2 | 0.020000 |
| `r128:fiqa` | 17 | 8 | 6 | 10 | -0.040000 |
| `r128:scidocs` | 7 | 3 | 2 | 3 | -0.010000 |
| `r64:arguana` | 15 | 5 | 5 | 5 | 0.000000 |
| `r64:cqadupstack` | 15 | 3 | 4 | 3 | 0.010000 |
| `r64:fiqa` | 17 | 6 | 5 | 8 | -0.030000 |
| `r64:scidocs` | 7 | 3 | 2 | 3 | -0.010000 |

## Worst Queries

| Source | Dataset | Query | dO@100 | Lost | Gained | Ridge dO@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `pca_oracle_r64` | `fiqa` | `594` | -0.020000 | 2 | 0 | 0.000000 |
| `pca_oracle_r128` | `fiqa` | `594` | -0.020000 | 2 | 0 | 0.000000 |
| `pca_oracle_r64` | `fiqa` | `4946` | -0.010000 | 2 | 1 | 0.000000 |
| `pca_oracle_r128` | `fiqa` | `4946` | -0.010000 | 2 | 1 | 0.000000 |
| `pca_oracle_r64` | `arguana` | `test-environment-aiahwagit-con01a` | -0.010000 | 1 | 0 | 0.000000 |
| `pca_oracle_r64` | `arguana` | `test-health-dhgsshbesbc-pro02a` | -0.010000 | 1 | 0 | 0.000000 |
| `pca_oracle_r64` | `arguana` | `test-health-hpehwadvoee-con04a` | -0.010000 | 1 | 0 | -0.010000 |
| `pca_oracle_r128` | `arguana` | `test-environment-aiahwagit-con01a` | -0.010000 | 1 | 0 | 0.000000 |
| `pca_oracle_r128` | `arguana` | `test-health-dhgsshbesbc-pro02a` | -0.010000 | 1 | 0 | 0.000000 |
| `pca_oracle_r128` | `arguana` | `test-health-hpehwadvoee-con04a` | -0.010000 | 1 | 0 | -0.010000 |
| `pca_oracle_r64` | `cqadupstack` | `android_79085` | -0.010000 | 1 | 0 | -0.010000 |
| `pca_oracle_r128` | `cqadupstack` | `android_79085` | -0.010000 | 1 | 0 | -0.010000 |

## Decision

```json
{
  "loss_queries_by_rank": {
    "r128": 18,
    "r64": 17
  },
  "status": "broad_test_o100_instability"
}
```

## Conclusion

The M731 test failure is broad enough that another global PCA-style
query-side compiler is unlikely to be the next efficient move.
