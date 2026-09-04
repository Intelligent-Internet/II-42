# II-42 M413 Deep Student Dense Distillation Report

Date: 2026-06-27

## Goal

M413 moves beyond small feasibility probes and starts a deeper dense-only
distillation schedule:

```text
raw text -> trainable MiniLM last layers + dense head -> pplx teacher dense
```

The objective remains dense teacher imitation only.  Posting and BM25/ranking
losses stay disabled until dense parity improves substantially.

## Starting Point

M410 proved that the teacher target is reachable when the same `pplx` encoder
is used.  M411/M412 showed that shallow frozen-head training is insufficient,
but partial fine-tuning has a small positive slope.

Best prior local baseline:

| Route | Dense NDCG@10 | Posting NDCG@10 | Student + BM25 NDCG@10 |
| --- | ---: | ---: | ---: |
| M411 MiniLM frozen MLP2048 allrows | 0.35688 | 0.28727 | 0.39264 |
| M412 MiniLM last2 allrows e16 | 0.36999 | 0.28049 | 0.39699 |
| Teacher dense from same split | 0.53283 | 0.42291 | 0.53569 |

## M413 Run

First deep run:

- host: `spark-1`
- student: `sentence-transformers/all-MiniLM-L6-v2`
- trainable layers: last 2 transformer layers plus MLP2048 dense head
- rows: all FiQA materialized docs + queries
- epochs: 32
- checkpoint cadence: every 4 epochs
- loss: dense cosine + dense MSE

Artifact root:

`/home/huoju/leask/runs/ii42-m413-deep-student-dense-distill-v1/fiqa_minilm_last2_allrows_e32_seed417`

Capacity/depth contrast run:

- host: `spark-2`
- student: `BAAI/bge-base-en-v1.5`
- trainable layers: last 4 transformer layers plus MLP2048 dense head
- rows: all FiQA materialized docs + queries
- epochs: 16
- checkpoint cadence: every 4 epochs
- loss: dense cosine + dense MSE

Artifact root:

`/home/huoju/leask/runs/ii42-m413-deep-student-dense-distill-v1/fiqa_bge_base_last4_allrows_e16_seed418`

The spark-2 run reached epoch 7 but did not produce a final JSON before
another higher-priority task occupied the GPU.  The same BGE configuration was
therefore restarted on `spark-1` with an independent output root:

`/home/huoju/leask/runs/ii42-m413-deep-student-dense-distill-v1/fiqa_bge_base_last4_allrows_e16_seed418_spark1`

## Results

| Variant | Dense NDCG@10 | Posting NDCG@10 | Student + BM25 NDCG@10 | Doc Dense Cos | Query Dense Cos |
| --- | ---: | ---: | ---: | ---: | ---: |
| M412 MiniLM last2 e16 | 0.36999 | 0.28049 | 0.39699 | 0.81965 | 0.79452 |
| M413 MiniLM last2 e32 | 0.35450 | 0.25736 | 0.37830 | 0.83336 | 0.81452 |
| M413 BGE-base last4 e16 | 0.43778 | 0.32742 | 0.46009 | 0.85619 | 0.83495 |

The MiniLM e32 result improves vector cosine but worsens retrieval.  This
means the deeper schedule is overfitting the teacher vector regression surface
without preserving the retrieval-relevant geometry.  More epochs alone should
not be promoted.

The BGE-base run is the first student route in this series to make a material
retrieval jump.  It does not match the teacher yet, but it narrows the dense
gap substantially and also improves deterministic posting.

## Decision Rule

Continue this route only if dense teacher imitation improves meaningfully over
M412 e16.  If dense NDCG/cosine plateaus near the same level, the next step is
not more FiQA tuning.  It is broader teacher-target materialization across more
corpora/texts.

## Live Status

As of launch:

| Host | Variant | Status | Early Signal |
| --- | --- | --- | --- |
| `spark-1` | MiniLM last2 e32 | complete | dense NDCG@10 `0.35450`, below M412 e16 |
| `spark-2` | BGE-base last4 e16 | interrupted | reached epoch 7: `0.28987 -> 0.18669`, no final JSON |
| `spark-1` | BGE-base last4 e16 | complete | dense NDCG@10 `0.43778`, posting `0.32742` |
