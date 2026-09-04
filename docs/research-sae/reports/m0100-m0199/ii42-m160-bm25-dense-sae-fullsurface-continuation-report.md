# ii42 M160 BM25/Dense/SAE Full-Surface Continuation Report

Date: 2026-06-04

## Status

This report tracks the clean M160 continuation after the C6/C7 admission and
fusion diagnostics showed that post-hoc fixed policies and runtime-safe gates
were not enough. The current route is not another small fusion repair. It
rebuilds Stage-B rows from a full-corpus source surface that contains:

- BM25 top-k scores and ranks.
- Dense teacher top-k scores and ranks.
- SAE top-k scores and ranks from the current M160 checkpoint.
- BM25+dense and BM25+SAE fusion ranks.
- Qrel positives.
- Explicit categories for SAE positives lost by fusion, BM25 positives lost by
  fusion, dense-only representation gaps, BM25+SAE rank regressions, and
  high-BM25 false positives.

The training loss is responsible for final BM25+SAE admission/ranking rather
than only sparse atom imitation.

## Current Run

Run name:

```text
ii42-m160a-c6-b12-rankreg-admission-v1
```

Spark paths:

```text
/home/huoju/leask/runs/ii42-m160a-c6-b10-bm25-dense-sae-fullsurface-v1
/home/huoju/leask/runs/ii42-m160a-c6-b10-bm25-dense-sae-fullsurface-v1-surfaces
/home/huoju/leask/logs/ii42_m160a_c6_b10_bm25_dense_sae_fullsurface_v1.log
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-surfaces
/home/huoju/leask/logs/ii42_m160a_c6_b12_rankreg_admission_v1.log
```

Tmux session:

```text
ii42_m160a_c6_b12_rankreg_admission_v1
```

B9 was stopped because it used the pre-C6 M160A Stage-B checkpoint as the
SAE source and training initializer. B10 fixed that checkpoint problem, but
its sorted first-N query surface was mostly ordinary `bm25_sae_hit` rows and
did not expose the failure mode shown by the official gate. B11 added random
hard-pool query selection, but the row categories still collapsed the main
error into `bm25_sae_hit`: relevant documents were often present in
BM25+SAE, but ranked worse than dense/SAE.

B12 is the current clean continuation. It starts from the strong C6 checkpoint
and adds `bm25_sae_rank_regression` as a first-class hard category.

## Data/Dimension Guardrails

The new row builder refuses legacy 768-dimensional inputs and validates the
checkpoint before row construction:

```text
expected_dim = 1024
expected_features = 16384
```

The checkpoint used for B12 is:

```text
/home/huoju/leask/runs/ii42-m160a-stageb-direct-c6-official-dense-miss-v1/bm25sae_stageb_best.pt
```

## Important Pipeline Fix

The first B9 attempt exposed a performance bug in the large-corpus row builder.
BM25 only needs document text, but the builder was scanning
`documents.jsonl`, which includes 1024-dimensional embeddings. For large BEIR
datasets this is huge:

```text
msmarco documents.input.jsonl = 4.1G
msmarco documents.jsonl       = 62G
hotpotqa documents.input.jsonl = 2.3G
hotpotqa documents.jsonl       = 37G
fever documents.input.jsonl    = 3.7G
fever documents.jsonl          = 39G
dbpedia documents.input.jsonl  = 2.4G
dbpedia documents.jsonl        = 33G
```

The builder now uses:

- `documents.input.jsonl` for BM25 stats/ranking.
- `documents.jsonl` for dense and SAE scoring.

This keeps the training target unchanged while making large-corpus surface
generation feasible.

Observed speed change on `msmarco_train_max100`:

- Before fix: about 3 hours to reach `2M / 8.8M` docs in BM25 stats.
- After fix: about 2 minutes to reach `8M+ / 8.8M` docs in BM25 stats.

## Completed Smoke

A Spark smoke on `fiqa train max=5` passed with the corrected text-only BM25
path:

```text
bm25_documents_path = /home/huoju/leask/runs/m150-beir-full-pplx/fiqa/documents.input.jsonl
dense_documents_path = /home/huoju/leask/runs/m150-beir-full-pplx/fiqa/documents.jsonl
bm25_nonzero_scores = 206
sae_nonzero_scores = 227
bm25_sae_ranked_candidates = 301
```

