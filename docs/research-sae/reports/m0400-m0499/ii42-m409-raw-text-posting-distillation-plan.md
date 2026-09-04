# II-42 M409 Raw Text Posting Distillation Plan

Date: 2026-06-27

## Goal

M409 starts Stage B:

```text
raw text -> dense-derived posting representation
```

The target is still M408's deterministic posting surface.  Qrels and ranking
loss remain out of training.

## First Model

Use a lightweight raw-text encoder:

- stable hashed word and character n-gram features;
- `EmbeddingBag` token pooling;
- a coordinate head that predicts rotated dense-derived coordinates;
- deterministic active posting and tail-sketch post-process from the predicted
  coordinate.

This is intentionally smaller than `pplx-embed-v1-0.6B`.  The first question is
not final quality; it is whether direct text-to-posting distillation has a
clean learning signal.

## Loss

Representation-only losses:

- full coordinate MSE;
- active support and signed magnitude MSE;
- inactive coordinate MSE;
- optional cosine/scale diagnostics through evaluation.

No qrels.  No BM25-aware loss.  No retrieval ranking loss.

## Gate

Run FiQA first:

- compare against `m409_teacher_structural_tail`;
- report representation metrics: coord cosine, active Jaccard, sketch cosine;
- report retrieval metrics only as diagnostics.

Promotion rule:

- If active Jaccard and coordinate cosine move substantially above random and
  retrieval is not degenerate, continue with better text encoders.
- If it fails, improve text capacity/tokenization before adding ranking loss.
