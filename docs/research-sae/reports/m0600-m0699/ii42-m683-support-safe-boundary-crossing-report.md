# M683 Support-Safe Boundary Crossing Audit

## Status

M683 produces the first strong positive signal after M682.

It confirms that the next route should not return to traditional SAE
reconstruction, and should not enlarge a global atom table. The useful shape is
query-local generated posting with explicit support and rank safety.

This is still an oracle feasibility result. M683 uses M681 qrels-positive
documents at evaluation time to generate query deltas. It is not a deployable
model and must not be reported as a production score.

## Question

Can generated postings cross the top100 boundary while preserving:

- candidate upper bound,
- protected head geometry,
- NDCG/MRR rank quality,
- and native DB evaluation semantics?

M682 answered that a simple global atom table cannot do this. M683 tests
whether query-local generated postings have the right target shape at all.

## Setup

- Surface: native shared15 PostgreSQL path
- Queries: `1342`
- Event queries with M681 under-ranked positives: `337`
- Teacher events loaded: `13885`
- Generated deltas:
  - append top `4` or `8` positive-doc atoms,
  - scale `0.03` or `0.05`,
  - shared same-sign boost `1.0` or `1.15`,
  - top95 floor `0.97`.
- Output teacher rows:
  `runs/m683_support_safe_boundary_crossing_v1/m683_oracle_rows.jsonl`

The `m683_oracle_rows.jsonl` file has one row per evaluated query and is small
enough to keep. It is the input for the next non-oracle selector/compiler
stage.

## Main Result

| Source | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 0.864763 | 0.667856 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| M674 | 0.870086 | 0.668196 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| support-safe oracle | 0.869953 | 0.669036 | 0.737631 | 0.821142 | 0.947551 | 0.997796 |
| rank-safe oracle | 0.869697 | 0.668958 | 0.737834 | 0.821142 | 0.947495 | 0.997937 |
| fixed append8/s0.05/shared1.15 | 0.872515 | 0.669758 | 0.738118 | 0.821576 | 0.947386 | 0.988634 |

Delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 |
| support-safe oracle | +0.005190 | +0.001180 | +0.000345 | +0.000263 | +0.000445 |
| rank-safe oracle | +0.004934 | +0.001102 | +0.000548 | +0.000263 | +0.000390 |
| fixed append8/s0.05/shared1.15 | +0.007752 | +0.001902 | +0.000831 | +0.000698 | +0.000281 |

## What This Proves

1. Support-safe boundary crossing exists.

   The rank-safe oracle improves Recall, MAP, NDCG, MRR, and CUB together.
   This directly answers the M682 blocker: the problem is not that generated
   posting cannot cross safely; the problem is that global atom tables cannot
   choose the safe crossing pattern.

2. The target shape can beat M674.

   The fixed oracle `append8_s0.05_shared1.15` beats M674 on Recall, MAP,
   NDCG, MRR, and CUB. It does not preserve the head as strictly as M674, but
   top95 remains `0.988634`, above the M683 floor.

3. The signal is not a single dataset artifact.

   Support-safe oracle improves the event-bearing datasets broadly:
   `climate-fever`, `cqadupstack`, `dbpedia-entity`, `fiqa`, `msmarco`,
   `nfcorpus`, `scidocs`, `scifact`, `trec-covid`, and `webis-touche2020`.

4. Dense-only mimic is no longer the bottleneck.

   The improvement comes from retrieval-expanded generated postings. This
   supports the critique that the main route should be dense-faithful plus
   retrieval-constrained, not dense mimic alone and not SAE reconstruction.

## What This Does Not Prove

M683 does not prove we have a deployable model.

It uses qrels-positive document atoms at evaluation time. That is deliberate:
this is an oracle feasibility audit. The next stage must replace oracle
knowledge with a learned or deterministic query-local selector that uses only
allowed inference-time signals.

M683 also does not justify unrestricted delta generation. The support/rank-safe
oracle is valuable because it rejects unsafe deltas. M682 already showed that
uncontrolled global modifications can lower candidate upper bound.

## Core Breakthrough Signal

The useful primitive is:

> Generate query-local posting deltas from candidate/document atom evidence, but
> admit them only when the boundary displacement is support-safe and rank-safe.

This is the first recent result where the oracle target can exceed M674 on the
full native shared15 matrix while also improving CUB.

## Next Step

Proceed to M684:

1. Use `m683_oracle_rows.jsonl` as the teacher surface.
2. Train a non-oracle query-local selector/compiler that predicts when to apply
   the accepted delta shape.
3. Use inference-time features only:
   - query atom shape,
   - BM25/native candidate features,
   - support overlap,
   - atom fanout/IDF style features,
   - candidate boundary margins.
4. Keep train/eval split discipline:
   - do not select model or threshold on full eval qrels,
   - evaluate through the same native shared15 path.
5. Gate against both P1 baseline and M674:
   - Recall/MAP must improve over P1,
   - CUB must not drop,
   - NDCG/MRR must not materially regress,
   - if it cannot approach M674 on Recall or beat M674 on MAP/CUB, stop or
     redesign the selector.

## Decision

Promote the M683 target shape to the next training stage.

Do not promote the oracle itself. The next real work is a query-local,
support-safe generated-posting compiler that approximates the accepted M683
oracle deltas without qrels at inference.
