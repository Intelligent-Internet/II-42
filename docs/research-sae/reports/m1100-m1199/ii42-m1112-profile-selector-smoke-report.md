# ii42 M1112 Profile Selector Smoke

Verdict: `fail`

## Selector

- Feature: `lex_nonzero`
- Direction: `lt`
- Threshold: `37.000000`
- Train accuracy: `0.662921`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `selector` | 0.512426 | 0.402719 | 0.336195 | 0.267775 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

## Selector Delta Versus Base

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.000119 | +0.001515 | +0.000580 | +0.001645 |
