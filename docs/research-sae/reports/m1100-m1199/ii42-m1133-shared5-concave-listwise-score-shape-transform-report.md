# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g1.25`
- Heldout-best: `a0.75_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.712518 | 0.560255 | 0.500709 | 0.391782 |
| heldout-best | 0.715026 | 0.584719 | 0.515970 | 0.410797 |
| fixed a0.50 g1.00 | 0.704942 | 0.571312 | 0.505876 | 0.397542 |
| fixed a0.60 g1.00 | 0.710751 | 0.577472 | 0.513693 | 0.406023 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.007576 | -0.011057 | -0.005167 | -0.005760 | 4 |
| `baseline_060` | +0.001767 | -0.017217 | -0.012984 | -0.014241 | 5 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
