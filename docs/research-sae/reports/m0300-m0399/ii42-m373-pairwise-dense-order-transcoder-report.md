# II-42 M373 Pairwise Dense-Order Transcoder Report

## Goal

M373 tests whether the remaining M372 ranking gap can be closed by training the
same posting-only neural scorer on sampled dense-order pairs.

The model uses:

- the same M371/M372 posting-derived features;
- pairwise dense-order loss;
- a small dense-score anchor to prevent score drift;
- no BM25;
- no qrel training.

## Command

```bash
python3 scripts/research_sae_m373_pairwise_dense_order_transcoder.py
```

Output:

- `/tmp/ii42-m373-pairwise-dense-order-transcoder/m373_pairwise_dense_order.json`
- `/tmp/ii42-m373-pairwise-dense-order-transcoder/m373_pairwise_dense_order.md`

Runtime: 142.09 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `128`
- Budgets: `0.10,0.20,0.35,0.50,0.80`
- Train pairs: 343552
- Mean pair weight: 0.9496
- Losses: `[0.33795942508039023, 0.2199225521513394, 0.20092430178608214, 0.192522641093958, 0.18809487493265242, 0.1856788382643745]`
- Pair losses: `[0.301076251126471, 0.18390246622619175, 0.16105620811382929, 0.15191868444283804, 0.14692884470735276, 0.14416428619907015]`
- Anchor losses: `[0.46103969571136294, 0.45025109889961423, 0.4983511723223187, 0.5075494576068151, 0.5145753976844606, 0.5189319111052013]`

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Frontier

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense-rerank NDCG@10 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.10 | none | | | | | | | |
| 0.20 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7478 | 0.7761 | 0.1000 |
| 0.35 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7478 | 0.7761 | 0.1000 |
| 0.50 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7478 | 0.7761 | 0.1000 |
| 0.80 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7478 | 0.7761 | 0.1000 |
| 1.00 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7478 | 0.7761 | 0.1000 |

## Verdict

M373 does not beat M372.

It slightly improves recall over M372 at 10% touch, but loses ranking quality:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| M372 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.1000 |
| M373 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.1000 |

The simple pairwise objective is therefore not the immediate breakthrough. It
probably over-pressures relative order while weakening dense-score calibration.
M372 remains the cleaner baseline.
