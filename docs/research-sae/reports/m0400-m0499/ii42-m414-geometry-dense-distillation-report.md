# II-42 M414 Geometry Dense Distillation Report

Date: 2026-06-28

## Goal

M414 keeps the first-stage objective pure:

```text
raw text -> student dense encoder -> pplx teacher dense geometry
```

No BM25, qrels, or ranking labels are used for training.  They remain
evaluation-only.  The change from M412/M413 is that dense imitation is no
longer only pointwise cosine/MSE.  M414 also distills the teacher's local
geometry inside each batch.

## Why

M413 showed two different failure/success signals:

| Route | Dense NDCG@10 | Posting NDCG@10 | +BM25 NDCG@10 | Doc Cos | Query Cos |
| --- | ---: | ---: | ---: | ---: | ---: |
| M412 MiniLM last2 e16 | 0.36999 | 0.28049 | 0.39699 | 0.81965 | 0.79452 |
| M413 MiniLM last2 e32 | 0.35450 | 0.25736 | 0.37830 | 0.83336 | 0.81452 |
| M413 BGE-base last4 e16 | 0.43778 | 0.32742 | 0.46009 | 0.85619 | 0.83495 |

MiniLM e32 improved vector cosine but hurt retrieval, so pointwise vector
regression is not enough.  BGE-base has enough capacity to improve retrieval,
but still trails the dense teacher.  M414 tests whether preserving the teacher
similarity geometry closes part of that gap.

## Loss

Training loss:

- pointwise dense cosine loss
- pointwise dense MSE
- in-batch teacher/student similarity KL
- off-diagonal similarity matrix MSE
- teacher top-k neighbor similarity MSE

This is still dense-only teacher distillation.  It does not optimize qrels,
BM25, or final hybrid weights.

## First Run

- host: `spark-1`
- student: `BAAI/bge-base-en-v1.5`
- trainable layers: last 4 transformer layers plus MLP2048 dense head
- rows: all FiQA materialized docs + repeated query texts
- query repeat: 4
- epochs: 16
- checkpoint cadence: every 4 epochs
- output root:
  `/home/huoju/leask/runs/ii42-m414-geometry-dense-distill-v1/fiqa_bge_base_last4_geometry_e16_seed414`

## First Result

| Variant | Dense NDCG@10 | Posting NDCG@10 | Student + BM25 NDCG@10 | Doc Cos | Query Cos |
| --- | ---: | ---: | ---: | ---: | ---: |
| M414 BGE-base geometry e16 seed414 | 0.43126 | 0.36430 | 0.45084 | 0.83722 | 0.89796 |

This run is not a strict apples-to-apples comparison with M413 BGE seed418,
because the heldout query split changes with the seed.  It is still useful as
a geometry signal: deterministic posting improves strongly, while exact dense
is not clearly better.

The formal comparison run uses seed418 to match the M413 BGE split:

`/home/huoju/leask/runs/ii42-m414-geometry-dense-distill-v1/fiqa_bge_base_last4_geometry_e16_seed418`

## Formal Result

| Variant | Dense NDCG@10 | Posting NDCG@10 | Student + BM25 NDCG@10 | Doc Cos | Query Cos |
| --- | ---: | ---: | ---: | ---: | ---: |
| M413 BGE-base last4 e16 seed418 | 0.43778 | 0.32742 | 0.46009 | 0.85619 | 0.83495 |
| M414 BGE-base geometry e16 seed418 | 0.44615 | 0.36302 | 0.46300 | 0.83818 | 0.89712 |

M414 improves exact dense retrieval and deterministic posting on the matched
split.  Dense cosine is not uniformly higher: doc cosine drops, query cosine
rises.  This is acceptable because promotion is based on retrieval geometry,
not average vector cosine.

The strongest signal is posting NDCG@10: `0.32742 -> 0.36302`.  This supports
the hypothesis that preserving teacher local similarity geometry is closer to
the target posting surface than pointwise vector regression alone.

## Decision Rule

Promote this direction only if it improves dense retrieval over M413 BGE-base
without merely increasing cosine.  The key metrics are:

- `exact_dense_student` NDCG@10
- deterministic posting NDCG@10
- doc/query dense cosine as diagnostics, not the promotion metric

## Live Status

| Host | Variant | Status | Notes |
| --- | --- | --- | --- |
| `spark-1` | BGE-base last4 geometry e16 seed414 | complete | dense `0.43126`, posting `0.36430` |
| `spark-1` | BGE-base last4 geometry e16 seed418 | complete | dense `0.44615`, posting `0.36302` |
