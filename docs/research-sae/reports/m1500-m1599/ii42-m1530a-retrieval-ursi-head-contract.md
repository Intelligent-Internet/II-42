# M1530A Retrieval-Supervised URSI Head Contract

Date: 2026-07-10

## Decision Boundary

M1530 starts a new representation source. M1520's qrels-free 32K codebook is
used only to initialize a trainable concept projection. It is not a frozen
teacher, posting source, or capacity claim.

M1530A asks one causal question:

> Can paired retrieval supervision train a shared concept head to reproduce
> teacher query-document ordering without destroying the bounded posting
> shape established by M1520?

This stage does not include BM25 in the loss, contextual routing, backbone
unfreezing, reconstruction loss, a cost-weight grid, or native index changes.

## Fixed Inputs

- Frozen retrieval backbone: `BAAI/bge-base-en-v1.5`.
- Trainable vocabulary: 32,768 concepts initialized by the locked M1520C
  centroids.
- Data: the pinned M1518 MS MARCO teacher artifact, with one positive, eight
  hard negatives, and frozen cross-encoder scores per query.
- Capacity smoke: at most 256 train and 128 validation rows.
- Query/document shared concepts with fixed unit impact scales and one
  trainable score-calibration scalar. Separate query/document scales are not
  identifiable under a dot product and are deliberately excluded.
- Token TopK=8, query K=24, document K=96.
- Non-negative semantic impacts and the M1520 background/null mask.

Train and validation query sets are already disjoint. BEIR qrels and rows are
not read in M1530A.

## Objective

For a query and its nine candidate documents:

```text
semantic_score(q, d) = dot(query_postings, document_postings)

L = KL(
    softmax(cross_encoder_scores),
    softmax(semantic_scores)
)
```

The objective deliberately contains no reconstruction or FLOPS term. Hard
TopK and background masking keep the capacity shape fixed while the experiment
isolates whether paired supervision can change shared posting membership.
Cost constraints are added only after alignment is demonstrated.

## Measurements

Every evaluation checkpoint reports:

- teacher KL;
- teacher top-1 agreement;
- teacher pair-order agreement;
- positive top-1 rate;
- query/document active concepts;
- unique active concepts;
- maximum concept DF and head 1% posting share;
- exact touched-document mean/p95 on the deduplicated validation candidate
  corpus;
- score scale and query/document impact scales.

The step-0 artifact is the matched frozen-initialization control.

## Promotion Gate

M1530A passes only if a measured checkpoint satisfies all conditions relative
to step 0:

- validation KL decreases by at least 10%;
- teacher top-1 agreement improves by at least 0.05, or teacher pair-order
  agreement improves by at least 0.03 without top-1 regression;
- exact sampled touch mean does not exceed the larger of 30% or 1.2 times the
  initial value;
- maximum DF does not exceed the larger of 50% or 1.2 times the initial value;
- active vocabulary remains at least 50% of the initial active vocabulary;
- all impacts remain finite and non-negative.

Checkpoint selection is conjunctive and uses evaluation metrics, never
training loss alone.

## Next Decisions

- Pass: scale the same head-only mechanism to the full 10K/1K artifact before
  adding BM25 residual supervision.
- Alignment improves but the gate is still rising at the final checkpoint:
  allow one depth extension of the same run, without changing the objective.
- Healthy optimization but no ordering improvement: allow one controlled
  final-layer/LoRA unfreeze as a new paired control.
- Collapse, flat ordering, or scale-only KL improvement: stop the source before
  larger training.

No result from this capacity smoke is a product-quality or OOD claim.

## Predeclared Depth Extension

The 200-step canary was numerically healthy and placed its lowest validation
KL at the final checkpoint, but did not pass the promotion gate. This triggers
the one depth extension allowed above.

The extension reruns from the same seed to 800 optimizer steps with the same
256/128 rows, objective, learning rate, budgets, and 50-step evaluation
interval. No hyperparameter grid is permitted. If no checkpoint passes the
conjunctive gate by step 800, head-only depth is considered exhausted for this
source. A lower KL without the required ordering gain is not sufficient to
continue scaling.
