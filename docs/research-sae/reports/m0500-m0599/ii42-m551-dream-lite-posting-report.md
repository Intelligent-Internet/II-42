# M551 DREAM-lite Posting Encoder

M551 tests a BM25-free first-stage training signal inspired by DREAM-style
candidate-set competition.  The goal is not to optimize dataset labels or BM25
fusion.  The goal is to learn a small output transform over frozen dense rows so
the emitted sparse posting scores better preserve dense-teacher candidate
distributions.

## Design

- Teacher: frozen dense-derived scores over per-query candidate sets.
- Training signal: candidate-set KL from teacher softmax to posting-score
  softmax.
- Model: identity-initialized residual output layer over dense rows.
- Evaluation: held-out qrels only after training, with BM25 disabled.
- Safety gate: qrels-free validation KL can select a checkpoint only if support
  loss and active top-k membership stay within configured limits.

The first normal residual attempt lowered KL but immediately changed top128
membership.  On the three-task limited canary, epoch-1 validation active recall
already fell to about `0.94-0.97`, which explained the unstable retrieval
surface.  The current default is therefore `shared_locked_support_residual`,
which only changes values inside each row's original dense top128 support.

## Limited Three-Task Canary

Run:

- Host: `spark-1`
- Path:
  `/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/canary3_limited_locked_seed551`
- Tasks: `FiQA2018`, `SCIDOCS`, `TRECCOVID`
- Limits: `DOC_LIMIT=10000`, `QUERY_LIMIT=500`, `MAX_TRAIN_GROUPS=128`
- BM25: disabled

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.45657 | 0.36168 | 0.71434 | 0.58344 | 1.00000 |
| `dense_topk128_sparse` | 0.40798 | 0.30757 | 0.66497 | 0.50918 | 0.59417 |
| `m551_shared_locked_support_residual_dream_lite` | 0.41536 | 0.30617 | 0.66264 | 0.52648 | 0.59196 |

Per-task NDCG@10:

| Task | Dense Top128 Sparse | M551 Locked Support | Delta |
| --- | ---: | ---: | ---: |
| `FiQA2018` | 0.53084 | 0.52535 | -0.00549 |
| `SCIDOCS` | 0.19693 | 0.20350 | +0.00657 |
| `TRECCOVID` | 0.49616 | 0.51722 | +0.02106 |

Training diagnostics:

| Task | Selected Epoch | Validation Active Recall | Validation Support Loss |
| --- | ---: | --- | --- |
| `FiQA2018` | 4 | `[1.0, 1.0, 1.0, 1.0]` | `[0.00063, 0.001494, 0.003156, 0.005659]` |
| `SCIDOCS` | 4 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000397, 0.001173, 0.002921, 0.005819]` |
| `TRECCOVID` | 4 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000264, 0.000963, 0.002011, 0.003538]` |

## Interpretation

The locked-support route is the first M551 variant that changes the teacher
distribution while provably preserving active top-k membership.  It improves
limited-canary macro NDCG@10 by `+0.00738` over dense top128 sparse and improves
MRR@20 by `+0.01730`, but Recall@100 drops by `-0.00233` and MAP@100 drops by
`-0.00140`.

This is not a promotable final surface yet.  It is a useful positive signal:
candidate-set KL is not sufficient by itself, but it becomes viable when the
retrieval interface is locked to the dense-derived support.

## Current Broad Run

A full broad10 validation run completed on `spark-1`.

The first broad10 launch accidentally ran CPU-only because the runner inherited
the M550 Docker command and omitted `--gpus all`.  The runner is now fixed with
`--gpus all --ipc=host --ulimit memlock=-1 --ulimit stack=67108864`, and a
minimal container test confirmed `torch.cuda.is_available() == True` on
`NVIDIA GB10`.

