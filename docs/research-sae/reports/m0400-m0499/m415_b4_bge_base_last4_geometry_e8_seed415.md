# M415 Shared Geometry Dense Distillation

M415 trains one shared raw-text student encoder across multiple
materialized PPLX dense-teacher task roots.  Qrels, posting, and
BM25 are evaluation-only.

## Config

- Shared root: `/home/huoju/leask/runs/bm25sae-m150-c4-official-beir-df012-v2/all-test`
- Tasks: `nfcorpus, scifact, fiqa, arguana`
- Student model: `BAAI/bge-base-en-v1.5`
- Rows: `35908`
- Max docs/task: `8192`
- Max queries/task: `2048`
- Query repeat: `4`
- Epochs: `8`
- Train last layers: `4`
- Loss weights: `{'cosine': 1.0, 'mse': 1.0, 'relational_kl': 0.1, 'relational_mse': 5.0, 'topk_sim': 5.0}`

## Macro Mean

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `exact_dense_teacher` | 0.51895 | 4 |
| `m415_teacher_structural_tail_bm25_zblend_a010` | 0.51811 | 4 |
| `m415_teacher_structural_tail` | 0.48213 | 4 |
| `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.48130 | 4 |
| `exact_dense_student` | 0.47053 | 4 |
| `m415_shared_student_structural_tail` | 0.43707 | 4 |

## Sampled Training Rows

| Task | Docs Used / Seen | Queries Used / Seen |
| --- | ---: | ---: |
| `nfcorpus` | 3633 / 3633 | 323 / 323 |
| `scifact` | 5183 / 5183 | 300 / 300 |
| `fiqa` | 8192 / 57638 | 648 / 648 |
| `arguana` | 8192 / 8674 | 1406 / 1406 |

## Representation Diagnostics

| Task | Doc Dense Cos | Query Dense Cos | Doc Active Jaccard | Query Active Jaccard |
| --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | 0.81381 | 0.86290 | 0.41053 | 0.48995 |
| `scifact` | 0.78668 | 0.88039 | 0.39046 | 0.51561 |
| `fiqa` | 0.74259 | 0.87632 | 0.36005 | 0.51141 |
| `arguana` | 0.83059 | 0.90365 | 0.46300 | 0.56186 |

## Training Diagnostics

- Final loss: `0.216831`
- Final cosine loss: `0.194901`
- Final relational KL: `0.000816`
- Final relational MSE: `0.001790`
- Final top-k sim MSE: `0.002503`

## Unit Rows

| Task | Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `exact_dense_student` | 0.43064 | 1.00000 | 1.00000 | 0.99714 | 0.29660 | 0.29792 |
| `arguana` | `exact_dense_teacher` | 0.44279 | 1.00000 | 1.00000 | 1.00000 | 0.30875 | 0.30959 |
| `arguana` | `m415_shared_student_structural_tail` | 0.42031 | 0.08001 | 0.75556 | 0.99714 | 0.28788 | 0.28921 |
| `arguana` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.41891 | 0.13523 | 0.75809 | 0.99857 | 0.28743 | 0.28848 |
| `arguana` | `m415_teacher_structural_tail` | 0.42589 | 0.08001 | 0.94699 | 1.00000 | 0.28858 | 0.28953 |
| `arguana` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.42460 | 0.13537 | 0.92650 | 1.00000 | 0.28945 | 0.29032 |
| `fiqa` | `exact_dense_student` | 0.38290 | 1.00000 | 1.00000 | 0.73192 | 0.46924 | 0.31807 |
| `fiqa` | `exact_dense_teacher` | 0.51242 | 1.00000 | 1.00000 | 0.82853 | 0.60716 | 0.45124 |
| `fiqa` | `m415_shared_student_structural_tail` | 0.27422 | 0.08002 | 0.36244 | 0.47843 | 0.36501 | 0.22190 |
| `fiqa` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.40342 | 0.14588 | 0.51759 | 0.73811 | 0.48711 | 0.33594 |
| `fiqa` | `m415_teacher_structural_tail` | 0.40539 | 0.08002 | 0.55494 | 0.57951 | 0.50646 | 0.34544 |
| `fiqa` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.51225 | 0.14514 | 0.80278 | 0.79892 | 0.60812 | 0.44821 |
| `nfcorpus` | `exact_dense_student` | 0.34772 | 1.00000 | 1.00000 | 0.32818 | 0.53968 | 0.16066 |
| `nfcorpus` | `exact_dense_teacher` | 0.35587 | 1.00000 | 1.00000 | 0.31657 | 0.58179 | 0.16128 |
| `nfcorpus` | `m415_shared_student_structural_tail` | 0.34782 | 0.08010 | 0.58043 | 0.33058 | 0.54539 | 0.15967 |
| `nfcorpus` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.37669 | 0.14161 | 0.58379 | 0.33707 | 0.58446 | 0.17618 |
| `nfcorpus` | `m415_teacher_structural_tail` | 0.34726 | 0.08010 | 0.87789 | 0.31655 | 0.56055 | 0.15622 |
| `nfcorpus` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.37132 | 0.14179 | 0.87149 | 0.32480 | 0.60519 | 0.17255 |
| `scifact` | `exact_dense_student` | 0.72087 | 1.00000 | 1.00000 | 0.95333 | 0.69113 | 0.67674 |
| `scifact` | `exact_dense_teacher` | 0.76471 | 1.00000 | 1.00000 | 0.96667 | 0.74346 | 0.72818 |
| `scifact` | `m415_shared_student_structural_tail` | 0.70591 | 0.08007 | 0.64380 | 0.95333 | 0.66986 | 0.65666 |
| `scifact` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.72616 | 0.13434 | 0.65127 | 0.96667 | 0.69135 | 0.68392 |
| `scifact` | `m415_teacher_structural_tail` | 0.74999 | 0.08007 | 0.90707 | 0.96000 | 0.72454 | 0.70799 |
| `scifact` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.76425 | 0.13461 | 0.88993 | 0.96667 | 0.73844 | 0.72323 |
