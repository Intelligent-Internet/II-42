# II-42 M404 Multitask Dense-Preservation Report

Date: 2026-06-27

## Goal

M404 tested the hypothesis that broader dense-teacher distillation can improve
the M403-A signal.  The model was a shared dense-space query adapter:

```text
query_dense -> adapted query_dense
```

Each task then projected the adapted query through its own deterministic
M392-style structural route.  Document postings were frozen.  Training stayed
qrels-free; qrels were used only for held-out evaluation.

## Runs

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m404-multitask-dense-preservation-v1`

Execution was CPU-only and low priority:

```bash
CUDA_VISIBLE_DEVICES= OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 \
OPENBLAS_NUM_THREADS=4 nice -n 10 ionice -c2 -n7 ...
```

For the 4-task rerun, task names were corrected to the actual materialized
MTEB task roots:

```text
FiQA2018, ArguAna, SCIDOCS, TRECCOVID
```

The shared root does not contain `SciFact` or `NFCorpus`.

## FiQA Sanity

Task: `FiQA2018`, seed `404`, 1 epoch.

| Route | NDCG@10 |
| --- | ---: |
| exact dense | 0.51790 |
| teacher structural | 0.42180 |
| M404 dense-space adapter | 0.41326 |
| teacher structural + BM25 a0.10 | 0.51750 |
| M404 adapter + BM25 a0.10 | 0.49743 |

This fails the basic gate.  The learned dense-space adapter is below the
deterministic structural teacher.

## Four-Task Canary

Run:
`/home/huoju/leask/runs/ii42-m404-multitask-dense-preservation-v1/multitask_4x64_e1/m404_multitask_4x64_e1.json`

Config:

- tasks: `FiQA2018,ArguAna,SCIDOCS,TRECCOVID`
- epochs: `1`
- max train groups per task: `64`

### Macro

| Source | NDCG@10 |
| --- | ---: |
| exact dense | 0.48814 |
| teacher structural | 0.43313 |
| M404 dense-space adapter | 0.43071 |
| teacher structural + BM25 a0.10 | 0.50269 |
| M404 adapter + BM25 a0.10 | 0.50223 |

### Per Task

| Task | Source | NDCG@10 | Dense O@100 | Recall@100 |
| --- | --- | ---: | ---: | ---: |
| ArguAna | teacher structural | 0.44425 | 0.94404 | 1.00000 |
| ArguAna | M404 adapter | 0.45459 | 0.87461 | 1.00000 |
| FiQA2018 | teacher structural | 0.42180 | 0.54985 | 0.60346 |
| FiQA2018 | M404 adapter | 0.41889 | 0.52392 | 0.59566 |
| SCIDOCS | teacher structural | 0.21797 | 0.83600 | 0.47497 |
| SCIDOCS | M404 adapter | 0.21093 | 0.76648 | 0.46887 |
| TRECCOVID | teacher structural | 0.64849 | 0.19760 | 0.08658 |
| TRECCOVID | M404 adapter | 0.63841 | 0.19160 | 0.08280 |

## Interpretation

M404 does not validate the broad shared dense-space adapter idea.

The model improves ArguAna NDCG, but it damages dense overlap on every task and
loses on FiQA, SCIDOCS, and TRECCOVID.  The BM25 blend also remains below the
deterministic structural + static BM25 route.

This suggests that adapting original dense coordinates before task-specific
structural projection is the wrong abstraction.  It is too easy to disrupt the
corpus-specific structural geometry that made M392-M396 work.

## Decision

Do not scale M404 as-is.

The next useful route is M405: keep the correction in structural query space,
freeze deterministic document postings, and add explicit dense-overlap anchors
to reduce M403-A's Dense O@100 regression.
