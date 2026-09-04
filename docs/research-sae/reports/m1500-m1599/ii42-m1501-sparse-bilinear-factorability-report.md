# M1501 Sparse Bilinear Factorability Report

Date: 2026-07-09
Decision: proceed to M1502 transcodability gate

## Result

M1501 tested the lineage-safe M1500 pair teacher without training a text
encoder. The audit used 118 queries, 7,443 teacher documents, and complete
local corpus tables for six datasets. All scores were replayed against the
frozen M549U/PPLX-1024 native atom scorer.

The direct query-document penalty field has effective rank 39 at 90% energy
and rank 45 at 95% energy. It is not extremely low rank, but a bounded sparse
factorization retains most of its retrieval utility.

| Method | Rank | TopK | dRecall | dMAP | dNDCG | dMRR | dCUB | Utility | Touch ratio | Touched docs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Direct field oracle | - | - | +0.018540 | +0.159389 | +0.171675 | +0.162544 | +0.008319 | +1.247621 | 0.0010 | 0.0102 |
| Query-only bias | - | - | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.0000 | 0.0000 |
| Document-only bias | - | - | +0.000280 | +0.082531 | +0.066679 | +0.082149 | +0.000398 | +0.547047 | 0.0284 | 0.2845 |
| Sparse bilinear | 8 | 4 | +0.001783 | +0.127489 | +0.099832 | +0.126830 | +0.000242 | +0.844948 | 0.0596 | 0.1500 |
| **Sparse bilinear** | **16** | **4** | **+0.018031** | **+0.149308** | **+0.146029** | **+0.162544** | **+0.009228** | **+1.164455** | **0.0289** | **0.1446** |
| Sparse bilinear | 32 | 4 | +0.019095 | +0.154467 | +0.157579 | +0.159339 | +0.009445 | +1.202158 | 0.0203 | 0.1164 |

The selected rank-16/TopK-4 configuration retains 93.33% of direct-field
oracle utility. It passes all macro metric and cost gates. The one-sided
document control retains only 43.85%, so the capacity is not explained by a
query bias or global document-quality memorizer.

## Important Boundary

This is a qrels-aware free-factor capacity result, not a deployable model.
Dense overlap at 100 falls by 0.0363 for the selected configuration. M1501 did
not require the final overlap floor; M1502 and later gates must determine
whether a text-predictable residual preserves useful gain without reproducing
this oracle-only movement.

The initially generated sparse matrix was invalid because a large-array
temporary reduction returned an inconsistent scale under the local
Python 3.14/Accelerate stack. The implementation now uses explicit Frobenius
`einsum`, runs the audit with single-threaded Accelerate, and asserts exact
dense/sparse parity when `TopK == rank`. The invalid stop result was discarded
before interpretation.

## Next Gate

M1502 freezes the selected factors and trains small query/document heads from
the canonical 1024-dimensional PPLX root. It must use held-out queries,
held-out documents, and three seeds. It proceeds only if text heads recover at
least 70% of the free-factor gain and a trained non-epoch0 checkpoint is
selected.

Artifacts:

- `scripts/audit_m1501_sparse_bilinear_factorability.py`
- `runs/m1501_sparse_bilinear_factorability_v1/m1501_factorability.json`
- `runs/m1501_sparse_bilinear_factorability_v1/m1501_selected_factors.npz`
- `tests/test_audit_m1501_sparse_bilinear_factorability.py`
