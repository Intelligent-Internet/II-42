# ii42 M1113 Logistic Profile Selector

Verdict: `fail`

## Selector

- Threshold: `0.366778`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `selector` | 0.518942 | 0.405867 | 0.334700 | 0.266870 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

## Selector Delta Versus Base

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.006397 | +0.004663 | -0.000915 | +0.000741 |
