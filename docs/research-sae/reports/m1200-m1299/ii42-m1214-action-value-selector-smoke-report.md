# M1214 Action-Value Selector Smoke

M1214 tests whether the M1213 oracle gap can be learned by an action-value
selector instead of another binary harm classifier.

This is a hard-row smoke only.  The full shared15 run was not launched because
the smoke failed the safety gate.

Datasets:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Action family:

- baseline
- high: `bm25_top_docs3_s0.10`
- mid: `bm25_top_docs3_s0.075`
- low: `fused_tail_docs3_s0.10`

Model:

- leave-one-dataset-out
- `HistGradientBoostingRegressor`
- target 1: per-action protected score delta
- target 2: per-action CUB delta
- train-fold threshold selection over score margin and CUB floor
- inference-time features only

## Smoke Result

| Variant | Action Mix | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best4_with_baseline` | baseline 0.538 / high 0.313 / mid 0.044 / low 0.104 | 0 | +0.008544 | +0.007490 | +0.007550 | +0.005471 | +0.001888 | 0.093118 |
| `oracle_best3_actions` | high 0.719 / mid 0.108 / low 0.173 | 0 | +0.007535 | +0.006646 | +0.006291 | +0.005029 | +0.001085 | 0.081337 |
| `low_fused_tail` | low 1.000 | 0 | +0.001582 | +0.000658 | +0.000774 | +0.002524 | +0.000000 | 0.016482 |
| `baseline` | baseline 1.000 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000000 |
| `action_value_score_cub_hgb` | baseline 0.655 / high 0.064 / mid 0.120 / low 0.161 | 2 | -0.000803 | +0.000364 | +0.000220 | -0.000826 | +0.000000 | -0.012397 |

## Fold Diagnostics

The failure is not lack of train signal.  Train-fold threshold selection looked
strong:

| Heldout | Train Score | Train dRecall | Train dMAP | Train dNDCG | Train dMRR | Train dCUB | Heldout Oracle Match |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 0.106938 | +0.012453 | +0.005603 | +0.006321 | +0.006058 | +0.003104 | 0.510 |
| `scidocs` | 0.036486 | +0.001940 | +0.004675 | +0.004493 | +0.001678 | +0.000419 | 0.300 |
| `webis-touche2020` | 0.097263 | +0.009940 | +0.006040 | +0.007618 | +0.006102 | +0.002000 | 0.286 |

The train-fold score is high, but the held-out action selection regresses
Recall and MRR.  This is exactly the distribution-shift pattern we wanted to
test.

## Interpretation

M1214 rejects the local action-value selector route in its current form.

The result says:

- The M1213 oracle remains real.
- Existing qrels-free features do not make action choice reliably observable.
- Replacing harm classifiers with a value regressor does not solve the gap.
- The selector overfits train folds and fails on held-out hard rows.

This is not a reason to abandon unified posting.  It is a reason to stop this
local selector family.

## Decision

- Do not run full shared15 for M1214.
- Do not continue local selector variants over the same action family.
- Treat M1191/M1210 as the current learned frontier.
- Move the next experiment out of threshold/selector tuning.

## Next Direction

The next useful direction must change what is being learned, not only how the
selector chooses among fixed actions.

Two viable options remain:

1. Train a generated-posting objective that directly creates safer proposal
   atoms under dense/CUB constraints.
2. Build a richer native proposal generator with source-aware supervision,
   then re-evaluate with the existing M1191/M1210 safety policy.

The immediate next step should be a small generated-posting teacher audit:

- use oracle-winning actions from M1213 as positive movement examples
- decompose them into atom/query deltas
- measure whether the winning deltas are predictable from text/native atom
  features before training any larger model
- stop if positive deltas are not more structured than negatives

## Artifacts

- Script: `scripts/audit_m1214_action_value_selector.py`
- Smoke JSON: `runs/m1214_action_value_selector_smoke_v1/m1214_action_value_selector.json`
- Smoke Markdown: `runs/m1214_action_value_selector_smoke_v1/m1214_action_value_selector.md`
