# II-42 M372 Neural Dense-Score Transcoder Report

## Goal

M372 keeps the M371 posting-only neural scorer, but changes the target from
dense-top100 membership to exact dense-score regression.

This tests whether the M371 weakness was the architecture or the objective. It
still uses no BM25 and no qrel training.

## Command

```bash
python3 scripts/research_sae_m372_neural_dense_score_transcoder.py
```

Output:

- `/tmp/ii42-m372-neural-dense-score-transcoder/m372_neural_dense_score.json`
- `/tmp/ii42-m372-neural-dense-score-transcoder/m372_neural_dense_score.md`

Runtime: 133.85 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `128`
- Budgets: `0.10,0.20,0.35,0.50,0.80`
- Train examples: 500000
- Weighted positive rate: 0.2500
- Losses: `[0.23939358118561008, 0.06250969410663651, 0.032111643212697195, 0.02655807609159139, 0.02496488094930687, 0.0243177252310899]`

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Frontier

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense-rerank NDCG@10 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.10 | none | | | | | | | |
| 0.20 | neural_pca_doc_k128_budget0.10 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.7761 | 0.1000 |
| 0.35 | neural_pca_doc_k128_budget0.10 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.7761 | 0.1000 |
| 0.50 | neural_pca_doc_k128_budget0.35 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.7761 | 0.3500 |
| 0.80 | neural_pca_doc_k128_budget0.35 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.7761 | 0.3500 |
| 1.00 | neural_pca_doc_k128_budget0.35 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.7761 | 0.3500 |

## Macro Rows

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |
| neural_raw_k128_budget0.10 | 0.8349 | 0.8470 | 0.7449 | 0.6587 | 0.7762 | 0.6532 | 0.1000 |
| neural_raw_k128_budget0.20 | 0.8349 | 0.8470 | 0.7449 | 0.6587 | 0.7761 | 0.6532 | 0.2000 |
| neural_raw_k128_budget0.35 | 0.8349 | 0.8470 | 0.7449 | 0.6587 | 0.7761 | 0.6532 | 0.3500 |
| neural_raw_k128_budget0.50 | 0.8349 | 0.8470 | 0.7449 | 0.6587 | 0.7761 | 0.6532 | 0.5000 |
| neural_raw_k128_budget0.80 | 0.8349 | 0.8470 | 0.7449 | 0.6587 | 0.7761 | 0.6532 | 0.8000 |
| neural_pca_doc_k128_budget0.10 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.1000 |
| neural_pca_doc_k128_budget0.20 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.2000 |
| neural_pca_doc_k128_budget0.35 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.3500 |
| neural_pca_doc_k128_budget0.50 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.5000 |
| neural_pca_doc_k128_budget0.80 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.7996 |

## Comparison

| Model | Best 10% touch NDCG@10 | Best NDCG@10 | Touch at best | Dense gap |
| --- | ---: | ---: | ---: | ---: |
| M370 logistic admission | 0.7542 | 0.7673 | 0.7996 | -0.0086 |
| M371 BCE neural scorer | 0.7137 | 0.7137 | 0.1000 | -0.0622 |
| M372 dense-score neural scorer | 0.7674 | 0.7674 | 0.1000 | -0.0085 |
| Exact dense | 0.7759 | 0.7759 | 1.0000 | 0.0000 |

## Verdict

M372 validates the neural posting transcoder route.

The same architecture that failed with BCE top100 labels becomes competitive
when trained against dense scores. At only 10% touched docs, M372 reaches:

- Recall@100 0.8480 versus dense 0.8533;
- MRR@20 0.8609 versus dense 0.8722;
- NDCG@10 0.7674 versus dense 0.7759;
- MAP@100 0.6854 versus dense 0.6947.

The remaining gap is small enough that the next useful pressure point is not
more candidate admission. It is top-order calibration: train against dense
pairwise/listwise order and, later, test leave-dataset-out generalization.
