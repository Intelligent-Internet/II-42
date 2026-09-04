# M736A Positive-Enriched Bundle Replay

M736A increases teacher density after M735 by replaying small atom
bundles through the native unified-posting scorer.  Oracle surfaces
may use M735 labels and are teacher-only; model surfaces use only
M735B model scores.

## Outputs

- Rows: `runs/m736c_expanded_bundle_replay_v1/m736a_rows.jsonl`
- JSON: `runs/m736c_expanded_bundle_replay_v1/m736a_summary.json`

## Surface Summary

| Surface | Split | Rows | Bundle Safe | Productive | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_productive_bundle_s0.005` | `full` | 199 | 0.944724 | 0.020101 | +0.000000 | -0.000002 | +0.000000 | +0.000000 | -0.001005 | +0.000101 | +0.000157 | 0 |
| `model_productive_bundle_s0.005` | `holdout` | 29 | 0.931034 | 0.034483 | +0.000000 | +0.000003 | +0.000000 | +0.000000 | +0.000000 | -0.000345 | +0.000135 | 0 |
| `oracle_productive_bundle_s0.005` | `full` | 42 | 0.976190 | 0.976190 | +0.004762 | +0.005819 | +0.006249 | +0.006746 | +0.000000 | +0.001905 | +0.002697 | 1 |
| `oracle_productive_bundle_s0.005` | `holdout` | 5 | 1.000000 | 1.000000 | +0.040000 | +0.007242 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `full` | 42 | 0.952381 | 0.809524 | +0.004762 | +0.005709 | +0.006001 | +0.005952 | +0.000000 | +0.002143 | +0.002418 | 1 |
| `oracle_productive_plus_safe_bundle_s0.005` | `holdout` | 5 | 1.000000 | 1.000000 | +0.040000 | +0.007166 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |

## Decision

M736A model-selected bundles found productive rows but failed the strict holdout gate. Improve selector constraints before expansion.
