# M785 Interaction Witness Selector

M785 trains a bounded selector on M784 interaction-witness features,
hard-filtered to the cross-zero safety set.

## Test Results

| Model | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| logistic | 0 | 0 | 2.7 | +0.000007 | +0.000080 | +0.000000 | +0.000005 | +0.000000 | +0.000090 | seed7643:nfcorpus |
| hgb | 0 | 0 | 17.3 | -0.000054 | +0.000062 | +0.000000 | -0.000021 | +0.000000 | -0.000003 | original:climate-fever,webis-touche2020; seed7642:nfcorpus; seed7643:dbpedia-entity,fiqa,nfcorpus,trec-covid |

## Decision

M785 found no clean interaction-witness selector. Stop native bundle selector work.
