# M1670 Score-Spectrum Sparse Distillation Report

Date: 2026-07-11

Decision: **retain the heterogeneous full-set teacher evidence; close
three-candidate score-spectrum distillation and do not scale it.**

## S0 Observability

The frozen 2,048-row audit used one positive and all eight MS MARCO negatives.
The held-out 512-row result was:

| Source | Three-teacher coverage | Entropy | Slice top1 | Slice pair |
| --- | ---: | ---: | ---: | ---: |
| Legacy first3 | 0.572613 | 0.883433 | 0.890625 | 0.950521 |
| Cross top3 | 0.438188 | 0.794591 | 0.796875 | 0.895833 |
| Cross stratified3 | 0.883601 | 1.005872 | 0.808594 | 0.923828 |

Stratified sampling increased teacher score-range coverage by `+0.310987`
over M1650's legacy slice. The lower slice accuracy is expected: the source
contains both the hardest and easiest ends instead of three mostly easy rows.

On the common full-nine surface, adding the stored cross teacher to the
normalized dense+sparse teacher was also positive:

| Teacher | Top1 | Pair | Sparse-error recovery | Dense-new errors |
| --- | ---: | ---: | ---: | ---: |
| Sparse root | 0.750000 | 0.932617 | 0.0000 | 40 |
| Dense+sparse | 0.755859 | 0.938232 | 0.1563 | 20 |
| Dense+sparse+cross | 0.796875 | 0.950684 | 0.2813 | 12 |

S0 therefore correctly authorized a causal paired training test. ClearML task:
`5d6ace5d26364e9089637601cc7d0ab6`.

## S1 Paired Training

Both branches started from the identical public OpenSearch sparse root and
used the same 1,536 training rows, 512 held-out rows, normalized three-teacher
target, seed, optimizer, 500 steps, and M1651 root-relative mass/FLOPS
constraints. Only the training candidate source differed. Both were selected
on full-nine heldout candidates.

| Source | Step | Full9 KL | KL gain | Pair | Anchor rho | Max cost ratio | Pass |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Legacy | 0 | 0.030034 | 0.00% | 0.932617 | 0.998649 | 1.0000 | no |
| Legacy | 100 | 0.030083 | -0.16% | 0.932861 | 0.994197 | 1.0000 | no |
| Legacy | 250 | 0.029873 | 0.53% | 0.932861 | 0.990811 | 1.0200 | no |
| Legacy | 500 | 0.030024 | 0.03% | 0.932617 | 0.989729 | 1.0000 | no |
| Stratified | 0 | 0.030034 | 0.00% | 0.932617 | 0.998649 | 1.0000 | no |
| Stratified | 100 | 0.030206 | -0.57% | 0.932129 | 0.995565 | 1.0012 | no |
| Stratified | 250 | 0.030208 | -0.58% | 0.932861 | 0.994328 | 1.0000 | no |
| Stratified | 500 | 0.030056 | -0.07% | 0.932617 | 0.991348 | 1.0137 | no |

Legacy ClearML: `b10bb7faaea14ce1be489c04c24edb71`.
Stratified ClearML: `c0a6e563229f4008a3df9ecce1bfa168`.

## Interpretation

The experiment separates two claims that must not be merged:

1. A normalized dense+sparse+cross teacher is more informative than the old
   dense+sparse teacher on the full candidate set.
2. Selecting three score-spectrum anchors is not sufficient to transfer that
   full distribution through local KL training.

Neither branch was blocked by cost, max DF, anchor drift, or non-finite
optimization. Legacy's best full-set KL gain was only `0.53%`; stratified did
not produce a positive checkpoint. More steps cannot be justified from this
trajectory because the trained objective improves on sampled batches without
moving the held-out full-nine surface.

This closes M1670 S2/S3. Do not try five/eight-candidate grids, quantile grids,
teacher-weight grids, or a longer continuation. Those would change capacity
without addressing the demonstrated mismatch between subset loss and complete
teacher geometry.

## Route Consequence

M1660 remains the retained engineering breakthrough: mature vocabulary sparse
outputs work in one exact inverted index. The next model experiment must change
the representation foundation, not candidate sampling. The evidence-backed
fallback is full Vocabulary Transfer with semantic embedding initialization
and Activation Potential Calibration, followed by established SPLADE
distillation only after its activation/geometry gate passes.

## Artifacts

- Contract: `docs/research-sae/reports/m1600-m1699/ii42-m1670-score-spectrum-sparse-distillation-contract.md`
- S0: `scripts/audit_m1670_score_spectrum_teacher.py`
- S1: `scripts/train_m1670_score_spectrum_branch.py`
- Remote root: `/home/huoju/leask/runs/ii42-m1670-score-spectrum-v1`

No M1670 process remains on spark-1. Neither branch emitted a selected
checkpoint.
