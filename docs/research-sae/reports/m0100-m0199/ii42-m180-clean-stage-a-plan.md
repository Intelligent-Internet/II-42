# ii42 M180 Clean Stage-A Plan

## Goal

M180 is a clean restart of the Stage-A line. It keeps the successful lessons
from M150, M160, and M170, but removes the unclear parts:

- no stale M130 candidate/coverage rows;
- no hidden query repeat inside a source repeat;
- no final BM25+SAE ranking loss in Stage A;
- no official BEIR test query text or test qrels in training;
- every source ratio, embedding dimension, and manifest input must be auditable.

The immediate target is a strong sparse semantic base that preserves PPLX dense
geometry and is ready for a later, separately supervised BM25-complement Stage
B/C. Stage A is not allowed to claim final product ranking quality by itself.

## Lessons Being Applied

M150A1 was the strongest historical path after C6/C7, but the strength came
from later dense-miss and BM25+dense supervised correction. The Stage-A artifact
itself was mostly dense-neighborhood representation training and did not use
coverage auxiliary rows in the actual run.

M160A fixed the major M150 data execution gap by consuming BEIR plus
Wikipedia/arXiv/PubMed PPLX rows. Its broad-data direction is correct, but
later B/C work did not reproduce the sharp M150 C6 target.

M170A mixed a broad Stage-A manifest with stale M130 coverage/candidate rows.
That made it an unclear comparison. The 5,620-row auxiliary was too small and
too old to supervise a 150M-example Stage-A run.

## Stage A Data Contract

M180A starts from scratch and uses PPLX embeddings only:

```text
backbone: perplexity-ai/pplx-embed-v1-0.6B
input_dims: 1024
n_features: 16384
feature_k: 96
checkpoint source: none
```

Data sources:

- full official BEIR corpus documents;
- BEIR query-style rows with `qrels/test.tsv` query ids removed;
- neutral Wikipedia documents and query-style rows;
- neutral arXiv documents and query-style rows;
- neutral PubMed documents and query-style rows;
- optional external BEIR non-heldout/proxy rows from the PPLX source split.

M180A separates document repeat and query repeat. This is the key correction
from M170A. Neutral document repeat is useful because the neutral corpus is much
smaller than BEIR full-corpus documents. Neutral query repeat must be lower
because synthetic/query-style rows should not dominate retrieval-shaped BEIR
queries.

Initial effective ratio target:

| Group | Target |
| --- | ---: |
| BEIR docs | `68-74%` |
| BEIR non-test queries | `3.5-6.5%` |
| neutral docs | `18-26%` |
| neutral queries | `2-5%` |

Initial repeat policy:

| Source | Doc repeat | Query repeat |
| --- | ---: | ---: |
| BEIR official | `1` | `2` |
| Wikipedia | `16` | `8` |
| arXiv | `16` | `8` |
| PubMed | `16` | `8` |
| external BEIR nonheldout | `4` | `4` |

The training runner passes `--query-repeat 1` to the pretrain script. All query
exposure is encoded explicitly in the manifest. This avoids double-counting.

The training stream must use `combined_row_weighted`. The previous
document/query outer interleave is not valid for M180 because it can turn a
correct manifest into a query-heavy stream. M180 list files therefore include a
fourth `unique_rows` column, and the loader samples by remaining rows instead
of by file count or by document/query stream count.

## Stage A Objective

M180A uses the M150A1/M160A representation objective:

1. dense reconstruction;
2. cosine preservation;
3. in-batch dense-neighborhood KL/MSE;
4. query/document neighborhood preservation;
5. fanout/DF regularization.

The old coverage auxiliary is disabled in M180A. A new coverage surface will be
rebuilt only after M180A has a clean representation checkpoint.

## Coverage Auxiliary Redesign

The next auxiliary must be generated from current M180 data and current
checkpoint outputs. It must not reuse M130/M150 stale rows.

Required candidate sources:

- non-test qrel positives;
- BM25 topK;
- PPLX dense topK;
- BM25+dense topK;
- current SAE topK;
- current BM25+SAE topK;
- high-BM25 false positives.

