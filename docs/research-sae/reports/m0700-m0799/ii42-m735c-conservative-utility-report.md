# M735C Conservative Utility

M735C combines safe/productive/danger atom classifiers into a
conservative utility score.  It still does not train a deep compiler.

- Status: `evaluated`

## Selected Atom Summary

| Split | Rows | Safe | Productive | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `full` | 79 | 72 | 2 | +0.000000 | +0.000044 | +0.001241 | +0.000000 | -0.002483 | -0.000127 | -0.000049 |
| `holdout` | 10 | 9 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.000391 |

## Decision

M735C did not select any holdout productive atom.  Stop before tiny bundle smoke or redesign the utility surface.
