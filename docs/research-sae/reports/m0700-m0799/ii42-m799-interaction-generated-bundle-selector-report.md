# M799 Interaction Generated Bundle Selector

M799 adds M798 lexical moved-document interaction witnesses to the
M796 generated-bundle selector.  The default setting hard-filters to
the cross-zero safety set before threshold selection.

## Test Results

| Score | Model | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| conservative_margin | hgb | 1 | 0 | 34.3 | +0.000063 | +0.000139 | +0.000000 | +0.000020 | +0.000080 | +0.000000 | +0.000245 | original:trec-covid,webis-touche2020; seed7642:nfcorpus,trec-covid,webis-touche2020; seed7643:dbpedia-entity,nfcorpus,trec-covid,webis-touche2020 |
| margin_bundle | hgb | 0 | 0 | 10.7 | +0.000016 | +0.000039 | +0.000000 | +0.000000 | +0.000026 | +0.000000 | +0.000068 | seed7643:nfcorpus |
| conservative_margin | logistic | 0 | 0 | 4.0 | +0.000000 | +0.000058 | +0.000000 | +0.000000 | +0.000012 | +0.000000 | +0.000065 | seed7642:climate-fever |
| margin_bundle | logistic | 0 | 0 | 3.0 | -0.000005 | +0.000035 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000030 | seed7642:climate-fever,nfcorpus |
| score_mean_margin | hgb | 0 | 0 | 6.3 | -0.000006 | +0.000018 | +0.000000 | +0.000006 | +0.000028 | +0.000000 | +0.000028 | original:webis-touche2020; seed7642:webis-touche2020; seed7643:nfcorpus,webis-touche2020 |
| score_mean_margin | logistic | 0 | 0 | 3.3 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | +0.000012 | +0.000000 | +0.000005 | seed7642:climate-fever |

## Decision

M799 found no clean interaction-aware selector. Interaction features are observable but not yet deployable in this form.
