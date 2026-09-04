# SAE Text-To-Sparse-Atoms Training Report

Date: 2026-05-13

## Scope

This report records the first execution of the direct text-to-sparse-atoms
training plan:

```text
text -> sparse atom ids + atom weights
```

The teacher remains the current SAE-over-dense path:

```text
text -> Snowflake 768 embedding -> SAE 8192/64 -> sparse atoms
```

The student is evaluated as a serving-path replacement for the dense teacher
encoder, not as a vector search layer.

## Implementation

New trainer:

```text
scripts/research_sae_text_atom_train.py
```

The first student model is intentionally simple:

```text
tokenized text
  -> learned token embeddings
  -> masked mean pooling
  -> shared trunk
  -> asymmetric document/query atom heads
  -> top-k sparse atoms
```

The model has separate document and query heads for support logits and atom
weights. Training uses:

- support distillation against teacher active atoms;
- active-value regression against teacher atom weights;
- fixed student active atom budget;
- separate teacher and student SAE score weights at evaluation time.

The trainer writes:

```text
results/sae/text-atoms/{run}/text_atom_student.pt
results/sae/text-atoms/{run}/student_doc_latents.jsonl
results/sae/text-atoms/{run}/student_query_latents.jsonl
results/sae/text-atoms/{run}/text_atom_training_matrix.json
results/sae/text-atoms/{run}/summary.md
```

## Data

The first run uses the existing sampled BEIR matrix artifacts:

```text
data_root = /tmp/ii42_sae_quality_matrix
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
teacher = sae_8192_64
```

Dataset sizes:

| Dataset | Docs | Queries | Qrels |
| --- | ---: | ---: | ---: |
| `scifact` | 2000 | 100 | 116 |
| `scidocs` | 2000 | 100 | 492 |
| `nfcorpus` | 2063 | 100 | 3818 |
| `arguana` | 2000 | 100 | 100 |
| `fiqa` | 2000 | 100 | 267 |

## Main Run

Command:

```bash
python3 scripts/research_sae_text_atom_train.py \
  --output-dir results/sae/text-atoms/t2-baseline-calibrated \
  --epochs 12 \
  --batch-size 96 \
  --hidden-dim 256 \
  --max-vocab 50000 \
  --student-sae-weight 0.25 \
  --device mps
```

The teacher path keeps the current `sae_weight = 2.0`. The student path uses
`student_sae_weight = 0.25` because the first sweep showed that raw student atom
scores are not calibrated to the teacher score scale yet.

Run artifact:

```text
results/sae/text-atoms/t2-baseline-calibrated/summary.md
```

## Quality Matrix

| Dataset | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `scifact` | `bm25` | 0.9517 | 0.7803 | 0.7982 | 0.7578 |
| `scifact` | `bm25_teacher_sae` | 0.9800 | 0.8102 | 0.8234 | 0.7940 |
| `scifact` | `bm25_student_atoms` | 0.9700 | 0.8097 | 0.8191 | 0.7861 |
| `scidocs` | `bm25` | 0.5720 | 0.5524 | 0.3405 | 0.2506 |
| `scidocs` | `bm25_teacher_sae` | 0.7040 | 0.6624 | 0.4418 | 0.3406 |
| `scidocs` | `bm25_student_atoms` | 0.5915 | 0.5703 | 0.3525 | 0.2648 |
| `nfcorpus` | `bm25` | 0.2660 | 0.6106 | 0.3730 | 0.1834 |
| `nfcorpus` | `bm25_teacher_sae` | 0.3763 | 0.6850 | 0.4478 | 0.2464 |
| `nfcorpus` | `bm25_student_atoms` | 0.3257 | 0.6206 | 0.3982 | 0.2053 |
| `arguana` | `bm25` | 0.9700 | 0.4615 | 0.5346 | 0.4643 |
| `arguana` | `bm25_teacher_sae` | 1.0000 | 0.5126 | 0.6245 | 0.5126 |
| `arguana` | `bm25_student_atoms` | 1.0000 | 0.4779 | 0.5773 | 0.4795 |
| `fiqa` | `bm25` | 0.7578 | 0.5473 | 0.4694 | 0.4109 |
| `fiqa` | `bm25_teacher_sae` | 0.9131 | 0.7472 | 0.6806 | 0.6322 |
| `fiqa` | `bm25_student_atoms` | 0.7865 | 0.5720 | 0.4966 | 0.4401 |

