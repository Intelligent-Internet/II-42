# II-42 M1936 Semantic-Tail Provenance Report

Date: 2026-07-13

Decision: **close direct input-provenance tail filtering. Do not expand the
two partial routes or train a classifier to imitate them.**

## Question Answered

M1936 partitioned only the canonical SciDocs `b1` to `b1.125` additions by a
publisher-visible property: whether the semantic vocabulary dimension was
present in the document's original Granite tokenizer input. All `b1`
postings, M1931 query calibration, exact lexical postings, and scoring
remained frozen.

The split was strongly asymmetric:

| Source | Added postings | Fraction |
| --- | ---: | ---: |
| input-aligned | 45,238 | 13.56% |
| learned expansion | 288,334 | 86.44% |
| complete tail | 333,572 | 100.00% |

## Canary Result

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 | Added posting fraction | Added touch fraction |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| b1 | 0.199702 | 0.141874 | 0.466350 | 0.347758 | 0.730383 | 0.00% | 0.00% |
| input-aligned tail | 0.200123 | 0.142143 | 0.465950 | 0.349364 | 0.730733 | 13.56% | 7.94% |
| learned-expansion tail | 0.200595 | 0.142214 | 0.467100 | 0.349181 | 0.734433 | 86.44% | 92.06% |
| nested b1.125 | 0.200653 | 0.142329 | 0.467550 | 0.349422 | 0.734433 | 100.00% | 100.00% |

Input-aligned additions are inexpensive and retain most of the full MRR gain,
but they reduce Recall and retain only 8.6% of CUB gain. Learned expansions
retain 93.8% of NDCG, 74.6% of MAP, 85.6% of MRR, 62.5% of Recall, and all CUB
gain, but they consume 86.4% of added postings and 92.1% of added touches.

Neither source passed the predeclared quality-retention and 75% cost gates.
The experiment therefore stopped before Quora/TREC-COVID.

## Interpretation

The source-provenance split is mechanistically informative but not a useful
tail filter:

- input-aligned additions mainly refine head ordering;
- learned expansions carry candidate reach and most overall quality;
- the useful expansion signal is broad rather than concentrated in a cheap
  provenance class.

This supports the M1930 residual premise while rejecting its simplest
publisher implementation. Exact lexical evidence does not make every
input-aligned semantic impact disposable, and merely retaining all learned
expansions does not solve cost.

The one remaining deterministic provenance diagnostic is cost-neutral
replacement: preserve the exact `b1` posting count, add learned-expansion tail
postings, and remove the same number of lowest-impact input-aligned `b1`
postings within each document. This directly tests whether lexical coverage
can fund semantic expansion without increasing index size. It must be a
single fixed rule, not a swap-ratio grid. Failure closes source-token
provenance as a standalone allocation teacher and leaves context-trained
residual generation as a separate, higher-risk route.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1936-semantic-tail-provenance-contract.md`
- Auditor: `scripts/audit_m1936_semantic_tail_provenance.py`
- Matrix: `runs/m1936_semantic_tail_provenance_v1/scidocs-canary/matrix.json`
- Generated report: `runs/m1936_semantic_tail_provenance_v1/scidocs-canary/report.md`
- Log: `runs/m1936_semantic_tail_provenance_v1/scidocs-canary/run.log`
