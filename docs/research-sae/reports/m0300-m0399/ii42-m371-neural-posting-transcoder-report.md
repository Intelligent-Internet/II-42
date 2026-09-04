# II-42 M371 Neural Posting Transcoder Report

## Goal

M371 tests whether a small neural scorer over posting-derived features can
replace the M370 logistic admission policy and recover ranking directly from
posting evidence.

This is still a dense-only route:

- no BM25;
- no qrel training;
- labels are exact dense top100 membership;
- qrels are used only for final evaluation;
- the scorer only sees aggregated posting features.

## Command

```bash
python3 scripts/research_sae_m371_neural_posting_transcoder.py
```

Output:

- `/tmp/ii42-m371-neural-posting-transcoder/m371_neural_transcoder.json`
- `/tmp/ii42-m371-neural-posting-transcoder/m371_neural_transcoder.md`

Runtime: 128.51 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `128`
- Budgets: `0.10,0.20,0.35,0.50,0.80`
- Train examples: 500000
- Positive rate: 0.2500
- Losses: `[0.49957275006078905, 0.33506241104295176, 0.3237006371059725, 0.315762308816756, 0.31067690733940373]`

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Frontier

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense-rerank NDCG@10 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.10 | none | | | | | | | |
| 0.20 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.5984 | 0.7761 | 0.1000 |
| 0.35 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.5984 | 0.7761 | 0.1000 |
| 0.50 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.5984 | 0.7761 | 0.1000 |
| 0.80 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.5984 | 0.7761 | 0.1000 |
| 1.00 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.5984 | 0.7761 | 0.1000 |

## Macro Rows

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |
| neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7761 | 0.5984 | 0.1000 |
| neural_raw_k128_budget0.20 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7760 | 0.5984 | 0.2000 |
| neural_raw_k128_budget0.35 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7760 | 0.5984 | 0.3500 |
| neural_raw_k128_budget0.50 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7760 | 0.5984 | 0.5000 |
| neural_raw_k128_budget0.80 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7760 | 0.5984 | 0.8000 |
| neural_pca_doc_k128_budget0.10 | 0.8420 | 0.7818 | 0.6843 | 0.6061 | 0.7762 | 0.5935 | 0.1000 |
| neural_pca_doc_k128_budget0.20 | 0.8420 | 0.7818 | 0.6843 | 0.6061 | 0.7761 | 0.5935 | 0.2000 |
| neural_pca_doc_k128_budget0.35 | 0.8420 | 0.7818 | 0.6843 | 0.6061 | 0.7761 | 0.5935 | 0.3500 |
| neural_pca_doc_k128_budget0.50 | 0.8420 | 0.7818 | 0.6843 | 0.6061 | 0.7761 | 0.5935 | 0.5000 |
| neural_pca_doc_k128_budget0.80 | 0.8420 | 0.7818 | 0.6843 | 0.6061 | 0.7761 | 0.5935 | 0.7996 |

## Verdict

M371 is not the right training target.

The candidate set is strong: exact dense rerank over the selected candidates
reaches NDCG@10 0.7761, matching dense baseline 0.7759. But the neural BCE
scorer itself only reaches NDCG@10 0.7137. That means dense-top100
classification learns admission, but it does not learn enough ordering.

The failure is useful: the route should not be abandoned. It says the next
model must train directly on dense scores or dense ordering, not only on
membership in dense top100.
