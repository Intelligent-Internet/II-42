# M401 Structural Dense-Tail Distillation Plan

## Motivation

M392-M396 is the current durable route because it preserves dense retrieval
through an index-shaped structure:

- active signed-coordinate postings;
- a compact joint-PCA dense-tail sketch;
- BM25 candidate union and low-weight z-score blend.

M371-M376 and M397-M400 tried adjacent neural posting/scoring ideas, but they
did not fully distill the M392-M396 structure. M401 starts that missing proof
surface.

## Hypothesis

A search-optimized encoder should not only emit sparse postings. It should
emit the structural pieces that made M392-M396 work:

- a rotated coordinate vector whose top coordinates form signed postings;
- a tail sketch compatible with dense-tail reranking;
- scores that preserve the M392 dense-tail teacher over candidate groups.

## First Canary Scope

This first M401 canary stays at the dense-embedding input layer:

`dense embedding -> learned structural active coordinates + learned tail sketch`

It does not yet train a text encoder. This isolates whether the structural
target and loss are viable before adding text-model complexity.

## Evaluation Gates

The canary must report:

- exact dense teacher;
- deterministic M392 structural teacher;
- learned structural dense-only route;
- learned structural + BM25 union route;
- touch ratio, Dense O@100, Recall@100, MRR@20, NDCG@10, MAP@100.

Qrels are used only for final held-out evaluation.

## Stop Rules

- If learned structural output cannot approach deterministic M392 on FiQA, do
  not scale to BEIR15/MTEB.
- If learned structural output matches deterministic M392 on FiQA, next run is
  BEIR15 subset/full and then MTEB 10-task.
- If structural dense-only works but BM25 union regresses, keep the encoder
  BM25-free and handle BM25 only as posthoc fusion.
