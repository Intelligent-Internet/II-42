# II-42 M406 Static-Hybrid Survival Gate Report

Date: 2026-06-27

## Goal

M406 tested whether the current hybrid weakness is an admission problem.

The baseline route admits structural candidates using sparse coordinate scores,
then applies a fixed static BM25 blend.  M406 tried two alternatives:

- direct static-blend admission: select structural candidates by the same fixed
  hybrid score used for final ranking;
- learned survival gate: train a qrels-free gate to predict which structural
  candidates survive fixed static hybrid top-k.

The final ranker stayed deterministic:

```text
score = (1 - alpha) * zscore(structural_tail) + alpha * zscore(BM25)
```

No learned BM25 alpha was used.

## Runs

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m406-static-hybrid-survival-gate-v1`

Execution was CPU-only and low priority:

```bash
CUDA_VISIBLE_DEVICES= OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 \
OPENBLAS_NUM_THREADS=2 nice -n 15 ionice -c2 -n7 ...
```

## FiQA Seed404, 64 Train Groups, Survival K 1000

Run:
`/home/huoju/leask/runs/ii42-m406-static-hybrid-survival-gate-v1/fiqa_64_seed404/m406_fiqa_64_seed404.json`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51790 | 1.00000 | 1.00000 | 0.84661 | 0.60205 | 0.45420 |
| teacher structural | 0.42180 | 0.08002 | 0.54985 | 0.60346 | 0.52587 | 0.35875 |
| teacher + BM25 a0.10 | 0.51750 | 0.14524 | 0.79926 | 0.82970 | 0.61054 | 0.45105 |
| teacher static admission | 0.51638 | 0.14296 | 0.79852 | 0.82970 | 0.60902 | 0.44956 |
| teacher survival gate | 0.51750 | 0.14307 | 0.79858 | 0.82970 | 0.61053 | 0.45109 |
| M405 query + BM25 a0.10 | 0.51676 | 0.14532 | 0.79725 | 0.82610 | 0.60910 | 0.45008 |
| M405 static admission | 0.51549 | 0.14304 | 0.79648 | 0.82610 | 0.60789 | 0.44865 |
| M405 survival gate | 0.51549 | 0.14290 | 0.79651 | 0.82610 | 0.60789 | 0.44866 |

The gate matches the teacher hybrid score while slightly reducing fanout, but
it does not improve quality.  Static admission is worse than the original
sparse-admission baseline.

## FiQA Seed404, 64 Train Groups, Survival K 100

Run:
`/home/huoju/leask/runs/ii42-m406-static-hybrid-survival-gate-v1/fiqa_64_seed404_k100/m406_fiqa_64_seed404_k100.json`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| teacher + BM25 a0.10 | 0.51750 | 0.14524 | 0.79926 | 0.82970 | 0.61054 | 0.45105 |
| teacher survival gate | 0.51534 | 0.13838 | 0.78515 | 0.82970 | 0.60820 | 0.44910 |
| M405 query + BM25 a0.10 | 0.51676 | 0.14532 | 0.79725 | 0.82610 | 0.60910 | 0.45008 |
| M405 survival gate | 0.51598 | 0.13769 | 0.78293 | 0.82559 | 0.60851 | 0.44866 |

Focusing the target on top-100 survival reduces fanout, but it also reduces
Dense O@100 and NDCG.  This is not a valid quality improvement.

## Interpretation

M406 does not validate the learned survival-gate idea.

The current sparse admission baseline is already close to optimal for the
static BM25 blend on this FiQA surface.  Direct static-blend admission and the
learned gate both fail to beat deterministic structural + static BM25.

The only useful signal is efficiency: the survival gate can preserve roughly
the same score with slightly lower touch when `survival_k=1000`.  But the gain
is too small and the evaluation cost is higher, so this is not worth scaling.

## Decision

Stop M406 in its current form.  Do not run the 4-task canary.

The stronger validated path remains:

- M392-M396 deterministic structural dense-tail + static BM25 for broad
  benchmark quality;
- M405 one-epoch structural query correction as a small dense-faithful local
  improvement, but not deeper training;
- no shared dense-space adapter;
- no learned BM25 alpha;
- no learned survival gate unless a future version changes the runtime
  objective, for example by reducing fanout substantially under a strict
  quality-preservation constraint.
