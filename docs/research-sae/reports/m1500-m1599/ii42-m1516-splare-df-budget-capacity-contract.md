# M1516 SPLARE DF-Budget Capacity Contract

## Question

M1510 proves that an independently retrieval-trained SPLARE reproduction has
real sparse quality, but its pooled postings touch every document. M1511 shows
that removing a few universal atoms is insufficient. M1513 shows that a
generic frozen-BERT grouped projector learns its objective but remains far
below the P1 quality/cost frontier.

M1516 asks a narrower question before any new training: can the existing
retrieval-trained SPLARE signal survive a strict, qrels-free posting budget?

This is a capacity audit, not a promoted model. It uses only corpus document
frequency, learned sparse weights, and fixed global budgets for source
construction. Qrels are visible only to final evaluation.

## Fixed configurations

Every dataset uses the same predeclared configurations. DF is computed from
the complete stored document artifact. A DF cap is applied first; each row
then keeps its highest-weight allowed dimensions. IDF, when enabled, is
applied once to the query-document dot-product contribution.

| Name | Document K | Query K | Max DF ratio | IDF power |
| --- | ---: | ---: | ---: | ---: |
| baseline_d400_q40 | 400 | 40 | 1.00 | 0 |
| budget_d128_q32 | 128 | 32 | 1.00 | 0 |
| budget_d64_q16 | 64 | 16 | 1.00 | 0 |
| budget_d32_q8 | 32 | 8 | 1.00 | 0 |
| idf_d128_q32 | 128 | 32 | 1.00 | 1 |
| df20_d128_q32 | 128 | 32 | 0.20 | 0 |
| df20_idf_d128_q32 | 128 | 32 | 0.20 | 1 |
| df10_idf_d64_q16 | 64 | 16 | 0.10 | 1 |
| df05_idf_d64_q16 | 64 | 16 | 0.05 | 1 |
| df05_idf_d32_q8 | 32 | 8 | 0.05 | 1 |

No per-dataset threshold, alpha, fallback, or configuration selection is
allowed.

## Surfaces

- Complete official NFCorpus, SciFact, and FiQA artifacts already encoded by
  M1510.
- Exact reproduction of the M1510 baseline is required before interpretation.
- Metrics: NDCG@10, MAP@100, Recall@100, MRR@20, candidate upper bound,
  all-touched upper bound, posting count, max DF, touched postings, touched
  documents, and empty-query rate.

## Gate

A single global configuration may authorize one bounded M1517 training
canary only when:

- at least two of three datasets retain at least 90% of every M1510 baseline
  quality metric;
- candidate upper bound retains at least 90% on those rows;
- mean touched-document ratio is at most 0.50 on those rows;
- empty-query rate is at most 1%;
- no dataset falls below 75% of baseline on any quality metric.

Three passing datasets constitute a strong pass. Two constitute only a
bounded-canary pass. Fewer than two stop this source-construction branch.

If no configuration passes, do not train a projector to imitate a capacity
shape that does not exist. The next hypothesis must change the retrieval
backbone or make corpus selectivity part of pretraining, rather than add a
post-hoc gate.
