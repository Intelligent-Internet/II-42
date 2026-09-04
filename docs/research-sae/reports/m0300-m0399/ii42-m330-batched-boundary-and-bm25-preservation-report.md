# M330 Batched Boundary And BM25 Preservation Report

## Status

M330 followed up on M329's sampled boundary scorer. The goal was to improve
iteration speed without changing the candidate surface, then test the next
diagnostic blocker: BM25-supported qrel positives being suppressed below
semantic-only non-qrels.

The result is mixed:

- The M330 code path reproduces M329 exactly when `scorer_batch_size=1`.
- Training with optimizer batches is not ready as a promotion path.
- A small BM25-positive preservation term is a real positive signal.
- Larger preservation is a tradeoff, not a monotonic improvement.

## Common Surface

All runs used the same 5-dataset candidate cache and exact dense teacher:

- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`
- checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- candidate cache:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`
- dense teacher:
  `/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json`
- scorer variant: `df_le_0p25`
- M329-compatible knobs:
  `hidden_dim=64`, `teacher_weight=0.15`,
  `teacher_false_positive_weight=0.30`, `boundary_weight=0.35`,
  `boundary_cutoff=10/20/100`, `boundary_negatives_per_cutoff=16`,
  `hard_negatives_per_source=8`.

## Code Change

M330 adds `--scorer-batch-size`.

The default remains `1`, which preserves the pre-M330 optimizer behavior. The
shared forward path can score multiple query candidate pools at once. The
evaluator also batches scorer forward calls internally. This is safe for
inference/evaluation because the model is in eval mode.

Training with `scorer_batch_size > 1` changes optimizer dynamics because one
optimizer step now covers multiple query losses. The experiments below confirm
that this is not a drop-in replacement for M329.

## Aggregate Metrics

Macro heldout metrics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M329 / M330 exact batch1 | 0.7276 | 0.3660 | 0.3347 | 0.2425 |
| M330 batch2, epochs60 | 0.7273 | 0.3633 | 0.3336 | 0.2405 |
| M330 batch4, epochs60 | 0.7280 | 0.3625 | 0.3307 | 0.2391 |
| M330 preserve 0.10 | 0.7276 | 0.3648 | 0.3346 | 0.2426 |
| M330 preserve 0.20 | 0.7265 | 0.3644 | 0.3359 | 0.2424 |

Best heldout event metrics, measured over the combined heldout pool:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M330 exact batch1 | 0.7478 | 0.3569 | 0.3372 | 0.2435 |
| M330 batch2, epochs60 | 0.7478 | 0.3549 | 0.3368 | 0.2419 |
| M330 batch4, epochs60 | 0.7481 | 0.3537 | 0.3334 | 0.2403 |
| M330 preserve 0.10 | 0.7479 | 0.3561 | 0.3374 | 0.2438 |
| M330 preserve 0.20 | 0.7469 | 0.3557 | 0.3389 | 0.2437 |

## Diagnostics

| Run | BM25-supported qrel suppressed | Qrel admitted but low-ranked | Semantic FP over-ranked | Teacher qrel top10 lost |
| --- | ---: | ---: | ---: | ---: |
| M330 exact batch1 | 343 | 166 | 266 | 154 |
| M330 batch2, epochs60 | 342 | 161 | 264 | 156 |
| M330 batch4, epochs60 | 350 | 173 | 274 | 160 |
| M330 preserve 0.10 | 335 | 159 | 265 | 154 |
| M330 preserve 0.20 | 336 | 159 | 262 | 152 |

The BM25-positive preservation term directly improves the intended failure
mode. It reduces BM25-supported qrel suppression without increasing semantic
false positives. However, it also redistributes dataset quality, so a fixed
global weight should not be promoted yet.

## Per-Dataset Preservation Effect

| Dataset | Exact NDCG@10 | Preserve 0.10 NDCG@10 | Preserve 0.20 NDCG@10 |
| --- | ---: | ---: | ---: |
| `arguana` | 0.3381 | 0.3406 | 0.3447 |
| `fiqa` | 0.4031 | 0.3986 | 0.3991 |
| `nfcorpus` | 0.3502 | 0.3486 | 0.3490 |
| `scidocs` | 0.1845 | 0.1816 | 0.1818 |
| `scifact` | 0.6544 | 0.6630 | 0.6580 |

Preservation helps `arguana` and `scifact`, but hurts `fiqa` and `scidocs`.
That is the key reason not to hard-code a larger global preservation weight.

## Runtime Finding

The scorer task is effectively CPU-bound in this environment. `nvidia-smi` did
not show a PyTorch GPU process while the scorer was training, and the Python
process used about one CPU core. The current model is small enough that this is
not surprising, but it means batch-size experiments are mostly CPU/Python
execution-shape experiments, not GPU utilization experiments.

The slow repeated parts are now:

- dataset metadata/qrel loading,
- `torch.load` of large candidate caches, especially `fiqa`,
- per-query loss construction and optimizer stepping.

Candidate cache is useful but not enough for rapid scorer sweeps. The next
infrastructure step should be a scorer-ready examples cache that stores the
already-built `QueryExample` tensors for a fixed candidate cache and teacher.

## Decision

Do not promote `scorer_batch_size > 1` for quality runs. Use it only for quick
canaries if needed.

Keep the batched evaluator/scoring code because batch1 reproduces M329 exactly
and the default is behavior-preserving.

Promote BM25-positive preservation as the next modeling direction, but not as a
fixed global scalar. The useful next step is a runtime/query-feature-aware
preservation gate:

- increase preservation when BM25 has strong concentration and SAE-only top
  docs are mostly teacher false positives,
- reduce preservation for semantic-heavy queries where BM25 top docs are weak,
- train/evaluate this as a small scorer feature or loss gate, not as
  dataset-specific tuning.

## Artifacts

- Exact parity:
  `/home/huoju/leask/runs/ii42-m330-batched-boundary-v1/m330_batch1_exact_m329_seed1050.json`
- Batch2:
  `/home/huoju/leask/runs/ii42-m330-batched-boundary-v1/m330_batch2_epochs60_seed1050.json`
- Batch4:
  `/home/huoju/leask/runs/ii42-m330-batched-boundary-v1/m330_batch4_epochs60_seed1050.json`
- Preservation 0.10:
  `/home/huoju/leask/runs/ii42-m330-batched-boundary-v1/m330_bm25_preserve010_seed1050.json`
- Preservation 0.20:
  `/home/huoju/leask/runs/ii42-m330-batched-boundary-v1/m330_bm25_preserve020_seed1050.json`
