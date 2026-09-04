# M1156 Marginal Atom Policy

## Objective

Approximate the M1155 per-atom oracle with deployable LODO atom-level policies.
The policies use atom rank/proxy score, proxy-score shape, and native boundary
features from M1154.

## Inputs

- M1154 native boundary cache:
  `runs/m1154_native_boundary_risk_policy_v1/native_boundary_features.json`
- M1155 marginal replay:
  `runs/m1155_marginal_atom_admission_v1/marginal_atom_replay.json`
- M1156 policy audit:
  `runs/m1156_marginal_atom_policy_v1/marginal_atom_policy.json`

## Key Results

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | min_ds_MAP | min_ds_NDCG |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| oracle_single | +0.001118 | +0.019178 | +0.005896 | +0.008171 | +0.005705 | +0.000000 | +0.000000 |
| atom_proxy_safe_gain_t0.50 | +0.000093 | +0.006208 | +0.002221 | +0.000891 | -0.002685 | -0.000123 | -0.001793 |
| atom_proxy_safe_gain_t0.70 | +0.000000 | +0.004519 | +0.001724 | +0.000821 | -0.002685 | +0.000000 | +0.000000 |
| atom_proxy_combo_g0.30_s0.70 | +0.000013 | +0.000258 | +0.000477 | +0.000202 | +0.000000 | -0.000132 | -0.001124 |

## Interpretation

M1156 did not produce a deployable policy close to the M1155 oracle.

The best `atom_proxy` policies recover Recall/MAP/NDCG, but they still damage
MRR. Adding native boundary features did not help; it generally made atom-level
selection worse. The safe+gain combo removes some damage only by becoming
mostly no-op.

This is not a negative result for per-atom admission. It is a negative result
for the current feature set and simple logistic policy. The oracle gap is large
and clean, but current deployable features cannot reliably identify
top-rank-destructive atoms.

## Decision

Stop simple atom-level logistic policy tuning. The next useful direction needs
new atom-specific features, not another threshold sweep:

1. Reconstruct M1148/M1151 atom proposal features into the M1155 policy rows.
2. Add marginal native descriptors per atom, such as atom document fanout,
   overlap with current top100, source overlap, and boundary-slot impact.
3. Train for explicit top-rank safety, not only `safe_gain`.
4. Evaluate with row-floor constraints before any broader native replay.