Required row categories:

- `dense_hit_bm25_miss`;
- `bm25_only_positive`;
- `score_low_positive`;
- `high_bm25_false_positive`;
- `not_retrieved_by_controls`;
- `bm25_sae_hit`.

The auxiliary target for Stage A is SAE-only coverage of BM25-weak positives.
It must not train final BM25+SAE fusion. Final admission/ranking belongs to
Stage B/C.

Minimum surface requirements before enabling coverage in Stage A:

- PPLX `1024` dimensions only;
- no official test split in train rows;
- dataset/source metadata present on every row;
- full-corpus or streaming/sharded full-corpus topK, not sampled corpus;
- enough rows to diagnose per-dataset behavior, not a tiny 5k-row mixed
  surface.

## Stage Gates

M180A can advance only if all of these are true:

- manifest ratios match the target bands;
- embedding dimensions are exactly `1024`;
- no test query ids are present in BEIR query-style rows;
- dense-neighborhood metrics are not worse than M160A/M170A clean checkpoints;
- official full-corpus SAE-only gate does not materially regress against
  M160A;
- postings/fanout remain in the same order of magnitude as M160A.

If Stage A fails, fix manifest/source balance first. Do not start B/C repair
from a weak or unclear Stage-A base.

## Execution Notes

The first M180A formal run was stopped immediately after early events showed a
streaming bug: the manifest audit was correct, but the legacy pretrain loader
still interleaved documents and queries as two outer streams. That made the
actual training stream query-heavy despite a valid manifest.

The fix is now part of the M180 contract:

- manifest list files include `path`, `source`, `repeat`, and `unique_rows`;
- Stage-A uses `--stream-mode combined_row_weighted`;
- the loader samples specs by remaining row count, not by file count or by
  document/query stream count.

The row-weighted smoke on `spark-1` reached step `500` with actual training
ratios matching the manifest:

| Group | Manifest | Smoke Step 500 |
| --- | ---: | ---: |
| BEIR docs | `70.74%` | `70.89%` |
| BEIR non-test queries | `4.27%` | `4.24%` |
| neutral docs | `22.06%` | `21.95%` |
| neutral queries | `2.93%` | `2.92%` |

The formal M180A run is `ii42-m180a-clean-stagea-v1` on `spark-1`.
At step `5500`, it was still aligned with the manifest:

| Group | Manifest | Formal Step 5500 |
| --- | ---: | ---: |
| BEIR docs | `70.74%` | `70.78%` |
| BEIR non-test queries | `4.27%` | `4.26%` |
| neutral docs | `22.06%` | `22.04%` |
| neutral queries | `2.93%` | `2.92%` |

`spark-2` was checked as a fallback, but it is not a valid M180 training node
yet because its BEIR PPLX materialization is incomplete. It only has a small
subset of `.materialized_done` datasets, so using it would violate the M180
data contract.

## Stage B Indexed Row Contract

M180B is the next supervised BM25+SAE stage after the clean M180A
representation checkpoint. It must start from current M180 artifacts only:

```text
run: ii42-m180b-indexed-m150c6-v1
stage-a checkpoint: /home/huoju/leask/runs/ii42-m180a-clean-stagea-v1/bm25sae_stagea_latest.pt
materialized root: /home/huoju/leask/runs/m150-beir-full-pplx
official BEIR root: /home/huoju/leask/data/beir_official
candidate_k: 192
embedding dims: 1024
SAE features: 16384
```

The indexed package root is:

```text
/home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-indexed-roots
```

The merged training roots consumed by Stage B are:

```text
train: /home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-candidate-rows
validation: /home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-eval-candidate-rows
preflight: /home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-preflight/indexed_post_merge.json
ready marker: /home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-rows-ready
```

Each indexed dataset package must contain the following row-level artifacts:

