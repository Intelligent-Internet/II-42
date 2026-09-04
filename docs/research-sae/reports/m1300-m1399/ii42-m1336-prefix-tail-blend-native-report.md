# M1336 Prefix Tail Blend Native

## Question

M1335 showed that `candidate_tail` has useful target coverage and target base
rate, while M1333 showed that selecting a tiny subset of that tail is not
separable. M1336 tests a different construction:

> Keep `rank_prefix` as a high-precision anchor, then blend the whole
> candidate/fill tail at a low continuous weight.

This is a hard-row native smoke. It is not a full `shared15` gate.

## Smoke Result

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1336_candidate_tail_s0p5` | 4.683 | 1 | 4 | +0.001847 | +0.000109 | -0.000083 | +0.000807 | +0.000000 | +0.010185 |
| `m1336_candidate_x_fill_s0p5` | 3.414 | 1 | 4 | +0.001847 | +0.000739 | +0.000395 | +0.001364 | -0.000803 | -0.005911 |
| `m1336_candidate_x_fill_s0p25` | 3.414 | 1 | 4 | +0.001847 | +0.000569 | +0.000480 | +0.000695 | -0.000788 | -0.007193 |
| `rank_prefix` | 1.309 | 1 | 1 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | -0.001606 | -0.026600 |
| `m1336_candidate_tail_s0p1` | 4.683 | 1 | 4 | +0.001847 | +0.000426 | +0.000337 | +0.000494 | -0.001606 | -0.029591 |

The best variant is `candidate_tail_s0p5`. It is the first variant in this
tail-blend branch to make the hard-row macro score positive while preserving
macro CUB.

However, it is not row-clean.

Per-dataset shape for `m1336_candidate_tail_s0p5`:

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 3 | +0.000922 | -0.001605 | -0.001070 | -0.000471 | +0.000000 |
| `scidocs` | 0 | +0.004000 | +0.001748 | +0.000278 | +0.002482 | +0.000000 |
| `webis-touche2020` | 1 | -0.000658 | +0.000262 | +0.001196 | +0.000000 | +0.000000 |

## Interpretation

M1336 is not deployable, but it is not a dead result.

It preserves the M1335 insight: candidate tail has useful recall-bearing
signal, and continuous low-weight blending can recover macro CUB while moving
Recall/MRR. The failure is row-level ranking harm, concentrated especially in
`cqadupstack`, plus a small Recall loss in `webis-touche2020`.

This is more promising than:

- M1332 global atom prior;
- M1333 candidate-tail selector;
- M1334 support-generated tail.

But it still fails the hard-row row-safety gate.

## Decision

Do not run full `shared15` for M1336 as-is.

Keep `candidate_tail_s0p5` as a structural signal:

- whole-tail low-weight blending is viable enough to study;
- atom-level tail selection remains weak;
- row-level safety needs to be built into the blend objective, not added as a
  post-hoc selector.

## Next Valid Work

M1337 should not be another scale sweep. It should analyze why
`candidate_tail_s0p5` flips the harm pattern:

- `rank_prefix` loses CUB on `scidocs`;
- `candidate_tail_s0p5` fixes CUB but hurts `cqadupstack` ranking and
  `webis-touche2020` Recall.

The next useful experiment is a row-anatomy audit over moved documents and
tail atoms for these three datasets. If the harm comes from a small identifiable
tail substructure, then a constrained blend may be worth testing. If harm is
diffuse, stop this local branch.

## Artifacts

- Script:
  `scripts/replay_m1336_prefix_tail_blend_native.py`
- Smoke JSON:
  `runs/m1336_prefix_tail_blend_native_smoke_v1/m1336_prefix_tail_blend_native.json`
- Smoke Markdown:
  `runs/m1336_prefix_tail_blend_native_smoke_v1/m1336_prefix_tail_blend_native.md`
