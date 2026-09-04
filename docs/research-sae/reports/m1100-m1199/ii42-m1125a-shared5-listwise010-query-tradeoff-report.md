# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 101 |
| 0.250000 | 13 |
| 0.300000 | 6 |
| 0.500000 | 11 |
| 0.750000 | 19 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.010173 | +0.014386 | +0.009754 | +0.011976 | clean_recall_gain:11, pure_top_damage:31, top_gain_no_recall:32, unchanged:76 |
| 0.25 -> 0.30 | +0.003890 | -0.004928 | -0.005275 | -0.005861 | clean_recall_gain:3, pure_top_damage:26, top_gain_no_recall:19, unchanged:102 |
| 0.30 -> 0.50 | +0.003113 | -0.007998 | -0.005864 | -0.005026 | clean_recall_gain:3, mixed_gain_damage:2, pure_top_damage:41, top_gain_no_recall:24, unchanged:80 |
| 0.50 -> 0.75 | +0.000991 | -0.019400 | -0.015131 | -0.010747 | clean_recall_gain:5, mixed_gain_damage:1, pure_top_damage:54, top_gain_no_recall:14, unchanged:76 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 6 | 7 | 17 |
| `fiqa` | 3 | 0 | 3 | 7 | 17 |
| `nfcorpus` | 6 | 0 | 6 | 6 | 12 |
| `scidocs` | 2 | 0 | 14 | 9 | 5 |
| `scifact` | 0 | 0 | 2 | 3 | 25 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 5 | 1 | 24 |
| `fiqa` | 1 | 0 | 3 | 5 | 21 |
| `nfcorpus` | 2 | 0 | 5 | 3 | 20 |
| `scidocs` | 0 | 0 | 11 | 10 | 9 |
| `scifact` | 0 | 0 | 2 | 0 | 28 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 11 | 1 | 18 |
| `fiqa` | 1 | 0 | 4 | 8 | 17 |
| `nfcorpus` | 2 | 1 | 3 | 8 | 16 |
| `scidocs` | 0 | 1 | 18 | 5 | 6 |
| `scifact` | 0 | 0 | 5 | 2 | 23 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 13 | 0 | 17 |
| `fiqa` | 0 | 0 | 8 | 7 | 15 |
| `nfcorpus` | 5 | 0 | 11 | 3 | 11 |
| `scidocs` | 0 | 1 | 17 | 4 | 8 |
| `scifact` | 0 | 0 | 5 | 0 | 25 |
