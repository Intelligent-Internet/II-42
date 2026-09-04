# II-42 M397 Dense-Distilled Posting Encoder Report

## Summary

M397 starts the new search-optimized posting encoder line.

This is not a traditional SAE. The model is a dense-teacher-distilled
posting encoder: it maps dense embeddings into signed sparse posting
coordinates and is trained to preserve dense teacher scores. Qrels are used
only for final held-out evaluation.

## Smoke Run

Remote run:

- host: `spark-2`
- mode: CPU-only (`CUDA_VISIBLE_DEVICES=`)
- run root:
  `/home/huoju/leask/runs/ii42-m397-dense-distilled-posting-encoder-v1/fiqa_smoke`
- log:
  `/home/huoju/leask/logs/ii42_m397_fiqa_smoke.log`
- task: `FiQA2018`
- train/eval split: `324` train queries, `324` eval queries
- training examples: `46656`
- epochs: `2`
- posting dims: `256`
- active dims: `128`
- candidate budget: `0.08`
- BM25 posthoc alphas: `0.10`, `0.18`

## Result Matrix

| Source | NDCG@10 | Touch | Dense O@100 |
| --- | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.50353 | 1.00000 | 1.00000 |
| `m397_pca_doc_coord_bm25_zblend_a018` | 0.48604 | 0.14516 | 0.69259 |
| `m397_pca_doc_coord_bm25_zblend_a010` | 0.48515 | 0.14516 | 0.72812 |
| `m397_shared_dense_distilled_posting_bm25_zblend_a010` | 0.48068 | 0.14231 | 0.61485 |
| `m397_shared_dense_distilled_posting_bm25_zblend_a018` | 0.47676 | 0.14231 | 0.60025 |
| `m397_shared_dense_distilled_posting` | 0.41272 | 0.08002 | 0.53515 |
| `m397_pca_doc_coord` | 0.39678 | 0.08002 | 0.50923 |
| `m397_dual_dense_distilled_posting_bm25_zblend_a018` | 0.25082 | 0.14730 | 0.28244 |
| `m397_dual_dense_distilled_posting_bm25_zblend_a010` | 0.19227 | 0.14730 | 0.26204 |
| `m397_dual_dense_distilled_posting` | 0.04477 | 0.08002 | 0.14012 |

Training losses:

| Variant | Device | Examples | Losses |
| --- | --- | ---: | --- |
| `shared` | `cpu` | 46656 | `0.004182 -> 0.001997` |
| `dual` | `cpu` | 46656 | `0.055878 -> 0.010841` |

## Interpretation

The first canary is positive for the new encoder direction:

- The `shared` dense-distilled posting encoder beats deterministic
  `pca_doc_coord` without BM25: `0.41272` vs `0.39678`.
- Dense overlap also improves: `0.53515` vs `0.50923`.
- Touch ratio stays the same: `0.08002`.

This means the learned encoder is already learning a better posting shape than
the deterministic PCA coordinate baseline on this held-out FiQA split.

The `dual` variant fails badly. A separate query head and document head can
drive regression loss down while losing posting-space alignment. This strongly
suggests that early models need shared or tightly tied query/document geometry.

BM25 posthoc is mixed:

- PCA + BM25 is still best in this smoke: `0.48604`.
- Shared learned posting + BM25 reaches `0.48068`.
- The learned posting encoder helps BM25-free retrieval, but the learned score
  is not yet calibrated enough for posthoc BM25 fusion.

## Decision

Continue this line. The key proof surface is BM25-free posting quality, and the
first canary already improves that surface.

Do not scale the current dual-head architecture. It needs an alignment
constraint or a tied-head design before more compute.

## M397B Shared Stronger Canary

Remote run:

- host: `spark-2`
- mode: CPU-only (`CUDA_VISIBLE_DEVICES=`)
- run root:
  `/home/huoju/leask/runs/ii42-m397-dense-distilled-posting-encoder-v1/fiqa_shared_e6`
- log:
  `/home/huoju/leask/logs/ii42_m397b_fiqa_shared.log`
- task: `FiQA2018`
- variants: `shared`
- epochs: `6`
- training examples: `62208`
- teacher top-k: `64`
- random negatives: `128`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.50353 | 1.00000 | 1.00000 | 0.83239 | 0.58672 | 0.44449 |
| `m397_pca_doc_coord_bm25_zblend_a018` | 0.48604 | 0.14516 | 0.69259 | 0.80077 | 0.58113 | 0.42675 |
| `m397_pca_doc_coord_bm25_zblend_a010` | 0.48515 | 0.14516 | 0.72812 | 0.80621 | 0.57626 | 0.42563 |
| `m397_shared_dense_distilled_posting_bm25_zblend_a018` | 0.48273 | 0.14190 | 0.62525 | 0.77925 | 0.56340 | 0.42336 |
| `m397_shared_dense_distilled_posting_bm25_zblend_a010` | 0.47958 | 0.14190 | 0.64414 | 0.78494 | 0.56413 | 0.42119 |
| `m397_shared_dense_distilled_posting` | 0.41340 | 0.08002 | 0.56744 | 0.67316 | 0.49914 | 0.35609 |
| `m397_pca_doc_coord` | 0.39678 | 0.08002 | 0.50923 | 0.58866 | 0.50174 | 0.33777 |

Training loss:

| Variant | Device | Examples | Losses |
| --- | --- | ---: | --- |
| `shared` | `cpu` | 62208 | `0.003993 -> 0.001775 -> 0.001099 -> 0.000801 -> 0.000598 -> 0.000468` |

M397B confirms the direction but also exposes the next bottleneck.

- BM25-free learned posting improves from `0.41272` to `0.41340`.
- Dense overlap improves from `0.53515` to `0.56744`.
- Posthoc BM25 improves from `0.48068` to `0.48273`, but still trails
  deterministic PCA + BM25 at `0.48604`.

This says the learned shared geometry is better than deterministic PCA at the
same `0.08002` touch ratio, but pure score-regression training is already near a
short-canary plateau. The next experiment should change the training objective
or architecture instead of only adding more epochs.

## Next Experiments

M397B should test:

M398 should test:

- shared head with pairwise dense-order loss added to score regression;
- tied dual head: shared base projection plus small query adapter;
- posthoc BM25 calibration using learned-score temperature or rank-normalized
  fusion;
- optional BM25-aware training as a separate proof surface, not as a replacement
  for BM25-free evaluation.

The immediate target is to make the learned encoder beat PCA both BM25-free and
with posthoc BM25 on FiQA, then expand to `ArguAna`, `SCIDOCS`, and one larger
hard-negative task.
