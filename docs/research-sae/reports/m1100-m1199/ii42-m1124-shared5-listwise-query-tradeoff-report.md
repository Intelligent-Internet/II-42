# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 89 |
| 0.250000 | 19 |
| 0.300000 | 8 |
| 0.500000 | 12 |
| 0.750000 | 22 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.026325 | +0.017438 | +0.016332 | +0.017565 | clean_recall_gain:15, mixed_gain_damage:1, pure_top_damage:30, top_gain_no_recall:38, unchanged:66 |
| 0.25 -> 0.30 | +0.000000 | +0.000333 | +0.000542 | +0.000599 | pure_top_damage:21, top_gain_no_recall:28, unchanged:101 |
| 0.30 -> 0.50 | +0.009637 | -0.009577 | -0.002560 | -0.001996 | clean_recall_gain:8, mixed_gain_damage:2, pure_top_damage:36, top_gain_no_recall:23, unchanged:81 |
| 0.50 -> 0.75 | +0.013252 | +0.002476 | -0.003473 | -0.001245 | clean_recall_gain:7, mixed_gain_damage:1, pure_top_damage:43, top_gain_no_recall:21, unchanged:78 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 6 | 9 | 15 |
| `fiqa` | 2 | 0 | 3 | 10 | 15 |
| `nfcorpus` | 9 | 0 | 8 | 6 | 7 |
| `scidocs` | 3 | 1 | 11 | 11 | 4 |
| `scifact` | 1 | 0 | 2 | 2 | 25 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 3 | 2 | 25 |
| `fiqa` | 0 | 0 | 4 | 5 | 21 |
| `nfcorpus` | 0 | 0 | 5 | 9 | 16 |
| `scidocs` | 0 | 0 | 8 | 11 | 11 |
| `scifact` | 0 | 0 | 1 | 1 | 28 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 5 | 2 | 23 |
| `fiqa` | 3 | 0 | 4 | 7 | 16 |
| `nfcorpus` | 2 | 2 | 6 | 7 | 13 |
| `scidocs` | 3 | 0 | 17 | 6 | 4 |
| `scifact` | 0 | 0 | 4 | 1 | 25 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 7 | 1 | 22 |
| `fiqa` | 2 | 0 | 7 | 6 | 15 |
| `nfcorpus` | 5 | 1 | 11 | 3 | 10 |
| `scidocs` | 0 | 0 | 13 | 10 | 7 |
| `scifact` | 0 | 0 | 5 | 1 | 24 |