The strict preflight also passed when run with the matching `candidate_k=64`.

## Rank-Regression Surface Fix

The strong C6 checkpoint already has high SAE semantic coverage. The official
gate failure was mainly ranking/fusion: BM25+SAE could find relevant documents
but rank them below the corresponding dense/SAE position. The previous row
builder marked these rows as normal `bm25_sae_hit`, so hard-pool selection
kept too many easy rows and too few actual correction samples.

The builder now emits `bm25_sae_rank_regression` when:

- The relevant document is present in BM25+SAE source ranks.
- Dense or SAE ranks the relevant document within the configured anchor window.
- BM25+SAE ranks it worse than the dense/SAE anchor by the configured margin.

Default B12 settings:

```text
rank_regression_anchor_top_k = 50
rank_regression_margin = 8
row_weight_rank_regression = 4.5
```

Spark smoke on `fiqa dev max=120 pool=500` confirmed that this category finds
the intended signal:

```text
bm25_sae_rank_regression = 63
bm25_sae_hit = 25
high_bm25_false_positive = 17
sae_positive_lost_by_fusion = 6
dense_only_representation_gap = 7
bm25_positive_lost_by_fusion = 2
```

## Completed B10 Surface Shards

The first B10 train shards have been generated from the C6 checkpoint:

| Shard | Rows | BM25 Nonzero | Dense Nonzero | SAE Nonzero | BM25+SAE Ranked | Category Summary |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `fiqa_train_max300` | 299 | 35,055 | 57,363 | 40,114 | 55,251 | `bm25_sae_hit=299` |
| `nfcorpus_train_max300` | 300 | 15,152 | 51,535 | 36,802 | 41,622 | `bm25_sae_hit=297`, `high_bm25_false_positive=3` |
| `scifact_train_max300` | 300 | 39,089 | 57,599 | 44,729 | 55,978 | `bm25_sae_hit=300` |

## B12 Early Surface Check

B12 passed strict preflight:

- Sampled document and query embeddings are 1024-dimensional.
- Checkpoint feature count is 16,384.
- BM25 scores, SAE source scores, and BM25+SAE ranks are required by preflight.

Early B12 training shards:

| Shard | Rows | BM25 Nonzero | Dense Nonzero | SAE Nonzero | BM25+SAE Ranked | Category Summary |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `fiqa_train_max150_pool600` | 148 | 17,110 | 28,384 | 19,845 | 27,548 | `bm25_sae_hit=118`, `bm25_sae_rank_regression=26`, `high_bm25_false_positive=3`, `sae_positive_lost_by_fusion=1` |
| `nfcorpus_train_max150_pool600` | 150 | 11,077 | 26,652 | 19,001 | 22,568 | `bm25_sae_hit=123`, `bm25_sae_rank_regression=20`, `dense_only_representation_gap=4`, `high_bm25_false_positive=2`, `sae_positive_lost_by_fusion=1` |
| `scifact_train_max160_pool500` | 160 | 21,224 | 30,719 | 24,188 | 29,830 | `bm25_sae_hit=156`, `bm25_sae_rank_regression=3`, `high_bm25_false_positive=1` |

This is the expected shape: small/easy corpora remain mostly hit rows, while
the corrected category now extracts rank-regression cases instead of hiding
them inside `bm25_sae_hit`. B12 is currently building the large-corpus
`msmarco` surface.

## Next Gate

B12 is only useful if it improves the official/full-corpus gate. Candidate-row
or local validation metrics are diagnostic only. Promotion requires checking:

- Standalone SAE does not regress versus the current C6/M160 best profile.
- BM25+SAE improves final top-100 admission without sacrificing top-20 ranking.
- Large-corpus shards do not collapse on `msmarco`, `hotpotqa`, `fever`, or
  `dbpedia-entity`.
- Physical cost remains trackable: candidate docs, SAE postings, and latency.

## B12 Milestone Result

B12 completed successfully on Spark. The tmux session exited normally after
training, full-corpus evaluation, and miss taxonomy generation.

