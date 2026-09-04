# M744 Coordinate Selector Route Verdict

> Superseded note: M744 was correct for coordinate-only and scale-only
> features, but M745 later added native candidate-context features and found a
> positive held-out signal.  Treat this report as the stop condition for the
> coordinate-identity selector subline, not for the broader native-context
> selector route.

## Scope

M744 summarizes the local follow-up after importing spark-1 M654/M666/M667
artifacts.  The goal was to test whether the M654 coordinate oracle can be
converted into a deployable selector before scaling to shared8/native.

Inputs:

- `runs/remote_spark1_m654_m668_artifacts_v1/m654/`
- `runs/remote_spark1_m654_m668_artifacts_v1/m666/`
- `runs/remote_spark1_m654_m668_artifacts_v1/m667/`
- `runs/m742a_local_m654_scale_export_shared4_v1/scale_rows.jsonl`
- `runs/m742a_local_m654_scale_export_shared4_v1/coordinate_rows.jsonl`

Remote status check:

- `spark-1` has no newer M6/M7 reports beyond M667 in the checked repo path.
- `spark-2` has no newer M6/M7 reports in the checked repo path and is running
  unrelated GPU work, so it was not disturbed.

## Results

### M741: coordinate-only exact selector

M741 trained on M654 coordinate rows and tried to select the exact accepted
coordinate.

Result:

- Training AUC was high (`0.9553` conservative, `0.9577` teacher features).
- Held-out selected accepted hits were `0`.
- M654 oracle still had positive held-out movement.

Conclusion:

Coordinate-level global classification is not enough.  The accepted coordinate
depends on query-local native replay interactions.

### M742: scale-aware exact selector

M742 moved from coordinate rows to concrete coordinate x scale native replay
attempts and removed direct leakage of the `accepted` label from teacher
features.

Held-out test:

| Feature set | Applied | Accepted hits | dRecall | dMAP | dNDCG | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| conservative | 6 | 0 | +0.000000 | +0.000004 | +0.000000 | +0.000000 | -0.000800 | 0 |
| teacher feature | 2 | 0 | +0.000000 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | 0 |

M654 oracle on the same held-out boundary slice:

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

Conclusion:

Adding scale makes the replay surface explicit, but current features still do
not identify the oracle row.

### M743: accepted-region selector

M743 relaxed the target from "select the exact M654 best row" to "select any
accepted native replay attempt".

Held-out test:

| Feature set | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| native | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| native_group | 2 | 1 | 0 | +0.000209 | +0.000182 | +0.000000 | +0.000209 | -0.000400 | 0 |
| teacher | 4 | 1 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| teacher_group | 2 | 2 | 0 | +0.000000 | +0.000002 | +0.000000 | +0.000065 | +0.000000 | 1 |

Conclusion:

The safe accepted region is partially learnable, but it only produces tiny
safe MAP/CUB movement.  It does not move top100/NDCG/MRR in a useful way.
`native_group` can move Recall slightly, but it spends dense overlap and fails
the strict gate.

## Verdict

This route did not break through.

What survives:

- M654 remains valuable as an oracle proof that support-safe coordinate
  movement exists.
- M743 shows a weak, safe accepted-region signal exists.
- The exact best coordinate/scale is not learnable from current coordinate
  features, even with teacher-derived M654 features.

What should stop:

- Do not expand M741/M742 exact selector to shared8.
- Do not spend more cycles on global coordinate/scale classifiers with only
  coordinate statistics, scale, and simple protected/intruder summaries.
- Do not treat high row-level AUC as evidence of deployable boundary movement;
  query-group top selection is the relevant gate.

Next viable direction:

Move one level up from coordinate features to native candidate-context
features.  A deployable selector needs features that describe the local rank
boundary and score distribution after a candidate delta, for example:

- top100/top128/top256 score margins before and after delta,
- protected dense-neighbor displacement risk,
- atom/posting fanout and IDF evidence at the boundary,
- query-local BM25/native agreement,
- candidate-set entropy and score slope,
- whether the delta changes boundary ordering without broad support churn.

The next experiment should be an M745 native-replay-context selector:

1. Use M654-style replay to export pre/post candidate-context features for each
   coordinate x scale attempt.
2. Exclude qrels-derived metrics from deployable features.
3. Train on accepted attempts but select by query-group top score and dev
   threshold.
4. Require held-out dRecall or dNDCG movement with no O/CUB/MAP/MRR regression.
5. Stop if richer native-context features still cannot move the boundary.

This is still aligned with the new route, but it avoids the dead loop of
trying to predict boundary movement from coordinate identity alone.
