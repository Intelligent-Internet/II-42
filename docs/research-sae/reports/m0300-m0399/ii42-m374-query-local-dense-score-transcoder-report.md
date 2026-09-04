# II-42 M374 Query-Local Dense-Score Transcoder Report

## Goal

M374 keeps the M372 dense-score regression objective, but adds query-local
z-score and rank features over the candidate pool.

This tests whether the remaining gap is caused by missing relative-position
features rather than model capacity.

## Command

```bash
python3 scripts/research_sae_m374_query_local_dense_score_transcoder.py
```

Output:

- `/tmp/ii42-m374-query-local-dense-score-transcoder/m374_query_local_dense_score.json`
- `/tmp/ii42-m374-query-local-dense-score-transcoder/m374_query_local_dense_score.md`

Runtime: 141.35 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `128`
- Budgets: `0.10,0.20,0.35,0.50,0.80`
- Feature count: 33
- Train examples: 500000
- Weighted positive rate: 0.2500
- Losses: `[0.19758654830436553, 0.038252688223315824, 0.028081451602760824, 0.025857275622265953, 0.02452464521892609, 0.02399288472389021]`

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Frontier

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense-rerank NDCG@10 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.10 | none | | | | | | | |
| 0.20 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.7762 | 0.1000 |
| 0.35 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.7762 | 0.1000 |
| 0.50 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.7762 | 0.1000 |
| 0.80 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.7762 | 0.1000 |
| 1.00 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.7762 | 0.1000 |

## Verdict

M374 does not beat M372.

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M372 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7960 | 0.1000 |
| M374 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7896 | 0.1000 |

Query-local calibration is not enough. The next useful feature change should
expose richer posting-list structure or direct coordinate evidence, rather than
only adding local rank/z-score transforms.
