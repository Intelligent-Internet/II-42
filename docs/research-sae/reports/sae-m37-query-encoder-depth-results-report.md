# SAE M37 Query Encoder Depth Results Report

Status: closed after one transformer-token-LSE primary arm.

## Summary

M37 tested whether the current blocker is mainly query-encoder depth. It is
not solved by simply adding a transformer layer on top of the current
text-to-atoms pipeline.

The transformer arm gives a small `trec-covid` ranking improvement, but causes
large aggregate and cross-dataset regressions. This is negative evidence for:

```text
same data + same objective + deeper query encoder
  -> product-ready robust text-to-atoms
```

The result is consistent with the M36 audit: the available training data and
supervision do not contain enough broad many-positive semantic-neighborhood
signal. A deeper encoder has more capacity, but it learns an unstable
semantic activation pattern without enough hard-bucket supervision.

## Implementation

M37 added opt-in model capacity controls to the M31 trainer:

```text
--model-encoder-mode
--model-transformer-layers
--model-transformer-heads
--model-dropout
--partial-checkpoint-load
```

This keeps existing M31 behavior unchanged by default. With partial loading,
the trainer instantiates the overridden model and copies only compatible
checkpoint tensors. New layers, such as the transformer block, are initialized
fresh.

Smoke run:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m32_teacher/m32-train-20k-q300 \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets scifact \
  --datasets scifact \
  --output-dir results/sae/m37/smoke-transformer \
  --device mps \
  --model-encoder-mode transformer_token_lse \
  --model-transformer-layers 1 \
  --model-transformer-heads 4 \
  --model-dropout 0.10 \
  --partial-checkpoint-load \
  --epochs 1 \
  --train-query-limit 2 \
  --collapse-aware-selection
```

Primary run:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m32_teacher/m32-train-20k-q300 \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets dbpedia-entity fever fiqa hotpotqa msmarco nfcorpus quora scifact \
  --output-dir results/sae/m37/transformer-token-lse-m32-train20k-eval-current \
  --device mps \
  --model-encoder-mode transformer_token_lse \
  --model-transformer-layers 1 \
  --model-transformer-heads 4 \
  --model-dropout 0.10 \
  --partial-checkpoint-load \
  --learning-rate 8.0e-5 \
  --head-learning-rate 8.0e-4 \
  --epochs 4 \
  --teacher-loss-weight 2.0 \
  --qrel-loss-weight 0.10 \
  --bm25-preserve-weight 0.35 \
  --fanout-loss-weight 0.03 \
  --scale-prior-weight 0.05 \
  --collapse-aware-selection
```

## Full15 Matrix

| Run | Best | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M32 shallow | `m31_fixed_w0p5` | 0.8694 | 0.8780 | 0.7734 | 0.7449 | 2964.8 | 2778.6 |
| M36 nfcorpus-expanded | `m31_fixed_w0p5` | 0.8671 | 0.8795 | 0.7731 | 0.7429 | 2923.6 | 2384.2 |
| M37 transformer | `m31_fixed_w0p25` | 0.7868 | 0.7870 | 0.6680 | 0.6275 | 2969.6 | 2175.5 |

M37 lowers SAE postings, but quality collapses toward BM25-level aggregate
performance. This is not a valid cost-quality tradeoff.

## Hard Dataset Table

### `trec-covid`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 shallow | 0.6226 | 0.4545 | 0.1435 | -0.2124 | -0.3094 |
| M36 nfcorpus-expanded | 0.6429 | 0.4516 | 0.1419 | -0.1922 | -0.3123 |
| M37 transformer | 0.6620 | 0.4689 | 0.1390 | -0.1731 | -0.2950 |

The transformer improves `trec-covid` NDCG and MAP relative to M32/M36, but
the gap remains far outside the gate and Recall@100 does not improve.

### `msmarco`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 shallow | 0.6320 | 0.8765 | 0.7894 | -0.0929 | -0.0587 |
| M36 nfcorpus-expanded | 0.6395 | 0.8784 | 0.7900 | -0.0853 | -0.0568 |
| M37 transformer | 0.5837 | 0.7867 | 0.7394 | -0.1411 | -0.1485 |

`msmarco` collapses.

### `dbpedia-entity`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 shallow | 0.6942 | 0.7542 | 0.8523 | 0.0085 | -0.0283 |
| M36 nfcorpus-expanded | 0.6834 | 0.7438 | 0.8558 | -0.0023 | -0.0387 |
| M37 transformer | 0.6270 | 0.6552 | 0.7818 | -0.0587 | -0.1273 |

`dbpedia-entity` collapses.

### `nfcorpus`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 shallow | 0.5156 | 0.3138 | 0.5475 | 0.0922 | 0.0873 |
| M36 nfcorpus-expanded | 0.5009 | 0.2956 | 0.5331 | 0.0775 | 0.0691 |
| M37 transformer | 0.3737 | 0.1859 | 0.3143 | -0.0497 | -0.0406 |

`nfcorpus` collapses despite being part of train. This is strong evidence that
the deeper arm is not just under-trained on this dataset; it is destabilizing
the learned atom distribution.

## Depth And Data Interpretation

There are two different meanings of "not enough training depth":

1. More neural depth.
   M37 tested this directly. A 1-layer transformer token-LSE encoder has more
   query-side capacity, but it over-shifts the atom distribution. The best
   source falls back to `m31_fixed_w0p25`, and higher SAE weights degrade
   sharply. This means depth alone is not enough under the current data and
   objective.

2. More epochs / longer training.
   The epoch traces do not point to a simple under-training problem. Teacher
   loss starts low, while BM25-preservation loss is high and only gradually
   recovers. Longer training may improve lexical preservation, but the arm
   already shows broad aggregate collapse after four epochs. More epochs would
   be a costly fine-tuning question, not the main blocker.

The data issue remains stronger:

- M36 found only 3 TREC-like `broad_high_df + many_positive` queries in M32
  train, compared with 50 in current eval.
- Expanded real `nfcorpus` data adds many-positive supervision, but does not
  match TREC-style broad public-health intent and fails to fix the gate.
- Existing M35/M35b synthetic sources are distributionally unsafe or already
  proven ineffective.

## Decision

M37 is closed as failed gate.

Do not promote:

- `transformer_token_lse` M37 primary;
- deeper encoder without stronger hard-bucket supervision;
- longer transformer training as the next mainline.

Keep:

- M31 opt-in model override and partial-load mechanism;
- M37 as evidence that capacity can affect `trec-covid`, but not robustly;
- the conclusion that the next breakthrough needs better supervision, not just
  more depth.

## Next Direction

The next valid route is a supervised hard-bucket generator or teacher-query
distillation source that creates real broad many-positive intent:

```text
validated broad query distribution
+ teacher neighborhood labels
+ qrel / near-miss boundaries
+ fixed-doc gate
  -> train query encoder
```

If a deeper encoder is retried, it should only be after that data source exists
and should start as a small controlled arm with explicit lexical-preservation
gates.
