# II-42 M613/M614 P1.4 Local-Packing Compiler Report

Date: 2026-07-06

## Objective

M613/M614 test whether a local packing compiler can improve the P1 first-stage
dense-equivalent posting surface after M612 coordinate-gain calibration failed.

This is still a first-stage line:

- no BM25 in training,
- no qrels in training,
- no dataset-specific tuning,
- frozen dense teacher outputs only,
- native benchmark replay only after the dense-only gate passes.

The target is to improve dense candidate membership or dense ranking geometry
without materially regressing active recall, selector overlap, selector
coverage, or support preservation.

## Implementation

M613 added `shared_local_soft_packing` and `dual_local_soft_packing` variants to
`scripts/research_sae_m551_dream_lite_posting.py`.

The local packing layer learns a per-row coordinate rank and uses:

- soft local packing during training,
- hard active-k packing during evaluation/publishing,
- the existing sparse posting evaluation and selector gate.

M614 added a dense-only top-k recall surrogate:

- `--topk-recall-weight`
- `--topk-recall-k`
- `--topk-recall-scale`
- `--selection-topk-recall-weight`

The surrogate tries to protect dense teacher top-k membership by pushing teacher
top-k candidates above dense-tail candidates.  It is qrels-free and defaults to
disabled, preserving previous behavior.

## Bounded Canary Setup

All meaningful canaries below use:

- Dataset: `nfcorpus`
- Active dims: `512`
- Teacher pool: `1000`
- Max train groups: `256`
- Epochs: `2`
- Selector split: `20%`
- Selector gates: overlap@20, overlap@100, overlap@200, candidate coverage,
  and teacher-mass utility

Artifacts:

- M613A:
  `runs/m613_p1p4_nfcorpus_canary_v1/m613a_nfcorpus_local_packing_seed6131_remote_artifacts.tgz`
- M613B:
  `runs/m613_p1p4_nfcorpus_canary_v2/m613b_nfcorpus_local_packing_active998_seed6131_remote_artifacts.tgz`
- M613C:
  `runs/m613_p1p4_nfcorpus_canary_v3/m613c_nfcorpus_local_packing_rowabs_seed6131_remote_artifacts.tgz`
- M614A:
  `runs/m614_p1p3_nfcorpus_canary_v1/m614a_nfcorpus_tail_preserve_seed6141_remote_artifacts.tgz`
- M614B:
  `runs/m614_p1p3_nfcorpus_canary_v2/m614b_nfcorpus_tail_preserve_seed6131_remote_artifacts.tgz`
- M614C:
  `runs/m614_p1p3_nfcorpus_canary_v3/m614c_nfcorpus_tail025_seed6131_remote_artifacts.tgz`
- M614D:
  `runs/m614_p1p3_nfcorpus_canary_v4/m614d_nfcorpus_smallstep_seed6131_remote_artifacts.tgz`

## Same-Seed Results

Same-seed rows use `seed=6131`, so selector splits are comparable.

| Run | Change | Selected epoch | dO@100 | selector dO@20 | selector dO@100 | selector dO@200 | Coverage delta | Utility delta | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M613A | local packing, active floor 1.0 | 0 | -0.00850 | +0.00500 | +0.00150 | +0.00000 | -0.00100 | +0.000068 | reject: coverage |
| M613B | local packing, active floor 0.998 | 1 | -0.00900 | +0.00750 | +0.00300 | -0.00075 | -0.00100 | +0.000072 | reject: top200, coverage |
| M613C | `row_abs` support, active floor 0.998 | 0 | +0.00000 | +0.00000 | +0.00000 | +0.00000 | +0.00000 | +0.000000 | pass: no-op |
| M614B | top-k recall 0.75, K=200 | 0 | -0.00850 | +0.00500 | +0.00150 | +0.00000 | -0.00100 | +0.000068 | reject: coverage |
| M614C | top-k recall 0.25, K=200 | 0 | -0.00850 | +0.00500 | +0.00150 | +0.00000 | -0.00100 | +0.000068 | reject: coverage |
| M614D | smaller LR, no top-k recall | 0 | -0.00850 | +0.00500 | +0.00150 | +0.00000 | -0.00100 | +0.000068 | reject: coverage |

M614A used `seed=6141`, so it is not directly comparable to the M613 same-seed
rows.  It still provided useful shape evidence:

| Run | Selected epoch | dO@100 | selector dO@20 | selector dO@100 | selector dO@200 | Coverage delta | Utility delta | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M614A | 1 | +0.00200 | -0.00250 | -0.00900 | +0.00225 | +0.00050 | -0.000214 | reject: overlap, utility |

## Interpretation

M613 local packing exposed a real but unsafe signal:

- It can improve selector overlap@20 and selector overlap@100 on `nfcorpus`.
- It can improve MRR@20 strongly on the qrels-facing heldout slice.
- It does this while reducing dense overlap@100 and/or selector coverage.

That violates the first-stage dense-equivalence gate.  The first-stage goal is
not to find a small qrels-facing perturbation; it is to preserve dense-derived
posting behavior better than P1-a0125.

M614 top-k recall surrogate did not rescue the family:

- With same seed, top-k recall weights `0.75` and `0.25` both selected epoch0.
- The surrogate prevented some top200 regression but did not produce a trained
  improvement.
- A smaller learning rate also selected epoch0 and retained the same coverage
  regression.
- On a different selector split, M614A improved top200/coverage but sacrificed
  top20/top100 overlap and utility.

The family is therefore trading head overlap, tail overlap, and coverage rather
than improving the underlying dense-equivalent score surface.

## Decision

Do not scale M613/M614 local-packing variants to shared3/shared15.

Do not publish these checkpoints to native DB/plugin evaluation.

Do not restart M605 from these artifacts.

The valid conclusion is negative: local packing and topK-recall surrogate are
not currently solving the P1 first-stage bottleneck.

## Next Direction

Stop same-family micro-tuning.

The next useful step should return to score-interface diagnostics rather than
another local-packing weight search:

1. Revisit M606/M608 native score interface diagnostics and compare raw dense,
   dense-topk sparse, and P1 native scoring normalization.
2. Audit whether the remaining loss is caused by candidate generation, score
   normalization after hard active-k packing, or native atom scoring semantics.
3. If the issue is native scoring normalization, implement a score-interface
   correction that preserves active support instead of learning a new support
   selector.
4. If the issue remains under-ranked positives after candidate presence is
   proven, only then reopen a separately scoped second-stage scorer.

M613/M614 should remain as evidence that local-packing support changes are
currently too unstable for the first-stage dense-equivalent requirement.
