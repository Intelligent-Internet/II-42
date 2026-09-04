# M400 Neural Feature Ranker

M400 keeps the M398/M399 learned posting admission surface fixed and
trains a qrels-free neural feature ranker over learned-admit and
BM25-union candidates. Dense teacher scores provide pairwise order
and score anchors; qrels are used only for held-out evaluation.

## Config

- Shared root: `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- Tasks: `FiQA2018`
- Posting dims: `256`
- Active dims: `128`
- Encoder epochs: `24`
- Ranker epochs: `16`
- Ranker hidden dims: `96`
- BM25 alphas: `0.1, 0.18`

## Interpretation

M400 is a useful negative result. The qrels-free neural feature ranker trains
cleanly, but it does not beat the M399 ridge calibrator on the same seed398
FiQA heldout split. The best learned-admit + BM25 union neural ranker reaches
`0.45962` NDCG@10, below the ridge union `0.47175`, PCA+BM25 `0.47491`, and
the learned-admit dense-rerank upper bound `0.48140`.

The admission surface is still strong: learned-admit dense rerank keeps
`0.85395` Dense O@100 at only `0.08002` touched ratio. The remaining gap is
ranking/fusion, but a shallow feature MLP trained from per-query dense-score
pairs is not enough. The next viable direction should move away from
hand-built feature reranking and toward query-local/listwise teacher
distillation over the candidate graph, or a model that directly predicts a
dense-like score from richer query-document interactions before compression.

## Macro Mean

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `exact_dense_teacher` | 0.50335 | 1 |
| `m400_learned_admit_dense_rerank_upper` | 0.48140 | 1 |
| `m400_pca_doc_coord_bm25_zblend_a010` | 0.47491 | 1 |
| `m400_learned_admit_bm25_union_ridge` | 0.47175 | 1 |
| `m400_pca_doc_coord_bm25_zblend_a018` | 0.47132 | 1 |
| `m400_learned_admit_bm25_zblend_a018` | 0.46781 | 1 |
| `m400_learned_admit_bm25_zblend_a010` | 0.46231 | 1 |
| `m400_learned_admit_bm25_union_neural_ranker` | 0.45962 | 1 |
| `m400_learned_admit_ridge_dense_teacher` | 0.45215 | 1 |
| `m400_learned_admit_neural_ranker` | 0.44073 | 1 |
| `m400_learned_posting_score` | 0.42356 | 1 |
| `m400_pca_doc_coord` | 0.39620 | 1 |

## Unit Rows

| Task | Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `FiQA2018` | `exact_dense_teacher` | 0.50335 | 1.00000 | 1.00000 | 0.81999 | 0.57717 | 0.44694 |
| `FiQA2018` | `m400_learned_admit_bm25_union_neural_ranker` | 0.45962 | 0.14219 | 0.78741 | 0.80136 | 0.52836 | 0.40271 |
| `FiQA2018` | `m400_learned_admit_bm25_union_ridge` | 0.47175 | 0.14219 | 0.80074 | 0.80887 | 0.53524 | 0.41643 |
| `FiQA2018` | `m400_learned_admit_bm25_zblend_a010` | 0.46231 | 0.14219 | 0.56216 | 0.75683 | 0.52906 | 0.40232 |
| `FiQA2018` | `m400_learned_admit_bm25_zblend_a018` | 0.46781 | 0.14219 | 0.55787 | 0.75815 | 0.53620 | 0.40776 |
| `FiQA2018` | `m400_learned_admit_dense_rerank_upper` | 0.48140 | 0.08002 | 0.85395 | 0.76525 | 0.56656 | 0.42454 |
| `FiQA2018` | `m400_learned_admit_neural_ranker` | 0.44073 | 0.08002 | 0.73642 | 0.75185 | 0.52656 | 0.38737 |
| `FiQA2018` | `m400_learned_admit_ridge_dense_teacher` | 0.45215 | 0.08002 | 0.74948 | 0.75593 | 0.52697 | 0.39357 |
| `FiQA2018` | `m400_learned_posting_score` | 0.42356 | 0.08002 | 0.51938 | 0.70489 | 0.50004 | 0.36237 |
| `FiQA2018` | `m400_pca_doc_coord` | 0.39620 | 0.08002 | 0.51920 | 0.60573 | 0.49216 | 0.34308 |
| `FiQA2018` | `m400_pca_doc_coord_bm25_zblend_a010` | 0.47491 | 0.14499 | 0.73636 | 0.79642 | 0.54914 | 0.41812 |
| `FiQA2018` | `m400_pca_doc_coord_bm25_zblend_a018` | 0.47132 | 0.14499 | 0.70130 | 0.78466 | 0.54996 | 0.41292 |

## Training

### `FiQA2018`
- encoder losses: `[1.93057, 1.629503, 1.572115, 1.529533, 1.512083, 1.49969, 1.460914, 1.522087, 1.491314, 1.480925, 1.413055, 1.399272, 1.415904, 1.464933, 1.464568, 1.438838, 1.451846, 1.442847, 1.432353, 1.413036, 1.475102, 1.435769, 1.446087, 1.426424]`
- ridge calibration: `{'examples': 2651993, 'feature_count': 11, 'ridge': 0.01}`
- ranker device `cuda`, features `14`, points `663552`, pairs `165888`
- ranker losses: `[0.226913, 0.040557, 0.034222, 0.032274, 0.030599, 0.029858, 0.029249, 0.02836, 0.027735, 0.027348, 0.026884, 0.026527, 0.025986, 0.025548, 0.02544, 0.02499]`
- ranker pair losses: `[0.116579, 0.01509, 0.013913, 0.013847, 0.013494, 0.013409, 0.013413, 0.013405, 0.013368, 0.013331, 0.013366, 0.013443, 0.013457, 0.013497, 0.013489, 0.013501]`
- ranker anchor losses: `[0.315242, 0.072763, 0.058026, 0.052648, 0.048873, 0.046997, 0.045248, 0.042727, 0.041049, 0.040048, 0.038624, 0.037385, 0.035796, 0.034432, 0.034145, 0.032826]`
