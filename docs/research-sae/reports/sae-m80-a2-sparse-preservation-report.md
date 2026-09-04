# SAE M80-A2 Sparse Preservation Report

Date: 2026-05-21

Status: active implementation with one clear positive update. The A2 harness
now supports hard, soft, and straight-through TopK training, explicit top-rank
pairwise loss, and validation-based best-epoch selection. Best-epoch selection
is currently the strongest A2 improvement.

## Summary

M80-A2 starts after the current M80-A1 dense checkpoint:

```text
/home/huoju/leask/runs/m80-a1-snowflake-last2-truncate768-lr5e6-e2-savebest/trainable_dense_student.best.pt
```

The A2 objective is not BM25+SAE product ranking. It is a narrower sparse-tax
question:

```text
Can hard TopK SAE atoms preserve the A1 dense candidate ranking with low loss?
```

The implemented script is:

```text
scripts/research_sae_m80_sparse_preservation.py
```

It does the following:

- loads the M80-A1 dense checkpoint;
- encodes candidate-set documents and queries into dense student vectors;
- trains a `TopKSAE` over the dense vectors;
- adds candidate-set dense-teacher KL so sparse scores preserve dense ranking;
- evaluates dense vs sparse candidate metrics and reports sparse tax.

## Harness Smoke

Run:

```text
m80-a2-sparse-preservation-smoke-l256-k16
latent_dims = 256
active_dims = 16
epochs = 1
train_rows = 32
eval_rows = 32
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5431 | 0.8840 | 0.7869 |
| sparse | 0.5126 | 0.7388 | 0.6936 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0305 |
| MRR | -0.1452 |
| NDCG@10 | -0.0933 |

This validates the path, but it is far below the sparse-tax gate.

## First Medium Run

Run:

```text
m80-a2-sparse-preservation-l2048-k32-e5
latent_dims = 2048
active_dims = 32
epochs = 5
train_rows = 367
eval_rows = 73
candidate_kl_weight = 0.20
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.4988 | 0.7695 | 0.7214 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0856 |
| MRR | -0.1164 |
| NDCG@10 | -0.1143 |

Source-family detail:

| Family | Source | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| BEIR current eval surface | dense | 0.5190 | 0.8638 | 0.7980 |
| BEIR current eval surface | sparse | 0.4140 | 0.7374 | 0.6803 |
| broad generated query surface | dense | 0.6781 | 0.9175 | 0.8899 |
| broad generated query surface | sparse | 0.6204 | 0.8154 | 0.7804 |

## Current Interpretation

The A2 implementation is functioning, but the first capacity is not enough.
The failure mode is not document reconstruction; reconstruction loss is already
small. The issue is sparse ranking preservation: MRR and NDCG drop too much.

Immediate next test:

```text
m80-a2-sparse-preservation-l4096-k64-kl1-e15
latent_dims = 4096
active_dims = 64
candidate_kl_weight = 1.0
```

## Capacity And Scoring Diagnostics

### 4096/k64 normalized sparse score

Run:

```text
m80-a2-sparse-preservation-l4096-k64-kl1-e15
latent_dims = 4096
active_dims = 64
candidate_kl_weight = 1.0
score = normalized sparse dot
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.5530 | 0.7933 | 0.7568 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0314 |
| MRR | -0.0926 |
| NDCG@10 | -0.0789 |

This materially improves Recall@10 tax but still loses too much top-rank
quality.

### 4096/k128 normalized sparse score

Run:

```text
m80-a2-sparse-preservation-l4096-k128-kl1-e15
latent_dims = 4096
active_dims = 128
candidate_kl_weight = 1.0
score = normalized sparse dot
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.5552 | 0.8111 | 0.7646 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0292 |
| MRR | -0.0747 |
| NDCG@10 | -0.0711 |

Increasing active budget helps, but does not close the ranking tax.

### 4096/k128 raw sparse score

Run:

```text
m80-a2-sparse-preservation-l4096-k128-kl1-e15-rawscore
latent_dims = 4096
active_dims = 128
candidate_kl_weight = 1.0
score = raw sparse dot
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.5416 | 0.7241 | 0.7070 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0428 |
| MRR | -0.1617 |
| NDCG@10 | -0.1288 |

Raw sparse dot is worse. Normalized sparse dot remains the better first scoring
surface.

## Current Interpretation After A2 Diagnostics

Capacity and candidate KL help recall, but hard TopK still loses too much
top-rank ordering. If the active 8192/k128 run does not materially reduce
MRR/NDCG tax, the next code change should be soft/differentiable TopK or a
learned sparse score calibration head, not simply more epochs on the same hard
TopK objective.

### 8192/k128 normalized sparse score

Run:

```text
m80-a2-sparse-preservation-l8192-k128-kl1-e15
latent_dims = 8192
active_dims = 128
candidate_kl_weight = 1.0
score = normalized sparse dot
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.5690 | 0.8117 | 0.7778 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0154 |
| MRR | -0.0742 |
| NDCG@10 | -0.0580 |

8192/k128 nearly closes Recall@10, but top-rank quality remains weak on BEIR
current-surface rows.

### 8192/k256 normalized sparse score

Run:

```text
m80-a2-sparse-preservation-l8192-k256-kl1-e15
latent_dims = 8192
active_dims = 256
candidate_kl_weight = 1.0
score = normalized sparse dot
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.5661 | 0.8296 | 0.7981 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0182 |
| MRR | -0.0563 |
| NDCG@10 | -0.0376 |

This is the best A2 result so far, but k256 is expensive and still does not
meet the sparse-tax gate. It is useful evidence, not a promoted sparse model.

## A2 Next Direction

The next A2 implementation should not be another scalar sweep over the same
shared hard-TopK SAE. The useful options are:

- query/document asymmetric sparse encoders that share the same latent
  coordinate space but do not force query and document text through identical
  encoder weights;
- soft or straight-through TopK training, with hard TopK retained for export;
- sparse score calibration tied to candidate KL, if asymmetric encoders still
  leave top-rank tax.

The immediate preferred direction is asymmetric query/document sparse
preservation, because the current failures are strongest on query-side
top-rank ordering rather than Recall@10 coverage.

## Asymmetric Encoder Probe

Run:

```text
m80-a2-asym-l4096-q128-d256-kl1-e15
latent_dims = 4096
active_query_dims = 128
active_doc_dims = 256
candidate_kl_weight = 1.0
```

Result:

| Source | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| dense | 0.5844 | 0.8859 | 0.8357 |
| sparse | 0.4961 | 0.6727 | 0.6511 |

Sparse tax:

| Metric | Sparse - dense |
| --- | ---: |
| Recall@10 | -0.0882 |
| MRR | -0.2131 |
| NDCG@10 | -0.1846 |

Decision:

```text
naive asymmetric encoders are parked
```

This version starts query and document SAE encoders independently. That breaks
latent coordinate alignment, and candidate KL alone is not enough to align the
two spaces on the current M80-A0 V0 surface. If asymmetric sparse preservation
is reopened, it should use shared initialization, tied decoder/atoms, or an
explicit coordinate-alignment loss.

## Revised Next Direction

The best current A2 evidence is shared 8192/k256 hard TopK. It nearly closes
Recall@10 but still loses too much MRR/NDCG and uses a large active budget.

The next implementation should therefore be one of:

- shared encoder plus query-side adapter/gate, initialized from the shared SAE
  rather than independently random query/doc encoders;
- soft or straight-through TopK training with hard TopK export;
- explicit score/rank calibration loss over the top few positions.

Do not continue naive asymmetric training without an alignment mechanism.

## M80-A2.1 Soft/ST TopK And Best-Epoch Selection

Implementation update:

```text
scripts/research_sae_m80_sparse_preservation.py
```

New controls:

- `--train-topk-mode`: `hard`, `soft`, or `straight_through`;
- `--eval-topk-mode`: hard export-style evaluation remains the default;
- `--pairwise-rank-weight`: optional top-rank pairwise loss over dense-teacher
  top positions;
- `--eval-every-epoch`, `--selection-metric`, `--select-best-eval`: hard-eval
  validation and best checkpoint selection.

The important finding is that soft/ST TopK did not fix top-rank preservation.
It can improve candidate coverage, but hard export ranking still regresses.
The larger gain came from selecting the right hard-TopK epoch instead of using
the last epoch.

### A2.1 comparison matrix

All runs use the same A1 dense checkpoint, 4096 text records, 367 train
candidate rows, and 73 eval candidate rows.

| Run | Train TopK | k | Selection | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | --- | ---: | --- | ---: | ---: | ---: |
| `m80-a2-sparse-preservation-l8192-k128-kl1-e15` | hard | 128 | last epoch | -0.0154 | -0.0742 | -0.0580 |
| `m80-a2-sparse-preservation-l8192-k256-kl1-e15` | hard | 256 | last epoch | -0.0182 | -0.0563 | -0.0376 |
| `m80-a2-st-l8192-k128-kl1-pair01-e50` | straight-through | 128 | last epoch | -0.0189 | -0.0994 | -0.0723 |
| `m80-a2-st-l8192-k256-kl1-pair01-e50` | straight-through | 256 | last epoch | -0.0016 | -0.0952 | -0.0566 |
| `m80-a2-soft-l8192-k128-kl1-pair01-e50` | soft | 128 | last epoch | -0.0222 | -0.1250 | -0.0806 |
| `m80-a2-hard-l8192-k256-kl1-e50` | hard | 256 | last epoch | +0.0009 | -0.0752 | -0.0473 |
| `m80-a2-hard-l8192-k256-kl1-e50-select-ndcg` | hard | 256 | best NDCG epoch 33 | +0.0077 | -0.0495 | -0.0266 |
| `m80-a2-hard-l8192-k128-kl1-e50-select-ndcg` | hard | 128 | best NDCG epoch 4 | -0.0150 | -0.0484 | -0.0436 |
| `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-ndcg` | hard + pairwise | 256 | best NDCG epoch 2 | -0.0154 | -0.0278 | -0.0177 |

### Decision

Promote for continued A2 exploration:

```text
m80-a2-hard-l8192-k256-kl1-pair01-e50-select-ndcg
```

This is the best top-rank preservation point so far. It cuts MRR tax from
`-0.0563` to `-0.0278` and NDCG@10 tax from `-0.0376` to `-0.0177` versus the
previous 8192/k256 baseline. Recall@10 tax remains slightly worse at `-0.0154`,
so this is not a final sparse model, but it is the first A2 checkpoint that
gets close to the top-rank target.

Park for now:

```text
soft/ST TopK as the primary A2 fix
```

ST k256 nearly removes Recall@10 tax, but its MRR/NDCG tax is worse than the
hard-TopK baseline. The likely cause is train/eval mismatch: ST optimizes a
soft-gradient path but evaluation exports exact hard atoms.

Next A2 work should focus on a selection objective that balances Recall@10 and
NDCG/MRR, or on calibrated sparse scoring after hard atom export. More epochs
without validation selection are explicitly negative evidence.

## M80-A2.2 Balanced Selection And Score Calibration

Implementation update:

```text
scripts/research_sae_m80_sparse_preservation.py
```

New controls:

- `--selection-metric balanced_gate`: checkpoint selection over Recall@10,
  MRR, and NDCG@10 sparse tax instead of a single raw sparse metric.
- `--sparse-checkpoint`: load an existing sparse checkpoint for eval-only
  calibration tests.
- `--sparse-value-transform`: evaluate hard-export atom values with
  `identity`, `sqrt`, `log1p`, or `binary` transforms.

Balanced gate:

```text
score =
  2.0 * ndcg_tax
  + 1.5 * mrr_tax
  + 1.0 * recall_at_10_tax
  - recall_floor_penalty
