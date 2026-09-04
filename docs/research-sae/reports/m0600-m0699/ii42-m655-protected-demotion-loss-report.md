# M655 Protected Demotion Loss Report

## Summary

M655 tested whether direct score-floor protection for dense-hit P1 top100 docs
can prevent the query-side compiler from losing dense top100 boundary members.

Result: no trained checkpoint passed the strict dense-equivalence gate. The
protected score-floor loss is not a useful first-stage control surface by
itself, and combining it with M654Y swap-pair preservation also failed.

This is a negative result, but it narrows the failure mode: the boundary issue
is not solved by preventing protected docs from losing absolute score. Top100
membership is still broken when challenger docs rise, so the next loss must
control both protected-doc floors and challenger ceilings/listwise membership.

## Runs

| Run | Main change | Selected trained checkpoint | First epoch loss queries | Last epoch loss queries |
| --- | --- | ---: | ---: | ---: |
| M654Z | swap-pair preservation, strict no-loss gate | no | 1 | 25 |
| M655 floor-only | protected demotion floor, weight 4.0 | no | 36 | 95 |
| M655 combo | swap 1.0 + protected floor 1.0 | no | 23 | 49 |

Artifacts:

- `runs/m655_protected_demotion_gate_v1/m655_shared15_protect_w4_seed6545/m655_shared15_protect_w4_seed6545.json`
- `runs/m655_protected_demotion_gate_v1/m655_shared15_swap1_protect1_seed6545/m655_shared15_swap1_protect1_seed6545.json`
- generated root reports:
  - `docs/research-sae/reports/m0600-m0699/ii42-m655-protected-demotion-gate-report.md`
  - `docs/research-sae/reports/m0600-m0699/ii42-m655-protected-demotion-combo-report.md`

## M655 Floor-Only Trace

`protected-demotion-weight=4.0`, no swap loss.

| Epoch | Loss | Protected loss | Failed checks | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | --- | ---: | ---: | ---: |
| 1 | 0.050387 | 0.012516 | CUB, Recall, O@100, O@256, support, no-loss | 28 | 36 | -8 |
| 2 | 0.048161 | 0.011766 | CUB, O@100, O@256, support, no-loss | 45 | 57 | -17 |
| 3 | 0.046674 | 0.011193 | CUB, O@100, O@256, support, no-loss | 53 | 63 | -20 |
| 4 | 0.045643 | 0.010785 | CUB, O@100, O@256, support, no-loss | 59 | 74 | -31 |
| 5 | 0.044905 | 0.010471 | CUB, Recall, O@100, O@256, support, no-loss | 63 | 81 | -39 |
| 6 | 0.044295 | 0.010225 | CUB, Recall, O@100, O@256, support, no-loss | 66 | 86 | -39 |
| 7 | 0.043803 | 0.010040 | CUB, Recall, O@100, O@256, support, no-loss | 67 | 92 | -40 |
| 8 | 0.043346 | 0.009868 | CUB, O@100, O@256, support, no-loss | 69 | 93 | -47 |
| 9 | 0.042961 | 0.009716 | CUB, O@100, O@256, support, no-loss | 69 | 91 | -42 |
| 10 | 0.042613 | 0.009569 | CUB, O@100, O@256, support, no-loss | 72 | 94 | -47 |
| 11 | 0.042319 | 0.009468 | CUB, Recall, O@100, O@256, support, no-loss | 70 | 92 | -44 |
| 12 | 0.042012 | 0.009330 | CUB, O@100, O@256, support, no-loss | 75 | 95 | -44 |

The protected loss decreases, but dense-hit boundary loss gets worse. This
means the loss optimizes its local score-floor objective while still allowing
top100 membership churn.

## M655 Combo Trace

`swap-preservation-weight=1.0`, `protected-demotion-weight=1.0`.

| Epoch | Loss | Protected loss | Swap loss | Failed checks | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| 1 | 0.026668 | 0.012598 | 0.013822 | Recall, O@100, O@256, support, no-loss | 20 | 23 | -3 |
| 2 | 0.026400 | 0.012175 | 0.013808 | CUB, Recall, O@100, O@256, support, no-loss | 29 | 33 | -6 |
| 3 | 0.026243 | 0.011936 | 0.013790 | CUB, Recall, O@100, O@256, support, no-loss | 35 | 39 | -8 |
| 4 | 0.026127 | 0.011781 | 0.013771 | CUB, Recall, O@100, O@256, support, no-loss | 38 | 39 | -3 |
| 5 | 0.026033 | 0.011651 | 0.013750 | Recall, O@100, O@256, support, no-loss | 35 | 40 | -8 |
| 6 | 0.025948 | 0.011532 | 0.013727 | Recall, O@100, O@256, support, no-loss | 39 | 46 | -10 |
| 7 | 0.025879 | 0.011435 | 0.013705 | Recall, O@100, O@256, support, no-loss | 39 | 47 | -11 |
| 8 | 0.025821 | 0.011348 | 0.013682 | Recall, O@100, O@256, support, no-loss | 40 | 46 | -9 |
| 9 | 0.025775 | 0.011281 | 0.013662 | Recall, O@100, O@256, support, no-loss | 40 | 47 | -12 |
| 10 | 0.025729 | 0.011218 | 0.013638 | Recall, O@100, O@256, support, no-loss | 40 | 47 | -10 |
| 11 | 0.025686 | 0.011169 | 0.013616 | Recall, O@100, O@256, support, no-loss | 40 | 48 | -13 |
| 12 | 0.025642 | 0.011104 | 0.013594 | Recall, O@100, O@256, support, no-loss | 41 | 49 | -16 |

The combo is better than floor-only but worse than M654Z swap-only. It still
cannot eliminate dense-hit losses, so it does not solve the first-stage
dense-equivalence gate.

## Interpretation

M654Z showed that swap-pair preservation gets close to useful movement but
still loses dense-hit docs. M655 tested the obvious next patch: protect those
docs' absolute scores.

That patch failed because absolute protected-doc floor is only half of the
top100 membership constraint. A protected doc can keep or even increase its
score and still leave top100 if nearby challengers rise more. The current
query-side compiler changes the score field globally enough that many
challengers move together.

## Conclusion

M655 is not a candidate. Do not extend this exact floor-loss line by longer
training or weight search.

The next first-stage probe should be listwise membership preservation:

- protect dense-hit P1 top100 docs with a no-demotion floor;
- also constrain non-dense challengers around rank100 with a ceiling;
- compute the loss on the same candidate set used by the boundary audit;
- keep the strict query-level no-loss gate;
- accept a trained checkpoint only if zero lost dense-hit docs survives dev and
  test.

This shifts the control surface from "protected docs should score high" to
"protected docs must remain above the actual challengers that can evict them."
