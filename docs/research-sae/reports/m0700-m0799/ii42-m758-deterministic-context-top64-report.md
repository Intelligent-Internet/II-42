# M758 Deterministic Native-Context Policy

M758 searches auditable deterministic policies over global proposal
rows. Policy selection uses dev labels only for threshold choice;
test replay uses deployment-available native-context features.

## Top Policies

| Rank | Policy | Gate | Applied | Accepted | Best | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | `margin_bundle_o128_0.99_o256_1.0_md100_0.0001` | 1 | 197 | 18 | 2 | +0.000000 | +0.000177 | +0.000213 | +0.000000 | +0.000013 | +0.000000 |
| 2 | `margin_bundle_o128_None_o256_1.0_md100_0.0001` | 1 | 198 | 18 | 2 | +0.000000 | +0.000176 | +0.000213 | +0.000000 | +0.000013 | +0.000000 |
| 3 | `margin_bundle_o128_0.99_o256_1.0_md100_None` | 1 | 126 | 14 | 1 | +0.000000 | +0.000158 | +0.000191 | +0.000000 | +0.000014 | +0.000000 |
| 4 | `margin_bundle_o128_0.99_o256_1.0_md100_-0.001` | 1 | 126 | 14 | 1 | +0.000000 | +0.000158 | +0.000191 | +0.000000 | +0.000014 | +0.000000 |
| 5 | `margin_bundle_o128_None_o256_1.0_md100_None` | 1 | 134 | 14 | 1 | +0.000000 | +0.000157 | +0.000191 | +0.000000 | +0.000014 | +0.000000 |
| 6 | `margin_bundle_o128_None_o256_1.0_md100_-0.001` | 1 | 134 | 14 | 1 | +0.000000 | +0.000157 | +0.000191 | +0.000000 | +0.000014 | +0.000000 |
| 7 | `margin_bundle_o128_0.99_o256_1.0_md100_0.0` | 1 | 249 | 21 | 2 | +0.000000 | +0.000127 | +0.000174 | +0.000000 | +0.000013 | +0.000000 |
| 8 | `margin_bundle_o128_None_o256_1.0_md100_0.0` | 1 | 250 | 20 | 2 | +0.000000 | +0.000126 | +0.000174 | +0.000000 | +0.000013 | +0.000000 |
| 9 | `score_mean_margin_o128_0.99_o256_None_md100_None` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 10 | `score_mean_margin_o128_0.99_o256_None_md100_-0.001` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 11 | `score_mean_margin_o128_0.99_o256_0.98_md100_None` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 12 | `score_mean_margin_o128_0.99_o256_0.98_md100_-0.001` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 13 | `score_mean_margin_o128_0.99_o256_0.99_md100_None` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 14 | `score_mean_margin_o128_0.99_o256_0.99_md100_-0.001` | 1 | 270 | 24 | 1 | +0.000000 | +0.000037 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |
| 15 | `conservative_margin_o128_0.99_o256_None_md100_None` | 1 | 269 | 23 | 0 | +0.000000 | +0.000034 | +0.000153 | +0.000000 | +0.000043 | +0.000000 |

## Decision

M758 found a qrels-free deterministic native-context policy with strict test gate pass and non-zero ranking gain.

## Best Policy

```json
{
  "margin_delta_100": 0.0001,
  "overlap_128": 0.99,
  "overlap_256": 1.0,
  "score_name": "margin_bundle",
  "threshold": 0.364365565776825
}
```