Artifacts:

```text
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-full-corpus-eval
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-full-corpus-eval/m110_full_corpus_index_eval.json
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-full-corpus-eval/m110_full_corpus_rankings.jsonl
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-full-corpus-eval/bm25sae_miss_taxonomy.json
```

Training summary:

```text
best_metric = 1.3062388825445668
training_elapsed_seconds = 748.37
full_eval_elapsed_seconds = 288.17
```

Learned fusion profile emitted by the checkpoint:

```text
score_fusion_sae_weight = 1.0100988149642944
score_fusion_bm25_weight = 0.19722731411457062
bm25_sae_fusion_mode = residual
doc_active_k = 96
query_active_k = 96
```

Final full-corpus metrics:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| `dense` | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| `sae` | 0.3274 | 0.2975 | 0.2221 | 0.1539 |
| `bm25_dense_score_fusion` | 0.3185 | 0.2970 | 0.2256 | 0.1499 |
| `bm25_sae_score_fusion` | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `bm25_sae_rrf` | 0.3263 | 0.2795 | 0.1986 | 0.1310 |

Compared with dense, `bm25_sae_score_fusion` is now better on
`Recall@100`, `MRR@20`, and `MAP@100`, but still slightly below dense on
`NDCG@10`. This is the first M160 continuation checkpoint where BM25+SAE
clearly reaches the dense control on most aggregate metrics while keeping the
unified sparse route.

Miss taxonomy:

| Signal | Count |
| --- | ---: |
| `queries` | 886 |
| `relevant_docs` | 23,475 |
| `covered_by_bm25_sae` | 3,444 |
| `bm25_sae_relevant_hits` | 3,444 |
| `dense_relevant_hits` | 3,449 |
| `bm25_dense_relevant_hits` | 3,478 |
| `candidate_hit_score_low` | 891 |
| `dense_hit_sae_candidate_missed` | 373 |
| `dense_only_candidate_missed` | 125 |
| `not_retrieved_by_controls` | 18,642 |

The largest actionable error bucket is `candidate_hit_score_low`, not
representation miss. That means the next optimization should focus on
top-rank scoring/admission before any new Stage-A representation training.

## B12 Post-Hoc Fusion Diagnostic

A low-cost post-hoc fusion sweep was run over the completed B12 rankings:

```text
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-posthoc-fusion-diagnostic
```

The compact named-profile matrix is recorded in:

```text
docs/research-sae/reports/m0100-m0199/ii42-m160-b12-named-scoring-profile-report.md
```

Best fixed-policy signals:

| Metric | Best policy | Value |
| --- | --- | ---: |
| `recall@20` | `additive_w0.3_bm25top20` | 0.2506 |
| `recall@100` | `additive_w0.15_bm25top20` | 0.3368 |
| `mrr@20` | `bm25_sae_score_fusion` | 0.3017 |
| `ndcg@10` | `residual_w0.5_bm25top100` | 0.2278 |
| `map@100` | `residual_w0.5_bm25top100` | 0.1586 |

The important result is `residual_w0.5_bm25top100`:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `dense` | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| `bm25_sae_score_fusion` | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `residual_w0.5_bm25top100` | 0.3322 | 0.2945 | 0.2278 | 0.1586 |

This proves admission/fusion/top-rank scoring has real headroom. A simple
runtime-safe residual policy can push `NDCG@10` and `MAP@100` above dense, but
it gives back some `MRR@20`. Therefore the next step should not be a generic
adaptive gate or broad encoder retraining. It should be a narrow scoring
profile step:

1. Promote `bm25_sae_score_fusion` as the current MRR-oriented profile.
2. Add `residual_w0.5_bm25top100` as an NDCG/MAP-oriented profile.
3. Evaluate both as named ii42 scoring profiles in the official matrix.
4. Only train another admission/ranking head if fixed profiles cannot cover
   the product tradeoff.

Decision: B12 is a milestone checkpoint. Admission/fusion is worth pursuing,
but only as a narrow scoring-profile/top-rank optimization path. More Stage-A
or generic fusion training would be low-signal until this profile comparison is
formalized.
