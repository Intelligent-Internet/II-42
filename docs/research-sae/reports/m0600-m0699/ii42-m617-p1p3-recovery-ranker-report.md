# II-42 M617 P1.3 Recovery Ranker Report

Date: 2026-07-06

## Objective

M617 tests the first scoped second-stage scorer after M615/M616 showed that the
dominant remaining gap is candidate-present, under-ranked, and mostly
dense-miss.  It does not change the P1 encoder or posting generator.

The model is a global pairwise linear ranker:

- no dataset identity feature,
- no dense-rank or dense-hit feature at inference,
- trained on same-query positive-vs-blocker pairs,
- old M605 remains a benchmark only.

## Inputs

Base recovery dataset:

`runs/m616_p1p3_scorer_recovery_dataset_v1/m616_p1p3_scorer_recovery_dataset.jsonl`

Tail-aware recovery dataset:

`runs/m618_p1p3_tail_aware_recovery_dataset_v1/m618_tail_aware_recovery_dataset.jsonl`

The M618 dataset adds sampled tail negatives because M617B showed that the
selected M616 surface was too narrow and over-optimistic.

## Implementation

- Training script: `scripts/train_m617_recovery_ranker.py`
- Saved-model replay script: `scripts/apply_m617_recovery_ranker.py`
- Core feature invariant: no dataset id, no dense reference feature.

## Split-Level Results

M617A used `alpha=0.10`, preserved top5, and passed the macro split gate but
regressed several dataset heads.

| Run | Dataset | Alpha | Preserve | Status | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | --- | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| M617A | M616 | 0.10 | 5 | split pass, unsafe per-row | +0.001515 | +0.009950 | +0.015360 | +0.000821 |
| M617B | M616 | 0.10 | 10 | split pass | +0.000000 | +0.009374 | +0.015299 | +0.000307 |
| M617C | M618 tail-aware | 0.10 | 10 | rejected | +0.000000 | -0.006407 | +0.005051 | -0.000264 |

The M616 split result is real but not sufficient: it is measured on a selected
surface that excludes most tail negatives.

## Full M604 Replay

M617B was replayed over all M604 P1.3 shared15 candidate JSONL rows.

Without a rerank window, the model collapses Recall/MAP because it promotes
unseen tail negatives:

| Replay | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| Full tail | +0.000000 | -0.076120 | -0.154087 | -0.001328 |

Adding a global rerank window prevents collapse but leaves only tiny gains:

| Window | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| 300 | +0.000000 | +0.000221 | -0.000183 | -0.000043 |
| 500 | +0.000000 | +0.000062 | +0.000086 | +0.000017 |
| 700 | +0.000000 | +0.000100 | +0.000091 | +0.000022 |
| 1000 | +0.000000 | +0.000090 | +0.000078 | +0.000022 |

The best full-M604 replay signal is positive but too small to justify native
DB/plugin benchmark execution.

## Interpretation

The M617 pairwise-linear family has a useful diagnostic signal:

- It can promote under-ranked positives on the narrow recovery surface.
- It can avoid NDCG@10 regression by preserving top10.
- It confirms that full-tail negatives are the central difficulty.

But it is not yet a viable scorer:

- Selected-set gains do not transfer to full candidate replay.
- Tail-aware training protects against some tail failures but loses MAP.
- Windowed replay produces only near-zero improvements.

This means the current pairwise linear loss family should stop here.  Continuing
to tune alpha, preserve-k, or rank window is not likely to solve the bottleneck.

## Decision

Do not promote M617A/B/C.

Do not run the native DB/plugin benchmark for this model family.

Keep the artifacts as evidence:

- M616 proves the second-stage dataset shape.
- M617 proves simple pairwise linear scoring is underpowered.
- M618 proves full-tail negatives must be included or explicitly gated.

## Recommended Next Step

If continuing the scorer route, the next attempt should not be another global
linear pairwise model.  It needs a different objective or architecture:

1. Windowed admission model: learn a conservative tail-admission gate before
   reranking.
2. Listwise objective over a fixed window, optimizing Recall@100/MAP@100 while
   preserving top10.
3. Richer query-local features or atom-interaction features that explain why a
   dense-miss positive should beat dense-plausible blockers.

The next model must be evaluated first on full M604 replay, not only on the
selected M616 split.

## Verification

Local verification passed:

- `python3 -m py_compile` on M616/M617 builder, trainer, replay scripts.
- `pytest -q` on M616/M617 train and replay tests.
- `git diff --check`.
