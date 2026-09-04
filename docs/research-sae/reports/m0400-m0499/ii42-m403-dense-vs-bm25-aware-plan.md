# II-42 M403 Dense-Faithful vs BM25-Aware Plan

Date: 2026-06-27

## Motivation

M402 made the residual initialized encoder stable, but learned-only still did
not beat deterministic structural tail.  The weak positive signal appeared
only after BM25 blend.  M403 therefore splits the next step into two branches.

## Branch A: Dense-Faithful Query Correction

Keep document postings deterministic and learn only query-side correction:

- input: dense query embedding;
- base: deterministic rotated query coordinates and tail sketch;
- trainable: small residual correction to query coordinates/sketch;
- target: exact dense top-k scores;
- no BM25 in the pass condition.

Pass gate:

- learned-only NDCG@10 must beat deterministic structural tail;
- Dense O@100 should not drop.

## Branch B: BM25-Aware Hybrid

Treat the BM25 signal as part of the model, not as a post-hoc rescue:

- input: dense query embedding plus BM25 candidate scores at training/eval;
- base: same query-side correction as Branch A;
- trainable: query-conditioned fusion weight between structural and BM25
  scores;
- target: exact dense scores over dense/structural/BM25 candidate groups.

Pass gate:

- learned hybrid must beat deterministic structural + static BM25 blend;
- this does not count as dense-faithful success unless learned-only also wins.

## First Canary

Task: `FiQA2018`

Run both branches with CPU-only low-priority settings on `spark-2`, because
`spark-2` currently has active GPU work that must not be disturbed.