Output:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_gpu_seed551/m551_locked_broad10_gpu_seed551.json
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_gpu_seed551/m551_locked_broad10_gpu_seed551.md
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.57408 | 0.43280 | 0.74797 | 0.65243 | 1.00000 |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51003 | 0.36492 | 0.65981 | 0.59407 | 0.53044 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00372`
- MAP@100: `+0.00388`
- Recall@100: `+0.00165`
- MRR@20: `-0.00145`
- Dense overlap@100: `+0.00276`

This is a real broad10 positive signal, but not yet a promotion-quality result
because MRR@20 is slightly lower and several individual tasks still regress.
The next sweep stays BM25-free and targets temperature/candidate-set/gate
stability.

Promotion rule for the next step: continue this line only if a sweep preserves
active membership, improves NDCG/MRR over `dense_topk128_sparse`, and does not
materially lose Recall@100.  If later sweeps repeat the pattern "NDCG/MAP up
but MRR or task stability down", add a qrels-free top-rank guard before trying
BM25 or qrels training.

## Sharp Temperature Broad10

The sharp temperature run completed on `spark-1`.

Output:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_t025_s050_seed551/m551_locked_broad10_sharp_t025_s050_seed551.json
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_t025_s050_seed551/m551_locked_broad10_sharp_t025_s050_seed551.md
```

Config delta versus the first broad10 run:

- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- Other core settings unchanged.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51360 | 0.36967 | 0.66362 | 0.60242 | 0.53218 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00729`
- MAP@100: `+0.00863`
- Recall@100: `+0.00546`
- MRR@20: `+0.00690`
- Dense overlap@100: `+0.00450`

Delta versus the previous M551 locked broad10 run:

- NDCG@10: `+0.00357`
- MAP@100: `+0.00475`
- Recall@100: `+0.00381`
- MRR@20: `+0.00835`
- Dense overlap@100: `+0.00174`

Per-task deltas versus `dense_topk128_sparse`:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Selected Epoch |
| --- | ---: | ---: | ---: | ---: | ---: |
| `ArguAna` | +0.00538 | +0.00364 | +0.00000 | +0.00375 | 4 |
| `CQADupstackGamingRetrieval` | +0.00208 | +0.00102 | +0.00729 | -0.00146 | 1 |
| `CQADupstackUnixRetrieval` | +0.00050 | -0.00081 | -0.00296 | -0.00211 | 1 |
| `ClimateFEVERHardNegatives` | +0.03349 | +0.02994 | +0.00980 | +0.04841 | 1 |
| `FEVERHardNegatives` | +0.04773 | +0.05091 | +0.04135 | +0.05027 | 2 |
| `FiQA2018` | -0.00525 | +0.00045 | -0.00500 | -0.00478 | 2 |
| `HotpotQAHardNegatives` | -0.00036 | -0.00120 | -0.00500 | +0.00107 | 0 |
| `SCIDOCS` | -0.00380 | -0.00030 | +0.00017 | -0.00123 | 1 |
| `TRECCOVID` | +0.00464 | -0.00125 | -0.00094 | -0.05000 | 4 |
| `Touche2020Retrieval.v3` | -0.01141 | +0.00398 | +0.00986 | +0.02500 | 4 |

Interpretation: this is the first M551 broad10 run that improves all four
macro metrics over dense top128 sparse, and it is therefore a stronger positive
signal than the initial locked-support result.  It still has task-level
instability: `TRECCOVID` loses MRR@20, and `FiQA2018` / `SCIDOCS` lose NDCG@10.
The line is worth continuing, but the next check should constrain ranking-head
drift without using qrels.

## Sharp Seed552 Broad10

The seed robustness run completed on `spark-2`.

Output:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_seed552/m551_locked_broad10_sharp_seed552.json
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_seed552/m551_locked_broad10_sharp_seed552.md
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.57274 | 0.43327 | 0.74907 | 0.65333 | 1.00000 |
| `dense_topk128_sparse` | 0.49713 | 0.35647 | 0.65734 | 0.58886 | 0.52737 |
| `m551_shared_locked_support_residual_dream_lite` | 0.49974 | 0.36021 | 0.65793 | 0.59148 | 0.52936 |

Delta versus same-run `dense_topk128_sparse`:

- NDCG@10: `+0.00261`
- MAP@100: `+0.00374`
- Recall@100: `+0.00059`
- MRR@20: `+0.00262`
- Dense overlap@100: `+0.00199`

Interpretation: the sharp temperature shape survives a different seed/split,
but with smaller margin.  This supports continuing the line, while confirming
that it is not yet a strong promotion-grade replacement for the dense-topk128
baseline.

## Soft Temperature Broad10

The soft temperature run completed on `spark-2`.

Output:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_soft_t050_s100_seed551/m551_locked_broad10_soft_t050_s100_seed551.json
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_soft_t050_s100_seed551/m551_locked_broad10_soft_t050_s100_seed551.md
```

