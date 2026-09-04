# M1162 Stacked Selector Projection Gate

## Purpose

M1161 improved the M1159 event-query selector on the 149 M1147 boundary-event
queries.  M1162 asks a broader but cheap question:

> If M1161 is applied only to those known event queries and every other M1143
> shared15 query abstains, does the signal remain clean at the full-query
> surface?

This is a conservative projection, not a native replay replacement.

Artifacts:

- `scripts/audit_m1162_stacked_selector_projection.py`
- `runs/m1162_stacked_selector_projection_v1/stacked_selector_projection.json`
- `runs/m1162_stacked_selector_projection_v1/summary.md`

## Inputs

- Full query universe: `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- Event selector: `runs/m1161_stacked_compact_atom_selector_v1/stacked_compact_atom_selector.json`
- Event marginal replay: `runs/m1155_marginal_atom_admission_v1/marginal_atom_replay.json`

The selected M1161 policy is:

`stack_hard_g0.70_s0.50_r0.70_e0.50_then_product_035_t0.05`

## Result

Projection over the M1143 shared15 universe:

| Aggregate | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| query-weighted | +0.000008 | +0.000234 | +0.000112 | +0.000251 | +0.000447 |
| dataset macro | +0.000015 | +0.000209 | +0.000104 | +0.000233 | +0.000400 |
| min dataset | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

Coverage:

| Item | Value |
| --- | ---: |
| total queries | 1342 |
| event queries | 149 |
| event rate | 0.111028 |
| moved queries | 140 |
| moved rate | 0.104322 |
| row-floor clean | true |

## Interpretation

M1162 keeps the M1161 signal alive, but bounds its current impact:

- The event-query policy remains clean when projected to all shared15 queries.
- The full-surface gain is small because only 149/1342 queries are in the event
  surface and only 140 are moved by the selected policy.
- This means the current bottleneck is not another M1161 threshold search.  The
  bottleneck is event detection and proposal coverage.

The result supports this decision:

1. Keep M1161 as the current best event-query recovery policy.
2. Do not promote it as a full shared15 breakthrough.
3. Move the next research step to finding deployable event/query detection or
   expanding the proposal surface beyond the M1147 oracle-recall-gain events.

## Stop / Continue

Continue the branch, but change the question.

Rejected next step:

- More M1161 product/hard threshold micro-tuning on the same 149 queries.

Useful next step:

- M1163 should audit whether existing M1144/M1145 native boundary features can
  detect the broader M1143 `oracle_safe` or `oracle_recall_gain` action set with
  high enough precision.  If not, the branch needs new event features or a
  different proposal generator before native replay expansion.
