# M638 / P1.12 Boundary-Positive Query Report

Status: `boundary_crossing_signal_not_promoted`

## Goal

M638 tests whether the P1 query-side compiler can recover positives that
already exist in the frozen M549 candidate pool but sit outside top100.

This is still a first-stage posting/query compiler probe:

- no BM25
- no fixed-alpha fusion
- no learned gate
- no dataset-specific thresholds
- frozen document postings
- train only query-side compiler

## Boundary Audit

| Split | Queries | Boundary queries | Boundary positives |
| --- | ---: | ---: | ---: |
| train | 512 | 65 | 88 |
| dev | 192 | 24 | 34 |
| test | 256 | 29 | 41 |

Boundary positives are qrels positives outside frozen M549 top100 but inside
top1000. The sample is small, but it is not empty. M637 did not fail because
there were zero recoverable positives.

## Dev Epoch Trace

| Epoch | dR@100 boundary | dMAP boundary | dNDCG boundary | dO@100 boundary | Gate |
| ---: | ---: | ---: | ---: | ---: | --- |
| 1 | +0.000000 | -0.000547 | -0.000300 | +0.000000 | fail |
| 2 | +0.013889 | -0.000358 | -0.005952 | -0.000417 | fail |
| 3 | +0.006481 | -0.003518 | -0.008233 | -0.012500 | fail |
| 4 | +0.006481 | -0.003145 | -0.014902 | -0.030833 | fail |

Epoch 2 is the first real top100 boundary-crossing signal: Recall@100 improves
on the boundary slice while dense overlap remains almost unchanged. It is not
promotable because MAP and NDCG decline on the same boundary slice.

Later epochs over-optimize the boundary objective and damage dense geometry.

## Final Test Boundary Macro

Final selected checkpoint is the epoch0 fallback because no trained checkpoint
passes the combined gate.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_root | 0.981897 | 1.000000 | 0.246074 | 0.171318 | 0.488136 | 0.428802 |
| m549_frozen | 0.981897 | 0.997931 | 0.246797 | 0.171793 | 0.488136 | 0.431676 |
| m638_query | 0.981897 | 0.997931 | 0.246797 | 0.171793 | 0.488136 | 0.431676 |

## Final Test All-Query Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_root | 0.978743 | 1.000000 | 0.507880 | 0.409958 | 0.922810 | 0.480565 |
| m549_frozen | 0.978743 | 0.996758 | 0.506913 | 0.408632 | 0.922810 | 0.479576 |
| m638_query | 0.978743 | 0.996758 | 0.506913 | 0.408632 | 0.922810 | 0.479576 |

## Diagnosis

M638 is not a promoted model, but it is not a dead end.

The useful signal is that query-side training can move relevant documents across
the top100 boundary. The blocker is that the current boundary objective does not
control local rank quality: it raises boundary Recall by disturbing ordering,
which immediately shows up as boundary MAP/NDCG loss.

This changes the diagnosis from "query-side compiler cannot recover Recall" to
"query-side compiler needs rank-safe boundary promotion."

## Current Limits

1. Boundary data is sparse. The canary has only 65 train boundary queries across
   FiQA2018 and ArguAna.
2. The boundary loss is too blunt. It rewards crossing without enough protection
   for local positive ordering and hard-negative separation.
3. The selector is correctly conservative. It rejects epoch2 because the Recall
   gain is bought with boundary MAP/NDCG loss.
4. Broad expansion is premature. Shared15 or official full-matrix runs would
   only amplify an unstable objective.

## Next Breakthrough Point

M639 should keep M638's boundary-positive mining, but replace blunt promotion
with rank-safe promotion:

1. Lower or schedule the boundary weight so the model does not overstep after
   the first crossing signal.
2. Add explicit boundary MAP/NDCG preserving loss or local-order pairwise loss.
3. Preserve all-query dense overlap and top-rank quality as hard gates.
4. Save early checkpoints for analysis even when they are not promoted.
5. Only scale after a trained checkpoint improves boundary Recall without
   boundary MAP/NDCG loss.

M639 should stay on the same small surface first. If it cannot keep epoch2-like
Recall gains while protecting MAP/NDCG, this query-side boundary route should be
paused and the next search should move to native candidate-feature reranking.
