# M737 Damage-Witness Selector

M737 trains productive and damage classifiers from M736 bundle replay
labels.  The selector is evaluated only on `model_productive_bundle`
rows and uses pre-replay bundle/model features.

## Model Audit

| Split | Rows | Productive + | Damage + | Productive AUC | Damage AUC |
| --- | ---: | ---: | ---: | ---: | ---: |
| `train` | 170 | 3 | 9 | 0.946108 | 0.798482 |
| `holdout` | 29 | 1 | 2 | 0.785714 | 0.722222 |

## Selected Bundle Summary

| Split | Rows | Productive | Damage | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `train` | 16 | 3 | 0 | +0.000000 | +0.000044 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000488 | 1 |
| `holdout` | 5 | 0 | 1 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |

## Thresholds

- Productive threshold: `0.075492`
- Damage threshold: `0.672214`

## Decision

M737 removed damaging rows but did not keep productive holdout bundles.  Need stronger productive teacher features.
