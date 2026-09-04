# II-42 M370-M374 Dense-Only Neural Posting Summary

## Current Mainline

M372 is the current best dense-only posting route.

It uses:

- signed coordinate postings;
- PCA-doc rotation;
- k128 active dimensions;
- 10% candidate touch budget;
- neural dense-score regression;
- no BM25;
- no qrel training.

On the local shared BEIR15 face, M372 reaches NDCG@10 0.7674 versus exact dense
0.7759 at only 10% touched documents.

## Results

| Model | Best <=10% source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense-rerank NDCG | Dense O@10 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M370 | learned_pca_doc_k128_budget0.10 | 0.7628 | 0.8586 | 0.7542 | 0.6460 | 0.7631 | 0.6883 | 0.1000 |
| M371 | neural_raw_k128_budget0.10 | 0.8326 | 0.8123 | 0.7137 | 0.6255 | 0.7761 | 0.5984 | 0.1000 |
| M372 | neural_pca_doc_k128_budget0.10 | 0.8480 | 0.8609 | 0.7674 | 0.6854 | 0.7761 | 0.7960 | 0.1000 |
| M372B | neural_pca_doc_k128_budget0.10 | 0.8502 | 0.8625 | 0.7657 | 0.6859 | 0.7762 | 0.8103 | 0.1000 |
| M373 | neural_pca_doc_k128_budget0.10 | 0.8495 | 0.8569 | 0.7579 | 0.6821 | 0.7761 | 0.7478 | 0.1000 |
| M374 | query_local_pca_doc_k128_budget0.10 | 0.8488 | 0.8594 | 0.7648 | 0.6834 | 0.7762 | 0.7896 | 0.1000 |
| Dense | dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 0.7759 | 1.0000 | 1.0000 |

## Interpretation

The project-level lesson is clear:

- BCE dense-top100 membership is the wrong target for ranking.
- Dense-score regression is the right first objective.
- Candidate admission is already strong enough at 10% touch.
- Simple pairwise dense-order loss did not help.
- Query-local z-score/rank features did not help enough.
- Increasing model capacity/samples improved recall/MRR/MAP slightly, but did
  not improve NDCG.

The remaining gap is no longer a broad admission failure. It is a small
top-order approximation gap in the posting-only scorer.

## Next Step

Do not go back to BM25 rescue for this line.

The next useful pressure should be one of:

1. Held-out-dataset validation of M372 to verify whether the current strong
   result generalizes.
2. Richer posting evidence features: per-coordinate top impacts, signed
   coordinate interaction sketches, or a tiny set encoder over the matched
   posting edges instead of only aggregated scalar features.
3. Distillation with a stronger ranking-preserving objective, but anchored to
   dense-score regression so it does not repeat the M373 calibration loss.
