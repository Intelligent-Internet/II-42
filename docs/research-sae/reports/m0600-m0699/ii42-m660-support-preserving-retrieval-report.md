# M660 / Support-Preserving Retrieval Report

Status: `support_preservation_negative_missing_positive_boundary_needed`

M660 follows the M659 result.  M659 showed that bounded retrieval pressure from
the M658 checkpoint can improve Recall/CUB, but only by paying dense-overlap and
MAP cost.  M660 therefore adds a baseline-aware displacement penalty:

```text
M658 checkpoint
  + retrieval listwise/pairwise pressure
  + M658 topK/core support-preservation loss
  + safe weighting by baseline positive rank
```

This is still first-stage output-head work.  It does not add BM25, alpha search,
dataset-specific tuning, or a separate reranker.

## Implementation

The existing M636 trainer was extended with optional arguments.  Defaults keep
old behavior unchanged.

| Argument | M660 value |
| --- | ---: |
| `support-preservation-weight` | `0.35` |
| `support-preservation-core-k` | `64` |
| `support-preservation-top-k` | `100` |
| `retrieval-safe-slack-window` | `64` |
| `retrieval-safe-min-weight` | `0.05` |
| `listwise-weight` | `0.25` |
| `pairwise-weight` | `0.10` |
| `faithfulness-weight` | `2.0` |
| `max-train-steps` | `180` |

The support-preservation loss protects the baseline top64 core from documents
outside baseline top100.  Retrieval loss is downweighted when positives are far
outside the baseline top100 boundary.

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m660_support_preserving_retrieval_spark.sh` |
| Trainer | `scripts/train_m636_retrieval_constrained_compiler.py` |
| Geometry checker | `scripts/check_m659_bounded_retrieval_geometry.py` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Device | `cpu` in Docker |

Artifact root:

```text
runs/ii42-m660-support-preserving-retrieval-v1/
  m660_support_preserving_seed6601/
```

CUDA again failed to initialize in the Docker container.  This affects runtime,
not the result semantics.

## Training Signal

The only trained epoch was rejected.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | -0.00008409 |
| Recall@100 | +0.00000000 |
| dense overlap@100 | -0.00155208 |
| MAP@100 | +0.00060100 |
| MRR@20 | +0.00056320 |
| NDCG@10 | +0.00029748 |

The gate failed on:

```text
dense_overlap_guard
recall_positive
```

The final checkpoint is epoch0 fallback, so final test metrics are identical to
M658 baseline.

## Test Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.798953 | 1.000000 | 0.497172 | 0.257712 | 0.609112 | 0.562539 |
| M549 target | 0.799301 | 0.995617 | 0.496487 | 0.257641 | 0.610262 | 0.563399 |
| M658 baseline | 0.799307 | 0.994859 | 0.496860 | 0.257638 | 0.610262 | 0.563411 |
| M660 final | 0.799307 | 0.994859 | 0.496860 | 0.257638 | 0.610262 | 0.563411 |

## Diagnosis

M660 did not solve the M659 blocker.  It changed the failure mode:

1. M659 found small Recall/CUB improvement but damaged MAP and overlap.
2. M660 found MAP/NDCG/MRR improvement but lost the Recall/CUB gain and damaged
   overlap even more.

The trace explains why:

| Diagnostic | Value |
| --- | ---: |
| average best positive baseline rank | `15.52777778` |
| average retrieval weight | `0.97361111` |
| support-preservation loss | `0.64036569` |

The "safe" weighting used the best positive rank.  On this smoke surface, the
best positive is usually already inside top100.  Therefore M660 mostly trains
ordering of already-found positives rather than promoting missing positives
from the top100 boundary.  That explains why MAP/NDCG/MRR can rise while
Recall@100 stays flat.

The top64 core preservation is also too weak to protect dense overlap@100.  It
allows boundary swaps in ranks 65-100, and those swaps are exactly where the
dense-overlap guard fails.

## Decision

Do not promote M660.  Keep the code because it adds useful instrumentation and
a reusable baseline-aware loss, but treat this run as a negative result.

The key retained finding is diagnostic:

```text
best-positive rank is the wrong training gate for Recall@100 recovery
```

## Next Breakthrough Point: M661

M661 should not use best-positive gating.  It should target missing positives:

1. compute the nearest relevant document outside baseline top100;
2. apply retrieval loss only when that missing positive is within a boundary
   window, for example ranks 101-192;
3. protect the full baseline top100, not only top64 core, or use a stronger
   boundary-pair loss for ranks 65-100;
4. keep MAP/NDCG/MRR as guards, not as the primary training direction;
5. require a trained checkpoint with positive Recall@100 and non-regressed
   dense overlap/MAP before expansion.

If M661 cannot recover the M659 Recall/CUB signal while preserving M660's MAP
stability, then the retrieval-constrained output-head branch should pause.  The
next layer would be candidate upper-bound shape analysis rather than more
generic listwise training.

## Verification

- `python3 -m py_compile scripts/train_m636_retrieval_constrained_compiler.py scripts/check_m659_bounded_retrieval_geometry.py`
- `python3 -m pytest tests/test_m636_retrieval_constrained_compiler.py tests/test_check_m659_bounded_retrieval_geometry.py -q`
- `bash -n scripts/run_m660_support_preserving_retrieval_spark.sh scripts/run_m659_bounded_retrieval_compiler_spark.sh`
- `git diff --check`
