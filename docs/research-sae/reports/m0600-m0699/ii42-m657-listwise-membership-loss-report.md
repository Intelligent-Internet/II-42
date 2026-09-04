# M657 Listwise Membership Loss Report

## Summary

M657 tested a direct listwise top100 membership objective. Instead of separate
protected floors and challenger ceilings, it compares the protected dense-hit
P1 top100 tail against the actual P1 rank100+ challenger window in one
soft-margin loss.

Result: no trained checkpoint passed the strict no-loss dense-equivalence
gate. The first clamped target was too hard; the corrected unclamped target is
better calibrated but still worse than M654Z swap-only.

## Runs

| Run | Main loss | Selected trained checkpoint | Epoch 1 loss queries | Epoch 12 loss queries |
| --- | --- | ---: | ---: | ---: |
| M654Z | swap-pair preservation | no | 1 | 25 |
| M657 clamped | listwise margin, target clamped to `>=0` | no | 7 | 36 |
| M657 unclamped | listwise margin preserving baseline smooth margin | no | 4 | 23 |

Artifacts:

- `runs/m657_listwise_membership_gate_v1/m657_shared15_listwise1_seed6545/m657_shared15_listwise1_seed6545.json`
- `runs/m657_listwise_membership_gate_v1/m657_shared15_listwise1_unclamped_seed6545/m657_shared15_listwise1_unclamped_seed6545.json`
- generated root reports:
  - `docs/research-sae/reports/m0600-m0699/ii42-m657-listwise-membership-gate-report.md`
  - `docs/research-sae/reports/m0600-m0699/ii42-m657-listwise-membership-unclamped-report.md`

## Why the First M657 Was Too Hard

The clamped version used:

```text
target_margin = max(
    smooth_min(P1 protected scores) - smooth_max(P1 challenger scores),
    0,
)
```

This is stricter than preserving P1. With many challengers, `smooth_max` can
exceed the worst protected score even when the discrete P1 top100 membership is
valid. Clamping the target to zero therefore forced a stronger-than-baseline
margin and produced a high loss:

| Epoch | Loss | Listwise loss | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.116075 | 0.115869 | 8 | 7 | 1 |
| 12 | 0.115629 | 0.115179 | 29 | 36 | -10 |

This is not a useful first-stage objective.

## Unclamped M657

The corrected variant preserves the baseline smooth margin, even if it is
negative:

```text
target_margin =
    smooth_min(P1 protected scores) - smooth_max(P1 challenger scores)
```

This brought loss back to the M654Z scale, but it still did not satisfy the
strict no-loss gate:

| Epoch | Loss | Listwise loss | Gain queries | Loss queries | Net dense-hit delta |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.013980 | 0.013775 | 5 | 4 | 1 |
| 2 | 0.013957 | 0.013751 | 10 | 7 | 3 |
| 3 | 0.013939 | 0.013727 | 9 | 7 | 2 |
| 4 | 0.013923 | 0.013703 | 10 | 11 | -1 |
| 5 | 0.013910 | 0.013680 | 13 | 11 | 2 |
| 6 | 0.013901 | 0.013658 | 16 | 19 | -3 |
| 7 | 0.013892 | 0.013640 | 14 | 17 | -3 |
| 8 | 0.013884 | 0.013623 | 14 | 19 | -5 |
| 9 | 0.013875 | 0.013608 | 15 | 18 | -3 |
| 10 | 0.013867 | 0.013593 | 19 | 19 | 0 |
| 11 | 0.013859 | 0.013578 | 17 | 22 | -5 |
| 12 | 0.013854 | 0.013565 | 18 | 23 | -5 |

Unclamped listwise is better than clamped listwise, but still worse than M654Z
swap-only at the first epoch.

## Interpretation

M657 confirms that the loss must preserve the *actual baseline boundary
geometry*, not an idealized positive margin. That is useful, but not enough.

The remaining failure is now very narrow:

- M654Z epoch 1 loses only one dense-hit boundary query.
- M657 variants lose more than that.
- All variants degrade as training continues.

This suggests that the next probe should stop guessing new aggregate losses and
instead inspect the best rejected checkpoint/query directly.

## Conclusion

M657 is not a candidate. It should be preserved as evidence that:

- clamping smooth listwise target margins is too aggressive;
- preserving the baseline smooth margin is necessary but insufficient;
- current query-side aggregate losses cannot reliably eliminate the last
  dense-hit boundary loss.

Next action: save or reconstruct the closest rejected checkpoint, then run a
doc-swap audit on that exact state. The next objective should be derived from
the remaining lost query/doc pair rather than another broad loss guess.
