# M787 Interaction Delta Surface

M787 trains an interaction-witness scorer but uses it to generate a
continuous query-level delta from multiple cross-zero bundles.

## Test Results

| Model | Gate | Clean | Applied | TopN | Scale | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| hgb | 0 | 0 | 18.0 | 4 | 0.50 | -0.000045 | +0.000072 | +0.000000 | +0.000026 | +0.000000 | +0.000040 | original:webis-touche2020; seed7642:nfcorpus; seed7643:fiqa,trec-covid |
| logistic | 0 | 0 | 11.0 | 4 | 0.25 | +0.000012 | +0.000000 | +0.000000 | +0.000005 | +0.000000 | +0.000014 | original:dbpedia-entity,trec-covid; seed7642:nfcorpus,trec-covid; seed7643:trec-covid,webis-touche2020 |

## Decision

M787 found no clean generated-delta surface. Stop this bounded output-level smoke and expand supervision before training.
