# M733 Evidence Synthesis and Design

Status: `design_ready_for_m733a`

M733 reframes the next route as a constrained retrieval-conditioned
unified-posting compiler.  The goal is not to return to traditional SAE
reconstruction and not to continue blind first-stage query residual training.
The deployable form must remain native unified posting: query-local posting
delta, atom admission, scale, or gate over frozen `P1.3 / M549U native
signed-dot` document/index geometry.

## Objective

Build a compiler that can approximate the M683 support-safe boundary crossing
signal without using qrels or dense-overlap deltas at inference time.

The compiler may use native retrieval context available at query time:

- P1/native candidate ranks and scores;
- BM25/native lexical evidence;
- atom overlap, fanout, source channel, and IDF-style statistics;
- boundary margins and support/head-risk features;
- query atom shape and candidate atom source visibility.

It must not become a plain reranker, BM25 alpha search, or dataset-specific
threshold policy.

## Evidence Boundary

### M683: oracle route is feasible

M683 is the core positive feasibility result.  It showed that query-local
generated posting can cross the top100 boundary while preserving the head:

| Source | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |
| support-safe oracle | +0.005190 | +0.001180 | +0.000345 | +0.000263 | +0.000445 | 0.997796 |
| rank-safe oracle | +0.004934 | +0.001102 | +0.000548 | +0.000263 | +0.000390 | 0.997937 |
| fixed append8/s0.05/shared1.15 | +0.007752 | +0.001902 | +0.000831 | +0.000698 | +0.000281 | 0.988634 |

This proves that support-safe retrieval expansion exists.  It does not prove a
deployable model, because M683 uses qrels-positive event documents at
evaluation time.

### M684-M685: selector and document-copy deltas are insufficient

M684 trained a non-oracle query-level safe selector from M683 labels.  The gate
had signal (`holdout AUC 0.787937`) and kept Top95 high when applied narrowly,
but the selected candidate/doc atom deltas did not reproduce M683 gains.

M685 widened the proposal pool and learned boundary-relative candidate
features.  It improved target visibility, but copying or averaging raw selected
document atoms still failed: the conservative variant preserved head geometry
better but still lost holdout Recall/MAP.

Decision: M733 must not repeat a pure query-level selector or selected-doc atom
copying.  The selector surface is useful, but the delta basis is wrong.

### M686-M689: atom-level delta has signal, admission alone stops

M686 changed the target from document selection to atom-level delta prediction.
It produced the first non-oracle positive movement after M685, but still did
not beat M674 and carried small head/rank risk.

M687 and M688 showed that metric/head-risk gates can be learned, but safety
prediction is not the same as recall recovery.  M689 then showed a hard stop
for admission-gate tuning over the same proposal surface: most recall-bearing
target atoms were not visible to the candidate atom pool.

Decision: M733 should keep atom-level deltas, but it must change the atom
candidate interface before training another gate.

### M690-M698: source-aware atom interface is the live route

M690 identified the proposal gap between M683 retrieval-oracle targets and the
current atom proposals.  M691 showed that a retrieval-teacher proposal expansion
can produce substantially more strict safe+Recall rows.  M692 learned part of
that target, but the current inference-compatible atom source still exposed too
few target atoms.

M693 and M694 localized the bottleneck: teacher-positive-only source channels
recover target atoms, while mixed candidate ranking truncates those atoms away.
Source-aware quotas are therefore not an implementation detail; they are the
interface required before a compiler has a learnable target.

M695 and M696 tested qrels-free corpus-positive sources.  They did not recover
enough teacher-positive atoms, and the gap was mostly at atom candidate
truncation rather than document retrieval alone.

M697 was the strongest later positive signal: expanded qrels-free
source-specific atom budgets closed the visibility gap enough to justify the
next implementation target.  M698/M699 then showed that dense-boundary atom
interfaces improved over M700 but still did not create enough full-hit rows for
deep compiler training.

Decision: M733A should start from source-aware atom candidate channels and
fixed/limited delta families, not from a blind global residual.

### M700-M732: do not solve this by training longer

M700, M702, and M728 specifically test the "training depth" concern.  M700 and
M702 found interface ceilings before compiler training: label-visible
upper bounds or selected atom visibility were insufficient.  M728 audited 30
trace-bearing runs and found that loss usually decreased, while O@100/O@256
often moved downward.  Deeper optimization of the same surrogate is therefore
not the main missing ingredient.

M721/M721b showed that pair-interaction features can move dense-boundary pairs
across shared15.  M725/M726 showed that global scale is unsafe and post-hoc
dense gates are near-pass but not deployable.  M727 showed that the current
pre-retrieval proxy overfits train (`AUC 0.969454`) and barely transfers
(`eval AUC 0.569876`).

