# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 109 |
| 0.250000 | 12 |
| 0.300000 | 3 |
| 0.500000 | 7 |
| 0.750000 | 19 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.021464 | +0.015616 | +0.012114 | +0.016652 | clean_recall_gain:8, mixed_gain_damage:1, pure_top_damage:37, top_gain_no_recall:24, unchanged:80 |
| 0.25 -> 0.30 | +0.005333 | -0.002660 | +0.001391 | +0.001132 | clean_recall_gain:1, pure_top_damage:24, top_gain_no_recall:18, unchanged:107 |
| 0.30 -> 0.50 | +0.012000 | -0.008456 | -0.008438 | -0.005428 | clean_recall_gain:4, pure_top_damage:41, top_gain_no_recall:18, unchanged:87 |
| 0.50 -> 0.75 | -0.003037 | -0.013767 | -0.013968 | -0.013283 | clean_recall_gain:2, mixed_gain_damage:1, pure_top_damage:53, top_gain_no_recall:16, unchanged:78 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 9 | 7 | 14 |
| `fiqa` | 2 | 1 | 4 | 7 | 16 |
| `nfcorpus` | 4 | 0 | 9 | 1 | 16 |
| `scidocs` | 1 | 0 | 15 | 7 | 7 |
| `scifact` | 1 | 0 | 0 | 2 | 27 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 5 | 0 | 25 |
| `fiqa` | 1 | 0 | 3 | 7 | 19 |
| `nfcorpus` | 0 | 0 | 5 | 3 | 22 |
| `scidocs` | 0 | 0 | 10 | 7 | 13 |
| `scifact` | 0 | 0 | 1 | 1 | 28 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 9 | 1 | 20 |
| `fiqa` | 3 | 0 | 7 | 7 | 13 |
| `nfcorpus` | 0 | 0 | 7 | 5 | 18 |
| `scidocs` | 1 | 0 | 17 | 4 | 8 |
| `scifact` | 0 | 0 | 1 | 1 | 28 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 11 | 1 | 18 |
| `fiqa` | 1 | 0 | 10 | 8 | 11 |
| `nfcorpus` | 1 | 0 | 7 | 3 | 19 |
| `scidocs` | 0 | 1 | 20 | 3 | 6 |
| `scifact` | 0 | 0 | 5 | 1 | 24 |
