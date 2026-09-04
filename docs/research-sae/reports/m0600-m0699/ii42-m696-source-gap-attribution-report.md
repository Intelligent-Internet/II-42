# M696 Source Gap Attribution

## Status

M696 is complete and provides the missing explanation for M695.

M695 showed that qrels-free corpus-positive sources recovered only `0.263393`
of M691 target atoms, while the oracle teacher-positive quota recovered
`0.937500`. M696 decomposes that gap at the row/atom/doc level.

## Setup

- Script: `scripts/audit_m696_source_gap_attribution.py`
- Full summary:
  `runs/m696_source_gap_attribution_v1/m696_summary.json`
- Full generated report:
  `runs/m696_source_gap_attribution_v1/m696_report.md`
- Surface: native shared15 PostgreSQL path.
- Target rows: M691 `rank_safe_recall` rows.
- Primary q-free source: `quota_current_support_48`.
- Oracle source: `quota_oracle_teacher_48`.

## Aggregate

| Item | Value |
| --- | ---: |
| Queries | 29 |
| Target atoms | 448 |
| Q-free visible share | 0.263393 |
| Oracle visible share | 0.937500 |
| Inference-head visible share | 0.214286 |
| Oracle-visible q-free-missing atoms | 303 |
| Gap atom share | 0.676339 |

## Gap Attribution

For the `303` oracle-visible but q-free-missing atoms:

- Source doc in native feature rows: `303 / 303`.
- Source doc already in support source pool: `254 / 303`.
- Support rank `<=48`: `172`.
- Support rank `<=96`: additional `82`.
- Support rank `<=192`: additional `31`.
- Support rank `<=500`: additional `18`.
- Support rank absent: `0`.

This is the key finding. The qrels-free native rows already contain the
teacher-like source documents, and most of them are admitted into the support
source document pool. The missing atoms are mostly lost after document
admission, during atom candidate truncation.

## Interpretation

This strongly argues against immediately increasing training depth. The current
training candidate set is incomplete because the atom admission interface is too
narrow. A deeper compiler trained on that candidate interface would mostly
learn to classify the visible subset, not recover the missing target atoms.

## Decision

Do not deepen compiler training yet.

Do not spend the next iteration on new pseudo-positive document retrieval.

Proceed to an atom admission interface audit: source-specific atom candidate
budgets should be tested before training another generated-posting compiler.

## Next Step

M697 should test whether expanding qrels-free source-specific atom candidate
budgets recovers visibility. A positive M697 would justify training a
source-aware compiler using the expanded atom candidate interface.
