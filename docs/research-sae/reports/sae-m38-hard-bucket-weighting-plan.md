# SAE M38 Hard-Bucket Weighting Plan

Status: executed as a targeted follow-up to M36/M37.

## Motivation

M36 showed that current evaluation contains many more hard broad queries than
the clean M32 training split:

```text
current_eval TREC-like broad/many-positive queries: 50
M32 train TREC-like broad/many-positive queries: 3
```

M37 then showed that adding a small transformer query encoder does not solve
the blocker without better hard-bucket supervision. M38 therefore tests the
minimal remaining hypothesis before building a new query generator:

```text
same M31 final-ranking objective
+ explicit hard-bucket sample weights
  -> better broad/many-positive and semantic-heavy robustness
```

This is intentionally not a new model-capacity experiment.

## Implementation Scope

The implementation extends
`scripts/research_sae_m31_joint_final_ranking_train.py` with opt-in training
sample reweighting:

```text
--hard-bucket-weight-mode
--hard-bucket-weight
--hard-bucket-high-df-threshold
--hard-bucket-many-positive-threshold
--hard-bucket-teacher-advantage-threshold
```

Default behavior remains unchanged:

```text
hard_bucket_weight_mode = off
hard_bucket_weight = 1.0
```

The hard-bucket weight only multiplies ranking supervision:

```text
teacher listwise loss
+ qrel residual loss
+ BM25 preserve loss
```

It does not multiply:

```text
fanout regularization
support/value imitation
BM25/SAE scale prior
```

This keeps the experiment from buying apparent quality by widening the
physical query footprint.

## Bucket Definition

M38 uses runtime-safe and training-surface-safe query diagnostics:

| Bucket | Signal |
| --- | --- |
| `broad_high_df` | query content tokens have high corpus DF |
| `many_positive` | qrel positive count is at least 100 |
| `semantic_teacher_advantage` | candidate AP from teacher exceeds BM25 by at least 0.10 |

The primary run uses:

```text
hard_bucket_weight_mode = many_or_broad_semantic
hard_bucket_weight = 3.0
```

This means:

```text
many_positive
OR
(broad_high_df AND semantic_teacher_advantage)
```

## Training Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m36_train_merged/m36-real-nfcorpus-expanded \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets dbpedia-entity fever fiqa hotpotqa msmarco nfcorpus quora scifact \
  --output-dir results/sae/m38/hard-bucket-weighted-nfcorpus-expanded \
  --device mps \
  --learning-rate 1.0e-4 \
  --head-learning-rate 8.0e-4 \
  --epochs 4 \
  --teacher-loss-weight 2.0 \
  --qrel-loss-weight 0.10 \
  --bm25-preserve-weight 0.35 \
  --fanout-loss-weight 0.03 \
  --scale-prior-weight 0.05 \
  --hard-bucket-weight-mode many_or_broad_semantic \
  --hard-bucket-weight 3.0 \
  --collapse-aware-selection
```

## Acceptance Gate

M38 can only be promoted if it improves hard datasets without losing the M32/M36
aggregate and without increasing the physical footprint meaningfully:

| Gate | Requirement |
| --- | --- |
| Full15 quality | not below M36 aggregate |
| Hard datasets | improve `trec-covid`, `msmarco`, and `dbpedia-entity` collapse |
| Physical cost | candidate docs and SAE postings not materially above M36 |
| Decision | if unchanged, close weight-only hard-bucket route |
