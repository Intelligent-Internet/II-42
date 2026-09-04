# M734B Safe-Delta Separability Canary

M734B trains only small global logistic selectors over M734A native
delta rows.  It tests whether hard-safe deltas are predictable before
any deeper compiler or broader replay.

## Model Summary

| Surface | Label | Trained | Train AUC | Holdout AUC | Holdout + | Pred + | TP | Precision | Recall |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `combined` | `hard_safe` | 1 | 0.604897 | 0.551469 | 114 | 108 | 48 | 0.444444 | 0.421053 |
| `combined` | `productive_safe` | 1 | 0.784213 | 0.671756 | 18 | 6 | 0 | 0.000000 | 0.000000 |
| `head_risk_atoms_s0.005` | `hard_safe` | 1 | 0.597415 | 0.600329 | 32 | 7 | 5 | 0.714286 | 0.156250 |
| `head_risk_atoms_s0.005` | `productive_safe` | 1 | 0.732104 | 0.598485 | 4 | 4 | 1 | 0.250000 | 0.250000 |
| `head_risk_atoms_s0.01` | `hard_safe` | 1 | 0.600382 | 0.585608 | 31 | 7 | 4 | 0.571429 | 0.129032 |
| `head_risk_atoms_s0.01` | `productive_safe` | 1 | 0.736645 | 0.537879 | 4 | 5 | 1 | 0.200000 | 0.250000 |
| `source_atoms_s0.005` | `hard_safe` | 1 | 0.623347 | 0.485140 | 26 | 26 | 10 | 0.384615 | 0.384615 |
| `source_atoms_s0.005` | `productive_safe` | 1 | 0.856887 | 0.760000 | 5 | 2 | 0 | 0.000000 | 0.000000 |
| `source_atoms_s0.01` | `hard_safe` | 1 | 0.633636 | 0.467556 | 25 | 27 | 8 | 0.296296 | 0.320000 |
| `source_atoms_s0.01` | `productive_safe` | 1 | 0.889928 | 0.806154 | 5 | 1 | 0 | 0.000000 | 0.000000 |

## Best Deployable Model

```json
null
```

## Decision

M734B did not clear the separability gate.  The best-AUC model was `source_atoms_s0.01`/`productive_safe` with AUC=0.806154 but TP=0; the best-TP model was `combined`/`hard_safe` with TP=48 but AUC=0.551469.  No single deployable surface clears both the AUC and actual holdout selection gates.  Do not start deep compiler training from this teacher/interface surface.
