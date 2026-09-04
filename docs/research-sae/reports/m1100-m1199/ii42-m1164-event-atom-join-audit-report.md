# M1164 Event Detector x Atom Recovery Join Audit

M1164 joins the best clean M1163 consensus event detector with the best M1161
stacked atom-recovery policy.  The goal is to determine whether the deployable
event detector can simply hand off to the existing generated-atom branch, or
whether the atom proposal/teacher needs to be rebuilt.

## Inputs

- Query surface: M1143 shared15 protected-tail universe, 1342 queries.
- Event detector: M1163 `m1144_supported_by_m1145`,
  threshold44 = 0.60, threshold45 = 0.30.
- Atom policy: M1161
  `stack_hard_g0.70_s0.50_r0.70_e0.50_then_product_035_t0.05`.
- Output:
  `runs/m1164_event_atom_join_audit_v1/event_atom_join_audit.json`.

## Overall Matrix

All deltas are per-query contributions over the full 1342-query surface.

| View | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1163 detector direct | +0.000074 | +0.001419 | +0.001400 | +0.000900 | +0.000000 |
| M1161 atom on existing surface | +0.000008 | +0.000234 | +0.000112 | +0.000251 | +0.000447 |
| Atom if available else detector | -0.000300 | -0.000042 | +0.000116 | +0.000427 | +0.000447 |
| Detector minus atom | +0.000066 | +0.001186 | +0.001288 | +0.000649 | -0.000447 |
| Hybrid minus detector | -0.000374 | -0.001462 | -0.001284 | -0.000473 | +0.000447 |

## Category Breakdown

| Category | Count | Detector selected | Atom moved | Detector guard harm | Detector dCUB | Detector dRecall | Detector dMAP | Detector dNDCG | Detector dMRR | Atom dCUB | Atom dRecall | Atom dMAP | Atom dNDCG | Atom dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `recall_gain_atom_surface` | 149 | 47 | 140 | 4 | +0.000382 | +0.001695 | +0.001396 | +0.000724 | +0.000000 | +0.000008 | +0.000234 | +0.000112 | +0.000251 | +0.000447 |
| `safe_only_no_atom_surface` | 234 | 25 | 0 | 2 | +0.000005 | +0.000000 | +0.000129 | -0.000032 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `no_oracle_event` | 959 | 66 | 0 | 31 | -0.000313 | -0.000276 | -0.000125 | +0.000207 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

## Interpretation

The deployable M1163 detector is not primarily discovering a new
`oracle_safe`-only surface.  Most useful detector contribution comes from the
existing `recall_gain_atom_surface`: +0.001695 Recall and +0.001396 MAP over
149 event queries.  The current M1161 generated-atom policy on the same surface
is much weaker on CUB/Recall/MAP/NDCG, though it contributes MRR.

The naive hybrid is not acceptable.  Replacing detector actions with current
atom actions where atom coverage exists loses CUB, Recall, MAP, and NDCG.  This
means the bottleneck is not simply event detection.  The current atom proposal
or teacher shape does not reproduce the protected-tail direct action.

## Next Step

M1165 should audit the atom proposal gap on the M1163-selected event set:

- For selected `recall_gain_atom_surface` queries, compare detector direct
  action against the best available atom candidate, not only the chosen M1161
  action.
- If an atom candidate can match detector direct but M1161 misses it, fix the
  selector/objective.
- If no atom candidate can match detector direct, rebuild the atom proposal
  teacher around protected-tail direct actions instead of continuing selector
  tuning.
- `no_oracle_event` selections must be guarded or filtered because they carry
  negative CUB/Recall/MAP despite a small NDCG gain.
