# II-42 M392 BEIR15 Standard Recall Matrix

## Scope

This matrix compares the current best II-42 unified-posting configuration
against dense and dense+BM25 on the local BEIR15 artifact face.

Primary run:
`/tmp/ii42-m392-runtime-tail-bm25-budget008-seed379-beir15/m392_runtime_tail_bm25_budget008_seed379.json`

Current best II-42 configuration:

- dense coordinate posting budget: `0.08`;
- BM25 posting budget: `0.08`;
- dense-tail sketch: `joint_pca_256`, doc side `int8`;
- score: `0.90 * z(dense_tail_score) + 0.10 * z(bm25_score)`;
- candidate set: dense-coordinate posting union BM25 lexical posting;
- mean touch: `0.1338`.

The dense+BM25 reference uses the same fixed `alpha=0.10` z-score blend, but
with full exact dense scores over the whole local corpus. It is not a learned
or qrels-tuned oracle.

## Macro Matrix

| System | R@100 | MRR@20 | NDCG@10 | MAP@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| Dense exact | 0.8545 | 0.8677 | 0.7748 | 0.6931 | 1.0000 |
| Dense+BM25 exact zblend | 0.8572 | 0.8620 | 0.7773 | 0.7004 | 1.0000 |
| Ours dense-only tail256 int8 | 0.8533 | 0.8681 | 0.7755 | 0.6934 | 0.0801 |
| Ours unified tail+BM25 | 0.8557 | 0.8648 | 0.7773 | 0.6983 | 0.1338 |

## Macro Deltas

| Delta | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Ours unified - dense exact | +0.0012 | -0.0028 | +0.0025 | +0.0052 |
| Ours unified - dense+BM25 exact | -0.0016 | +0.0028 | +0.0000 | -0.0021 |

## Per-Dataset Recall/NDCG Matrix

| Dataset | Dense R@100 | Ours R@100 | Dense+BM25 R@100 | Ours gap R vs dense+BM25 | Dense NDCG@10 | Ours NDCG@10 | Dense+BM25 NDCG@10 | Ours gap NDCG vs dense+BM25 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 0.6438 | 0.6435 | 0.6523 | -0.0089 |
| climate-fever | 0.9643 | 0.9643 | 0.9643 | +0.0000 | 0.6403 | 0.6323 | 0.6269 | +0.0054 |
| cqadupstack | 0.9801 | 0.9765 | 0.9764 | +0.0001 | 0.8468 | 0.8417 | 0.8321 | +0.0095 |
| dbpedia-entity | 0.8937 | 0.8981 | 0.8979 | +0.0002 | 0.6973 | 0.6997 | 0.6986 | +0.0011 |
| fever | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 0.9975 | 1.0000 | 1.0000 | +0.0000 |
| fiqa | 0.9393 | 0.9343 | 0.9543 | -0.0200 | 0.6458 | 0.6330 | 0.6267 | +0.0063 |
| hotpotqa | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 0.9606 | 0.9730 | 0.9786 | -0.0055 |
| msmarco | 0.8175 | 0.8228 | 0.8233 | -0.0005 | 0.7481 | 0.7570 | 0.7500 | +0.0070 |
| nfcorpus | 0.3821 | 0.3807 | 0.3822 | -0.0015 | 0.4609 | 0.4636 | 0.4660 | -0.0023 |
| nq | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 1.0000 | 1.0000 | 1.0000 | +0.0000 |
| quora | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 0.9952 | 0.9958 | 0.9924 | +0.0035 |
| scidocs | 0.6610 | 0.6610 | 0.6610 | +0.0000 | 0.4142 | 0.4135 | 0.4077 | +0.0058 |
| scifact | 1.0000 | 1.0000 | 1.0000 | +0.0000 | 0.8270 | 0.8466 | 0.8623 | -0.0157 |
| trec-covid | 0.1963 | 0.2003 | 0.2023 | -0.0020 | 0.8607 | 0.8634 | 0.8582 | +0.0051 |
| webis-touche2020 | 0.9832 | 0.9969 | 0.9969 | +0.0000 | 0.8839 | 0.8968 | 0.9075 | -0.0108 |

## Robustness Macro

The same 8%+8% best setting was repeated on three query-heldout seeds.

| Seed | Dense exact NDCG@10 | Ours dense-only NDCG@10 | Ours unified NDCG@10 | Dense+BM25 exact NDCG@10 |
| ---: | ---: | ---: | ---: | ---: |
| 379 | 0.7748 | 0.7755 | 0.7773 | 0.7773 |
| 1379 | 0.7812 | 0.7815 | 0.7858 | 0.7875 |
| 2379 | 0.7613 | 0.7608 | 0.7658 | 0.7662 |
| mean | 0.7724 | 0.7726 | 0.7763 | 0.7770 |

## Reading

The current best unified-posting version is effectively tied with the full
dense+BM25 exact zblend on NDCG@10, slightly behind on R@100 and MAP@100, and
slightly ahead on MRR@20. The remaining gap is not dense preservation anymore;
it is mostly candidate coverage and lexical weighting on a few datasets such
as `fiqa`, `scifact`, and `webis-touche2020`.
