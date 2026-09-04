# M736A Positive-Enriched Bundle Replay

M736A increases teacher density after M735 by replaying small atom
bundles through the native unified-posting scorer.  Oracle surfaces
may use M735 labels and are teacher-only; model surfaces use only
M735B model scores.

## Outputs

- Rows: `runs/m736a_positive_enriched_bundle_replay_v1/m736a_rows.jsonl`
- JSON: `runs/m736a_positive_enriched_bundle_replay_v1/m736a_summary.json`

## Surface Summary

| Surface | Split | Rows | Bundle Safe | Productive | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_productive_bundle_s0.005` | `full` | 80 | 0.850000 | 0.037500 | +0.000000 | -0.000018 | +0.001129 | -0.000298 | -0.002500 | -0.000375 | +0.000098 | 0 |
| `model_productive_bundle_s0.005` | `holdout` | 10 | 0.800000 | 0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.001000 | +0.000000 | 0 |
| `oracle_productive_bundle_s0.005` | `full` | 18 | 1.000000 | 1.000000 | +0.000000 | +0.003795 | +0.008717 | +0.011111 | +0.000000 | +0.001111 | +0.002170 | 1 |
| `oracle_productive_bundle_s0.005` | `holdout` | 1 | 1.000000 | 1.000000 | +0.000000 | +0.000391 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `full` | 18 | 0.944444 | 0.888889 | +0.000000 | +0.003522 | +0.008139 | +0.009259 | +0.000000 | +0.001111 | +0.001736 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `holdout` | 1 | 1.000000 | 1.000000 | +0.000000 | +0.000391 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |

## Decision

M736A oracle bundles are viable, but the current M735B model scores do not recover productive holdout bundles. Next step is a larger positive-enriched teacher and selector redesign, not deep compiler training yet.
