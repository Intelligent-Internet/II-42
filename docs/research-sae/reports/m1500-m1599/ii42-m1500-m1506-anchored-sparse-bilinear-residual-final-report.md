# M1500-M1506 Anchored Sparse Bilinear Residual Final Report

Date: 2026-07-09
Final decision: route falsified at text transcodability gate

## Executive Result

The route produced one real structural finding and one decisive stop signal.

- **Structural finding:** the qrel-aware target/harm movement can be expressed
  by a compact sparse bilinear free-factor field. Rank-16/TopK-4 retains
  93.33% of direct oracle utility at low measured posting cost.
- **Stop signal:** the factor field is not predictable from frozen PPLX-1024
  text roots on held-out queries/documents. Both a nonlinear head and a
  cross-validated ridge ceiling recover approximately 0% of retrieval gain.

Therefore the project must not interpret free-factor capacity as evidence for
a deployable text-to-posting encoder.

## Stage Decisions

| Stage | Result | Decision |
| --- | --- | --- |
| M1500 teacher contract | 44,177 pairs, 100% PPLX lineage/ID/action linkage; 2,765 misordered target pairs | pass |
| M1501 factorability | rank-16/TopK-4 retained 93.33% oracle utility; all retrieval macro deltas positive | pass |
| M1502 nonlinear transcodability | mean recovery -0.11%; best epoch 0/2/1 | fail 1 |
| M1502B ridge ceiling | mean recovery -0.46%; all predicted utilities negative | fail 2 |
| M1503 qrels-free teacher | not run | closed by stop rule |
| M1504 joint constrained training | not run | closed by stop rule |
| M1505 hard-row/shared15 | not run | closed by stop rule |
| M1506 native residual namespace | not run | closed by stop rule |

## What Was Learned

### The joint-interaction hypothesis was partly correct

The direct oracle improves Recall@100 by 0.01854 and the rank-16/TopK-4 field
retains almost all of its utility. Query-only bias has zero capacity and a
document-only bias retains only 43.85% of oracle utility. The useful correction
therefore is genuinely query-document dependent.

### Sparse factorability is not text transcodability

The factor field is derived from qrels/action outcomes across only 118
queries. Its sparse basis captures which audited query-document pairs should
move, but that basis is not encoded in the PPLX sentence geometry. Query active
AUC of 0.29-0.50 and factor cosine near zero make this explicit.

### Training depth is not the missing variable

The nonlinear head stops generalizing at epoch 0-2. Closed-form ridge also
fails and often loses to the zero output. More epochs, a wider head, or a new
weighted loss would increase memorization without repairing observability.

### The old selector failure reappears inside the representation

M1225/M1251 established useful action-level oracle movement but weak safe
selection. M1501 embeds that choice into free factors; M1502 shows that the
choice remains unobservable from text. Moving the gate into a latent vector
does not make its supervision generalizable.

## Final Route

Retain P1.3/M549U as the native engineering baseline. Do not add the M150x
residual atom namespace and do not start qrels-free scaling from this teacher.

The free-factor artifacts remain useful only as a diagnostic oracle for future
research. A future route would need a teacher whose target/harm separation is
generated from text/corpus structure before factorization, not inferred from
BEIR action outcomes and then distilled afterward. That is a new hypothesis,
not a continuation of M1500-M1506.

## Artifacts

- `docs/research-sae/reports/m1500-m1599/ii42-m1500-anchored-sparse-bilinear-residual-contract.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1501-sparse-bilinear-factorability-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1502-factor-transcodability-report.md`
- `runs/m1500_sparse_bilinear_teacher_contract_v1/`
- `runs/m1501_sparse_bilinear_factorability_v1/`
- `runs/m1502_factor_transcodability_v1/`
- `runs/m1502b_ridge_factor_transcodability_v1/`
