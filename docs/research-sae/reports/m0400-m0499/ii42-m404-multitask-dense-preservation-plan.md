# II-42 M404 Multitask Dense-Preservation Plan

Date: 2026-06-27

## Motivation

M403-A gave the first repeated learned-only NDCG improvement over deterministic
structural tail, but Dense O@100 dropped.  That means the model learned a useful
top-rank query correction, but it was not dense-faithful enough.

M404 tests whether broader dense-teacher distillation fixes this.  The model
must be shared across tasks, otherwise the experiment becomes per-dataset
micro-tuning.

## Model

Train a shared dense-space query adapter:

```text
query_dense -> query_dense + residual(query_dense)
```

For each task, the adapted query is then mapped through that task's
deterministic M392-style rotation and tail-sketch projection.  Document postings
remain frozen.

This keeps the learned component task-agnostic while preserving task-specific
corpus geometry.

## Loss

The loss uses qrels-free dense teacher signals:

- exact dense score matching over dense/structural/BM25/random candidate groups;
- dense top-k listwise and pairwise losses;
- Dense O@100 preservation through base-structural anchoring on dense top
  candidates;
- query identity regularization to prevent adapter drift.

## First Canary

Run two surfaces:

1. FiQA-only seed403, to compare against M403-A.
2. Multitask seed404 on materialized MTEB tasks
   `FiQA2018,ArguAna,SCIDOCS,TRECCOVID`, using small train-group caps first.
   The current MTEB shared root does not contain `SciFact` or `NFCorpus`.

Pass gate:

- FiQA learned-only must beat deterministic structural tail;
- Dense O@100 must not regress more than M403-A;
- multitask macro should not collapse on non-FiQA tasks.
