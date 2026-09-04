# M1944 Full-Tail Latent Separability

Decision: **authorize_retrieval_conditioned_selector_canary**

## Dataset Summary

| Dataset | Residual | TargetRecall | HarmRecall | SourceDimsMean | SourceDimsP95 | SourcePostingsP95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | 250 | 1.0000 | 1.0000 | 7381.5 | 8445.8 | 75477.2 |
| arguana | 16 | 1.0000 | 1.0000 | 9321.2 | 9902.8 | 96340.2 |
| nfcorpus | 271 | 1.0000 | 1.0000 | 10572.0 | 11212.5 | 119545.0 |
| scifact | 14 | 1.0000 | 1.0000 | 9963.4 | 10939.5 | 111090.6 |

## LODO

| Task | PooledAUC | MinFoldAUC | Records |
| --- | ---: | ---: | ---: |
| `target_vs_harm` | 0.996981231775869 | 0.9967402010050251 | 8806 |
| `target_vs_rest` | 0.9885953048283785 | 0.9833214757424327 | 26438 |

## Gate

| Check | Passed |
| --- | --- |
| `dataset_count` | `true` |
| `residual_queries_per_dataset` | `true` |
| `source_dimensions_mean` | `true` |
| `source_dimensions_p95` | `true` |
| `source_postings_p95` | `true` |
| `source_target_recall` | `true` |
| `source_target_recall_per_dataset` | `true` |
| `target_harm_fold_count` | `true` |
| `target_harm_min_fold_auc` | `true` |
| `target_harm_pooled_auc` | `true` |
| `target_rest_fold_count` | `true` |
| `target_rest_min_fold_auc` | `true` |
| `target_rest_pooled_auc` | `true` |

## Interpretation

Full-tail context exposes a cross-corpus separable target within the predeclared cost floors. One fixed-budget selector canary is authorized; native promotion is not.

Qrels define diagnostic labels and sampling only. Every model feature is available from the frozen first-pass posting response.

## Scope And Closure

This result is a sampled-separability result, not a full action-space ranking
result. Each query contributes every visible target and harm but only 32 hard
neutral dimensions. The high LODO AUC therefore proves that target and harm are
distinguishable inside this diagnostic support; it does not prove that target
dimensions outrank the thousands of other source dimensions.

M1945 performed the authorized all-source canary without changing features,
classifier family or selector budget. The signal did not survive the complete
source space. M1944 remains useful evidence about target-versus-harm geometry,
but it does not authorize native replay or neural selector training.
