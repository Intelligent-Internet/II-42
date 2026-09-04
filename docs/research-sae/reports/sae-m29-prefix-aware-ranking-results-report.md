# SAE M29 Prefix-Aware Ranking Results Report

Status: M29 blocker push completed; query-side product gate still failed.

M29 tested whether the remaining direct `text -> atoms` blocker was caused by
Snowflake query-prefix mismatch, weak query-side ranking supervision, or a
too-small query encoder. The evidence says the prefix hypothesis is closed:
Snowflake prefixed queries remain the correct canonical teacher distribution.
The query-side ranking-first path can improve aggregate scores, but it is not
robust across full15 and therefore cannot unblock dense-removal product work.

## Executive Result

| Area | Decision | Evidence |
| --- | --- | --- |
| M29.0 prefix teacher diagnostic | `keep-prefixed-canonical` | prefixed teacher beats raw/no-prefix and mixed on NDCG@10 and MAP@100 while using fewer SAE postings |
| M29.1 query-side ranking-first | `failed-robustness-gate` | strong qrel target improves aggregate beyond teacher but collapses `trec-covid`, `msmarco`, and `dbpedia-entity` |
| M29.2 Arm A prefix/style features | `failed-robustness-gate` | query/style input prefix keeps aggregate near teacher but does not remove the same dataset collapses |
| M29.2 Arm B transformer control | `failed-quality-gate` | trainable transformer checkpoint with the same ranking loss stays below BM25+teacher and current baseline |
| M29.3 doc-side training | `blocked` | not run because fixed-doc query-side gate did not pass |

Final M29 state:

```text
prefixed Snowflake teacher remains canonical;
query-side ranking objective has signal;
dense-removal remains blocked by robustness, not by prefix formatting.
```

## M29.0 Prefix Teacher Diagnostic

Full15 mean quality:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `current_best_text_student` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| `dense_prefixed_query` | 0.8533 | 0.8722 | 0.7759 | 0.7427 |
| `dense_raw_query_no_prefix` | 0.8404 | 0.8283 | 0.7225 | 0.6826 |
| `dense_mixed_or_blended_query` | 0.8517 | 0.8644 | 0.7663 | 0.7319 |
| `teacher_prefixed_query` | 0.8455 | 0.8462 | 0.7530 | 0.7273 |
| `teacher_raw_query_no_prefix` | 0.8359 | 0.8198 | 0.7205 | 0.6955 |
| `teacher_mixed_or_blended_query` | 0.8437 | 0.8463 | 0.7492 | 0.7254 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| `teacher_prefixed_query` | 2923.9 | 10185.0 | 3385.5 |
| `teacher_raw_query_no_prefix` | 2992.4 | 10185.0 | 4061.3 |
| `teacher_mixed_or_blended_query` | 2996.7 | 10185.0 | 3971.5 |

Query-style buckets did not change the decision. Raw/no-prefix is weaker on the
important question-like and semantic-heavy groups. Mixed is close in a few
long-query buckets, but it does not beat prefixed query distribution on the
full15 ranking objective and costs more postings.

Detailed artifact:

```text
results/sae/m29/prefix-diagnostic/m29_prefix_diagnostic.md
```

## M29.1/M29.2 Query-Side Runs

All query-side runs fixed document atoms to teacher doc atoms. This isolates the
query encoder. The gate compares against `teacher_fixed_doc`, not against the
old full text-student.

| Run | Best source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Collapses | Decision |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| Arm A strong qrel target | `w0p75` | 0.8673 | 0.9080 | 0.7872 | 0.7536 | 5 | `failed-robustness` |
| Arm A clean teacher target | `w0p5` | 0.8009 | 0.8207 | 0.7108 | 0.6667 | 18 | `failed-quality` |
| Arm A qrel boost 0.25 | `w0p75` | 0.8560 | 0.8745 | 0.7555 | 0.7131 | 8 | `failed-robustness` |
| Arm A prefix/style qrel boost 0.25 | `w0p75` | 0.8515 | 0.8680 | 0.7510 | 0.7091 | 8 | `failed-robustness` |
| Arm B transformer qrel boost 0.25 | `w0p25` | 0.7934 | 0.7944 | 0.6734 | 0.6329 | 21 | `failed-quality` |

Physical profile for the main candidate runs:

| Run | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| Arm A strong qrel target | 2860.3 | 10185.0 | 1738.0 |
| Arm A clean teacher target | 2763.5 | 10185.0 | 867.6 |
| Arm A qrel boost 0.25 | 2792.7 | 10185.0 | 1001.3 |
| Arm A prefix/style qrel boost 0.25 | 2769.3 | 10185.0 | 1038.1 |
| Arm B transformer qrel boost 0.25 | 2792.6 | 10185.0 | 651.3 |
| Teacher fixed-doc reference | 2986.2 | 10185.0 | 4522.8 |

The strong-qrel run proves the query-side model can create a powerful ranking
signal and lower SAE postings. It is not acceptable because the same signal
causes large per-dataset collapses:

| Dataset | Metric | Strong qrel delta vs teacher |
| --- | --- | ---: |
| `trec-covid` | `map@100` | -0.4350 |
| `trec-covid` | `ndcg@10` | -0.3134 |
| `msmarco` | `ndcg@10` | -0.1569 |
| `msmarco` | `map@100` | -0.1512 |
| `dbpedia-entity` | `map@100` | -0.0869 |

The clean-target run proves the opposite boundary: if listwise teacher
distribution stays pure and qrels only enter through pairwise loss, the model is
too weak. The qrel-boost 0.25 and prefix/style runs sit between those extremes
but still collapse on the same datasets.

## Interpretation

The core blocker is not Snowflake query prefix handling. M29.0 confirms the
prefixed teacher is stronger and cheaper than raw/no-prefix or mixed teacher.

The core blocker is also not only encoder capacity. The transformer Arm B,
trained with the same ranking-first loss, performs worse than Arm A and below
the current best text-student frontier.

The real blocker is robust calibration of query-side semantic atoms. Qrels can
push aggregate ranking quality above the teacher, but the model learns a brittle
query distribution that over-promotes semantic atoms on datasets where exact
teacher/BM25 balance matters. Removing qrel target injection avoids the most
aggressive overfit, but then the model lacks enough ranking signal.

## M29 Exit Decision

M29 does not pass the product-research gate:

- prefix canonical decision is closed;
- query-side ranking-first objective has useful signal;
- no fixed-doc query-side run passed robustness;
- doc-side text-to-atoms training remains blocked;
- read-only teacher-path harness remains allowed only as an evaluation path;
- dense-removal claims remain not allowed.

The next model step should not be another qrel boost sweep. The next attempt
needs a supervision change that preserves teacher/BM25 calibration while using
qrels as a constraint rather than as a target rewrite. A practical next design
is dataset-family holdout training with a per-query calibration head or
teacher-residual loss, where the model learns when to trust qrels, BM25, or SAE
instead of applying one global query-side atom distribution.

## Artifacts

```text
results/sae/m29/prefix-diagnostic/m29_prefix_diagnostic.json
results/sae/m29/prefix-diagnostic/m29_prefix_diagnostic.md
results/sae/m29/query-ranking-arm-a-eval/m29_query_ranking_train.json
results/sae/m29/query-ranking-arm-a-clean-target-eval/m29_query_ranking_train.json
results/sae/m29/query-ranking-arm-a-qrel-boost025-eval/m29_query_ranking_train.json
results/sae/m29/query-ranking-arm-a-prefix-style/m29_query_ranking_train.json
results/sae/m29/query-ranking-arm-b-transformer/m29_query_ranking_train.json
```
