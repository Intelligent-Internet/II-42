# ii42 M1115 Rank-Geometry Profile Selector

Verdict: `pass`

## Selector

- Threshold: `0.364855`
- Heldout selected-alt rate: `0.157480`
- Heldout selected-alt precision: `0.183333`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `selector` | 0.519152 | 0.405350 | 0.337828 | 0.269756 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

## Selector Delta Versus Base

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.006606 | +0.004146 | +0.002214 | +0.003627 |

## Per Dataset Heldout

| Dataset | Queries | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Alt Rate | Alt Precision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa` | 194 | +0.002577 | -0.001151 | +0.000958 | +0.001282 | 0.139175 | 0.148148 |
| `nfcorpus` | 97 | +0.000176 | +0.005948 | +0.000805 | -0.000672 | 0.072165 | 0.142857 |
| `scifact` | 90 | +0.022222 | +0.013619 | +0.006439 | +0.013314 | 0.288889 | 0.230769 |
