# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g1.25`
- Heldout-best: `a0.50_g0.75`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.711665 | 0.568100 | 0.510718 | 0.409079 |
| heldout-best | 0.724981 | 0.598489 | 0.526144 | 0.423195 |
| fixed a0.50 g1.00 | 0.706753 | 0.593344 | 0.521901 | 0.421122 |
| fixed a0.60 g1.00 | 0.715765 | 0.586294 | 0.520127 | 0.416328 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.004913 | -0.025243 | -0.011183 | -0.012043 | 4 |
| `baseline_060` | -0.004099 | -0.018194 | -0.009408 | -0.007249 | 5 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
