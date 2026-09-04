# M775 Damage-Veto Selector

M775 adds one global qrels-free damage veto on top of the M774
selector score.  The veto is selected on dev surfaces and replayed
unchanged on test surfaces.

## Test Results

| Model | Label | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Veto | Negative Surfaces |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| hgb | query_strict_positive | 0 | 0 | 22.7 | +0.000029 | +0.000044 | -0.000062 | +0.000014 | +0.000000 | +0.000068 | `None` | original:dbpedia-entity,scidocs; seed7642:dbpedia-entity,fiqa; seed7643:trec-covid,webis-touche2020 |
| logistic | query_strict_positive | 0 | 0 | 9.7 | +0.000020 | +0.000046 | -0.000062 | +0.000000 | +0.000000 | +0.000054 | `tail_abs_share_at_100 le 0.44` | original:climate-fever,scidocs; seed7642:climate-fever,dbpedia-entity; seed7643:climate-fever |
| logistic | query_utility_positive | 0 | 0 | 9.0 | +0.000024 | +0.000011 | -0.000062 | +0.000009 | +0.000000 | +0.000027 | `entropy_delta_at_100 le -6.31286e-07` | original:scidocs; seed7642:trec-covid; seed7643:trec-covid |
| hgb | query_utility_positive | 0 | 0 | 23.0 | +0.000020 | +0.000005 | -0.000062 | +0.000011 | +0.000000 | +0.000018 | `cross_rate_at_256 le 0.00390625` | original:cqadupstack,dbpedia-entity,scidocs; seed7642:dbpedia-entity,trec-covid; seed7643:trec-covid,webis-touche2020 |

## Decision

M775 found no clean damage-veto selector; selector route needs task-damage constraints or a new proposal surface.
