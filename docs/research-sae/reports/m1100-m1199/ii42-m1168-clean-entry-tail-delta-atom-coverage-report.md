# M1168 Clean-Entry Tail-Delta Atom Coverage

M1168 checks whether M1166 direct-action rows have a compact query-atom delta
between M1129 base and M1137 tail, and whether the old M1151/M1155 proposal
pool covers that delta.

## Inputs

- Witness rows: M1166, 138 selected direct protected-tail actions.
- Base query atoms: M1129 export under
  `runs/m1142_sae_atom_export_shared15_v1/m1129`.
- Tail query atoms: M1137 export under
  `runs/m1142_sae_atom_export_shared15_v1/m1137`.
- Old proposals: M1151 admission proxy and M1155 marginal atom replay.
- Output:
  `runs/m1168_clean_entry_tail_delta_atom_coverage_v1/clean_entry_tail_delta_atom_coverage.json`.

## Class Summary

| Class | Count | Actions | Delta atoms | Added atoms | Top delta sum | M1151 delta@4 | M1151 added@4 | M1155 delta@4 | M1155 added@4 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `clean_relevant_entry` | 26 | `{'protect20': 25, 'protect5': 1}` | 3.962 | 3.692 | 2.354983 | 0.087 | 0.019 | 0.087 | 0.019 |
| `relevant_swap` | 57 | `{'protect20': 28, 'protect5': 29}` | 3.877 | 3.596 | 1.576482 | 0.022 | 0.000 | 0.022 | 0.000 |
| `rank_gain_no_top100_recall` | 16 | `{'protect20': 16}` | 5.688 | 5.250 | 2.343900 | 0.000 | 0.000 | 0.000 | 0.000 |
| `no_relevant_boundary_change` | 33 | `{'protect20': 21, 'protect5': 12}` | 6.212 | 5.879 | 3.279486 | 0.000 | 0.000 | 0.000 | 0.000 |
| `relevant_exit_only` | 6 | `{'protect20': 6}` | 5.000 | 4.833 | 0.804072 | 0.000 | 0.000 | 0.000 | 0.000 |

## Interpretation

The clean-entry direct action has a compact atom-delta explanation: roughly
four positive M1137-minus-M1129 query atoms per query.  But the old M1151/M1155
proposal pool almost never covers those atoms.  For clean-entry rows, M1155
top4 delta coverage is only 0.087, and added-atom coverage is 0.019.

This explains the M1165 gap.  The existing selector was asked to recover a
direct protected-tail behavior from a proposal pool that usually does not
contain the atoms responsible for that behavior.

## Decision

Stop recycling M1151/M1155 proposal atoms for this branch.  The next small
native replay should test tail-delta atoms directly:

1. Use M1166 `clean_relevant_entry` rows only.
2. Add top positive M1137-minus-M1129 atom deltas to the M1129 query.
3. Replay top1/top2/top4 delta additions through the native table path.
4. If top-delta replay recovers a meaningful fraction of direct protected-tail
   Recall/MAP without guard harm, promote this into a new teacher/recovery
   training objective.
5. If it fails, the direct action depends on full tail geometry rather than
   compact atom additions, and this branch should stop before training.
