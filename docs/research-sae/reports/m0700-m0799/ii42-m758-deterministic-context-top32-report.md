# M758 Deterministic Native-Context Policy

M758 searches auditable deterministic policies over global proposal
rows. Policy selection uses dev labels only for threshold choice;
test replay uses deployment-available native-context features.

## Top Policies

| Rank | Policy | Gate | Applied | Accepted | Best | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | `margin_bundle_o128_0.99_o256_None_md100_None` | 1 | 72 | 13 | 2 | +0.000000 | +0.000057 | +0.000684 | +0.000066 | +0.000012 | +0.000000 |
| 2 | `margin_bundle_o128_0.99_o256_None_md100_-0.001` | 1 | 72 | 13 | 2 | +0.000000 | +0.000057 | +0.000684 | +0.000066 | +0.000012 | +0.000000 |
| 3 | `margin_bundle_o128_0.99_o256_0.98_md100_None` | 1 | 72 | 13 | 2 | +0.000000 | +0.000056 | +0.000669 | +0.000066 | +0.000012 | +0.000000 |
| 4 | `margin_bundle_o128_0.99_o256_0.98_md100_-0.001` | 1 | 72 | 13 | 2 | +0.000000 | +0.000056 | +0.000669 | +0.000066 | +0.000012 | +0.000000 |
| 5 | `margin_bundle_o128_0.99_o256_None_md100_0.0` | 1 | 100 | 15 | 3 | +0.000000 | +0.000061 | +0.000661 | +0.000066 | +0.000012 | +0.000000 |
| 6 | `margin_bundle_o128_0.99_o256_None_md100_0.0001` | 1 | 96 | 14 | 2 | +0.000000 | +0.000051 | +0.000661 | +0.000066 | +0.000005 | +0.000000 |
| 7 | `margin_bundle_o128_0.99_o256_0.98_md100_0.0` | 1 | 100 | 15 | 3 | +0.000000 | +0.000060 | +0.000645 | +0.000066 | +0.000012 | +0.000000 |
| 8 | `margin_bundle_o128_0.99_o256_0.98_md100_0.0001` | 1 | 96 | 14 | 2 | +0.000000 | +0.000050 | +0.000645 | +0.000066 | +0.000005 | +0.000000 |
| 9 | `score_mean_margin_o128_1.0_o256_None_md100_None` | 1 | 245 | 28 | 6 | +0.000000 | +0.000137 | +0.000195 | +0.000000 | +0.000034 | +0.000000 |
| 10 | `score_mean_margin_o128_1.0_o256_0.98_md100_None` | 1 | 245 | 28 | 6 | +0.000000 | +0.000137 | +0.000195 | +0.000000 | +0.000034 | +0.000000 |
| 11 | `score_mean_margin_o128_0.99_o256_0.99_md100_None` | 1 | 263 | 25 | 4 | +0.000000 | +0.000094 | +0.000177 | +0.000000 | +0.000058 | +0.000000 |
| 12 | `score_mean_margin_o128_0.99_o256_0.99_md100_-0.001` | 1 | 263 | 25 | 4 | +0.000000 | +0.000094 | +0.000177 | +0.000000 | +0.000058 | +0.000000 |
| 13 | `score_mean_margin_o128_0.99_o256_None_md100_None` | 1 | 264 | 26 | 5 | +0.000000 | +0.000069 | +0.000171 | +0.000000 | +0.000058 | +0.000000 |
| 14 | `score_mean_margin_o128_0.99_o256_None_md100_-0.001` | 1 | 264 | 26 | 5 | +0.000000 | +0.000069 | +0.000171 | +0.000000 | +0.000058 | +0.000000 |
| 15 | `score_mean_margin_o128_0.99_o256_0.98_md100_None` | 1 | 264 | 26 | 5 | +0.000000 | +0.000069 | +0.000171 | +0.000000 | +0.000058 | +0.000000 |

## Decision

M758 found a qrels-free deterministic native-context policy with strict test gate pass and non-zero ranking gain.

## Best Policy

```json
{
  "margin_delta_100": null,
  "overlap_128": 0.99,
  "overlap_256": null,
  "score_name": "margin_bundle",
  "threshold": 1.3649893760681153
}
```
