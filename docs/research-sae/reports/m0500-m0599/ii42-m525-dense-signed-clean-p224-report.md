# M525 Dense-Signed Clean P224 Gate

M525 extends the clean-loader M524 baseline from prefix `192` to prefix `224`.
The goal is to test whether the clean tokenizer/revision surface can recover
the old M522 NDCG by spending modestly more support capacity.

Training remains dense-only: no BM25 and no qrels in the loss.

## Run

- Host: `spark-1`
- Remote run dir: `/home/huoju/leask/runs/ii42-m525-dense-signed-clean-p224-v1`
- Local output: `outputs/m525/dense_signed_clean_p224/`
- Tasks: `ArguAna, FiQA2018, SCIDOCS, TRECCOVID`
- Target mode: `dense_signed`
- Model revision: `2c4d510dd4a732063c31a0f70193e35067b51fd8`
- Fix Mistral regex: `True`
- Route prefix: `224`
- Route preset: `baseline`

The run completed cleanly and left no active Docker task on `spark-1`.

## Macro Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m511_lora_support_candidates_p224` | 0.47378 | 0.66005 | 0.52294 | 0.24814 | 0.96291 | 0.80454 |
| `m506_structural_candidates_p224` | 0.47700 | 0.66224 | 0.52317 | 0.24972 | 0.99598 | 0.96466 |
| `route_subset_materialized_dense` | 0.48043 | 0.66173 | 0.53335 | 0.25565 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 |

## Comparison

| Run | Loader | Prefix | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M522 | old warning loader | 192 | 0.47640 | 0.66053 | 0.52394 | 0.24817 | 0.94054 | 0.73666 |
| M524 | clean loader | 192 | 0.47312 | 0.65882 | 0.52297 | 0.24830 | 0.95450 | 0.77957 |
| M525 | clean loader | 224 | 0.47378 | 0.66005 | 0.52294 | 0.24814 | 0.96291 | 0.80454 |

## Interpretation

M525 does not recover the M522 learned frontier.  Moving clean-loader prefix
from `192` to `224` improves candidate recall (`0.95450 -> 0.96291`) and
slightly improves NDCG (`0.47312 -> 0.47378`), but the gain costs more touch
(`0.77957 -> 0.80454`) and remains below old-loader M522 p192 (`0.47640`).

This is now a warning against continuing blind prefix sweeps.  The learned
encoder can spend more posting budget to cover more dense candidates, but the
ranking quality does not recover proportionally.  The bottleneck is likely the
learned text-to-support shape, not route fanout alone.

## Stop Rule

Do not continue this line with another small prefix-only run unless the target
or loss changes.  The next useful experiment should directly diagnose whether
the model can reproduce the deterministic dense-derived posting target before
route evaluation:

1. Freeze route evaluation.
2. Train and measure direct text-to-posting teacher fit on dense-derived
   postings: active-coordinate recall, signed magnitude error, TopK support
   overlap, and dense-neighbor coverage.
3. Only after teacher-fit improves should broad4/broad8 route evaluation be
   repeated.

Current retained frontier:

- Best old-loader learned quality: M522 p192, NDCG `0.47640`, touch `0.73666`.
- Best clean-loader learned quality: M525 p224, NDCG `0.47378`, touch
  `0.80454`.
- Clean-loader p224 still trails structural p224 NDCG `0.47700` and dense
  NDCG `0.48032`.
