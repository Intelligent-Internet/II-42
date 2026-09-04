# II-42 M407 Dense-Function Posting Encoder Report

Date: 2026-06-27

## Goal

M407 tests a qrels-free dense-function posting encoder:

```text
dense/text embedding -> sparse signed coordinates + residual sketch
```

The goal is not benchmark tuning.  The student must preserve dense teacher
ranking behavior while producing an indexable posting representation.

## Runs

All runs were executed on `spark-2` with `CUDA_VISIBLE_DEVICES=` and low CPU/IO
priority to avoid interfering with active GPU work.

| Run | Variant | Train groups | Epochs | Scoring objective |
| --- | --- | ---: | ---: | --- |
| `fiqa_64_seed407` | `shared_linear,shared_mlp` | 64 | 2 | normalized active/sketch |
| `fiqa_192_e4_seed407` | `shared_linear` | 192 | 4 | normalized active/sketch |
| `route_score_64_seed407` | `shared_linear` | 64 | 2 | route-consistent active/sketch |

Remote artifacts:

- `/home/huoju/leask/runs/ii42-m407-dense-function-posting-encoder-v1/fiqa_64_seed407`
- `/home/huoju/leask/runs/ii42-m407-dense-function-posting-encoder-v1/fiqa_192_e4_seed407`
- `/home/huoju/leask/runs/ii42-m407-dense-function-posting-encoder-v1/route_score_64_seed407`

## Retrieval Matrix

| Run | Source | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Touch | Dense O@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa_64_seed407` | `exact_dense_teacher` | 0.53386 | 0.82708 | 0.62191 | 0.47403 | 1.00000 | 1.00000 |
| `fiqa_64_seed407` | `teacher_structural_tail_bm25_a010` | 0.52556 | 0.80018 | 0.62160 | 0.46210 | 0.14509 | 0.78670 |
| `fiqa_64_seed407` | `shared_linear_bm25_a010` | 0.49562 | 0.79037 | 0.58554 | 0.43610 | 0.14166 | 0.65407 |
| `fiqa_64_seed407` | `shared_linear` | 0.46959 | 0.73975 | 0.55788 | 0.40588 | 0.08002 | 0.60799 |
| `fiqa_64_seed407` | `teacher_structural_tail` | 0.41271 | 0.58409 | 0.51852 | 0.35521 | 0.08002 | 0.53883 |
| `fiqa_64_seed407` | `shared_mlp_bm25_a010` | 0.42030 | 0.70530 | 0.49551 | 0.35808 | 0.14552 | 0.41247 |
| `fiqa_64_seed407` | `shared_mlp` | 0.31853 | 0.52869 | 0.39711 | 0.26196 | 0.08002 | 0.31349 |
| `fiqa_192_e4_seed407` | `shared_linear_bm25_a010` | 0.45259 | - | - | - | - | - |
| `fiqa_192_e4_seed407` | `shared_linear` | 0.39733 | - | - | - | - | - |
| `route_score_64_seed407` | `shared_linear_bm25_a010` | 0.44441 | - | - | - | - | - |
| `route_score_64_seed407` | `shared_linear` | 0.39378 | - | - | - | - | - |

The full JSON artifacts contain the omitted per-metric rows for the latter two
runs.

## Dense-Function Metrics

| Run | Variant | Pearson | Top100 overlap | Pairwise agreement |
| --- | --- | ---: | ---: | ---: |
| `fiqa_64_seed407` | `shared_linear` | 0.82224 | 0.60156 | 0.79794 |
| `fiqa_64_seed407` | `shared_mlp` | 0.56217 | 0.22552 | 0.68308 |
| `fiqa_192_e4_seed407` | `shared_linear` | 0.82844 | 0.59677 | 0.80903 |
| `route_score_64_seed407` | `shared_linear` | 0.85001 | 0.62052 | 0.81530 |

## Findings

1. `shared_linear` has a real signal: at 64 groups it beats the deterministic
   structural tail without BM25 (`0.46959` vs `0.41271` NDCG@10).
2. The line does not yet preserve enough dense ability: it remains below exact
   dense (`0.53386`) and below the deterministic structural + BM25 route
   (`0.52556`).
3. More groups and more epochs do not solve it.  Global dense-function metrics
   improve slightly, but NDCG drops.  This means the naive loss learns broad
   correlation while damaging top-k/admission behavior.
4. Route-consistent scoring improves Pearson and Top100 overlap but still drops
   NDCG.  Geometry preservation alone is not sufficient unless the admitted
   posting set is protected.
5. Random-init `shared_mlp` is not viable in this gate.  It underperforms both
   dense and deterministic structural routes.

## Decision

Stop scaling the current random-init M407 line.  The next version should keep
the qrels-free dense distillation goal, but change the model and loss:

- initialize coordinates from the deterministic dense-tail route;
- initialize the sketch head from the joint tail projection;
- add an explicit dense-top-k/admission preservation objective;
- use a frozen or low-rate warmup before end-to-end fine-tuning;
- keep BM25 out of the learned objective and use it only as a post-training
  evaluation/fusion surface.

This is the first credible route toward a direct search-optimized posting
encoder without dataset-specific qrels tuning.

## Checks

- Local `py_compile` passed for
  `scripts/research_sae_m407_dense_function_posting_encoder.py`.
- Local whitespace check passed via `git diff --check --no-index`.
- Remote `py_compile` passed on `spark-2`.
- No residual `ii42_m407_*` tmux session or M407 Python process remained after
  the three sanity runs.
