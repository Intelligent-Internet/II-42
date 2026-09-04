# M811 Generated-Bundle Selector Stop Summary

## What Was Tested

This round repaired the M799 generated-bundle selector line with progressively
stronger safety controls:

1. M802 cached a reusable generated-bundle feature table.
2. M803 added separate strict-positive and harmful-row heads.
3. M805 replayed M803 in leave-surface-out mode.
4. M806 added explicit harmful-head abstention.
5. M807 mixed all score families with score-family one-hot features.
6. M808 audited task-level risk.
7. M809 profiled selected rows inside negative tasks.
8. M810 tried a qrels-free feature guard on top of M807.

## Main Evidence

M806 is the positive signal:

| Held-out | Best Clean Shape | Applied | Utility |
| --- | --- | ---: | ---: |
| original | margin_bundle / logistic / lambda=3 | 26.0 | +0.000159 |
| seed7642 | score_mean_margin / logistic / lambda=2 | 15.0 | +0.000271 |
| seed7643 | margin_bundle / logistic / lambda=0 | 31.0 | +0.000334 |

This proves harmful-aware abstention can find clean useful moves on every
surface.

M806 also exposes the blocker: those clean points do not share one common
deployable config.

M807 tested whether a mixed score-family model could solve that blocker.  It
did not.  Lambda 3/5 had positive macro utility, but strict clean failed on
seed7642 and seed7643.

M808 showed the risk is concentrated, not global:

| Lambda | Held-out | Negative Tasks |
| ---: | --- | --- |
| 3 | seed7642 | dbpedia-entity |
| 3 | seed7643 | fiqa, msmarco |
| 5 | seed7642 | climate-fever, dbpedia-entity |
| 5 | seed7643 | fiqa, msmarco |

M809 showed some negative-task selected rows are already query-level harmful,
so the harmful head is not useless.  However, risk and safe rows are not
separated reliably enough by p_harm alone.

M810 tested a qrels-free single-feature guard after M807.  It still did not
find a clean useful common config.

## Decision

Stop threshold/guard-based generated-bundle selector repair here.

The route produced useful diagnostics, but not a deployable common selector.
Continuing with more feature swaps would be local tuning without a new source
of information.

## What Is Retained

Keep these artifacts:

1. M802 feature cache and builder.
2. M803 harmful-aware training framework.
3. M806 abstention evidence.
4. M808/M809 task-risk diagnostics.

These are useful if a future stronger supervision source needs the same
feature table.

## Next Direction

The next useful step must add stronger supervision, not another threshold
variant.  Candidate directions:

1. Train generated-bundle selection directly with task/query-level risk labels
   produced from native replay, not only row-level harmful labels.
2. Add pair/listwise supervision over competing generated bundles for the same
   query, so the model learns which bundle wins under native metrics.
3. If native task-level risk labels remain too sparse, stop generated-bundle
   repair and return to the P1 substrate plus stronger retrieval-conditioned
   compiler training.
