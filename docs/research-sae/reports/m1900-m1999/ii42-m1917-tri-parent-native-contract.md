# II-42 M1917 Tri-Parent Native Contract

## Question

Choose the next learned-sparse parent from evidence rather than from a single
FiQA score. Compare frozen P1/M549U, calibrated Granite M1914, and OpenSearch
sparse-v2 on the same full official corpora, PostgreSQL normalized-postings
backend, qrels, candidate depth, and dense ranking reference.

M1917 is a parent-selection and teacher-observability experiment. It does not
add BM25, tune a dataset-specific parameter, or authorize model training from a
macro average alone.

## Frozen Routes

- `p1`: M549U active-128 signed postings, alpha zero.
- `m1914`: Granite 30M sparse support with the frozen M1914 global power
  transform.
- `opensearch`:
  `opensearch-project/opensearch-neural-sparse-encoding-v2-distill`, revision
  `269e6638b2c4f648996691f6d751495285d8f330`.
- `dense_teacher`: the existing frozen PPLX dense rankings. This route is used
  for observability and rescue analysis, not as a fourth posting index.

## Common Native Surface

- Full official rows: FiQA, ArguAna, NFCorpus, and SciFact.
- Exact PostgreSQL posting join, candidate depth 1,000, and identical native
  document/query IDs.
- No scan evaluator, ANN candidate source, BM25 fusion, qrels-derived tuning,
  or post-hoc reranker.
- Every sparse NPZ cache must pass source-size, row-count, dimension, query-set,
  and native-table identity checks before publication.

## Measurements

For every row and equal-weight macro:

- NDCG@10, MAP@100, Recall@100, MRR@20;
- dense overlap@100 and candidate upper bound at 1,000;
- normalized relation bytes and indexed p50/p95/p99;
- positive-document ranks at 100 and 1,000 for pairwise rescue analysis.

## Parent Decision

The report must distinguish three roles:

1. product baseline: best balanced native quality, row stability, and cost;
2. research parent: best trainable quality/cost frontier with a credible path
   to repair its measured deficit;
3. teacher: a surface that contributes unique positives without suppressing
   another parent's useful sparse behavior.

No route is promoted from a four-row canary alone. A route is rejected as the
sole parent if it has severe row harm (Recall below another frontier route by
more than 0.01, or at least two head metrics below it by more than 0.02).

## PPLX Training Gate

PPLX fine-tuning is not automatic. First measure, for each positive document:

- PPLX top100/top1000 rescue missed by the sparse parent;
- sparse top100/top1000 rescue missed by PPLX;
- shared and missed positives;
- whether the gap is candidate admission or ordering.

Authorize only a small qrels-free pilot when PPLX contributes material unique
coverage and a multi-teacher objective can preserve sparse-only positives. A
pure PPLX imitation run is rejected when sparse-only rescue is material. Any
pilot must use public training rows, a query-disjoint heldout split, ClearML,
fixed sparse budgets, parent-faithfulness, and native-index closure.

## Stop Conditions

- Stop a cache or index build on any manifest or ID mismatch.
- Stop parent calibration searches; M1914's scalar/support search is closed.
- Stop before PPLX training if the rescue audit shows no incremental teacher
  information or an unavoidable sparse-only regression.
- Stop a pilot if heldout ordering improves without preserving candidate
  coverage, parent support/cost, or query-disjoint generalization.
