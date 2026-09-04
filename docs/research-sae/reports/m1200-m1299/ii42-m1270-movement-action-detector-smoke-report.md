# M1270 Movement-Aware Action Detector Smoke

## Question

M1269 showed strong qrels-free doc/rank movement separability between useful
nonraw choices and false-positive nonraw choices.  M1270 tests whether this can
be turned into an abstain-first action detector on hard rows.

The detector treats every nonraw mix as an action candidate:

- action features: native doc/rank movement between raw and that mix;
- label: action improves over raw without any metric-level regression;
- validation: leave-one-dataset-out;
- train fold chooses an abstain threshold;
- heldout fold either accepts one action or falls back to raw.

## Run

- Script: `scripts/train_m1270_movement_action_detector_smoke.py`
- Output root: `runs/m1270_movement_action_detector_smoke_v1/`
- JSON:
  `runs/m1270_movement_action_detector_smoke_v1/m1270_movement_action_detector_smoke.json`
- Markdown:
  `runs/m1270_movement_action_detector_smoke_v1/m1270_movement_action_detector_smoke.md`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

## Candidate Labels

| Label | Count |
| --- | ---: |
| Safe action | 117 |
| Unsafe action | 879 |

The candidate-level label is not too sparse.  The failure is not simple lack of
positive examples.

## Result

| Policy | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw` | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 | +0.000000 | 0 | 0 |
| `movement_detector` | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 | +0.000000 | 0 | 0 |

The detector abstains on all heldout rows, so it is safe but useless.

## Fold Diagnostics

| Dataset | Threshold | Selected | TrainScore |
| --- | ---: | ---: | ---: |
| `cqadupstack` | 1.100000 | 0 | +0.022194 |
| `scidocs` | 1.100000 | 0 | -0.019458 |
| `webis-touche2020` | 1.100000 | 0 | +0.003519 |

Every fold selected the abstain threshold.  The trained probabilities did not
produce a heldout-safe operating point.

## Interpretation

M1270 is a controlled no-go for this implementation.

Retained:

- M1269 movement features are diagnostically useful;
- false-positive nonraw can be described after candidate movement is observed;
- abstain-first framing is safer than M1268's direct mix classifier.

Rejected:

- simple logistic action detector over current movement features;
- train-fold thresholding as enough to recover M1267 safe-oracle lift;
- scaling this implementation to full `shared15`.

The important distinction:

M1269 says the information exists in movement space, but M1270 says the current
model/loss/threshold pipeline cannot turn it into a useful deployable policy.

## Decision

Do not run full `shared15` for M1270.

Next work should change the loss or action proposal, not tune this detector:

1. Use pair/listwise ranking loss over candidate actions instead of binary safe
   classification.
2. Penalize false-positive nonraw selection directly, especially on
   `cqadupstack`.
3. Consider an abstain-first ordinal objective:
   `raw < small_mix < large_mix` only when movement produces a clear positive
   rank effect.
4. Keep M1267 safe oracle as the upper bound and M1270 as the current
   deployable lower bound.
