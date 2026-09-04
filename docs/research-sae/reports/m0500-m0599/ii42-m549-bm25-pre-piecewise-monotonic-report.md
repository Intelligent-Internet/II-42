# M549 BM25-Pre Piecewise Monotonic Report

## Purpose

M549 extends the M548 dense-only stage before introducing BM25, qrels-trained
ranking losses, or search-aware fusion.

M548 found that a small global monotonic power (`gamma=1.02562527`) is safe
enough to carry forward, but larger global gamma values trade dense geometry
for small retrieval movement.  M549 tests a narrower hypothesis:

```text
Keep the strongest per-row dense coordinates near identity, and apply the
M548 monotonic power only to the lower-ranked tail coordinates.
```

This is still a deterministic dense-derived output shape.  It does not use
dataset identity, BM25, or qrels for training.  Qrels are used only as the
dense-only retrieval safety matrix.

## Artifacts

Runner:

```text
scripts/run_m549_piecewise_monotonic_frontier_spark.sh
```

Evaluator:

```text
scripts/research_sae_m549_piecewise_monotonic_frontier.py
```

Remote full run:

```text
/home/huoju/leask/runs/ii42-m549-bm25-pre-piecewise-monotonic-v1/
  broad10_mild_piecewise_seed549/
```

Local mirror:

```text
outputs/m549/bm25_pre_piecewise_monotonic/
  broad10_mild_piecewise_seed549/
```

Smoke runs:

```text
outputs/m549/bm25_pre_piecewise_monotonic/
  smoke_fiqa_scidocs_trec_seed549/
  smoke_mild_fiqa_scidocs_trec_seed549/
```

## Gate

The M549 gate is intentionally stricter than simply reading NDCG:

- Recall@100 drop no worse than `0.0003`.
- Dense overlap@100 at least `0.994`.
- Teacher active drop no worse than `0.0`.
- Teacher support cosine drop no worse than `0.0005`.
- Teacher KL proxy must improve versus exact dense.

The teacher proxy is computed against a fixed `row_int8(exact_dense_doc)`
target from the materialized dense rows.  It is not a replacement for the M546
model-trace gate, but it is useful for comparing deterministic output shapes
without retraining the encoder.

## Smoke Result

The first aggressive piecewise grid failed: `tail128`, `tail256`, and
`tail512_1p06` moved too much geometry and failed overlap/support constraints.

The mild grid found one candidate that passed the three-task smoke:

```text
tail768_1p025625 = keep top768 coordinates identity,
                   apply gamma=1.02562527 to the remaining tail
```

Smoke macro for `tail768_1p025625`:

| Metric | Value |
| --- | ---: |
| NDCG@10 | 0.49807 |
| MAP@100 | 0.24714 |
| Recall@100 | 0.49604 |
| MRR@20 | 0.62365 |
| Dense overlap@100 | 0.99537 |
| Teacher KL relative improvement | 5.50% |
| Support drop vs exact | -0.00026819 |

## Full Broad10 Result

Full broad10 confirms the mild piecewise candidate.

Selected candidate:

```text
tail768_1p025625
```

Macro matrix:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00% | baseline |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | 3.39% | baseline |
| `gamma_1p025625` | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99414 | -16.85% | fail |
| `tail512_1p02` | 0.57278 | 0.43534 | 0.74792 | 0.65203 | 0.99108 | 12.93% | fail |
| `tail512_1p025625` | 0.57267 | 0.43531 | 0.74774 | 0.65203 | 0.98903 | 16.74% | fail |
| `tail768_1p02` | 0.57268 | 0.43538 | 0.74806 | 0.65187 | 0.99618 | 2.67% | pass |
| `tail768_1p025625` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | 3.32% | pass |
| `tail896_1p025625` | 0.57276 | 0.43535 | 0.74795 | 0.65174 | 0.99801 | 1.02% | pass |

Selected deltas versus exact dense:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.00015 |
| MAP@100 | +0.00010 |
| Recall@100 | -0.00001 |
| MRR@20 | +0.00016 |
| Dense overlap@100 | 0.99525 |
| Teacher KL relative improvement | +3.32% |

Per-task NDCG@10 deltas for `tail768_1p025625`:

| Task | dNDCG@10 | dRecall@100 | O@100 |
| --- | ---: | ---: | ---: |
| `ArguAna` | -0.00036 | +0.00000 | 0.99637 |
| `CQADupstackGamingRetrieval` | +0.00031 | -0.00063 | 0.99408 |
| `CQADupstackUnixRetrieval` | +0.00091 | +0.00000 | 0.99486 |
| `ClimateFEVERHardNegatives` | +0.00014 | +0.00050 | 0.99534 |
| `FEVERHardNegatives` | +0.00004 | +0.00000 | 0.99537 |
| `FiQA2018` | +0.00026 | -0.00030 | 0.99565 |
| `HotpotQAHardNegatives` | -0.00010 | +0.00000 | 0.99483 |
| `SCIDOCS` | -0.00010 | +0.00020 | 0.99525 |
| `TRECCOVID` | -0.00061 | -0.00023 | 0.99520 |
| `Touche2020Retrieval.v3` | +0.00100 | +0.00037 | 0.99551 |

## Interpretation

M549 gives a stronger pre-BM25 floor than M548 in one important way: it shows
that the useful monotonic calibration does not need to touch the entire vector.
Keeping the top `768` coordinates at identity and calibrating only the tail
keeps dense overlap safer than the global gamma while retaining a measurable
teacher-fit proxy improvement.

This does not change the larger conclusion: dense-only output-shape tuning is
near its ceiling.  The gains are real but small.  The best current stage-one
surfaces before BM25/search-aware work are:

```text
exact_dense
row_int8
M548 global gamma=1.02562527
M549 tail768_1p025625
```

M549 should be treated as the most conservative handoff candidate into the
next BM25-aware stage because it keeps `O@100=0.99525` while slightly improving
macro NDCG, MAP, MRR, and the teacher-fit proxy.

## Decision

Promote `tail768_1p025625` as the pre-BM25 piecewise monotonic milestone.

Next work should reintroduce BM25/search-aware optimization as a separate
stage, with exact dense, row-int8, M548 global gamma, and M549 tail768 as fixed
baselines.  Further dense-only deterministic output-shape tuning is unlikely
to produce a large standalone win.
