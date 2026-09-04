# M1160 Compact Multi-Task Atom Selector

## Purpose

M1160 tested whether the compact M1157/M1159 proposal/proxy feature surface can
support a soft multi-task atom selector rather than only a hard threshold gate.

The labels were trained in leave-one-dataset-out mode:

- `is_safe_gain`
- `is_safe`
- `is_rank_safe`
- `no_relevant_exit`
- `has_relevant_entrant`

The feature set was intentionally compact: `proposal_proxy`.

Artifacts:

- `scripts/audit_m1160_compact_multitask_atom_selector.py`
- `runs/m1160_compact_multitask_atom_selector_v1/compact_multitask_atom_selector.json`
- `runs/m1160_compact_multitask_atom_selector_v1/summary.md`

## Label Surface

- query_count: `149`
- atom_count: `2370`
- has_relevant_entrant_rate: `0.043460`
- is_rank_safe_rate: `0.965401`
- is_safe_gain_rate: `0.146414`
- is_safe_rate: `0.780591`
- no_relevant_exit_rate: `0.961603`
- relevant_balance_positive_rate: `0.029536`

## Key Results

M1159 hard protected-boundary baseline was reproduced exactly:

| Policy | row_floor_clean | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `hard_g0.50_s0.50_r0.70_e0.50` | True | +0.000051 | +0.000551 | +0.000403 | +0.001546 | +0.003356 |

Best clean product policies found a different tradeoff:

| Policy | row_floor_clean | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `product_035_t0.05` / `product_035_t0.08` | True | +0.000075 | +0.002105 | +0.000854 | +0.001718 | +0.000671 |
| `product_022_t0.10` | True | +0.000068 | +0.001985 | +0.000846 | +0.001718 | +0.000671 |

The highest macro-utility penalty policies were not acceptable because they
had row-level regressions:

- `penalty_045_t0.00`: dRecall@100 `+0.011370`, but min dataset MAP `-0.000978`
  and min dataset NDCG `-0.001124`.

## Interpretation

M1160 did not replace M1159 with a single better selector.  It exposed two
complementary clean behaviors:

- the hard protected-boundary gate is MRR-safe and top-rank conservative;
- the product selector admits many more safe actions and improves
  Recall/MAP/NDCG, but it gives up much of the MRR gain.

This is a useful structural signal.  The next step is not more raw threshold
search; it is a stacked policy that lets the hard gate protect high-rank
movements and uses the product selector only as a fallback expansion for
queries left at `base`.

That follow-up is M1161.
