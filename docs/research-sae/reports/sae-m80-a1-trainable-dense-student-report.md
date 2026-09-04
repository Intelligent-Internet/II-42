# SAE M80-A1 Trainable Dense Student Report

Date: 2026-05-21

Status: trainable SentenceTransformer/Snowflake-backed dense-student entrypoint
implemented and smoke-validated. This is the real M80-A1 training harness, not
yet a promoted Stage-A model.

## Summary

M80-A1 now has a trainer that can run the intended Stage-A dense representation
step:

```text
M80-A0 candidate rows
  -> SentenceTransformer / Snowflake text encoder
  -> projection head
  -> normalized dense query/doc vectors
  -> multi-positive candidate CE
  -> dense-teacher KL
  -> optional embedding-shape distillation
```

The script is:

```text
scripts/research_sae_m80_trainable_dense_student.py
```

It supports:

- `frozen`, `last_n`, and `full` encoder training modes;
- `truncate`, `linear`, and `mlp` projection heads;
- Snowflake query prefix default: `query: `;
- `eager`, `sdpa`, or `flash_attention_2` attention selection;
- default Snowflake-safe config overrides:
  `use_memory_efficient_attention=false` and `unpad_inputs=false`;
- batched candidate rows with variable candidate counts;
- multi-positive candidate CE and dense-teacher KL;
- source-family metrics for collapse/overfit diagnosis.
- `--no-save-checkpoint` for load/smoke runs that should not materialize a
  full 1GB+ model state.
- chunked text encoding via `--encode-batch-size`, which is required because
  each candidate row can contain up to 64 documents.
- eval-only frozen baselines, including zero-trainable-parameter
  `epochs=0` runs.
- best-epoch tracking by metric and optional source family, so long curves do
  not silently promote a worse final epoch.

## Why The Snowflake Override Matters

The first Snowflake local smoke failed before training because the cached
Snowflake remote model code tried to enable memory-efficient attention and
asserted that `xformers` must be installed. That is the wrong default for CPU
or ordinary Mac smoke tests.

The trainer now follows the safer `diffsae-codex` pattern:

```text
if Snowflake and disable_memory_efficient_attention:
    use_memory_efficient_attention = false

if Snowflake and disable_unpad_inputs:
    unpad_inputs = false
```

Spark or other NVIDIA hosts can explicitly choose optimized paths only if the
underlying HuggingFace architecture supports them. The Snowflake/GTE remote
model currently rejects `flash_attention_2`, so the stable path for this model
is the Snowflake-safe default config, CUDA bf16 autocast, and no forced FA2.

## Smoke Runs

### MiniLM trainable-path smoke

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m80_trainable_dense_student.py \
  --corpus-dir /Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-smoke-small \
  --output-dir results/sae/m80/trainable-dense-student-minilm-smoke \
  --model-id sentence-transformers/all-MiniLM-L6-v2 \
  --no-trust-remote-code \
  --encoder-train-mode frozen \
  --projection-mode linear \
  --output-dim 128 \
  --epochs 1 \
  --batch-rows 1 \
  --max-train-rows 4 \
  --max-eval-rows 4 \
  --device cpu \
  --no-save-checkpoint
```

Result:

| Metric | Value |
| --- | ---: |
| Train rows | 4 |
| Eval rows | 4 |
| Trainable parameters | 49,280 |
| Recall@10 | 0.5781 |
| MRR | 0.6382 |
| NDCG@10 | 0.6577 |

### Snowflake load/eval smoke

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m80_trainable_dense_student.py \
  --corpus-dir /Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-smoke-small \
  --output-dir results/sae/m80/trainable-dense-student-snowflake-load-smoke \
  --model-id Snowflake/snowflake-arctic-embed-m-v2.0 \
  --encoder-train-mode frozen \
  --projection-mode linear \
  --output-dim 256 \
  --epochs 0 \
  --batch-rows 1 \
  --max-train-rows 1 \
  --max-eval-rows 1 \
  --device cpu \
  --no-save-checkpoint
```

Result:

| Metric | Value |
| --- | ---: |
| Train rows | 1 |
| Eval rows | 1 |
| Trainable parameters | 196,864 |
| Recall@10 | 0.3125 |
| MRR | 1.0000 |
| NDCG@10 | 1.0000 |

This only proves the Snowflake entrypoint loads and scores successfully after
the attention config fix. The one-row metric is not a quality claim.

## Spark M80-A1 Findings

The first Spark runs used:

```text
host = huoju@100.123.2.95
container = leask/ii42-sae:pytorch26.03-aim
corpus = /home/huoju/leask/data/ii42_sae_m80/neutral-stage-a-v0
train_rows = 367
eval_rows = 73
```

### 256-dimensional projection run

Run:

```text
m80-a1-snowflake-last2-linear-v0-e3-progress
output_dim = 256
projection = linear
train_mode = last_n, last 2 layers
epochs = 3
lr = 2e-5
projection_lr = 2e-4
teacher_kl = 0.20
embedding_distill = 0.05
```

Result:

