# M770 Proposal Quality Audit

M770 audits whether the current proposal generator leaves enough
safe-positive rows for further policy work.

## Aggregate Counts

| Split | Selected | M764 Keep | M768 Keep | Strict Safe | Strict Positive | M768+Strict Positive | Utility Positive | Dense Spend |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dev | 667 | 161 | 11 | 600 | 79 | 5 | 81 | 0 |
| test | 680 | 173 | 18 | 609 | 78 | 5 | 87 | 0 |

## Surface Counts

| Surface | Split | Selected | M768 Keep | Strict Positive | Utility Positive | Dense Spend |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| original | dev | 221 | 4 | 25 | 25 | 0 |
| original | test | 224 | 4 | 26 | 29 | 0 |
| seed7642 | dev | 222 | 2 | 25 | 26 | 0 |
| seed7642 | test | 222 | 9 | 24 | 27 | 0 |
| seed7643 | dev | 224 | 5 | 29 | 30 | 0 |
| seed7643 | test | 234 | 5 | 28 | 31 | 0 |

## Top Test Task Counts

| Task | Selected | M768 Keep | Strict Positive | Dense Spend |
| --- | ---: | ---: | ---: | ---: |
| arguana | 44 | 0 | 0 | 0 |
| climate-fever | 54 | 0 | 6 | 0 |
| cqadupstack | 56 | 1 | 8 | 0 |
| dbpedia-entity | 47 | 1 | 10 | 0 |
| fever | 49 | 0 | 0 | 0 |
| fiqa | 53 | 2 | 0 | 0 |
| hotpotqa | 51 | 0 | 0 | 0 |
| msmarco | 27 | 7 | 4 | 0 |
| nfcorpus | 43 | 4 | 19 | 0 |
| nq | 44 | 0 | 0 | 0 |
| quora | 53 | 0 | 0 | 0 |
| scidocs | 56 | 0 | 7 | 0 |
| scifact | 46 | 0 | 2 | 0 |
| trec-covid | 29 | 2 | 12 | 0 |
| webis-touche2020 | 28 | 1 | 10 | 0 |

## Strongest Test Feature Separation

### M768 Keep

| Feature | Effect | Positive Mean | Negative Mean |
| --- | ---: | ---: | ---: |
| trace_position_keep_at_256 | +3.437 | 0.711589 | 0.399865 |
| trace_mean_abs_at_256 | -2.637 | 0.358073 | 1.30484 |
| trace_mean_drop_at_256 | -2.631 | 0.179253 | 0.662103 |
| trace_p95_abs_at_256 | -2.444 | 1.59722 | 4.40521 |
| trace_position_keep_at_128 | +2.200 | 0.786458 | 0.56728 |
| trace_max_drop_at_256 | -2.191 | 2.88889 | 8.12387 |
| trace_p95_abs_at_128 | -2.035 | 1.11111 | 2.52885 |
| trace_mean_drop_at_128 | -2.033 | 0.124132 | 0.35076 |
| trace_mean_abs_at_128 | -2.024 | 0.245226 | 0.690804 |
| trace_position_keep_at_100 | +1.809 | 0.816111 | 0.625211 |

### Query Strict Positive

| Feature | Effect | Positive Mean | Negative Mean |
| --- | ---: | ---: | ---: |
| ctx_margin_at_128 | -0.506 | 0.000254172 | 0.0004711 |
| ctx_top_mean_delta_at_10 | +0.423 | 0.000354821 | 0.000126925 |
| ctx_top_mean_delta_at_20 | +0.419 | 0.000333943 | 0.000120528 |
| ctx_margin_delta_at_128 | -0.371 | -0.000129673 | 1.00004e-05 |
| ctx_margin_at_50 | +0.371 | 0.00188246 | 0.00113115 |
| ctx_top_mean_delta_at_50 | +0.332 | 0.000286839 | 0.000128339 |
| ctx_threshold_delta_at_20 | +0.294 | 0.000278585 | 9.33206e-05 |
| ctx_top_mean_delta_at_100 | +0.293 | 0.000252277 | 0.000134923 |
| ctx_top_mean_delta_at_128 | +0.287 | 0.000239832 | 0.00013232 |
| trace_p95_abs_at_10 | +0.266 | 0.316026 | 0.195681 |

## Decision

M770 finds safe-positive proposal mass, but the robust rank-trace guard is too conservative; do not tune thresholds, test a richer native online accept/fallback.
