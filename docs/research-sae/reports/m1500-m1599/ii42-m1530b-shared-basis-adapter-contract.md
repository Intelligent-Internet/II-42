# M1530B Shared-Basis Adapter Contract

Date: 2026-07-10

## Decision Boundary

M1530A showed that retrieval supervision can move a bounded 32K posting head,
but independently updating all 25.2M centroid coordinates overfit the 256-row
capacity surface and failed the conjunctive gate. M1530B changes only the
parameterization of that update.

All M1520 centroids and the BGE backbone are frozen. A zero-initialized
rank-8 residual adapter transforms the shared 768-dimensional token basis
before projection onto the frozen centroids:

```text
h_delta = h + up(down(h))
h' = h_delta * (norm(h) / norm(h_delta))
posting(h) = TopK(log1p(relu(h' @ frozen_centroids.T)) * energy)
```

This gives every training example a shared geometric update and reduces the
trainable representation parameters from 25,198,592 to 12,288. One additional
global score-calibration scalar remains trainable.

## Fixed Surface

- Same BGE model, M1520 codebook, M1518 train/validation artifacts, seed,
  256/128 rows, candidate sets, teacher scores, and tokenization as M1530A.
- Same token TopK=8, query K=24, document K=96, background mask, and
  non-negative impact transform.
- Same pure candidate-set teacher KL, learning rate `2e-4`, weight decay,
  gradient accumulation, and 800-step budget.
- Rank 8 only. No rank, learning-rate, loss, threshold, or budget grid.
- No BM25, qrels, routing, reconstruction, FLOPS, backbone unfreezing, or
  native-index change.

The adapter `up` matrix starts at zero, so step 0 must reproduce M1530A step 0
within `1e-6` for KL and all discrete ordering/integrity metrics. Failure of
this identity control invalidates the run.

## Gates

The strict promotion gate remains unchanged:

- validation KL decreases by at least 10%;
- teacher top1 improves by at least 0.05, or pair-order improves by at least
  0.03 without top1 regression;
- touch, max DF, active-vocabulary, finite, and non-negative gates all pass.

M1530A's non-dominated diagnostic frontier is locked as:

| Point | Step | KL | Teacher top1 | Pair order |
| --- | ---: | ---: | ---: | ---: |
| lowest KL | 250 | 1.441690326 | 0.500000000 | 0.835616410 |
| highest top1 | 300 | 1.507292271 | 0.507812500 | 0.834637940 |

A rank-32 follow-up is authorized only if rank 8 remains integrity-safe and
weakly improves all three metrics of at least one frontier point, with a
strict improvement in at least one metric, but does not pass the strict gate.
This is the only continuation rule; a merely lower training loss or a
single-metric gain does not authorize more capacity.

## Decisions

- Strict pass: scale the same rank-8 mechanism to the pinned 10K/1K artifact.
- Frontier dominance without strict pass: allow one rank-32 control.
- No frontier dominance: stop shared-basis adaptation for this source.
- Any identity mismatch, numerical failure, or posting-integrity failure:
  invalidate the run and repair the implementation before interpreting it.

No M1530B result is an OOD, BEIR, native-index, or product-quality claim.
