# II-42 M1937 Cost-Neutral Provenance Swap Report

Date: 2026-07-13

Decision: **close input-token provenance as a standalone support-allocation
teacher. Do not run a smaller swap ratio or train a selector on this rule.**

## Question Answered

M1937 tested one fixed, qrels-free transformation on SciDocs. Within every
document, it added the highest-impact learned-expansion postings from the
canonical `b1->b1.125` tail and removed the same number of lowest-impact
input-aligned `b1` postings. Every document retained its exact semantic nnz,
so total semantic postings remained 2,668,575.

The rule swapped 287,793 postings:

- 10.78% of the `b1` semantic index;
- 99.81% of the available learned-expansion tail;
- against 897,412 available input-aligned `b1` postings.

## Result

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 | Semantic postings | Mean touches |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| b1 | 0.199702 | 0.141874 | **0.466350** | 0.347758 | 0.730383 | 2,668,575 | 98,271 |
| cost-neutral swap | **0.201541** | **0.142851** | 0.464800 | **0.351563** | **0.735483** | 2,668,575 | 102,130 |
| nested b1.125 | 0.200653 | 0.142329 | 0.467550 | 0.349422 | 0.734433 | 3,002,147 | 108,383 |

The swap improved NDCG, MAP, MRR, and CUB by more than the full additive tail
while preserving posting count and keeping touches within the 1.05x gate. It
failed the mandatory Recall gate by `-0.001550`.

## Interpretation

The result isolates a genuine quality tradeoff:

- learned expansions improve head ordering and candidate reach;
- input-aligned semantic postings still recover relevant documents that exact
  BM25 weights do not reproduce;
- provenance alone cannot identify which input-aligned postings are safe to
  remove.

This is why a smaller post-hoc swap ratio is not justified. It could interpolate
the SciDocs Pareto point, but it would not provide a corpus-independent rule or
explain which Recall evidence must survive.

M1937 closes deletion-based provenance allocation. It does not invalidate the
stronger representation idea: lexical and semantic evidence can share one
physical posting key while retaining separate impact channels. Such a
co-keyed multi-channel index would preserve the exact M1934 score and quality,
deduplicate only `(document, canonical term)` storage/traversal, and avoid the
Recall loss caused by deleting semantic evidence. That structural cost oracle
is the next valid experiment before any residual training.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1937-cost-neutral-provenance-swap-contract.md`
- Auditor: `scripts/audit_m1937_cost_neutral_provenance_swap.py`
- Matrix: `runs/m1937_cost_neutral_provenance_swap_v1/scidocs-canary/matrix.json`
- Generated report: `runs/m1937_cost_neutral_provenance_swap_v1/scidocs-canary/report.md`
- Log: `runs/m1937_cost_neutral_provenance_swap_v1/scidocs-canary/run.log`