Config delta versus the first broad10 run:

- `TEACHER_TEMPERATURE=0.050`
- `STUDENT_TEMPERATURE=0.100`
- Other core settings unchanged.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.50939 | 0.36484 | 0.65994 | 0.59158 | 0.53021 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00308`
- MAP@100: `+0.00380`
- Recall@100: `+0.00178`
- MRR@20: `-0.00394`
- Dense overlap@100: `+0.00253`

Interpretation: the softer teacher/student distribution is weaker than the
sharp temperature run.  It repeats the earlier pattern of small NDCG/MAP/Recall
gains with MRR loss, so this branch should not receive more scale unless a
later guard shows a specific reason to revisit it.

## Active Follow-Ups

The seed robustness run has completed and is summarized above.

The stricter geometry preservation run completed on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_support003_seed551
```

Config delta:

- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- `MAX_SUPPORT_LOSS=0.003`

Strict support result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_support003_seed551/m551_locked_broad10_sharp_support003_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.50751 | 0.36324 | 0.65995 | 0.59403 | 0.53027 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00120`
- MAP@100: `+0.00220`
- Recall@100: `+0.00179`
- MRR@20: `-0.00149`
- Dense overlap@100: `+0.00259`

Interpretation: tightening `MAX_SUPPORT_LOSS` to `0.003` is too conservative.
It preserves some positive movement but loses most of the sharp-run gain and
reintroduces MRR loss.  Do not continue this branch as-is.

A qrels-free relative teacher top-rank guard run completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_guard_rel_seed551
```

The guard run keeps the sharp temperatures and adds validation diagnostics for
candidate-set teacher top-10 recall and top-1 agreement.  Checkpoint selection
now rejects epochs that move too far below the task's own epoch0 identity
baseline:

- `RANK_GUARD_K=10`
- `MAX_TEACHER_TOPK_RECALL_DROP=0.01`
- `MAX_TEACHER_TOP1_AGREEMENT_DROP=0.05`

This remains BM25-free and qrels-free during training and checkpoint selection.

Guard result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_guard_rel_seed551/m551_locked_broad10_sharp_guard_rel_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51189 | 0.36640 | 0.66082 | 0.59947 | 0.53230 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00558`
- MAP@100: `+0.00536`
- Recall@100: `+0.00266`
- MRR@20: `+0.00395`
- Dense overlap@100: `+0.00462`

Delta versus the unguarded sharp run:

- NDCG@10: `-0.00171`
- MAP@100: `-0.00327`
- Recall@100: `-0.00280`
- MRR@20: `-0.00295`
- Dense overlap@100: `+0.00012`

Interpretation: the relative top-rank guard remains positive versus
dense-topk128, but it weakens the best sharp run and does not fix the
`TRECCOVID` MRR@20 regression.  Keep the teacher top-k/top-1 diagnostics, but
do not promote this guard as the default checkpoint policy yet.

The larger DREAM-style candidate competition run completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_pool256_neg256_seed551
```

Config delta:

- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- `TEACHER_POOL_K=256`
- `RANDOM_NEGATIVES=256`
- `BATCH_SIZE=8`

Large candidate-set result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_pool256_neg256_seed551/m551_locked_broad10_sharp_pool256_neg256_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.50740 | 0.36488 | 0.66152 | 0.59036 | 0.53008 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00109`
- MAP@100: `+0.00384`
- Recall@100: `+0.00336`
- MRR@20: `-0.00516`
- Dense overlap@100: `+0.00240`

Delta versus the unguarded sharp run:

- NDCG@10: `-0.00620`
- MAP@100: `-0.00479`
- Recall@100: `-0.00210`
- MRR@20: `-0.01206`
- Dense overlap@100: `-0.00210`

Interpretation: simply increasing DREAM-style candidate-set competition from
`128+128` to `256+256` weakens the line.  The positive signal is not coming
from larger candidate competition alone; it is more likely tied to the sharper
teacher/student distribution under the original candidate budget.

The follow-ups therefore shifted to loss-weight isolation:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_supportw025_seed551
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_score0_seed551
```

Config deltas:

- `supportw025`: `SUPPORT_WEIGHT=0.25`
- `score0`: `SCORE_WEIGHT=0.0`
- Both keep `TEACHER_TEMPERATURE=0.025` and `STUDENT_TEMPERATURE=0.050`.

