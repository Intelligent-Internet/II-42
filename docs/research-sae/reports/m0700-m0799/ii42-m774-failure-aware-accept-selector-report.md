# M774 Failure-Aware Accept Selector

M774 adds qrels-free native damage-witness features to the M772
accept-selector smoke: crossing counts, entropy shifts, score-shape
deltas, reciprocal-rank movement, and tail displacement share.

## Test Results

| Model | Label | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| hgb | query_utility_positive | 1 | 0 | 2.7 | +0.000018 | +0.000034 | +0.000000 | +0.000014 | +0.000000 | +0.000059 | original:trec-covid; seed7642:trec-covid |
| hgb | query_strict_positive | 0 | 0 | 26.3 | +0.000028 | +0.000044 | -0.000062 | +0.000039 | +0.000000 | +0.000080 | original:dbpedia-entity,scidocs; seed7642:dbpedia-entity,fiqa,nfcorpus; seed7643:trec-covid,webis-touche2020 |
| logistic | query_strict_positive | 0 | 0 | 2.0 | -0.000005 | +0.000019 | +0.000000 | +0.000000 | +0.000000 | +0.000015 | original:climate-fever; seed7642:climate-fever; seed7643:climate-fever,trec-covid |
| logistic | query_utility_positive | 0 | 0 | 2.3 | -0.000005 | +0.000019 | +0.000000 | +0.000000 | +0.000000 | +0.000015 | original:climate-fever; seed7642:climate-fever; seed7643:climate-fever,trec-covid |

## Decision

M774 found no clean trained selector; failure-aware features help only if task damage can be explicitly constrained.
