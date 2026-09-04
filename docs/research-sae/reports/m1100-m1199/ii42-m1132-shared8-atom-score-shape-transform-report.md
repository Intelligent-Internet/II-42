# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `promote_score_shape_candidate`

## Selected Transform

- Train-selected: `a0.75_g0.75`
- Heldout-best: `a0.75_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.685795 | 0.669946 | 0.580956 | 0.446006 |
| heldout-best | 0.696680 | 0.674602 | 0.585791 | 0.452213 |
| fixed a0.50 g1.00 | 0.658735 | 0.673472 | 0.578677 | 0.441150 |
| fixed a0.60 g1.00 | 0.670749 | 0.667014 | 0.578412 | 0.439473 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.027060 | -0.003526 | +0.002279 | +0.004857 | 6 |
| `baseline_060` | +0.015047 | +0.002932 | +0.002544 | +0.006533 | 5 |

## Interpretation

A train-selected global atom-score shape beats fixed alpha 0.60 on all heldout macro metrics. This should be replayed on a broader native surface before changing the training objective.