Support-weight result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_supportw025_seed551/m551_locked_broad10_sharp_supportw025_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51357 | 0.36963 | 0.66394 | 0.60232 | 0.53224 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00726`
- MAP@100: `+0.00859`
- Recall@100: `+0.00578`
- MRR@20: `+0.00680`
- Dense overlap@100: `+0.00456`

Delta versus the unmodified sharp run:

- NDCG@10: `-0.00003`
- MAP@100: `-0.00004`
- Recall@100: `+0.00032`
- MRR@20: `-0.00010`
- Dense overlap@100: `+0.00006`

Interpretation: reducing `SUPPORT_WEIGHT` from `0.5` to `0.25` is essentially
neutral.  It does not explain the sharp-run improvement and does not need more
scale.

The completed follow-up on `spark-1` tests training depth:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_epochs8_seed551
```

Config delta:

- `EPOCHS=8`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Training-depth result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_epochs8_seed551/m551_locked_broad10_sharp_epochs8_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51477 | 0.37010 | 0.66405 | 0.60245 | 0.53264 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00846`
- MAP@100: `+0.00906`
- Recall@100: `+0.00589`
- MRR@20: `+0.00693`
- Dense overlap@100: `+0.00496`

Delta versus the unmodified sharp run:

- NDCG@10: `+0.00117`
- MAP@100: `+0.00043`
- Recall@100: `+0.00043`
- MRR@20: `+0.00003`
- Dense overlap@100: `+0.00046`

Interpretation: deeper training is the first follow-up that improves the sharp
run.  The gain is modest but real, and selected epochs show that some tasks
benefit from training beyond epoch 4 (`ArguAna` selected epoch 8, `TRECCOVID`
and `Touche2020Retrieval.v3` selected epoch 6).  This supports testing whether
the line saturates around 8 epochs or can still improve.

The completed follow-up on `spark-1` tests the next depth step:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_epochs12_seed551
```

Config delta:

- `EPOCHS=12`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Depth-12 result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_epochs12_seed551/m551_locked_broad10_sharp_epochs12_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51305 | 0.37029 | 0.66449 | 0.59745 | 0.53259 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00674`
- MAP@100: `+0.00925`
- Recall@100: `+0.00633`
- MRR@20: `+0.00193`
- Dense overlap@100: `+0.00491`

Delta versus depth 8:

- NDCG@10: `-0.00172`
- MAP@100: `+0.00019`
- Recall@100: `+0.00044`
- MRR@20: `-0.00500`
- Dense overlap@100: `-0.00005`

Interpretation: depth 12 is not better.  It slightly increases MAP/Recall over
depth 8, but it loses NDCG and especially MRR.  Depth should remain around
8 unless combined with a smaller residual step that changes the trajectory.

The completed follow-up on `spark-1` tests a smaller residual step:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid0125_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.0125`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Smaller residual-scale result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid0125_seed551/m551_locked_broad10_sharp_resid0125_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51518 | 0.36752 | 0.66168 | 0.60593 | 0.53036 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00887`
- MAP@100: `+0.00648`
- Recall@100: `+0.00352`
- MRR@20: `+0.01041`
- Dense overlap@100: `+0.00268`

Delta versus `RESIDUAL_SCALE=0.025`:

- NDCG@10: `-0.00171`
- MAP@100: `-0.00274`
- Recall@100: `-0.00239`
- MRR@20: `-0.00306`
- Dense overlap@100: `-0.00112`

Interpretation: `RESIDUAL_SCALE=0.0125` is positive but weaker than `0.025`.
The residual-scale optimum is not below `0.025` under the current setup.

The completed follow-up on `spark-1` tests the midpoint between the best
known step and the original step:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid0375_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.0375`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Midpoint residual-scale result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid0375_seed551/m551_locked_broad10_sharp_resid0375_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51635 | 0.36987 | 0.66412 | 0.60776 | 0.53189 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.01004`
- MAP@100: `+0.00883`
- Recall@100: `+0.00596`
- MRR@20: `+0.01224`
- Dense overlap@100: `+0.00421`

Delta versus `RESIDUAL_SCALE=0.025`:

- NDCG@10: `-0.00054`
- MAP@100: `-0.00039`
- Recall@100: `+0.00005`
- MRR@20: `-0.00123`
- Dense overlap@100: `+0.00041`

Interpretation: `RESIDUAL_SCALE=0.0375` is close but weaker than `0.025`.
Together with the `0.0125` result, this localizes the current residual-scale
optimum around `0.025`.

Score-loss result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_score0_seed551/m551_locked_broad10_sharp_score0_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51353 | 0.36946 | 0.66355 | 0.60238 | 0.53165 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00722`
- MAP@100: `+0.00842`
- Recall@100: `+0.00539`
- MRR@20: `+0.00686`
- Dense overlap@100: `+0.00397`

