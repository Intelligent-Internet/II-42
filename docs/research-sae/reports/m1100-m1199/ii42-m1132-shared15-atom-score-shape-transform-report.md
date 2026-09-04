# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g1.00`
- Heldout-best: `a0.50_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.771468 | 0.763056 | 0.689540 | 0.600224 |
| heldout-best | 0.773637 | 0.777965 | 0.702270 | 0.609422 |
| fixed a0.50 g1.00 | 0.770982 | 0.777186 | 0.699337 | 0.604800 |
| fixed a0.60 g1.00 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.000486 | -0.014129 | -0.009797 | -0.004576 | 13 |
| `baseline_060` | -0.000930 | -0.016715 | -0.010426 | -0.005960 | 13 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
