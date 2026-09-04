# II-42 M368 Rotated Coordinate Posting Report

## Goal

M368 tests the direct next step after M367:

Can an orthogonal coordinate rotation preserve dense quality at lower active-k
before posting fanout becomes full scan?

This is still a dense-only representation experiment:

- no BM25;
- no qrel training;
- no learned SAE atoms;
- no reranking.

## Command

```bash
python3 scripts/research_sae_m368_rotated_coordinate_posting_eval.py
```

Output:

- `/tmp/ii42-m368-rotated-coordinate-posting/m368_rotated_coordinate.json`
- `/tmp/ii42-m368-rotated-coordinate-posting/m368_rotated_coordinate.md`

Runtime: 58.28 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc,random_orthogonal,hadamard1024`
- Active dims: `16,32,64,96,128,192,256,384`
- Seed: 368

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Best Frontier By Fanout

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.20 | none | | | | | | | |
| 0.50 | raw_k16 | 0.6029 | 0.5708 | 0.4411 | 0.3477 | 0.2267 | 0.2373 | 0.3918 |
| 0.85 | raw_k32 | 0.7258 | 0.7217 | 0.5956 | 0.4968 | 0.3525 | 0.3450 | 0.8289 |
| 0.999 | raw_k64 | 0.7991 | 0.8119 | 0.6967 | 0.6046 | 0.5001 | 0.4907 | 0.9987 |
| 1.00 | pca_doc_k384 | 0.8534 | 0.8719 | 0.7757 | 0.6944 | 0.9801 | 0.9846 | 1.0000 |

## Key Macro Rows

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| raw_k32 | 0.7258 | 0.7217 | 0.5956 | 0.4968 | 0.3525 | 0.3450 | 0.8289 |
| raw_k64 | 0.7991 | 0.8119 | 0.6967 | 0.6046 | 0.5001 | 0.4907 | 0.9987 |
| raw_k128 | 0.8366 | 0.8455 | 0.7426 | 0.6580 | 0.6588 | 0.6577 | 1.0000 |
| pca_doc_k32 | 0.8246 | 0.7877 | 0.6867 | 0.5981 | 0.5613 | 0.6406 | 0.9997 |
| pca_doc_k64 | 0.8438 | 0.8413 | 0.7453 | 0.6606 | 0.6972 | 0.7445 | 1.0000 |
| pca_doc_k128 | 0.8502 | 0.8622 | 0.7675 | 0.6857 | 0.8210 | 0.8452 | 1.0000 |
| pca_doc_k384 | 0.8534 | 0.8719 | 0.7757 | 0.6944 | 0.9801 | 0.9846 | 1.0000 |
| random_orthogonal_k64 | 0.7987 | 0.7949 | 0.6840 | 0.5962 | 0.4941 | 0.4855 | 0.9980 |
| hadamard1024_k128 | 0.6061 | 0.4551 | 0.3366 | 0.2651 | 0.1829 | 0.2365 | 1.0000 |

## Interpretation

PCA rotation is useful for quality, but not by itself a product solution.

Compared with raw coordinates:

- PCA k32 improves NDCG@10 from 0.5956 to 0.6867;
- PCA k64 improves NDCG@10 from 0.6967 to 0.7453;
- PCA k128 improves NDCG@10 from 0.7426 to 0.7675.

But PCA also makes fanout essentially full scan very early:

- PCA k16 already touches 98.17% of docs;
- PCA k32 touches 99.97% of docs;
- PCA k64+ touches 100%.

Random orthogonal rotation does not beat raw coordinates. Hadamard-1024 is much
worse at low-k and is not promising in this form.

## Verdict

M368 improves the representation-quality side with `pca_doc`, but it does not
solve fanout. The next lever must act on posting traversal, not just coordinate
basis.

The direct next experiment is impact-capped posting traversal:

- keep raw/PCA coordinate quality;
- for each query coordinate, open only the highest same-sign impact postings;
- measure the candidate union quality/fanout frontier.
