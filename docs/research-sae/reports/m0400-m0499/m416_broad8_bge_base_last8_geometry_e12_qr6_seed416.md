# M415 Shared Geometry Dense Distillation

M415 trains one shared raw-text student encoder across multiple
materialized PPLX dense-teacher task roots.  Qrels, posting, and
BM25 are evaluation-only.

## Config

- Shared root: `/home/huoju/leask/runs/bm25sae-m150-c4-official-beir-df012-v2/all-test`
- Tasks: `nfcorpus, scifact, fiqa, arguana, scidocs, trec-covid, cqadupstack, webis-touche2020`
- Student model: `BAAI/bge-base-en-v1.5`
- Rows: `92912`
- Max docs/task: `8192`
- Max queries/task: `2048`
- Query repeat: `6`
- Epochs: `12`
- Train last layers: `8`
- Loss weights: `{'cosine': 1.0, 'mse': 1.0, 'relational_kl': 0.1, 'relational_mse': 5.0, 'topk_sim': 5.0}`

## Macro Mean

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `m415_teacher_structural_tail_bm25_zblend_a010` | 0.48062 | 8 |
| `exact_dense_teacher` | 0.47747 | 8 |
| `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.43966 | 8 |
| `exact_dense_student` | 0.41661 | 8 |
| `m415_teacher_structural_tail` | 0.39430 | 8 |
| `m415_shared_student_structural_tail` | 0.34232 | 8 |

## Sampled Training Rows

| Task | Docs Used / Seen | Queries Used / Seen |
| --- | ---: | ---: |
| `nfcorpus` | 3633 / 3633 | 323 / 323 |
| `scifact` | 5183 / 5183 | 300 / 300 |
| `fiqa` | 8192 / 57638 | 648 / 648 |
| `arguana` | 8192 / 8674 | 1406 / 1406 |
| `scidocs` | 8192 / 25657 | 1000 / 1000 |
| `trec-covid` | 8192 / 171331 | 50 / 50 |
| `cqadupstack` | 8192 / 457199 | 2048 / 13145 |
| `webis-touche2020` | 8192 / 382545 | 49 / 49 |

## Representation Diagnostics

| Task | Doc Dense Cos | Query Dense Cos | Doc Active Jaccard | Query Active Jaccard |
| --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | 0.81255 | 0.86665 | 0.40556 | 0.50233 |
| `scifact` | 0.79450 | 0.88608 | 0.39596 | 0.52857 |
| `fiqa` | 0.76846 | 0.88375 | 0.37542 | 0.53100 |
| `arguana` | 0.83555 | 0.90959 | 0.46952 | 0.57886 |
| `scidocs` | 0.77110 | 0.87833 | 0.39261 | 0.53191 |
| `trec-covid` | 0.77711 | 0.89206 | 0.37325 | 0.52839 |
| `cqadupstack` | 0.70566 | 0.70006 | 0.35063 | 0.36674 |
| `webis-touche2020` | 0.74409 | 0.88346 | 0.36808 | 0.53537 |

## Training Diagnostics

- Final loss: `0.212581`
- Final cosine loss: `0.195800`
- Final relational KL: `0.000659`
- Final relational MSE: `0.001454`
- Final top-k sim MSE: `0.001812`

## Unit Rows

| Task | Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `exact_dense_student` | 0.41599 | 1.00000 | 1.00000 | 0.99857 | 0.28509 | 0.28643 |
| `arguana` | `exact_dense_teacher` | 0.43823 | 1.00000 | 1.00000 | 1.00000 | 0.30515 | 0.30634 |
| `arguana` | `m415_shared_student_structural_tail` | 0.40469 | 0.08001 | 0.77525 | 0.99715 | 0.27468 | 0.27611 |
| `arguana` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.40739 | 0.13533 | 0.77673 | 0.99857 | 0.27792 | 0.27898 |
| `arguana` | `m415_teacher_structural_tail` | 0.42140 | 0.08001 | 0.94551 | 1.00000 | 0.29000 | 0.29119 |
| `arguana` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.42077 | 0.13545 | 0.92619 | 1.00000 | 0.28844 | 0.28979 |
| `cqadupstack` | `exact_dense_student` | 0.35932 | 1.00000 | 1.00000 | 0.68958 | 0.35502 | 0.32148 |
| `cqadupstack` | `exact_dense_teacher` | 0.44955 | 1.00000 | 1.00000 | 0.78172 | 0.44214 | 0.40804 |
| `cqadupstack` | `m415_shared_student_structural_tail` | 0.14717 | 0.06689 | 0.14635 | 0.22622 | 0.15666 | 0.13044 |
| `cqadupstack` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.38639 | 0.14139 | 0.43032 | 0.69670 | 0.38512 | 0.34811 |
| `cqadupstack` | `m415_teacher_structural_tail` | 0.20348 | 0.06843 | 0.21336 | 0.27706 | 0.21679 | 0.18331 |
| `cqadupstack` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.44754 | 0.14267 | 0.76540 | 0.75649 | 0.44313 | 0.40589 |
| `fiqa` | `exact_dense_student` | 0.41262 | 1.00000 | 1.00000 | 0.74432 | 0.47213 | 0.34993 |
| `fiqa` | `exact_dense_teacher` | 0.51729 | 1.00000 | 1.00000 | 0.80992 | 0.58505 | 0.45903 |
| `fiqa` | `m415_shared_student_structural_tail` | 0.30398 | 0.08002 | 0.37299 | 0.47617 | 0.37733 | 0.25258 |
| `fiqa` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.43901 | 0.14625 | 0.53920 | 0.73998 | 0.50030 | 0.37376 |
| `fiqa` | `m415_teacher_structural_tail` | 0.42110 | 0.08002 | 0.55086 | 0.60576 | 0.49078 | 0.36001 |
| `fiqa` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.51898 | 0.14527 | 0.80012 | 0.79637 | 0.58785 | 0.45753 |
| `nfcorpus` | `exact_dense_student` | 0.32520 | 1.00000 | 1.00000 | 0.33140 | 0.52139 | 0.14790 |
| `nfcorpus` | `exact_dense_teacher` | 0.34715 | 1.00000 | 1.00000 | 0.32377 | 0.57103 | 0.16645 |
| `nfcorpus` | `m415_shared_student_structural_tail` | 0.31719 | 0.08010 | 0.60975 | 0.33457 | 0.50543 | 0.14352 |
| `nfcorpus` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.34702 | 0.14114 | 0.61478 | 0.33661 | 0.55403 | 0.16639 |
| `nfcorpus` | `m415_teacher_structural_tail` | 0.34254 | 0.08010 | 0.88578 | 0.32896 | 0.55723 | 0.16151 |
| `nfcorpus` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.35769 | 0.14129 | 0.87615 | 0.33451 | 0.57383 | 0.17516 |
| `scidocs` | `exact_dense_student` | 0.20094 | 1.00000 | 1.00000 | 0.46437 | 0.33880 | 0.13995 |
| `scidocs` | `exact_dense_teacher` | 0.22416 | 1.00000 | 1.00000 | 0.47957 | 0.38227 | 0.15560 |
| `scidocs` | `m415_shared_student_structural_tail` | 0.19698 | 0.08002 | 0.63524 | 0.45427 | 0.33552 | 0.13565 |
| `scidocs` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.20338 | 0.13541 | 0.67986 | 0.46457 | 0.34559 | 0.14206 |
| `scidocs` | `m415_teacher_structural_tail` | 0.21161 | 0.08002 | 0.83766 | 0.46417 | 0.36140 | 0.14589 |
| `scidocs` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.21720 | 0.13549 | 0.86776 | 0.47497 | 0.36906 | 0.15037 |
| `scifact` | `exact_dense_student` | 0.65743 | 1.00000 | 1.00000 | 0.94667 | 0.62049 | 0.61103 |
| `scifact` | `exact_dense_teacher` | 0.74180 | 1.00000 | 1.00000 | 0.96667 | 0.71179 | 0.69886 |
| `scifact` | `m415_shared_student_structural_tail` | 0.65597 | 0.08007 | 0.64667 | 0.94000 | 0.61860 | 0.60941 |
| `scifact` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.69239 | 0.13443 | 0.65340 | 0.96000 | 0.65132 | 0.64481 |
| `scifact` | `m415_teacher_structural_tail` | 0.71886 | 0.08007 | 0.90020 | 0.96667 | 0.69077 | 0.67774 |
| `scifact` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.73961 | 0.13487 | 0.88173 | 0.98000 | 0.70600 | 0.69484 |
| `trec-covid` | `exact_dense_student` | 0.71483 | 1.00000 | 1.00000 | 0.14601 | 0.88364 | 0.11167 |
| `trec-covid` | `exact_dense_teacher` | 0.82949 | 1.00000 | 1.00000 | 0.17039 | 1.00000 | 0.13996 |
| `trec-covid` | `m415_shared_student_structural_tail` | 0.57322 | 0.08000 | 0.18440 | 0.07620 | 0.84444 | 0.05094 |
| `trec-covid` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.74782 | 0.15305 | 0.61440 | 0.15672 | 0.88133 | 0.12362 |
| `trec-covid` | `m415_teacher_structural_tail` | 0.66936 | 0.08000 | 0.22240 | 0.08841 | 0.93619 | 0.06162 |
| `trec-covid` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.83652 | 0.15257 | 0.80040 | 0.16739 | 0.96000 | 0.13891 |
| `webis-touche2020` | `exact_dense_student` | 0.24658 | 1.00000 | 1.00000 | 0.47341 | 0.47370 | 0.15797 |
| `webis-touche2020` | `exact_dense_teacher` | 0.27213 | 1.00000 | 1.00000 | 0.50452 | 0.53050 | 0.16852 |
| `webis-touche2020` | `m415_shared_student_structural_tail` | 0.13940 | 0.07773 | 0.27520 | 0.23038 | 0.29421 | 0.07369 |
| `webis-touche2020` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.29387 | 0.15230 | 0.66360 | 0.52301 | 0.50085 | 0.19221 |
| `webis-touche2020` | `m415_teacher_structural_tail` | 0.16605 | 0.07967 | 0.29800 | 0.23888 | 0.38641 | 0.07891 |
| `webis-touche2020` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.30661 | 0.15406 | 0.84920 | 0.55226 | 0.56067 | 0.19951 |
