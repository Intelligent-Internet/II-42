# M656 Challenger Ceiling Loss Report

## Summary

M656 tested the next first-stage boundary hypothesis: M655 failed because it
protected dense-hit P1 top100 docs with score floors but did not stop nearby
non-dense challengers from rising. M656 added a challenger score ceiling over
the P1 rank100+ window.

Result: challenger ceilings are directionally useful but not sufficient. They
reduce M655's early boundary damage, but no trained checkpoint passes the
strict no-loss dense-equivalence gate.

## Runs

| Run | Main loss | Selected trained checkpoint | Epoch 1 loss queries | Epoch 12 loss queries |
| --- | --- | ---: | ---: | ---: |
| M654Z | swap-pair preservation | no | 1 | 25 |
| M655 floor-only | protected score floor | no | 36 | 95 |
| M656 floor+ceiling | protected floor + challenger ceiling | no | 6 | 21 |
| M656 combo | swap + protected floor + challenger ceiling | no | 4 | 39 |

Artifacts:

- `runs/m656_challenger_ceiling_gate_v1/m656_shared15_floor1_ceiling1_seed6545/m656_shared15_floor1_ceiling1_seed6545.json`
- `runs/m656_challenger_ceiling_gate_v1/m656_shared15_swap1_floor1_ceiling1_seed6545/m656_shared15_swap1_floor1_ceiling1_seed6545.json`
- generated root reports:
  - `docs/research-sae/reports/m0600-m0699/ii42-m656-challenger-ceiling-gate-report.md`
  - `docs/research-sae/reports/m0600-m0699/ii42-m656-challenger-ceiling-combo-report.md`

## Floor + Ceiling Trace

`protected-demotion-weight=1.0`, `challenger-ceiling-weight=1.0`, no swap loss.

| Epoch | Loss | Protected | Challenger | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.027887 | 0.012822 | 0.014858 | 9 | 6 | 3 |
| 2 | 0.027861 | 0.012858 | 0.014791 | 9 | 7 | 2 |
| 3 | 0.027842 | 0.012869 | 0.014754 | 13 | 10 | 3 |
| 4 | 0.027826 | 0.012877 | 0.014722 | 14 | 13 | 1 |
| 5 | 0.027812 | 0.012881 | 0.014693 | 14 | 12 | 2 |
| 6 | 0.027802 | 0.012880 | 0.014673 | 15 | 15 | 0 |
| 7 | 0.027792 | 0.012877 | 0.014656 | 14 | 18 | -4 |
| 8 | 0.027785 | 0.012875 | 0.014641 | 15 | 20 | -5 |
| 9 | 0.027777 | 0.012877 | 0.014624 | 17 | 20 | -3 |
| 10 | 0.027769 | 0.012874 | 0.014612 | 16 | 21 | -5 |
| 11 | 0.027762 | 0.012872 | 0.014601 | 17 | 23 | -6 |
| 12 | 0.027755 | 0.012869 | 0.014591 | 17 | 21 | -4 |

This is substantially better than M655 floor-only at epoch 1 (`36` loss
queries), so challenger ceilings are a real diagnostic improvement. However,
it is still worse than M654Z swap-only at epoch 1 (`1` loss query), and it
never reaches zero loss queries.

## Combo Trace

`swap-preservation-weight=1.0`, `protected-demotion-weight=1.0`,
`challenger-ceiling-weight=1.0`.

| Epoch | Loss | Protected | Challenger | Swap | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.041708 | 0.012804 | 0.014879 | 0.013820 | 5 | 4 | 1 |
| 2 | 0.041646 | 0.012819 | 0.014833 | 0.013781 | 8 | 6 | 2 |
| 3 | 0.041585 | 0.012827 | 0.014794 | 0.013738 | 14 | 11 | 3 |
| 4 | 0.041521 | 0.012828 | 0.014760 | 0.013690 | 18 | 19 | -1 |
| 5 | 0.041458 | 0.012827 | 0.014726 | 0.013638 | 18 | 24 | -6 |
| 6 | 0.041399 | 0.012823 | 0.014696 | 0.013585 | 22 | 27 | -5 |
| 7 | 0.041348 | 0.012817 | 0.014669 | 0.013533 | 20 | 28 | -8 |
| 8 | 0.041306 | 0.012815 | 0.014640 | 0.013485 | 27 | 35 | -8 |
| 9 | 0.041266 | 0.012812 | 0.014615 | 0.013441 | 24 | 34 | -11 |
| 10 | 0.041228 | 0.012805 | 0.014595 | 0.013396 | 26 | 35 | -10 |
| 11 | 0.041192 | 0.012803 | 0.014570 | 0.013356 | 28 | 39 | -11 |
| 12 | 0.041155 | 0.012793 | 0.014555 | 0.013315 | 27 | 39 | -15 |

The combo does not improve M654Z. It has 4 loss queries at epoch 1 versus
M654Z's 1, and then degrades.

## Interpretation

M656 confirms one part of the diagnosis: top100 membership needs both
protected-doc and challenger control. Adding a challenger ceiling reduces the
large damage caused by M655 floor-only.

However, the absolute floor/ceiling losses still do not align tightly enough
with the actual top100 selection boundary. They are local score constraints,
not a direct listwise membership constraint. When optimized together with the
teacher fit, they still allow unsafe boundary churn.

## Conclusion

M656 is not a candidate. It should be kept as diagnostic evidence:

- challenger ceiling is useful relative to floor-only;
- naive additive floor/ceiling/swap losses do not produce a safe trained
  query-side compiler;
- longer training is not the right next action because loss decreases while
  dense-hit loss queries remain nonzero or increase.

The next first-stage probe should use a direct per-query listwise top100
membership objective. Instead of separate absolute floors and ceilings, the
loss should compute the protected dense-hit P1 top100 docs and the actual
near-boundary challengers together, then optimize the protected set to remain
above the challenger set as a single candidate-list margin/softmax objective.
