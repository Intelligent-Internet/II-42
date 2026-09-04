# M757 Query-Listwise Constrained Policy

M757 predicts per-attempt metric deltas and selects query-wise under
predicted non-regression constraints.

## Test Results

| Model | Margin | Queries | Applied | Accepted | Best | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `ridge_margin_0` | 0 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `ridge_margin_1e-06` | 1e-06 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `ridge_margin_1e-05` | 1e-05 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `ridge_margin_5e-05` | 5e-05 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `hgb_margin_0` | 0 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `hgb_margin_1e-06` | 1e-06 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `hgb_margin_1e-05` | 1e-05 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `hgb_margin_5e-05` | 5e-05 | 271 | 0 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |

## Decision

M757 did not recover a strict-gate learned selector from the qrels-free global proposal pool.
