# II-42 M370 Learned Posting Admission Report

## Goal

M370 tests the hypothesis that the bottleneck is posting admission/traversal,
not harder vector compression.

The model is deliberately small:

- logistic posting-admission policy;
- labels from exact dense top100;
- qrels only for final evaluation;
- no BM25;
- no qrel training;
- no learned SAE encoder;
- no reranker during primary sparse scoring.

The policy scores shared coordinate posting edges, aggregates evidence per
document, then selects a fixed candidate budget.

## Command

```bash
python3 scripts/research_sae_m370_learned_posting_admission.py
```

Output:

- `/tmp/ii42-m370-learned-posting-admission/m370_learned_admission.json`
- `/tmp/ii42-m370-learned-posting-admission/m370_learned_admission.md`

Runtime: 235.88 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `128`
- Budgets: `0.10,0.20,0.35,0.50,0.80`
- Train examples: 300000
- Positive rate after sampling: 0.2500
- Train query fraction: 0.5

This is a diagnostic training surface, not yet a strict leave-dataset-out
generalization test. The labels are dense teacher labels, not qrels.

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Learned Frontier

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Dense-rerank NDCG@10 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.10 | none | | | | | | | | |
| 0.20 | learned_pca_doc_k128_budget0.10 | 0.7628 | 0.8586 | 0.7542 | 0.6460 | 0.6883 | 0.3894 | 0.7631 | 0.1000 |
| 0.35 | learned_pca_doc_k128_budget0.20 | 0.8006 | 0.8598 | 0.7618 | 0.6649 | 0.7460 | 0.5439 | 0.7693 | 0.2000 |
| 0.50 | learned_pca_doc_k128_budget0.35 | 0.8282 | 0.8616 | 0.7656 | 0.6767 | 0.7803 | 0.6751 | 0.7733 | 0.3500 |
| 0.80 | learned_pca_doc_k128_budget0.80 | 0.8473 | 0.8623 | 0.7673 | 0.6849 | 0.8169 | 0.8258 | 0.7762 | 0.7996 |
| 1.00 | learned_pca_doc_k128_budget0.80 | 0.8473 | 0.8623 | 0.7673 | 0.6849 | 0.8169 | 0.8258 | 0.7762 | 0.7996 |

## Macro Rows

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |
| learned_raw_k128_budget0.10 | 0.8342 | 0.8455 | 0.7424 | 0.6578 | 0.7754 | 0.6579 | 0.1000 |
| learned_raw_k128_budget0.20 | 0.8357 | 0.8455 | 0.7425 | 0.6579 | 0.7763 | 0.6583 | 0.2000 |
| learned_raw_k128_budget0.35 | 0.8364 | 0.8455 | 0.7425 | 0.6580 | 0.7761 | 0.6586 | 0.3500 |
| learned_raw_k128_budget0.50 | 0.8366 | 0.8455 | 0.7425 | 0.6580 | 0.7760 | 0.6586 | 0.5000 |
| learned_raw_k128_budget0.80 | 0.8366 | 0.8455 | 0.7426 | 0.6580 | 0.7760 | 0.6586 | 0.8000 |
| learned_pca_doc_k128_budget0.10 | 0.7628 | 0.8586 | 0.7542 | 0.6460 | 0.7631 | 0.6883 | 0.1000 |
| learned_pca_doc_k128_budget0.20 | 0.8006 | 0.8598 | 0.7618 | 0.6649 | 0.7693 | 0.7460 | 0.2000 |
| learned_pca_doc_k128_budget0.35 | 0.8282 | 0.8616 | 0.7656 | 0.6767 | 0.7733 | 0.7803 | 0.3500 |
| learned_pca_doc_k128_budget0.50 | 0.8379 | 0.8626 | 0.7664 | 0.6811 | 0.7745 | 0.7963 | 0.5000 |
| learned_pca_doc_k128_budget0.80 | 0.8473 | 0.8623 | 0.7673 | 0.6849 | 0.7762 | 0.8169 | 0.7996 |

## Comparison To M369

M370 is a real frontier improvement.

| Touch bucket | M369 best | M369 NDCG@10 | M370 best | M370 NDCG@10 |
| ---: | --- | ---: | --- | ---: |
| 0.20 | raw_k64_cap8 | 0.6567 | learned_pca_doc_k128_budget0.10 | 0.7542 |
| 0.35 | raw_k128_cap8 | 0.7184 | learned_pca_doc_k128_budget0.20 | 0.7618 |
| 0.50 | raw_k96_cap16 | 0.7242 | learned_pca_doc_k128_budget0.35 | 0.7656 |
| 0.80 | pca_doc_k128_cap32 | 0.7602 | learned_pca_doc_k128_budget0.80 | 0.7673 |

The surprising row is `learned_raw_k128_budget0.10`:

- sparse-score NDCG@10 is only 0.7424;
- but candidate-set exact dense rerank NDCG@10 is 0.7754;
- dense baseline is 0.7759.

That means the learned admission model can find an almost dense-quality candidate
set at only 10% touched docs. The remaining loss is mostly sparse scoring /
ordering, not admission.

PCA behaves differently:

- sparse scoring is much stronger than raw coordinates;
- candidate dense-rerank upper bound also improves with budget;
- at 35% touched docs, PCA sparse NDCG@10 already reaches 0.7656.

## Model Coefficients

Standardized logistic coefficients:

| Feature | Coefficient |
| --- | ---: |
| q_abs | 0.1229 |
| doc_abs | 0.0326 |
| impact_abs | -0.0271 |
| log_rank | -0.0343 |
| rank_fraction | -0.0097 |
| log_df | -0.0523 |
| query_dim_rank_fraction | -0.0103 |
| query_mass_fraction | 0.1088 |
| active_fraction | 0.0000 |
| is_pca_doc | 0.0110 |

The model mostly learns:

- favor high query-coordinate mass;
- favor high absolute query value;
- penalize deeper ranks and high-df lists.

This is exactly the kind of traversal policy the M369 evidence suggested.

## Verdict

The user's hypothesis is strongly supported.

The next optimization should train/adapt posting traversal, not keep trying to
hard-compress the dense vector with a weaker SAE basis.

M370 shows:

- learned posting admission beats fixed caps sharply at the same fanout;
- candidate admission can be near-dense at only 10% touch if reranked by exact
  dense;
- PCA sparse scoring gives the best end-to-end sparse score;
- raw coordinates may be better for admission, but need better scoring.

## Next Step

M371 should separate two products:

1. **Admission + dense rerank**:
   learned raw/PCA coordinate admission, then exact dense rerank on 5%-20%
   candidates. This is already close to dense quality on the sampled BEIR15 face.

2. **Pure sparse score**:
   learned PCA admission plus a trained lightweight candidate scorer to recover
   the dense-rerank ordering without reading full dense vectors.

The immediate rigorous test should be leave-dataset-out M371:

- train the admission policy on 14 datasets;
- evaluate on the held-out dataset;
- compare against M369 fixed caps and exact dense;
- keep qrels strictly evaluation-only.
