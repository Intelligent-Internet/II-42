# M578 Query-Adaptive Coordinate-Gain Posting Head

M578 follows the failed M577 global coordinate-gain smoke.  M577 preserved
active support, but both shared and dual global gains selected epoch 0 and did
not produce a usable update.

M578 tests the next smallest more expressive shape:

```text
doc posting: locked identity dense top-k
query posting: locked dense top-k with row-adaptive coordinate gains
loss: unchanged M551 sharp candidate-set KL
```

This keeps the document index fixed and indexable.  At query time, the query
encoder may reweight coordinates based on the query itself, while support and
sign stay tied to the dense-derived posting.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m551_dream_lite_posting_spark.sh
```

New variants:

- `query_adaptive_locked_coordinate_gain`
- `dual_adaptive_locked_coordinate_gain`

The query-adaptive layer applies:

```text
log_gain = clip * tanh(adapter(layer_norm(row)))
posting = normalize(row * exp(log_gain) * locked_support_mask)
```

The adapter is zero-initialized, so epoch 0 is exactly the identity locked
support surface.

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
HIDDEN_DIMS=384
TEACHER_TEMPERATURE=0.025
STUDENT_TEMPERATURE=0.050
COORDINATE_GAIN_CLIP=0.10
```

Runs:

```text
runs/m578_query_smoke_scidocs_seed577/
runs/m578_query_smoke_scidocs_seed578/
```

## Results

### Seed 577

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.26887 | 0.23173 | 0.53846 | 0.24840 | 0.57000 |
| query_adaptive_locked_coordinate_gain | 0.26595 | 0.23286 | 0.53846 | 0.23375 | 0.57000 |

Delta against `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| -0.00292 | +0.00113 | +0.00000 | -0.01465 | +0.00000 |

Training diagnostics:

- selected epoch: `0`
- validation listwise: `[1.088194, 1.088679, 1.089094]`
- validation active recall: `[1.0, 1.0, 1.0]`
- validation support loss: `[0.000001, 0.000004, 0.000008]`

### Seed 578

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.18667 | 0.16896 | 0.57500 | 0.19029 | 0.56900 |
| query_adaptive_locked_coordinate_gain | 0.19307 | 0.17575 | 0.57500 | 0.19205 | 0.56800 |

Delta against `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| +0.00640 | +0.00679 | +0.00000 | +0.00176 | -0.00100 |

Training diagnostics:

- selected epoch: `0`
- validation listwise: `[1.122666, 1.122740, 1.122820]`
- validation active recall: `[1.0, 1.0, 1.0]`
- validation support loss: `[0.000002, 0.000005, 0.000011]`

## Decision

Stop M578 at smoke.  Do not scale to three-task validation.

The apparent positive seed578 retrieval delta is not a learned improvement:
the validation gate selected epoch 0, so the accepted model is the initialized
identity surface.  The same-seed seed577 run reproduces the M577 negative early
rank behavior.  M578 therefore does not provide evidence that query-adaptive
sign-preserving coordinate gains are enough to improve the M551 route.

The useful conclusion is that small multiplicative-gain families are too weak
for the next encoder/posting milestone.  The route should keep M551 as the
stable baseline and move to either:

1. a genuinely larger locked-support residual/adapter with stricter early-rank
   validation, or
2. a closer high-quality frozen-judge signal, but only after a stronger judge
   head probe beats the current M562/M563 evidence.
