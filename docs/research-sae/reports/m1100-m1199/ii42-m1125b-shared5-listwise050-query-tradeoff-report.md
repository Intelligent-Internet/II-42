# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 84 |
| 0.250000 | 17 |
| 0.300000 | 4 |
| 0.500000 | 16 |
| 0.750000 | 29 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.028352 | +0.021715 | +0.018151 | +0.011597 | clean_recall_gain:15, mixed_gain_damage:1, pure_top_damage:30, top_gain_no_recall:41, unchanged:63 |
| 0.25 -> 0.30 | -0.000094 | +0.002222 | +0.002835 | +0.002175 | clean_recall_gain:1, mixed_gain_damage:1, pure_top_damage:18, top_gain_no_recall:33, unchanged:97 |
| 0.30 -> 0.50 | +0.017174 | +0.002951 | -0.003505 | +0.003671 | clean_recall_gain:6, mixed_gain_damage:3, pure_top_damage:36, top_gain_no_recall:31, unchanged:74 |
| 0.50 -> 0.75 | +0.007015 | -0.007188 | -0.009701 | -0.011272 | clean_recall_gain:4, mixed_gain_damage:4, pure_top_damage:44, top_gain_no_recall:22, unchanged:76 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 10 | 8 | 12 |
| `fiqa` | 3 | 0 | 2 | 10 | 15 |
| `nfcorpus` | 7 | 0 | 7 | 8 | 8 |
| `scidocs` | 3 | 1 | 10 | 12 | 4 |
| `scifact` | 2 | 0 | 1 | 3 | 24 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 1 | 1 | 28 |
| `fiqa` | 0 | 0 | 2 | 8 | 20 |
| `nfcorpus` | 1 | 1 | 6 | 8 | 14 |
| `scidocs` | 0 | 0 | 9 | 14 | 7 |
| `scifact` | 0 | 0 | 0 | 2 | 28 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 9 | 1 | 20 |
| `fiqa` | 4 | 0 | 5 | 8 | 13 |
| `nfcorpus` | 1 | 2 | 9 | 7 | 11 |
| `scidocs` | 1 | 1 | 12 | 11 | 5 |
| `scifact` | 0 | 0 | 1 | 4 | 25 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 6 | 0 | 24 |
| `fiqa` | 2 | 1 | 5 | 8 | 14 |
| `nfcorpus` | 2 | 1 | 9 | 7 | 11 |
| `scidocs` | 0 | 2 | 18 | 6 | 4 |
| `scifact` | 0 | 0 | 6 | 1 | 23 |
