# M1910 Latent Terms Native BMP Reopening Contract

Date: 2026-07-12

Status: **locked before full FiQA encoding**

## Question

M1540A reproduced the representation and scoring shape from *Latent Terms*:
retriever token states, a reconstruction-trained TopK SAE, sum pooling, a
square-root transform, and BM25 over latent features. It retained useful
semantic signal but was stopped because the raw union of posting lists touched
most canary documents.

M1660 later showed that raw union touch is not a valid production latency
proxy. The audited BMP engine can skip large parts of a learned-sparse index
while preserving exact top-k scores. M1910 therefore asks one narrow question:

> Does the frozen M1540A latent vocabulary remain impractical when its exact
> BM25 score is compiled into the real BMP inverted-index path?

This is an engine-interface correction, not another model or loss experiment.

## Frozen Inputs

- Backbone: the unchanged M1540A `BAAI/bge-base-en-v1.5` revision.
- SAE: the unchanged M1540A 32,768-dimensional TopK-16 checkpoint.
- Evaluation: complete official FiQA, 57,638 documents and 648 qrel queries.
- Pooling: token sum followed by square root.
- Scoring: global untuned latent BM25, `k1=8`, `b=0.7`.
- Engine: the M1660 audited BMP source and exact-top-k patch, block size 16.
- Quantization: the unchanged M1660 global document u8 and per-query scaling.

No qrels, dataset ID, retrieval loss, feature threshold, or top-k clipping may
affect encoding or scoring.

## Score Compilation

Latent BM25 is compiled into one sparse dot product. For latent term `t` and
document `d`, the stored document impact is:

```text
idf(t) * tf(t,d) * (k1 + 1)
---------------------------------
tf(t,d) + k1 * length_norm(d)
```

The query impact is the square-root transformed latent activation. This is
algebraically the same M1540A score and remains one physical inverted index.

## Surfaces

1. `full`: every positive pooled latent feature.
2. `prune1`: remove the corpus-global top 1% highest-DF latent dimensions.

`prune1` is a predeclared paper ablation, not a rescue grid. The full surface
is primary. The pruned surface is accepted only if it lowers cost without more
than a 2% relative Recall@100 loss.

## Gates

The route is retained for a Nomic/FineWeb reproduction only if:

1. BMP has exact score-multiset and strict-boundary parity on all queries.
2. Quantized Recall@100 retains at least 98% of the float latent reference.
3. BMP p95 is lower than exhaustive sparse scoring p95.
4. Index bytes and latency are finite and reported.
5. Full-FiQA latent quality is not materially below the M1540A canary
   retention ratio, or `prune1` preserves at least 98% of full Recall@100.

Passing these gates does not promote M1540A. It authorizes M1911: a clean
paper-shaped reproduction using Nomic token states and qrels-free FineWeb-Edu
SAE training. Failing exact native cost closes M1540A and prevents expensive
retraining on the same representation.
