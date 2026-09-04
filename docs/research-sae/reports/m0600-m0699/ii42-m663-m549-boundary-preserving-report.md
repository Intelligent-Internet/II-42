# M663 / M549 Boundary Preserving Report

Status: `m549_boundary_negative_dense_overlap_block`

M663 follows M661/M662.  M661 proved that missing-positive boundary gating can
recover Recall@100 with a trained checkpoint, but it failed strict M549 geometry
floors.  M662 showed that generic parameter tightening does not fix that
geometry blocker.  M663 therefore adds a direct loss for the failed quantity:

```text
M661 missing-positive boundary objective
+ M549 top100 boundary preservation loss
```

This is still first-stage output-head training.  It does not use BM25, fixed
alpha search, dataset-specific tuning, or a separate reranker.

## Implementation

The trainer now has an optional teacher-boundary loss:

```text
--m549-boundary-preservation-weight
--m549-boundary-core-k
--m549-boundary-top-k
--m549-boundary-margin
```

The loss keeps M549 teacher topK/core documents above documents outside the M549
teacher topK boundary within each query candidate set.

| Argument | M663 value |
| --- | ---: |
| `retrieval-safe-rank-mode` | `missing_positive` |
| `support-preservation-core-k` | `100` |
| `support-preservation-top-k` | `100` |
| `support-preservation-weight` | `0.60` |
| `m549-boundary-preservation-weight` | `0.45` |
| `m549-boundary-core-k` | `100` |
| `m549-boundary-top-k` | `100` |
| `candidate-k` | `384` |
| `max-train-steps` | `220` |

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m663_m549_boundary_preserving_spark.sh` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Device | `cpu` in Docker |

Artifact root:

```text
runs/ii42-m663-m549-boundary-preserving-v1/
  m663_m549_boundary_seed6631/
```

CUDA failed to initialize inside Docker, so this ran on CPU.

## Trained Epoch Signal

The trained epoch found a strong retrieval direction but was rejected by the
smoke gate.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | +0.00000000 |
| Recall@100 | +0.00056064 |
| dense overlap@100 | -0.00110938 |
| MAP@100 | +0.00019952 |
| MRR@20 | +0.00014994 |
| NDCG@10 | +0.00011412 |

The only failed check was:

```text
dense_overlap_guard
```

This is an important diagnostic.  M661 failed M549 geometry.  M663 adds M549
boundary pressure and the remaining trained-checkpoint blocker moves to dense
overlap.  That suggests the objective must preserve both dense and M549 top100
boundaries.

## Final Test Macro

The final checkpoint is epoch0 fallback because the trained epoch was rejected.
Therefore final metrics are identical to M658 baseline.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.839417 | 1.000000 | 0.506158 | 0.281102 | 0.647503 | 0.589581 |
| M549 target | 0.839358 | 0.995699 | 0.506018 | 0.280993 | 0.647418 | 0.590245 |
| M658 baseline | 0.839651 | 0.995000 | 0.505889 | 0.281069 | 0.647647 | 0.590251 |
| M663 final | 0.839651 | 0.995000 | 0.505889 | 0.281069 | 0.647647 | 0.590251 |

## Geometry Gate

The geometry checker reports `geometry_floor_passed`, but all deltas are zero
because final checkpoint is epoch0 fallback.

This is not evidence that the trained M663 checkpoint passed geometry.  It only
proves that the rollback checkpoint remains safe.

## Decision

Do not promote M663.

What is retained:

1. Direct M549 boundary preservation is a valid way to address the M661/M662
   M549-geometry blocker.
2. The trained epoch still recovers Recall and improves MAP/MRR/NDCG.
3. The remaining trained-checkpoint blocker is now dense top100 overlap.

What remains blocked:

1. trained dense overlap regresses by `-0.00110938`, just beyond the configured
   `0.001` tolerance;
2. final geometry pass is fallback-only;
3. candidate upper bound is flat, so the gain is still ranking within the
   existing candidate set.

## Next Step: M664

M664 should not abandon the branch.  The data now points to a specific fix:

```text
M661 missing-positive boundary
+ M549 top100 boundary preservation
+ dense-root top100 boundary preservation
```

The next probe should add a dense teacher boundary loss using the same
`boundary_preservation_loss` helper, with dense root scores as the teacher.  The
goal is to keep the M663 Recall/MAP signal while pulling dense overlap
regression under the hard gate.

Acceptance for M664:

1. trained checkpoint selected;
2. retrieval smoke gate passed;
3. strict geometry gate passed on the trained checkpoint;
4. Recall@100 positive and MAP non-negative.

If M664 cannot pass both dense and M549 boundary constraints, this
retrieval-pressure output-head sub-branch should pause and the next work should
move to candidate-upper-bound shape analysis.

## Verification

- `python3 -m py_compile scripts/train_m636_retrieval_constrained_compiler.py scripts/check_m659_bounded_retrieval_geometry.py`
- `python3 -m pytest tests/test_m636_retrieval_constrained_compiler.py tests/test_check_m659_bounded_retrieval_geometry.py -q`
- `bash -n scripts/run_m663_m549_boundary_preserving_spark.sh scripts/run_m662_geometry_tight_boundary_spark.sh`
- `git diff --check`