Delta versus the unmodified sharp run:

- NDCG@10: `-0.00007`
- MAP@100: `-0.00021`
- Recall@100: `-0.00007`
- MRR@20: `-0.00004`
- Dense overlap@100: `-0.00053`

Interpretation: removing standardized score MSE is also effectively neutral to
slightly weaker.  The sharp-run gain is not explained by this auxiliary loss;
do not keep tuning score-weight unless a later diagnostic points back here.

The completed follow-up on `spark-2` tests a smaller residual step size:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Residual-scale result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed551/m551_locked_broad10_sharp_resid025_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51689 | 0.37026 | 0.66407 | 0.60899 | 0.53148 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.01058`
- MAP@100: `+0.00922`
- Recall@100: `+0.00591`
- MRR@20: `+0.01347`
- Dense overlap@100: `+0.00380`

Delta versus the unmodified sharp run:

- NDCG@10: `+0.00329`
- MAP@100: `+0.00059`
- Recall@100: `+0.00045`
- MRR@20: `+0.00657`
- Dense overlap@100: `-0.00070`

Interpretation: `RESIDUAL_SCALE=0.025` is the strongest current result.  It
substantially improves MRR@20 while also raising NDCG@10, MAP@100, and
Recall@100.  This indicates that the step size, not candidate-pool size or
auxiliary loss weights, is the main controllable parameter found so far.

The completed follow-up on `spark-2` combines the best residual scale with
deeper training:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_epochs8_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `EPOCHS=8`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Residual-scale plus depth result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_epochs8_seed551/m551_locked_broad10_sharp_resid025_epochs8_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51453 | 0.37123 | 0.66541 | 0.60427 | 0.53190 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00822`
- MAP@100: `+0.01019`
- Recall@100: `+0.00725`
- MRR@20: `+0.00875`
- Dense overlap@100: `+0.00422`

Delta versus `RESIDUAL_SCALE=0.025, EPOCHS=4`:

- NDCG@10: `-0.00236`
- MAP@100: `+0.00097`
- Recall@100: `+0.00134`
- MRR@20: `-0.00472`
- Dense overlap@100: `+0.00042`

Interpretation: combining smaller residual steps with depth 8 shifts the
tradeoff toward MAP/Recall, but loses too much NDCG/MRR versus the epoch-4
residual-scale result.  The current best single run remains
`RESIDUAL_SCALE=0.025, EPOCHS=4`.

The completed follow-up on `spark-2` repeats the best single run with a
different seed:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed552
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `SEED=552`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Seed552 result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed552/m551_locked_broad10_sharp_resid025_seed552.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.49713 | 0.35647 | 0.65734 | 0.58886 | 0.52737 |
| `m551_shared_locked_support_residual_dream_lite` | 0.50141 | 0.36002 | 0.65980 | 0.59670 | 0.52988 |

Delta versus same-run `dense_topk128_sparse`:

- NDCG@10: `+0.00428`
- MAP@100: `+0.00355`
- Recall@100: `+0.00246`
- MRR@20: `+0.00784`
- Dense overlap@100: `+0.00251`

Interpretation: the `RESIDUAL_SCALE=0.025` improvement survives a different
seed/split and improves the seed552 sharp baseline most strongly on MRR.  This
makes residual scale the current strongest actionable finding from M551.

The completed follow-up on `spark-2` tests whether the best residual scale
benefits from an even sharper teacher/student distribution:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_resid025_t020_s040_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `TEACHER_TEMPERATURE=0.020`
- `STUDENT_TEMPERATURE=0.040`

Result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_resid025_t020_s040_seed551/m551_locked_broad10_resid025_t020_s040_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51664 | 0.37017 | 0.66328 | 0.60912 | 0.53126 |

Delta versus same-run `dense_topk128_sparse`:

