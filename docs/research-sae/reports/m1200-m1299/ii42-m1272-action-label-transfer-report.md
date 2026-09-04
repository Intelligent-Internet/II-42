# M1272 Action Label Transfer Audit

M1272 reviews the current M1266/M1267 raw-uniform action family after M1270
and M1271 failed to produce a deployable movement-aware selector.  The goal is
not to train another gate.  It is to check whether safe-vs-unsafe action labels
are stable and observable enough to justify another model, or whether the route
needs a different source/objective.

## Inputs

- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Actions: non-raw mix variants from the raw/uniform anchor family
- Action count: `996`
- Output JSON:
  `runs/m1272_action_label_transfer_v1/m1272_action_label_transfer.json`
- Output detail:
  `runs/m1272_action_label_transfer_v1/m1272_action_label_transfer.md`

## Label Stability

| Dataset | Count | Safe Rate | Mean Score Delta |
| --- | ---: | ---: | ---: |
| `cqadupstack` | 400 | 0.0850 | 0.000846 |
| `scidocs` | 400 | 0.1500 | -0.008912 |
| `webis-touche2020` | 196 | 0.1173 | -0.008829 |

| Mix | Count | Safe Rate | Mean Score Delta |
| --- | ---: | ---: | ---: |
| `m1266_mix0p25` | 249 | 0.0723 | -0.003571 |
| `m1266_mix0p5` | 249 | 0.1124 | -0.006433 |
| `m1266_mix0p75` | 249 | 0.1406 | -0.004313 |
| `m1266_mix1` | 249 | 0.1446 | -0.005591 |

The label rate does shift by row, but it is not the main blocker by itself:
the held-out safe-rate range is `0.0850` to `0.1500`, below the `0.15`
strong-shift threshold used by this audit.  The more important signal is that
most action sources have negative mean score delta even when they contain a
small safe subset.

## Observability

| Feature | Oriented AUC | Safe Mean | Unsafe Mean |
| --- | ---: | ---: | ---: |
| `rank_up_count` | 0.7352 | 21.8205 | 13.5438 |
| `rank_down_count` | 0.7345 | 21.3846 | 13.3515 |
| `shared_abs_rank_delta_mean` | 0.7321 | 0.7796 | 0.4411 |
| `exit_fused_rank_mean` | 0.5992 | 538.2621 | 679.8427 |
| `exit_fused_score_mean` | 0.5929 | 0.5770 | 0.3863 |
| `entrant_fused_score_mean` | 0.5922 | 0.5755 | 0.3863 |

Safe actions are not invisible.  Movement magnitude features reach about
`0.73` oriented AUC, which explains why M1269 saw real observability.  However,
M1270 and M1271 show that this observability is not enough for a deployable
pointwise classifier or pointwise action-value regressor.

## Interpretation

This supports the external review's core point:

- `M1000+` did not solve the route, but it did localize the bottleneck.
- Useful added atoms/actions exist.
- CUB-specific/action-conditioned structure exists.
- The current failure is not "no signal"; it is unsafe action/source selection
  under qrels-free query-time observability.

The practical update is narrower:

1. Stop ordinary selector/gate/filter micro-tuning on the same action family.
2. Do not promote raw/uniform mixes or movement gates as default.
3. Keep movement features as diagnostics and possible training features.
4. Move the next attempt into row-balanced/listwise objective or action-source
   construction where harm separation is built in, not bolted on afterwards.

## Decision

M1272 is a route-control result, not a deployable improvement.  It preserves the
raw/uniform anchor family as an oracle-capacity source, but rejects more
pointwise gate/regressor variants as the next step.

Next work should either:

- train a row-balanced/listwise action selector with explicit row harm budgets,
  using M1272 movement features as inputs; or
- rebuild the candidate/action source so that safe actions are generated with
  better target/harm separation before any selector is trained.
