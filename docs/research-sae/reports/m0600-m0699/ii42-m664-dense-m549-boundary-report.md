# M664 / Dense + M549 Boundary Report

Status: `boundary_constraints_negative_retrieval_gain_suppressed`

M664 follows M663.  M663 added direct M549 boundary preservation and moved the
trained-checkpoint blocker from M549 geometry to dense overlap.  M664 therefore
adds both boundary constraints:

```text
M661 missing-positive boundary objective
+ M549 top100 boundary preservation
+ dense-root top100 boundary preservation
```

This is still first-stage output-head training.  It does not use BM25, fixed
alpha search, dataset-specific tuning, or a separate reranker.

## Implementation

The trainer now has optional dense teacher boundary arguments:

```text
--dense-boundary-preservation-weight
--dense-boundary-core-k
--dense-boundary-top-k
--dense-boundary-margin
```

M664 uses the same generic `boundary_preservation_loss` for dense root scores
and M549 teacher scores.

| Argument | M664 value |
| --- | ---: |
| `retrieval-safe-rank-mode` | `missing_positive` |
| `support-preservation-weight` | `0.60` |
| `m549-boundary-preservation-weight` | `0.35` |
| `dense-boundary-preservation-weight` | `0.35` |
| `candidate-k` | `384` |
| `max-train-steps` | `220` |

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m664_dense_m549_boundary_spark.sh` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Device | `cpu` in Docker |

Artifact root:

```text
runs/ii42-m664-dense-m549-boundary-v1/
  m664_dense_m549_boundary_seed6641/
```

CUDA failed to initialize inside Docker, so this ran on CPU.

## Trained Epoch Signal

The trained epoch was rejected.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | -0.00009804 |
| Recall@100 | +0.00000000 |
| dense overlap@100 | -0.00026042 |
| MAP@100 | -0.00010995 |
| MRR@20 | -0.00008681 |
| NDCG@10 | -0.00007981 |

The failed checks were:

```text
map_non_negative
recall_positive
```

Dense overlap was fixed compared with M663, but the retrieval gain disappeared.

## Final Test Macro

The final checkpoint is epoch0 fallback because the trained epoch was rejected.
Final metrics are identical to M658 baseline.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.819550 | 1.000000 | 0.476960 | 0.252101 | 0.607211 | 0.536565 |
| M549 target | 0.818932 | 0.996496 | 0.476379 | 0.251670 | 0.607240 | 0.536265 |
| M658 baseline | 0.819453 | 0.995371 | 0.476534 | 0.252131 | 0.607285 | 0.536637 |
| M664 final | 0.819453 | 0.995371 | 0.476534 | 0.252131 | 0.607285 | 0.536637 |

## Geometry Gate

The strict geometry gate reports `geometry_floor_passed`, but this is
fallback-only.  All deltas are zero because no trained checkpoint was selected.

## Diagnosis

M664 resolves M663's dense-overlap issue only by suppressing the missing-positive
retrieval movement.  The result is worse than M661:

1. M661: trained checkpoint selected, Recall positive, geometry failed.
2. M663: trained Recall/MAP signal stronger, dense overlap failed.
3. M664: dense overlap acceptable, but Recall and MAP fail.

This means the simple boundary-preservation family has reached a local stop
condition.  The constraints are not shaping the candidate upper-bound surface;
they are only trading off boundary stability versus small ranking movement.

## Decision

Do not promote M664.

Retain:

1. M661 remains the best positive signal in this sub-branch.
2. M663 proves direct M549 boundary preservation changes the failure mode.
3. M664 proves adding dense boundary preservation kills the retrieval gain.

Stop:

```text
Do not continue this output-head retrieval-pressure sub-branch with more
boundary-loss weight tuning.
```

## Next Step: Candidate Upper-Bound Shape Analysis

The next useful work should inspect candidate upper-bound geometry directly:

1. compare dense root, M549 target, M658, and M661 query rows;
2. identify which queries have M661 Recall gains without CUB gain;
3. identify whether missing positives are absent from candidate_k, present but
   below top100, or swapped by boundary-preservation constraints;
4. decide whether the next model change should target candidate generation
   shape rather than ranking pressure.

This should be a diagnostic/audit stage before another training stage.  If the
audit shows gains are mostly ranking-only and CUB is flat, the next model should
change support shape.  If gains are concentrated in boundary-present positives,
then a narrower boundary-rerank objective may still be worth testing.

## Verification

- `python3 -m py_compile scripts/train_m636_retrieval_constrained_compiler.py scripts/check_m659_bounded_retrieval_geometry.py`
- `python3 -m pytest tests/test_m636_retrieval_constrained_compiler.py tests/test_check_m659_bounded_retrieval_geometry.py -q`
- `bash -n scripts/run_m664_dense_m549_boundary_spark.sh scripts/run_m663_m549_boundary_preserving_spark.sh`
- `git diff --check`
