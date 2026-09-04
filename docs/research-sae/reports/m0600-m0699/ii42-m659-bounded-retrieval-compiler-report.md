# M659 / Bounded Retrieval Compiler Report

Status: `positive_recall_signal_rejected_by_geometry_and_map_gate`

M659 tests whether the M658 support-geometry compiler reset can be pushed over
the remaining retrieval gap by adding a small bounded retrieval objective.  It
does not change the first-stage boundary:

```text
frozen 1024-d dense root -> M658 compiler checkpoint -> bounded retrieval update
```

The intended improvement over older M636-style retrieval training is that M659
starts from the accepted M658 checkpoint instead of training a fresh compiler.
The M636 trainer was updated with `--init-checkpoint` so the M658 state is used
as the actual optimization starting point, not only as a comparison baseline.

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m659_bounded_retrieval_compiler_spark.sh` |
| Trainer | `scripts/train_m636_retrieval_constrained_compiler.py` |
| Geometry checker | `scripts/check_m659_bounded_retrieval_geometry.py` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Baseline checkpoint | same M658 checkpoint |
| Compiler mode | `active_locked_bucket_delta` |
| Device | `cpu` in Docker |
| Train steps | `180` |
| Listwise / pairwise weights | `0.35 / 0.15` |
| Faithfulness weight | `2.0` |

Artifact root:

```text
runs/ii42-m659-bounded-retrieval-compiler-v1/
  m659_bounded_retrieval_seed6591/
```

## Current Limitations

1. This is still a 4-task smoke surface, not BEIR15/native DB validation.
2. The run used CPU Docker because CUDA failed to initialize in the container.
   This affects speed, not the interpretation of the smoke result.
3. The final checkpoint is epoch 0 fallback.  The geometry gate therefore
   proves the fallback preserved M658 geometry, not that the trained update is
   acceptable.
4. The report labels the comparison baseline as `m635_baseline` because the
   reused M636 trainer keeps that legacy source name.  In this M659 run that
   row is the M658 checkpoint.

## Training Signal

The only trained epoch produced a meaningful but rejected signal on dev:

| Metric | Delta vs M658 |
| --- | ---: |
| candidate upper bound | +0.00052083 |
| Recall@100 | +0.00004058 |
| dense overlap@100 | -0.00123958 |
| MAP@100 | -0.00054291 |
| MRR@20 | -0.00059800 |
| NDCG@10 | -0.00041193 |

This is the key finding.  Starting from M658 does make the retrieval objective
less destructive than the older free retrieval probes, and it can find a
positive Recall/CUB direction.  But it still buys that direction by moving
dense-neighborhood membership and damaging MAP, so the trained checkpoint is
not promotable.

The dev gate failed on:

```text
dense_overlap_guard
map_non_negative
```

The final decision also failed because no trained checkpoint was selected:

```text
recall_positive
trained_checkpoint_selected
```

## Final Test Macro

Because the trained checkpoint was rejected, final `m636_compiler` is identical
to M658 baseline.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.809694 | 1.000000 | 0.482033 | 0.251896 | 0.624134 | 0.550853 |
| M549 target | 0.809577 | 0.995359 | 0.480473 | 0.251880 | 0.624526 | 0.550996 |
| M658 baseline | 0.810055 | 0.994969 | 0.481194 | 0.252007 | 0.624526 | 0.551077 |
| M659 final | 0.810055 | 0.994969 | 0.481194 | 0.252007 | 0.624526 | 0.551077 |

## Geometry Gate

The geometry checker reports `geometry_floor_passed`, but all deltas are zero
because the selected checkpoint is epoch 0 fallback.

| Floor | Result |
| --- | --- |
| doc active recall | pass |
| query active recall | pass |
| doc support cosine | pass |
| query support cosine | pass |
| top100 overlap vs dense | pass |
| top100 overlap vs M549 | pass |

This gate is still useful: it prevents a bad trained checkpoint from silently
becoming the new baseline.  It is not evidence that the M659 retrieval update
itself passed geometry floors.

## Interpretation

M659 narrows the search space but does not solve the bottleneck.

What is retained:

1. M658 remains the correct first-stage reset branch.
2. Initializing from M658 is necessary; fresh retrieval training was too loose.
3. Bounded retrieval pressure can increase candidate upper bound and Recall.
4. The gate is doing the right thing by rejecting the trained checkpoint.

What is blocked:

1. Retrieval pressure still crosses support/ranking boundaries to get gains.
2. The current loss has no direct penalty for losing dense top100 membership.
3. MAP damage appears before the Recall gain is large enough to matter.
4. Post-hoc gating is not enough; the displacement cost must be in the loss or
   in the candidate selection constraint.

## Next Breakthrough Point: M660

The next experiment should not simply train longer or increase listwise weight.
M659 shows the objective direction is partially right but under-constrained.

M660 should make retrieval improvement displacement-aware:

1. initialize from M658, same as M659;
2. add an explicit top100/topK support-preservation loss against the M658
   baseline, not only a final overlap gate;
3. weight retrieval gains only for queries where the positive is already near a
   safe boundary or where candidate upper-bound improvement does not require
   dropping existing dense-neighborhood candidates;
4. select checkpoints by a multi-objective dev score:
   `Recall/CUB up`, `MAP non-negative`, `dense O@100 floor`, and `trained step`;
5. reject if the only positive signal comes from MAP or overlap regression.

Concretely, M660 should train a support-preserving retrieval objective rather
than a generic listwise/pairwise objective.  If M660 cannot retain the M659
Recall/CUB gain while keeping MAP and dense O@100 flat, then this branch should
stop and the next work should inspect the M658 candidate upper-bound deficit
itself instead of adding more ranking pressure.

## Verification

- `python3 -m py_compile scripts/check_m659_bounded_retrieval_geometry.py scripts/train_m636_retrieval_constrained_compiler.py`
- `python3 -m pytest tests/test_check_m659_bounded_retrieval_geometry.py tests/test_m636_retrieval_constrained_compiler.py -q`
- `bash -n scripts/run_m659_bounded_retrieval_compiler_spark.sh`
- `git diff --check`
