# M714 Pair-Impact Selector

M714 follows M713 by asking whether the weak single-atom native pair-impact
signal is learnable from qrels-free atom features.

This is still a first-stage dense-equivalence probe:

- no BM25;
- no reranker;
- no qrels-driven optimization;
- no dataset-specific tuning;
- frozen P1.3 / M549U native signed-dot baseline.

## Motivation

M713 showed that missed target atoms can move dense-boundary pairs at larger
scales, while selected non-target atoms become harmful. That means there is a
real but weak target-specific signal. M714 tests whether a small global model
can learn that signal before we justify deeper training.

## Method

M714 creates direct labels by rerunning the native scorer with one candidate
atom boosted at a time. Each atom receives a pair-impact label:

- useful: `fixed_pairs > regressed_pairs`;
- harmful: `regressed_pairs > fixed_pairs`;
- neutral: otherwise.

A small classifier and regressor are trained on train-split atom labels. At
evaluation time, the learned selector chooses a tiny budget of atoms from the
full candidate pool, applies the same query-side boost, and reruns the native
scorer.

## Runs

| Run | Datasets | Queries / dataset | Label atoms / query | Scale | Budgets |
| --- | --- | ---: | ---: | ---: | --- |
| canary_s005 | arguana, cqadupstack, fiqa, scidocs | 10 | 32 | 0.05 | 2, 4, 8, 16 |
| canary_s010 | arguana, cqadupstack, fiqa, scidocs | 10 | 32 | 0.10 | 2, 4, 8, 16 |

## Training Labels

For the scale 0.10 canary:

- label rows: `1013`
- train rows: `704`
- positive train rows: `29`
- positive train share: `0.041193`
- train AUC: `1.000000`

The perfect train AUC is not evidence of success. The positive label count is
small, and held-out eval is the only valid gate.

## Eval Results

| Scale | Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target selected |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.05 | classifier_b2 | 0.532609 | 0.532609 | 0 | 0 | 1.000000 | 0.000000 |
| 0.10 | classifier_b8 | 0.543478 | 0.532609 | 1 | 0 | 0.994737 | 0.000000 |
| 0.10 | classifier_b2 | 0.532609 | 0.532609 | 0 | 0 | 0.997368 | 0.000000 |
| 0.10 | classifier_b4 | 0.532609 | 0.532609 | 0 | 0 | 0.996491 | 0.000000 |
| 0.10 | regressor_b2 | 0.532609 | 0.532609 | 0 | 0 | 0.995614 | 0.041667 |

## Interpretation

M714 does not pass the eval gate.

The scale 0.10 best row has a small apparent lift:

- pair success: `0.532609 -> 0.543478`
- fixed/regressed: `1/0`
- top95: `0.994737`

This is not sufficient:

- the top95 head-preservation floor misses the `0.995` gate;
- only one eval pair is fixed;
- the selected target share is `0.000000`, so the model is not clearly learning
  the dense-boundary target atoms discovered in M713;
- scale 0.05 is safe but produces no boundary movement.

## Decision

Do not scale this selector as-is.

The result does not prove the route is dead. It does prove that the current
feature/label interface is too weak for deep training. The useful next step is
not more epochs on M714, but a better supervision surface:

- label atom sets or pairwise atom interactions, not only single atoms;
- include head-preservation loss directly in the selector target;
- separate target-support recovery from generic pair movement;
- require held-out target selection, not only pair-success lift.

This supports the current research discipline: small probes should reject weak
interfaces before long training, but weak positive movement should guide the
next label design rather than trigger a full route switch.

## Files

- `scripts/train_m714_pair_impact_selector.py`
- `runs/m714_pair_impact_selector_canary_v1/m714_summary.json`
- `runs/m714_pair_impact_selector_s005_canary_v1/m714_summary.json`
