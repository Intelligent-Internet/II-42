# M1271 Movement Action-Value Ranker Smoke

## Question

M1270 showed that a binary movement-aware safe-action detector abstains safely
but recovers no lift.  M1271 tests whether a value/ranking framing works better:

- train a regressor to predict per-action score delta versus raw;
- for each query, accept only the highest predicted action above a train-fold
  threshold;
- otherwise fall back to raw;
- validate leave-one-dataset-out on hard rows.

This is a hard-row smoke only.

## Run

- Script: `scripts/train_m1271_movement_action_value_ranker_smoke.py`
- Output root: `runs/m1271_movement_action_value_ranker_smoke_v1/`
- JSON:
  `runs/m1271_movement_action_value_ranker_smoke_v1/m1271_movement_action_value_ranker_smoke.json`
- Markdown:
  `runs/m1271_movement_action_value_ranker_smoke_v1/m1271_movement_action_value_ranker_smoke.md`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

## Candidate Labels

| Label | Count |
| --- | ---: |
| Safe action | 117 |
| Unsafe action | 879 |

## Result

| Policy | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw` | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 | +0.000000 | 0 | 0 |
| `oracle_safe_action` | 0 | +0.001640 | +0.001209 | +0.000472 | +0.001013 | +0.000031 | +0.014825 | +0.005947 | 42 | 0 |
| `ridge_action_value` | 1 | +0.001315 | +0.000478 | -0.000286 | +0.000585 | +0.000031 | +0.005782 | -0.000953 | 1 | 7 |
| `hgb_action_value` | 1 | +0.001331 | +0.000530 | -0.000286 | +0.000607 | +0.000031 | +0.006057 | -0.000445 | 7 | 10 |

The HGB regressor is slightly above raw in macro score but below raw in
query-level mean score and still leaves the same NDCG negative metric.  It does
not pass the smoke gate.

## Fold Diagnostics

| Model | Dataset | Threshold | Selected | TrainScore |
| --- | --- | ---: | ---: | ---: |
| `ridge` | `cqadupstack` | 0.002401 | 55 | +0.024357 |
| `ridge` | `scidocs` | inf | 0 | -0.019458 |
| `ridge` | `webis-touche2020` | inf | 0 | +0.003519 |
| `hgb` | `cqadupstack` | 0.004458 | 27 | +0.025732 |
| `hgb` | `scidocs` | inf | 0 | -0.019458 |
| `hgb` | `webis-touche2020` | 0.000000 | 24 | +0.016429 |

The fold behavior is unstable.  Some train folds find a promising threshold,
but the heldout replay does not transfer.

## Interpretation

M1271 is another controlled no-go.

What it rejects:

- simple scalar action-value regression over current movement features;
- thresholding predicted value as a sufficient abstain policy;
- scaling this exact model family to full `shared15`.

What remains:

- M1269 still showed real movement-space observability;
- M1267 still shows strong safe-oracle capacity;
- the missing piece is not just binary-vs-regression loss.

The likely issue is transfer: action labels that look safe in one hard-row
surface do not generalize cleanly to the heldout row.  Before another model, the
next step should directly audit action-level safe/unsafe separability across
datasets and action types.

## Decision

Do not run full `shared15` for M1271.

Next valid step:

1. Audit all candidate actions, not only M1268 predicted actions.
2. Measure safe-vs-unsafe separability by dataset and by mix.
3. Identify whether failures come from dataset shift, action-source mismatch,
   or feature insufficiency.
4. Only after that decide whether to change the candidate action source,
   add stronger observability, or abandon raw/uniform action learning.
