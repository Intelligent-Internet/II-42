# SAE M130 BM25+SAE Super-Recall Initial Report

Date: 2026-05-23

## Current State

M130 starts from the saved M129 frontier:

| Profile | SAE Postings / Query | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M125 teacher-coverage w1.0 | 36,743 | 0.4269 | 0.5035 | 0.3729 | 0.2388 |
| M128 recall-tune w1.0 | 35,113 | 0.4261 | 0.5077 | 0.3755 | 0.2401 |

M128 is the current top-rank profile. M125 remains the current balanced
Recall@100 profile.

## M130 Changes Started

- Added a BM25+dense row to the M110 full-corpus evaluator.
- Added optional per-query ranking export for miss taxonomy.
- Added an M130 miss taxonomy script to classify where BM25+SAE loses dense or
  BM25+dense relevant hits.
- Started distributed pplx TEI setup:
  - `homeai` GPU1: primary pplx endpoint with TEI `cuda-1.9`.
  - `spark`: secondary pplx endpoint with the local `tei-cuda:arm64` image,
    because the official TEI CUDA image does not publish a Linux ARM64
    manifest.
- TEI status:
  - `homeai` TEI reached model startup but did not become healthy on RTX 3090
    GPU1; the initial `65536/128` config OOMed and lower `4096/8` still reset
    requests.
  - `spark` ARM64 TEI failed with CUDA illegal address and was stopped.
  - PyTorch/SentenceTransformer with `transformers==4.57.3` succeeds on
    `homeai` GPU1 and returns `(2, 1024)` float32 arrays with int8-like
    unnormalized values, so M130 should use batch materialization first.

## First Validation Targets

1. Run pplx batch materialization through the pinned PyTorch route on `homeai`
   GPU1 while keeping `spark` for training.
2. Run the M110 evaluator with `--write-rankings` to produce BM25, dense,
   BM25+dense, BM25+SAE, and ranking JSONL.
3. Run `research_sae_m130_miss_taxonomy.py` over that ranking JSONL.
4. Use taxonomy results to decide whether the next training change should
   prioritize candidate coverage, scoring calibration, or fanout suppression.

## Same-Surface Snowflake/M128 Baseline

The first same-surface M130 rerun uses the M128 checkpoint and writes per-query
rankings to:

```text
/home/huoju/leask/runs/m130-m128-full-corpus-rankings/
```

| Row | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.3693 | 0.4463 | 0.3216 | 0.2002 |
| Snowflake dense | 0.4388 | 0.5044 | 0.3844 | 0.2584 |
| BM25+dense score fusion | 0.4412 | 0.4960 | 0.3796 | 0.2537 |
| M128 SAE | 0.3712 | 0.4109 | 0.2980 | 0.1885 |
| BM25+M128 SAE score fusion | 0.4261 | 0.5077 | 0.3755 | 0.2401 |

This confirms the active blocker under the same full-corpus evaluator:
BM25+SAE has stronger top-rank MRR than BM25+dense, but still trails dense and
BM25+dense on Recall@100, NDCG@10, and MAP@100.

Physical diagnostics for this run:

| Row | Accumulator Entries / Query | Postings / Query |
| --- | ---: | ---: |
| BM25 | 12,531 | 53,406 |
| SAE | 17,564 | 35,113 |

## First Miss Taxonomy

Taxonomy output:

```text
/home/huoju/leask/runs/m130-m128-full-corpus-rankings/m130_miss_taxonomy.json
```

| Category | Count |
| --- | ---: |
| covered_by_bm25_sae | 5,732 |
| candidate_hit_score_low | 1,532 |
| dense_hit_sae_candidate_missed | 1,214 |
| dense_only_candidate_missed | 622 |
| not_retrieved_by_controls | 14,306 |

Control hit counts:

| Control | Relevant Hits @100 |
| --- | ---: |
| Dense | 6,684 |
| BM25+dense | 6,540 |
| BM25+SAE | 5,732 |

The useful signal is split between candidate coverage and scoring:
`1,836` dense-backed relevant hits are absent from the current SAE/BM25
candidate surface, while `1,532` relevant docs enter BM25 or SAE candidates but
are ranked too low. M130 training should therefore combine semantic teacher
coverage with BM25-aware ranking calibration rather than only increasing SAE
fanout.

