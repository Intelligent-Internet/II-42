# M661 / Missing-Positive Boundary Report

Status: `retrieval_gate_passed_geometry_floor_failed`

M661 continues the M659/M660 sequence.

M659 showed that retrieval pressure from M658 can produce a small Recall/CUB
signal but damages MAP and overlap.  M660 added support preservation, but it
used the best positive rank and therefore mostly trained already-found positives.
M661 changes the training gate:

```text
apply retrieval pressure only when a relevant document is outside baseline
top100 and near the top100 boundary
```

This is still first-stage output-head training.  It does not use BM25, fixed
alpha search, dataset-specific tuning, or a separate reranker.

## Implementation

The M636 trainer now supports:

```text
--retrieval-safe-rank-mode missing_positive
```

In this mode, the retrieval weight is based on the nearest relevant document
outside the baseline topK boundary, not the best relevant document.  If all
relevant documents are already inside baseline topK, retrieval pressure is set
to zero for that query.

M661 also protects the full baseline top100 rather than only the top64 core.

| Argument | M661 value |
| --- | ---: |
| `retrieval-safe-rank-mode` | `missing_positive` |
| `support-preservation-core-k` | `100` |
| `support-preservation-top-k` | `100` |
| `support-preservation-weight` | `0.60` |
| `retrieval-safe-slack-window` | `96` |
| `retrieval-safe-min-weight` | `0.0` |
| `candidate-k` | `384` |
| `listwise-weight` | `0.35` |
| `pairwise-weight` | `0.15` |
| `faithfulness-weight` | `2.0` |
| `max-train-steps` | `220` |

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m661_missing_positive_boundary_spark.sh` |
| Trainer | `scripts/train_m636_retrieval_constrained_compiler.py` |
| Geometry checker | `scripts/check_m659_bounded_retrieval_geometry.py` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Device | `cpu` in Docker |

Artifact root:

```text
runs/ii42-m661-missing-positive-boundary-v1/
  m661_missing_boundary_seed6611/
```

CUDA again failed to initialize inside the Docker container, so this smoke ran
on CPU.

## Training Trace

M661 selected a trained checkpoint.  This is the first accepted retrieval smoke
in this branch after the epoch0 fallback fix.

| Diagnostic | Value |
| --- | ---: |
| selected epoch | `1` |
| selected step | `220` |
| average best positive baseline rank | `15.51363636` |
| average missing positive baseline rank | `320.66363636` |
| average retrieval weight | `0.15383523` |
| support-preservation loss | `0.64455284` |

The low retrieval weight is expected: most qrels are either already found or
too far outside the top100 boundary.  The key change is that M661 does not spend
most of the gradient on already-found positives.

## Dev Gate

The trained checkpoint passed the smoke gate on dev.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | -0.00009804 |
| Recall@100 | +0.00004058 |
| dense overlap@100 | -0.00011458 |
| MAP@100 | +0.00042188 |
| MRR@20 | +0.00041733 |
| NDCG@10 | +0.00011857 |

## Test Gate

The final test decision also passed the retrieval smoke gate.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | -0.00001819 |
| Recall@100 | +0.00039062 |
| dense overlap@100 | -0.00050781 |
| MAP@100 | +0.00001091 |
| MRR@20 | +0.00000113 |
| NDCG@10 | -0.00055532 |

The NDCG regression is within the configured tolerance, but it is still a real
warning.  M661 improves Recall without materially damaging MAP/MRR, but it has
not yet become a clean all-metric win.

## Test Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.804952 | 1.000000 | 0.480560 | 0.259461 | 0.608111 | 0.539072 |
| M549 target | 0.804587 | 0.995816 | 0.480023 | 0.259546 | 0.608517 | 0.539132 |
| M658 baseline | 0.804697 | 0.995406 | 0.480693 | 0.259830 | 0.608517 | 0.539359 |
| M661 compiler | 0.804679 | 0.994898 | 0.480138 | 0.259841 | 0.608907 | 0.539360 |

## Geometry Gate

The strict geometry gate failed.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| doc active recall | +0.00000000 |
| query active recall | +0.00000000 |
| doc support cosine | -0.00000289 |
| query support cosine | -0.00000307 |
| top100 overlap vs dense | +0.00000000 |
| top100 overlap vs M549 | -0.00045156 |

This is not acceptable for promotion as the first-stage frozen baseline.  The
important nuance is that dense top100 overlap and active recall did not regress;
the geometry failure is a tiny support-cosine regression and an M549 overlap
regression.  That makes the result much more promising than M659/M660, but it
still needs a geometry-tight follow-up before expansion.

## Decision

Keep M661 as the first real positive retrieval signal in this branch, but do
not promote its checkpoint.

What is now proven:

1. `missing_positive` boundary gating is the correct direction for Recall@100
   recovery.
2. The previous `best_positive` gate was training the wrong queries.
3. A trained checkpoint can pass the retrieval smoke gate after the epoch0
   fallback fix.

What remains blocked:

1. strict support geometry still regresses slightly;
2. NDCG is not positive on test;
3. candidate upper bound does not improve, so the gain is ranking within the
   available candidate set, not broader candidate generation;
4. this is still a 4-task smoke, not full/native validation.

## Next Step: M662

M662 should keep the M661 missing-positive objective but tighten geometry:

1. reduce learning rate or train step budget;
2. increase `faithfulness-weight` and `support-preservation-weight`;
3. preserve full top100 and possibly top128 against M658;
4. add support-cosine or M549-overlap as a training penalty if parameter-only
   tightening is not enough;
5. require both retrieval smoke pass and geometry floor pass.

If M662 keeps the M661 Recall gain while passing geometry, then the branch
should expand to a larger smoke surface before any BEIR15/native DB work.  If
the Recall gain disappears under geometry-tight settings, this branch should
stop and the next investigation should focus on candidate upper-bound shape
rather than output-head ranking pressure.

## Verification

- `python3 -m py_compile scripts/train_m636_retrieval_constrained_compiler.py scripts/check_m659_bounded_retrieval_geometry.py`
- `python3 -m pytest tests/test_m636_retrieval_constrained_compiler.py tests/test_check_m659_bounded_retrieval_geometry.py -q`
- `bash -n scripts/run_m661_missing_positive_boundary_spark.sh scripts/run_m660_support_preserving_retrieval_spark.sh`
- `git diff --check`
