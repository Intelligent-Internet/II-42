# M1915 Granite Fixed-Budget Support Audit Contract

Date: 2026-07-12

Status: **complete; see
`docs/research-sae/reports/m1900-m1999/ii42-m1915-granite-fixed-budget-support-audit-report.md`**

## Question

M1914 recovered a reproducible part of Granite's ranking deficit without
changing support. M1915 asks whether dimensions immediately below the released
top-50 query and top-192 document boundaries contain additional observable
teacher signal before any support-changing model is trained.

## M1915A Expansion Observability

Use only the 1,000 query-disjoint M1518 MS MARCO validation rows. Encode the
frozen Granite outputs once at query top100 and document top384, then apply the
selected M1914 powers without any fitting. Compare four surfaces:

1. official query50/document192;
2. expanded query100/document192;
3. official query50/expanded document384;
4. expanded query100/document384.

The expanded surfaces are diagnostics, not deployable candidates. They may
not be evaluated on BEIR or used as a product score.

Support-boundary signal exists only if a one-sided expanded surface improves
positive pairwise or teacher top1 by at least 0.005 absolute over the official
M1914 support, while positive top1 falls by no more than 0.005. A gain visible
only when both sides double their budget does not authorize fixed-budget
training because it does not localize the missing capacity.

## M1915B Conditional Canary

Run only if M1915A passes. Train global pre-TopK term scales on a 2,000-row
train canary and select on the unchanged 1,000-row validation split. Candidate
pools may be expanded internally, but output budgets remain exactly 50/192.

Predeclared branches isolate the responsible side:

- query support scale only;
- document support scale only;
- separate query/document support scales.

Term log-scales remain bounded, are regularized toward zero, and may only alter
which dimensions cross the fixed boundary. The M1914 global powers remain
frozen. A branch must improve the M1914 heldout pairwise metric by at least
0.005 and teacher top1 by at least 0.005 without reducing positive top1.

Only then may M1915 scale from 2,000 to all 10,000 train rows and proceed to
complete FiQA/native evaluation. BEIR may not choose a branch or checkpoint.

## Stop Rules

- Stop if expanded one-sided support has no heldout ranking signal.
- Stop if only the both-expanded surface improves.
- Stop if a pre-TopK scale changes support but cannot transfer its train gain to
  query-disjoint validation.
- Do not use oracle query-document interactions to choose support; document
  support must remain static and query support must be generated from query
  text alone.
- Do not add BM25, corpus DF, qrels, a reranker, or another index.

## Required Artifacts

- M1915A expanded-support summary and report;
- M1915B cache/training artifacts only if M1915A passes;
- explicit authorization or stop decision before M1916.
