# SAE M76 Source-Utility Query Curriculum Results Report

Date: 2026-05-21

Status: query-level source-utility row weighting is implemented and tested. It
is not promoted.

## Summary

M76 tested the natural follow-up to M75. M75 showed that teacher-extra qrel
positives are a real signal, but per-candidate weighting is diluted inside the
large candidate tensor. M76 therefore moved the same signal to query-row level:
if dense/SAE teacher finds qrel positives that BM25 misses, the whole query row
can receive a higher loss weight.

The goal was to emphasize useful semantic-neighborhood queries without hard
filtering away lexical/calibration queries.

## Implemented Pieces

The trainer now supports:

```text
--source-utility-row-weight
--source-utility-row-max-weight
--source-utility-row-mode none|extra_positive_any|extra_positive_log_count|extra_positive_fraction
```

The row weight is applied to:

- listwise ranking loss;
- pairwise ranking loss;
- semantic coverage loss;
- semantic teacher loss;
- semantic neighborhood loss.

The output report now includes:

```text
source_utility_row_weighted_queries
source_utility_row_weight_mean
epoch_metrics[].row_weight_mean
```

Default behavior is unchanged because `--source-utility-row-weight=0.0` keeps
every row weight at `1.0`.

## Local Smoke

Scope: `scifact + nfcorpus`, 2,400 documents, 112 train queries, 43 eval
queries, 2 epochs.

| Run | Row Mode | Row Weight Mean | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | n/a | n/a | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| M74 no curriculum | none | 1.0000 | 0.5410 | 0.6040 | 0.4686 | 0.3537 |
| M75 candidate `w=1.0` | candidate | 1.0000 | 0.5512 | 0.6026 | 0.4686 | 0.3537 |
| M76 row `any,w=1.0` | extra_positive_any | 1.3839 | 0.5270 | 0.6026 | 0.4686 | 0.3535 |
| M76 row `any,w=0.25` | extra_positive_any | 1.0960 | 0.5254 | 0.6026 | 0.4701 | 0.3545 |
| M76 row `fraction,w=1.0` | extra_positive_fraction | 1.0931 | 0.5290 | 0.6026 | 0.4686 | 0.3533 |

Family observations:

- `broad_many_positive` improves locally, but this was already true in earlier
  M70/M74 runs and does not rescue aggregate quality.
- `semantic_heavy` recall improves versus BM25, but remains below the M74
  no-curriculum smoke.
- `short_keyword` recall is weak under row weighting, which is a regression
  versus the stronger M75 candidate-weight recall smoke.

## Decision

Promoted:

- reusable source-utility row weight plumbing;
- row-weight diagnostics;
- weighted row losses as a tool for future controlled experiments.

Rejected as current mainline:

- query-level row weighting as the M76 training route;
- running the M76 row-weight variants on Spark medium;
- more scalar sweeps on row-weight magnitude.

Reason:

```text
M76 amplifies the right queries too bluntly. It changes the whole row's ranking
and calibration surface, while the useful teacher signal is still tied to
specific teacher-extra qrel positives and candidate-budget coverage.
```

## Next Direction

The next useful step is no longer query weighting. It should move to a
candidate-budget target:

```text
M77: teacher-extra candidate-budget target
  - preserve the normal BM25/qrel final-ranking objective;
  - add a separate objective that trains query atoms to retrieve teacher-extra
    qrel positives under a fixed sparse candidate budget;
  - measure teacher-extra qrel coverage before final scoring;
  - reject if candidate-budget coverage improves but final MRR/NDCG/MAP
    regresses.
```

This aligns better with the actual problem: SAE should contribute candidates
that BM25 misses, but the final ranker should still decide how much those
semantic candidates matter.
