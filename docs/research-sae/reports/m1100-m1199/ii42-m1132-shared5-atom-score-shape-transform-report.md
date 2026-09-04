# ii42 M1132 Atom Score Shape Transform Audit

Verdict: `reject_score_shape_keep_fixed_band`

## Selected Transform

- Train-selected: `a0.75_g1.50`
- Heldout-best: `a0.60_g0.50`

## Heldout Macro

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected | 0.714686 | 0.549733 | 0.483482 | 0.374065 |
| heldout-best | 0.722968 | 0.564050 | 0.502446 | 0.394164 |
| fixed a0.50 g1.00 | 0.715489 | 0.562618 | 0.498005 | 0.391700 |
| fixed a0.60 g1.00 | 0.715648 | 0.561217 | 0.495022 | 0.388300 |

## Selected Delta

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline_050` | -0.000802 | -0.012885 | -0.014523 | -0.017635 | 4 |
| `baseline_060` | -0.000962 | -0.011484 | -0.011541 | -0.014235 | 5 |

## Interpretation

The train-selected global atom-score shape does not beat fixed alpha 0.60 on all heldout macro metrics. This suggests that simple post-hoc score-shape wrappers are not enough; any next attempt should alter the training objective or candidate evidence itself.
