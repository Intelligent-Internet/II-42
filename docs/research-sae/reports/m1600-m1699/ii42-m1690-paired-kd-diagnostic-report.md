# M1690 Paired KD Diagnostic Report

## Decision

**Stop the constant-LR, batch-two implementation before running its ensemble
branch. Preserve teacher observability and move to the single M1691
paper-schedule repair.**

The stored-score branch completed its locked 2,000 steps and failed every
trained checkpoint gate. A post-run audit found that the implementation did
not follow the relevant pinned optimizer recipe, so this result is an
optimizer diagnostic rather than evidence against the ensemble teacher.

## Full-Candidate Result

| Step | KL | KL gain | Top1 | Pairwise | Spearman | Pass |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 0 | 1.656219 | 0.00% | 0.522461 | 0.785161 | 0.738735 | no |
| 500 | 1.646966 | +0.56% | 0.539062 | 0.771557 | 0.710466 | no |
| 1,000 | 1.941086 | -17.20% | 0.524414 | 0.750762 | 0.664939 | no |
| 2,000 | 2.273130 | -37.25% | 0.503906 | 0.734033 | 0.626699 | no |

Step 500 shows the recurring failure shape precisely: the sampled objective
can improve top1 and slightly reduce KL while damaging full-100 pairwise and
Spearman geometry. Deeper training does not recover it.

Sparse cost is also unsafe. At step 500, document FLOPS is 2.10x and document
max DF is 4.48x root. At step 2,000, document max DF is 10.72x and query max DF
is 12.48x root. No checkpoint is a model or engine candidate.

## Root Cause Audit

The run used:

- batch 2;
- constant LR 2e-5 from step one;
- gradient clipping at 1.0;
- absolute L0/FLOPS ramp 0.08/T=40,000.

The pinned OpenSearch precomputed-KD `config_l0.yaml` instead uses:

- batch 20;
- AdamW LR 2e-5 with a linear 100,000-step schedule;
- 6,000 warmup steps;
- no gradient clipping;
- the same absolute L0/FLOPS ramp.

The M1690 implementation combined the author's slow sparsity clock with an
immediate full optimizer LR and a 10x smaller batch. The measured early max-DF
explosion is consistent with that mismatch. Running the ensemble under the same
invalid optimizer would not answer whether the better teacher is absorbable.

[Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) also uses a non-tiny
KD batch (16), K=8 stratified candidates, one epoch, and a prior contrastive
adaptation stage. This independently argues against interpreting batch-two
constant-LR drift as a route-level sparse-KD failure.

## Artifacts

- ClearML: `25f8af6191df41c7b77e148e522e9568`.
- Run: `m1690-s2a-stored-cross-kd-v1` on `spark-2`.
- Step-zero score array SHA-256:
  `d19b77df5083dbf1261f8ec0d823de7aa65ff824050448e06b218f89aa5e3002`.
- Selected checkpoint: none.

## Authorized Repair

M1691 changes only batch, LR schedule, warmup, and clipping to the pinned
OpenSearch recipe. It keeps root/model bytes, fixed K=8 data, seed, teacher
targets, full validation, FLOPS formula, and joint gates unchanged.

No second optimizer correction is authorized. If the M1691 ensemble cannot
pass, pure KD stops and the only remaining model route is a separately
contracted broad contrastive curriculum followed by KD.
