# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `210`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 109 |
| 0.250000 | 20 |
| 0.300000 | 4 |
| 0.500000 | 13 |
| 0.750000 | 64 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.026147 | +0.027013 | +0.020748 | +0.019590 | clean_recall_gain:41, mixed_gain_damage:6, pure_top_damage:32, top_gain_no_recall:43, unchanged:88 |
| 0.25 -> 0.30 | +0.000496 | +0.001915 | -0.001329 | -0.000454 | clean_recall_gain:11, mixed_gain_damage:1, pure_top_damage:31, top_gain_no_recall:50, unchanged:117 |
| 0.30 -> 0.50 | +0.010264 | -0.003479 | +0.002853 | +0.004687 | clean_recall_gain:19, mixed_gain_damage:8, pure_top_damage:48, top_gain_no_recall:43, unchanged:92 |
| 0.50 -> 0.75 | +0.002938 | -0.010493 | -0.009733 | -0.002471 | clean_recall_gain:16, mixed_gain_damage:6, pure_top_damage:59, top_gain_no_recall:46, unchanged:83 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 8 | 8 | 14 |
| `cqadupstack` | 2 | 1 | 4 | 8 | 15 |
| `fiqa` | 5 | 0 | 3 | 7 | 15 |
| `nfcorpus` | 9 | 2 | 2 | 4 | 13 |
| `scidocs` | 2 | 1 | 11 | 12 | 4 |
| `scifact` | 2 | 0 | 1 | 1 | 26 |
| `trec-covid` | 10 | 2 | 3 | 0 | 0 |
| `webis-touche2020` | 11 | 0 | 0 | 3 | 1 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 5 | 0 | 25 |
| `cqadupstack` | 0 | 0 | 4 | 7 | 19 |
| `fiqa` | 0 | 0 | 2 | 10 | 18 |
| `nfcorpus` | 1 | 0 | 3 | 8 | 18 |
| `scidocs` | 0 | 0 | 10 | 10 | 10 |
| `scifact` | 0 | 0 | 3 | 2 | 25 |
| `trec-covid` | 7 | 1 | 4 | 3 | 0 |
| `webis-touche2020` | 3 | 0 | 0 | 10 | 2 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 9 | 3 | 18 |
| `cqadupstack` | 0 | 0 | 9 | 7 | 14 |
| `fiqa` | 3 | 0 | 4 | 8 | 15 |
| `nfcorpus` | 2 | 2 | 4 | 8 | 14 |
| `scidocs` | 0 | 1 | 16 | 7 | 6 |
| `scifact` | 1 | 0 | 3 | 2 | 24 |
| `trec-covid` | 8 | 4 | 3 | 0 | 0 |
| `webis-touche2020` | 5 | 1 | 0 | 8 | 1 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 8 | 4 | 18 |
| `cqadupstack` | 0 | 0 | 11 | 6 | 13 |
| `fiqa` | 1 | 0 | 6 | 9 | 14 |
| `nfcorpus` | 3 | 1 | 6 | 9 | 11 |
| `scidocs` | 0 | 2 | 18 | 4 | 6 |
| `scifact` | 0 | 0 | 6 | 4 | 20 |
| `trec-covid` | 6 | 2 | 4 | 3 | 0 |
| `webis-touche2020` | 6 | 1 | 0 | 7 | 1 |
