# II-42 M402 Direct Dense-Rank Encoder Plan

Date: 2026-06-27

## Motivation

M401 showed that the M392-M396 structural dense-tail target remains valuable,
but the learned shared encoder collapses when trained on all FiQA train
queries.  The failure mode is not lack of depth.  It is the loss: global
coordinate/sketch reconstruction plus group score matching does not preserve
per-query dense rank geometry.

M402 changes the proof surface:

- train against exact dense top-k scores, not M392 structural scores;
- initialize from the deterministic M392 structural transform;
- learn only residual calibration first, instead of relearning the geometry
  from random initialization;
- require learned-only to beat deterministic structural tail before scaling.

## First Canary

Task: `FiQA2018`

Inputs:

- dense document/query embeddings from the existing MTEB shared root;
- qrels only for held-out evaluation;
- dense dot-product scores for qrels-free training groups.

Default model:

- separate document and query projection heads;
- frozen deterministic base maps;
- trainable per-dimension scales;
- zero-initialized residual linear maps;
- active signed coordinates plus tail sketch retrieval.

## Success Gate

The first gate is learned-only NDCG@10 on held-out FiQA queries:

- must beat deterministic structural tail alone;
- should preserve or improve Dense O@100;
- BM25 blend is reported, but cannot be used as the primary pass condition.

## Stop Rules

Stop this line if:

- learned-only stays below deterministic structural tail after residual
  calibration;
- increasing epochs degrades Dense O@100;
- improvements only appear after BM25 blend.

If M402 passes the FiQA gate, run BEIR15 standard recall matrix next.
