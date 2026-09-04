# II-42 M611 P1.3 Score Separability Report

Date: 2026-07-06

## Objective

M611 checks whether the remaining P1.3-a010 M604 scorer gap is recoverable by
a global scorer.  It compares candidate-present positives outside top100
against non-relevant documents near the top100 boundary.

This is a diagnostic, not a promoted reranker.

Frozen input:

- Candidate surface: P1.3-a010
- Evidence root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1/`
- M610-A bounded probe:
  `docs/research-sae/reports/m0600-m0699/ii42-m610-p1p3-guarded-admission-report.md`

## Artifacts

- Script:
  `scripts/audit_m611_score_separability.py`
- JSON:
  `runs/m611_p1p3_score_separability_v1/m611_p1p3_score_separability.json`
- Report:
  `runs/m611_p1p3_score_separability_v1/m611_p1p3_score_separability.md`

## Result

Recommendation from the audit:

`return_to_score_calibration_low_separability`

The best same-query feature is only weakly above random:

| Feature | Global AUC | Query-pair AUC | Query macro AUC |
| --- | ---: | ---: | ---: |
| bm25_rr | 0.44132 | 0.51699 | 0.45799 |
| source_count | 0.42814 | 0.44507 | 0.45346 |
| bm25_score | 0.71001 | 0.41923 | 0.43186 |
| p1_score | 0.85716 | 0.00702 | 0.01736 |
| fused_score | 0.10077 | 0.00000 | 0.00000 |

The distinction between global AUC and query-pair AUC is the key evidence.
`p1_score` has high global AUC, but within the same query the under-ranked
positives score below bottom-top100 negatives almost always.  A global score
threshold or simple global reranker cannot fix that; it needs query-local score
geometry to be better calibrated before ranking.

Dataset AUC for the selected feature `bm25_rr` is also unstable:

| Dataset | Query-pair AUC |
| --- | ---: |
| climate-fever | 0.56746 |
| cqadupstack | 0.30108 |
| dbpedia-entity | 0.34812 |
| fiqa | 0.43351 |
| msmarco | 0.46158 |
| nfcorpus | 0.52956 |
| scidocs | 0.37016 |
| scifact | 0.52381 |
| trec-covid | 0.54032 |
| webis-touche2020 | 0.71667 |

This is not a stable global scorer signal.  The best feature helps some rows
but is actively wrong on others.

## Bottom-Slot Oracle Ceiling

The oracle ceiling shows that bottom-slot admission has enough theoretical
room to matter, but M610-A recovered only a tiny fraction of it.

| Preserve top-k | Top100 recall | Candidate recall | Oracle dRecall micro | Oracle dRecall query mean | Extra hits |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 95 | 0.34266 | 0.83101 | 0.01940 | 0.05485 | 771 |
| 98 | 0.34266 | 0.83101 | 0.01152 | 0.04548 | 458 |
| 99 | 0.34266 | 0.83101 | 0.00692 | 0.03251 | 275 |

M610-A best bottom-slot result at preserve top98 recovered only query-average
Recall@100 `+0.000836`, far below the oracle query-mean ceiling `+0.04548`.
The limit is not lack of available positives; it is inability of current
native score/rank features to identify them safely.

## Decision

Do not continue M605/M610-style scorer tuning as the main path.

The current evidence says:

1. P1.3 first-stage dense overlap is now high with aligned reference.
2. Candidate-present positives remain under-ranked.
3. Existing native rank/score features cannot reliably separate those
   positives from bottom-top100 negatives within the same query.
4. Bottom-slot admission is safe only when extremely conservative, and the
   gains are too small.

Therefore the next meaningful work is first-stage score calibration, not
another global reranker.  The likely target is query-local score geometry:
make P1 native scores preserve dense per-query ordering and margins better,
especially around the top100 boundary.

## Recommended Next Step

Start M612: P1.4 query-local score calibration.

Scope:

- Keep BM25/qrels out of first-stage training.
- Use frozen dense teacher rankings/scores.
- Train or transform the P1 score surface to improve same-query pairwise order
  and margin around dense top100/top1000 boundaries.
- Evaluate first on dense-equivalent gates, then native P1.3/P1.4 matrix, then
  M604 scorer-gap audit.

Stop condition:

If query-local dense score calibration cannot improve same-query pairwise order
or native M604 under-ranked-positive rate, stop this scorer-recovery family and
return to the posting compiler/root encoder design.

