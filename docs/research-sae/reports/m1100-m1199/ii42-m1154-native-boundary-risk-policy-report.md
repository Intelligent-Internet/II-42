# M1154 Native Boundary Risk Policy

## Objective

Extend M1153 with native query-time boundary features and test whether a
deployable risk model can keep M1153-style macro gains while removing
dataset-level MAP/NDCG regressions.

## Inputs

- M1151 proxy predictions:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- M1152 native replay:
  `runs/m1152_admission_proxy_native_replay_v1/native_replay.json`
- M1154 native boundary cache:
  `runs/m1154_native_boundary_risk_policy_v1/native_boundary_features.json`
- M1154 policy audit:
  `runs/m1154_native_boundary_risk_policy_v1/risk_policy.json`

## Best Policies

| Policy | Action mix | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | min_ds_MAP | min_ds_NDCG |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed_top4_s0.01 | top4_s0.01:149 | +0.000100 | +0.001216 | +0.000614 | +0.001320 | +0.000112 | -0.000233 | +0.000000 |
| proxy_high_safe_gain_fbtop4_s0.01_t0.30 | top4_s0.01:18, top4_s0.05:131 | +0.000100 | +0.011946 | +0.001775 | +0.000954 | +0.001790 | -0.001589 | -0.001506 |
| proxy_native_high_safe_gain_fbtop4_s0.01_t0.70 | top4_s0.01:112, top4_s0.05:37 | +0.000100 | +0.010095 | +0.001189 | +0.001209 | +0.000112 | -0.000792 | +0.000000 |
| proxy_native_high_safe_gain_fbbase_t0.90 | base:129, top4_s0.05:20 | +0.000000 | +0.000171 | +0.000171 | +0.000014 | +0.000000 | -0.000128 | +0.000000 |

## Interpretation

Native boundary features helped, but did not solve the row-floor problem.

- `proxy_native_high_safe_gain_fbtop4_s0.01_t0.70` keeps most of the useful
  macro gain and removes NDCG row regression, but still has MAP regressions.
- `proxy_native_high_safe_gain_fbbase_t0.90` nearly removes row regression, but
  it is mostly no-op and the gain is too small to justify promotion.
- No non-base policy is fully row-floor clean.

This is a useful negative result. The failure is not that learned admission has
no signal; M1152/M1153 already show signal. The failure is that a query-level
gate is too coarse. M1150's oracle operated at the per-atom admission level,
while M1152-M1154 admitted topK atoms as a batch. The remaining regressions are
consistent with one or two harmful atoms contaminating an otherwise useful
query move.

## Decision

Stop query-level threshold/gate tuning here. The next structurally different
experiment should be per-atom marginal admission:

1. Replay the top M1151 predicted atoms individually through the native path.
2. Label each atom by marginal gain/regression instead of labeling the whole
   query move.
3. Train an atom-level risk/admission model using proposal features plus native
   boundary context.
4. Compose accepted atoms greedily with metric floors, approximating M1150
   without qrels at inference time.
