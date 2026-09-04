# II-42 M604 P1.3 Native Scorer Gap Audit

Date: 2026-07-06

## Objective

This audit repeats M604 on the P1.3 signed-dot query score interface.  The
question is whether the native score-interface improvement also reduces the
remaining scorer gap, especially relevant documents that are present in the
candidate pool but under-ranked outside top100.

Frozen candidate:

- First-stage surface: P1.3 active512 signed-dot query atoms
- Fusion row: P1.3-a010
- BM25 weight: `0.10`
- Semantic weight: `0.90`
- Dataset surface: local shared15
- Base schema: `ii42_shared15`
- P1.3 atom schema: `ii42_p1p3`

## Artifacts

- Audit root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1/`
- Native benchmark matrix:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_shared15_matrix.json`
- Aligned-reference native benchmark matrix:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.json`
- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1/`

Each dataset has:

- `<dataset>_m604_p1_scorer_gap.json`
- `<dataset>_m604_p1_scorer_gap.md`
- `<dataset>_m604_p1_scorer_gap.jsonl`

## Native Benchmark Context

P1.3-a010 improves the native qrels-facing matrix over P1.2-a010.  Its
aligned O@100 is generated from the same P1.3 atom/root embeddings and
supersedes the earlier old-reference O@100 value `0.84244`.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.2-a010 | 0.94180 | needs aligned reference | 0.77748 | 0.69676 | 0.85327 | 0.86968 |
| P1.3-a010 | 0.94166 | 0.92843 | 0.77881 | 0.70001 | 0.85426 | 0.87182 |

Delta P1.3-a010 minus P1.2-a010:

| Metric | Delta |
| --- | ---: |
| CUB | -0.00014 |
| NDCG@10 | +0.00133 |
| MAP@100 | +0.00325 |
| R@100 | +0.00099 |
| MRR@20 | +0.00214 |

O@100 is intentionally omitted from the delta because P1.2 has not been
regenerated with an aligned dense-ranking root in this report.

## Gap Categories

Weighted by qrels positives:

| Category | P1.2-a0125 | P1.3-a010 | Delta |
| --- | ---: | ---: | ---: |
| top100 hit | 0.34188 | 0.34266 | +0.00078 |
| candidate present but under-ranked | 0.48724 | 0.48835 | +0.00111 |
| candidate miss | 0.17088 | 0.16899 | -0.00189 |

Macro over datasets:

| Category | P1.2-a0125 | P1.3-a010 | Delta |
| --- | ---: | ---: | ---: |
| top100 hit | 0.80592 | 0.80663 | +0.00071 |
| candidate present but under-ranked | 0.16369 | 0.16357 | -0.00012 |
| candidate miss | 0.03039 | 0.02980 | -0.00059 |

## Dataset Breakdown

| Dataset | P1.2 top100 | P1.3 top100 | dTop100 | P1.2 under | P1.3 under | dUnder | P1.2 miss | P1.3 miss | dMiss |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 1.00000 | 1.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| climate-fever | 0.95890 | 0.95890 | +0.00000 | 0.04110 | 0.04110 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| cqadupstack | 0.69504 | 0.70394 | +0.00889 | 0.30496 | 0.29606 | -0.00889 | 0.00000 | 0.00000 | +0.00000 |
| dbpedia-entity | 0.78687 | 0.78658 | -0.00029 | 0.20995 | 0.21082 | +0.00087 | 0.00318 | 0.00260 | -0.00058 |
| fever | 1.00000 | 1.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| fiqa | 0.93258 | 0.93258 | +0.00000 | 0.06742 | 0.06742 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| hotpotqa | 1.00000 | 1.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| msmarco | 0.64237 | 0.64286 | +0.00049 | 0.34983 | 0.34983 | +0.00000 | 0.00780 | 0.00731 | -0.00049 |
| nfcorpus | 0.24227 | 0.24411 | +0.00183 | 0.59979 | 0.60136 | +0.00157 | 0.15794 | 0.15453 | -0.00340 |
| nq | 1.00000 | 1.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| quora | 1.00000 | 1.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| scidocs | 0.68699 | 0.68496 | -0.00203 | 0.27439 | 0.27846 | +0.00407 | 0.03862 | 0.03659 | -0.00203 |
| scifact | 0.99138 | 0.99138 | +0.00000 | 0.00862 | 0.00862 | +0.00000 | 0.00000 | 0.00000 | +0.00000 |
| trec-covid | 0.16419 | 0.16484 | +0.00065 | 0.58752 | 0.58919 | +0.00166 | 0.24829 | 0.24598 | -0.00231 |
| webis-touche2020 | 0.98820 | 0.98927 | +0.00107 | 0.01180 | 0.01073 | -0.00107 | 0.00000 | 0.00000 | +0.00000 |