M729/M730/M731/M732 close the remaining blind query-side branches:

- M729: coverage does not explain tail overlap failure;
- M730: two-head dense-tail compiler still fails dense-equivalence gates;
- M731: low-rank oracle safe subspace is weak and predictor fails;
- M732: PCA-style query-side reconstruction causes broad test O@100
  instability.

Decision: do not expand another blind query-side compiler, low-rank PCA
compiler, or longer version of the same loss.  M733 needs retrieval-conditioned
features and hard native gates.

## Research Support

This design is consistent with the external IR literature, but the promotion
gate remains local native evidence:

- SPLADE and SPLADE v2 support learned sparse retrieval as a valid first-stage
  inverted-index form, but they use IR-aligned objectives rather than generic
  reconstruction.
- DeepCT/DeepImpact-style impact models support contextual term/impact
  weighting stored in inverted indexes.
- ColBERT supports retaining candidate-level interaction instead of collapsing
  all retrieval behavior into one global vector.
- DREAM supports the principle that retriever supervision should come from
  functional retrieval use and candidate competition, not only from embedding
  shape mimicry.  M733 does not adopt a frozen LLM judge in this phase; it
  borrows only the interface lesson.

## M733 Design

M733 is a first/second-stage merged compiler:

1. Stage A: frozen native substrate.
   - Keep `P1.3 / M549U native signed-dot` document posting geometry frozen.
   - Use it to retrieve the candidate context and source-aware atom channels.

2. Stage B: retrieval-conditioned posting compiler.
   - Input: query atoms, candidate ranks/scores, BM25/native evidence,
     boundary margins, atom source channels, support/head-risk features.
   - Output: query-local atom delta, admission, scale, or quota.
   - No plain rerank score is accepted as the final output.

3. Stage C: hard constrained selection.
   - Apply only if dense-equivalence and retrieval gates remain non-negative.
   - Rejection is part of the model; accepted-delta rate is reported.

## M733A: First Executable Probe

M733A must not rerun M684.  M684 already answered "can a query-level selector
choose safe movement?" with "partially yes, but proposal/delta basis fails."

M733A should instead test a narrow source-aware atom interface:

1. Build teacher rows from M683/M691/M697 evidence.
2. Candidate atoms come from source-specific channels:
   - current native/P1 support;
   - BM25/native head or tail evidence;
   - qrels-free support/composite source rows that survived M697 visibility;
   - protected dense/P1 head atoms as explicit negatives or hard floors.
3. Delta family is fixed:
   - no arbitrary vector generation;
   - small append/boost families only;
   - source-aware atom quotas before scoring.
4. Train only a small selector/admission model first:
   - logistic or GBDT;
   - global threshold learned on train split only;
   - no dataset-specific tuning.
5. Evaluate through native canary:
   - `fiqa,arguana,scidocs,cqadupstack`;
   - expand to shared15 only if canary passes.

## Metrics and Gates

Report every run with:

- NDCG@10;
- MAP@100;
- Recall@100;
- MRR@20;
- dense overlap@10/@50/@100/@256;
- candidate upper bound;
- support/head-preservation diagnostics;
- accepted-delta rate;
- failure classes:
  - no safe source atoms;
  - unsafe head risk;
  - CUB regression;
  - O@100/O@256 regression;
  - retrieval metrics negative;
  - selector abstained.

Promotion gates:

- `Recall@100 >= P1`;
- `MAP@100 >= P1`;
- `O@100 >= P1`;
- `O@256` no material regression;
- `CUB >= P1`;
- no material NDCG@10/MRR@20 regression;
- gains must appear on multiple datasets.

## Stop Conditions

Stop or redesign if any of these occur:

1. Safe movement remains oracle-only and cannot be selected from inference-time
   source-aware atom features.
2. Gains require qrels-positive documents, post-hoc dense-overlap gates, or
   dataset-specific thresholds at inference.
3. Output collapses into a reranker score rather than unified-posting behavior.
4. M733A canary repeats M684/M685: positive gate AUC but no Recall/MAP lift.
5. M733B/C produce gains only by spending O@100/O@256/CUB.

## Current Recommendation

Proceed with M733A only as a source-aware atom-interface probe.  The prior work
already rules out the tempting shortcuts:

- not SAE reconstruction;
- not fixed alpha search;
- not plain reranker;
- not another query-level selector over raw candidate-doc atoms;
- not deeper training on the same surrogate.

The highest-value next test is whether the M697-style source-aware atom budget,
combined with M683 support-safe delta families and hard dense floors, can
produce a small but real native canary lift without qrels-like runtime
features.
