# II-42 M1940 CLEAR-Style Residual Canary Contract

Date: 2026-07-13

## Authorization

M1939 passed all nine predeclared observability gates on the query-disjoint
RLHN surface. Heldout contained 133 BM25-error rows and 58 BM25 plus M1914
joint errors. M1914 rescued 56.39% of BM25 errors; PPLX rescued 43.10% of the
remaining joint errors. Train/heldout behavior was stable.

The signal is uneven: 56 of 58 heldout joint errors are NQ rows. M1940 must
therefore prove source-balanced transfer rather than optimize the aggregate.

## Objective

Test whether a mature M1914/Granite sparse parent can absorb lexical-residual
supervision without losing its released sparse geometry, lexical-correct rows,
or fixed-support cost shape.

This is a bounded public-data canary. It is not a BEIR-selected checkpoint and
is not a product result.

## Training Difference From M1918

M1918 applied parent KL, PPLX KL, and generic pairwise loss to every sampled
row. It improved a small heldout score but failed full native transfer.

M1940 changes the causal training unit:

1. classify rows using a fixed BM25 candidate-universe score;
2. balance six groups: three corpora times residual/protection;
3. apply CLEAR-style pairwise pressure only on BM25-error rows;
4. apply PPLX KL only when BM25 and M1914 both fail and PPLX succeeds;
5. preserve parent score geometry and support on every row;
6. retain periodic corpus-DF control.

The residual margin uses a training-only robust BM25 scale:

```text
required = 0.20 + 0.50 * clamp(-bm25_margin / median_error_scale, 0, 2)
loss = softplus(required - standardized_semantic_margin)
```

No BEIR qrels, dataset alpha, or post-hoc threshold enters training or
selection.

## Canary

- parent: fixed `ibm-granite/granite-embedding-30m-sparse` revision;
- query support: 50;
- document support: 192;
- trainable: last transformer layer and existing LM head only;
- steps: 256;
- checkpoints: 0, 32, 64, 128, 256;
- source: 2,048 train / 512 query-disjoint heldout M1918 rows;
- tracking: ClearML project `II42/M1940`.

## Heldout Gate

A trained checkpoint passes only if it simultaneously:

- improves global BM25-error rescue by at least 0.010;
- improves equal-corpus rescue by at least 0.005;
- loses no more than 0.020 rescue on any corpus;
- increases global lexical-correct harm by at most 0.005;
- increases lexical-correct harm by at most 0.020 on every corpus;
- does not regress pairwise accuracy, positive top1, or positive MRR;
- preserves at least 99% of parent-positive top1 rows;
- keeps support cosine at least 0.995;
- keeps mean document support at most 1.05x parent;
- keeps sampled maxDF at most parent plus 0.005.

## Escalation

Only a passing heldout checkpoint may be regenerated over the full M1916
four-row native surface. Native success then requires all macro quality
metrics positive, no Recall row below -0.01, and cost within the M1934 b1.125
budget. The unseen M1934 three-row transfer remains mandatory before any
promotion claim.

## Stop Conditions

- Stop if no checkpoint passes the joint heldout gate.
- Stop if the only gain is NQ aggregate rescue.
- Stop if rescue is purchased by lexical-correct harm or support drift.
- Stop if native replay removes the public heldout gain.
- Do not extend steps, change gates, or sweep weights after observing this
  canary; a follow-up requires a new mechanism and contract.
