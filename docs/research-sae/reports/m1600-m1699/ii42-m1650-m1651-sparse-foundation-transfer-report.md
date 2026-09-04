# M1650-M1651 Sparse-Foundation Transfer Report

Date: 2026-07-11

Decision: **retain the frozen OpenSearch learned-sparse root; close this bounded
dense-plus-sparse continuation family and do not run shared3 from either
trained checkpoint.**

## Question

M1640 established that a mature vocabulary-sparse encoder can carry useful
semantic retrieval quality in one posting map. M1650 then asked whether a
frozen BGE dense teacher provides complementary supervision that can improve
that root without losing its sparse access shape. M1651 tested one structural
correction when ordinary continuation improved the teacher fit by expanding
the posting surface.

Both training runs used the same public OpenSearch checkpoint, 4,096 MS MARCO
training rows, 512 held-out rows, one positive plus three hard negatives, 500
steps, seed 1650, and the same frozen teacher surface. Qrels were not used for
training or selection.

## S1: Teacher Observability

The held-out candidate-set audit passed before training:

| Surface | Top-1 accuracy | Pair accuracy |
| --- | ---: | ---: |
| Frozen dense | 0.816406 | 0.907552 |
| Frozen sparse | 0.796875 | 0.900391 |
| Equal normalized ensemble | 0.816406 | 0.910156 |
| Stored cross-encoder | 0.876953 | 0.938802 |

The dense and sparse scores had mean Pearson/Spearman agreement
`0.896102/0.798918`. Dense supervision recovered 26 of 104 sparse top-1
errors, while only seven of those recoveries converted a dense-correct row
into an ensemble error. This is a real complementary teacher signal, not an
assumption.

## S2: Unconstrained Continuation

Ordinary continuation produced a useful but non-promotable frontier:

| Step | KL improvement | Pair accuracy | Anchor rho | Doc nnz | Doc FLOPS | Query nnz |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 1.86% | 0.900391 | 0.992969 | 1.0058x | 1.0175x | 1.0033x |
| 250 | 2.63% | 0.901693 | 0.988962 | 1.0533x | 1.0708x | 1.0311x |
| 500 | 7.48% | 0.904948 | 0.985447 | 1.1366x | 1.1047x | 1.1002x |

Step 500 passed teacher-fit, pair, anchor, and max-DF checks, but failed the
locked sparse-cost gate. Earlier steps stayed within cost but did not reach
the predeclared 5% KL improvement. The model learned useful transfer partly by
opening more posting mass.

## M1651: Root-Relative Constraints

M1651 changed only optimization geometry. A frozen copy of the root supplied
query/document activation-mass and FLOPS constraints; projected dual ascent
replaced an arbitrary fixed sparsity coefficient.

| Step | KL improvement | Pair accuracy | Anchor rho | Doc nnz | Doc FLOPS | Query nnz | Query FLOPS |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 0.52% | 0.900391 | 0.992869 | 0.9661x | 0.9766x | 0.9671x | 0.9900x |
| 250 | 1.09% | 0.901693 | 0.990134 | 0.9784x | 0.9853x | 0.9667x | 0.9843x |
| 500 | 4.06% | 0.906250 | 0.985056 | 0.9939x | 0.9421x | 0.9864x | 0.9800x |

The correction worked as intended: step 500 improved pair accuracy more than
M1650 while remaining below the root on every measured cost. It nevertheless
missed the locked 5% teacher-KL gate. No trained checkpoint passed all gates.

## Interpretation

Three conclusions must remain separate:

1. Dense-to-sparse transfer signal exists and improves candidate ordering.
2. Root-relative constraints control posting cost without a threshold or TopK
   repair.
3. On this bounded 4K-row continuation, the required distribution transfer
   and locked sparse cost did not coexist at the declared gate.

This is not evidence that learned sparse retrieval is exhausted. It closes
small continuation mutations of this already trained root: no dual-step grid,
longer run, fixed sparsity coefficient, or post-hoc pruning is authorized.
The remaining evidence-backed route is a standard, larger sparse-foundation
training recipe, but only after a real learned-sparse engine confirms that the
representation has acceptable exact-index cost.

## Artifacts

- S1 ClearML: `1f1da28de1d240f4a11698f006a5b859`
- M1650 S2 ClearML: `32e6d1823ff34ccea15c4e2ba24b7da1`
- M1651 ClearML: `dcf81593c2c7498789d06219c96d68b7`
- Contracts: `docs/research-sae/reports/m1600-m1699/ii42-m1650-dense-teacher-sparse-foundation-contract.md`,
  `docs/research-sae/reports/m1600-m1699/ii42-m1651-root-relative-cost-constraint-contract.md`
- Scripts: `scripts/audit_m1650_teacher_observability.py`,
  `scripts/train_m1650_sparse_foundation.py`,
  `scripts/train_m1651_constrained_sparse_foundation.py`

No M1650/M1651 process remains on spark-1. No checkpoint was selected or
promoted.
