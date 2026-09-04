# M1295 Multi-Objective Pair Native Replay

## Purpose

M1295 tests whether the positive signals from recent reviews can be combined
inside source/objective construction rather than added as a post-hoc gate.

Specifically, it keeps the M1288 pair/witness target-risk source and adds soft
movement/support terms before native replay:

- `pair`
- `pair_move025`
- `pair_move05`
- `pair_support025`
- `pair_move025_support025`

This is intentionally a small `limit25` smoke.  The goal is not to promote a
default, but to decide whether hand-scored multi-objective variants deserve
more scale.

## Command

```bash
python3 scripts/replay_m1295_multi_objective_pair_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,pair_move025,pair_move05,pair_support025,pair_move025_support025 \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --feature-groups pair \
  --limit-queries 25 \
  --output-root runs/m1295_multi_objective_pair_native_limit25_v1
```

Outputs:

- `runs/m1295_multi_objective_pair_native_limit25_v1/m1295_native.json`
- `runs/m1295_multi_objective_pair_native_limit25_v1/m1295_native.md`

## Result

Best eval variant:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |

The best variant is still pair-only.  The closest multi-objective variant is:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_move025_pair_risk1_b192_s0.05` | 0.693333 | 0.520000 | 52 | 0 | 0.956842 | 0.952632 | 0.974001 | 0.002865 |

This reduces negative-only selection, but it does not meet the promotion rule:
it loses more pair success than allowed and does not improve the protected
overlap frontier.

## Interpretation

The review feedback is supported by this smoke:

- useful atom structure exists;
- pair/witness remains the strongest source on this surface;
- simple hand-scored movement/support additions do not turn that source into a
  better deployable policy;
- ordinary filters, thresholds, and small weight swaps should stop here.

The next step should not be another hand-score variant.  The positive signals
from `signed_sum_s1`, `mean_teacher_s0.75`, CUB-specific teachers, and
pair/witness should be moved into a learned objective or a source construction
that creates harm separation before selection.

## Decision

Stop this hand-scored multi-objective branch unless a new source changes the
candidate distribution.  Keep `pair/witness` as a teacher/source diagnostic,
not as a deployable default.

Next recommended direction:

1. Train a learned multi-objective compiler/ranker over pair/witness candidates.
2. Include objective terms for target utility, risk, movement geometry, and
   protected overlap directly in the loss.
3. Use the same staged gate: small limit smoke, native replay, LODO, then
   shared15 only if the small smoke improves the pair-only frontier.

## Verification

```bash
python3 -m py_compile scripts/replay_m1295_multi_objective_pair_native.py
```

Passed.