```

The recall floor penalty activates below Recall@10 tax `-0.01`. This is only a
checkpoint-selection metric, not a training loss.

### Balanced selection matrix

| Run | k | Pairwise weight | Selected epoch | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-ndcg` | 256 | 0.10 | 2 | -0.0154 | -0.0278 | -0.0177 |
| `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-balanced` | 256 | 0.10 | 9 | +0.0039 | -0.0295 | -0.0202 |
| `m80-a2-hard-l8192-k192-kl1-pair01-e50-select-balanced` | 192 | 0.10 | 3 | -0.0097 | -0.0578 | -0.0342 |
| `m80-a2-hard-l8192-k256-kl1-pair005-e50-select-balanced` | 256 | 0.05 | 6 | +0.0077 | -0.0478 | -0.0223 |

Decision:

```text
balanced k256 pair0.1 is the current best practical A2 checkpoint
```

The NDCG-only checkpoint is still the best top-rank point, but it loses too much
Recall@10 for the intended candidate-recall role. The balanced checkpoint keeps
Recall@10 slightly above dense while keeping MRR/NDCG tax close to the NDCG-only
profile. k192 does not preserve top-rank well enough, and pairwise `0.05`
improves Recall@10 but weakens MRR too much.

### Sparse value transform matrix

All rows evaluate the same balanced k256 pair0.1 checkpoint with hard TopK
export.

| Transform | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| `identity` | +0.0039 | -0.0295 | -0.0202 |
| `sqrt` | +0.0051 | -0.0497 | -0.0302 |
| `log1p` | +0.0048 | -0.0423 | -0.0287 |
| `binary` | +0.0053 | -0.0582 | -0.0346 |

Decision:

```text
value flattening is parked
```

The activation magnitudes carry useful ranking information. Flattening atom
values gives a small Recall@10 bump but consistently damages MRR and NDCG.

### Current A2 State

The current strongest sparse-preservation checkpoint is:

```text
/home/huoju/leask/runs/m80-a2-hard-l8192-k256-kl1-pair01-e50-select-balanced/m80_sparse_sae.best.pt
```

It is still not a final product model because it uses k256, but it is the
first A2 result where candidate recall is at or above dense while top-rank tax
is materially reduced. The next genuine breakthrough should come from either:

- lowering active budget while preserving the balanced k256 quality;
- adding a calibrated sparse scoring head that does not flatten activation
  values;
- moving from this candidate-set sparse-preservation harness to a larger Stage
  A sparse-preservation surface once the dense checkpoint/data scale improves.

## M80-A2.3 Calibrated Scoring And Budget Curve

Implementation update:

```text
scripts/research_sae_m80_sparse_preservation.py
scripts/research_sae_m80_sparse_calibration_grid.py
```

New controls:

- `--score-raw-dot-weight`: add activation-magnitude raw sparse dot as a
  calibrated score term after support selection.
- `--score-overlap-weight`: add normalized query-atom overlap as a calibrated
  score term.
- `--score-doc-norm-weight` and `--score-doc-mass-weight`: test whether document
  activation magnitude helps ranking.
- `--selection-metric budget_balanced_gate`: record the same balanced gate with
  an explicit active-budget penalty. Within a fixed-k training run this does
  not change epoch ordering, but it makes cross-k comparisons explicit.
- `research_sae_m80_sparse_calibration_grid.py`: load the dense checkpoint and
  sparse checkpoint once, encode the eval candidate rows once, then sweep
  scoring heads.

### k-budget matrix

All rows use `latent_dims=8192`, hard TopK export, candidate KL `1.0`, and the
current M80-A1 dense checkpoint.

