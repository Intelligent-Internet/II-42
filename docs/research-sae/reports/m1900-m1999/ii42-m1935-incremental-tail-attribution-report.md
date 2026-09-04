# II-42 M1935 Incremental-Tail Attribution Report

Date: 2026-07-13

Decision: **close DF-only incremental-tail selection. Do not run a finer DF
grid, seven-row LODO, or a selector trained to imitate this policy.**

## Question Answered

M1934 established that increasing the frozen M1914 semantic posting budget
from `1.0x` to `1.125x` lexical postings improves all primary unseen macro
metrics. M1935 tested whether corpus document frequency alone can isolate a
cheaper subset of that added support.

Every `b1` posting was frozen. The candidate was rebuilt as a canonical nested
expansion so that no baseline posting could be replaced. Only postings added
after `b1` were filtered. The predeclared cumulative routes retained added
atoms with document-frequency ratio at most `0.01`, `0.10`, `0.50`, or `1.0`.
No qrels selected a route or cutoff.

## Canonical Nested Endpoint

The independently pruned M1934 reference was already nested on SciDocs and
TREC-COVID. On Quora it replaced 264 of 5,698,832 `b1` postings while adding
712,618 others. M1935 instead preserved all baseline postings and added
712,354 postings to reach the same exact target count.

The nested endpoint retained the M1934 signal:

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| b1 | 0.596009 | 0.358258 | 0.538584 | 0.701456 | 0.766375 |
| M1934 independent b1.125 | 0.599866 | 0.360373 | 0.539188 | 0.706068 | 0.768689 |
| M1935 nested b1.125 | **0.600053** | **0.360607** | **0.539394** | **0.706295** | 0.768633 |

This removes support replacement as the explanation for M1934. The gain is a
real capacity effect. The tiny nested-versus-reference CUB change (`-0.000056`)
does not alter that conclusion.

## DF Attribution

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 | Added posting fraction | Added touch fraction |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| DF <= 0.01 | 0.597381 | 0.359225 | 0.538722 | 0.702058 | 0.766587 | 38.83% | 8.06% |
| DF <= 0.10 | 0.598871 | 0.360299 | 0.539361 | 0.702976 | 0.768094 | 88.98% | 49.62% |
| DF <= 0.50 | 0.600053 | 0.360598 | 0.539365 | 0.706295 | 0.768706 | 99.81% | 95.41% |
| nested b1.125 | 0.600053 | 0.360607 | 0.539394 | 0.706295 | 0.768633 | 100.00% | 100.00% |

Relative to the complete nested gain:

| Route | NDCG retention | MAP retention | Recall retention | MRR retention | CUB retention | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| DF <= 0.01 | 33.94% | 41.15% | 17.03% | 12.43% | 9.41% | fail |
| DF <= 0.10 | 70.78% | 86.88% | 95.97% | 31.41% | 76.15% | fail |
| DF <= 0.50 | 100.00% | 99.58% | 96.46% | 100.00% | 103.26% | fail |

All three diagnostic routes passed every row-level safety floor. They failed
because no route both retained the declared quality signal and reduced added
postings and touches to at most 75% of the full expansion.

## Interpretation

The useful tail is not concentrated in a small set of low-DF atoms.
`DF<=0.01` is inexpensive but loses most of the gain. `DF<=0.10` preserves
most Recall, CUB, and MAP gain at half the added touches, but it still requires
89% of the added postings and retains only 31% of the MRR gain. Almost all
head-ranking gain returns only when medium-frequency atoms are restored.

This result also explains why M1932's stronger global IDF/DF penalties reduced
cost but harmed retrieval. High frequency is not equivalent to redundancy in
the combined lexical-semantic index.

M1935 therefore closes:

- another cumulative DF cutoff grid;
- a static DF-only tail selector;
- a learned selector whose target is the failed DF policy;
- seven-row LODO for any of these exact routes.

The fixed `b1.125` policy remains a validated quality/cost operating point,
not the default low-cost publisher. The next admissible observability test
must use a different structural variable. The most direct unresolved variable
from the M1930 contract is whether an added semantic posting is input-token
aligned or a learned expansion. That qrels-free provenance can test lexical
redundancy without treating document frequency as semantic utility.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1935-incremental-tail-attribution-contract.md`
- Auditor: `scripts/audit_m1935_incremental_tail_attribution.py`
- Matrix: `runs/m1935_incremental_tail_attribution_v1/full/matrix.json`
- Generated report: `runs/m1935_incremental_tail_attribution_v1/full/report.md`
- Log: `runs/m1935_incremental_tail_attribution_v1/full/run.log`
