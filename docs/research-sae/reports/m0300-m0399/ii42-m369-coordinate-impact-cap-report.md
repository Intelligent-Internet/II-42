# II-42 M369 Coordinate Impact-Cap Report

## Goal

M369 tests the first product-like candidate generation step after M368.

Instead of opening every document that shares an active coordinate, it opens only
the top same-sign impact postings for each active query coordinate, then scores
the resulting candidate union with sparse coordinate cosine.

This remains dense-only:

- no BM25;
- no qrel training;
- no learned SAE atoms;
- no reranking.

## Command

```bash
python3 scripts/research_sae_m369_coordinate_impact_cap_eval.py
```

Output:

- `/tmp/ii42-m369-coordinate-impact-cap/m369_coordinate_impact_cap.json`
- `/tmp/ii42-m369-coordinate-impact-cap/m369_coordinate_impact_cap.md`

Runtime: 208.44 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Datasets: 15
- Rotations: `raw,pca_doc`
- Active dims: `32,64,96,128`
- Caps per dim: `8,16,32,64,128,256`

## Dense Baseline

| R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8533 | 0.8722 | 0.7759 | 0.6947 |

## Best Frontier By Fanout

| Touched ratio <= | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Actual touched |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.05 | none | | | | | | | |
| 0.10 | none | | | | | | | |
| 0.20 | raw_k64_cap8 | 0.6865 | 0.7866 | 0.6567 | 0.5435 | 0.4358 | 0.3557 | 0.1946 |
| 0.35 | raw_k128_cap8 | 0.7620 | 0.8329 | 0.7184 | 0.6138 | 0.5919 | 0.5199 | 0.3471 |
| 0.50 | raw_k96_cap16 | 0.8052 | 0.8404 | 0.7242 | 0.6347 | 0.5706 | 0.5364 | 0.4631 |
| 0.85 | pca_doc_k128_cap32 | 0.8271 | 0.8599 | 0.7602 | 0.6704 | 0.7691 | 0.7567 | 0.7715 |
| 1.00 | pca_doc_k128_cap128 | 0.8497 | 0.8622 | 0.7679 | 0.6851 | 0.8117 | 0.8330 | 0.9662 |

## Top Macro Rows

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| pca_doc_k128_cap128 | 0.8497 | 0.8622 | 0.7679 | 0.6851 | 0.8117 | 0.8330 | 0.9662 |
| pca_doc_k128_cap64 | 0.8475 | 0.8621 | 0.7677 | 0.6835 | 0.8017 | 0.8146 | 0.9152 |
| pca_doc_k128_cap32 | 0.8271 | 0.8599 | 0.7602 | 0.6704 | 0.7691 | 0.7567 | 0.7715 |
| pca_doc_k96_cap32 | 0.8154 | 0.8500 | 0.7508 | 0.6569 | 0.7172 | 0.6979 | 0.6874 |
| pca_doc_k64_cap32 | 0.7868 | 0.8322 | 0.7275 | 0.6267 | 0.6395 | 0.6166 | 0.5539 |
| raw_k128_cap8 | 0.7620 | 0.8329 | 0.7184 | 0.6138 | 0.5919 | 0.5199 | 0.3471 |
| raw_k96_cap16 | 0.8052 | 0.8404 | 0.7242 | 0.6347 | 0.5706 | 0.5364 | 0.4631 |
| raw_k64_cap8 | 0.6865 | 0.7866 | 0.6567 | 0.5435 | 0.4358 | 0.3557 | 0.1946 |

## Comparison To M368

M369 moves the practical frontier forward.

M368 without impact caps:

- raw k32: NDCG@10 0.5956 at 82.89% touch;
- raw k64: NDCG@10 0.6967 at 99.87% touch;
- PCA k64: NDCG@10 0.7453 at 100% touch;
- PCA k128: NDCG@10 0.7675 at 100% touch.

M369 with impact caps:

- raw k64 cap8: NDCG@10 0.6567 at 19.46% touch;
- raw k128 cap8: NDCG@10 0.7184 at 34.71% touch;
- raw k96 cap16: NDCG@10 0.7242 at 46.31% touch;
- PCA k128 cap32: NDCG@10 0.7602 at 77.15% touch.

This is the first dense-only route in this sequence that meaningfully improves
the quality/fanout product surface. It still does not fully solve the problem,
because near-dense quality remains expensive, but the frontier is now tunable.

## Verdict

M369 is worth continuing.

The main signal is not that it reaches exact dense. It does not. The signal is
that impact-capped coordinate postings create a controllable candidate frontier:

- around 20% touch, quality is already usable but not dense-like;
- around 35%-50% touch, quality becomes materially stronger than the uncapped
  low-k frontier;
- around 77% touch, PCA impact-capped coordinates approach dense quality better
  than any previous non-full-scan coordinate row.

Next useful work:

1. Replace fixed per-dim cap with score-threshold or budgeted WAND traversal.
2. Add candidate budget targets such as 5%, 10%, 20%, 40%, 80%.
3. Tune query-adaptive caps from coordinate mass instead of fixed constants.
4. Push only the best frontier configs to a larger/full BEIR15 surface.
