# M415 Shared Geometry Dense Distillation

M415 trains one shared raw-text student encoder across multiple
materialized PPLX dense-teacher task roots.  Qrels, posting, and
BM25 are evaluation-only.

## Config

- Shared root: `/home/huoju/leask/runs/bm25sae-m150-c4-official-beir-df012-v2/all-test`
- Tasks: `nfcorpus, scifact, fiqa, arguana, scidocs, trec-covid, cqadupstack, webis-touche2020`
- Student model: `BAAI/bge-base-en-v1.5`
- Rows: `81264`
- Max docs/task: `8192`
- Max queries/task: `2048`
- Query repeat: `4`
- Epochs: `8`
- Train last layers: `4`
- Loss weights: `{'cosine': 1.0, 'mse': 1.0, 'relational_kl': 0.1, 'relational_mse': 5.0, 'topk_sim': 5.0}`

## Macro Mean

| Source | NDCG@10 | Tasks |
| --- | ---: | ---: |
| `m415_teacher_structural_tail_bm25_zblend_a010` | 0.48101 | 8 |
| `exact_dense_teacher` | 0.47718 | 8 |
| `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.44510 | 8 |
| `exact_dense_student` | 0.41677 | 8 |
| `m415_teacher_structural_tail` | 0.40756 | 8 |
| `m415_shared_student_structural_tail` | 0.35041 | 8 |

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
| `nfcorpus` | 0.79651 | 0.78898 | 0.38684 | 0.41072 |
| `scifact` | 0.77640 | 0.83068 | 0.37854 | 0.44759 |
| `fiqa` | 0.74976 | 0.83150 | 0.36076 | 0.45703 |
| `arguana` | 0.81719 | 0.87341 | 0.44616 | 0.51539 |
| `scidocs` | 0.75100 | 0.81744 | 0.37434 | 0.45063 |
| `trec-covid` | 0.76198 | 0.85547 | 0.35794 | 0.45868 |
| `cqadupstack` | 0.68636 | 0.68059 | 0.33779 | 0.34696 |
| `webis-touche2020` | 0.72728 | 0.84438 | 0.35242 | 0.48612 |

## Training Diagnostics

- Final loss: `0.256520`
- Final cosine loss: `0.233939`
- Final relational KL: `0.000914`
- Final relational MSE: `0.001878`
- Final top-k sim MSE: `0.002528`

## Unit Rows

| Task | Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `exact_dense_student` | 0.42184 | 1.00000 | 1.00000 | 0.99714 | 0.29341 | 0.29463 |
| `arguana` | `exact_dense_teacher` | 0.44279 | 1.00000 | 1.00000 | 1.00000 | 0.30875 | 0.30959 |
| `arguana` | `m415_shared_student_structural_tail` | 0.41020 | 0.08001 | 0.73720 | 0.99714 | 0.28384 | 0.28522 |
| `arguana` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.41520 | 0.13503 | 0.73981 | 0.99857 | 0.28543 | 0.28697 |
| `arguana` | `m415_teacher_structural_tail` | 0.42589 | 0.08001 | 0.94699 | 1.00000 | 0.28858 | 0.28953 |
| `arguana` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.42460 | 0.13537 | 0.92650 | 1.00000 | 0.28945 | 0.29032 |
| `cqadupstack` | `exact_dense_student` | 0.34249 | 1.00000 | 1.00000 | 0.67921 | 0.33975 | 0.30580 |
| `cqadupstack` | `exact_dense_teacher` | 0.44527 | 1.00000 | 1.00000 | 0.77848 | 0.43825 | 0.40180 |
| `cqadupstack` | `m415_shared_student_structural_tail` | 0.13576 | 0.06703 | 0.13878 | 0.21524 | 0.14680 | 0.11989 |
| `cqadupstack` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.37510 | 0.14144 | 0.40952 | 0.68426 | 0.37538 | 0.33764 |
| `cqadupstack` | `m415_teacher_structural_tail` | 0.20117 | 0.06843 | 0.21208 | 0.27807 | 0.21663 | 0.18009 |
| `cqadupstack` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.44098 | 0.14267 | 0.76491 | 0.74972 | 0.43862 | 0.39889 |
| `fiqa` | `exact_dense_student` | 0.38147 | 1.00000 | 1.00000 | 0.72855 | 0.46910 | 0.31594 |
| `fiqa` | `exact_dense_teacher` | 0.51242 | 1.00000 | 1.00000 | 0.82853 | 0.60716 | 0.45124 |
| `fiqa` | `m415_shared_student_structural_tail` | 0.27885 | 0.08002 | 0.35065 | 0.47950 | 0.36500 | 0.22979 |
| `fiqa` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.40610 | 0.14559 | 0.50154 | 0.72926 | 0.49580 | 0.33952 |
| `fiqa` | `m415_teacher_structural_tail` | 0.40539 | 0.08002 | 0.55494 | 0.57951 | 0.50646 | 0.34544 |
| `fiqa` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.51225 | 0.14514 | 0.80278 | 0.79892 | 0.60812 | 0.44821 |
| `nfcorpus` | `exact_dense_student` | 0.33518 | 1.00000 | 1.00000 | 0.32680 | 0.51802 | 0.14905 |
| `nfcorpus` | `exact_dense_teacher` | 0.35587 | 1.00000 | 1.00000 | 0.31657 | 0.58179 | 0.16128 |
| `nfcorpus` | `m415_shared_student_structural_tail` | 0.32593 | 0.08010 | 0.54143 | 0.33331 | 0.51751 | 0.14608 |
| `nfcorpus` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.35313 | 0.14140 | 0.54677 | 0.34539 | 0.56799 | 0.16314 |
| `nfcorpus` | `m415_teacher_structural_tail` | 0.34726 | 0.08010 | 0.87789 | 0.31655 | 0.56055 | 0.15622 |
| `nfcorpus` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.37132 | 0.14179 | 0.87149 | 0.32480 | 0.60519 | 0.17255 |
| `scidocs` | `exact_dense_student` | 0.21173 | 1.00000 | 1.00000 | 0.47760 | 0.36031 | 0.14787 |
| `scidocs` | `exact_dense_teacher` | 0.23454 | 1.00000 | 1.00000 | 0.49203 | 0.39788 | 0.16543 |
| `scidocs` | `m415_shared_student_structural_tail` | 0.20506 | 0.08002 | 0.59978 | 0.45777 | 0.35103 | 0.14278 |
| `scidocs` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.21516 | 0.13520 | 0.64392 | 0.47397 | 0.35539 | 0.15045 |
| `scidocs` | `m415_teacher_structural_tail` | 0.22638 | 0.08002 | 0.84028 | 0.48120 | 0.38914 | 0.15840 |
| `scidocs` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.22946 | 0.13559 | 0.87212 | 0.49270 | 0.38893 | 0.16129 |
| `scifact` | `exact_dense_student` | 0.68638 | 1.00000 | 1.00000 | 0.94000 | 0.64925 | 0.63687 |
| `scifact` | `exact_dense_teacher` | 0.76471 | 1.00000 | 1.00000 | 0.96667 | 0.74346 | 0.72818 |
| `scifact` | `m415_shared_student_structural_tail` | 0.68023 | 0.08007 | 0.62160 | 0.94000 | 0.64159 | 0.62746 |
| `scifact` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.71630 | 0.13415 | 0.63333 | 0.96000 | 0.68259 | 0.67005 |
| `scifact` | `m415_teacher_structural_tail` | 0.74999 | 0.08007 | 0.90707 | 0.96000 | 0.72454 | 0.70799 |
| `scifact` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.76425 | 0.13461 | 0.88993 | 0.96667 | 0.73844 | 0.72323 |
| `trec-covid` | `exact_dense_student` | 0.78297 | 1.00000 | 1.00000 | 0.14987 | 0.94000 | 0.11624 |
| `trec-covid` | `exact_dense_teacher` | 0.83933 | 1.00000 | 1.00000 | 0.16795 | 0.94000 | 0.13876 |
| `trec-covid` | `m415_shared_student_structural_tail` | 0.66554 | 0.08000 | 0.16280 | 0.07662 | 0.90000 | 0.04899 |
| `trec-covid` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.85159 | 0.15261 | 0.63240 | 0.15905 | 0.98000 | 0.13266 |
| `trec-covid` | `m415_teacher_structural_tail` | 0.76352 | 0.08000 | 0.20320 | 0.09035 | 0.96000 | 0.06184 |
| `trec-covid` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.85919 | 0.15222 | 0.81680 | 0.17130 | 0.96000 | 0.14589 |
| `webis-touche2020` | `exact_dense_student` | 0.17212 | 1.00000 | 1.00000 | 0.43721 | 0.35879 | 0.11460 |
| `webis-touche2020` | `exact_dense_teacher` | 0.22251 | 1.00000 | 1.00000 | 0.46106 | 0.41683 | 0.12889 |
| `webis-touche2020` | `m415_shared_student_structural_tail` | 0.10169 | 0.07791 | 0.26360 | 0.21385 | 0.30565 | 0.04452 |
| `webis-touche2020` | `m415_shared_student_structural_tail_bm25_zblend_a010` | 0.22822 | 0.15223 | 0.67560 | 0.49436 | 0.40910 | 0.15119 |
| `webis-touche2020` | `m415_teacher_structural_tail` | 0.14087 | 0.07971 | 0.27880 | 0.23177 | 0.30235 | 0.06085 |
| `webis-touche2020` | `m415_teacher_structural_tail_bm25_zblend_a010` | 0.24599 | 0.15404 | 0.86320 | 0.51087 | 0.49338 | 0.15305 |
