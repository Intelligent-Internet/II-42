# M634-B/C / Compiler-Basis Repair Smoke

## Objective

Run deterministic compiler-basis suppression over ranks 21..1000 while
preserving top20. The gate requires Recall@100 gain, no MAP loss, no material
NDCG/MRR loss, dense overlap preservation, bounded KL regression, and gain that
is not isolated to one dataset.

- Status: `stop_compiler_basis_repair_failed`
- Reason: No compiler-basis artifact suppression passed the smoke gate.
- Best family: `negative_low_conflict_artifact`
- Best strength: `0.2`
- Best threshold: `1.0`
- Best shift cap: `0.1`
- Best gate: `dense_overlap50_guard_failed`

## Best Delta

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.000000 |
| MAP@100 | +0.000087 |
| Recall@100 | +0.000408 |
| MRR@20 | +0.000000 |
| dense overlap@20 | +0.000000 |
| dense overlap@50 | -0.001961 |
| dense overlap@100 | -0.001373 |
| dense KL | +0.003171 |

## Top Trials

| Family | Strength | Threshold | Cap | Gate | dRecall | dMAP | dNDCG | dMRR | dOverlap100 | dKL |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `negative_low_conflict_artifact` | 0.200 | 1.000 | 0.100 | `dense_overlap50_guard_failed` | +0.000408 | +0.000087 | +0.000000 | +0.000000 | -0.001373 | +0.003171 |
| `negative_low_conflict_artifact` | 0.200 | 1.000 | 0.050 | `dense_overlap50_guard_failed` | +0.000345 | +0.000030 | +0.000000 | +0.000000 | -0.000784 | +0.001515 |
| `negative_low_conflict_artifact` | 0.100 | 1.000 | 0.050 | `dense_overlap50_guard_failed` | +0.000314 | +0.000008 | +0.000000 | +0.000000 | +0.000000 | +0.001488 |
| `negative_low_conflict_artifact` | 0.100 | 1.000 | 0.100 | `map_guard_failed` | +0.000281 | -0.000066 | +0.000000 | +0.000000 | -0.000980 | +0.002639 |
| `negative_low_conflict_artifact` | 0.100 | 1.000 | 0.200 | `map_guard_failed` | +0.000281 | -0.000083 | +0.000000 | +0.000000 | -0.001765 | +0.003129 |
| `negative_low_conflict_artifact` | 0.200 | 0.500 | 0.100 | `dense_overlap50_guard_failed` | +0.000109 | +0.000019 | +0.000000 | +0.000000 | -0.003137 | +0.002287 |
| `overlap_artifact` | 0.200 | 0.000 | 0.200 | `dataset_support_guard_failed` | +0.000090 | +0.000075 | +0.000000 | +0.000000 | +0.000000 | +0.010855 |
| `overlap_artifact` | 0.200 | 0.500 | 0.100 | `dataset_support_guard_failed` | +0.000090 | +0.000075 | +0.000000 | +0.000000 | +0.000000 | +0.008027 |
| `negative_low_conflict_artifact` | 0.200 | 1.000 | 0.200 | `map_guard_failed` | +0.000015 | -0.000251 | +0.000000 | +0.000000 | -0.004510 | +0.005749 |
| `support_artifact` | 0.020 | 0.000 | 0.050 | `no_recall_signal` | +0.000000 | -0.000013 | +0.000000 | +0.000000 | +0.000588 | +0.004955 |
| `support_artifact` | 0.020 | 0.000 | 0.100 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.005186 |
| `support_artifact` | 0.020 | 0.000 | 0.200 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.005186 |
| `support_artifact` | 0.020 | 0.500 | 0.050 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.005038 |
| `support_artifact` | 0.020 | 0.500 | 0.100 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.005077 |
| `support_artifact` | 0.020 | 0.500 | 0.200 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.005077 |
| `support_artifact` | 0.020 | 1.000 | 0.050 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.004269 |
| `support_artifact` | 0.020 | 1.000 | 0.100 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.004280 |
| `support_artifact` | 0.020 | 1.000 | 0.200 | `no_recall_signal` | +0.000000 | -0.000010 | +0.000000 | +0.000000 | +0.000588 | +0.004280 |
| `support_artifact` | 0.050 | 1.000 | 0.050 | `no_recall_signal` | +0.000000 | -0.000000 | +0.000000 | +0.000000 | +0.000588 | +0.007785 |
| `support_artifact` | 0.050 | 1.000 | 0.100 | `no_recall_signal` | +0.000000 | -0.000013 | +0.000000 | +0.000000 | +0.000588 | +0.010744 |

## Decision

M634 does not pass smoke. The best suppression candidate improves Recall@100 by
only +0.000408 and MAP@100 by +0.000087, but fails dense overlap guards:
dense overlap@50 drops by -0.001961 and dense overlap@100 drops by -0.001373.

The result is weaker than M633's best artifact penalty. It confirms that simple
deterministic basis-family suppression cannot repair P1 while preserving dense
faithfulness. The next step should not be another scorer or another
deterministic suppression grid. The useful branch is to return to a true
unified encoder/output-head training objective that changes how postings are
generated, not how the frozen materialized rows are reordered.
