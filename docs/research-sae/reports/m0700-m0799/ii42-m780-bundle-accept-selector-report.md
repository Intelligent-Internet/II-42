# M780 Bundle Accept Selector

M780 is a bounded selector replay for the M779 stable bundle oracle.
It trains only on strict-positive labels and chooses at most one
bundle per query by model score.

## Test Results

| Model | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| hgb | 1 | 1 | 5.7 | +0.000007 | +0.000033 | +0.000000 | +0.000000 | +0.000000 | +0.000040 | None |
| logistic | 0 | 0 | 7.0 | -0.000119 | +0.000009 | +0.000000 | +0.000000 | +0.000000 | -0.000110 | original:dbpedia-entity,nfcorpus; seed7642:climate-fever,nfcorpus; seed7643:climate-fever,nfcorpus |

## Decision

M780 selector is safe but diagnostic-scale only. Keep M779 as an oracle signal and stop bundle selector replay.
