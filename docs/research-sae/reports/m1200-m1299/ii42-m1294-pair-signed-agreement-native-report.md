# M1294 Pair-Signed Agreement Native Replay

## Question

M1288 showed that pair/witness-selected atoms strongly improve dense-boundary
pair movement through the native scorer.  M1293 showed query-delta movement is
useful but row-fragile.  M1294 tests whether the two signals can be combined at
the source level:

- pair/witness model supplies the teacher-side target/risk structure;
- query-time signed features constrain the selected atoms.

The desired result would be a source that preserves most M1288 pair-success
lift while improving protected-head overlap.

## Setup

- Surface: `arguana,cqadupstack,fiqa,scidocs`
- Limit: `25` queries per dataset
- Feature group: `pair`
- Budgets: `96`, `192`
- Risk penalties: `0.5`, `1.0`
- Scale: `0.05`
- Sources:
  - `pair`
  - `pair_signed_filter`
  - `pair_signed_fill`
  - `pair_boundary_fill`
- Output:
  `runs/m1294_pair_signed_agreement_native_limit25_v1/m1294_native.json`

## Result

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_signed_fill_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_signed_filter_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `pair_signed_fill_pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `pair_signed_filter_pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `pair_boundary_fill_pair_risk0.5_b192_s0.05` | 0.686667 | 0.520000 | 50 | 0 | 0.953684 | 0.951316 | 0.981611 | 0.022005 |
| `pair_boundary_fill_pair_risk1_b192_s0.05` | 0.680000 | 0.520000 | 48 | 0 | 0.955263 | 0.952632 | 0.974001 | 0.016536 |
| `pair_pair_risk0.5_b96_s0.05` | 0.660000 | 0.520000 | 42 | 0 | 0.963684 | 0.957895 | 0.970197 | 0.014323 |
| `pair_pair_risk1_b96_s0.05` | 0.656667 | 0.520000 | 41 | 0 | 0.965526 | 0.961842 | 0.948003 | 0.003385 |

`pair_signed_filter` and `pair_signed_fill` are identical to pair-only at the
top of the ranking.  This means signed consistency does not add a new source
constraint inside the pair/witness selector on this surface.

`pair_boundary_fill` is worse: it lowers pair success and increases
negative-only share.

## Interpretation

M1294 rejects a tempting but too-simple objective constraint.

The positive M1288 pair/witness source already selects atoms that pass the
signed-consistency checks tested here.  Adding signed agreement does not
improve safety.  Boundary fill actively hurts.

This clarifies the role of the retained signals:

- M1288 pair/witness should remain a teacher/source signal for pair movement.
- M1293 `mean_teacher_s0.75` / M1251 `signed_sum_s1` should remain movement
  geometry signals.
- They should not be merged by simple intersection, filter, or fill rules.

## Decision

Do not add signed agreement as a hard source constraint.

Do not continue with more pair × signed filter variants.

## Next Direction

The next branch should train or construct a multi-objective compiler rather
than hand-combine sources:

1. Pair/witness objective: preserve M1288 pair movement.
2. Movement objective: match `mean_teacher_s0.75` or `signed_sum_s1` geometry.
3. Safety objective: penalize M1252/M1293 row-level negative movement.

The current evidence says these signals are complementary at the objective
level, not composable by a simple source filter.
