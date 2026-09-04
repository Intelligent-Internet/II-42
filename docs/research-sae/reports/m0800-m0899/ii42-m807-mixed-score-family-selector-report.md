# M807 Mixed Score-Family Harmful Selector

M807 trains one selector over all generated score families with
score-family one-hot features.  Selection still uses the abstained
policy from M806: `score >= threshold` and `p_harmful <= cap`.

## Held-Out Cases

| Held-out | Model | Lambda | Cap | Gate | Clean | Applied | dMAP | dNDCG | dCUB | dO@100 | Utility | Negative Tasks |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| original | logistic | 3.0 | 0.3227 | 1 | 1 | 48.0 | +0.000071 | +0.000066 | +0.000000 | +0.000000 | +0.000150 | None |
| original | logistic | 5.0 | 0.2420 | 1 | 1 | 51.0 | +0.000070 | +0.000066 | +0.000000 | +0.000000 | +0.000149 | None |
| seed7642 | logistic | 3.0 | 0.2356 | 1 | 0 | 44.0 | +0.000130 | +0.000113 | +0.000000 | +0.000000 | +0.000268 | seed7642:dbpedia-entity |
| seed7642 | logistic | 5.0 | 0.2356 | 1 | 0 | 48.0 | +0.000123 | +0.000113 | +0.000000 | +0.000000 | +0.000261 | seed7642:climate-fever;dbpedia-entity |
| seed7642 | logistic | 2.0 | 0.1030 | 1 | 0 | 31.0 | +0.000122 | +0.000113 | +0.000000 | +0.000000 | +0.000260 | seed7642:climate-fever |
| seed7643 | logistic | 3.0 | 0.2646 | 1 | 0 | 31.0 | +0.000065 | +0.000066 | +0.000000 | +0.000000 | +0.000141 | seed7643:fiqa;msmarco |
| seed7643 | logistic | 5.0 | 0.2646 | 1 | 0 | 40.0 | +0.000048 | +0.000066 | +0.000000 | +0.000000 | +0.000124 | seed7643:fiqa;msmarco |
| seed7642 | logistic | 0.5 | 0.4558 | 1 | 0 | 19.0 | +0.000003 | +0.000024 | +0.000000 | +0.000000 | +0.000030 | seed7642:webis-touche2020 |
| original | logistic | 0.0 | 0.4785 | 0 | 0 | 25.0 | +0.000030 | +0.000314 | +0.000000 | +0.000000 | +0.000282 | original:cqadupstack;scidocs;webis-touche2020 |
| original | logistic | 1.0 | 0.4785 | 0 | 0 | 37.0 | +0.000085 | +0.000018 | +0.000000 | +0.000000 | +0.000055 | original:cqadupstack;scidocs;webis-touche2020 |
| seed7643 | logistic | 0.0 | 0.2646 | 0 | 0 | 21.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | -0.000003 | seed7643:fiqa;msmarco |
| seed7643 | logistic | 2.0 | 0.2646 | 0 | 0 | 28.0 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | -0.000004 | seed7643:fiqa;msmarco |
| seed7643 | logistic | 0.5 | 0.3538 | 0 | 0 | 7.0 | -0.000004 | +0.000000 | +0.000000 | +0.000000 | -0.000004 | seed7643:webis-touche2020 |
| seed7643 | logistic | 1.0 | 0.3538 | 0 | 0 | 27.0 | -0.000007 | +0.000000 | +0.000000 | +0.000000 | -0.000010 | seed7643:fiqa;msmarco;webis-touche2020 |
| original | logistic | 0.5 | 0.7828 | 0 | 0 | 31.0 | +0.000020 | +0.000021 | +0.000000 | +0.000000 | -0.000021 | original:cqadupstack;scidocs;webis-touche2020 |
| seed7642 | logistic | 1.0 | 0.4558 | 0 | 0 | 52.0 | +0.000055 | -0.000089 | +0.000000 | +0.000000 | -0.000067 | seed7642:dbpedia-entity;scidocs;webis-touche2020 |
| original | logistic | 2.0 | 0.4785 | 0 | 0 | 36.0 | -0.000006 | -0.000034 | +0.000000 | +0.000000 | -0.000088 | original:scidocs |
| seed7642 | logistic | 0.0 | 0.3072 | 0 | 0 | 49.0 | -0.000081 | -0.000225 | +0.000000 | +0.000000 | -0.000368 | seed7642:cqadupstack;dbpedia-entity;fiqa;scidocs |

## Common Config Check

| Model | Lambda | All Clean | Min Utility | Mean Utility | Applied Sum |
| --- | ---: | ---: | ---: | ---: | ---: |
| logistic | 3.0 | 0 | +0.000141 | +0.000186 | 123.0 |
| logistic | 5.0 | 0 | +0.000124 | +0.000178 | 139.0 |
| logistic | 0.5 | 0 | -0.000021 | +0.000002 | 57.0 |
| logistic | 1.0 | 0 | -0.000067 | -0.000008 | 116.0 |
| logistic | 2.0 | 0 | -0.000088 | +0.000056 | 95.0 |
| logistic | 0.0 | 0 | -0.000368 | -0.000029 | 95.0 |

## Decision

M807 found held-out clean cases, but no common mixed config. The mixture idea is not deployable yet.
