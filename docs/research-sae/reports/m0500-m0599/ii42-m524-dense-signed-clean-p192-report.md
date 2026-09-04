# M524 Dense-Signed Clean P192 Gate

M524 reruns the M522 best operating point under a cleaner PPLX loader:

- model revision pinned to `2c4d510dd4a732063c31a0f70193e35067b51fd8`;
- `fix_mistral_regex=True`;
- `local_files_only=True`;
- baseline route phase;
- only prefix `192`.

The goal is reproducibility hygiene before any broad8, BIRE, or MTEB
promotion.  Training remains dense-only: no BM25 and no qrels in the loss.

## Run

- Host: `spark-1`
- Remote run dir: `/home/huoju/leask/runs/ii42-m524-dense-signed-clean-p192-v1`
- Local output: `outputs/m524/dense_signed_clean_p192/`
- Tasks: `ArguAna, FiQA2018, SCIDOCS, TRECCOVID`
- Target mode: `dense_signed`
- Route prefix: `192`
- Route preset: `baseline`
- Elapsed: `1198.668s`

The run completed cleanly and left no active Docker task.  The previous
tokenizer-regex and remote-code download warnings were removed.  A
`TRANSFORMERS_CACHE` deprecation warning remains and is not result-affecting.

## Macro Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m511_lora_support_candidates_p192` | 0.47312 | 0.65882 | 0.52297 | 0.24830 | 0.95450 | 0.77957 |
| `m506_structural_candidates_p192` | 0.47674 | 0.66194 | 0.52316 | 0.24950 | 0.99308 | 0.94991 |
| `route_subset_materialized_dense` | 0.48043 | 0.66173 | 0.53335 | 0.25565 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 |

## Comparison To M522 And M523

| Run | Loader | Route | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M522 p192 | unpinned regex-warning loader | baseline | 0.47640 | 0.66053 | 0.52394 | 0.24817 | 0.94054 | 0.73666 |
| M523 p192 | unpinned regex-warning loader | `load_preserve`, 3 epochs | 0.47352 | 0.65287 | 0.52558 | 0.24532 | 0.91127 | 0.72964 |
| M524 p192 | pinned + fixed tokenizer | baseline | 0.47312 | 0.65882 | 0.52297 | 0.24830 | 0.95450 | 0.77957 |

## Interpretation

M524 succeeds as a loader hygiene fix but not as a quality improvement.  It
removes the tokenizer and remote-code warnings, and candidate recall improves
over M522 (`0.94054 -> 0.95450`).  However, NDCG drops from `0.47640` to
`0.47312`, and touch rises from `0.73666` to `0.77957`.

The likely explanation is that `fix_mistral_regex=True` changes tokenization
enough that M522 and M524 are not directly trend-comparable.  M522 remains the
best old-loader learned quality point, but M524 is the right clean baseline for
future runs.

This means the project should not promote M522 directly into broad8 without
rerunning on the clean loader.  The reproducibility surface changed, so future
claims should use M524-style pinned/tokenizer-fixed runs.

## Next Step

M525 should rebuild the clean-loader direction rather than continue
old-loader tuning:

1. Run clean-loader p160/p192/p224 or p192/p224 on the same broad4 surface to
   see whether the clean tokenization recovers NDCG with slightly more support.
2. If clean p224 reaches M522-level NDCG while candidate recall stays above
   M524, then promote clean-loader dense-signed to broad8.
3. If clean p224 only spends touch without NDCG recovery, return to the
   first-stage support objective and add an explicit teacher TopK coverage
   loss/diagnostic.

Current frontier:

- Best old-loader learned quality: M522 p192, NDCG `0.47640`, touch `0.73666`.
- Clean reproducible baseline: M524 p192, NDCG `0.47312`, touch `0.77957`.
- Deterministic structural reference: p192 NDCG `0.47674`, touch `0.94991`.