| Artifact | Purpose |
| --- | --- |
| `rows/candidate_rows.jsonl` | Per-query candidate surface used by Stage B. |
| `rows/documents.jsonl` | Filtered document records referenced by candidates/qrels. |
| `rows/queries.jsonl` | Query records and PPLX query embeddings for the package. |
| `rows/quality_qrels.json` | Filtered qrel positives for the package query/doc set. |
| `rows/m130_candidate_rows_summary.json` | Package count summary. Historical filename, still used by the tooling. |
| `rows/indexed_stageb_rows_summary.json` | Indexed row-builder summary. |
| `eval/m110_full_corpus_rankings.jsonl` | Full-corpus ranking evidence used to build/debug the candidate surface. |
| `eval/m110_full_corpus_index_eval.json` | Full-corpus package-level evaluation summary. |

Each candidate row must expose the supervised BM25/dense/SAE surface, not just
query/doc ids. Required fields include:

| Field group | Required fields |
| --- | --- |
| Query and split metadata | `id`, `query_id`, `split`, `source_family`, `row_category`, `row_weight` |
| Candidate ids and labels | `candidate_doc_ids`, `labels` |
| BM25 signal | `bm25_scores`, `bm25_ranks` |
| Dense teacher signal | `dense_scores`, `dense_raw_scores`, `dense_ranks` |
| SAE signal | `sae_source_scores`, `sae_ranks` |
| Fusion supervision | `bm25_dense_scores`, `bm25_dense_ranks`, `bm25_sae_scores`, `bm25_sae_ranks` |

The candidate rows must include BM25 hits, dense hits, SAE hits, BM25+SAE hits,
rank-regression cases, dense-only representation gaps, high-BM25 false
positives, and positives lost by BM25+SAE fusion. This is the key correction
from the old tiny stale auxiliary surfaces.

## M180B Dataset Requirements

M180B uses a narrower direct training surface than M180A by design.
M180A is the broad representation stage and consumes full BEIR15 documents plus
neutral Wikipedia/arXiv/PubMed/external-BEIR rows. M180B is the supervised
BM25-complement stage and therefore only uses rows that can be grounded in
non-test qrels. External neutral rows do not have human relevance labels, so
they must not be mixed directly into the Stage-B ranking loss unless a separate
pseudo-label/self-distillation experiment is explicitly created.

The broad-data contribution to M180B is therefore indirect:

1. M180A trains the SAE representation on the broad manifest.
2. M180B starts from the M180A checkpoint.
3. M180B candidate rows are built from full-corpus BM25/dense/SAE surfaces on
   supervised non-test BEIR qrels only.

This is the intended separation. If the goal changes to pseudo-supervise
Wikipedia/arXiv/PubMed or BEIR test-only datasets, that must be a new M180B+
experiment with a separate contract and validation gate.

Current train merge spec:

```text
fiqa:train:0:0
nfcorpus:train:0:0
scifact:train:0:0
msmarco:train:6000:6000
hotpotqa:train:6000:6000
fever:train:6000:6000
dbpedia-entity:dev:6000:6000
```

Current validation merge spec:

```text
fiqa:dev:0:0
nfcorpus:dev:0:0
quora:dev:1500:1500
msmarco:dev:1200:1200
hotpotqa:dev:1200:1200
fever:dev:1200:1200
```

`dbpedia-entity:dev` is intentionally used as a small train-side package in
this run. A separate `validation/dbpedia-entity:dev` package is retained and
validated for auditability, but it is not part of the current merged validation
root. If a later experiment needs dbpedia validation rows, the validation merge
spec must be changed explicitly and the merged validation root must be rebuilt.

Per-package requirements and verified counts:

