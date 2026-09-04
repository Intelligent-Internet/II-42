# M1161 Stacked Compact Atom Selector

## Purpose

M1161 tested the structural hypothesis from M1160:

> use the M1159-style hard gate first, then apply the M1160 product selector
> only for queries left at the base action.

This keeps the MRR/top-rank protection from the hard gate while allowing the
product selector to recover additional Recall/MAP/NDCG on unchanged queries.

Artifacts:

- `scripts/audit_m1161_stacked_compact_atom_selector.py`
- `runs/m1161_stacked_compact_atom_selector_v1/stacked_compact_atom_selector.json`
- `runs/m1161_stacked_compact_atom_selector_v1/summary.md`

## Best Policy

Best stacked policy:

`stack_hard_g0.70_s0.50_r0.70_e0.50_then_product_035_t0.05`

Parameters:

- hard gate: `gain_threshold=0.70`, `safe_threshold=0.50`,
  `rank_threshold=0.70`, `exit_threshold=0.50`
- product fallback: `safe_power=0.5`, `rank_power=2.0`,
  `exit_power=2.0`, `entrant_weight=1.0`, `threshold=0.05`

## Result

The best stacked policy is row-floor clean and strictly dominates the M1159
clean baseline across all tracked macro deltas on this native replay surface.

| Policy | row_floor_clean | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1159 hard baseline | True | +0.000051 | +0.000551 | +0.000403 | +0.001546 | +0.003356 |
| M1161 best stacked | True | +0.000075 | +0.002105 | +0.001008 | +0.002263 | +0.004027 |
| Gain vs M1159 | - | +0.000024 | +0.001554 | +0.000604 | +0.000717 | +0.000671 |

The JSON check also records:

- `dominates_m1159_baseline: true`
- min dataset deltas for MAP/NDCG/MRR/Recall/CUB are all `0.0`

## Interpretation

This is a real local improvement, but not yet a final benchmark claim.

The important lesson is that the clean signal is hierarchical:

- first choose high-confidence MRR-safe boundary movements;
- then use a broader product selector only for base-leftover queries.

This avoids the failure mode where a globally aggressive product or penalty
selector improves macro Recall while causing row-level MAP/NDCG regressions.

## Next Gate

M1161 should be promoted to a broader native replay/evaluation gate before it
is treated as a new baseline:

1. replay the stacked policy on a broader shared/native surface;
2. verify row-floor cleanliness still holds;
3. compare against M1159 and the frozen P1 native baseline;
4. only then consider moving it into the engineering index path.

If the stacked advantage disappears outside this surface, keep M1159 as the
conservative baseline and treat M1161 as a surface-local diagnostic.
