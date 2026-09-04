# SAE M101 Batched Stage-B Ranker Report

Date: 2026-05-22

## Decision

M101 promotes the M99/M100 Stage-B candidate residual ranker into a batched
and exportable research runtime path. The model preserves M100-level quality,
finishes the full 220-epoch training run in `6.00s` on Spark CUDA, and validates
strict top-10 parity between row-loop scoring and batched/exported scoring.

This closes the immediate Stage-B row-loop engineering bottleneck. It does not
complete full-corpus retrieval: the next blocker is fanout-aware candidate
generation and native/runtime integration around the exported scorer.

M101 ports the M99/M100 candidate residual ranker to a batched 
tensor path and validates export/parity. Runtime features remain 
BM25/SAE-only; dense remains a training/control teacher.

## Main Matrix

| Split | Run | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| `validation` | `bm25_dense_w2` | 0.3360 | 0.6642 | 0.5050 |
| `validation` | `bm25_sae_w2` | 0.3435 | 0.6680 | 0.5119 |
| `validation` | `m101_batched_ranker` | 0.3853 | 0.6932 | 0.5494 |
| `holdout` | `bm25_dense_w2` | 0.3735 | 0.6617 | 0.5242 |
| `holdout` | `bm25_sae_w2` | 0.3721 | 0.6611 | 0.5247 |
| `holdout` | `m101_batched_ranker` | 0.4259 | 0.6927 | 0.5733 |
| `eval` | `bm25_dense_w2` | 0.3530 | 0.6631 | 0.5137 |
| `eval` | `bm25_sae_w2` | 0.3564 | 0.6649 | 0.5177 |
| `eval` | `m101_batched_ranker` | 0.4036 | 0.6930 | 0.5602 |

## Training

- `best_epoch`: `180`
- `best_validation_score`: `0.972822`
- `elapsed_seconds`: `6.00`
- `device`: `cuda`

## Parity

| Check | Rows | TopK | Max Abs Delta | TopK Mismatches |
| --- | ---: | ---: | ---: | ---: |
| `m101_export_vs_row_loop` | 4806 | 10 | 0.00000191 | 0 |
| `best_m100_row_loop_vs_batch` | 4806 | 10 | 0.00000191 | 0 |

## Decision Notes

- This closes the row-loop engineering gap for Stage-B research 
  training and scoring.
- This is still candidate-surface ranking, not native full-corpus 
  retrieval.
- The next step should move the exported scorer into a minimal 
  runtime path and pair it with fanout-aware candidate generation.
