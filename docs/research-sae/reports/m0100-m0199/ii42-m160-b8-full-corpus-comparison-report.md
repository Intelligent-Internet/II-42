# ii42 M160 B8 Full-Corpus Comparison Report

Date: 2026-06-03

## Summary

B8 completed training, full-corpus evaluation, and miss-taxonomy generation.
The result is useful as a corrected Stage-B data-surface experiment, but it is
not a promotion candidate.

Legacy artifact paths still use `bm25sae` names because they were produced
before the ii42 rebrand. This report uses `ii42` for the exploration and
product direction.

Main decision:

- B8 improves Recall@100 over B7 by `+0.0031`, but MRR/NDCG/MAP regress.
- B8 does not beat dense or BM25+dense.
- B8 does not beat the best B-series checkpoint in this eval set.
- B8 is far behind the M150 A1 C6 frontier, which remains the most important
  route to re-check and reuse.

## Artifact Surface

Run:

```text
/home/huoju/leask/runs/bm25sae-m160-stageb-b8-bm25falsepositive-v1
```

Evaluation:

```text
/home/huoju/leask/runs/bm25sae-m160-stageb-b8-bm25falsepositive-v1-full-corpus-eval/m110_full_corpus_index_eval.json
```

Miss taxonomy:

```text
/home/huoju/leask/runs/bm25sae-m160-stageb-b8-bm25falsepositive-v1-full-corpus-eval/bm25sae_miss_taxonomy.json
```

This is the M110 continuity full-corpus surface, not a completed official
full BEIR15 matrix.

## B8 Matrix

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3182 | 0.2972 | 0.2257 | 0.1501 |
| SAE | 0.2938 | 0.2630 | 0.1989 | 0.1361 |
| BM25+SAE score fusion | 0.3090 | 0.2654 | 0.2001 | 0.1335 |
| BM25+SAE RRF | 0.3050 | 0.2634 | 0.1886 | 0.1254 |

Interpretation:

- Standalone SAE is stronger than BM25 on all four metrics.
- Adding BM25 to SAE improves Recall@100 but hurts MAP@100 against standalone
  SAE, so the fusion/ranking side is still not well calibrated.
- BM25+SAE is below dense by `-0.0042` Recall@100, `-0.0338` MRR@20,
  `-0.0250` NDCG@10, and `-0.0169` MAP@100.
- BM25+SAE is below BM25+dense by `-0.0092` Recall@100, `-0.0318` MRR@20,
  `-0.0256` NDCG@10, and `-0.0166` MAP@100.

## Nearby M160 B-Series Comparison

The table compares the BM25+SAE score-fusion row across nearby M160 Stage-B
attempts.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M160A Stage-A continuation | 0.3020 | 0.2739 | 0.2002 | 0.1343 |
| B4 ranking calibration | 0.3110 | 0.2765 | 0.2039 | 0.1366 |
| B5 PPLX no-self C0 | 0.3073 | 0.2575 | 0.1982 | 0.1356 |
| B6 current deep rows | 0.3088 | 0.2658 | 0.2012 | 0.1369 |
| B7 streaming large | 0.3059 | 0.2658 | 0.2005 | 0.1358 |
| B8 BM25 false-positive rows | 0.3090 | 0.2654 | 0.2001 | 0.1335 |

B8 vs B7:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.0031 |
| MRR@20 | -0.0004 |
| NDCG@10 | -0.0004 |
| MAP@100 | -0.0023 |

B8 vs B6:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.0002 |
| MRR@20 | -0.0004 |
| NDCG@10 | -0.0011 |
| MAP@100 | -0.0033 |

Interpretation:

- B8 confirms that BM25 false-positive supervision can slightly improve
  admission/Recall over B7.
- It does not improve ranking quality.
- B4 still dominates the later B-series rows on this evaluator, so continuing
  B8-style data expansion alone is not enough.

## Frontier Comparison

The table compares BM25+SAE score fusion against stronger historical
checkpoints on the same eval artifact format.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| B8 BM25 false-positive rows | 0.3090 | 0.2654 | 0.2001 | 0.1335 |
| M130 Stage C | 0.3388 | 0.3648 | 0.2490 | 0.1583 |
| M150 C0 | 0.3328 | 0.3771 | 0.2542 | 0.1579 |
| M150 C1 doc64/q80 | 0.3279 | 0.3817 | 0.2559 | 0.1608 |
| M150 A1 C6 official dense-miss | 0.3841 | 0.4271 | 0.3010 | 0.2008 |

B8 vs M150 A1 C6:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.0751 |
| MRR@20 | -0.1618 |
| NDCG@10 | -0.1009 |
| MAP@100 | -0.0673 |

Interpretation:

- M150 A1 C6 is currently the strongest parsed frontier row.
- The gap is too large to explain as noise.
- Before pushing more B8-style training, the C6 artifact and training surface
  should be re-verified and treated as the main source of reusable evidence.

## Miss Taxonomy

B8 miss-taxonomy summary:

| Field | Count |
| --- | ---: |
| Queries | 886 |
| Relevant docs | 23,475 |
| Dense relevant hits | 3,449 |
| BM25+dense relevant hits | 3,481 |
| BM25+SAE relevant hits | 3,215 |
| Dense-hit SAE candidate missed | 392 |
| Dense-only candidate missed | 158 |
| Candidate hit but score low | 831 |
| Not retrieved by controls | 18,879 |

Interpretation:

- `dense_hit_sae_candidate_missed=392` shows B8 still has a semantic coverage
  gap.
- `candidate_hit_score_low=831` shows ranking/calibration is at least as
  important as admission.
- The combination explains why Recall improves slightly while MAP drops.

## Decision

B8 should not be promoted.

Recommended next step for ii42:

1. Re-verify M150 A1 C6 artifact/corpus comparability because it dominates
   both dense and the recent M160 B-series on this surface.
2. If C6 is valid, rebuild the current ii42 training branch around the C6-style
   official dense-miss/full-corpus supervision rather than continuing B8 as the
   main route.
3. Preserve B8 as auxiliary evidence for BM25 false-positive handling, but do
   not treat it as the main training objective.
4. Keep any future Stage-B rows strict about dimensionality, BM25 score
   presence, `source_top_k`, and final evaluator geometry before training.
