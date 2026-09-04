# M1247 Action-Atom Prior Native Replay

## Goal

M1246 showed that consensus/action-prior atoms are clean evidence but not a
better candidate-set source.  M1247 tests the only remaining small hypothesis:
can that clean evidence transfer as native query impact calibration?

This is a bounded native smoke, not a threshold sweep.

## Inputs

- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Baseline: native P1/BM25 path from the current shared15 setup
- Selectors:
  - `source_abs_top8`
  - `consensus_delta_top4`
  - `consensus_delta_top8`
  - `action_clean_delta_top4`
- Scales: `0.5`, `1.0`
- Output:
  - `runs/m1247_action_atom_prior_native_smoke_v1/m1247_action_atom_prior_native.json`
  - `runs/m1247_action_atom_prior_native_smoke_v1/m1247_action_atom_prior_native.md`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1247_source_abs_top8_s1` | 4.574 | 0 | +0.000821 | +0.000031 | +0.000271 | +0.000379 | +0.000015 | +0.005513 |
| `m1247_consensus_delta_top4_s1` | 1.751 | 1 | +0.001120 | +0.000360 | +0.000044 | +0.000766 | -0.000788 | -0.012184 |
| `m1247_consensus_delta_top8_s1` | 1.839 | 2 | +0.001120 | +0.000247 | -0.000064 | +0.000566 | -0.000788 | -0.013781 |
| `m1247_action_clean_delta_top4_s1` | 1.803 | 2 | +0.000842 | +0.000009 | -0.000248 | +0.000778 | -0.000788 | -0.017674 |

The clean consensus/action-prior variants produce small Recall/MAP/MRR gains,
but consistently reduce candidate upper bound.  Under the current gate, that
is not a safe native transfer.

The only safe positive variant is the older `source_abs_top8_s1` shape.  It is
positive on this smoke surface, but the gain is very small and does not come
from the new M1246 clean-evidence hypothesis.

## Interpretation

M1247 closes this branch:

- M1244 action-source oracle remains a real upper bound.
- M1245 says the action source is not predictable from current query features.
- M1246 says clean consensus/action-prior atoms are already mostly inside the
  all-actions set and cannot become a better candidate source.
- M1247 says clean evidence does not transfer safely as native impact
  calibration.

The retained baseline is `source_abs_top8_s1`, which is effectively the M1225
CUB-specific replay shape.  The new action-atom prior branch should not be
expanded.

## Decision

Stop the M1246/M1247 action-atom prior branch.

Do not run full shared15 for `consensus_delta` or `action_clean_delta`, and do
not train a larger model on this feature family.  The next useful change must
alter the teacher/candidate source, not the selector over this source.

The next source should be required to pass this stricter precondition before
replay:

1. It must contribute target atoms outside the current `all_actions_top8` set,
   or improve native metrics without CUB loss.
2. It must show target/harm separation before training.
3. It must be validated on smoke before full shared15.
