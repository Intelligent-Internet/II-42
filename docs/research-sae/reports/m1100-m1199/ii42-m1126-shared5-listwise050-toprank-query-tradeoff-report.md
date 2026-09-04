# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 73 |
| 0.250000 | 21 |
| 0.300000 | 7 |
| 0.500000 | 8 |
| 0.750000 | 41 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.036138 | +0.032332 | +0.027123 | +0.025681 | clean_recall_gain:21, mixed_gain_damage:1, pure_top_damage:21, top_gain_no_recall:47, unchanged:60 |
| 0.25 -> 0.30 | +0.001537 | +0.006099 | +0.003131 | +0.006660 | clean_recall_gain:3, pure_top_damage:19, top_gain_no_recall:34, unchanged:94 |
| 0.30 -> 0.50 | +0.010637 | +0.003861 | -0.000631 | -0.001362 | clean_recall_gain:5, mixed_gain_damage:1, pure_top_damage:31, top_gain_no_recall:34, unchanged:79 |
| 0.50 -> 0.75 | +0.019374 | +0.002895 | -0.000942 | +0.000382 | clean_recall_gain:9, mixed_gain_damage:2, pure_top_damage:32, top_gain_no_recall:33, unchanged:74 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 5 | 12 | 13 |
| `fiqa` | 1 | 1 | 3 | 12 | 13 |
| `nfcorpus` | 13 | 0 | 4 | 6 | 7 |
| `scidocs` | 5 | 0 | 8 | 13 | 4 |
| `scifact` | 2 | 0 | 1 | 4 | 23 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 3 | 1 | 26 |
| `fiqa` | 0 | 0 | 3 | 8 | 19 |
| `nfcorpus` | 2 | 0 | 7 | 8 | 13 |
| `scidocs` | 1 | 0 | 6 | 14 | 9 |
| `scifact` | 0 | 0 | 0 | 3 | 27 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 4 | 1 | 25 |
| `fiqa` | 4 | 0 | 5 | 7 | 14 |
| `nfcorpus` | 1 | 1 | 7 | 10 | 11 |
| `scidocs` | 0 | 0 | 11 | 12 | 7 |
| `scifact` | 0 | 0 | 4 | 4 | 22 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 7 | 1 | 22 |
| `fiqa` | 2 | 0 | 2 | 10 | 16 |
| `nfcorpus` | 4 | 1 | 9 | 9 | 7 |
| `scidocs` | 3 | 1 | 11 | 11 | 4 |
| `scifact` | 0 | 0 | 3 | 2 | 25 |
