# M668 Strict-Overlap Boundary Reranker

Status: `accepted`

M668 tightens the M667 second-stage boundary-reranker smoke by requiring a
top100 overlap floor of `0.95`.  The candidate set is still fixed to the M658
top1000 pool, so this experiment measures whether a global scorer can promote
already-present positives into top100 without changing candidate generation.

## Result

Selected grid:

- alpha: `1.0`
- preserve_top_k: `95`
- rerank_max_rank: `200`
- top100 overlap floor: `0.95`

Test deltas versus M658 baseline:

| Metric | Delta |
| --- | ---: |
| Candidate upper bound | `+0.000000` |
| Recall@100 | `+0.003259` |
| MAP@100 | `+0.000023` |
| NDCG@10 | `+0.000000` |
| MRR@20 | `+0.000000` |
| Dense overlap@100 | `-0.045533` |

Test matrix:

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m658_baseline` | 0.899800 | 0.761723 | 0.331258 | 0.427089 | 0.446442 | 1.000000 |
| `m668_reranker` | 0.899800 | 0.764982 | 0.331281 | 0.427089 | 0.446442 | 0.954467 |
| `oracle_candidate_set` | 0.899800 | 0.891704 | 0.891704 | 0.932420 | 0.994924 | 0.986345 |

Per-task signal:

| Task | Recall delta | MAP delta | NDCG delta | MRR delta |
| --- | ---: | ---: | ---: | ---: |
| ArguAna | `+0.000000` | `+0.000000` | `+0.000000` | `+0.000000` |
| FiQA2018 | `+0.002344` | `-0.000042` | `+0.000000` | `+0.000000` |
| SCIDOCS | `+0.007812` | `+0.000177` | `+0.000000` | `+0.000000` |
| TRECCOVID | `-0.001605` | `-0.000793` | `+0.000000` | `+0.000000` |

## Interpretation

This is a real positive signal for the second-stage scorer.  The same candidate
upper bound is preserved, while Recall@100 improves by `+0.003259` under a
strict top100 overlap floor.  That means the model is using boundary score
geometry to rescue positives already present below top100.

This is not evidence that the first-stage encoder/posting generator improved.
Candidate upper bound is unchanged, and dense overlap@100 drops by about
`0.0455`.  M668 therefore belongs to the scorer/reranker track, not the
generated-posting objective track.

## Current Limits And Blocks

- Surface is small: only `FiQA2018`, `SCIDOCS`, `TRECCOVID`, and `ArguAna` on
  the 1024 shared root.  It is not a full BEIR15/MTEB/native result.
- The run used remote Docker CPU because CUDA initialization still fails in
  this environment.  Runtime is acceptable for this smoke, but not for broad
  scaling.
- The scorer uses M658/M661 score geometry.  It has not yet been moved into the
  native P1/BM25 engineering path.
- The gain is below a final promotion threshold.  It is enough to justify a
  broader scorer experiment, but not enough to replace a frozen baseline.
- TRECCOVID regresses slightly in Recall/MAP.  A broader gate must include
  per-dataset regression checks instead of relying only on macro movement.

## Next Breakthrough Point

The best next step is not more fixed-alpha tuning and not another boundary-loss
micro-adjustment.  The useful direction is to port this constrained scorer idea
to the native P1 candidate pool:

1. Build a native M669 audit over P1-a0125 candidates and BM25/dense/P1 signals.
   Split errors into candidate miss, present-but-under-ranked, and BM25-rescue.
2. Train one global constrained reranker over native features only: P1 score,
   BM25 score, dense/vector score if available, ranks, reciprocal ranks,
   score z-scores, atom overlap, idf-weighted atom hits, and term coverage.
3. Keep candidate upper bound fixed.  Gate on Recall@100 improvement, MAP
   non-regression, NDCG/MRR guard, per-dataset regression guard, and top100
   overlap floor.
4. Validate in stages: 4-row smoke, shared15, then native DB/plugin path.  If
   gains disappear in the native path, stop this scorer route and return to
   first-stage output-head/generated-posting training.

## Artifacts

- JSON: `runs/ii42-m668-strict-overlap-reranker-v1/m668_m661_strict_overlap_seed6681/m668_m661_strict_overlap_seed6681.json`
- Model JSON: `runs/ii42-m668-strict-overlap-reranker-v1/m668_m661_strict_overlap_seed6681/m668_m661_strict_overlap_seed6681_model.json`