- NDCG@10: `+0.01033`
- MAP@100: `+0.00913`
- Recall@100: `+0.00512`
- MRR@20: `+0.01360`
- Dense overlap@100: `+0.00358`

Delta versus `RESIDUAL_SCALE=0.025, T=0.025/S=0.050`:

- NDCG@10: `-0.00025`
- MAP@100: `-0.00009`
- Recall@100: `-0.00079`
- MRR@20: `+0.00013`
- Dense overlap@100: `-0.00022`

Interpretation: sharpening from `T=0.025/S=0.050` to `T=0.020/S=0.040`
does not produce a clean new best.  It very slightly improves MRR, but gives
back NDCG, MAP, Recall, and dense-overlap.  The useful temperature range is now
localized around `T=0.020-0.025`, with the current promoted setting remaining
`T=0.025/S=0.050` unless a softer follow-up beats it more broadly.

The completed follow-up on `spark-1` tests whether a slightly softer
teacher/student distribution improves the residual-scale route:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_resid025_t030_s060_seed551
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `TEACHER_TEMPERATURE=0.030`
- `STUDENT_TEMPERATURE=0.060`

Result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_resid025_t030_s060_seed551/m551_locked_broad10_resid025_t030_s060_seed551.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50631 | 0.36104 | 0.65816 | 0.59552 | 0.52768 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51514 | 0.36843 | 0.66311 | 0.60656 | 0.53186 |

Delta versus same-run `dense_topk128_sparse`:

- NDCG@10: `+0.00883`
- MAP@100: `+0.00739`
- Recall@100: `+0.00495`
- MRR@20: `+0.01104`
- Dense overlap@100: `+0.00418`

Delta versus `RESIDUAL_SCALE=0.025, T=0.025/S=0.050`:

- NDCG@10: `-0.00175`
- MAP@100: `-0.00183`
- Recall@100: `-0.00096`
- MRR@20: `-0.00243`
- Dense overlap@100: `+0.00038`

Interpretation: softening to `T=0.030/S=0.060` preserves a positive gain over
the dense-topk sparse baseline, but weakens every main metric relative to the
promoted `T=0.025/S=0.050` setting.  Together with the sharper
`T=0.020/S=0.040` run, this closes the local temperature sweep: the best
balanced setting remains `RESIDUAL_SCALE=0.025, T=0.025/S=0.050`.

The completed robustness follow-up on `spark-2` repeats the promoted setting
with one more seed:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed553
```

Config delta:

- `RESIDUAL_SCALE=0.025`
- `SEED=553`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`

Result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed553/m551_locked_broad10_sharp_resid025_seed553.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.50653 | 0.36196 | 0.65578 | 0.61196 | 0.53436 |
| `m551_shared_locked_support_residual_dream_lite` | 0.51178 | 0.36874 | 0.66179 | 0.62304 | 0.53817 |

Delta versus same-run `dense_topk128_sparse`:

- NDCG@10: `+0.00525`
- MAP@100: `+0.00678`
- Recall@100: `+0.00601`
- MRR@20: `+0.01108`
- Dense overlap@100: `+0.00381`

Current promoted M551 setting:

- `RESIDUAL_SCALE=0.025`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- `EPOCHS=4`
- `RANDOM_NEGATIVES=128`
- `TEACHER_POOL_K=128`

Three-seed delta summary for the promoted setting:

| Metric | Mean delta | Std | Min | Max |
| --- | ---: | ---: | ---: | ---: |
| NDCG@10 | +0.00670 | 0.00277 | +0.00428 | +0.01058 |
| MAP@100 | +0.00652 | 0.00232 | +0.00355 | +0.00922 |
| Recall@100 | +0.00479 | 0.00165 | +0.00246 | +0.00601 |
| MRR@20 | +0.01080 | 0.00231 | +0.00784 | +0.01347 |
| Dense overlap@100 | +0.00337 | 0.00061 | +0.00251 | +0.00381 |

Conclusion: within the currently materialized MTEB broad10 surface, the
DREAM-lite candidate-set signal is a real positive signal rather than a
single-run accident.  The effect size is moderate, not a new end-state
breakthrough, but it is strong enough to promote M551 as the current
BM25-free first-stage training milestone.  Further local temperature
micro-sweeps are lower value; the next valuable test needs either a restored
larger BEIR/MTEB task root or a materially closer frozen-judge objective.
