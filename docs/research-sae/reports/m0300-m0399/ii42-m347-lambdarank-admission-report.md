# ii42 M347 LambdaRank Admission Report

## Goal

M346 validated that final-rank-aware pressure is more useful than simple
schedule or loss-weight sweeps, but the improvement remains small. M347 adds a
binary LambdaRank-style term: positive-vs-current-top-negative pairs are
weighted by their NDCG@100 swap delta.

The new term is disabled by default through `LAMBDA_RANK_WEIGHT=0.0`, preserving
all previous runners unless explicitly enabled.

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
  `/home/huoju/leask/runs/ii42-m347-lambdarank-admission-v1`

## M347A

Output:

`/home/huoju/leask/runs/ii42-m347-lambdarank-admission-v1/m347_lambda050_rankdisc030_listwise080_lr3e4_eval5_seed1050_split1050.json`

Parameters:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`
- `LAMBDA_RANK_WEIGHT=0.50`
- `LAMBDA_RANK_CUTOFF=100`
- `LAMBDA_RANK_NEGATIVES=64`
- `LAMBDA_RANK_MARGIN=0.03`

Reference:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M346A rank-discount | 0.683902 | 0.378038 | 0.368333 | 0.299527 |

Stop rule:

If M347A does not improve final aggregate heldout NDCG@10 or MAP@100 over
M346A, try a lambda-only ablation before investing in a larger objective
rewrite.

Status:

- Started on `spark-1` in tmux session `ii42_m347_lambda050`.
- Passed dataset read and 8/8 candidate-cache load.
- Entered `train scorer variant=df_le_0p25`.
- Epoch 5: `Recall@100=0.6966`, `MRR@20=0.3854`,
  `NDCG@10=0.3800`, `MAP@100=0.3264`.
- Epoch 10: `Recall@100=0.6964`, `MRR@20=0.3852`,
  `NDCG@10=0.3808`, `MAP@100=0.3256`.
- Completed with early stop at epoch 25 and wrote:
  `/home/huoju/leask/runs/ii42-m347-lambdarank-admission-v1/m347_lambda050_rankdisc030_listwise080_lr3e4_eval5_seed1050_split1050.json`.

M347A did not beat M346A on the training-event surface. The combination of
rank-discount admission and LambdaRank appears over-constrained.

## M347B

Output:

`/home/huoju/leask/runs/ii42-m347-lambdarank-admission-v1/m347_lambda030_only_listwise080_lr3e4_eval5_seed1050_split1050.json`

Parameters:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.0`
- `LAMBDA_RANK_WEIGHT=0.30`
- `LAMBDA_RANK_CUTOFF=100`
- `LAMBDA_RANK_NEGATIVES=64`
- `LAMBDA_RANK_MARGIN=0.03`

Reason:

M347A's early events underperform M346A on NDCG/MAP, suggesting that
rank-discount and LambdaRank may be over-constraining the same top-k errors.
M347B isolates LambdaRank without the M346 rank-discount term.

Status:

- Started on `spark-1` in tmux session `ii42_m347_lambda030_only`.
- Passed dataset read and 8/8 candidate-cache load.
- Entered `train scorer variant=df_le_0p25`.
- Epoch 5: `Recall@100=0.6994`, `MRR@20=0.3852`,
  `NDCG@10=0.3795`, `MAP@100=0.3261`.
- Completed with early stop at epoch 30 and wrote:
  `/home/huoju/leask/runs/ii42-m347-lambdarank-admission-v1/m347_lambda030_only_listwise080_lr3e4_eval5_seed1050_split1050.json`.

## Result

Final aggregate heldout scorer comparison:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M344 | 0.686372 | 0.375673 | 0.366046 | 0.296308 |
| M346A rank-discount | 0.683902 | 0.378038 | 0.368333 | 0.299527 |
| M347A lambda + rank-discount | 0.682834 | 0.376239 | 0.364822 | 0.298607 |
| M347B lambda-only | 0.682729 | 0.377569 | 0.367955 | 0.299346 |

Final aggregate all-query scorer comparison:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M344 | 0.705336 | 0.376512 | 0.370506 | 0.304863 |
| M346A rank-discount | 0.707999 | 0.378358 | 0.372187 | 0.307328 |
| M347A lambda + rank-discount | 0.709636 | 0.377755 | 0.369854 | 0.307470 |
| M347B lambda-only | 0.706972 | 0.378506 | 0.372125 | 0.307431 |

## Interpretation

M347 does not beat M346A on final heldout NDCG@10 or MAP@100. M347B is close
to M346A, but not better; M347A degrades heldout NDCG. The binary LambdaRank
approximation is therefore not the next productive direction in this form.

The current best broad8 variant remains M346A. The next useful step should be a
diagnostic pass over M346A failures to identify whether the remaining gap is
mostly:

- candidate positives admitted but ranked below top 20;
- BM25-supported positives suppressed by semantic false positives;
- dataset-specific failures such as `trec-covid` and `webis-touche2020`;
- capacity/feature limitations in the current scorer.
