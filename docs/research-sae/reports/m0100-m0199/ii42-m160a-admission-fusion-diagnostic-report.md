# ii42 M160A Admission/Fusion Diagnostic Report

## Status

This is a partial official BEIR full-corpus diagnostic over completed
`all-test` datasets from:

`/home/huoju/leask/runs/ii42-m160a-c6-fixed-policy-official-beir-v1`

Completed datasets in this snapshot:

- `arguana`
- `cqadupstack`
- `fiqa`
- `nfcorpus`
- `quora`
- `scidocs`
- `scifact`
- `trec-covid`
- `webis-touche2020`

Still missing from the official matrix:

- `nq`
- `dbpedia-entity`
- `hotpotqa`
- `fever`
- `climate-fever`
- `msmarco`

The `nq` full-corpus evaluator was stopped after it stayed in
`loading document embeddings` while using about `119Gi / 121Gi` RAM and
`8Gi` swap. Later large datasets likely require a sharded evaluator rather
than another monolithic full-corpus process.

## Artifacts

- Fixed-policy matrix:
  `/home/huoju/leask/runs/ii42-m160a-c6-fixed-policy-official-beir-v1-fixed-policy-matrix-partial`
- Admission/fusion diagnostic:
  `/home/huoju/leask/runs/ii42-m160a-c6-fixed-policy-official-beir-v1-admission-diagnostic-partial`
- Fixed-policy grid search:
  `/home/huoju/leask/runs/ii42-m160a-c6-fixed-policy-official-beir-v1-fusion-grid-partial`

## Macro Metrics

9-dataset partial macro:

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 0.4518 | 0.5625 | 0.4894 | 0.4195 | 0.2849 |
| `dense` | 0.5520 | 0.6726 | 0.5999 | 0.5357 | 0.3748 |
| `bm25_dense_score_fusion` | 0.5562 | 0.6734 | 0.6010 | 0.5342 | 0.3716 |
| `sae` | 0.5220 | 0.6436 | 0.5483 | 0.4791 | 0.3382 |
| `bm25_sae_score_fusion` | 0.5393 | 0.6574 | 0.5764 | 0.5055 | 0.3573 |

Fixed BM25+SAE policies did not beat learned BM25+SAE on ranking metrics.
Their main value is diagnostic: they expose extra Recall@100 headroom, but
they do not solve final ordering.

## Admission/Fusion Findings

Macro diagnostic over the same 9 datasets:

| Metric | Value | Meaning |
| --- | ---: | --- |
| `sae_union_bm25_recall` | 0.6924 | Oracle top100 recall if SAE top100 and BM25 top100 were both admitted. |
| `sae_union_bm25_headroom_recall` | 0.0488 | Extra recall available from BM25 admission over SAE-only. |
| `fusion_recall_delta_vs_sae` | 0.0139 | Net recall gained by the current learned fusion over SAE-only. |
| `fusion_recovered_headroom_recall` | 0.0277 | Positive part of BM25 admission recovered by fusion. |
| `fusion_sae_lost_recall` | 0.0197 | SAE qrel positives dropped by the fusion top100. |
| `dense_only_vs_bm25_sae_union_recall` | 0.0312 | Dense positives not covered by SAE top100 or BM25 top100. |

Query ratios:

| Ratio | Value | Meaning |
| --- | ---: | --- |
| `query_ratio_with_bm25_only_relevant` | 0.2981 | BM25 has a qrel positive not present in SAE top100. |
| `query_ratio_with_fusion_admitted_bm25_positive` | 0.2736 | Fusion actually admits a BM25-only qrel positive. |
| `query_ratio_with_sae_positive_lost_by_fusion` | 0.2257 | Fusion drops at least one SAE qrel positive. |
| `query_ratio_with_bm25_only_nonrel_in_fusion_top20` | 0.4148 | BM25-only non-qrel positives enter fusion top20. |
| `query_ratio_with_high_bm25_false_positive_risk` | 0.0754 | BM25-only top20 admission coincides with worse NDCG than SAE. |

