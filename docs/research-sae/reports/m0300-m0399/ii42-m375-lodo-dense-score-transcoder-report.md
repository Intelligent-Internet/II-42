# II-42 M375 LODO Dense-Score Transcoder Report

## Goal

M375 validates the M372 dense-only posting scorer with leave-dataset-out
training.

Each fold trains the M372 dense-score transcoder on 14 datasets and evaluates
only on the held-out dataset. This checks whether M372 is learning a reusable
posting scorer or merely benefiting from mixed target-domain teacher data.

## Command

```bash
python3 scripts/research_sae_m375_lodo_dense_score_transcoder.py
```

Output:

- `/tmp/ii42-m375-lodo-dense-score-transcoder/m375_lodo_dense_score.json`
- `/tmp/ii42-m375-lodo-dense-score-transcoder/m375_lodo_dense_score.md`

Runtime: 499.39 seconds locally.

## Setup

- Data root: `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`
- Heldout folds: 15
- Rotation: `pca_doc`
- Active dims: `128`
- Budget: `0.10`
- Max train examples per fold: 500000
- Epochs: 6
- BM25: not used
- Qrels: evaluation only

## Macro Result

| Run | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M372 mixed | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.1000 |
| M375 LODO | 0.8407 | 0.8393 | 0.7379 | 0.6755 | 0.7762 | 0.7604 | 0.1000 |
| exact dense | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |

## Per-Fold Result

| Heldout | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG@10 | Dense O@10 | Gap to dense NDCG |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 1.0000 | 0.5272 | 0.6386 | 0.5272 | 0.6650 | 0.8950 | -0.0264 |
| climate-fever | 0.9592 | 0.7048 | 0.5893 | 0.4967 | 0.6022 | 0.8040 | -0.0130 |
| cqadupstack | 0.9569 | 0.8606 | 0.8230 | 0.7642 | 0.8295 | 0.8350 | -0.0064 |
| dbpedia-entity | 0.9000 | 0.9775 | 0.7092 | 0.7587 | 0.7202 | 0.8230 | -0.0110 |
| fever | 1.0000 | 1.0000 | 0.9988 | 0.9975 | 0.9988 | 0.7570 | 0.0000 |
| fiqa | 0.9248 | 0.7535 | 0.6986 | 0.6476 | 0.7124 | 0.8170 | -0.0138 |
| hotpotqa | 1.0000 | 1.0000 | 0.9461 | 0.9253 | 0.9573 | 0.7400 | -0.0112 |
| msmarco | 0.8219 | 1.0000 | 0.7408 | 0.8124 | 0.7498 | 0.8163 | -0.0089 |
| nfcorpus | 0.3551 | 0.6789 | 0.4447 | 0.2223 | 0.4566 | 0.7430 | -0.0119 |
| nq | 1.0000 | 0.9950 | 0.9960 | 0.9942 | 0.9963 | 0.7610 | -0.0003 |
| quora | 1.0000 | 0.9933 | 0.9914 | 0.9864 | 0.9918 | 0.8360 | -0.0004 |
| scidocs | 0.6735 | 0.6543 | 0.4166 | 0.3201 | 0.4411 | 0.8050 | -0.0246 |
| scifact | 0.9700 | 0.7656 | 0.7849 | 0.7594 | 0.8097 | 0.7420 | -0.0248 |
| trec-covid | 0.0635 | 0.6885 | 0.4502 | 0.0382 | 0.8626 | 0.1300 | -0.4085 |
| webis-touche2020 | 0.9851 | 0.9898 | 0.8402 | 0.8824 | 0.8497 | 0.9020 | -0.0095 |

## Verdict

M372 mostly generalizes, but not universally.

14 of 15 heldout datasets stay close to the mixed M372 result. The macro drop is
dominated by `trec-covid`. On `trec-covid`, the candidate set is excellent:
dense reranking the selected 10% candidates reaches NDCG@10 0.8626. The neural
posting scorer itself reaches only 0.4502, with dense overlap@10 only 0.1300.

That means the failure is not admission. It is cross-domain score calibration
or ranking transfer for `trec-covid`.

## Next Step

Run target-domain dense-teacher adaptation without BM25 or qrels:

- keep target train queries disjoint from target eval queries;
- use dense scores as teacher labels;
- verify whether a small amount of target-domain dense supervision restores
  `trec-covid` ranking.

If it works, the product direction should support per-corpus dense-teacher
distillation. If it does not, we need richer posting-edge features rather than
only scalar aggregate features.
