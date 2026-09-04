# II-42 M407 Dense-Function Posting Encoder Plan

Date: 2026-06-27

## Goal

M407 restarts the encoder line with the right objective:

```text
text/dense embedding -> indexable sparse signed coordinates + residual sketch
```

The student should approximate the dense teacher similarity function, not a
single benchmark's top-k labels.

## Why This Differs From M401-M406

M401-M406 mostly tested local adapters and gates around a deterministic route.
Those experiments were useful diagnostics, but they did not train a full
text-to-posting representation.

M407 trains query and document encoders directly.  The model output is already
the runtime representation:

- sparse signed coordinates for inverted-list admission;
- small residual sketch for dense-tail scoring;
- qrels-free dense-function distillation loss.

## Loss

The first prototype uses dense embeddings already materialized by the MTEB
shared root.  Later versions can replace the input with raw text encoders.

Training losses:

- score distillation across teacher-top, structural hard, and random pairs;
- listwise dense distribution matching within each training group;
- pairwise dense-order preservation;
- posting balance regularization to avoid overloaded coordinates;
- quantization-aware hard top-k masking during training.

Qrels are evaluation only.

## Metrics

M407 must report both retrieval and dense-function metrics:

- NDCG@10 / Recall@100 / MRR@20 / MAP@100;
- Dense O@100;
- full-score Pearson on heldout queries;
- Top100 overlap against dense teacher;
- sampled pairwise order agreement.

The dense-function metrics are the primary gate.  NDCG gains without geometry
preservation are not enough.

## First Gate

Run FiQA seed407 with a small training cap:

- tasks: `FiQA2018`;
- max train groups: `64`;
- epochs: `2`;
- compare against exact dense and deterministic M392-style structural route.

If the learned encoder is worse than the deterministic structural route on both
dense-function metrics and NDCG, stop and redesign the model/loss before any
4-task expansion.
