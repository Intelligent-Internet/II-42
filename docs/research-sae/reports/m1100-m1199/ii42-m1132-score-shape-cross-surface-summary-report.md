# ii42 M1132 Score-Shape Cross-Surface Summary

## Question

After M1130/M1131 rejected another alpha selector loop, M1132 tested a
different hypothesis:

> The listwise-only atom signal is useful, but the atom score shape is too
> sharp when added linearly. A global concave transform may preserve more
> useful atom evidence while reducing top-rank damage.

The tested transform was signed power:

```text
score = normalized_lexical + alpha * sign(atom) * abs(atom)^gamma
```

No query-level gating, no dataset-specific policy, no retraining.

## Surface Results

### Shared5

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fixed a0.50 g1.00 | 0.715489 | 0.562618 | 0.498005 | 0.391700 |
| fixed a0.60 g1.00 | 0.715648 | 0.561217 | 0.495022 | 0.388300 |
| heldout-best a0.60 g0.50 | 0.722968 | 0.564050 | 0.502446 | 0.394164 |
| train-selected a0.75 g1.50 | 0.714686 | 0.549733 | 0.483482 | 0.374065 |

### Shared8

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fixed a0.50 g1.00 | 0.658735 | 0.673472 | 0.578677 | 0.441150 |
| fixed a0.60 g1.00 | 0.670749 | 0.667014 | 0.578412 | 0.439473 |
| heldout-best a0.75 g0.50 | 0.696680 | 0.674602 | 0.585791 | 0.452213 |
| train-selected a0.75 g0.75 | 0.685795 | 0.669946 | 0.580956 | 0.446006 |

### Shared15

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fixed a0.50 g1.00 | 0.770982 | 0.777186 | 0.699337 | 0.604800 |
| fixed a0.60 g1.00 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| heldout-best a0.50 g0.50 | 0.773637 | 0.777965 | 0.702270 | 0.609422 |
| train-selected a0.75 g1.00 | 0.771468 | 0.763056 | 0.689540 | 0.600224 |

## Interpretation

There are two simultaneous facts:

1. Train-selected wrappers still overfit toward high atom pressure. That
   invalidates naive train-selected post-hoc wrappers.
2. The heldout-best transform is consistently concave (`gamma=0.5`) across
   shared5, shared8, and shared15. That is not enough for promotion, but it is
   a real structural clue.

This means the next useful route is not another selector and not another fixed
alpha search. The training objective should internalize concave atom-score
calibration directly, so the learned atom signal does not require damaging
linear pressure at scoring time.

## Recommendation

Promote the idea, not the post-hoc wrapper:

- Keep M1129 listwise-only as the current best training baseline.
- Add a new small training canary where the listwise objective scores
  in-batch positives with concave atom geometry, starting with `gamma=0.5`.
- Keep alpha fixed in the safe band (`0.50` or `0.60`) during evaluation.
- Reject the canary if it only wins by train-selected high alpha or causes
  shared15 row damage to increase.

Expected value:

- Higher than more alpha gate tuning, because the repeated `gamma=0.5`
  heldout optimum points to score-shape mismatch.
- Still unproven, because train selection did not choose the same shape.

Next experiment:

`M1133 concave-listwise atom objective`.

Acceptance:

- On shared5 canary, concave-listwise must beat M1125b/M1129-style linear
  listwise at fixed safe alpha on MAP/NDCG and not regress MRR materially.
- If shared5 passes, scale to shared8.
- If shared8 passes, run shared15.

Stop condition:

- If concave-listwise repeats M1132 train-overpressure behavior, stop and
  return to evidence generation rather than tuning wrappers.