## PPLX Materialization Status

TEI is parked for now. The working pplx path is PyTorch/SentenceTransformer
inside `nvcr.io/nvidia/pytorch:26.03-py3` with:

```text
transformers==4.57.3
sentence-transformers
trust_remote_code=True
```

`homeai` GPU1 completed query embeddings:

```text
/home/huoju/leask/runs/m130-pplx-embeddings/queries.pplx.jsonl
```

The output shape is 1024 dimensions and the values are unnormalized
int8-like float32 values, matching the intended pplx dense baseline contract.
Document embedding is running in tmux session `m130_pplx_docs`; once complete,
the PPLX corpus will be built with
`research_sae_m130_build_embedding_corpus.py` and evaluated with
`research_sae_m110_full_corpus_index_eval.py --skip-sae`.

## PPLX Baseline Results

PPLX document materialization completed on `homeai` GPU1:

```text
/home/huoju/leask/runs/m130-pplx-embeddings/documents.pplx.jsonl
/home/huoju/leask/runs/m130-pplx-embeddings/queries.pplx.jsonl
```

The full PPLX corpus has all `30,059` documents and `886` queries:

```text
/home/huoju/leask/runs/m130-pplx-corpus/
/home/huoju/leask/runs/m130-pplx-dense-baseline/
```

Full-coverage PPLX baseline:

| Row | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.4075 | 0.4483 | 0.3452 | 0.2340 |
| PPLX dense | 0.4967 | 0.5357 | 0.4331 | 0.3128 |
| BM25+PPLX dense score fusion | 0.4913 | 0.5235 | 0.4181 | 0.2982 |

This row is useful for the target full-corpus direction, but it is not directly
comparable with M128 because the old Snowflake/SAE artifact only has embeddings
for `25,863` documents. For apples-to-apples comparison, M130 also builds a
restricted PPLX corpus with exactly the Snowflake docset:

```text
/home/huoju/leask/runs/m130-pplx-corpus-snowflake-docset/
/home/huoju/leask/runs/m130-pplx-dense-baseline-snowflake-docset/
```

Restricted PPLX baseline, same `25,863` documents and `823` queries as M128:

| Row | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Snowflake dense | 0.4388 | 0.5044 | 0.3844 | 0.2584 |
| BM25+Snowflake dense score fusion | 0.4412 | 0.4960 | 0.3796 | 0.2537 |
| M128 BM25+SAE score fusion | 0.4261 | 0.5077 | 0.3755 | 0.2401 |
| PPLX dense | 0.4600 | 0.5157 | 0.4003 | 0.2712 |
| BM25+PPLX dense score fusion | 0.4547 | 0.5062 | 0.3872 | 0.2583 |

PPLX is therefore the strongest teacher/control so far under the same docset.
It raises the dense-only teacher by about `+0.0211` Recall@100, `+0.0159`
NDCG@10, and `+0.0128` MAP@100 versus Snowflake dense. Against M128 BM25+SAE,
restricted PPLX dense is higher by about `+0.0338` Recall@100, `+0.0080`
MRR@10, `+0.0248` NDCG@10, and `+0.0311` MAP@100.

## PPLX-vs-M128 Cross Taxonomy

Cross-taxonomy output:

```text
/home/huoju/leask/runs/m130-pplx-dense-baseline-snowflake-docset/m130_pplx_vs_m128_cross_taxonomy.json
```

| Category | Count |
| --- | ---: |
| covered_by_candidate_bm25_sae | 5,732 |
| candidate_hit_score_low | 1,532 |
| teacher_dense_and_fusion_candidate_missed | 1,362 |
| teacher_dense_candidate_missed | 616 |
| not_retrieved_by_teacher_or_candidate | 14,164 |

Relevant hit counts:

| Surface | Relevant Hits @100 |
| --- | ---: |
| PPLX dense | 6,788 |
| BM25+PPLX dense | 6,664 |
| M128 BM25+SAE | 5,732 |

