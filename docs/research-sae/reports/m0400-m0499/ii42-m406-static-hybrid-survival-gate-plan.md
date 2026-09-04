# II-42 M406 Static-Hybrid Survival Gate Plan

Date: 2026-06-27

## Motivation

M405 showed that structural-space query correction can preserve Dense O@100
better than M403-A, but the 4-task hybrid macro still lagged deterministic
structural + static BM25.

The next question is whether the failure is the admission layer: the current
route admits structural candidates with sparse coordinate scores, then applies
static BM25 blend after admission.  Some documents that would score well under
the final static blend may never enter the structural candidate set.

## Hypothesis

Train a qrels-free candidate survival gate:

```text
structural candidate features -> probability candidate survives static hybrid top-k
```

The final ranking remains deterministic:

```text
score = zscore(structural_tail_score) + fixed_alpha * zscore(BM25)
```

No learned BM25 alpha is used.

## Required Baselines

M406 must compare against two non-neural baselines:

- sparse admission: current deterministic route;
- direct static-blend admission: select structural candidates by the same fixed
  hybrid score used for final ranking.

If direct static-blend admission wins, the right next step is an index/runtime
change, not a neural gate.

## Gate

The survival target is qrels-free.  For each train query:

1. Build all structural candidates from signed-coordinate postings.
2. Add BM25 top candidates.
3. Score the union with fixed static BM25 blend.
4. Label structural candidates that survive the hybrid top-k as positives.

The gate sees only runtime-available features: sparse admission score,
structural tail score, BM25 score, and simple score interactions.

## First Gate

Run FiQA seed404 first.  Continue only if at least one gate/static-admission
route beats deterministic structural + static BM25 without increasing fanout.
