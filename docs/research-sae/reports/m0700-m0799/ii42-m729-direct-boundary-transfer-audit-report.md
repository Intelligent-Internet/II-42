# M729 Direct Boundary Transfer Audit

Status: `coverage_and_tail_transfer_issue`

M729 audits the M653 direct dense-boundary compiler.  The compiler
already trained on dense top100 missing-vs-false-P1 pairs and passed
the canary gate, but failed full shared15 on O@256.  This report joins
shared15 replay deltas with the original boundary-row coverage.  It
does not train a model and does not use BM25, reranker, learned gates,
or qrels-driven selection.

## Macro

```json
{
  "boundary_pair_count_mean": 9.080476900149032,
  "d_candidate_upper_bound": 3.667338738456823e-05,
  "d_dense_overlap_at_10": -0.00014903129657228007,
  "d_dense_overlap_at_100": 5.9612518628911875e-05,
  "d_dense_overlap_at_256": -1.4553837555886736e-05,
  "d_dense_overlap_at_50": 0.0002235469448584207,
  "d_dense_top100_rank_corr": 6.196481388583318e-05,
  "d_map_at_100": -8.827732251333679e-05,
  "d_mrr_at_20": 0.00037338705974192954,
  "d_ndcg_at_10": 6.789776747711054e-05,
  "d_recall_at_100": 9.041239541620644e-05,
  "d_source_boundary_error": -6.864252659851677e-06,
  "d_source_false_count": -0.005961251862891207,
  "d_source_missing_dense_count": -0.005961251862891207,
  "o100_positive_o256_negative_queries": 34.0,
  "o256_negative_queries": 216.0,
  "o256_positive_queries": 211.0,
  "query_count": 1342.0,
  "recall_positive_o256_negative_queries": 0.0
}
```

## Coverage Summary

| Group | Q | dO@100 | dO@256 | dR@100 | dMAP@100 | O256- Q | O256+ Q | Pair count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boundary_query` | +399.00000000 | +0.00047619 | -0.00001958 | +0.00038191 | -0.00009586 | +79.00000000 | +76.00000000 | +30.54135338 |
| `same_dataset_no_boundary_query` | +1.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 |
| `unseen_dataset` | +942.00000000 | -0.00011677 | -0.00001244 | -0.00003296 | -0.00008516 | +137.00000000 | +135.00000000 | +0.00000000 |

## Boundary Split Summary

| Group | Q | dO@100 | dO@256 | dR@100 | dMAP@100 | O256- Q | O256+ Q | Pair count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dev` | +60.00000000 | +0.00033333 | +0.00052083 | +0.00000000 | -0.00000952 | +7.00000000 | +14.00000000 | +30.70000000 |
| `none` | +943.00000000 | -0.00011665 | -0.00001243 | -0.00003292 | -0.00008507 | +137.00000000 | +135.00000000 | +0.00000000 |
| `test` | +86.00000000 | +0.00011628 | +0.00000000 | +0.00177187 | +0.00039740 | +15.00000000 | +16.00000000 | +28.96511628 |
| `train` | +253.00000000 | +0.00063241 | -0.00015440 | +0.00000000 | -0.00028401 | +57.00000000 | +46.00000000 | +31.03952569 |

## Worst O@256 Queries

| Dataset | Query | Coverage | Split | Pairs | dO@100 | dO@256 | dR@100 | dMAP@100 |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| `msmarco` | `207786` | `unseen_dataset` | `none` | 0 | +0.00000000 | -0.01562500 | +0.00000000 | +0.00000000 |
| `nfcorpus` | `PLAIN-924` | `unseen_dataset` | `none` | 0 | +0.00000000 | -0.01171875 | +0.00000000 | -0.00001212 |
| `nq` | `test38` | `unseen_dataset` | `none` | 0 | +0.01000000 | -0.01171875 | +0.00000000 | +0.00000000 |
| `cqadupstack` | `android_17433` | `boundary_query` | `dev` | 64 | -0.01000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `cqadupstack` | `android_36706` | `boundary_query` | `train` | 16 | -0.01000000 | -0.00781250 | +0.00000000 | -0.00326797 |
| `dbpedia-entity` | `INEX_LD-2010020` | `unseen_dataset` | `none` | 0 | -0.01000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `arguana` | `test-health-ahiahbgbsp-pro02a` | `boundary_query` | `train` | 36 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `arguana` | `test-health-dhgsshbesbc-con02a` | `boundary_query` | `train` | 25 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `arguana` | `test-health-hpehwadvoee-pro04a` | `boundary_query` | `train` | 4 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `climate-fever` | `141` | `unseen_dataset` | `none` | 0 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `climate-fever` | `86` | `unseen_dataset` | `none` | 0 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00000000 |
| `cqadupstack` | `android_12834` | `boundary_query` | `train` | 16 | +0.00000000 | -0.00781250 | +0.00000000 | +0.00127426 |

## Interpretation

Full shared15 dO@256 is -0.00001455 with +216.00000000 negative-O@256 queries. Boundary-covered queries have dO@256 -0.00001958; same-dataset queries without boundary rows have dO@256 +0.00000000; unseen datasets have dO@256 -0.00001244.  If the loss is mostly in unseen or no-boundary rows, the next step is broader direct boundary training coverage.  If boundary-covered rows are also negative, the next step must change the compiler geometry.

## Decision

Direct boundary coverage alone does not explain the O@256 failure.  The next trainable probe should change compiler geometry, e.g. a two-head dense-tail compiler or an isolated doc-side probe, before scaling the same query-side objective.