This is stronger evidence than the Snowflake-only taxonomy. M130 should train
against PPLX neighborhoods first, but the loss must be BM25-aware: `1,362`
PPLX-backed relevant docs are found by both PPLX dense and BM25+PPLX dense but
are absent from the current BM25+SAE candidate surface, while `1,532` relevant
docs are candidates and need ranking calibration. The next model step should
not be another scalar fusion sweep; it should explicitly optimize dense-hit
coverage, candidate-surface inclusion, and BM25/SAE score calibration together.

## Full-Data Correction

The first executable PPLX route reused an M107-compatible train surface. That
is useful for wiring, but it is too small for the M130 product question. The
available staged data is larger:

| Surface | Documents | Queries | Supervised rows |
| --- | ---: | ---: | ---: |
| M107 train root | 71,417 | 3,920 | 3,920 |
| M81 large Stage-A stable | 315,840 | 54,807 | 4,806 |
| M52 balanced Stage-A source | 797,754 | 199,438 | 0 |
| M60 MS MARCO supervised split | 100,000 | 2,000 | 2,000 |

M130 is therefore redirected to a fresh, all-data PPLX/DiffSAE route:

1. rebuild the source surface with full M52 and full M60 limits instead of the
   earlier capped smoke limits;
2. filter out the held-out M97/M130 eval query IDs before constructing
   training qrels and candidate rows;
3. materialize PPLX embeddings for the resulting full document/query surface
   with resumable output;
4. train the Stage-A PPLX SAE from scratch, then run BM25-aware Stage-B
   ranking calibration from that fresh PPLX checkpoint.

This also corrects an important design point: M128/Snowflake checkpoints remain
controls only. M130 should not initialize from them unless the run is explicitly
marked as a legacy ablation.

## Full-Data Source Built

The first full-data source build completed on `spark`:

```text
/home/huoju/leask/data/ii42_sae_m130/all-data-source-v0
```

Source size:

| Item | Count |
| --- | ---: |
| Documents | 993,336 |
| Queries | 205,945 |
| Candidate rows | 6,506 |

After filtering held-out M97/M130 eval query IDs, the train input is:

```text
/home/huoju/leask/data/ii42_sae_m130/all-data-train-input-v0
```

Filtered training surface:

| Item | Count |
| --- | ---: |
| Documents | 993,336 |
| Queries | 205,059 |
| Qrel queries | 5,620 |
| Qrel pairs | 100,060 |
| Candidate rows | 5,620 |

Documents are intentionally not filtered out by eval doc ID. The protected
signal is query/qrel relevance; index-time document encoding must still cover
the corpus.

The input has been sharded for distributed PPLX materialization:

```text
/home/huoju/leask/data/ii42_sae_m130/all-data-train-shards-v0
```

Current shard execution:

| Host | Shards |
| --- | --- |
| `spark` | `documents.shard004-007`, `queries.shard002-003` |
| `homeai` GPU1 | `documents.shard000-003`, `queries.shard000-001` |

Spark tmux/log:

```text
tmux: m130_pplx_embed_spark
log: /home/huoju/leask/logs/m130_pplx_embed_spark.log
```

Homeai tmux/log:

```text
tmux: m130_pplx_embed_homeai_all
log: /home/huoju/leask/logs/m130_pplx_embed_homeai_all.log
```

Homeai pushes completed shards back to Spark with:

```text
tmux: m130_homeai_push
log: /home/huoju/leask/logs/m130_homeai_push.log
```

Spark runs the unattended downstream pipeline with:

```text
tmux: m130_after_embeddings
log: /home/huoju/leask/logs/m130_after_embeddings.log
```

That pipeline waits for all PPLX shards, merges them, builds the full-data
PPLX corpus, runs BM25/PPLX dense baselines, builds candidate rows, then trains
fresh `8192/k64`, `16384/k96`, and `16384/k128` PPLX DiffSAE variants until
one clears the BM25+dense full-corpus gate or all configured variants fail.

ClearML SDK smoke succeeded against the no-password server:

```text
http://100.116.110.26:8080/projects/0c10e0ce204049fd9ce88fc009b04885/experiments/fd987fd9ab8f409099284f19b4a015ba/output/log
```

The M109 fresh PPLX DiffSAE trainer now supports ClearML logging through the
shared tracking helper.
