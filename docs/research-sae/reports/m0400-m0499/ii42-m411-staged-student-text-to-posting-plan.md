# II-42 M411 Staged Student Text-to-Posting Plan

Date: 2026-06-27

## Goal

M411 starts the real student route after M410:

```text
raw text -> smaller semantic student -> dense/posting target
```

The target is not qrels and not BM25.  The first target is the materialized
`pplx-embed-v1-0.6B` dense/quantized embedding, then the M408 deterministic
posting representation derived from it.

## Key M410 Constraint

Queries must use no prefix for the current materialized data.

M410 sample parity:

- documents: dense cosine `0.99919`, active Jaccard `0.99412`;
- queries with `query: `: dense cosine `0.91077`, active Jaccard `0.56938`;
- queries with no prefix: dense cosine `1.00000`, active Jaccard `1.00000`.

Therefore M411 must not add `query: ` unless a new embedding corpus is created
with that prompt convention.

## What Is Different From Earlier Text-Atom Lines

Earlier MiniLM/text-atom routes tried to emit sparse atoms directly and then
used ranking/fusion surfaces to judge them.  M411 is staged:

1. learn the dense teacher function;
2. learn the deterministic posting projection;
3. only then test retrieval;
4. only after representation parity is strong, consider retrieval-aware
   fine-tuning.

This avoids hiding a bad encoder behind BM25 or qrels-specific ranking loss.

## Stage A: Dense Teacher Imitation

Train a student to match the materialized dense vector from text.

Primary losses:

- cosine loss against L2-normalized materialized dense;
- MSE against L2-normalized materialized dense;
- optional quantizer-aware loss if the teacher output is quantized.

Primary diagnostics:

- doc/query dense cosine;
- dense top-k overlap on heldout queries;
- exact dense NDCG/Recall using student vectors.

No posting loss yet.  No qrels in the loss.

## Stage B: Posting Imitation

Freeze or warm-start from Stage A, then add M408 targets:

- rotated coordinate MSE;
- active support and signed magnitude loss;
- inactive coordinate loss;
- tail-sketch cosine/MSE.

Primary diagnostics:

- active Jaccard;
- coordinate cosine;
- sketch cosine;
- structural-tail retrieval and BM25-blended retrieval as diagnostics only.

## Stage C: Efficiency And Compression

Only after Stage B passes, compress:

- smaller hidden size;
- lower output dimension;
- int8/quantized head;
- active-k/fanout caps.

Compression must preserve dense/posting parity first.  Retrieval gains from
BM25 blending are not promotion evidence by themselves.

## Candidate Model Ladder

Run from lowest risk to highest risk:

1. **Frozen pretrained encoder + trainable projection head.**
   - Fast sanity gate.
   - Tests whether a different semantic encoder already contains enough
     signal to approximate the pplx teacher.
2. **Trainable projection plus last-layer/adapter tuning.**
   - More capacity without fully fine-tuning a large model.
3. **Small end-to-end student.**
   - Only after Stage A metrics show the frozen route is insufficient.
4. **Pplx adapter upper bound.**
   - Not the product target, but useful to prove the loss and data are able to
     improve beyond frozen projection.

## First Gate

After M410 full FiQA completes:

1. Run M411 Stage A on FiQA with heldout query split.
2. Do not use qrels in training.
3. Compare:
   - materialized exact dense;
   - M410 re-encoded exact dense;
   - M411 student exact dense;
   - M408 structural-tail teacher;
   - M411 student structural-tail.

Promotion requires the student to recover dense parity before posting/ranking
claims are considered.
