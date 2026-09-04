# M815C Pairwise Ranker Abstention Replay

M815C keeps the M815 pairwise ranker but adds a second global
abstention classifier trained on whether the selected top bundle is
safe-positive under native replay labels.

## Held-out Replay

| Held-out | Threshold | Clean | Applied | dMAP | dNDCG | dMRR | dRecall | dCUB | dO@100 | Utility | Eval Sel Rate | Eval Safe Rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 0.4061 | 0 | 31 | +0.000021 | +0.000117 | +0.000000 | +0.000000 | +0.000022 | +0.000000 | +0.000149 | 0.163 | 0.548 |
| seed7642 | 0.3219 | 0 | 31 | +0.000057 | +0.000062 | +0.000018 | +0.000000 | +0.000001 | +0.000000 | +0.000123 | 0.163 | 0.548 |
| seed7643 | 0.6086 | 0 | 10 | -0.000006 | -0.000015 | +0.000000 | +0.000000 | -0.000008 | +0.000000 | -0.000025 | 0.051 | 0.600 |

## Train Abstention Summary

| Held-out | Clean Threshold | Train Mean Utility | Train Sel Rate | Train Safe Rate |
| --- | ---: | ---: | ---: | ---: |
| original | 1 | +0.000917 | 0.176 | 0.957 |
| seed7642 | 1 | +0.000745 | 0.176 | 0.900 |
| seed7643 | 1 | +0.000117 | 0.069 | 1.000 |

## Held-out Selected Task Summary

| Held-out | Task | Selected | Safe Rate | Mean Utility | Sum Utility |
| --- | --- | ---: | ---: | ---: | ---: |
| original | climate-fever | 1 | 1.000 | +0.000794 | +0.000794 |
| original | cqadupstack | 3 | 0.667 | +0.001280 | +0.003840 |
| original | dbpedia-entity | 7 | 0.714 | +0.011875 | +0.083124 |
| original | fever | 1 | 0.000 | +0.000000 | +0.000000 |
| original | fiqa | 2 | 0.500 | +0.000052 | +0.000103 |
| original | msmarco | 3 | 0.667 | +0.004930 | +0.014789 |
| original | nfcorpus | 3 | 0.667 | -0.021653 | -0.064958 |
| original | scifact | 1 | 0.000 | +0.000000 | +0.000000 |
| original | trec-covid | 7 | 0.429 | +0.000364 | +0.002548 |
| original | webis-touche2020 | 3 | 0.333 | +0.000075 | +0.000225 |
| seed7642 | climate-fever | 7 | 0.429 | +0.000686 | +0.004805 |
| seed7642 | cqadupstack | 1 | 1.000 | +0.000337 | +0.000337 |
| seed7642 | dbpedia-entity | 5 | 0.800 | +0.005081 | +0.025405 |
| seed7642 | fiqa | 2 | 0.000 | -0.000156 | -0.000313 |
| seed7642 | msmarco | 1 | 1.000 | +0.000534 | +0.000534 |
| seed7642 | nfcorpus | 6 | 0.500 | +0.000413 | +0.002479 |
| seed7642 | trec-covid | 6 | 0.500 | -0.000097 | -0.000584 |
| seed7642 | webis-touche2020 | 3 | 0.667 | +0.000251 | +0.000752 |
| seed7643 | cqadupstack | 1 | 1.000 | +0.000337 | +0.000337 |
| seed7643 | dbpedia-entity | 1 | 1.000 | +0.003151 | +0.003151 |
| seed7643 | fiqa | 1 | 0.000 | +0.000000 | +0.000000 |
| seed7643 | msmarco | 1 | 1.000 | +0.000649 | +0.000649 |
| seed7643 | nfcorpus | 3 | 0.333 | -0.004093 | -0.012278 |
| seed7643 | trec-covid | 3 | 0.667 | +0.000493 | +0.001478 |

## Decision

M815C improves abstention but still has only partial held-out replay gains. Inspect failed surfaces before scaling.
