# M1170 Selected Tail Surface Router Audit

M1170 tests a structural pivot suggested by M1169.  Instead of adding compact
M1137-M1129 query deltas into the M1129 base index, it compares the full M1137
tail surface against the M1163 direct protected-tail action on the same 138
selected queries.

## Inputs

- Selected rows: 138 M1163-selected direct protected-tail actions from M1166.
- Direct action: M1163/M1166 protected-tail composition.
- Tail surface: full M1137 query atoms against the M1137 doc atom table.
- Base reference: M1129 query atoms against the M1129 doc atom table.
- Output:
  `runs/m1170_selected_tail_surface_router_v1/selected_tail_surface_router.json`.

## Overall

| View | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct_protect | +0.000721 | +0.013801 | +0.013618 | +0.008751 | +0.000000 |
| tail_full | +0.000733 | +0.013691 | +0.018345 | +0.016553 | +0.022145 |
| tail_minus_direct | +0.000011 | -0.000110 | +0.004727 | +0.007801 | +0.022145 |

## By Class

| Class | Count | Tail better | Tail recall better | Direct dRecall | Tail dRecall | Tail-Direct dMAP | Tail-Direct dNDCG | Tail-Direct dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `clean_relevant_entry` | 26 | 12 | 0 | +0.064717 | +0.064717 | +0.011232 | +0.000461 | +0.019231 |
| `relevant_swap` | 57 | 26 | 2 | +0.012476 | +0.012502 | +0.004995 | +0.026173 | +0.051170 |
| `rank_gain_no_top100_recall` | 16 | 10 | 0 | +0.000000 | +0.000000 | +0.009773 | +0.018427 | +0.012277 |
| `no_relevant_boundary_change` | 33 | 9 | 0 | +0.000000 | +0.000000 | -0.003125 | -0.018971 | -0.016883 |
| `relevant_exit_only` | 6 | 4 | 0 | -0.081534 | -0.084311 | +0.003733 | -0.016009 | +0.000000 |

## Interpretation

This is the strongest structural signal in the recent branch.  M1169 showed
query-only tail-delta additions do not transfer to the M1129 base index.  M1170
shows the full M1137 surface itself is valuable:

- same CUB and almost same Recall as protected-tail direct action;
- much better MAP, NDCG, and MRR on selected queries;
- especially strong on `clean_relevant_entry`, `relevant_swap`, and
  `rank_gain_no_top100_recall`.

The failure mode is also clear.  `no_relevant_boundary_change` and
`relevant_exit_only` rows should not be blindly switched to tail_full because
tail_full hurts rank metrics or Recall there.

## Decision

The next branch should be a safe surface router, not compact query-delta
injection into the base index:

1. Keep M1129 as base.
2. Use M1163-style detector to identify candidate queries.
3. Learn a deployable gate for whether to route selected queries to M1137
   tail_full or keep protected-tail/direct/base behavior.
4. Guard Recall/CUB first; use tail_full mainly for MAP/NDCG/MRR recovery.

M1171 should test whether this tail-vs-direct decision can be predicted from
M1144/M1145 query-time features under leave-one-dataset-out validation.
