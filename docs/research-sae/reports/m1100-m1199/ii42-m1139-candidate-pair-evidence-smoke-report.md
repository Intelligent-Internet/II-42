# ii42 M1139 Candidate-Pair Evidence Smoke

## Question

M1138 showed a large oracle ceiling when selectively choosing between M1129 and
M1137, but query-level safe features could not identify the useful subset.

M1139 asks whether candidate/doc-pair features are strong enough to recover the
M1137-only useful evidence.

## Setup

Compared rows:

- M1129 fixed `alpha=0.60`, `gamma=1.00`
- M1137 heldout-best `alpha=0.50`, `gamma=0.50`
- Split: shared15 heldout

Candidate-level positive target:

- qrels-positive document
- M1137 ranks it in top100
- M1129 either misses top100 or M1137 improves its rank by at least 25

Hard negatives:

- qrels-negative documents in M1129 or M1137 top100

Inference-safe features:

- M1129 and M1137 shaped scores
- score z-scores
- reciprocal ranks
- clipped rank fractions
- rank delta
- lexical scores
- atom scores

The model was a leave-one-dataset-out logistic regression with balanced class
weight.

## Separability

Candidate/doc-pair separability is much stronger than query-scalar
separability, but the class imbalance is severe.

| Row | Value |
| --- | ---: |
| examples | 49232 |
| positives | 513 |
| positive rate | 0.010420 |
| macro AUC | 0.900235 |
| macro average precision | 0.189850 |
| macro precision@0.5 | 0.095177 |
| macro recall@0.5 | 0.796590 |
| macro F1@0.5 | 0.139797 |

This means the model can roughly rank useful candidate docs, but cannot accept
them directly at a fixed threshold without many false positives.

## Replay: Probability Bonus

Replay policy:

`final_score = M1129_score + beta * p(useful_candidate)`

| Beta | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| baseline | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| 0.01 | 0.758662 | 0.779772 | 0.699915 | 0.604222 |
| 0.03 | 0.759053 | 0.779906 | 0.699906 | 0.604966 |
| 0.05 | 0.759510 | 0.779994 | 0.700492 | 0.605517 |
| 0.08 | 0.759260 | 0.780065 | 0.700674 | 0.606175 |
| 0.10 | 0.759903 | 0.780050 | 0.700570 | 0.606511 |
| 0.15 | 0.761438 | 0.780140 | 0.701834 | 0.607353 |
| 0.20 | 0.765777 | 0.779017 | 0.701937 | 0.607360 |

This is not the desired effect. It slightly improves NDCG/MAP at some betas,
but it loses too much Recall.

## Replay: Positive M1137 Delta Bonus

Replay policy:

`final_score = M1129_score + beta * p(useful_candidate) * max(0, M1137_score - M1129_score)`

| Beta | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| baseline | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| 0.05 | 0.758878 | 0.779747 | 0.699906 | 0.604426 |
| 0.10 | 0.758969 | 0.779886 | 0.699837 | 0.604773 |
| 0.20 | 0.759402 | 0.779994 | 0.700350 | 0.605395 |
| 0.40 | 0.760424 | 0.780001 | 0.701444 | 0.606293 |
| 0.80 | 0.766427 | 0.777468 | 0.702258 | 0.606489 |
| 1.20 | 0.770333 | 0.774994 | 0.700680 | 0.603478 |

The delta-bonus variant has the same failure shape: small NDCG/MAP movements,
but it does not recover the M1138 safe-oracle Recall gain.

## Verdict

Stop the M1139 candidate-pair logistic rerank route.

Useful evidence exists, but the simple candidate-pair classifier turns it into
rank polishing rather than safe expansion. This is another indication that the
current M1137-derived evidence is not sufficient as a standalone promotion
source.

## Implication

The next route should not be another wrapper around M1137 scores. It should use
M1129 as the frozen baseline and change the training target itself:

1. protect M1129 top-rank behavior as a hard teacher;
2. add only safe-oracle expansion rows as auxiliary positives;
3. train the posting head or scorer against that combined target directly;
4. require shared15 LODO/native replay before promotion.

If that direct target cannot recover a material fraction of the safe-oracle
ceiling, the evidence says this family is exhausted.
