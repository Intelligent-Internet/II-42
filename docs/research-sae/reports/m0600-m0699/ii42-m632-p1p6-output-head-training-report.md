# M632 / P1.6 Output-Head Training Report

Status: M632-B/C completed; no output head promoted.

## Objective

M632-B/C tests whether a deeper first-stage output-head/posting-compiler can
learn dense-usefulness targets better than the M630 linear calibration line.

Training targets combine:

- candidate-set dense softmax distribution,
- dense top100 membership,
- dense-tail-vs-false-head pairwise margin,
- P1 score anchoring for top-rank stability.

Forbidden as inference features:

- BM25/fused features,
- qrels labels,
- dense rank/score fields,
- dataset/query/doc ids.

## Runs

Two bounded smoke configurations were run:

| Run | Path | Purpose |
| --- | --- | --- |
| Aggressive | `runs/m632_p1p6_output_head_smoke_v1/` | Test whether capacity can promote dense-tail positives. |
| Guarded | `runs/m632_p1p6_output_head_smoke_guarded_v1/` | Test whether stronger anchor / weaker pairwise preserves top metrics. |

Each run trained:

- `linear`
- `small_mlp`
- `residual_monotonic`

Each was evaluated with conservative top100 swaps:

- preserve top20,
- swap budget 5,
- swap budget 10.

## Aggressive Configuration

| Head | Swap | Gate | Loss first -> final | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap | dKL |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `linear` | 5 | `ndcg_guard_failed` | 7.645690 -> 5.120526 | -0.077996 | -0.087987 | +0.003231 | -0.092591 | -0.027451 | -0.452278 |
| `linear` | 10 | `ndcg_guard_failed` | 7.645690 -> 5.120526 | -0.077996 | -0.088245 | +0.003151 | -0.092591 | -0.048431 | -0.452278 |
| `small_mlp` | 5 | `no_recall_signal` | 6.840484 -> 4.520603 | -0.219219 | -0.186433 | -0.000920 | -0.259722 | -0.018627 | -0.859391 |
| `small_mlp` | 10 | `no_recall_signal` | 6.840484 -> 4.520603 | -0.219219 | -0.186549 | -0.001169 | -0.259722 | -0.042353 | -0.859391 |
| `residual_monotonic` | 5 | `no_recall_signal` | 5.783340 -> 5.729539 | +0.000000 | +0.000132 | -0.000058 | +0.000000 | -0.006667 | -0.062336 |
| `residual_monotonic` | 10 | `no_recall_signal` | 5.783340 -> 5.729539 | +0.000000 | +0.000068 | -0.000126 | +0.000000 | -0.012157 | -0.062336 |

## Guarded Configuration

| Head | Swap | Gate | Loss first -> final | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap | dKL |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `linear` | 5 | `ndcg_guard_failed` | 5.810720 -> 4.667939 | -0.146484 | -0.130506 | +0.003462 | -0.164816 | -0.004314 | -0.453376 |
| `linear` | 10 | `ndcg_guard_failed` | 5.810720 -> 4.667939 | -0.146484 | -0.130506 | +0.003462 | -0.164816 | -0.004510 | -0.453376 |
| `small_mlp` | 5 | `ndcg_guard_failed` | 6.463301 -> 4.459323 | -0.167946 | -0.154373 | +0.000090 | -0.230053 | -0.000980 | -0.896751 |
| `small_mlp` | 10 | `ndcg_guard_failed` | 6.463301 -> 4.459323 | -0.167946 | -0.154373 | +0.000090 | -0.230053 | -0.000980 | -0.896751 |
| `residual_monotonic` | 5 | `no_recall_signal` | 5.398114 -> 5.333724 | -0.004617 | -0.001045 | -0.000485 | -0.000901 | -0.001176 | -0.040577 |
| `residual_monotonic` | 10 | `no_recall_signal` | 5.398114 -> 5.333724 | -0.004617 | -0.001045 | -0.000485 | -0.000901 | -0.001176 | -0.040577 |

## Interpretation

This is not a shallow "training did nothing" failure.  The train loss drops in
both configurations for all heads, and dense KL improves.  The problem is the
tradeoff:

- Heads that increase `Recall@100` damage `NDCG@10`, `MAP@100`, and `MRR@20`
  by far beyond the guard.
- The conservative residual head preserves the top ranking best, but does not
  produce positive `Recall@100`.
- The MLP lowers dense KL most aggressively, but it damages ranking most.

The current feature/target surface can learn dense distribution pressure, but
it cannot decide safe top100 displacements.  Increasing depth alone made the
tradeoff worse, not better.

## Decision

Do not expand M632 to shared15.

Do not promote any M632 head.  The evidence points away from more scorer-depth
tuning and toward encoder/posting representation or a more structured compiler
target that can preserve head ordering while recovering dense-tail support.