Mean:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.7347 | 0.6101 | 0.5288 | 0.4352 |

## Weight Calibration Finding

The same 12-epoch student with different student atom weights produced:

| Student SAE Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.000 | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| 0.125 | 0.7265 | 0.6095 | 0.5240 | 0.4312 |
| 0.250 | 0.7347 | 0.6101 | 0.5288 | 0.4353 |
| 0.500 | 0.7440 | 0.6055 | 0.5266 | 0.4330 |
| 0.750 | 0.7478 | 0.6077 | 0.5260 | 0.4306 |
| 1.000 | 0.7489 | 0.5981 | 0.5139 | 0.4177 |
| 1.500 | 0.7516 | 0.5481 | 0.4769 | 0.3791 |
| 2.000 | 0.7430 | 0.4949 | 0.4367 | 0.3354 |

Interpretation:

- Student atoms already add recall over pure BM25.
- Raw student atom weights are not teacher-calibrated.
- `0.25` is the best first-page balance in this run.
- Higher weights continue to improve or preserve Recall@100 for a while, but
  they damage MRR/NDCG/MAP because noisy student atoms dominate exact lexical
  evidence.

## Support Distillation Diagnostics

For the calibrated 12-epoch run:

| Dataset | Split | Mean Jaccard | Mean Teacher Recall |
| --- | --- | ---: | ---: |
| `scifact` | `docs` | 0.2134 | 0.3494 |
| `scifact` | `queries` | 0.8040 | 0.8886 |
| `scidocs` | `docs` | 0.2020 | 0.3336 |
| `scidocs` | `queries` | 0.7994 | 0.8859 |
| `nfcorpus` | `docs` | 0.2448 | 0.3900 |
| `nfcorpus` | `queries` | 0.8487 | 0.9163 |
| `arguana` | `docs` | 0.2474 | 0.3937 |
| `arguana` | `queries` | 0.6794 | 0.8061 |
| `fiqa` | `docs` | 0.1966 | 0.3261 |
| `fiqa` | `queries` | 0.7865 | 0.8772 |

The query side is already close to the teacher support. The document side is
the first clear bottleneck.

## Longer Training Check

A 24-epoch run lowered the training loss but did not improve the retrieval
frontier:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `12e student weight 2.0` | 0.7430 | 0.4949 | 0.4367 | 0.3354 |
| `24e student weight 2.0` | 0.7372 | 0.4981 | 0.4365 | 0.3439 |

This suggests the next improvement should come from loss design and model shape,
not merely more epochs.

## Current Conclusion

The first direct text-to-atoms student is not ready to replace the teacher, but
the direction is validated enough to continue:

- `BM25 + student atoms` beats pure BM25 on all mean quality metrics.
- It remains materially behind `BM25 + Snowflake-SAE teacher`.
- The serving abstraction is viable: no dense vector index and no dense
  retrieval layer are used in the student evaluation.
- The main gaps are document-side atom support, atom weight calibration, and
  retrieval-aware training.

## Next Training Steps

1. Replace dense all-atom BCE with sampled-negative sparse support loss.
2. Add listwise dense-teacher top-k distillation so the student learns
   retrieval neighborhoods, not only per-row active atoms.
3. Add BM25 hard negatives and qrel positives to directly optimize ranking.
4. Add index-aware fanout penalty before increasing model size.
5. Test a stronger document encoder shape before moving to full-corpus runs.

The next meaningful target is:

```text
BM25 + text-student atoms
  Recall@100 within 0.02 of BM25 + Snowflake-SAE teacher
  while keeping MRR/NDCG/MAP above pure BM25
```

