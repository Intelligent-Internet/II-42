# SAE M80-A1 Dense Student Smoke Report

Date: 2026-05-21

Status: smoke trainer implemented and executed. This is a pipeline validation
result, not a promotable dense-student checkpoint.

## Purpose

M80-A1 must eventually train a strong dense retrieval student before sparse SAE
compression starts. The first implementation deliberately stays small:

```text
M80-A0 candidate rows
  -> text token dual encoder
  -> dense query/doc vectors
  -> candidate-set multi-positive CE
  -> small dense-teacher KL
```

This smoke does not use the final Snowflake-backed trainable encoder. It
validates the candidate-set loss path, source-family reporting, and checkpoint
output format.

## Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m80_dense_student_smoke.py \
  --corpus-dir /Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0 \
  --output-dir results/sae/m80/dense-student-smoke \
  --epochs 1 \
  --max-train-rows 160 \
  --max-eval-rows 80 \
  --device cpu
```

Output:

```text
results/sae/m80/dense-student-smoke
```

## Smoke Metrics

| Metric | Value |
| --- | ---: |
| Train rows | 160 |
| Eval rows | 73 |
| Recall@1 | 0.0225 |
| Recall@5 | 0.0968 |
| Recall@10 | 0.1613 |
| MRR | 0.4063 |
| NDCG@10 | 0.3796 |

Source-family breakdown:

| Family | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| `beir15_current_eval_surface` | 0.1493 | 0.4436 | 0.4040 |
| `broad_generated_query_surface` | 0.1784 | 0.3527 | 0.3447 |

## Interpretation

The smoke result is intentionally modest. It proves the following pieces now
work in this repo:

- M80-A0 candidate rows can train a text-based dense scorer.
- Multi-positive candidate rows do not collapse from missing positives.
- Dense-teacher scores can be used as auxiliary KL targets.
- Metrics are split by source family, which is required for overfit/collapse
  diagnosis in the larger run.

The next run should move from this token-level smoke encoder to the real
Snowflake/SentenceTransformer-backed M80-A1 encoder. That larger run should be
executed on Spark or another GPU host and must report teacher-neighborhood
overlap, not just candidate-set metrics.

## Next Engineering Step

Port the `diffsae-codex` trainable text encoder pattern into the M80-A1
trainer:

- Snowflake backbone with frozen / last-N / full train modes.
- Candidate-set multi-positive CE.
- Dense-teacher KL over qrel/BM25/dense/random candidates.
- Same source-family split reports as the smoke trainer.
- Best-checkpoint selection on validation source-family robustness, not final
  epoch alone.
