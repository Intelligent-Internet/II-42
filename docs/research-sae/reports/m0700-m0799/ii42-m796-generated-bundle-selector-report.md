# M796 Generated Bundle Selector

M796 trains a bounded selector over M794 query-local generated
bundles.  Features are limited to bundle metadata and native
candidate-vs-baseline trace signals available before qrels scoring.

## Test Results

| Score | Model | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| conservative_margin | hgb | 1 | 0 | 17.0 | +0.000076 | +0.000101 | +0.000000 | +0.000041 | +0.000004 | +0.000000 | +0.000187 | original:webis-touche2020; seed7642:climate-fever,nfcorpus,quora,scidocs; seed7643:nfcorpus |
| score_mean_margin | hgb | 1 | 0 | 13.7 | +0.000038 | +0.000049 | +0.000000 | +0.000020 | +0.000005 | +0.000000 | +0.000094 | original:cqadupstack; seed7642:nfcorpus,trec-covid; seed7643:nfcorpus,trec-covid |
| score_mean_margin | logistic | 0 | 0 | 5.0 | +0.000025 | +0.000076 | +0.000000 | +0.000000 | -0.000005 | +0.000000 | +0.000098 | seed7642:climate-fever; seed7643:trec-covid |
| margin_bundle | logistic | 0 | 0 | 3.0 | -0.000004 | +0.000066 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000062 | seed7642:climate-fever,nfcorpus |
| conservative_margin | logistic | 0 | 0 | 3.3 | -0.000010 | +0.000058 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000048 | original:nfcorpus; seed7642:climate-fever,nfcorpus; seed7643:nfcorpus |
| margin_bundle | hgb | 0 | 0 | 15.0 | +0.000009 | +0.000020 | +0.000000 | +0.000000 | +0.000015 | +0.000000 | +0.000037 | original:dbpedia-entity,trec-covid; seed7642:climate-fever,trec-covid; seed7643:trec-covid |

## Decision

M796 found no clean generated-bundle selector. M795 oracle signal is not recoverable by the current observable features.
