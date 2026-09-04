# M1914-M1915 Granite Parent Exploration Report

Date: 2026-07-12

Decision: **freeze M1914 global power as the next Granite parent. The local
fixed-support and fixed-budget search is complete. Do not run M1916 on the
same 10K-row teacher; any further model training must be a separately
contracted, larger public-data dense-to-sparse continuation.**

## Exploration Ladder

| Stage | Degree of freedom | Result | Decision |
| --- | --- | --- | --- |
| M1913 | official Granite checkpoint, 192/50 support | high Recall/CUB, compact exact BMP, weaker head metrics | mature parent |
| M1914 global | query/document power and score scale | heldout, FiQA, and broad3 positive | promote |
| M1914 diagonal | post-TopK per-term scales | early heldout overfit | reject |
| M1914 combined | global power plus term scales | positive but below pure global | reject |
| M1915 query expansion | query 50 to 100 | no heldout gain | stop |
| M1915 document expansion | document 192 to 384 | KL-only gain, no ranking gain | stop |
| M1915 both expansion | both budgets doubled | no additional ranking gain | stop |

The exploration changes one structural degree of freedom at each step and
uses query-disjoint MS MARCO validation before BEIR. It therefore avoids the
older pattern where a more flexible model was justified only by a post-hoc
dataset metric.

## New Frozen Parent

The M1914 output transform is:

```text
q_t = granite_q_t ^ 1.851864 * 0.696368
d_t = granite_d_t ^ 0.562796
```

It keeps Granite's active support exactly unchanged. On complete FiQA exact
BMP:

| Metric | M1913 | M1914 | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.352112 | **0.357677** | +0.005565 |
| MAP@100 | 0.294284 | **0.298860** | +0.004575 |
| Recall@100 | 0.665725 | **0.668468** | +0.002743 |
| MRR@20 | 0.438845 | **0.443793** | +0.004947 |
| index bytes | 156,338,660 | **156,338,660** | 0 |
| BMP p95 | 12.555 ms | **11.551 ms** | -1.004 ms |

On the locked ArguAna/NFCorpus/SciFact transfer, macro NDCG improves
`+0.005679`, MAP `+0.002645`, MRR `+0.010222`, and Recall `+0.000103`.
ArguAna and SciFact expose small single-metric harms, so M1914 is not yet a
full-row product default. It is nevertheless a stronger and cheaper Granite
research parent than M1913.

## What Is Exhausted

The current evidence closes these local routes:

- another global power or score-temperature grid;
- post-TopK per-term weighting;
- query/document support expansion at 2x budget;
- pre-TopK support-scale training on the same M1518 rows;
- using lower KL as authorization when ranking does not improve;
- dataset-specific powers or support thresholds.

M1915 is especially important: dimensions just below the official boundary do
not improve query-disjoint ordering even when included for free. The remaining
gap is not a simple TopK capacity shortage.

## Reasonable Product Scheme

The immediate unified-index candidate is:

```text
text
  -> Granite 30M Sparse
  -> official top50 query / top192 document support
  -> M1914 asymmetric power transform
  -> one exact BMP posting index
```

OpenSearch sparse-v2 remains the balanced-quality product regression baseline.
M1914 may replace M1913 as the compact research baseline after broader row
validation, but it does not yet replace OpenSearch because its FiQA NDCG/MAP/
MRR remain lower and two transfer rows have small head-metric tradeoffs.

## Only Justified Further Training

Further progress requires changing learned semantic impacts, not selecting more
current logits. The next model stage should follow Granite's successful recipe
at a materially larger scale:

1. start from the pinned M1914/Granite checkpoint, never a random output head;
2. use a license-clean public corpus with at least 100K query/candidate sets
   before any long run;
3. distill a strong dense or cross-encoder teacher with candidate competition;
4. preserve parent scores/support through an explicit parent-faithfulness loss;
5. retain total-NORM/FLOPS and hard 192/50 inference budgets;
6. train only the sparse head or a small final-layer LoRA first;
7. select on query-disjoint public heldout data, then test untouched BEIR and
   exact BMP.

This proposed M1916 is not another 10K-row canary. Before training, its data
and teacher must pass a coverage audit showing materially more term/context
exposure than M1518. A small pilot must improve both heldout pairwise and
teacher top1 by at least 0.005 over M1914 while preserving parent support and
positive top1. If it fails, stop Granite adaptation and retain M1914 plus the
OpenSearch product baseline.

## Expected Space

M1914 recovered roughly one third of Granite's FiQA head-metric gap to
OpenSearch without paying cost. The remaining plausible gain is therefore
about another `0.005-0.012` NDCG/MAP/MRR, but it is no longer accessible through
calibration or support budget. Matching OpenSearch head quality while retaining
M1914 Recall and cost is a reasonable strong-success target; materially
exceeding mature sparse and dense systems remains unproven.

## Reports

- `docs/research-sae/reports/m1900-m1999/ii42-m1913-granite-30m-sparse-native-control-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1914-granite-fixed-support-calibration-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1915-granite-fixed-budget-support-audit-report.md`
