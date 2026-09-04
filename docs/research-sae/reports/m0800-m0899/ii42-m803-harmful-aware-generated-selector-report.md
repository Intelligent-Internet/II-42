# M803 Harmful-Aware Generated Selector

M803 uses the M802 cached feature table and trains separate
strict-positive and harmful-row heads.  Candidate score is
`p_positive - lambda * p_harmful`; dev selects a global threshold
under the same strict surface gate.

## Test Results

| Score | Model | Lambda | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility | Negative Surfaces |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| margin_bundle | logistic | 0.5 | 1 | 1 | 3.0 | +0.000012 | +0.000078 | +0.000000 | +0.000000 | +0.000041 | +0.000000 | +0.000110 | None |
| margin_bundle | logistic | 1.0 | 1 | 1 | 1.0 | +0.000005 | +0.000078 | +0.000000 | +0.000000 | +0.000041 | +0.000000 | +0.000104 | None |
| score_mean_margin | logistic | 2.0 | 1 | 1 | 2.7 | +0.000002 | +0.000078 | +0.000000 | +0.000000 | +0.000041 | +0.000000 | +0.000101 | None |
| margin_bundle | hgb | 2.0 | 1 | 0 | 116.0 | +0.000183 | +0.000278 | +0.000000 | +0.000099 | +0.000046 | +0.000000 | +0.000504 | seed7642:dbpedia-entity,scidocs; seed7643:msmarco,nfcorpus,trec-covid |
| margin_bundle | hgb | 5.0 | 1 | 0 | 115.7 | +0.000099 | +0.000176 | +0.000000 | +0.000099 | +0.000041 | +0.000000 | +0.000315 | original:cqadupstack,hotpotqa,scidocs; seed7642:dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,hotpotqa,msmarco,nfcorpus,scidocs |
| margin_bundle | logistic | 5.0 | 1 | 0 | 43.3 | +0.000098 | +0.000160 | +0.000000 | +0.000085 | +0.000041 | +0.000000 | +0.000295 | seed7643:msmarco |
| score_mean_margin | logistic | 5.0 | 1 | 0 | 23.0 | +0.000089 | +0.000160 | +0.000000 | +0.000085 | +0.000041 | +0.000000 | +0.000286 | seed7643:msmarco |
| conservative_margin | logistic | 5.0 | 1 | 0 | 37.3 | +0.000086 | +0.000160 | +0.000000 | +0.000085 | +0.000041 | +0.000000 | +0.000283 | seed7643:climate-fever,msmarco |
| conservative_margin | hgb | 0.0 | 1 | 0 | 34.3 | +0.000063 | +0.000139 | +0.000000 | +0.000020 | +0.000080 | +0.000000 | +0.000245 | original:trec-covid,webis-touche2020; seed7642:nfcorpus,trec-covid,webis-touche2020; seed7643:dbpedia-entity,nfcorpus,trec-covid,webis-touche2020 |
| margin_bundle | hgb | 1.0 | 1 | 0 | 18.7 | +0.000056 | +0.000113 | +0.000000 | +0.000014 | +0.000067 | +0.000000 | +0.000206 | original:webis-touche2020; seed7642:climate-fever,dbpedia-entity; seed7643:dbpedia-entity,nfcorpus |
| score_mean_margin | logistic | 3.0 | 1 | 0 | 10.7 | +0.000048 | +0.000122 | +0.000000 | +0.000044 | +0.000041 | +0.000000 | +0.000199 | seed7643:msmarco |
| margin_bundle | hgb | 3.0 | 0 | 0 | 119.0 | +0.000115 | +0.000241 | +0.000000 | +0.000099 | +0.000041 | +0.000000 | +0.000397 | original:hotpotqa; seed7642:dbpedia-entity,hotpotqa,scidocs; seed7643:hotpotqa,msmarco,nfcorpus,scidocs |
| conservative_margin | hgb | 1.0 | 0 | 0 | 133.0 | +0.000094 | +0.000240 | +0.000000 | -0.000101 | +0.000082 | +0.000000 | +0.000355 | original:hotpotqa,scidocs,webis-touche2020; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,dbpedia-entity,hotpotqa,msmarco,nfcorpus,trec-covid,webis-touche2020 |
| conservative_margin | hgb | 0.5 | 0 | 0 | 79.7 | +0.000038 | +0.000264 | +0.000000 | -0.000144 | +0.000087 | +0.000000 | +0.000317 | original:hotpotqa,scidocs,webis-touche2020; seed7642:climate-fever,dbpedia-entity,hotpotqa,quora,scidocs; seed7643:dbpedia-entity,hotpotqa,msmarco,nfcorpus,scidocs,trec-covid,webis-touche2020 |
| margin_bundle | logistic | 3.0 | 0 | 0 | 42.0 | +0.000098 | +0.000118 | +0.000000 | +0.000085 | +0.000041 | +0.000000 | +0.000254 | seed7642:dbpedia-entity; seed7643:msmarco |
| score_mean_margin | hgb | 2.0 | 0 | 0 | 115.3 | +0.000077 | +0.000085 | +0.000000 | -0.000101 | +0.000043 | +0.000000 | +0.000163 | original:fiqa,hotpotqa,scidocs,webis-touche2020; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,hotpotqa,msmarco,nfcorpus,webis-touche2020 |
| score_mean_margin | hgb | 3.0 | 0 | 0 | 114.3 | +0.000076 | +0.000083 | +0.000000 | -0.000101 | +0.000043 | +0.000000 | +0.000161 | original:hotpotqa,scidocs,webis-touche2020; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,hotpotqa,msmarco,webis-touche2020 |
| conservative_margin | hgb | 5.0 | 0 | 0 | 116.3 | +0.000083 | +0.000072 | +0.000000 | -0.000101 | +0.000041 | +0.000000 | +0.000155 | original:hotpotqa,scidocs; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,dbpedia-entity,hotpotqa,msmarco,nfcorpus |
| conservative_margin | logistic | 2.0 | 0 | 0 | 60.7 | +0.000020 | +0.000116 | +0.000000 | -0.000124 | +0.000041 | +0.000000 | +0.000132 | original:climate-fever,scidocs,webis-touche2020; seed7642:climate-fever,fiqa,scidocs,webis-touche2020; seed7643:climate-fever,fiqa,msmarco,webis-touche2020 |
| score_mean_margin | hgb | 5.0 | 0 | 0 | 113.3 | +0.000052 | +0.000074 | +0.000000 | -0.000101 | +0.000043 | +0.000000 | +0.000127 | original:cqadupstack,hotpotqa,scidocs,webis-touche2020; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,hotpotqa,msmarco,webis-touche2020 |
| score_mean_margin | hgb | 1.0 | 0 | 0 | 49.0 | +0.000062 | +0.000057 | +0.000000 | -0.000144 | +0.000061 | +0.000000 | +0.000122 | original:scidocs,trec-covid,webis-touche2020; seed7642:climate-fever,dbpedia-entity,nfcorpus,scidocs; seed7643:cqadupstack,nfcorpus,webis-touche2020 |
| score_mean_margin | logistic | 0.5 | 0 | 0 | 3.7 | -0.000005 | +0.000101 | +0.000000 | +0.000000 | +0.000041 | +0.000000 | +0.000116 | seed7642:climate-fever,nfcorpus |
| margin_bundle | hgb | 0.5 | 0 | 0 | 14.0 | +0.000059 | +0.000035 | +0.000000 | +0.000014 | +0.000026 | +0.000000 | +0.000109 | seed7642:climate-fever; seed7643:nfcorpus |
| conservative_margin | logistic | 3.0 | 0 | 0 | 60.7 | +0.000020 | +0.000093 | +0.000000 | -0.000130 | +0.000041 | +0.000000 | +0.000108 | original:climate-fever,scidocs; seed7642:climate-fever,fiqa,scidocs; seed7643:climate-fever,fiqa,msmarco |
| score_mean_margin | logistic | 1.0 | 0 | 0 | 9.3 | +0.000007 | +0.000085 | +0.000000 | -0.000103 | +0.000041 | +0.000000 | +0.000092 | seed7642:scidocs |
| margin_bundle | logistic | 2.0 | 0 | 0 | 27.7 | +0.000013 | +0.000036 | +0.000000 | +0.000000 | +0.000041 | +0.000000 | +0.000070 | seed7642:dbpedia-entity; seed7643:msmarco |
| conservative_margin | hgb | 2.0 | 0 | 0 | 121.3 | +0.000083 | -0.000006 | +0.000000 | -0.000101 | +0.000026 | +0.000000 | +0.000070 | original:hotpotqa,scidocs,webis-touche2020; seed7642:dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,dbpedia-entity,hotpotqa,msmarco,nfcorpus,webis-touche2020 |
| margin_bundle | hgb | 0.0 | 0 | 0 | 10.7 | +0.000016 | +0.000039 | +0.000000 | +0.000000 | +0.000026 | +0.000000 | +0.000068 | seed7643:nfcorpus |
| conservative_margin | logistic | 0.0 | 0 | 0 | 4.0 | +0.000000 | +0.000058 | +0.000000 | +0.000000 | +0.000012 | +0.000000 | +0.000065 | seed7642:climate-fever |
| score_mean_margin | hgb | 0.5 | 0 | 0 | 13.0 | +0.000016 | +0.000033 | +0.000000 | +0.000020 | +0.000020 | +0.000000 | +0.000063 | original:climate-fever,trec-covid,webis-touche2020; seed7642:climate-fever,webis-touche2020; seed7643:nfcorpus,webis-touche2020 |
| conservative_margin | hgb | 3.0 | 0 | 0 | 116.7 | +0.000081 | -0.000006 | +0.000000 | -0.000101 | +0.000000 | +0.000000 | +0.000055 | original:hotpotqa,scidocs; seed7642:climate-fever,dbpedia-entity,hotpotqa,scidocs; seed7643:cqadupstack,dbpedia-entity,hotpotqa,msmarco,nfcorpus |
| conservative_margin | logistic | 0.5 | 0 | 0 | 6.0 | -0.000005 | +0.000023 | +0.000000 | +0.000000 | +0.000026 | +0.000000 | +0.000031 | seed7642:climate-fever |
| margin_bundle | logistic | 0.0 | 0 | 0 | 3.0 | -0.000005 | +0.000035 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000030 | seed7642:climate-fever,nfcorpus |
| score_mean_margin | hgb | 0.0 | 0 | 0 | 6.3 | -0.000006 | +0.000018 | +0.000000 | +0.000006 | +0.000028 | +0.000000 | +0.000028 | original:webis-touche2020; seed7642:webis-touche2020; seed7643:nfcorpus,webis-touche2020 |
| score_mean_margin | logistic | 0.0 | 0 | 0 | 3.0 | -0.000004 | +0.000000 | +0.000000 | +0.000000 | +0.000012 | +0.000000 | +0.000002 | seed7642:climate-fever |
| conservative_margin | logistic | 1.0 | 0 | 0 | 17.7 | -0.000054 | -0.000044 | +0.000000 | -0.000205 | +0.000000 | +0.000000 | -0.000139 | original:scidocs,webis-touche2020; seed7642:scidocs,webis-touche2020; seed7643:msmarco,webis-touche2020 |

## Decision

M803 found a clean harmful-aware selector worth one broader replay.
