# SAE M75 Source-Utility Weighted Training Results Report

Date: 2026-05-21

Status: per-candidate source-utility weighting is implemented and tested. It is
useful evidence, but not promoted as the next training breakthrough.

## Summary

M75 tested the M74 conclusion directly: keep all training queries, but upweight
qrel-positive candidates found by dense/SAE teacher and missed by BM25. The
goal was to use teacher signal as a training utility signal rather than a hard
filter or a residual label for every query.

Implemented:

- per-candidate `utility_weights`;
- `--source-utility-weight`;
- `--source-utility-max-weight`;
- `--source-utility-loss-mode coverage_only|ranking_and_coverage`;
- diagnostics for weighted positives and mean utility weight.

The implementation preserves the default path because
`--source-utility-weight=0.0` keeps all utility weights at `1.0`.

## Local Smoke

Scope: `scifact + nfcorpus`, 2,400 documents, 112 train queries, 43 eval
queries, 2 epochs.

| Run | Utility Mode | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| BM25 | n/a | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| M74 no curriculum | none | 0.5410 | 0.6040 | 0.4686 | 0.3537 |
| M75 `w=0.25` | ranking + coverage | 0.5373 | 0.6040 | 0.4686 | 0.3537 |
| M75 `w=0.50` | ranking + coverage | 0.5382 | 0.6056 | 0.4702 | 0.3543 |
| M75 `w=0.75` | ranking + coverage | 0.5322 | 0.6026 | 0.4686 | 0.3537 |
| M75 `w=1.00` | ranking + coverage | 0.5512 | 0.6026 | 0.4686 | 0.3537 |
| M75 `w=1.00` | coverage only | 0.5308 | 0.6026 | 0.4686 | 0.3535 |

Local interpretation:

- Source-utility weight is a real signal.
- `w=1.0` strongly improves Recall@100, especially `semantic_heavy` and
  `short_keyword` recall.
- The recall gain comes with top-ranking degradation, so it does not pass the
  M75 promotion rule.
- `coverage_only` is too weak; it avoids changing the listwise target, but the
  effect is not enough.

## M39 Medium Spark Run

Scope: 9 datasets, 92,816 documents, 1,650 train queries, 679 eval queries, 12
epochs on Spark CUDA. This run used the same M73 medium profile plus:

```text
--source-utility-weight 1.0
--source-utility-loss-mode ranking_and_coverage
```

Quality:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.6661 | 0.6685 | 0.5858 | 0.4949 |
| M73 family baseline rerun | 0.6647 | 0.6687 | 0.5858 | 0.4949 |
| M75 source-utility `w=1.0` | 0.6649 | 0.6693 | 0.5858 | 0.4950 |

Candidate utility:

| Item | Value |
| --- | ---: |
| Candidate count mean | 194.24 |
| Qrel positives in pool | 31,904 / 31,904 |
| Source-utility weighted positives | 5,432 |
| Source-utility weight mean | 1.0169 |
| Teacher extra recall over BM25 | 0.1703 |
| Queries with teacher extra hit | 544 |

Family deltas versus BM25:

| Family | Recall Delta | MRR Delta | NDCG Delta | MAP Delta |
| --- | ---: | ---: | ---: | ---: |
| `all` | -0.0012 | +0.0008 | +0.0000 | +0.0001 |
| `broad_many_positive` | -0.0006 | +0.0001 | -0.0006 | -0.0002 |
| `semantic_heavy` | -0.0007 | +0.0000 | -0.0002 | +0.0001 |
| `short_keyword` | -0.0043 | +0.0028 | +0.0001 | +0.0003 |
| `long_query` | +0.0000 | +0.0000 | -0.0000 | -0.0000 |

Medium interpretation:

- Compared with the M73 family baseline rerun, M75 improves Recall/MRR/NDCG/MAP
  slightly.
- Compared with BM25, M75 still loses Recall@100.
- The signal is too diluted: only 5,432 positives are weighted inside a large
  candidate tensor, so the average utility weight is only 1.0169.
- The top-ranking improvements are real but tiny.

## Decision

Promoted:

- per-candidate source-utility weights as a reusable training feature;
- `coverage_only` versus `ranking_and_coverage` switch;
- weighted-positive diagnostics in training reports.

Rejected as current mainline:

- per-candidate utility weighting alone;
- more small scalar sweeps on `source_utility_weight`;
- coverage-only weighting as the next large run.

## Next Direction

M75 shows that teacher utility is the right signal but the wrong injection
granularity. The next stage should move from per-candidate weighting to a
query-level or candidate-budget mechanism:

```text
M76: source-utility query curriculum
  - keep all queries, but oversample or upweight rows where teacher adds qrel
    positives BM25 misses;
  - keep lexical-only rows in the mix for calibration;
  - use source utility to build the batch schedule, not just candidate weights;
  - evaluate whether query-level emphasis improves Recall without damaging
    MRR/NDCG/MAP.

M77: teacher-extra candidate-budget target
  - train query atoms to recover teacher-extra qrel positives under a fixed
    candidate budget;
  - use BM25-preserving ranking loss as the final objective;
  - report candidate-budget coverage, not only final ranking metrics.
```

The practical lesson is clear: teacher extra positives are valuable, but they
need to control which queries and candidate neighborhoods the model sees more
often, not just slightly scale individual candidate losses.
