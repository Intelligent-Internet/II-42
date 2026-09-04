# M1165 Detector Atom Proposal Gap

M1165 audits the 138 M1163-selected queries and compares three choices:

- M1163 detector direct protected-tail action.
- M1161 selected generated atom action.
- M1155 oracle-safe best single generated atom from the existing proposal pool.

This separates selector error from proposal/teacher error.

## Inputs

- Query universe: M1143 shared15 protected-tail universe, 1342 queries.
- Selected event detector: M1163 `m1144_supported_by_m1145`,
  threshold44 = 0.60, threshold45 = 0.30.
- Selected atom policy: M1161
  `stack_hard_g0.70_s0.50_r0.70_e0.50_then_product_035_t0.05`.
- Output:
  `runs/m1165_detector_atom_proposal_gap_v1/detector_atom_proposal_gap.json`.

## Selected-Query Matrix

All deltas are per-query contributions over the full 1342-query surface.

| View | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1163 detector direct | +0.000074 | +0.001419 | +0.001400 | +0.000900 | +0.000000 |
| M1161 selected atom | +0.000006 | +0.000061 | +0.000024 | +0.000069 | +0.000000 |
| M1155 oracle-safe atom | +0.000079 | +0.000207 | +0.000116 | +0.000191 | +0.000000 |
| Best-any atom | +0.000010 | +0.000196 | +0.000118 | +0.000275 | +0.000000 |
| Oracle-safe minus selected atom | +0.000073 | +0.000146 | +0.000092 | +0.000122 | +0.000000 |
| Oracle-safe minus detector | +0.000005 | -0.001212 | -0.001284 | -0.000709 | +0.000000 |

## Counts

```json
{
  "best_any_dominates_detector": 54,
  "best_any_moved": 43,
  "oracle_safe_dominates_detector": 54,
  "oracle_safe_dominates_selected": 137,
  "oracle_safe_ge_detector_candidate_upper_bound": 121,
  "oracle_safe_ge_detector_map_at_100": 57,
  "oracle_safe_ge_detector_mrr_at_20": 138,
  "oracle_safe_ge_detector_ndcg_at_10": 122,
  "oracle_safe_ge_detector_recall_at_100": 82,
  "oracle_safe_moved": 42,
  "selected_atom_moved": 44,
  "selected_equals_oracle_safe": 95
}
```

## Interpretation

The selector is not the main bottleneck.  Oracle-safe atoms dominate the M1161
selected atom on 137/138 selected queries, but the improvement is still far
below detector direct on Recall, MAP, and NDCG.

The current proposal pool can often protect candidate upper bound:
oracle-safe atom is slightly above detector direct on CUB, and it is CUB-safe
on 121/138 selected queries.  But it cannot reproduce the ranking movement
that detector direct obtains.  This is a proposal/teacher shape problem, not
just a classifier threshold problem.

Best-any atom does not solve the gap either.  It slightly improves NDCG over
oracle-safe atom but still remains far below detector direct on Recall and MAP.

## Decision

Stop tuning M1161-style atom selector on the current M1155 proposal pool.  The
next useful branch should rebuild generated atom targets around the M1163
protected-tail direct action:

- derive atom deltas from the direct protected-tail movement, not only from
  top admission atoms;
- explicitly train for top100 boundary entry/exit effects;
- keep dense-equivalence guards, especially CUB and no relevant exit;
- evaluate first on the 138 M1163-selected queries, then project to shared15.