| Role | Dataset | BEIR qrels split | Query cap / pool | Rows | Docs | Queries | Qrel labels | Merged into Stage B |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| Train | `fiqa` | `train` | `0 / 0` | `5500` | `54563` | `5500` | `14166` | Yes |
| Train | `nfcorpus` | `train` | `0 / 0` | `2590` | `3633` | `2590` | `110575` | Yes |
| Train | `scifact` | `train` | `0 / 0` | `809` | `5177` | `809` | `919` | Yes |
| Train | `msmarco` | `train` | `6000 / 6000` | `6000` | `998331` | `6000` | `6384` | Yes |
| Train | `hotpotqa` | `train` | `6000 / 6000` | `6000` | `802184` | `6000` | `12000` | Yes |
| Train | `fever` | `train` | `6000 / 6000` | `6000` | `557364` | `6000` | `7588` | Yes |
| Train | `dbpedia-entity` | `dev` | `6000 / 6000` | `67` | `12721` | `67` | `1405` | Yes |
| Validation | `fiqa` | `dev` | `0 / 0` | `500` | `33402` | `500` | `1238` | Yes |
| Validation | `nfcorpus` | `dev` | `0 / 0` | `324` | `3633` | `324` | `11385` | Yes |
| Validation | `quora` | `dev` | `1500 / 1500` | `1500` | `198814` | `1500` | `2309` | Yes |
| Validation | `msmarco` | `dev` | `1200 / 1200` | `1200` | `220754` | `1200` | `1282` | Yes |
| Validation | `hotpotqa` | `dev` | `1200 / 1200` | `1200` | `208760` | `1200` | `2400` | Yes |
| Validation | `fever` | `dev` | `1200 / 1200` | `1200` | `154491` | `1200` | `1478` | Yes |
| Validation audit | `dbpedia-entity` | `dev` | `1200 / 1200` | `67` | `12721` | `67` | `1405` | No, package retained only |

BEIR15 coverage audit:

| Dataset | Qrels available locally | Direct M180B supervised use | Reason |
| --- | --- | --- | --- |
| `arguana` | `test` | No | Test-only qrels; held out from training. |
| `climate-fever` | `test` | No | Test-only qrels; held out from training. |
| `cqadupstack` | `test` | No | Test-only qrels; held out from training. |
| `dbpedia-entity` | `dev`, `test` | Train package from `dev`; validation package retained for audit | No train split; `dev` is used as limited supervision. |
| `fever` | `train`, `dev`, `test` | Train and validation | Non-test qrels available. |
| `fiqa` | `train`, `dev`, `test` | Train and validation | Non-test qrels available. |
| `hotpotqa` | `train`, `dev`, `test` | Train and validation | Non-test qrels available. |
| `msmarco` | `train`, `dev`, `test` | Train and validation | Non-test qrels available. |
| `nfcorpus` | `train`, `dev`, `test` | Train and validation | Non-test qrels available. |
| `nq` | `test` | No | Test-only qrels; held out from training. |
| `quora` | `dev`, `test` | Validation only | No train split; dev is kept for validation in current spec. |
| `scidocs` | `test` | No | Test-only qrels; held out from training. |
| `scifact` | `train`, `test` | Train only | No dev split in local BEIR qrels. |
| `trec-covid` | `test` | No | Test-only qrels; held out from training. |
| `webis-touche2020` | `test` | No | Test-only qrels; held out from training. |

M180A broad manifest coverage, already baked into the checkpoint used by
M180B:

| Group | Unique rows | Effective rows after repeat |
| --- | ---: | ---: |
| BEIR15 full documents | `33860456` | `33860456` |
| BEIR non-test query-style rows | `1021158` | `2042316` |
| Neutral documents | `993336` | `10559646` |
| Neutral query-style rows | `205059` | `1401125` |

The neutral sources include Wikipedia, arXiv, PubMed,
`beir15_non_heldout_corpus`, and smaller BEIR-derived neutral shards. These
sources are not lost; they affect M180B through the M180A SAE checkpoint, not
through direct Stage-B qrel loss.

Merged root requirements and verified counts:

| Root | Rows | Documents | Queries | Qrel labels | Candidate positives in top-K |
| --- | ---: | ---: | ---: | ---: | ---: |
| Train merged root | `26966` | `2433973` | `26966` | `153037` | `127775` |
| Validation merged root | `5924` | `819854` | `5924` | `20092` | `18706` |

`Qrel labels` is the filtered qrel label count in `quality_qrels.json`.
`Candidate positives in top-K` is the number of positive labels actually
present inside the candidate rows. It is expected to be smaller than the full
filtered qrel count because not every qrel positive is admitted into the
candidate surface.

## M180B Readiness Check

The latest Spark-1 live validation after the machine reboot confirmed:

