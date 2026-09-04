# II-42 M397 Dense-Distilled Posting Encoder Plan

## Goal

M397 starts a new model line that should not be called traditional SAE.

The target is a search-optimized posting encoder:

> encode a query or document into signed sparse postings that can be indexed
> directly, while distilling the retrieval behavior of the dense teacher.

The primary objective is not embedding reconstruction. It is dense-neighbor and
dense-ranking preservation under posting cost constraints.

## Why This Is Worth Testing

M392-M396 showed that dense retrieval quality can be preserved surprisingly well
when dense geometry is converted into a posting-shaped route:

- signed dense-coordinate postings
- joint-PCA dense-tail sketch
- low-touch candidate admission
- small BM25 blend only after the dense-tail route is already strong

This means we now understand the required shape better than before. A learned
encoder can target that shape directly instead of hoping a reconstruction SAE
accidentally preserves it.

## Non-Goals

- Do not train on qrels.
- Do not optimize per dataset.
- Do not use reconstruction as the primary loss.
- Do not treat a BM25 blend improvement as proof that the posting encoder got
  better.

## Stage A: Dense to Posting Canary

Input is the existing dense embedding. The model learns a posting projection
head:

- shared projection: same encoder for queries and documents
- dual projection: separate query and document posting heads

Training signals:

- weighted dense-score regression on teacher top-k and random negatives
- rank-weighted emphasis on dense top neighbors
- optional pairwise/listwise extension after the regression canary is stable

Eval surfaces:

- BM25-free learned posting retrieval
- BM25 posthoc union/blend with learned posting scores
- deterministic PCA/signed-coordinate baseline
- exact dense teacher baseline

Pass criteria:

- learned posting must improve over deterministic PCA/signed-coordinate posting
  or match it at lower touch ratio;
- BM25 posthoc may improve final score, but it cannot be used to hide a weak
  posting encoder;
- dense-overlap metrics must move with qrel metrics.

## Stage B: Text to Posting Encoder

Only start after Stage A shows that the target/loss is valid.

The text encoder should emit signed sparse postings directly. Candidate model
families:

- frozen text encoder plus trainable posting head
- LoRA/adapter text encoder plus posting head
- dual query/document heads with shared trunk

Training remains qrels-free:

- dense teacher top-k ranking distillation
- dense top-k overlap
- posting budget and DF balance
- quantization/indexability constraints

## BM25 Research Questions

BM25 should be tested in three separate places:

1. `BM25-free`: train and evaluate the posting encoder alone.
2. `posthoc BM25`: train posting encoder alone, then union/blend with BM25 at
   retrieval time.
3. `BM25-aware training`: include BM25 candidate/score features in the loss or
   gating objective, but still train without qrels.

The first question answers whether the encoder itself learned the dense search
shape. The second answers product quality. The third answers whether BM25
should influence the learned posting shape. These should not be collapsed into
one metric.

## Immediate Experiment

Run `scripts/research_sae_m397_dense_distilled_posting_encoder.py` as a small
canary:

- default tasks: `FiQA2018`
- variants: `shared`, `dual`
- posting dims: `256`
- active dims: `128`
- budget: `0.08`
- BM25 alphas: `0.10`, `0.18`

If this small canary fails to beat or approach deterministic PCA postings, the
next step is to revise the loss before scaling to larger training.
