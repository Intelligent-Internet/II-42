# SAE M47 Cost-Aware Exported Ranking Plan

Status: closed as a focused negative training result.

## Goal

M46 showed that post-hoc runtime profile selection improves the frontier. M47
tests whether direct cost-aware exported-ranking training can beat that
post-hoc selector.

The minimum useful M47 arm should:

- start from the M40 checkpoint;
- train through the same physical export surface used by M45 high-quality;
- add a small top-k fanout regularizer;
- penalize broad high-DF queries more strongly;
- evaluate against M46, M45, and M44.

## Focused Arm

```text
checkpoint = M40
export_pool_active_dims = 96
export_active_dims = 48
export_fanout_power = 0.15
train_exported_query_scoring = true
topk_fanout_loss_weight = 0.01
topk_fanout_active_dims = 48
broad_topk_fanout_multiplier = 2.0
epochs = 2
learning_rate = 1.0e-4
head_learning_rate = 3.0e-4
```

This deliberately avoids a large sweep. The question is whether training on
the exported surface moves the frontier in the right direction. If not, M47
should be parked rather than tuned blindly.

## Acceptance Rule

Promote M47 only if it beats M46 on the quality-cost frontier:

- NDCG/MAP close to or above M46 `fanout_to_m44`;
- TREC MAP not below M46;
- SAE postings meaningfully lower than M46;
- no new hard-dataset collapse.
