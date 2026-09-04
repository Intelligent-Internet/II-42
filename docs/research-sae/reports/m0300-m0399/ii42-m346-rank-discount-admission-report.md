# ii42 M346 Rank-Discount Admission Report

## Goal

M345 showed that schedule and simple objective-weight sweeps only recover a
small amount of broad8 ranking quality. The remaining gap to the candidate
upper bound is not a cache or simple LR problem.

M346 adds a code-level final-rank objective: rank-discounted admission loss.
It targets the current top-k non-qrel false positives and weights them by DCG
discount, so rank-1 mistakes are more expensive than rank-100 mistakes.

The default weight is `0.0`, so existing M344/M345 behavior is unchanged unless
the runner explicitly enables the new term.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Code change:
  `scripts/research_sae_m322_candidate_pool_scorer.py`
- Shared runner env:
  `scripts/run_m334_interaction_feature_scorer_spark.sh`
- Host: `spark-1`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m346-rank-discount-admission-v1`

## M346A

Output:

`/home/huoju/leask/runs/ii42-m346-rank-discount-admission-v1/m346_rankdisc030_listwise080_lr3e4_eval5_seed1050_split1050.json`

Parameters:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

M345 best final aggregate heldout reference:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M345A low LR | 0.683824 | 0.377837 | 0.367564 | 0.298540 |
| M345C high listwise | 0.683086 | 0.376953 | 0.367599 | 0.298391 |

Stop rule:

If M346A does not materially beat M345A/C on final aggregate heldout NDCG@10
or MAP@100, stop env/objective micro-sweeps and move to a larger objective
redesign.

Status:

- Started on `spark-1` in tmux session `ii42_m346_rankdisc030`.
- Passed dataset read and 8/8 candidate-cache load.
- Entered `train scorer variant=df_le_0p25`.
- Epoch 5: `Recall@100=0.7001`, `MRR@20=0.3848`,
  `NDCG@10=0.3793`, `MAP@100=0.3258`.
- Epoch 10: `Recall@100=0.6982`, `MRR@20=0.3859`,
  `NDCG@10=0.3818`, `MAP@100=0.3261`.
- Completed with early stop at epoch 30 and wrote:
  `/home/huoju/leask/runs/ii42-m346-rank-discount-admission-v1/m346_rankdisc030_listwise080_lr3e4_eval5_seed1050_split1050.json`.

Final aggregate heldout scorer metrics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M344 | 0.686372 | 0.375673 | 0.366046 | 0.296308 |
| M345A low LR | 0.683824 | 0.377837 | 0.367564 | 0.298540 |
| M345C high listwise | 0.683086 | 0.376953 | 0.367599 | 0.298391 |
| M346A rank-discount | 0.683902 | 0.378038 | 0.368333 | 0.299527 |

Delta vs M344 heldout scorer:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.002470 |
| MRR@20 | +0.002364 |
| NDCG@10 | +0.002288 |
| MAP@100 | +0.003218 |

Final aggregate all-query scorer metrics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M344 | 0.705336 | 0.376512 | 0.370506 | 0.304863 |
| M345A low LR | 0.701915 | 0.378049 | 0.371346 | 0.306200 |
| M345C high listwise | 0.707770 | 0.377590 | 0.371651 | 0.306529 |
| M346A rank-discount | 0.707999 | 0.378358 | 0.372187 | 0.307328 |

## Interpretation

M346A is the best current variant on heldout MRR, NDCG, and MAP, and it also
sets the best all-query NDCG/MAP among M344/M345/M346. The improvement is still
small, so this is not a full breakthrough. It does, however, validate the
direction: the scorer benefits from final-rank-aware pressure more than from
plain schedule or simple loss-weight sweeps.

The remaining gap to the upper bound is still large. The next useful step is
not more small env sweeps; it is a larger final-ranking objective redesign,
likely LambdaRank/NDCG-delta weighting or direct top-k listwise admission, while
continuing to reuse the same 8/8 broad8 candidate cache.
