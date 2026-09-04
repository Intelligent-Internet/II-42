# M1180 Tail Score-Shape Distillation Audit

M1180 tests whether the useful M1137 `tail_full` ranking shape can be recovered
as a scorer over existing deployable base/direct features.  This is a stricter
follow-up after M1178/M1179 showed that qrel pairwise reranking is not clean.

Artifacts:

- Script: `scripts/audit_m1180_tail_score_shape_distill.py`
- JSON: `runs/m1180_tail_score_shape_distill_v1/tail_score_shape_distill.json`
- Summary: `runs/m1180_tail_score_shape_distill_v1/summary.md`

Implementation note:

- A heavier HGB regressor was started first, but it exceeded the fast-iteration
  budget and was stopped.
- The final audit uses a constrained decision tree as a cheap non-linear smoke
  test.  The goal is to detect a viable signal, not to optimize a final model.

## Result

The teacher tail surface itself has positive average deltas over protected
direct action:

| Teacher | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: |
| tail_minus_direct | +0.000000 | -0.000110 | +0.004727 | +0.007801 | +0.022145 |

But distilling the tail ordering from base/direct features fails:

| Variant | Clean | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| predict_tail_rr | false | +0.000000 | +0.000000 | -0.015512 | -0.009092 | -0.002226 |
| predict_tail_delta_rr | false | +0.000000 | +0.000000 | -0.021622 | -0.016973 | -0.012714 |

## Interpretation

This closes the scorer-space branch for now:

- M1178/M1179 showed qrel-pair supervision over base/direct features is not
  clean.
- M1180 shows even direct tail-rank shape distillation over those features is
  not clean.

The useful tail geometry is therefore not recoverable as a shallow scorer over
the current base/direct score features.  The next viable branch must operate on
the posting compiler/output shape itself:

1. Inspect atom/posting deltas between M1129 and M1137 for M1177 pair examples.
2. Identify whether positive pair movement corresponds to tail-added atom
   overlap or impact changes.
3. If the atom-level signal is structured, train a tiny output-shape compiler
   on that target with direct/P1 floor constraints.
4. If atom-level signal is diffuse, stop this tail-teacher branch instead of
   starting another scorer/reranker loop.