| Epoch | All Recall@10 | All MRR | All NDCG@10 | BEIR Recall@10 | BEIR MRR | BEIR NDCG@10 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.5743 | 0.8750 | 0.8231 | 0.5118 | 0.8107 | 0.7802 |
| 2 | 0.5754 | 0.8706 | 0.8178 | 0.5106 | 0.8032 | 0.7694 |
| 3 | 0.5616 | 0.8544 | 0.8053 | 0.4819 | 0.7988 | 0.7549 |

Interpretation:

- the training loss fell, but BEIR current-surface quality degraded after the
  first epoch;
- the 256 random projection head is not the right first promotion path for
  Stage A;
- more epochs on this exact configuration would optimize the narrow candidate
  rows while damaging the teacher-shaped retrieval surface.

### Frozen teacher ceiling

Run:

```text
m80-a1-snowflake-frozen768-teacher-baseline
output_dim = 768
projection = truncate
train_mode = frozen
epochs = 0
```

Result:

| Split | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| All eval rows | 0.5830 | 0.8836 | 0.8337 |
| BEIR current eval surface | 0.5167 | 0.8599 | 0.7942 |
| Broad generated query surface | 0.6781 | 0.9175 | 0.8904 |

This is the current teacher-geometry ceiling for the M80-A0 V0 candidate rows.

### 768-dimensional low-LR trainable run

Run:

```text
m80-a1-snowflake-last2-truncate768-lr5e6-e2-nosave
output_dim = 768
projection = truncate
train_mode = last_n, last 2 layers
epochs = 2
lr = 5e-6
teacher_kl = 0.20
embedding_distill = 0.20
```

Result:

| Split | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| All eval rows | 0.5844 | 0.8859 | 0.8357 |
| BEIR current eval surface | 0.5190 | 0.8638 | 0.7980 |
| Broad generated query surface | 0.6781 | 0.9175 | 0.8899 |

This is the first positive M80-A1 signal: preserve the full 768-dimensional
teacher shape, avoid a random projection head, keep the learning rate small,
and use candidate supervision as a light residual rather than the dominant
objective.

### 768-dimensional long-curve check

Run:

```text
m80-a1-snowflake-last2-truncate768-lr5e6-e6-beir-select
output_dim = 768
projection = truncate
train_mode = last_n, last 2 layers
epochs = 6
selection = beir15_current_eval_surface / ndcg_at_10
```

Result:

| Epoch | All Recall@10 | All MRR | All NDCG@10 | BEIR Recall@10 | BEIR MRR | BEIR NDCG@10 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.5816 | 0.8836 | 0.8325 | 0.5144 | 0.8599 | 0.7925 |
| 2 | 0.5844 | 0.8859 | 0.8357 | 0.5190 | 0.8638 | 0.7980 |
| 3 | 0.5828 | 0.8855 | 0.8337 | 0.5163 | 0.8632 | 0.7946 |
| 4 | 0.5830 | 0.8787 | 0.8317 | 0.5167 | 0.8517 | 0.7926 |
| 5 | 0.5830 | 0.8788 | 0.8314 | 0.5167 | 0.8518 | 0.7927 |
| 6 | 0.5967 | 0.8768 | 0.8330 | 0.5399 | 0.8485 | 0.7966 |

Interpretation:

- epoch 2 is the current best selection point for ranking quality;
- later epochs improve Recall@10 but reduce MRR, so candidate CE is widening
  coverage while damaging top-rank calibration;
- the next promotable checkpoint should save the epoch-2 profile and then move
  to sparse-preservation diagnostics, not keep training the same candidate-row
  objective.

## Current Decision

M80-A1 should continue from the 768 truncate route, not the 256 projection
route. The next run is a longer 768 curve with best-epoch selection on the
BEIR current-surface NDCG. The first long-curve check peaks at epoch 2. A
saved-best rerun completed successfully:

```text
run = m80-a1-snowflake-last2-truncate768-lr5e6-e2-savebest
checkpoint = /home/huoju/leask/runs/m80-a1-snowflake-last2-truncate768-lr5e6-e2-savebest/trainable_dense_student.best.pt
best_epoch = 2
all_eval = Recall@10 0.5844, MRR 0.8859, NDCG@10 0.8357
beir_current = Recall@10 0.5190, MRR 0.8638, NDCG@10 0.7980
```

This is the current M80-A1 checkpoint candidate before M80-A2 sparse
preservation.

## Next Step

The next meaningful run is:

```text
model = Snowflake/snowflake-arctic-embed-m-v2.0
train_mode = last_n
projection = truncate
output_dim = 768
corpus = /Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0
rows = all available M80-A0 candidate rows first
selection = BEIR current-surface NDCG@10
checkpoint = saved-best epoch 2 profile
```

Acceptance evidence for the Spark run must include:

- validation candidate recall/MRR/NDCG;
- source-family deltas;
- teacher-neighborhood overlap;
- checkpoint selected by validation robustness, not final epoch alone;
- follow-up M80-A2 sparse-tax evaluation before any Stage-B ranking work.
