# M553 Frozen LM Judge Posting Encoder Report

M553 tests whether the DREAM-inspired signal can improve the BM25-free
first-stage posting encoder beyond the M551 candidate-set/listwise milestone.

The implementation keeps the M551/M552 encoder and evaluation surface intact:

- dense rows are frozen;
- qrels are used only for held-out evaluation;
- the trainable component remains the identity-initialized residual posting
  output layer;
- the new target mode is `lm_query_likelihood_blend`;
- the frozen judge score is `-loss(query tokens | "Document: {doc}\nQuery:")`;
- the LM utility is standardized and blended with the dense teacher utility.

This is a bounded approximation of DREAM. It does not inject retriever scores
into retrieval heads of a frozen LLM.

## Baseline

Same-task M551 promoted seed552 baseline on:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

Remote result:

```text
/home/huoju/leask/runs/ii42-m551-dream-lite-posting-v1/locked_broad10_sharp_resid025_seed552/m551_locked_broad10_sharp_resid025_seed552.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m551_shared_locked_support_residual_dream_lite` | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |

M551 delta versus `dense_topk128_sparse`:

- NDCG@10: `+0.00432`
- MAP@100: `-0.00087`
- Recall@100: `+0.00097`
- MRR@20: `+0.00759`
- Dense overlap@100: `+0.00402`

## M553 Smokes

All smokes use:

- `SEED=552`
- `TASKS=FiQA2018,SCIDOCS,TRECCOVID`
- `TEACHER_TARGET_MODE=lm_query_likelihood_blend`
- `TEACHER_POOL_K=32`
- `RANDOM_NEGATIVES=32`
- `MAX_TRAIN_GROUPS=64`
- M551 promoted residual and temperature settings otherwise unchanged.

### `distilgpt2`, `JUDGE_WEIGHT=0.25`

Remote result:

```text
/home/huoju/leask/runs/ii42-m553-frozen-lm-judge-posting-v1/lm_distilgpt2_w0.25_seed552/m553_lm_distilgpt2_w0.25_seed552.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m553_shared_locked_support_residual_dream_lite` | 0.44622 | 0.20129 | 0.43414 | 0.57926 | 0.54464 |

Delta versus M551 same-task baseline:

- NDCG@10: `-0.00024`
- MAP@100: `-0.00042`
- Recall@100: `-0.00277`
- MRR@20: `-0.01014`
- Dense overlap@100: `-0.00360`

Interpretation: NDCG is nearly tied, but recall, MRR, and dense-overlap are
materially weaker than M551. This is not promotable.

### `distilgpt2`, `JUDGE_WEIGHT=0.05`

Remote result:

```text
/home/huoju/leask/runs/ii42-m553-frozen-lm-judge-posting-v1/lm_distilgpt2_w0.05_seed552/m553_lm_distilgpt2_w0.05_seed552.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m553_shared_locked_support_residual_dream_lite` | 0.44486 | 0.20043 | 0.43330 | 0.58755 | 0.54495 |

Delta versus M551 same-task baseline:

- NDCG@10: `-0.00159`
- MAP@100: `-0.00128`
- Recall@100: `-0.00361`
- MRR@20: `-0.00185`
- Dense overlap@100: `-0.00328`

Interpretation: lowering the LM judge weight reduces MRR damage but weakens
NDCG and recall further. This does not rescue the distilgpt2 judge.

### `Qwen/Qwen2.5-0.5B-Instruct`, `JUDGE_WEIGHT=0.25`

Remote result:

```text
/home/huoju/leask/runs/ii42-m553-frozen-lm-judge-posting-v1/qwen25_05b_w0.25_seed552/m553_qwen25_05b_w0.25_seed552.json
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_topk128_sparse` | 0.44214 | 0.20258 | 0.43594 | 0.58182 | 0.54421 |
| `m553_shared_locked_support_residual_dream_lite` | 0.44198 | 0.20076 | 0.43397 | 0.58754 | 0.54527 |

Delta versus M551 same-task baseline:

- NDCG@10: `-0.00448`
- MAP@100: `-0.00095`
- Recall@100: `-0.00294`
- MRR@20: `-0.00187`
- Dense overlap@100: `-0.00296`

Interpretation: a stronger small LM judge does not improve the approximation.
It loses NDCG even against the dense-topk sparse baseline.

## Conclusion

M553 does not beat M551. The DREAM paper remains useful, but the useful part
we could validate locally is M551-style candidate-set competition, not this
cheap frozen-LM likelihood proxy.

Current route decision:

- keep M551 as the promoted BM25-free first-stage milestone;
- keep M553 code and runner as negative evidence and future infrastructure;
- do not scale `distilgpt2` or `Qwen2.5-0.5B` likelihood-blend targets;
- only revisit DREAM-style frozen LLM supervision if we implement a closer
  interface, such as score injection into selected retrieval-relevant heads, or
  if a stronger judge can produce a target distribution that passes a cheap
  candidate-set diagnostic before training.

Practical next step: return to M551 as the stable first-stage training signal,
then test improvements around candidate-set construction and support gating
rather than more query-likelihood blending.
