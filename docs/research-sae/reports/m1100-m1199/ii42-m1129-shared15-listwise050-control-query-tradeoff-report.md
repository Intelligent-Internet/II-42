# ii42 M1122 Query-Level Atom-Pressure Tradeoff Audit

- Split: `heldout`
- Query count: `403`
- Alphas: `[0.0, 0.25, 0.3, 0.5, 0.75]`

## Best Alpha Counts

| Alpha | Queries |
| ---: | ---: |
| 0.000000 | 241 |
| 0.250000 | 39 |
| 0.300000 | 15 |
| 0.500000 | 33 |
| 0.750000 | 75 |

## Transition Summary

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Classes |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.00 -> 0.25 | +0.016744 | +0.021398 | +0.024125 | +0.022624 | clean_recall_gain:47, mixed_gain_damage:5, pure_top_damage:60, top_gain_no_recall:92, unchanged:199 |
| 0.25 -> 0.30 | +0.000557 | +0.001035 | -0.000262 | +0.002692 | clean_recall_gain:15, mixed_gain_damage:2, pure_top_damage:50, top_gain_no_recall:72, unchanged:264 |
| 0.30 -> 0.50 | +0.002834 | +0.011952 | +0.002221 | +0.008437 | clean_recall_gain:21, mixed_gain_damage:5, pure_top_damage:82, top_gain_no_recall:75, unchanged:220 |
| 0.50 -> 0.75 | -0.000835 | -0.015570 | -0.011706 | -0.007987 | clean_recall_gain:20, mixed_gain_damage:1, pure_top_damage:110, top_gain_no_recall:60, unchanged:212 |

## Dataset Class Counts

### 0.00 -> 0.25

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 8 | 7 | 15 |
| `climate-fever` | 3 | 0 | 4 | 13 | 10 |
| `cqadupstack` | 1 | 1 | 7 | 8 | 13 |
| `dbpedia-entity` | 4 | 1 | 10 | 6 | 9 |
| `fever` | 0 | 0 | 0 | 2 | 28 |
| `fiqa` | 3 | 0 | 2 | 11 | 14 |
| `hotpotqa` | 1 | 0 | 2 | 7 | 20 |
| `msmarco` | 6 | 0 | 1 | 2 | 4 |
| `nfcorpus` | 3 | 0 | 8 | 7 | 12 |
| `nq` | 1 | 0 | 0 | 5 | 24 |
| `quora` | 0 | 0 | 2 | 4 | 24 |
| `scidocs` | 4 | 1 | 11 | 11 | 3 |
| `scifact` | 3 | 0 | 2 | 3 | 22 |
| `trec-covid` | 10 | 1 | 2 | 2 | 0 |
| `webis-touche2020` | 8 | 1 | 1 | 4 | 1 |

### 0.25 -> 0.30

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 2 | 2 | 26 |
| `climate-fever` | 0 | 0 | 8 | 10 | 12 |
| `cqadupstack` | 0 | 0 | 5 | 5 | 20 |
| `dbpedia-entity` | 0 | 1 | 10 | 7 | 12 |
| `fever` | 0 | 0 | 0 | 0 | 30 |
| `fiqa` | 0 | 0 | 1 | 7 | 22 |
| `hotpotqa` | 0 | 0 | 0 | 6 | 24 |
| `msmarco` | 3 | 0 | 1 | 2 | 7 |
| `nfcorpus` | 3 | 0 | 7 | 5 | 15 |
| `nq` | 0 | 0 | 0 | 2 | 28 |
| `quora` | 0 | 0 | 1 | 0 | 29 |
| `scidocs` | 0 | 0 | 6 | 11 | 13 |
| `scifact` | 0 | 0 | 2 | 3 | 25 |
| `trec-covid` | 8 | 1 | 4 | 2 | 0 |
| `webis-touche2020` | 1 | 0 | 3 | 10 | 1 |

### 0.30 -> 0.50

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 9 | 3 | 18 |
| `climate-fever` | 1 | 0 | 5 | 14 | 10 |
| `cqadupstack` | 0 | 0 | 10 | 4 | 16 |
| `dbpedia-entity` | 1 | 2 | 15 | 3 | 9 |
| `fever` | 0 | 0 | 0 | 2 | 28 |
| `fiqa` | 3 | 0 | 4 | 5 | 18 |
| `hotpotqa` | 0 | 0 | 1 | 7 | 22 |
| `msmarco` | 3 | 0 | 3 | 2 | 5 |
| `nfcorpus` | 2 | 0 | 8 | 7 | 13 |
| `nq` | 0 | 0 | 0 | 5 | 25 |
| `quora` | 0 | 0 | 4 | 0 | 26 |
| `scidocs` | 0 | 1 | 18 | 7 | 4 |
| `scifact` | 0 | 0 | 1 | 4 | 25 |
| `trec-covid` | 8 | 2 | 3 | 2 | 0 |
| `webis-touche2020` | 3 | 0 | 1 | 10 | 1 |

### 0.50 -> 0.75

| Dataset | clean_recall_gain | mixed_gain_damage | pure_top_damage | top_gain_no_recall | unchanged |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0 | 0 | 8 | 2 | 20 |
| `climate-fever` | 0 | 0 | 10 | 8 | 12 |
| `cqadupstack` | 1 | 0 | 10 | 5 | 14 |
| `dbpedia-entity` | 1 | 0 | 16 | 5 | 8 |
| `fever` | 0 | 0 | 0 | 0 | 30 |
| `fiqa` | 1 | 0 | 7 | 9 | 13 |
| `hotpotqa` | 0 | 0 | 5 | 3 | 22 |
| `msmarco` | 3 | 0 | 4 | 2 | 4 |
| `nfcorpus` | 3 | 0 | 7 | 6 | 14 |
| `nq` | 0 | 0 | 6 | 3 | 21 |
| `quora` | 0 | 0 | 3 | 0 | 27 |
| `scidocs` | 0 | 0 | 19 | 7 | 4 |
| `scifact` | 0 | 0 | 5 | 3 | 22 |
| `trec-covid` | 8 | 1 | 6 | 0 | 0 |
| `webis-touche2020` | 3 | 0 | 4 | 7 | 1 |
