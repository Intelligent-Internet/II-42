# II-42 M399 Separated Admission and Ranking Report

## Summary

M399 tested the next hypothesis after M398: keep the learned posting encoder as
an admission mechanism, then evaluate ranking and BM25 fusion separately.

The result is positive and diagnostic:

- M398's best posting score is reproducible on the same seed/split:
  `0.42356`.
- Dense reranking inside the learned admitted set reaches `0.48140`, higher than
  the PCA + BM25 baseline at `0.47491`.
- A qrels-free dense-teacher calibrator improves ranking from `0.42356` to
  `0.45215` on admitted candidates.
- A qrels-free BM25-union calibrator reaches `0.47175`, close to PCA + BM25 but
  still below it.

This means the learned admission surface has enough signal. The remaining gap
is mostly ranking/fusion, not candidate coverage.

## Apples-to-Apples Seed 398 Matrix

Run:

- host: `spark-2`
- output:
  `/home/huoju/leask/runs/ii42-m399-separated-admission-ranking-v1/fiqa_seed398_e24`
- seed: `398`
- task: `FiQA2018`
- encoder: M398 best shape, shared linear, e24
- feature mode: `base`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.50335 | 1.00000 | 1.00000 | 0.81999 | 0.57717 | 0.44694 |
| `m399_learned_admit_dense_rerank_upper` | 0.48140 | 0.08002 | 0.85395 | 0.76525 | 0.56656 | 0.42454 |
| `m399_pca_doc_coord_bm25_zblend_a010` | 0.47491 | 0.14499 | 0.73636 | 0.79642 | 0.54914 | 0.41812 |
| `m399_learned_admit_bm25_union_calibrated` | 0.47175 | 0.14219 | 0.80074 | 0.80887 | 0.53524 | 0.41643 |
| `m399_pca_doc_coord_bm25_zblend_a018` | 0.47132 | 0.14499 | 0.70130 | 0.78466 | 0.54996 | 0.41292 |
| `m399_learned_admit_bm25_zblend_a018` | 0.46781 | 0.14219 | 0.55787 | 0.75815 | 0.53620 | 0.40776 |
| `m399_learned_admit_bm25_zblend_a010` | 0.46231 | 0.14219 | 0.56216 | 0.75683 | 0.52906 | 0.40232 |
| `m399_learned_admit_calibrated_dense_teacher` | 0.45215 | 0.08002 | 0.74948 | 0.75593 | 0.52697 | 0.39357 |
| `m399_learned_posting_score` | 0.42356 | 0.08002 | 0.51938 | 0.70489 | 0.50004 | 0.36237 |
| `m399_pca_doc_coord` | 0.39620 | 0.08002 | 0.51920 | 0.60573 | 0.49216 | 0.34308 |

## Seed 399 Sanity

Run:

- output:
  `/home/huoju/leask/runs/ii42-m399-separated-admission-ranking-v1/fiqa_e24`
- seed: `399`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.49908 | 1.00000 | 1.00000 | 0.81337 | 0.58372 | 0.44301 |
| `m399_learned_admit_dense_rerank_upper` | 0.48415 | 0.08002 | 0.85241 | 0.76797 | 0.57627 | 0.42566 |
| `m399_pca_doc_coord_bm25_zblend_a010` | 0.48369 | 0.14517 | 0.73593 | 0.79117 | 0.56511 | 0.42785 |
| `m399_learned_admit_bm25_union_calibrated` | 0.47255 | 0.14241 | 0.80278 | 0.80812 | 0.54298 | 0.41475 |
| `m399_learned_admit_calibrated_dense_teacher` | 0.46282 | 0.08002 | 0.75182 | 0.76606 | 0.54345 | 0.40323 |
| `m399_learned_posting_score` | 0.43540 | 0.08002 | 0.51701 | 0.70949 | 0.51913 | 0.37822 |
| `m399_pca_doc_coord` | 0.40757 | 0.08002 | 0.52235 | 0.60176 | 0.51377 | 0.35636 |

The same shape holds under a different split: dense-rerank upper is extremely
strong, and calibration closes part of the ranking gap.

## Negative Control

M399B tried polynomial/interaction features for the ridge calibrator:

- output:
  `/home/huoju/leask/runs/ii42-m399-separated-admission-ranking-v1/fiqa_seed398_poly_e24`
- feature mode: `poly`
- best union calibrated score: `0.46584`

This is worse than the base feature mode's `0.47175`. The added interactions
overfit or distort the dense-teacher scoring surface, so base features stay the
default.

## Interpretation

M399 answers the key question from M398: the learned posting encoder is not only
a better scorer; it is also a strong admission mechanism.

On seed 398, exact dense reranking inside the learned admitted candidates reaches
`0.48140`, above the PCA + BM25 comparator. That means the candidate set has
enough relevant material. The deployed ranking surface fails to extract all of
that value:

- raw posting score: `0.42356`
- qrels-free calibrated admitted ranking: `0.45215`
- qrels-free calibrated BM25 union: `0.47175`
- dense-rerank upper: `0.48140`

The next quality gap is the ranker/fusion model, not the admission encoder.

## Decision

Continue this route.

Next stage should not make the posting encoder deeper. It should train a
qrels-free neural feature ranker over admitted/union candidates:

- input: learned posting score/rank, PCA score/rank, BM25 score/rank, candidate
  source flags, and query-shape features;
- target: dense-teacher order and dense-teacher score;
- loss: pairwise/listwise dense-order loss plus small score anchor;
- eval gates: BM25-free, learned-admit dense-rerank upper, calibrated union,
  PCA+BM25 comparator.

Only after the qrels-free ranker approaches the dense-rerank upper should BM25
be considered inside encoder training.
