# II-42 M415 Shared Geometry Dense Distillation Report

Date: 2026-06-28

## Goal

M415 tests the next logical step after M414:

```text
multi-task raw text -> one shared student dense encoder -> PPLX teacher geometry
```

The key change is not a new ranking loss.  It is removing the per-task
training limitation in M414.  One BGE-base student is trained across multiple
materialized PPLX1024 task roots, then evaluated per task.

Qrels, posting, and BM25 remain evaluation-only.  Stage 1 is still dense
teacher imitation.

## Why

M414 showed that dense geometry preservation improves FiQA on a matched split:

| Variant | Dense NDCG@10 | Posting NDCG@10 | +BM25 NDCG@10 |
| --- | ---: | ---: | ---: |
| M413 BGE-base last4 e16 seed418 | 0.43778 | 0.32742 | 0.46009 |
| M414 BGE-base geometry e16 seed418 | 0.44615 | 0.36302 | 0.46300 |

That result is still single-task.  M415 asks whether a shared encoder trained
on broader PPLX teacher geometry becomes more generally dense-faithful, rather
than overfitting a single FiQA surface.

## Implementation

Script:

`scripts/research_sae_m415_shared_geometry_dense_distill.py`

Differences from M414:

- samples dense teacher rows from multiple task roots before training;
- trains one shared raw-text student encoder;
- evaluates the same trained encoder on each task root;
- keeps the M414 geometry loss: pointwise cosine/MSE, in-batch similarity KL,
  off-diagonal similarity MSE, and teacher top-k similarity MSE.

## First Canary

Host: `spark-1`

Output root:

`/home/huoju/leask/runs/ii42-m415-shared-geometry-dense-distill-v1/b4_bge_base_last4_geometry_e8_seed415`

Log:

`/home/huoju/leask/runs/ii42-m415-shared-geometry-dense-distill-v1/b4_bge_base_last4_geometry_e8_seed415.log`

Source root:

`/home/huoju/leask/runs/bm25sae-m150-c4-official-beir-df012-v2/all-test`

Tasks:

`nfcorpus, scifact, fiqa, arguana`

Config:

- student: `BAAI/bge-base-en-v1.5`
- trainable layers: last 4 transformer layers plus MLP2048 dense head
- max docs per task: `8192`
- max queries per task: `2048`
- query repeat: `4`
- epochs: `8`
- training rows: `35908`
- seed: `415`

## Live Status

| Host | Session | Stage | Status |
| --- | --- | --- | --- |
| `spark-1` | `ii42_m415_b4_shared_geometry` | b4 canary | complete |
| `spark-1` | `ii42_m415_broad8_shared_geometry` | broad8 canary | complete |

## B4 Result

Completed output:

`/home/huoju/leask/runs/ii42-m415-shared-geometry-dense-distill-v1/b4_bge_base_last4_geometry_e8_seed415/m415_b4_bge_base_last4_geometry_e8_seed415.json`

Runtime: `1812.661s`

Macro:

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `exact_dense_teacher` | 0.51895 | 4 |
| `m415_teacher_structural_tail_bm25_zblend_a010` | 0.51811 | 4 |
| `m415_teacher_structural_tail` | 0.48213 | 4 |
| `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.48130 | 4 |
| `exact_dense_student` | 0.47053 | 4 |
| `m415_shared_student_structural_tail` | 0.43707 | 4 |

Per-task exact dense NDCG@10:

| Task | Student | Teacher |
| --- | ---: | ---: |
| `arguana` | 0.43064 | 0.44279 |
| `fiqa` | 0.38290 | 0.51242 |
| `nfcorpus` | 0.34772 | 0.35587 |
| `scifact` | 0.72087 | 0.76471 |

Per-task student structural + BM25 NDCG@10:

| Task | Student | Teacher Structural |
| --- | ---: | ---: |
| `arguana` | 0.41891 | 0.42589 |
| `fiqa` | 0.40342 | 0.40539 |
| `nfcorpus` | 0.37669 | 0.34726 |
| `scifact` | 0.72616 | 0.74999 |

Interpretation:

- the shared student is not yet a dense replacement: exact dense macro is
  `0.47053` vs teacher `0.51895`;
- it is strong enough for the posting route: student structural + BM25
  `0.48130` is essentially tied with teacher structural `0.48213`;
- FiQA exact dense remains the largest gap, so broader training should test
  whether the shared encoder learns a more stable cross-task geometry.

## Next Gate

Scale the same objective to broad8 on `spark-1`:

`nfcorpus, scifact, fiqa, arguana, scidocs, trec-covid, cqadupstack, webis-touche2020`

Promotion criteria:

1. exact dense student should improve relative to the b4 macro/teacher ratio;
2. deterministic student posting should stay close to teacher structural;
3. BM25 blend must be reported separately and cannot hide dense-only failure.

## Broad8 Result

Completed output:

`/home/huoju/leask/runs/ii42-m415-shared-geometry-dense-distill-v1/broad8_bge_base_last4_geometry_e8_seed415/m415_broad8_bge_base_last4_geometry_e8_seed415.json`

Runtime: `11582.782s`

Macro:

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `m415_teacher_structural_tail_bm25_zblend_a010` | 0.48101 | 8 |
| `exact_dense_teacher` | 0.47718 | 8 |
| `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.44510 | 8 |
| `exact_dense_student` | 0.41677 | 8 |
| `m415_teacher_structural_tail` | 0.40756 | 8 |
| `m415_shared_student_structural_tail` | 0.35041 | 8 |

Per-task exact dense NDCG@10:

| Task | Student | Teacher |
| --- | ---: | ---: |
| `arguana` | 0.42184 | 0.44279 |
| `cqadupstack` | 0.34249 | 0.44527 |
| `fiqa` | 0.38147 | 0.51242 |
| `nfcorpus` | 0.33518 | 0.35587 |
| `scidocs` | 0.21173 | 0.23454 |
| `scifact` | 0.68638 | 0.76471 |
| `trec-covid` | 0.78297 | 0.83933 |
| `webis-touche2020` | 0.17212 | 0.22251 |

Per-task student structural + BM25 NDCG@10:

| Task | Student | Teacher Structural + BM25 |
| --- | ---: | ---: |
| `arguana` | 0.41520 | 0.42460 |
| `cqadupstack` | 0.37510 | 0.44098 |
| `fiqa` | 0.40610 | 0.51225 |
| `nfcorpus` | 0.35313 | 0.37132 |
| `scidocs` | 0.21516 | 0.22946 |
| `scifact` | 0.71630 | 0.76425 |
| `trec-covid` | 0.85159 | 0.85919 |
| `webis-touche2020` | 0.22822 | 0.24599 |

Interpretation:

- broad8 confirms the shared encoder is learning real dense-like geometry, but
  it is still not a dense replacement: exact dense macro is `0.41677` vs
  teacher `0.47718`;
- the largest gaps are `fiqa`, `cqadupstack`, and `webis-touche2020`;
- BM25 blend rescues much of the runtime route, but the dense-only gap remains
  too large to call this Stage 1 solved;
- next work should increase teacher-fit capacity/depth or add a harder
  teacher-geometry schedule, not jump to ranking/BM25 training yet.
