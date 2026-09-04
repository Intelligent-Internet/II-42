# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g0.75`
- Heldout-best: `a0.60_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.669013 | 0.665583 | 0.571391 | 0.432877 |
| heldout-best | 0.665262 | 0.680417 | 0.582149 | 0.444132 |
| fixed a0.50 g1.00 | 0.653205 | 0.674080 | 0.572509 | 0.431031 |
| fixed a0.60 g1.00 | 0.654631 | 0.675288 | 0.571384 | 0.432484 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | +0.015808 | -0.008496 | -0.001118 | +0.001847 | 7 |
| `baseline_060` | +0.014383 | -0.009704 | +0.000007 | +0.000394 | 7 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
