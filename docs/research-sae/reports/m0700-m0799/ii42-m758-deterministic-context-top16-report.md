# M758 Deterministic Native-Context Policy

M758 searches auditable deterministic policies over global proposal
rows. Policy selection uses dev labels only for threshold choice;
test replay uses deployment-available native-context features.

## Top Policies

| Rank | Policy | Gate | Applied | Accepted | Best | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | `score_mean_margin_o128_0.99_o256_0.99_md100_0.0` | 1 | 224 | 26 | 10 | +0.000000 | +0.000246 | +0.000332 | +0.000000 | +0.000032 | +0.000000 |
| 2 | `score_mean_margin_o128_0.99_o256_None_md100_0.0` | 1 | 229 | 26 | 12 | +0.000000 | +0.000254 | +0.000215 | +0.000000 | +0.000036 | +0.000000 |
| 3 | `score_mean_margin_o128_0.99_o256_0.98_md100_0.0` | 1 | 229 | 26 | 12 | +0.000000 | +0.000254 | +0.000215 | +0.000000 | +0.000036 | +0.000000 |
| 4 | `margin_bundle_o128_None_o256_1.0_md100_0.0001` | 1 | 24 | 6 | 4 | +0.000000 | +0.000059 | +0.000156 | +0.000000 | +0.000000 | +0.000000 |
| 5 | `margin_bundle_o128_0.99_o256_1.0_md100_0.0001` | 1 | 23 | 6 | 4 | +0.000000 | +0.000059 | +0.000156 | +0.000000 | +0.000000 | +0.000000 |
| 6 | `margin_bundle_o128_None_o256_1.0_md100_None` | 1 | 31 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 7 | `margin_bundle_o128_None_o256_1.0_md100_-0.001` | 1 | 31 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 8 | `margin_bundle_o128_0.99_o256_1.0_md100_None` | 1 | 30 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 9 | `margin_bundle_o128_0.99_o256_1.0_md100_-0.001` | 1 | 30 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 10 | `margin_bundle_o128_None_o256_1.0_md100_0.0` | 1 | 28 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 11 | `margin_bundle_o128_0.99_o256_1.0_md100_0.0` | 1 | 27 | 6 | 4 | +0.000000 | +0.000062 | +0.000150 | +0.000000 | +0.000000 | +0.000000 |
| 12 | `conservative_margin_o128_None_o256_1.0_md100_0.0001` | 1 | 30 | 6 | 4 | +0.000000 | +0.000085 | +0.000100 | +0.000000 | +0.000000 | +0.000000 |
| 13 | `conservative_margin_o128_0.99_o256_1.0_md100_0.0001` | 1 | 25 | 6 | 4 | +0.000000 | +0.000085 | +0.000100 | +0.000000 | +0.000000 | +0.000000 |
| 14 | `conservative_margin_o128_None_o256_1.0_md100_0.0` | 1 | 44 | 6 | 4 | +0.000000 | +0.000084 | +0.000094 | +0.000000 | +0.000000 | +0.000000 |
| 15 | `conservative_margin_o128_0.99_o256_1.0_md100_0.0` | 1 | 39 | 6 | 4 | +0.000000 | +0.000084 | +0.000094 | +0.000000 | +0.000000 | +0.000000 |

## Decision

M758 found a qrels-free deterministic native-context policy with strict test gate pass and non-zero ranking gain.

## Best Policy

```json
{
  "margin_delta_100": 0.0,
  "overlap_128": 0.99,
  "overlap_256": 0.99,
  "score_name": "score_mean_margin",
  "threshold": 0.6226385235786438
}
```
