# SAE M99 Stage-B Candidate Residual Ranking Report

Date: 2026-05-22

## Decision

M99 is a positive Stage-B ranking result. It promotes the next optimization
direction from query-level scalar calibration to candidate-level residual
ranking over runtime-safe BM25/SAE features.

The result is materially better than M98 on validation, holdout, combined eval,
and the broad-generated family that remained weak after M98. This does not
complete full-corpus productization; it is still candidate-surface ranking over
the M81 surface.

M99 trains a small candidate-level residual ranker over the M98 full 
M81 candidate score cache. Runtime scoring still uses BM25 and SAE 
only; dense remains a training/control teacher.

## Data

- `rows`: `4806`
- `train_rows`: `3920`
- `validation_rows`: `486`
- `holdout_rows`: `400`
- `eval_rows`: `886`

## Main Matrix

| Split | Run | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| `validation` | `bm25_dense_w2` | 0.3360 | 0.6642 | 0.5050 |
| `validation` | `bm25_sae_w2` | 0.3435 | 0.6680 | 0.5119 |
| `validation` | `m98_calibrated` | 0.3436 | 0.6765 | 0.5150 |
| `validation` | `m99_candidate_residual` | 0.3845 | 0.6934 | 0.5472 |
| `holdout` | `bm25_dense_w2` | 0.3735 | 0.6617 | 0.5242 |
| `holdout` | `bm25_sae_w2` | 0.3721 | 0.6611 | 0.5247 |
| `holdout` | `m98_calibrated` | 0.3817 | 0.6687 | 0.5304 |
| `holdout` | `m99_candidate_residual` | 0.4296 | 0.6869 | 0.5704 |
| `eval` | `bm25_dense_w2` | 0.3530 | 0.6631 | 0.5137 |
| `eval` | `bm25_sae_w2` | 0.3564 | 0.6649 | 0.5177 |
| `eval` | `m98_calibrated` | 0.3608 | 0.6729 | 0.5219 |
| `eval` | `m99_candidate_residual` | 0.4049 | 0.6905 | 0.5576 |

## Eval By Family

| Family | Run | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| `beir15_current_eval_surface` | `bm25_dense_w2` | 0.5875 | 0.7512 | 0.7094 |
| `beir15_current_eval_surface` | `bm25_sae_w2` | 0.5889 | 0.7375 | 0.7010 |
| `beir15_current_eval_surface` | `m98_calibrated` | 0.6017 | 0.7409 | 0.7104 |
| `beir15_current_eval_surface` | `m99_candidate_residual` | 0.6092 | 0.7496 | 0.7208 |
| `broad_generated_query_surface` | `bm25_dense_w2` | 0.2779 | 0.6450 | 0.4679 |
| `broad_generated_query_surface` | `bm25_sae_w2` | 0.2783 | 0.6364 | 0.4628 |
| `broad_generated_query_surface` | `m98_calibrated` | 0.2807 | 0.6433 | 0.4651 |
| `broad_generated_query_surface` | `m99_candidate_residual` | 0.3313 | 0.6632 | 0.5062 |
| `large_supervised_split_surface` | `bm25_dense_w2` | 0.9259 | 0.7600 | 0.7937 |
| `large_supervised_split_surface` | `bm25_sae_w2` | 0.9683 | 0.9030 | 0.9206 |
| `large_supervised_split_surface` | `m98_calibrated` | 0.9841 | 0.9320 | 0.9407 |
| `large_supervised_split_surface` | `m99_candidate_residual` | 1.0000 | 0.9339 | 0.9468 |

## Training

- `best_epoch`: `150`
- `best_validation_score`: `0.970597`
- `epochs`: `240`
- `learning_rate`: `0.006`
- `hidden_dim`: `48`
- `residual_max`: `0.2`

## Decision Notes

- This is Stage-B ranking calibration over an existing candidate 
  surface, not full-corpus retrieval.
- Dense scores are used only as training/control signals.
- Promotion requires holdout safety and broad-generated MRR/NDCG 
  improvement, not just validation improvement.
