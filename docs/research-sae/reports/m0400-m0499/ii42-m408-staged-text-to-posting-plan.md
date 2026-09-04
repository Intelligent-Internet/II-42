# II-42 M408 Staged Text-to-Posting Plan

Date: 2026-06-27

## Goal

The final goal is:

```text
raw text -> indexable posting representation
```

M408 separates that goal into diagnostic stages instead of mixing text,
posting, ranking, and BM25 in one loss.

## Stages

### Stage A: Dense to Deterministic Posting

Input: materialized dense embedding.

Target: deterministic posting derived from the dense embedding:

- rotated coordinates;
- active signed-coordinate support;
- document tail sketch;
- query sketch;
- exact deterministic route scores.

This stage should be close to trivial.  If a dense-input model cannot reproduce
the deterministic posting route, the loss or implementation is wrong.

### Stage B: Text to Dense-Derived Posting

Input: raw text.

Target: the same dense-derived posting from Stage A.

This is the real encoder problem.  It distills the dense model plus posting
post-process into one direct text-to-posting model.

### Stage C: Optional Retrieval Fine-Tune

Only after Stage B preserves the deterministic posting surface, add retrieval
or ranking losses.  Qrels remain evaluation-only unless explicitly running a
separate supervised experiment.

## M408 First Gate

Run FiQA with two surfaces:

- `exact_postprocess`: no learning, implemented through the encoder
  post-process path.  It must match the deterministic teacher route.
- `linear_fit`: a dense-input linear coordinate encoder trained only on
  representation imitation.  This tests optimization and loss shape without
  raw-text noise.

Promotion rule:

- `exact_postprocess` must equal the deterministic teacher within numerical
  noise.
- `linear_fit` must improve representation metrics toward teacher before any
  raw-text experiment starts.

If either condition fails, do not run raw-text training.
