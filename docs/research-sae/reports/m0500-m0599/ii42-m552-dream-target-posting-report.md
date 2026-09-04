# M552 DREAM Target-Utility Posting Encoder

M552 is a BM25-free follow-up to M551.  It tests whether one more part of
DREAM's training signal is useful for our posting encoder: target-passage
utility.

M551 already proved that candidate-set competition against a frozen dense
teacher distribution is useful.  M552 keeps the same identity-initialized
locked-support residual encoder and the same held-out retrieval evaluation,
but changes the training target distribution.

For each training query:

1. Select the dense top document as a frozen target passage.
2. Build the same candidate set as M551: dense teacher pool plus random
   negatives.
3. Score each candidate by a blend of query-document dense similarity and
   candidate-to-target-document dense similarity.
4. Train the posting encoder with the same candidate-set KL objective.

This is not full DREAM: it does not inject scores into a frozen LLM's selected
retrieval heads and does not use next-token loss.  It is a bounded proxy for
the target-passage utility part of DREAM.  If this fails, the result does not
disprove DREAM; it only says that the cheap target-doc proxy is not enough.

## Baseline To Beat

Current M551 promoted setting:

- `RESIDUAL_SCALE=0.025`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- `EPOCHS=4`
- `RANDOM_NEGATIVES=128`
- `TEACHER_POOL_K=128`

M551 three-seed mean delta versus `dense_topk128_sparse` on the materialized
MTEB broad10 surface:

| Metric | Mean delta |
| --- | ---: |
| NDCG@10 | +0.00670 |
| MAP@100 | +0.00652 |
| Recall@100 | +0.00479 |
| MRR@20 | +0.01080 |
| Dense overlap@100 | +0.00337 |

## First Smoke

Runner:

```text
scripts/run_m552_dream_target_posting_spark.sh
```

Default smoke config:

- Tasks: `FiQA2018,SCIDOCS,TRECCOVID`
- `TEACHER_TARGET_MODE=target_doc_dense_blend`
- `JUDGE_WEIGHT=0.20`
- `TARGET_DOC_RANK=0`
- M551 promoted residual/temperature settings otherwise unchanged.

Promotion rule: continue only if M552 beats or matches the corresponding M551
promoted setting on the same task set without losing active-support stability.

### `target_doc_dense_blend`, `JUDGE_WEIGHT=0.20`

Run:

```text
/home/huoju/leask/runs/ii42-m552-dream-target-posting-v1/target_blend_w0.20_seed552/m552_target_blend_w0.20_seed552.json
```

Macro on `FiQA2018,SCIDOCS,TRECCOVID`:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m552_shared_locked_support_residual_dream_lite` | 0.44475 | 0.19964 | 0.43642 | 0.58569 | 0.54670 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00261`
- MAP@100: `-0.00294`
- Recall@100: `+0.00048`
- MRR@20: `+0.00387`
- Dense overlap@100: `+0.00249`

Same-task comparison against M551 promoted seed552:

| Metric | M551 delta | M552 delta | M552 - M551 variant |
| --- | ---: | ---: | ---: |
| NDCG@10 | +0.00432 | +0.00261 | -0.00171 |
| MAP@100 | -0.00087 | -0.00294 | -0.00207 |
| Recall@100 | +0.00097 | +0.00049 | -0.00049 |
| MRR@20 | +0.00759 | +0.00387 | -0.00372 |
| Dense overlap@100 | +0.00402 | +0.00249 | -0.00153 |

Interpretation: `JUDGE_WEIGHT=0.20` is weaker than M551 on the same task set.
The target-doc proxy is not strong enough at this weight.  A lower weight is
worth one smoke because the proxy may be over-regularizing the query-doc
distribution.

### `target_doc_dense_blend`, `JUDGE_WEIGHT=0.05`

Run:

```text
/home/huoju/leask/runs/ii42-m552-dream-target-posting-v1/target_blend_w0.05_seed552/m552_target_blend_w0.05_seed552.json
```

Macro on `FiQA2018,SCIDOCS,TRECCOVID`:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m552_shared_locked_support_residual_dream_lite` | 0.44521 | 0.20035 | 0.43624 | 0.58768 | 0.54713 |

Delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00307`
- MAP@100: `-0.00223`
- Recall@100: `+0.00030`
- MRR@20: `+0.00586`
- Dense overlap@100: `+0.00292`

Same-task comparison against M551 promoted seed552:

| Metric | M551 delta | M552 delta | M552 - M551 variant |
| --- | ---: | ---: | ---: |
| NDCG@10 | +0.00432 | +0.00307 | -0.00125 |
| MAP@100 | -0.00087 | -0.00223 | -0.00136 |
| Recall@100 | +0.00097 | +0.00030 | -0.00067 |
| MRR@20 | +0.00759 | +0.00586 | -0.00172 |
| Dense overlap@100 | +0.00402 | +0.00292 | -0.00111 |

Interpretation: lowering the target-doc judge weight reduces the damage but
does not beat M551 on any main metric.  This cheap target-passage proxy is not
worth broad10 scale-up.  The useful paper signal remains M551's candidate-set
competition; the next DREAM-derived test must use a materially closer frozen
judge, not a dense target-doc similarity shortcut.
