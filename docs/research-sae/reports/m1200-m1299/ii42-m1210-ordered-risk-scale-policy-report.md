# M1210 Ordered-Risk Scale Policy

M1210 tests an ordered action policy built from the M1208 scale frontier.
Instead of a binary `top3` versus `top1` veto, it adds a middle action:

- high: `top3_s0.10`
- mid: `top3_s0.075`
- low: `top1_s0.10`

The learned policy is a two-stage leave-one-dataset-out logistic cascade:

1. Predict whether high should be avoided.
2. If high is risky, predict whether mid should also be avoided.

All runtime features are qrels-free.  Qrels are used only for train-fold labels
and threshold selection.

## Macro Result

| Variant | Action Mix | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best4_with_baseline` | baseline 0.689 / high 0.206 / mid 0.028 / low 0.077 | 0 | +0.003013 | +0.006642 | +0.006824 | +0.005578 | +0.000714 | 0.060505 |
| `oracle_best3_actions` | high 0.828 / mid 0.048 / low 0.124 | 0 | +0.002599 | +0.005778 | +0.005905 | +0.004721 | +0.000511 | 0.052089 |
| `ordered_utility_or_cub_logistic` | high 0.735 / mid 0.084 / low 0.180 | 0 | +0.002009 | +0.003318 | +0.002748 | +0.002275 | +0.000004 | 0.030049 |
| `mid_top3_s0.075` | mid 1.000 | 1 | +0.001610 | +0.003509 | +0.003358 | +0.002682 | -0.000033 | 0.029799 |
| `high_top3_s0.10` | high 1.000 | 1 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.029555 |
| `ordered_rank_metric_logistic` | high 0.734 / mid 0.047 / low 0.219 | 0 | +0.001929 | +0.003206 | +0.002344 | +0.002162 | +0.000050 | 0.028324 |
| `ordered_any_metric_logistic` | high 0.761 / mid 0.076 / low 0.163 | 0 | +0.002056 | +0.002814 | +0.002190 | +0.001672 | +0.000051 | 0.026498 |
| `low_top1_s0.10` | low 1.000 | 0 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 | 0.016189 |

## Comparison To M1191

The current best deployable control route before this run was M1191
`veto_utility_or_cub_logistic`:

- dRecall: +0.002065
- dMAP: +0.003412
- dNDCG: +0.002557
- dMRR: +0.002157
- dCUB: +0.000002

M1210 `ordered_utility_or_cub_logistic` is effectively tied with M1191:

- slightly lower Recall and MAP
- higher NDCG and MRR
- CUB remains non-negative
- weighted score is slightly higher under the current score function

This is a real signal, but not large enough to call a breakthrough.

## Key Observation

The useful new evidence is the oracle mix.

`oracle_best4_with_baseline` chooses baseline for about 69% of queries and
achieves much larger gains than any always-delta or binary-veto route.  This
means the remaining policy problem is not only top3-vs-top1 risk.  A large
part of the frontier is deciding when not to perturb the query at all.

This also explains why previous local policies kept plateauing: they forced
every query to accept some generated posting delta, even when baseline was the
best action.

## Decision

- Keep M1191 as the current default because it has the stronger Recall/MAP
  profile and is already documented.
- Keep M1210 as a serious candidate because it matches M1191 while improving
  rank-side metrics.
- Do not continue scale/classifier swaps inside the no-abstain policy.

## Next Direction

Run M1211: abstain-aware ordered policy.

The next policy family should have four ordered choices:

1. baseline abstain
2. high `top3_s0.10`
3. mid `top3_s0.075`
4. low `top1_s0.10`

The scientific question is whether the large oracle_best4 gap is learnable
from qrels-free features.  If it is not learnable, the local policy family is
probably capped and the next stage must change proposal generation rather than
continue risk-classifier tuning.

## Artifacts

- JSON: `runs/m1210_ordered_risk_scale_policy_v1/m1210_ordered_risk_scale_policy.json`
- Markdown: `runs/m1210_ordered_risk_scale_policy_v1/m1210_ordered_risk_scale_policy.md`
- Script: `scripts/audit_m1210_ordered_risk_scale_policy.py`
