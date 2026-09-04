# M1915 Granite Fixed-Budget Support Audit Report

Date: 2026-07-12

Decision: **stop support reallocation. Preserve the M1914 global-power
milestone and do not train a pre-TopK selector or per-term support scale.**

## Question Answered

M1914 improved Granite ranking without changing any posting. M1915A tested
whether dimensions immediately below the released top-50 query and top-192
document boundaries contained additional teacher ordering signal. It encoded
the complete query-disjoint M1518 validation split at top100/top384, applied
the frozen M1914 power transform, and changed no parameter.

## Heldout Expansion Audit

| Surface | Query K | Document K | Pairwise | Teacher top1 | Positive top1 | KL |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| official M1914 | 50 | 192 | **0.876375** | **0.653000** | **0.611000** | 0.605511 |
| query expanded | 100 | 192 | 0.876125 | 0.652000 | 0.610000 | 0.606641 |
| document expanded | 50 | 384 | 0.877125 | 0.651000 | 0.610000 | **0.597550** |
| both expanded | 100 | 384 | 0.877000 | 0.651000 | 0.610000 | 0.598928 |

Neither one-sided expansion meets the predeclared 0.005 ranking-gain gate.
Doubling query support slightly harms every metric. Doubling document support
improves KL but changes pairwise by only `+0.000750`, while teacher and positive
top1 both fall. Expanding both sides does not recover the loss.

This is the same failure pattern already observed in earlier project routes:
more candidate capacity can fit a teacher distribution without producing
better positive ordering. The current Granite boundary is not the measurable
bottleneck on this heldout surface.

## Consequence

M1915B was conditional on a positive M1915A signal and is not run. In
particular, the project will not:

- learn global pre-TopK term scales from this 10K-row teacher set;
- double query or document budgets as a rescue;
- use candidate-aware oracle support selection;
- evaluate expanded support on BEIR after a failed heldout gate.

The remaining M1913/M1914 head-ranking gap cannot be assigned to dimensions
just below the fixed TopK boundary. Recovering more would require changing the
representation learned by the backbone/output head through a substantially
larger, paper-native distillation run. That is a new training stage, not a
small support correction, and should be separately contracted.

## Current Survivor

M1914 global power remains the only promoted local modification:

- fixed query/document support and nnz;
- exact BMP index size `156,338,660` bytes;
- FiQA BMP NDCG `0.357677`, MAP `0.298860`, Recall `0.668468`, MRR `0.443793`;
- FiQA p95 `11.551 ms`;
- broad3 macro NDCG/MAP/MRR gains with flat-positive Recall.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1915-granite-fixed-budget-support-audit-contract.md`
- Audit: `scripts/audit_m1915_granite_support_boundary.py`
- Runner: `scripts/run_m1915_granite_support_boundary_audit_spark.sh`
- Remote summary:
  `ii42-m1915-granite-support-boundary-v1/audit/summary.json`