The central failure mode is not that BM25 has no useful signal. It has useful
admission headroom. The failure is that current fusion admits some useful BM25
documents while also dropping enough SAE positives and promoting enough
BM25-only non-qrel positives to lose ranking quality on datasets such as
`fiqa`, `scidocs`, `trec-covid`, and `cqadupstack`.

## Per-Dataset Read

| Dataset | SAE R@100 | Fusion R@100 | Dense R@100 | BM25+SAE Union R@100 | Headroom | SAE Lost |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9893 | 0.9936 | 1.0000 | 0.9950 | 0.0057 | 0.0007 |
| `cqadupstack` | 0.7211 | 0.7222 | 0.7826 | 0.7624 | 0.0414 | 0.0281 |
| `fiqa` | 0.7659 | 0.7583 | 0.8290 | 0.8011 | 0.0352 | 0.0325 |
| `nfcorpus` | 0.3023 | 0.3140 | 0.3270 | 0.3475 | 0.0452 | 0.0169 |
| `quora` | 0.9933 | 0.9956 | 0.9960 | 0.9973 | 0.0040 | 0.0012 |
| `scidocs` | 0.4634 | 0.4549 | 0.4958 | 0.5056 | 0.0422 | 0.0333 |
| `scifact` | 0.9450 | 0.9767 | 0.9633 | 0.9833 | 0.0383 | 0.0067 |
| `trec-covid` | 0.1286 | 0.1301 | 0.1673 | 0.1858 | 0.0572 | 0.0279 |
| `webis-touche2020` | 0.4832 | 0.5717 | 0.4928 | 0.6534 | 0.1701 | 0.0297 |

`webis-touche2020` and `scifact` prove BM25 admission can help substantially.
`fiqa` and `scidocs` show the opposite case: naive admission hurts because it
drops SAE positives and admits high-BM25 non-qrel positives. The next model
needs conditional admission, not a single global BM25 weight.

## Fixed Policy Grid Search

The grid search over `additive`, `residual`, and `excess` fixed policies did
not find a BM25+SAE fixed policy that beats dense or BM25+dense on the partial
macro.

Macro best:

| Metric | Best source | Value |
| --- | --- | ---: |
| `recall@20` | `bm25_dense_score_fusion` | 0.5562 |
| `recall@100` | `bm25_dense_score_fusion` | 0.6734 |
| `mrr@20` | `bm25_dense_score_fusion` | 0.6010 |
| `ndcg@10` | `dense` | 0.5357 |
| `map@100` | `dense` | 0.3748 |

The best fixed BM25+SAE policy is dataset-specific. For example, `scifact`
likes `additive_w0.75_bm25top50`, while `webis-touche2020` still prefers
lexical-heavy behavior. This reinforces that fixed policy deployment is not
enough.

## Next Training Target

Do not train a generic adaptive gate yet. The C7 runtime gate was weaker than
fixed policies, and fixed policies are weaker than the learned fusion on
ranking. The next useful training should be an admission/ranking objective
with explicit examples:

- Positive admission examples: qrel positives in `BM25 top100` but not in
  `SAE top100`.
- Negative admission examples: high-BM25, BM25-only non-qrel positives that
  enter fusion top20 and reduce NDCG/MRR versus SAE-only.
- Preservation examples: qrel positives already in `SAE top100` that current
  fusion drops from top100.
- Dense-gap examples: dense positives not covered by `SAE top100 ∪ BM25 top100`,
  which point back to representation rather than fusion.

The training target should optimize final BM25+SAE top100 admission and top20
ranking directly:

1. Keep SAE positives unless BM25 provides stronger evidence.
2. Admit BM25-only positives when BM25 is likely additive rather than noisy.
3. Penalize high-BM25 false positives that suppress SAE qrel positives.
4. Preserve query-level runtime safety by tracking fanout and candidate count.

This is a Stage-B/C reset around admission/ranking supervision, not another
Stage-A representation run.
