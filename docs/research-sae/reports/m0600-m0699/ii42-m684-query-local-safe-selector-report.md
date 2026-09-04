# M684 Query-Local Safe Selector

## Status

M684 is complete and should not be promoted.

It is the first non-oracle approximation of M683. It trains a candidate
selector from M681 broader retrieval events and a query-level gate from M683
rank-safe oracle labels. At inference time it uses only allowed query/native
candidate features, indexed candidate atoms, and BM25/native tail signals.

The result is a useful negative signal. The gate learns a real support-safe
pattern, but the candidate proposal interface does not reproduce the M683
oracle gains.

## Setup

- Surface: native shared15 PostgreSQL path
- Queries: `1342`
- Holdout queries: `244`
- Candidate selector rows: `5520`
- Positive selector rows: `165`
- Positive selector share: `0.029891`
- Expansion:
  - selected candidate docs: `3`
  - doc atom head: `8`
  - expansion scale: `0.05`
  - shared same-sign boost: `1.15`
- Gate threshold mode: train-only F0.5
- Artifacts:
  - `runs/m684_query_local_safe_selector_v1/m684_summary.json`
  - `runs/m684_query_local_safe_selector_v1/m684_report.md`

## Gate Quality

| Split | Rows | Positives | AUC | Precision | Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| train | 1098 | 168 | 0.812999 | 0.503401 | 0.440476 |
| holdout | 244 | 41 | 0.787937 | 0.500000 | 0.414634 |

The gate is not random. It has enough signal to identify a subset of queries
where generated posting might be safe. This matters because the M684 failure
should not be interpreted as "support-safe boundary crossing is impossible".

The failure is downstream: the selected candidate documents and their raw atom
deltas are not the same as the M683 oracle deltas.

## Full Native Surface

| Source | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Top95 | Apply rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 0.864763 | 0.667856 | 0.737286 | 0.820878 | 0.947106 | 1.000000 | 0.000000 |
| M674 | 0.870086 | 0.668196 | 0.737286 | 0.820878 | 0.947106 | 1.000000 | 1.000000 |
| bm25_expansion_all | 0.866056 | 0.666991 | 0.736871 | 0.820334 | 0.946935 | 0.966703 | 1.000000 |
| selector_expansion_all | 0.864243 | 0.668356 | 0.737270 | 0.821886 | 0.945995 | 0.955322 | 1.000000 |
| selector_expansion_gate | 0.864916 | 0.667954 | 0.737210 | 0.820697 | 0.946916 | 0.993937 | 0.134873 |

Delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 |
| bm25_expansion_all | +0.001293 | -0.000865 | -0.000415 | -0.000545 | -0.000171 |
| selector_expansion_all | -0.000520 | +0.000500 | -0.000017 | +0.001007 | -0.001111 |
| selector_expansion_gate | +0.000153 | +0.000098 | -0.000077 | -0.000181 | -0.000190 |

## Holdout Surface

| Source | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Top95 | Apply rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 0.847609 | 0.660034 | 0.728544 | 0.808663 | 0.936393 | 1.000000 | 0.000000 |
| M674 | 0.851356 | 0.660224 | 0.728544 | 0.808663 | 0.936393 | 1.000000 | 1.000000 |
| bm25_expansion_all | 0.849399 | 0.657752 | 0.726643 | 0.803864 | 0.935536 | 0.966048 | 1.000000 |
| selector_expansion_all | 0.848154 | 0.659570 | 0.727900 | 0.805746 | 0.937436 | 0.954530 | 1.000000 |
| selector_expansion_gate | 0.847303 | 0.660120 | 0.729045 | 0.808743 | 0.936607 | 0.992278 | 0.139344 |

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 |
| bm25_expansion_all | +0.001790 | -0.002282 | -0.001901 | -0.004799 | -0.000857 |
| selector_expansion_all | +0.000545 | -0.000464 | -0.000644 | -0.002917 | +0.001043 |
| selector_expansion_gate | -0.000306 | +0.000086 | +0.000501 | +0.000080 | +0.000215 |

## Interpretation

M684 validates the route critique but not the specific implementation.

The useful high-level route remains:

1. Keep dense-root unified posting as the engineering form.
2. Do not return to traditional SAE reconstruction loss.
3. Preserve dense support and head geometry as hard floors.
4. Add retrieval-constrained generated postings only when boundary crossing is
   support-safe and rank-safe.

M684 shows that the next bottleneck is not the query-level gate. The gate has
holdout AUC `0.787937` and keeps top95 high after applying to only `13.93%` of
holdout queries.

The bottleneck is proposal quality:

- M683 oracle uses qrels-positive document atoms at evaluation time.
- M684 selects visible native/BM25 tail candidate docs without qrels.
- The candidate-positive training surface is sparse: only `2.99%` positives.
- Raw selected-doc atom deltas do not reliably match the oracle boundary delta.

This explains the metric shape:

- applying BM25 expansion everywhere improves holdout Recall but damages
  MAP/NDCG/MRR;
- applying selector expansion everywhere improves CUB but damages rank metrics;
- gating repairs some head/rank risk but removes the Recall gain.

## Relation To The Review

The review's main claim is consistent with the data:

- original SAE reconstruction is not the route to return to;
- direct/unified posting remains the right engineering shape;
- dense-only mimic and small scorer tweaks are now at a bottleneck;
- the next route should be dense-faithful plus retrieval-constrained generated
  posting.

M684 adds one refinement: "retrieval-constrained" is not enough by itself. The
system also needs a better proposal interface. A query-local gate can tell when
not to move, but it cannot synthesize the right atoms if the proposal docs are
wrong or if raw candidate atoms are the wrong delta basis.

## Decision

Reject M684 as a promotable route.

Keep it as a negative diagnostic and as evidence that the next version should
not be another query-level gate. The next version must improve proposal/delta
construction.

## Next Step

Proceed to M685 only if it changes the proposal interface.

Recommended M685:

1. Train a candidate-level boundary proposal model from M683 accepted rows, not
   only from M681 event-doc membership.
2. Predict candidate utility against boundary displacement features:
   score gap, current top100 threshold, support overlap, atom sign agreement,
   BM25 rank, native fused rank, and head-risk features.
3. Generate aggregate delta atoms from multiple high-probability candidates
   instead of copying raw atoms from the top selected doc.
4. Keep the M683 floors:
   CUB non-regression, top95 floor, Recall/MAP joint lift, and native shared15
   full-surface promotion gate.
5. Stop if candidate proposal cannot improve holdout Recall/MAP without
   damaging NDCG/MRR/CUB.

This is still the same main line as P1:

- first phase: dense-root unified posting and support preservation;
- second phase: retrieval-constrained expansion only through a safe compiler.

The immediate work should be M685 proposal/delta compiler, not SAE
reconstruction and not another fixed-alpha or query-only gate.
