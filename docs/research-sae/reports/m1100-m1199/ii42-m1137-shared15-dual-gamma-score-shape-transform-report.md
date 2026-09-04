# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g0.75`
- Heldout-best: `a0.50_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.785958 | 0.744507 | 0.683239 | 0.581610 |
| heldout-best | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| fixed a0.50 g1.00 | 0.771768 | 0.752251 | 0.688795 | 0.587888 |
| fixed a0.60 g1.00 | 0.775801 | 0.747724 | 0.687443 | 0.583750 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.014190 | -0.007744 | -0.005556 | -0.006278 | 10 |
| `baseline_060` | +0.010157 | -0.003217 | -0.004204 | -0.002140 | 10 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
