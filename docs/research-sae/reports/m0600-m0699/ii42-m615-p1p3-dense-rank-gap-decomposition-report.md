# II-42 M615 P1.3 Dense-Rank Gap Decomposition Report

Date: 2026-07-06

## Objective

M615 follows the negative M613/M614 local-packing result.  It asks a narrower
diagnostic question:

Are the remaining P1.3-a010 under-ranked positives actually aligned-dense
top100 documents that the first-stage dense-equivalent surface should recover,
or are they mostly documents that aligned dense itself would not rank in
top100?

This matters because the active goal says M605 should only be restarted when
the first-stage candidate pool is strong but a measurable scorer gap remains.
If the under-ranked positives are mostly not dense-top100, more dense-only
first-stage training is not the right primary lever.

## Inputs

- M604 P1.3 scorer-gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Aligned dense ranking root:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1`
- Diagnostic script:
  `scripts/audit_m615_dense_rank_gap_decomposition.py`
- Output JSON:
  `runs/m615_p1p3_dense_rank_gap_decomposition_v1/m615_p1p3_dense_rank_gap_decomposition.json`
- Output Markdown:
  `runs/m615_p1p3_dense_rank_gap_decomposition_v1/m615_p1p3_dense_rank_gap_decomposition.md`

## Macro Result

| Macro | Positives | Dense hit | P1 hit | BM25 hit | Fused hit | Under-ranked | Under-ranked dense-hit share | Under-ranked dense-miss share | P1 hit lost by fusion | P1 miss rescued by fusion |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Micro | 39742 | 0.341553 | 0.341150 | 0.282396 | 0.342660 | 0.488350 | 0.019322 | 0.980678 | 0.006668 | 0.008178 |
| Dataset macro | 2649.47 | 0.806032 | 0.804706 | 0.734364 | 0.806628 | 0.163571 | 0.055312 | 0.944688 | 0.003255 | 0.005176 |

## Dataset Signals

The strongest evidence is the under-ranked dense-hit share:

| Dataset | Under-ranked rate | Under-ranked dense-hit share |
| --- | ---: | ---: |
| climate-fever | 0.041096 | 0.250000 |
| fiqa | 0.067416 | 0.111111 |
| dbpedia-entity | 0.210816 | 0.060357 |
| cqadupstack | 0.296061 | 0.038627 |
| msmarco | 0.349829 | 0.035540 |
| scidocs | 0.278455 | 0.029197 |
| trec-covid | 0.589187 | 0.016097 |
| nfcorpus | 0.601362 | 0.012195 |
| webis-touche2020 | 0.010730 | 0.000000 |
| scifact | 0.008621 | 0.000000 |

Most under-ranked positives are not in aligned dense top100.

## Interpretation

M608 showed that the P1.3 signed-dot query interface mostly fixed the native
dense-equivalence scoring mismatch.  P1.3-a010 has high aligned overlap while
also improving the native qrels-facing matrix.

M611 then showed that a simple global scorer/reranker is weak: within the same
query, current native features cannot reliably distinguish under-ranked
positives from bottom-top100 negatives.

M612-M614 tested first-stage dense-only score-surface repair families:

- coordinate gain,
- baseline score anchoring,
- local soft packing,
- dense topK recall surrogate.

All failed the dense-only gate or selected a no-op epoch.

M615 now explains why continuing that loop is likely low value:

- Under-ranked positives are overwhelmingly dense-miss, not dense-hit.
- Micro under-ranked dense-hit share is only `0.019322`.
- Dataset-macro under-ranked dense-hit share is only `0.055312`.
- P1 and aligned dense positive top100 hit rates are already very close:
  `0.341150` vs `0.341553` micro.
- Fusion is slightly helpful over P1 alone: `0.342660` vs `0.341150` micro.

This means dense-equivalent first-stage training can still polish small
dense-hit misses, but it cannot solve the dominant remaining gap.  The
dominant under-ranked positives are not examples that a dense-only teacher
would teach us to rank in top100.

## Decision

Stop the current dense-only loss-family exploration loop.

Do not scale M612/M613/M614 artifacts.

Keep P1.3-a010 as the current first-stage/native benchmark candidate.

Do not promote M605 as-is.  M605 remains a rejected benchmark because M611
showed weak same-query separability for the old feature family.

However, the active-goal condition for reopening a second-stage line is now
met in a narrower sense:

- P1.3 candidate/fusion surface is strong enough to keep.
- A scorer gap remains.
- Most remaining under-ranked positives are not dense-top100, so dense-only
teacher preservation is not the right main signal.

The next useful step should be a new second-stage design, not the old M605
feature-grid rerun and not another dense-only output-head perturbation.

## Recommended Next Step

Start a new scoped second-stage experiment after freezing P1.3-a010:

1. Keep P1.3-a010, BM25, and dense references frozen.
2. Do not use dataset-specific thresholds.
3. Do not reuse the old M605 scorer as a promoted component.
4. Build training/evaluation around examples that are candidate-present but
   dense-miss and under-ranked.
5. Add features or supervision that are not reducible to aligned dense top100,
   because dense-only supervision cannot recover the dominant misses.
6. Preserve the native DB/plugin path as the final evaluation route.

If the next scorer line cannot improve Recall@100/MAP@100 without NDCG/MRR
regression, then the remaining route is not scorer tuning; it requires changing
the retrieval objective or adding a new teacher beyond dense-equivalent
preservation.

## Verification

Local verification passed after adding the M615 diagnostic and import-stability
test configuration:

- `python3 -m py_compile` on the modified M603/M605/M606/M608/M611/M615/M551
  Python scripts.
- `bash -n` on the M607/M608/M612/M613/M614 runner scripts.
- `pytest -q` on the relevant native, dense-surface, local-packing, M605, and
  M615 test subset: `42 passed`.
- `git diff --check`.
