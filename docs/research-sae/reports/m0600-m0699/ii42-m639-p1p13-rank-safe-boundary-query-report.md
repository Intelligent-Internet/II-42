# M639 / P1.13 Rank-Safe Boundary Query Report

Status: `not_promoted_phase_transition_mapped`

## Goal

M638 proved that query-side training can move boundary positives across top100,
but the crossing damaged boundary MAP/NDCG. M639 tests whether a rank-safe
objective can keep that Recall crossing while protecting local ordering.

This is still a first-stage generated-posting/query compiler probe:

- no BM25
- no fixed alpha
- no learned gate
- no dataset-specific tuning
- frozen document postings
- train query-side compiler only

## Objective Change

M638 used a blunt boundary loss: boundary positives competed against the full
top100 non-relevant set. M639 replaces that with:

1. edge-only promotion: boundary positives compete with the top100 edge band
2. top-head anchor: preserve frozen M549 top-head score distribution/order
3. positive anchor: keep positives already inside top100 ahead of newly promoted
   boundary positives

The gate was intentionally not relaxed. A trained checkpoint must improve
boundary Recall while keeping boundary MAP non-negative and protecting all-query
NDCG/MRR/dense-overlap.

## Runs

| Run | Seed | Boundary weight | Edge width | Top anchor | Positive anchor | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| A | 6391 | 2.0 | 24 | 0.75 | 0.50 | rank preserved, no Recall crossing |
| B | 6392 | 3.0 | 48 | 0.35 | 0.25 | MAP/NDCG improved, no Recall crossing |
| C | 6381 | 6.0 | 48 | 0.20 | 0.10 | Recall crossing returns, MAP/NDCG loss returns |

Run C uses the same seed/split shape as M638 for direct comparison.

## Dev Trace Summary

| Run | Best boundary dR@100 | Boundary dMAP at crossing | Boundary dNDCG at crossing | Best rank-safe signal |
| --- | ---: | ---: | ---: | --- |
| A | +0.000000 | n/a | n/a | boundary MAP +0.000265, no Recall |
| B | +0.000000 | n/a | n/a | all MAP +0.002166, all NDCG +0.002377, no Recall |
| C | +0.013889 | -0.000377 | -0.005952 | same crossing shape as M638, still not rank-safe |

## Key Evidence

M639-A is too conservative. It preserves rank quality but never changes
Recall@100.

M639-B is a useful negative/side signal. It improves ranking metrics without
moving boundary positives across top100. This says the anchors can act as a
query-side rank calibrator, but they do not solve the candidate-boundary problem.

M639-C recovers the M638 top100 crossing behavior:

| Epoch | Boundary dR@100 | Boundary dMAP | Boundary dNDCG | Boundary dO@100 |
| ---: | ---: | ---: | ---: | ---: |
| 1 | +0.000000 | -0.000541 | -0.000300 | +0.000000 |
| 2 | +0.013889 | -0.000377 | -0.005952 | +0.000000 |
| 3 | +0.011111 | -0.002807 | -0.007610 | -0.010833 |
| 4 | +0.006481 | -0.003038 | -0.014902 | -0.028333 |

This confirms a phase transition:

- strong boundary pressure gives Recall crossing
- rank anchors can preserve/improve ranking when pressure is low
- current combined objective does not produce both at once

## Diagnosis

The M638/M639 blocker is now narrower than before. It is not "no boundary
examples" and not "query compiler cannot affect ranking." The blocker is that
top100 boundary crossing is a discontinuous ranking event. A smooth query-vector
objective either stays too conservative or crosses by perturbing local order.

This suggests further scalar loss blending is unlikely to be the next best use
of time. The next breakthrough needs a more direct top100/listwise mechanism or
a second-stage native reranker over the candidate pool.

## Decision

Do not promote M639.

Do not expand M639 to shared15 or official full matrix.

Keep two reusable findings:

1. M638/M639-C: boundary Recall crossing is possible from query-side updates.
2. M639-B: query-side rank calibration can improve MAP/NDCG without Recall.

## Next Step

M640 should stop scalar blending and test one of two direct mechanisms:

1. top100 replacement surrogate: explicitly train replacement of lowest-scoring
   top100 non-relevant candidates by boundary positives, while preserving head
   positives and top-k dense overlap
2. native second-stage reranker: keep P1 candidate generation frozen and train a
   global candidate-feature scorer over P1/BM25/rank/atom features

The first option stays in first-stage query compiler research. The second option
directly attacks the scorer/reranker bottleneck found in P1.1. Given M639-B's
ranking-only positive signal and M638/C's unstable crossing signal, the next
highest-value probe is a very small M640 top100 replacement surrogate. If it
cannot produce Recall crossing without MAP/NDCG loss, move to the native
reranker path instead of continuing scalar loss tuning.
