# M1153 Admission Policy Proxy

## Objective

Test whether the M1152 high-gain admission move can be selected by a deployable
query policy instead of by oracle qrels. The policy only uses inference-time
M1151 proxy score-shape features and is validated with leave-one-dataset-out
training.

## Inputs

- M1151 proxy predictions:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- M1152 native replay:
  `runs/m1152_admission_proxy_native_replay_v1/native_replay.json`
- M1153 policy audit:
  `runs/m1153_admission_policy_proxy_v1/policy_proxy.json`

## Macro Result

| Policy | Action mix | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| fixed_top4_s0.01 | top4_s0.01:149 | +0.000100 | +0.001216 | +0.000614 | +0.001320 | +0.000112 |
| fixed_top4_s0.05 | top4_s0.05:149 | -0.000359 | +0.012081 | +0.001922 | +0.002228 | +0.001790 |
| oracle_safe | mixed | +0.000488 | +0.013156 | +0.003487 | +0.004800 | +0.001790 |
| lodo_multiclass | mixed | +0.000000 | +0.004256 | +0.001480 | +0.000766 | +0.000000 |
| lodo_high_gate_t0.30 | top4_s0.01:18, top4_s0.05:131 | +0.000100 | +0.011946 | +0.001775 | +0.000954 | +0.001790 |
| lodo_high_gate_t0.50 | top4_s0.01:56, top4_s0.05:93 | +0.000100 | +0.011540 | +0.001280 | +0.000940 | +0.000112 |

## Interpretation

This is a meaningful positive signal:

- The fixed conservative policy `top4_s0.01` is safe but small.
- The fixed high-gain policy `top4_s0.05` has large recall gain but spends CUB.
- A LODO high-gate using only proxy-score-shape features recovers most of the
  high-gain recall while keeping all macro metrics positive.

The result also has a clear limitation. The best macro policy is not
per-dataset safe: `dbpedia-entity`, `webis-touche2020`, and a few small rows
still show local MAP/NDCG regressions. Therefore M1153 should not be promoted
as a final default policy.

Threshold tuning is not enough. Every non-oracle deployable policy still has at
least one dataset-level regression:

- `fixed_top4_s0.01`: small `dbpedia-entity` Recall and `cqadupstack` MAP
  regressions.
- `lodo_high_gate_t0.30`: stronger macro gains, but small MAP/NDCG regressions
  on `dbpedia-entity` and MAP regression on `webis-touche2020`.
- `lodo_multiclass`: safer action mix, but still has MAP/NDCG regressions on
  `dbpedia-entity`.

Only `oracle_safe` is row-floor clean, which means the missing ingredient is
not a better global threshold. It is a deployable row-risk/native-boundary-risk
model.

## Decision

Keep M1153 as a true signal that learned risk selection is possible. The next
step should add an explicit row-floor objective:

1. Penalize dataset-level MAP/NDCG regressions in the policy selection audit.
2. Add native query-risk features beyond proxy score shape, such as base rank
   margins, top100 boundary margins, atom fanout, and score dispersion.
3. Keep `top4_s0.01` as the fallback action.
4. Require both macro improvement and no material row-level MAP/NDCG regression
   before broader native replay.
