# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g1.00`
- Heldout-best: `a0.35_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.714703 | 0.568982 | 0.499731 | 0.391464 |
| heldout-best | 0.710180 | 0.584677 | 0.511483 | 0.404932 |
| fixed a0.50 g1.00 | 0.711676 | 0.577304 | 0.503071 | 0.397278 |
| fixed a0.60 g1.00 | 0.713194 | 0.576476 | 0.505444 | 0.398139 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.003027 | -0.008322 | -0.003339 | -0.005814 | 4 |
| `baseline_060` | +0.001509 | -0.007495 | -0.005712 | -0.006675 | 4 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
