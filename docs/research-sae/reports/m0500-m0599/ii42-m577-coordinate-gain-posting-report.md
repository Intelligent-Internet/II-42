# M577 Coordinate-Gain Posting Head

M577 tests a minimal model-form change after M576 showed that adding another
candidate-set support proxy was not a good promotion path.

The hypothesis is intentionally narrow:

```text
keep M551 sharp candidate-set KL
keep locked dense top-k support
learn only bounded per-coordinate multiplicative gains
```

This is a cleaner test of the output-layer route than another loss term.  If
it works, it should improve candidate-set fit without changing active support.
If it fails, the result says that a simple diagonal/gain output head is not
enough for the M551-family encoder.

## Implementation

Code paths:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m551_dream_lite_posting_spark.sh
```

New variants:

- `shared_locked_coordinate_gain`
- `dual_locked_coordinate_gain`

The head selects the locked active support from the original dense row, then
applies:

```text
posting = normalize(dense_row * exp(clamp(log_gain, -clip, clip)) * mask)
```

The active support mask is therefore fixed.  Training can only change
coordinate magnitudes inside that support.

New argument:

- `--coordinate-gain-clip`

Runner environment:

- `COORDINATE_GAIN_CLIP`

## Smoke

Both smoke runs completed on `spark-1` with GPU docker enabled.

Common config:

```text
TASKS=SCIDOCS
QUERY_LIMIT=120
DOC_LIMIT=2000
EPOCHS=3
MAX_TRAIN_GROUPS=64
TEACHER_POOL_K=32
RANDOM_NEGATIVES=32
TEACHER_TEMPERATURE=0.025
STUDENT_TEMPERATURE=0.050
COORDINATE_GAIN_CLIP=0.10
```

Runs:

```text
runs/m577_smoke_scidocs_seed577/
runs/m577_dual_smoke_scidocs_seed577/
```

### Shared Gain

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.26887 | 0.23173 | 0.53846 | 0.24840 | 0.57000 |
| shared_locked_coordinate_gain | 0.26595 | 0.23286 | 0.53846 | 0.23375 | 0.57000 |

Delta against `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| -0.00292 | +0.00113 | +0.00000 | -0.01465 | +0.00000 |

Training diagnostics:

- selected epoch: `0`
- validation listwise: `[0.857692, 0.857713, 0.857727]`
- validation active recall: `[1.0, 1.0, 1.0]`
- validation support loss: `[0.0, 0.000001, 0.000001]`
- coordinate gain summary: all gains stayed at `1.0` because the gate restored
  epoch 0.

### Dual Gain

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.26887 | 0.23173 | 0.53846 | 0.24840 | 0.57000 |
| dual_locked_coordinate_gain | 0.26595 | 0.23286 | 0.53846 | 0.23375 | 0.57000 |

Delta against `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| -0.00292 | +0.00113 | +0.00000 | -0.01465 | +0.00000 |

Training diagnostics:

- selected epoch: `0`
- validation listwise: `[1.045682, 1.045704, 1.045700]`
- validation active recall: `[1.0, 1.0, 1.0]`
- validation support loss: `[0.0, 0.000001, 0.000001]`
- coordinate gain summary: all gains stayed at `1.0`.

## Decision

Stop M577 at smoke.  Do not run the three-task or broad validation.

The positive part is that the implementation preserves active support exactly.
The negative part is decisive: validation chooses epoch 0 for both shared and
dual gain heads, so the learned coordinate gains are not providing a usable
candidate-set improvement under the M551 objective.  The observed held-out
ranking also weakens early rank, especially `MRR@20`.

This means a simple bounded diagonal output head is too weak as the next
encoder/posting step.  The route should continue from M551, but the next model
change needs more expressive query-doc interaction than coordinate-wise global
gain, while still keeping the M551 support/active gates as the safety surface.
