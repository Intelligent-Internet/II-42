# M1224 CUB-Specific Atom Observability

## Question

M1223 showed that a query-level CUB gate can make M1222 macro-safe, but the
per-dataset breakdown remained row-fragile.  M1224 changes the teacher label
surface itself:

- target atoms: oracle-winning action atoms from actions that do not reduce CUB
- harm atoms: atoms from actions that reduce CUB

This tests whether support risk should be represented in the atom teacher,
rather than patched later by a query-level gate.

## Results

### Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_logistic_top3` | 0.7972 | 0.2526 | 0.0102 | +0.2423 | 0.3735 | 2.751 |
| `model_logistic_top8` | 0.9770 | 0.1850 | 0.0061 | +0.1789 | 0.3855 | 4.602 |
| `rule_source_abs_top8` | 0.9770 | 0.1850 | 0.0061 | +0.1789 | 0.3855 | 4.602 |

### Full Shared15

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `rule_source_abs_top8` | 0.7793 | 0.2138 | 0.0298 | +0.1840 | 0.2928 | 7.975 |
| `rule_agreement_abs_top8` | 0.7793 | 0.2138 | 0.0298 | +0.1840 | 0.2928 | 7.975 |
| `model_logistic_top8` | 0.7568 | 0.2076 | 0.0298 | +0.1778 | 0.2966 | 7.975 |
| `model_logistic_top3` | 0.3205 | 0.2338 | 0.0313 | +0.2025 | 0.2720 | 2.999 |
| `rule_prior_x_source_top8` | 0.6856 | 0.1881 | 0.0298 | +0.1583 | 0.2914 | 7.975 |

## Interpretation

This is a meaningful improvement over M1220-M1223.

M1220 made targets visible but left harm entangled.  M1221 separated generic
target/harm somewhat, but M1222 showed that support risk still leaked through
native replay.  M1223 guarded that risk after the fact, but row-level evidence
was fragile.

M1224 moves support risk into the teacher label surface.  The result is much
cleaner:

- hard-row and full shared15 both show large target-vs-harm gaps
- rule-based inference is as good as or better than the learned model
- the best full variant has target recall `0.7793` with harm precision only
  `0.0298`

This suggests the previous failure was not just model depth or thresholding.
The teacher surface was mixing ranking-positive and support-risky atoms.

## Decision

Promote M1224 to a bounded native replay test.

Replay should start with one rule-based selector, not another broad sweep:

- selector: `rule_source_abs_top8`
- teacher: CUB-specific target/harm
- scale: use conservative `0.5` first, optionally compare `1.0`
- gate: none initially; the point is to test whether CUB-specific atom labels
  reduce the need for the M1223 query gate

If native replay cannot preserve CUB despite this clean observability, stop and
move the CUB-specific signal into the generated-posting training objective
rather than continuing hand-designed replay.

## Artifacts

- Script: `scripts/audit_m1224_cub_specific_atom_observability.py`
- Smoke JSON: `runs/m1224_cub_specific_atom_observability_smoke_v1/m1224_cub_specific_atom_observability.json`
- Full JSON: `runs/m1224_cub_specific_atom_observability_v1/m1224_cub_specific_atom_observability.json`
