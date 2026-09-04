# M1169 Clean-Entry Tail-Delta Native Replay

M1169 tests whether the compact M1137-minus-M1129 query-atom deltas from
M1168 can be added to the M1129 base query and recover the M1166 clean-entry
direct protected-tail gains through the native table path.

## Inputs

- Rows: 26 M1166 `clean_relevant_entry` queries.
- Base surface: M1129 query atoms and M1129 doc atom table.
- Tail reference: M1137 query atoms and M1137 doc atom table.
- Delta replay: add top positive M1137-minus-M1129 query atom deltas to the
  M1129 query, then score against the M1129 doc atom table.
- Output:
  `runs/m1169_clean_entry_tail_delta_native_v1/clean_entry_tail_delta_native.json`.

## Result

| Variant | Queries | Recall+ count | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `direct_protect` | 26 | 0 | -0.000211 | +0.064717 | +0.048494 | -0.000068 | +0.000000 |
| `tail_full` | 26 | 26 | -0.000211 | +0.064717 | +0.059726 | +0.000394 | +0.019231 |
| `delta_base_top1_s0.25` | 26 | 0 | +0.000000 | +0.000000 | +0.000199 | +0.001631 | +0.000000 |
| `delta_base_top1_s0.50` | 26 | 1 | -0.000211 | +0.001057 | +0.000945 | +0.001780 | +0.000000 |
| `delta_base_top1_s1.00` | 26 | 1 | -0.000211 | +0.001479 | +0.001435 | +0.001780 | +0.000000 |
| `delta_base_top2_s0.25` | 24 | 0 | +0.000000 | +0.000000 | +0.000146 | +0.001767 | +0.000000 |
| `delta_base_top2_s0.50` | 24 | 1 | -0.000229 | +0.001145 | +0.000945 | +0.001929 | +0.000000 |
| `delta_base_top2_s1.00` | 24 | 1 | -0.000229 | +0.001603 | -0.002771 | +0.000056 | +0.000000 |
| `delta_base_top4_s0.25` | 14 | 0 | +0.000000 | +0.000000 | +0.000362 | +0.003030 | +0.000000 |
| `delta_base_top4_s0.50` | 14 | 2 | +0.000000 | +0.002722 | +0.001965 | +0.003306 | +0.000000 |
| `delta_base_top4_s1.00` | 14 | 2 | +0.000000 | +0.003507 | -0.002773 | +0.003108 | +0.000000 |

## Interpretation

The full M1137 tail stream reproduces the clean-entry Recall gain exactly and
improves MAP/MRR over direct protected-tail.  But portable top-delta additions
to the M1129 base index recover only a tiny fraction of the gain:

- best `delta_base_*` Recall is +0.003507 on the subset with at least four
  positive delta atoms;
- direct/tail reference Recall is +0.064717;
- positive Recall appears in only 1 to 2 queries, while `tail_full` helps all
  26.

This means the direct gain is not explained by simply adding a few M1137 query
atoms to the M1129 query against the M1129 doc index.  It depends on the M1137
query and doc surface together, or on a richer transformation than compact
positive query-delta injection.

## Decision

Stop the "compact tail-delta additions to M1129 base index" branch.  It is not
a viable generated-atom teacher by itself.

The next plausible branches are:

1. Treat M1137-like behavior as a separate native surface and learn a safe
   query-time surface router/protected-tail policy.
2. Train a unified doc+query surface that internalizes the M1137 clean-entry
   behavior, then re-run the M1166/M1169 gates.
3. If staying single-index, use native replay supervision over doc/query
   surface jointly; do not assume query-only atom deltas are transferable.

This is useful because it prevents another training loop on a target that the
native replay already shows is too weak.
