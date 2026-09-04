# M1721 CLEAR-Style Residual Factorability Contract

## Objective

M1720A falsified the assumption that lexical-complementary dense information
is an orthogonal, lower-rank subspace. The measured residual became
higher-rank, less clusterable, and more imbalanced.

M1721 tests a distinct hypothesis:

> Complementarity may be relational at the query-document score level even
> when it is not an orthogonal embedding subspace.

The teacher is qrels-free and derived only from frozen BM25 and frozen BGE
rankings. At candidate depth 1,000, ranking `r` is converted to the fixed
discount `1 / log2(r + 2)`. The semantic residual target is:

```text
teacher(q, d) = relu(dense_discount(q, d) - bm25_discount(q, d))
target(q, d) = bm25_discount(q, d) + teacher(q, d)
             = max(bm25_discount(q, d), dense_discount(q, d))
```

This retains lexical evidence and asks a semantic branch to supply only the
dense advantage. It follows CLEAR's error-complement principle without using
qrels or assuming BM25 error is an orthogonal dense direction.

## Why This Is Not M1500-M1502

M1501 factorized a qrels/action-derived correction field and M1502 showed that
its factors were not predictable from held-out PPLX roots. M1721 instead uses
a corpus-computable teacher generated from two frozen first-stage retrievers.
The teacher is available at arbitrary scale and contains no qrel-selected
movement. The experiment still inherits M1502's central lesson: free-factor
capacity and text transcodability are separate gates.

## Frozen Audit

- BGE/shared3 embeddings and M1520 BM25 rankings;
- NFCorpus, SciFact, and FiQA, 100 queries each;
- candidate depth 1,000;
- rank discount fixed to `1 / log2(r + 2)`;
- relative ridge coefficient `1e-3`;
- bilinear rank 32;
- sparse diagnostic TopK 4;
- leave-one-dataset-out evaluation;
- no qrels, dataset thresholds, or metric grid in fitting or selection.

For each training corpus, ridge projection first asks whether document BGE
vectors can represent the score residual for each query. A second ridge map
learns one global query-to-document bilinear transform from the two training
datasets. The held-out dataset is scored as:

```text
semantic(q, d) = q^T A d
unified(q, d) = BM25_discount(q, d) + relu(semantic(q, d))
```

The rank-32 SVD of `A` is an explicit asymmetric query/document factorization
that can later be compiled into a shared posting namespace. M1721 does not yet
claim that those factors have acceptable DF.

## Diagnostics

Each row reports:

- teacher active density and rank-32 energy;
- free rank-32 reconstruction quality;
- document-vector projection ceiling;
- LODO full and rank-32 bilinear score cosine and R2;
- target-combined O@100;
- recovery of `dense_top1000 minus BM25_top1000`;
- qrels-based NDCG@10, MAP@100, Recall@100, MRR@20, and CUB;
- TopK-4 factor max/p99 DF and retrieval result as a product-shape warning.

Qrels are evaluation-only and cannot affect the transform or decision.

## Gates

At least two of three held-out rows must pass each stage.

1. **Free factor capacity**: rank-32 target O@100 `>=0.95` and
   dense-minus-BM25 recovery `>=0.90`.
2. **Document observability**: projected target O@100 `>=0.90` and recovery
   `>=0.90`.
3. **LODO transcodability**: rank-32 target O@100 `>=0.85`, recovery `>=0.80`,
   score cosine `>=0.70`, and predicted CUB `>=0.95` of the target CUB.

The TopK-4 result is diagnostic. It cannot authorize a product because only
32 factors make the expected DF too high. If LODO passes, the only authorized
next step is a balanced 4K+ residual-code source that preserves the learned
bilinear score while spreading document load. Text-head training remains
forbidden until that deterministic source passes exact DF and traversal gates.

## Stop Conditions

- If free rank-32 capacity fails, the score residual is not compact enough;
  stop without a neural model.
- If free capacity passes but document projection fails, the teacher is not
  observable from the frozen dense root.
- If document projection passes but LODO fails, the correction is
  corpus/query-specific and not a global text-to-posting function.
- Do not sweep rank, ridge, discount, candidate depth, or clipping after a
  failure.
- A qrels-positive result may describe retrieval utility but cannot override a
  failed qrels-free factorability gate.
