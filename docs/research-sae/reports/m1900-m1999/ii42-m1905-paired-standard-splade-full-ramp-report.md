# M1905 Paired Standard-SPLADE Full-Ramp Report

Date: 2026-07-12

Decision: **standard vocabulary SPLADE is the only post-ramp survivor on the
locked paired surface. Stop treating the local from-scratch SAE basis as the
next optimization parent.**

## Question Answered

M1904 showed that a full FLOPS ramp could lower the SAE branch's sampled
activation-cost proxies only by losing positive ordering. M1905 replaced only
the width-65,536 reconstructed SAE output basis with a standard pretrained
DistilBERT MLM vocabulary head. It kept the same 10,000 teacher rows,
selection and confirmation rows, seed, batch schedule, optimizer, ranking
loss, FLOPS weights, and 6,000-step ramp.

The standard branch survives the completed ramp and continues improving its
quality/cost frontier through step 8,000. The paired decision is
`standard_splade_only_post_ramp_survivor`.

## Locked Selection Trajectory

| Step | Eligible | Pairwise | Positive top1 | KL | Doc nnz | Doc FLOPS | Doc maxDF |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | no | 0.448242 | 0.117188 | 1.696127 | 28.03 | 69.474289 | 1.000000 |
| 500 | no | 0.565430 | 0.226562 | 1.766259 | 15.56 | 44.280011 | 1.000000 |
| 1,000 | no | 0.597656 | 0.179688 | 2.208792 | 9.41 | 13.962637 | 1.000000 |
| 2,000 | yes | 0.871094 | 0.562500 | 0.965890 | 106.15 | 4.798641 | 0.985243 |
| 4,000 | yes | 0.903320 | 0.640625 | 0.672987 | 65.38 | 1.173473 | 0.367188 |
| 6,000 | yes | 0.895508 | 0.656250 | 0.613649 | 42.38 | 0.573568 | 0.206597 |
| 8,000 | **yes, selected** | 0.900391 | 0.656250 | 0.600523 | 37.87 | 0.427381 | 0.123264 |
| 10,000 | yes | 0.895508 | 0.640625 | 0.624310 | 37.84 | 0.429038 | 0.157986 |

The 8,000-step checkpoint is selected by the predeclared cost-first frontier.
Step 10,000 remains eligible, but maxDF, FLOPS, pairwise agreement, positive
top1, and KL all move slightly away from the 8,000-step frontier. This is a
useful training-depth boundary: the early 100-step result was undertrained,
but additional steps are not monotonically beneficial after the mature basis
has converged on this bounded surface.

## Disjoint Confirmation

| Surface | Pairwise | Positive top1 | KL | Doc nnz | Doc FLOPS | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1904 SAE selected step 2,000 | 0.798584 | 0.460938 | 1.017299 | 101.36 | 10.072014 | 0.987630 |
| M1905 standard selected step 8,000 | **0.869629** | **0.562500** | **0.585913** | **37.38** | **0.325047** | **0.153646** |
| OpenSearch sparse-v2 control | 0.924072 | 0.675781 | 1.250686 | 177.41 | 0.617832 | 0.063802 |

Relative to the paired M1904 SAE checkpoint, M1905 changes:

- pairwise agreement: `+0.071045`;
- positive top1: `+0.101562`;
- KL: `-0.431385`;
- document nnz: `0.3688x`;
- document FLOPS: `0.0323x`;
- sampled document maxDF: `0.1556x`.

All six disjoint confirmation checks pass: finite output, non-collapsed query
and document vectors, safe pairwise and positive-top1 ordering, and improved
teacher fit. The result is therefore not selection-only.

The mature OpenSearch checkpoint still has higher pairwise agreement and
positive top1, and its sampled maxDF is lower. M1905's lower KL and lower
sampled FLOPS do not reverse that ranking-quality gap. This repeats an
important result from M1903: candidate-set teacher fit is not a substitute for
native retrieval quality.

## Interpretation

This paired experiment localizes the bounded M1904 failure. Under an identical
teacher, optimizer, and regularization schedule, the pretrained vocabulary
output basis supports both useful ranking and a much healthier activation
shape. The from-scratch SAE basis does not. The local SAE-SPLADE mechanism
therefore remains a research control, not the next model parent.

M1905 is not an official SPLADE reproduction and has no full-corpus native
evaluation. It exposes only 80,000 query presentations (`10,000 x 8`), whereas
the pinned Apache-2.0 Unified LSR framework's mature recipe uses 150,000 steps,
per-device batch 128, hard-negative/distillation data, and a 6,000-step
warmup. Candidate-sample maxDF is also not a native index latency measurement.

The result authorizes a mature-route reset, not promotion of this checkpoint:

1. keep OpenSearch sparse-v2 plus M1660 exact BMP as the executable product
   baseline;
2. use the Apache-2.0 Unified LSR framework as the auditable training and
   architecture-ablation framework;
3. if vocabulary sparse optimization is selected after M1911/M1912, begin
   from a successful checkpoint and first reproduce the published asymmetric
   `qMLP + dMLM` ablation instead of inventing another loss;
4. retain SAE routes only when pretrained-SAE or Latent Terms reproduction
   provides independent complete-corpus evidence.

## Scope Boundary

M1905 answers whether the M1904 post-ramp failure is shared by a standard
vocabulary head on the same bounded teacher surface. It does not establish:

- official MS MARCO or BEIR quality;
- complete-corpus document frequency or native latency;
- commercial usability of Naver's non-commercial SPLADE artifacts;
- superiority over the locally native-verified OpenSearch sparse-v2 model.

Those questions remain assigned to M1911, M1912A, and the consolidated parent
selection report.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1905-paired-standard-splade-full-ramp-contract.md`
- Runner: `scripts/run_m1905_paired_standard_splade_full_ramp_spark.sh`
- Training script: `scripts/train_m1905_paired_standard_splade_full_ramp.py`
- ClearML task: `2048e41d64fb405db86058eda9d83c0b`
- Remote summary: `m1905-paired-standard-splade-full-ramp-v1/summary.json`
- Selected checkpoint: `standard_splade_selected_step8000.pt`