## Interpretation

P1.3-a010 improves the native benchmark and reduces candidate misses slightly,
but it does not materially shrink the scorer gap.  The later aligned-reference
check confirms that P1.3's first-stage dense overlap is high, so the remaining
M604 gap should be treated as a top100 promotion/scoring problem, not as an
overlap-reference blocker.  In the weighted view, under-ranked positives
remain the dominant category and are slightly higher than P1.2-a0125.  The
macro view is essentially flat.

This separates two facts:

1. P1.3 is a better first-stage native score interface than P1.2.
2. P1.3 does not solve the remaining top100 promotion problem.

The remaining bottleneck is still scorer/ranking recovery on heavy-positive
rows such as `trec-covid`, `nfcorpus`, `msmarco`, and `dbpedia-entity`.
However, the previous M605.2 feature family already failed on P1.2 and should
not simply be rerun as a larger grid.

## Decision

Keep P1.3-a010 as the current first-stage candidate.

Do not claim the bottleneck is solved.  The next attempt should be a new
second-stage scorer design or a more targeted first-stage score-calibration
probe.  It should not be another unconstrained M605.2 feature-grid expansion.

Recommended next step:

1. Freeze P1.3-a010 as the benchmark candidate.
2. Use P1.3 M604 JSONL exports as evidence only if the scorer design is
   materially different from M605.2.
3. Require a guard that preserves saturated rows and P1.3 native metrics.
4. Treat M610-A guarded admission as a weak-positive, non-promoted probe:
   bottom-slot admission is safe but too small to solve the gap.
5. Continue with score-separability/calibration diagnostics before any larger
   reranker search.

## M611 Follow-Up

M611 consumed the P1.3 M604 JSONL exports and tested whether the under-ranked
positives are separable from bottom-top100 negatives by existing native score
and rank features.

Artifact:

`docs/research-sae/reports/m0600-m0699/ii42-m611-p1p3-score-separability-report.md`

The diagnostic recommendation is:

`return_to_score_calibration_low_separability`

The strongest evidence is that `p1_score` is globally separable but
query-locally wrong for this exact failure mode:

| Feature | Global AUC | Query-pair AUC | Query macro AUC |
| --- | ---: | ---: | ---: |
| p1_score | 0.85716 | 0.00702 | 0.01736 |
| bm25_score | 0.71001 | 0.41923 | 0.43186 |
| bm25_rr | 0.44132 | 0.51699 | 0.45799 |

The scorer gap therefore should not be treated as a simple missing global
feature or alpha problem.  The top100 promotion failure is mainly caused by
same-query score geometry: relevant documents already in the candidate pool do
not receive a native score surface that orders them above bottom-top100
negatives.

Updated decision:

1. Keep P1.3-a010 as the frozen benchmark.
2. Do not rerun M605/M610 as larger global scorer searches.
3. Start M612 P1.4 query-local dense score calibration.
4. After each M612 candidate, rerun M604 and M611.  Promotion requires the
   under-ranked-positive rate to shrink and same-query separability to improve,
   before any BM25-aware second-stage work is considered.
