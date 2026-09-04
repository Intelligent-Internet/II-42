# ii42 M345 Explicit-Admission Broad8 Sweep Report

## Goal

M344 completed the broad8 explicit-admission promotion and proved that the
candidate pool is strong but the learned admission/ranking objective still
leaves a large gap to the candidate upper bound.

M345 keeps the exact same 8/8 candidate cache and tests whether the M344
failure mode is mainly training schedule drift. In M344, the best validation
event appeared at epoch 10, while later epochs reduced loss but degraded
NDCG/MAP.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Host: `spark-1`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1`

Datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`
- `trec-covid`
- `cqadupstack`
- `webis-touche2020`

## Variants

### M345A: low-LR dense-eval schedule

Output:

`/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_lr3e4_eval5_seed1050_split1050.json`

Changes from M344:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`

Unchanged:

- `candidate_k=1000`
- `feature_rank_reference_k=160`
- `feature_core_k=160`
- `ranking_policy=score`
- `explicit_admission_weight=0.60`
- `explicit_admission_margin=0.04`
- `explicit_admission_positive_cutoff=20`
- `explicit_admission_hard_negatives_per_family=16`

Status:

- Started on `spark-1` in tmux session `ii42_m345_lr3e4_eval5`.
- Passed dataset read and 8/8 candidate-cache load.
- Entered `train scorer variant=df_le_0p25`.
- First eval at epoch 5: `Recall@100=0.7019`, `MRR@20=0.3836`,
  `NDCG@10=0.3787`, `MAP@100=0.3241`.
- Epoch 10 improved over M344 best-score surface:
  `Recall@100=0.6948`, `MRR@20=0.3858`, `NDCG@10=0.3808`,
  `MAP@100=0.3255`.
- Epoch 20 remained close:
  `Recall@100=0.7002`, `MRR@20=0.3855`, `NDCG@10=0.3794`,
  `MAP@100=0.3250`.
- Completed with early stop at epoch 30 and wrote:
  `/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_lr3e4_eval5_seed1050_split1050.json`.

Final aggregate heldout scorer metrics:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `df_le_0p25_m322_scorer` | 0.683824 | 0.377837 | 0.367564 | 0.298540 |

Delta vs M344 heldout scorer:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.002548 |
| MRR@20 | +0.002164 |
| NDCG@10 | +0.001519 |
| MAP@100 | +0.002232 |

## Stop Rule

If M345A does not improve heldout NDCG@10 or MAP@100 over M344, the next
iteration should adjust objective balance rather than keep extending the same
schedule.

### M345B: lower-admission objective balance

Output:

`/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_lowadm03_boundary035_lr3e4_eval5_seed1050_split1050.json`

Changes from M345A:

- `EXPLICIT_ADMISSION_WEIGHT=0.30`
- `BOUNDARY_WEIGHT=0.35`

Unchanged from M345A:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`

Reason:

M344 loss kept improving while ranking metrics degraded after epoch 10. M345B
tests whether the explicit admission and boundary losses are overpowering the
ranking objective on broad8.

Status:

- Started on `spark-1` in tmux session `ii42_m345_lowadm03_boundary035`.
- Running in parallel with M345A because `spark-2` is occupied and M345A is
  CPU-heavy with safe memory headroom on `spark-1`.
- Completed with early stop at epoch 25 and wrote:
  `/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_lowadm03_boundary035_lr3e4_eval5_seed1050_split1050.json`.

Final aggregate heldout scorer metrics:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `df_le_0p25_m322_scorer` | 0.692155 | 0.375025 | 0.364804 | 0.297159 |

Delta vs M344 heldout scorer:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.005783 |
| MRR@20 | -0.000649 |
| NDCG@10 | -0.001241 |
| MAP@100 | +0.000851 |

### M345C: stronger listwise ranking pressure

Output:

`/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_listwise080_lr3e4_eval5_seed1050_split1050.json`

Changes from M345A:

- `LISTWISE_WEIGHT=0.80`

Unchanged from M345A:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `EXPLICIT_ADMISSION_WEIGHT=0.60`
- `BOUNDARY_WEIGHT=0.50`

Reason:

M345A improves ranking metrics slightly, while M345B mostly trades toward
Recall@100. M345C tests whether the scorer needs stronger direct listwise
ranking pressure rather than weaker admission losses.

Status:

- Started on `spark-1` in tmux session `ii42_m345_listwise080`.
- Passed dataset read and 8/8 candidate-cache load.
- Entered `train scorer variant=df_le_0p25`.
- Epoch 5: `Recall@100=0.6995`, `MRR@20=0.3854`,
  `NDCG@10=0.3798`, `MAP@100=0.3262`.
- Epoch 10: `Recall@100=0.6975`, `MRR@20=0.3851`,
  `NDCG@10=0.3814`, `MAP@100=0.3254`.
- Completed with early stop at epoch 30 and wrote:
  `/home/huoju/leask/runs/ii42-m345-explicit-admission-broad8-sweep-v1/m345_listwise080_lr3e4_eval5_seed1050_split1050.json`.

Final aggregate heldout scorer metrics:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `df_le_0p25_m322_scorer` | 0.683086 | 0.376953 | 0.367599 | 0.298391 |

Delta vs M344 heldout scorer:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.003285 |
| MRR@20 | +0.001280 |
| NDCG@10 | +0.001554 |
| MAP@100 | +0.002083 |

## Summary

Final aggregate heldout scorer comparison:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M344 | 0.686372 | 0.375673 | 0.366046 | 0.296308 |
| M345A low LR | 0.683824 | 0.377837 | 0.367564 | 0.298540 |
| M345B low admission | 0.692155 | 0.375025 | 0.364804 | 0.297159 |
| M345C high listwise | 0.683086 | 0.376953 | 0.367599 | 0.298391 |

M345A is the best MAP/MRR variant. M345C is effectively tied on NDCG and is
slightly better on all-query NDCG/MAP, but it does not produce a material
broad8 breakthrough. M345B confirms that reducing admission and boundary
pressure mostly trades toward Recall@100 while hurting NDCG/MRR.

The useful conclusion is negative but concrete: schedule and simple weight
sweeps can recover a small amount of ranking quality, but the remaining gap to
the candidate upper bound is not a tuning-only problem. The next step should be
a code-level final-rank objective that directly targets top-k false positives
and qrel-positive placement, then reuses the same 8/8 candidate cache.

M344 heldout reference:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `df_le_0p25_m322_scorer` | 0.686372 | 0.375673 | 0.366046 | 0.296308 |
| `df_le_0p25_candidate_upper_bound` | 0.888848 | 1.000000 | 0.930000 | 0.888848 |
