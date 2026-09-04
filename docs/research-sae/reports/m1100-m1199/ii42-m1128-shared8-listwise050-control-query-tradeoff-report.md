# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `210`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 112 |
| 0.250000 | 20 |
| 0.300000 | 12 |
| 0.500000 | 14 |
| 0.750000 | 52 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.014367 | +0.035037 | +0.034860 | +0.030603 | clean_recall_gain:37, mixed_gain_damage:5, pure_top_damage:35, top_gain_no_recall:44, unchanged:89 |
| 0.25 -> 0.30 | +0.000371 | +0.004476 | +0.002049 | +0.005162 | clean_recall_gain:15, mixed_gain_damage:2, pure_top_damage:23, top_gain_no_recall:44, unchanged:126 |
| 0.30 -> 0.50 | +0.011664 | +0.010342 | +0.002734 | +0.007301 | clean_recall_gain:24, mixed_gain_damage:3, pure_top_damage:52, top_gain_no_recall:37, unchanged:94 |
| 0.50 -> 0.75 | +0.015038 | -0.011100 | -0.006942 | -0.005447 | clean_recall_gain:20, mixed_gain_damage:3, pure_top_damage:61, top_gain_no_recall:33, unchanged:93 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 3 | 10 | 17 |
| `cqadupstack` | 2 | 0 | 5 | 7 | 16 |
| `fiqa` | 4 | 0 | 2 | 8 | 16 |
| `nfcorpus` | 8 | 1 | 5 | 7 | 9 |
| `scidocs` | 3 | 0 | 14 | 8 | 5 |
| `scifact` | 1 | 0 | 3 | 1 | 25 |
| `trec-covid` | 9 | 4 | 1 | 1 | 0 |
| `webis-touche2020` | 10 | 0 | 2 | 2 | 1 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 3 | 2 | 25 |
| `cqadupstack` | 2 | 0 | 2 | 3 | 23 |
| `fiqa` | 0 | 0 | 1 | 11 | 18 |
| `nfcorpus` | 3 | 0 | 3 | 5 | 19 |
| `scidocs` | 0 | 0 | 9 | 10 | 11 |
| `scifact` | 0 | 0 | 1 | 1 | 28 |
| `trec-covid` | 8 | 2 | 3 | 1 | 1 |
| `webis-touche2020` | 2 | 0 | 1 | 11 | 1 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 6 | 2 | 22 |
| `cqadupstack` | 4 | 0 | 5 | 3 | 18 |
| `fiqa` | 3 | 0 | 5 | 8 | 14 |
| `nfcorpus` | 4 | 0 | 8 | 7 | 11 |
| `scidocs` | 0 | 1 | 15 | 10 | 4 |
| `scifact` | 0 | 0 | 6 | 1 | 23 |
| `trec-covid` | 7 | 2 | 3 | 2 | 1 |
| `webis-touche2020` | 6 | 0 | 4 | 4 | 1 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 8 | 0 | 22 |
| `cqadupstack` | 4 | 0 | 10 | 3 | 13 |
| `fiqa` | 3 | 0 | 6 | 6 | 15 |
| `nfcorpus` | 1 | 1 | 7 | 9 | 12 |
| `scidocs` | 0 | 1 | 17 | 8 | 4 |
| `scifact` | 0 | 0 | 3 | 1 | 26 |
| `trec-covid` | 8 | 1 | 6 | 0 | 0 |
| `webis-touche2020` | 4 | 0 | 4 | 6 | 1 |
