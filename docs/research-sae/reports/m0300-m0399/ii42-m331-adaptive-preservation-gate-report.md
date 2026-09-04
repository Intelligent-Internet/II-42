# ii42 M331 Adaptive Preservation Gate Report

## Summary

M331 tested the final local preservation-gate idea on the M329/M330 five-dataset
candidate surface. The hypothesis was that a lightweight query-level gate could
learn when to preserve BM25-supported positives, improving top-rank balance
without the fixed-profile tradeoff seen in M330.

The result is a useful negative result. The adaptive gate did not learn a useful
query-dependent policy. It drove the learned preservation weight essentially to
zero on both train and heldout queries, while macro metrics only moved by tiny
amounts. M331 should not be promoted as a new mainline strategy.

## Run

- Host: `spark-1`
- Run directory:
  `/home/huoju/leask/runs/ii42-m331-adaptive-preservation-v1`
- Result:
  `/home/huoju/leask/runs/ii42-m331-adaptive-preservation-v1/m331_adaptive_preserve020_seed1050.json`
- Base checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Surface:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`
- Datasets:
  `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`

The run used the exact M330 single-query scorer optimizer shape and added:

- `--adaptive-bm25-preservation-max-weight 0.20`
- `--adaptive-bm25-preservation-base-weight 0.0`
- `--adaptive-bm25-preservation-hidden-dim 16`

The gate starts at the M330 fixed `0.10` preservation point because the final
gate layer is zero-initialized and `sigmoid(0) = 0.5`.

## Macro Heldout Metrics

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M330 exact batch1 | 0.7276 | 0.3660 | 0.3347 | 0.2425 |
| M330 fixed preserve 0.10 | 0.7276 | 0.3648 | 0.3346 | 0.2426 |
| M330 fixed preserve 0.20 | 0.7265 | 0.3644 | 0.3359 | 0.2424 |
| M331 adaptive preserve max 0.20 | 0.7268 | 0.3649 | 0.3352 | 0.2428 |

M331 slightly improves MAP over the fixed preservation runs, but it does not
improve the overall frontier. It loses recall versus exact and fixed `0.10`, and
it does not match fixed `0.20` NDCG.

## Best Training Event

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 0.7453 | 0.3517 | 0.3310 | 0.2395 |
| 10 | 0.7490 | 0.3575 | 0.3372 | 0.2440 |
| 15 | 0.7472 | 0.3563 | 0.3380 | 0.2440 |
| 20 | 0.7473 | 0.3496 | 0.3327 | 0.2393 |
| 25 | 0.7466 | 0.3523 | 0.3344 | 0.2406 |

The best checkpoint was epoch 15 by MAP. Later epochs degraded top-rank
quality, so the run did not reveal a deeper training path.

## Learned Gate Behavior

| Split | Mean | Min | Max | BM25-heavy mean | Atom-heavy mean |
| --- | ---: | ---: | ---: | ---: | ---: |
| Train | 1.43e-08 | 1.70e-12 | 4.05e-06 | 1.82e-08 | 4.38e-11 |
| Heldout | 8.73e-10 | 1.83e-12 | 4.22e-07 | 1.10e-09 | 9.96e-11 |

This is the decisive result. The adaptive preservation branch collapsed toward
zero instead of learning a useful query-dependent preservation strength. The
observed metric movement is therefore not evidence that adaptive preservation is
working.

## Diagnostics

| Run | BM25-supported qrel suppressed | Qrel admitted but low | Semantic false positives | Teacher top10 qrel lost |
| --- | ---: | ---: | ---: | ---: |
| M330 exact batch1 | 343 | 166 | 266 | 154 |
| M330 fixed preserve 0.10 | 335 | 159 | 265 | 154 |
| M330 fixed preserve 0.20 | 336 | 159 | 262 | 152 |
| M331 adaptive preserve max 0.20 | 337 | 160 | 267 | 156 |

M331 improves suppression versus exact, but fixed preservation already does this
more cleanly. Compared with fixed `0.10`, M331 is worse on all four diagnostic
counts.

## Per-Dataset Heldout Scorer Metrics

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.9903 | 0.2232 | 0.3410 | 0.2255 |
| `fiqa` | 0.7242 | 0.4843 | 0.3987 | 0.3304 |
| `nfcorpus` | 0.3107 | 0.5798 | 0.3495 | 0.1620 |
| `scidocs` | 0.4284 | 0.3402 | 0.1832 | 0.1256 |
| `scifact` | 0.9447 | 0.6203 | 0.6616 | 0.6127 |

The remaining imbalance is still dataset/query-shape dependent. M331 did not
learn a robust runtime-safe policy to resolve that imbalance.

## Verdict

M331 closes the local BM25 preservation-gate line as a non-promoted result.

Keep the code path as an opt-in research switch because it is isolated and
defaults to off, but do not spend more cycles on small preservation-weight
variants. The better evidence from M310-M323 remains that any real breakthrough
must train a deeper posting-level admission/ranking objective, not a small
post-hoc preservation gate.

