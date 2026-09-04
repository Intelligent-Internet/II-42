# M1211 Abstain-Aware Ordered-Risk Policy

M1211 tests the main structural implication from M1210: the oracle frontier is
much larger when the policy can choose baseline abstain.  The action set is:

- baseline abstain
- high: `top3_s0.10`
- mid: `top3_s0.075`
- low: `top1_s0.10`

The learned policy is a three-stage leave-one-dataset-out logistic cascade:

1. Predict whether to abstain.
2. If not abstaining, predict whether high should be avoided.
3. If high is risky, predict whether mid should also be avoided.

All runtime features are qrels-free.  Qrels are only used for train-fold labels
and threshold selection.

## Macro Result

| Variant | Action Mix | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best4_with_baseline` | baseline 0.689 / high 0.206 / mid 0.028 / low 0.077 | 0 | +0.003013 | +0.006642 | +0.006824 | +0.005578 | +0.000714 | 0.060505 |
| `oracle_best3_actions` | high 0.828 / mid 0.048 / low 0.124 | 0 | +0.002599 | +0.005778 | +0.005905 | +0.004721 | +0.000511 | 0.052089 |
| `mid_top3_s0.075` | mid 1.000 | 1 | +0.001610 | +0.003509 | +0.003358 | +0.002682 | -0.000033 | 0.029799 |
| `high_top3_s0.10` | high 1.000 | 1 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.029555 |
| `abstain_ordered_utility_or_cub_logistic` | baseline 0.214 / high 0.554 / mid 0.060 / low 0.172 | 0 | +0.002110 | +0.002345 | +0.002060 | +0.001815 | +0.000046 | 0.025379 |
| `abstain_ordered_rank_metric_logistic` | baseline 0.088 / high 0.694 / mid 0.045 / low 0.172 | 1 | +0.001456 | +0.002949 | +0.002401 | +0.001581 | -0.000046 | 0.022900 |
| `abstain_ordered_any_metric_logistic` | baseline 0.095 / high 0.721 / mid 0.063 / low 0.121 | 0 | +0.001020 | +0.002387 | +0.001913 | +0.001139 | +0.000124 | 0.018488 |
| `low_top1_s0.10` | low 1.000 | 0 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 | 0.016189 |

## Interpretation

This is a useful negative result.

The oracle_best4 gap is real and large.  If we could select abstain/high/mid/low
with qrels visibility, the surface would be much better than M1191/M1210.
However, the learned abstain-aware policies do not recover that gap.  The best
safe learned variant is below M1210 and below the M1191 frontier.

The failure mode is clear: abstain is valuable, but the current qrels-free
features do not identify the right abstain cases.  Adding abstain as another
threshold/classifier decision makes the policy more conservative and safer, but
it removes too much MAP/NDCG/MRR gain.

## Decision

- Do not promote M1211.
- Stop this local policy family: top3/top1 plus scale plus qrels-free logistic
  risk/abstain classification is capped.
- Preserve the oracle_best4 result as a design signal, not as a deployable
  route.

## Consequence

The next useful direction is not more local threshold tuning.  The data now
points to one of two routes:

1. Change proposal generation so the default delta is less dependent on
   query-level abstain classification.
2. Add a stronger observability source for abstain, such as native score-shape
   calibration, candidate boundary replay features, or a trained proposal
   confidence head with a real heldout gate.

The immediate recommendation is to stop M1210/M1211-style policy changes and
move to a new proposal generator or confidence signal.

## Artifacts

- JSON: `runs/m1211_abstain_ordered_risk_policy_v1/m1211_abstain_ordered_risk_policy.json`
- Markdown: `runs/m1211_abstain_ordered_risk_policy_v1/m1211_abstain_ordered_risk_policy.md`
- Script: `scripts/audit_m1211_abstain_ordered_risk_policy.py`
