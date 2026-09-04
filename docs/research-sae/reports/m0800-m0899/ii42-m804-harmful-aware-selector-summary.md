# M804 Harmful-Aware Selector Summary

## Scope

M802/M803 continues the generated-bundle repair route after M798-M801.

The specific failure being repaired is that interaction witnesses are strongly
observable, but the M799 single-head selector still admits task-negative rows.
M803 tests whether an explicit harmful-row head can improve the deployable
operating point.

## M802 Feature Table

M802 persists the generated-bundle interaction feature table:

- Path: `runs/m802_generated_bundle_feature_table_v1/feature_rows.jsonl`
- Rows: 10007
- Size: 22 MB
- Surfaces: original, seed7642, seed7643
- Score families: score_mean_margin, margin_bundle, conservative_margin

This table avoids recomputing moved-document lexical witnesses for every
selector or threshold diagnostic.

## M803 Objective

M803 trains two global heads on the M802 table:

- Positive head: predicts `query_strict_positive`.
- Harmful head: predicts rows with dense-overlap spend, candidate-UB spend, or
  negative MAP/NDCG/MRR delta.

The selector score is:

```text
p(strict_positive) - lambda * p(harmful)
```

Dev selects a global threshold under the same strict multi-surface gate.

## Result

M803 finds a clean harmful-aware selector:

| Best clean case | Gate | Clean | Applied | dMAP | dNDCG | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| margin_bundle + logistic, lambda=0.5 | 1 | 1 | 3.0 | +0.000012 | +0.000078 | +0.000041 | +0.000000 | +0.000110 |

This is better than the M800 clean threshold oracle, whose best useful clean
case was around utility `+0.000046`.  It means harmful supervision is a real
repair, not just another threshold tweak.

## Limitation

The useful clean scale is still small: only about 1-3 applied queries across
the three held surfaces.  Larger HGB operating points produce stronger macro
utility but still fail per-task clean gates.

Examples of unsafe high-utility cases:

| Case | Applied | Utility | Failure mode |
| --- | ---: | ---: | --- |
| margin_bundle + HGB, lambda=2.0 | 116.0 | +0.000504 | seed7642/seed7643 task negatives |
| margin_bundle + logistic, lambda=5.0 | 43.3 | +0.000295 | seed7643 msmarco negative |

So M803 proves the safety objective direction is useful, but it is not yet a
route promotion.

## Decision

Keep M802 feature table and M803 harmful-aware selector as the current best
repair artifact.

Do not promote to P1 route yet.  The clean gain is real but still diagnostic
scale.

## Next Step

Run a robustness audit before broader replay:

1. Leave-one-surface-out training: train on two surfaces, test on the third.
2. Compare M803 against M799 and M800 clean operating points.
3. Track whether the same `margin_bundle + logistic` shape remains clean.
4. If clean utility stays positive but tiny, add query-level abstention margin
   and worst-surface-aware threshold selection.
5. If clean utility disappears under leave-one-surface-out, stop this selector
   route and keep only the interaction witnesses for a redesigned objective.

The next experiment should be M805 leave-surface-out harmful-aware robustness,
using the M802 cached feature table.
