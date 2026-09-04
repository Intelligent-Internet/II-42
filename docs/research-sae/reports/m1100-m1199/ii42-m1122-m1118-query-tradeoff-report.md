# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `150`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 106 |
| 0.250000 | 18 |
| 0.300000 | 5 |
| 0.500000 | 8 |
| 0.750000 | 13 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.002400 | +0.030477 | +0.018816 | +0.021483 | clean_recall_gain:10, pure_top_damage:25, top_gain_no_recall:27, unchanged:88 |
| 0.25 -> 0.30 | +0.000000 | -0.001730 | -0.004565 | -0.001054 | pure_top_damage:15, top_gain_no_recall:17, unchanged:118 |
| 0.30 -> 0.50 | +0.011444 | -0.020333 | -0.012483 | -0.012221 | clean_recall_gain:4, pure_top_damage:42, top_gain_no_recall:16, unchanged:88 |
| 0.50 -> 0.75 | +0.001852 | -0.017467 | -0.015793 | -0.011271 | clean_recall_gain:2, pure_top_damage:49, top_gain_no_recall:13, unchanged:86 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 6 | 6 | 18 |
| `fiqa` | 3 | 0 | 3 | 7 | 17 |
| `nfcorpus` | 5 | 0 | 5 | 3 | 17 |
| `scidocs` | 2 | 0 | 9 | 10 | 9 |
| `scifact` | 0 | 0 | 2 | 1 | 27 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 3 | 0 | 27 |
| `fiqa` | 0 | 0 | 2 | 5 | 23 |
| `nfcorpus` | 0 | 0 | 4 | 3 | 23 |
| `scidocs` | 0 | 0 | 4 | 9 | 17 |
| `scifact` | 0 | 0 | 2 | 0 | 28 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 11 | 3 | 16 |
| `fiqa` | 4 | 0 | 5 | 5 | 16 |
| `nfcorpus` | 0 | 0 | 6 | 4 | 20 |
| `scidocs` | 0 | 0 | 16 | 4 | 10 |
| `scifact` | 0 | 0 | 4 | 0 | 26 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 10 | 1 | 19 |
| `fiqa` | 0 | 0 | 10 | 6 | 14 |
| `nfcorpus` | 1 | 0 | 8 | 2 | 19 |
| `scidocs` | 1 | 0 | 16 | 4 | 9 |
| `scifact` | 0 | 0 | 5 | 0 | 25 |
