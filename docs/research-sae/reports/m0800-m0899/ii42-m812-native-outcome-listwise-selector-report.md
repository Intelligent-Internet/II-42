# M812 Native-Outcome Listwise Selector

M812 trains a generated-bundle selector on native replay outcome
deltas instead of row-level strict-positive labels.  It evaluates
leave-surface-out: train and threshold on two surfaces, replay on the
held-out surface.

## Held-Out Cases

| Held-out | Model | Risk Lambda | Risk Cap | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Negative Tasks |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| seed7643 | ridge | 0.25 | 0.1452 | 1 | 0 | 130.0 | +0.000083 | +0.000264 | +0.000066 | +0.000123 | +0.000000 | +0.000422 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;nfcorpus;webis-touche2020 |
| seed7643 | ridge | 0.10 | 0.1452 | 1 | 0 | 124.0 | +0.000082 | +0.000264 | +0.000066 | +0.000114 | +0.000000 | +0.000416 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;nfcorpus;trec-covid;webis-touche2020 |
| seed7643 | ridge | 0.00 | 0.1452 | 1 | 0 | 118.0 | +0.000080 | +0.000264 | +0.000066 | +0.000114 | +0.000000 | +0.000415 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;nfcorpus;trec-covid;webis-touche2020 |
| seed7643 | ridge | 0.50 | 0.1452 | 1 | 0 | 134.0 | +0.000075 | +0.000264 | +0.000066 | +0.000123 | +0.000000 | +0.000414 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;msmarco;nfcorpus;webis-touche2020 |
| original | hgb | 0.25 | 0.5569 | 1 | 0 | 34.0 | +0.000093 | +0.000243 | +0.000041 | +0.000023 | +0.000000 | +0.000357 | original:climate-fever;trec-covid;webis-touche2020 |
| seed7643 | ridge | 1.00 | 0.4401 | 1 | 0 | 141.0 | +0.000061 | +0.000169 | +0.000051 | +0.000176 | +0.000000 | +0.000328 | seed7643:cqadupstack;dbpedia-entity;fiqa;hotpotqa;msmarco;nfcorpus;webis-touche2020 |
| seed7642 | hgb | 0.50 | 0.9246 | 1 | 0 | 88.0 | +0.000177 | +0.000020 | +0.000141 | +0.000010 | +0.000000 | +0.000230 | seed7642:cqadupstack;dbpedia-entity;scidocs;trec-covid |
| original | hgb | 0.00 | 0.5569 | 1 | 0 | 24.0 | +0.000062 | +0.000130 | +0.000000 | +0.000023 | +0.000000 | +0.000204 | original:trec-covid;webis-touche2020 |
| original | hgb | 0.10 | 0.5569 | 1 | 0 | 24.0 | +0.000052 | +0.000130 | +0.000000 | +0.000023 | +0.000000 | +0.000194 | original:climate-fever;trec-covid;webis-touche2020 |
| seed7643 | hgb | 1.00 | 0.8086 | 1 | 0 | 135.0 | +0.000009 | +0.000001 | +0.000066 | +0.000170 | +0.000000 | +0.000108 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;nfcorpus;scidocs;trec-covid;webis-touche2020 |
| seed7642 | hgb | 1.00 | 0.9246 | 0 | 0 | 139.0 | +0.000075 | +0.000249 | -0.000167 | +0.000010 | +0.000000 | +0.000296 | seed7642:cqadupstack;dbpedia-entity;hotpotqa;scidocs;trec-covid |
| original | hgb | 1.00 | 0.5569 | 0 | 0 | 134.0 | +0.000041 | +0.000155 | -0.000201 | +0.000016 | +0.000000 | +0.000164 | original:hotpotqa;scidocs;trec-covid |
| seed7642 | hgb | 0.00 | 0.0319 | 0 | 0 | 65.0 | +0.000138 | -0.000012 | +0.000123 | +0.000000 | +0.000000 | +0.000150 | seed7642:climate-fever;dbpedia-entity;scidocs |
| seed7642 | hgb | 0.10 | 0.0319 | 0 | 0 | 67.0 | +0.000138 | -0.000012 | +0.000123 | +0.000000 | +0.000000 | +0.000150 | seed7642:climate-fever;dbpedia-entity;scidocs |
| seed7642 | hgb | 0.25 | 0.0319 | 0 | 0 | 71.0 | +0.000135 | -0.000012 | +0.000123 | +0.000000 | +0.000000 | +0.000148 | seed7642:climate-fever;dbpedia-entity;scidocs |
| original | hgb | 0.50 | 0.5569 | 0 | 0 | 65.0 | +0.000033 | +0.000122 | -0.000267 | +0.000023 | +0.000000 | +0.000114 | original:scidocs;trec-covid;webis-touche2020 |
| original | ridge | 0.50 | 0.1499 | 0 | 0 | 118.0 | +0.000058 | +0.000080 | -0.000201 | +0.000000 | +0.000000 | +0.000097 | original:climate-fever;cqadupstack;dbpedia-entity;fiqa;scidocs |
| original | ridge | 0.00 | 0.1499 | 0 | 0 | 117.0 | +0.000056 | +0.000080 | -0.000201 | +0.000000 | +0.000000 | +0.000096 | original:climate-fever;cqadupstack;dbpedia-entity;fiqa;scidocs |
| original | ridge | 0.10 | 0.1499 | 0 | 0 | 120.0 | +0.000056 | +0.000080 | -0.000201 | +0.000000 | +0.000000 | +0.000096 | original:climate-fever;cqadupstack;dbpedia-entity;fiqa;scidocs |
| seed7643 | hgb | 0.50 | 0.1452 | 0 | 0 | 61.0 | +0.000026 | -0.000032 | +0.000000 | +0.000123 | +0.000000 | +0.000055 | seed7643:dbpedia-entity;nfcorpus |
| original | ridge | 1.00 | 0.5569 | 0 | 0 | 134.0 | +0.000009 | +0.000047 | -0.000201 | +0.000000 | +0.000000 | +0.000016 | original:climate-fever;dbpedia-entity;fiqa;hotpotqa;scidocs;trec-covid;webis-touche2020 |
| seed7643 | hgb | 0.25 | 0.0543 | 0 | 0 | 95.0 | -0.000014 | +0.000013 | +0.000066 | +0.000000 | +0.000000 | +0.000013 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;msmarco;nfcorpus;scidocs |
| original | ridge | 0.25 | 0.1499 | 0 | 0 | 123.0 | +0.000005 | +0.000047 | -0.000201 | +0.000000 | +0.000000 | +0.000012 | original:climate-fever;cqadupstack;dbpedia-entity;fiqa;hotpotqa;scidocs |
| seed7642 | ridge | 1.00 | 0.0613 | 0 | 0 | 119.0 | +0.000037 | -0.000121 | -0.000185 | +0.000000 | +0.000000 | -0.000121 | seed7642:dbpedia-entity;fiqa;hotpotqa;scidocs |
| seed7643 | hgb | 0.00 | 0.0543 | 0 | 0 | 87.0 | -0.000079 | -0.000053 | +0.000000 | +0.000000 | +0.000000 | -0.000132 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;msmarco;nfcorpus;scidocs |
| seed7643 | hgb | 0.10 | 0.0543 | 0 | 0 | 89.0 | -0.000079 | -0.000053 | +0.000000 | +0.000000 | +0.000000 | -0.000132 | seed7643:cqadupstack;dbpedia-entity;hotpotqa;msmarco;nfcorpus;scidocs |
| seed7642 | ridge | 0.25 | 0.0613 | 0 | 0 | 114.0 | +0.000023 | -0.000121 | -0.000185 | +0.000000 | +0.000000 | -0.000135 | seed7642:dbpedia-entity;fiqa;hotpotqa;scidocs |
| seed7642 | ridge | 0.50 | 0.0613 | 0 | 0 | 115.0 | +0.000023 | -0.000121 | -0.000185 | +0.000000 | +0.000000 | -0.000135 | seed7642:dbpedia-entity;fiqa;hotpotqa;scidocs |
| seed7642 | ridge | 0.00 | 0.0613 | 0 | 0 | 109.0 | -0.000003 | -0.000121 | -0.000185 | +0.000000 | +0.000000 | -0.000161 | seed7642:climate-fever;dbpedia-entity;hotpotqa;scidocs |
| seed7642 | ridge | 0.10 | 0.0613 | 0 | 0 | 112.0 | -0.000003 | -0.000121 | -0.000185 | +0.000000 | +0.000000 | -0.000161 | seed7642:climate-fever;dbpedia-entity;hotpotqa;scidocs |

## Common Config Check

| Model | Risk Lambda | All Clean | Min Utility | Mean Utility | Applied Sum |
| --- | ---: | ---: | ---: | ---: | ---: |
| hgb | 1.00 | 0 | +0.000108 | +0.000189 | 408.0 |
| hgb | 0.50 | 0 | +0.000055 | +0.000133 | 214.0 |
| hgb | 0.25 | 0 | +0.000013 | +0.000172 | 200.0 |
| ridge | 1.00 | 0 | -0.000121 | +0.000074 | 394.0 |
| hgb | 0.00 | 0 | -0.000132 | +0.000074 | 176.0 |
| hgb | 0.10 | 0 | -0.000132 | +0.000071 | 180.0 |
| ridge | 0.50 | 0 | -0.000135 | +0.000125 | 367.0 |
| ridge | 0.25 | 0 | -0.000135 | +0.000100 | 367.0 |
| ridge | 0.10 | 0 | -0.000161 | +0.000117 | 356.0 |
| ridge | 0.00 | 0 | -0.000161 | +0.000116 | 344.0 |

## Decision

M812 did not find a deployable native-outcome common policy. The next step needs stronger data or a different compiler objective.