| Run | k | Selected epoch | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m80-a2-hard-l8192-k128-kl1-e50-select-ndcg` | 128 | 4 | -0.0150 | -0.0484 | -0.0436 |
| `m80-a23-hard-l8192-k160-kl1-pair01-e50-select-budget` | 160 | 8 | -0.0129 | -0.0659 | -0.0444 |
| `m80-a2-hard-l8192-k192-kl1-pair01-e50-select-balanced` | 192 | 3 | -0.0097 | -0.0578 | -0.0342 |
| `m80-a23-hard-l8192-k224-kl1-pair01-e50-select-budget` | 224 | 2 | -0.0154 | -0.0616 | -0.0482 |
| `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-balanced` | 256 | 9 | +0.0039 | -0.0295 | -0.0202 |

Decision:

```text
low-k sparse preservation is not solved by budget-aware selection alone
```

k128/160/192/224 all lose too much top-rank quality. k256 remains the only
usable A2 point on V0. This means active budget pressure must move earlier into
the representation/training objective; it cannot be treated as a post-hoc
selection knob.

### Scoring calibration matrix

All rows evaluate the same balanced k256 checkpoint on the V0 eval rows.

| Config | Balanced score | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: | ---: |
| `raw_dot_weight=0.005` | -0.0779 | +0.0039 | -0.0289 | -0.0193 |
| `raw_dot_weight=0.003` | -0.0785 | +0.0039 | -0.0289 | -0.0196 |
| `baseline` | -0.0808 | +0.0039 | -0.0295 | -0.0202 |
| `overlap_weight=0.05` | -0.0989 | +0.0039 | -0.0357 | -0.0246 |
| `raw_dot_weight=0.010` | -0.1191 | +0.0039 | -0.0426 | -0.0296 |
| `raw_dot_weight=0.050` | -0.2026 | -0.0089 | -0.0638 | -0.0490 |

Decision:

```text
small raw-dot calibration is useful but not a breakthrough
```

Preserving activation magnitude after support selection gives a very small
top-rank improvement. Larger raw-dot weights and overlap-based scoring overfit
the active support and damage ranking. This supports the earlier value-transform
finding: magnitude matters, but the current atom support is the limiting factor.

### Focused expanded Stage-A surface

A focused expanded surface was built on Spark to check whether the V0 result was
too candidate-set specific:

```text
/home/huoju/leask/data/ii42_sae_m80/neutral-stage-a-v1-focused-expanded
```

Manifest:

| Documents | Queries | Candidate rows | Train docs | Validation docs | Holdout docs |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 36,867 | 1,013 | 812 | 30,249 | 3,739 | 2,879 |

The surface keeps the V0 source families but increases per-source documents,
queries, and candidate depth. Missing real commons proxy paths are now skipped
with an explicit manifest note instead of failing the build.

Best V1 scoring rows:

| Config | Balanced score | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: | ---: |
| `overlap_weight=-0.05` | -0.2074 | -0.0161 | -0.0572 | -0.0467 |
| `overlap_weight=0.10` | -0.2155 | -0.0193 | -0.0551 | -0.0475 |
| `baseline` | -0.2190 | -0.0193 | -0.0571 | -0.0477 |

Source-family breakdown for the V1 baseline:

| Family | MRR tax | NDCG@10 tax |
| --- | ---: | ---: |
| `beir15_current_eval_surface` | -0.0175 | -0.0187 |
| `broad_generated_query_surface` | -0.1009 | -0.0799 |

Decision:

```text
current k256 checkpoint is not robust enough on the expanded Stage-A surface
```

The V1 BEIR-family sparse tax remains moderate, but the broad generated query
family collapses. This is evidence that the current A2 checkpoint still depends
too much on the small V0 candidate surface. Stage B should not be used to hide
this problem.

### A2.3 Stage-B gate

Stage B remains blocked.

The condition for entering Stage B was: sparse preservation must be stable
enough that BM25/qrel complementarity can be optimized without using ranking
loss to repair weak atoms. A2.3 did not meet that condition:

- low-k variants do not approach k256 quality;
- calibrated scoring only gives a marginal improvement;
- expanded-surface evaluation shows a clear broad-query generalization gap.

The next A-side work should therefore focus on representation/layout changes:
retrieval-aware atom allocation, active-budget-aware support learning, and a
larger Stage-A training/eval surface. Do not continue scalar scoring or k-budget
sweeps as the primary route.
