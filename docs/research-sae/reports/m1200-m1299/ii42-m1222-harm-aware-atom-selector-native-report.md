# M1222 Harm-Aware Atom Selector Native Replay

## Question

M1221 showed that harm-aware selection can separate target atoms from harmful
atoms on full shared15.  M1222 tests whether that selector transfers to native
retrieval metrics.

Only one selector is replayed:

`target_score - harm_score > 0`, top8 atoms.

Two scales are used as a sanity check: `0.5` and `1.0`.

## Results

Hard-row smoke: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1222_harm_aware_top8_s0.5` | 0.683 | -0.000076 | -0.000118 | -0.000415 | -0.000037 | +0.000266 | 4 |
| `m1222_harm_aware_top8_s1` | 0.683 | -0.000205 | -0.000037 | -0.000371 | -0.000014 | +0.000266 | 4 |

Full shared15:

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1222_harm_aware_top8_s0.5` | 1.841 | +0.000177 | +0.000121 | +0.000113 | +0.000114 | -0.000305 | 1 |
| `m1222_harm_aware_top8_s1` | 1.841 | -0.000023 | +0.000478 | +0.000364 | +0.000276 | -0.000285 | 2 |

## Interpretation

M1222 has a mixed but useful outcome.

The positive part:

- Full shared15 ranking metrics improve.
- Scale `0.5` also improves Recall@100.
- The M1221 selector is not random noise; it can move native ranking in the
  right direction.

The blocker:

- Candidate upper bound drops on full shared15.
- Hard rows remain unsafe.
- Scale does not solve the problem: both scales preserve the same CUB/support
  risk pattern.

This means the route is not ready for expansion.  The selector can improve
rank geometry, but it does not preserve candidate support.

## Decision

Do not promote M1222.

Do not continue with lambda/scale micro-tuning.

The next useful step must add a candidate-upper-bound/support guard into the
selector objective.  The next experiment should ask whether the system can keep
the M1222 ranking gains while preventing CUB/support loss.

## Artifacts

- Script: `scripts/replay_m1222_harm_aware_atom_selector_native.py`
- Smoke JSON: `runs/m1222_harm_aware_atom_selector_native_smoke_v1/m1222_harm_aware_atom_selector_native.json`
- Full JSON: `runs/m1222_harm_aware_atom_selector_native_v1/m1222_harm_aware_atom_selector_native.json`
