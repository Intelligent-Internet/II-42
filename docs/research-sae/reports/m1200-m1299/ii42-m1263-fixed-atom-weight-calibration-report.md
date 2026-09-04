# M1263 Fixed-Atom Weight Calibration

## Question

M1262 kept `source_abs_top8` / `reserve0_s1` as the cleanest atom source.
M1263 freezes that atom set and changes only the atom impact transform before
native replay.

This tests score geometry without changing atom admission.

## Runs

Smoke:

- `runs/m1263_fixed_atom_weight_calibration_smoke_v1/`
- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- all calibration variants were tested.

Full `shared15`:

- JSON:
  `runs/m1263_fixed_atom_weight_calibration_v1/m1263_fixed_atom_weight_calibration.json`
- Markdown:
  `runs/m1263_fixed_atom_weight_calibration_v1/m1263_fixed_atom_weight_calibration.md`
- variants:
  `raw`, `uniform_l1`, `clip_median2_l1`, `rank_decay_l1`,
  `rank_only_l1`

## Full Shared15 Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `uniform_l1` | 7.975 | 0 | +0.001734 | +0.003460 | +0.003526 | +0.003029 | +0.000416 | +0.032578 |
| `raw` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |
| `clip_median2_l1` | 7.975 | 0 | +0.001399 | +0.003331 | +0.003625 | +0.003127 | +0.000114 | +0.030605 |
| `rank_decay_l1` | 7.975 | 1 | +0.001482 | +0.003023 | +0.003056 | +0.002452 | -0.000069 | +0.025702 |
| `rank_only_l1` | 7.975 | 1 | +0.000579 | +0.002831 | +0.002557 | +0.002320 | -0.000181 | +0.016440 |

## Interpretation

This is the first clean positive result after M1259-M1262.

Key properties:

- atom set is unchanged;
- no new gate, no threshold, no source replacement;
- `uniform_l1` preserves the total L1 impact budget per query but distributes it
  equally over selected atoms;
- it improves score over raw signed-sum by `+0.001958`;
- it improves Recall@100 and Candidate Upper Bound materially;
- it slightly trades off NDCG/MRR versus raw, but remains macro-positive on all
  metrics.

The negative result from rank-based transforms is also informative:

- rank decay and rank-only weighting reduce CUB and are not safe;
- clipping is almost identical to raw and does not move the frontier;
- the useful change is not "less tail" or "rank confidence";
- it is specifically reducing magnitude inequality among the selected atoms.

## Decision

Promote `uniform_l1` to a stability audit.

Do not call it default yet.  It needs:

1. per-dataset delta vs raw;
2. query-level win/loss counts vs raw;
3. worst-row inspection;
4. comparison against `reserve1` / M1258 movement-constrained candidates.

## Next Step

Run `M1264` stability audit:

- compare `raw_s1` vs `uniform_l1_s1`;
- keep the source atom set fixed;
- report per-dataset and query-level stability;
- if stable, test whether uniform weighting composes with reserve1 or M1258;
- if unstable, keep it as a score-geometry signal for a learned/listwise
  calibrator rather than a direct policy.

## Artifacts

- Script: `scripts/replay_m1263_fixed_atom_weight_calibration.py`
- Smoke JSON:
  `runs/m1263_fixed_atom_weight_calibration_smoke_v1/m1263_fixed_atom_weight_calibration.json`
- Full JSON:
  `runs/m1263_fixed_atom_weight_calibration_v1/m1263_fixed_atom_weight_calibration.json`