- Spark-1 reachable through Tailscale direct path;
- all `14 / 14` indexed packages pass `validate_packages.py`;
- direct line counts match every package requirement in the table above;
- merged train root has exactly `26966` candidate rows;
- merged validation root has exactly `5924` candidate rows;
- merged roots include filtered `documents.jsonl`, `queries.jsonl`, and
  `quality_qrels.json`;
- preflight reports `ok=true`, `expected_dim=1024`,
  `expected_features=16384`, and `errors=[]`;
- `source_family` metadata has been normalized after merge, for example
  `ii42-m180b-indexed-m150c6-v1:train:fiqa:train`;
- original pre-normalization merged row files are retained as
  `candidate_rows.jsonl.before_source_family_normalize` in both merged roots.

Readiness verdict: the current M180B indexed data is ready for the next Stage B
training step under the current merge spec. The only policy decision left
before starting is whether to keep `dbpedia-entity:dev` train-only, as the
current spec does, or explicitly add the retained dbpedia validation package to
the validation merge and rerun the merge/preflight.

## M180B Indexed Stage-B Training Parameters

The prepared training job is:

```text
job: /home/huoju/leask/runs/ii42-m180b-indexed-stageb-v1-job/host_job.sh
run: ii42-m180b-indexed-stageb-v1
ClearML project: ii42/M180
ClearML task: ii42-m180b-indexed-stageb-v1
```

It intentionally reuses the verified indexed rows instead of rebuilding them:

| Parameter | Value |
| --- | --- |
| `REUSE_EXISTING_ROWS` | `1` |
| `TRAIN_ROWS` | `/home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-candidate-rows` |
| `EVAL_ROWS` | `/home/huoju/leask/runs/ii42-m180b-indexed-m150c6-v1-eval-candidate-rows` |
| `OUTPUT_DIR` | `/home/huoju/leask/runs/ii42-m180b-indexed-stageb-v1` |
| `STAGE_A_CKPT` | `/home/huoju/leask/runs/ii42-m180a-clean-stagea-v1/bm25sae_stagea_latest.pt` |
| `CANDIDATE_K` | `192` |
| `DENSE_TOP_K` / `BM25_TOP_K` / `SAE_TOP_K` / `SOURCE_TOP_K` | `500 / 500 / 500 / 500` |
| `SAE_DOC_ACTIVE_K` / `SAE_QUERY_ACTIVE_K` | `96 / 96` |
| `FEATURE_K` | `96` |
| `RETRIEVAL_K` | `100` |
| `FUSION_BM25_MODE` | `residual` |
| `STEPS` | `12000` |
| `BATCH_SIZE` | `8` |
| `LR` | `2.0e-5` |

Current loss configuration:

| Loss/control | Value |
| --- | ---: |
| `LOSS_RECALL` | `1.35` |
| `LOSS_SINGLE_CE` | `0.15` |
| `LOSS_MULTI_CE` | `1.00` |
| `LOSS_TEACHER_KL` | `0.65` |
| `LOSS_COMPLEMENT` | `0.75` |
| `LOSS_FINAL_HARD_PAIRWISE` | `0.80` |
| `LOSS_SAE_HARD_PAIRWISE` | `0.80` |
| `LOSS_ROW_WEIGHTED_ADMISSION` | `1.00` |
| `LOSS_BM25_FALSE_POSITIVE` | `0.50` |
| `LOSS_SAE_LOST_PRESERVATION` | `0.80` |
| `LOSS_RECONSTRUCTION` | `0.02` |
| `LOSS_K_BUDGET` | `0.0` |
| `LOSS_SCALE_REGULARIZATION` | `0.03` |

Parameter readiness verdict:

- the job uses the verified indexed roots and will not rebuild rows;
- dimensions, candidate length, checkpoint path, and row schema match the
  M180B preflight contract;
- the configuration is a supervised Stage-B continuation, not another Stage-A
  broad-data run;
- after this training job finishes, official BEIR15 full-corpus evaluation is
  still required before promotion. The inherited continuity eval is useful for
  regression tracking, but it does not replace the official full BEIR15 gate.
