# M736A Positive-Enriched Bundle Replay

M736A increases teacher density after M735 by replaying small atom
bundles through the native unified-posting scorer.  Oracle surfaces
may use M735 labels and are teacher-only; model surfaces use only
M735B model scores.

## Outputs

- Rows: `runs/m737b_single_atom_model_replay_v1/m736a_rows.jsonl`
- JSON: `runs/m737b_single_atom_model_replay_v1/m736a_summary.json`

## Surface Summary

| Surface | Split | Rows | Bundle Safe | Productive | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_productive_bundle_s0.005` | `full` | 199 | 0.954774 | 0.005025 | +0.000000 | -0.000003 | +0.000000 | +0.000000 | -0.000986 | +0.000101 | +0.000079 | 0 |
| `model_productive_bundle_s0.005` | `holdout` | 29 | 0.965517 | 0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `oracle_productive_bundle_s0.005` | `full` | 42 | 1.000000 | 1.000000 | +0.004762 | +0.005604 | +0.006001 | +0.005952 | +0.000000 | +0.001905 | +0.002697 | 1 |
| `oracle_productive_bundle_s0.005` | `holdout` | 5 | 1.000000 | 1.000000 | +0.040000 | +0.007166 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `full` | 42 | 1.000000 | 1.000000 | +0.004762 | +0.005604 | +0.006001 | +0.005952 | +0.000000 | +0.001905 | +0.002697 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `holdout` | 5 | 1.000000 | 1.000000 | +0.040000 | +0.007166 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |

## Decision

M736A oracle bundles are viable, but the current M735B model scores do not recover productive holdout bundles. Next step is a larger positive-enriched teacher and selector redesign, not deep compiler training yet.
