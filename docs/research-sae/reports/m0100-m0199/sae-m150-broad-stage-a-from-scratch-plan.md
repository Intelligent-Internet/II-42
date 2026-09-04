# SAE M150 Broad Stage-A From-Scratch Plan

Date: 2026-05-25

## Summary

M150 resets Stage A from zero. It does not inherit any M130/M128/M52
checkpoint. The goal is to train a broader PPLX-space sparse representation
that preserves dense semantic neighborhoods across the official BEIR full
corpora and high-volume neutral corpora before any Stage B ranking calibration.

The reason for this reset is concrete: M130 Stage A used about `993k` document
rows, but only `200k` rows came from the BEIR full-corpus family. The current
official BEIR canary results show that `SAE` standalone recall still trails
`dense`, especially on hard domains such as `trec-covid`, `fiqa`, and
`scidocs`. That pattern points first to representation and dense-neighborhood
retention, not only final BM25/SAE score calibration.

M150 therefore starts with a larger and less conservative Stage A surface:

- all official BEIR corpus documents;
- no official test qrel labels in training;
- high-volume Wikipedia, arXiv, and PubMed neutral documents;
- safe query-style rows from train/dev queries and synthetic document-derived
  prompts;
- PPLX dense-neighborhood preservation as the primary semantic target;
- fanout/DF control as a hard physical constraint.

The active run family is:

```text
bm25sae-m150-pplx16384k96-stagea-*
```

## Core Principle

Use all corpus documents, but protect relevance labels.

For Stage A, including official BEIR corpus documents is acceptable because a
production retrieval system must encode its corpus. What must stay isolated are
official test query/qrel relevance signals. M150 can use:

- all BEIR corpus documents;
- BEIR train/dev qrel queries where available;
- synthetic query-style text generated from documents;
- held-out official test queries only for evaluation, not training.

## What M150 Changes From M130

| Area | M130 | M150 |
| --- | --- | --- |
| Checkpoint | Stage B/C continue from M130 Stage A | Stage A starts from random initialization |
| BEIR corpus exposure | `200k` BEIR-derived document rows | full official BEIR corpus documents |
| Neutral corpus | about `200k` each from Wikipedia/arXiv/PubMed | high-volume neutral supplement, sampled every epoch |
| Query ratio | about `17%` training examples were query rows | target `25-35%` query-style rows |
| Sampling | mostly file-order interleaving with shuffle buffer | source-balanced/sqrt-size sampling |
| Objective | reconstruction + local neighborhood + fanout | dense-neighborhood preservation + coverage + fanout |
| Gate | M130 held-out all-data gate | official BEIR representative gate first, then full15 |

## Data Plan

### M150.0 Inventory

Build an inventory before embedding:

- per-dataset official BEIR document counts;
- per-dataset query/qrel split counts;
- existing PPLX embedding coverage;
- neutral corpus availability for Wikipedia/arXiv/PubMed;
- estimated embedding storage and wall time.

This inventory decides shard sizes and launch order. It does not change the
training objective.

### M150.1 Full BEIR Corpus Materialization

Materialize PPLX embeddings for every official BEIR corpus document.

Rules:

- keep each dataset in its own shard directory;
- do not merge into one huge JSONL during embedding;
- use stable IDs: `beir15:<dataset>:d:<raw_id>`;
- keep raw text and metadata for taxonomy;
- use the same text truncation policy across dense baselines and SAE training;
- reuse completed canary embeddings only if their truncation/model settings
  exactly match the canonical M150 policy.

Initial canonical embedding policy:

```text
model: perplexity-ai/pplx-embed-v1-0.6B
dimension: 1024
normalize: false
similarity: cosine
max_seq_length: 512
max_text_chars: 6000
dtype: fp32
attention: sdpa
```

The truncation policy is chosen for reproducibility and long-document stability.
If later evidence shows TREC-COVID quality is truncation-limited, run a named
long-context ablation. Do not silently mix policies.

M150 runtime note, recorded on 2026-05-26: Spark has both `flash_attn`
`2.7.4.post1` and `torch.compile` available in the NVIDIA PyTorch 26.03
container. A controlled climate-fever embedding benchmark found:

| Mode | Rows/s | Decision |
| --- | ---: | --- |
| `sdpa fp32` current policy | `86-119` | Keep for the active M150 materialization |
| `sdpa fp32 + torch.compile` | `117` | Reject; no material speedup and higher memory |
| `flash_attention_2 fp32` | failed | FlashAttention does not support fp32 |
| `sdpa bf16` | `147` | Candidate for next clean materialization only |
| `flash_attention_2 bf16` | `226-295` | Best next-run candidate |

The active M150 materialization must not switch to bf16/flash mid-run because
that would mix embedding distributions inside the same training surface.
`flash_attention_2 + bf16` should be the default candidate for the next full
materialization run after a clean from-zero rebuild or a new corpus shard with
an explicit policy label.

### M150.2 Neutral Corpus Expansion

Add a large neutral supplement:

- Wikipedia for entities and broad natural language;
- arXiv for scientific/technical language;
- PubMed for biomedical language.

Neutral rows should be large enough to prevent BEIR overfitting. The exact
stored row count depends on available disk and embedding throughput, but the
training sampler should target a high neutral exposure even if raw BEIR has
more documents.

Initial target exposure per epoch:

| Group | Target exposure |
| --- | ---: |
| BEIR full corpus docs | 45% |
| Wikipedia docs | 15% |
| arXiv docs | 15% |
| PubMed docs | 15% |
| Query-style rows | 10% additive or interleaved to reach 25-35% total query rows |

Implementation detail: the sampler may use source weights instead of physically
duplicating rows.

### M150.3 Query-Style Rows

Stage A should see query-shaped language, but must not train on official test
qrel labels.

Allowed query rows:

- BEIR train/dev queries where available;
- synthetic prompts generated from document title/abstract/body snippets;
- prior neutral synthetic queries from arXiv/PubMed/Wikipedia;
- dense-neighborhood pseudo queries if they do not include test qrel labels.

Disallowed:

- official test qrel labels;
- any candidate row whose positive/negative labels come from held-out test
  qrels;
- using test-query outcome to tune Stage A checkpoint selection.

## Training Plan

### M150.4 Stage A From Scratch

Primary config:

```text
run_name: bm25sae-m150-pplx16384k96-stagea-v1
teacher/backbone: perplexity-ai/pplx-embed-v1-0.6B
input_dims: 1024
n_features: 16384
feature_k: 96
batch_size: start 384 on Spark, adjust only for stability
epochs: at least 3 broad epochs, extend if validation still improves
checkpoint source: none
```

Loss components:

1. Dense reconstruction and cosine preservation.
2. In-batch dense-neighborhood KL/MSE.
3. Query/document neighborhood preservation with query-shaped rows.
4. Dense-hit/SAE-miss coverage rows from official BEIR canaries.
5. Fanout/DF regularization with warmup, so useful high-DF atoms are not
   killed before the model learns semantic structure.

### M150.4.1 Stage A Objective Revision

The first broad M150 Stage A run proved that a larger neutral corpus improves
the dense-retention objective but does not automatically improve product
recall after BM25+SAE fusion.

Observed same-surface Stage A deltas versus M130:

| Signal | M130 Stage A v2 | M150 Stage A | Interpretation |
| --- | ---: | ---: | --- |
| training examples | `3.60M` | `107.71M` | broader corpus worked |
| neighbor overlap | `0.8586` | `0.9445` | dense-neighborhood retention improved |
| SAE Recall@100 | `0.2335` | `0.2404` | average SAE recall improved |
| BM25+SAE Recall@100 | `0.2832` | `0.2799` | fusion recall regressed |
| SAE postings/query | `825k` | `778k` | representation became more compact |

The failure mode is therefore not "Stage A did not learn dense shape". It is
that the learned shape is not sufficiently aligned with BM25 complementarity.
M150 improves query-average SAE recall while losing total relevant hits on
multi-positive queries, especially in `nfcorpus` and biomedical-style query
families. Fixed BM25+SAE fusion then pushes some SAE-only positives out of
top-100.

Revised Stage A objective:

1. Keep dense reconstruction, cosine preservation, and in-batch
   dense-neighborhood KL/MSE as the base representation objective.
2. Add a lightweight candidate-surface coverage auxiliary when safe train
   candidate rows are available.
3. In that auxiliary, optimize SAE-only retrieval coverage, not final
   BM25+SAE ranking. Stage B remains responsible for final fusion ranking.
4. Weight positive rows by BM25 weakness:
   positives already covered well by BM25 get less pressure, while positives
   that BM25 scores poorly get stronger SAE coverage pressure.
5. Keep dense teacher KL inside the auxiliary so the model does not turn qrels
   into a narrow supervised overfit signal.
6. Select checkpoints using both dense-neighborhood retention and candidate
   coverage diagnostics. Do not choose a checkpoint by neighbor overlap alone.

Implementation status:

- `scripts/research_sae_m130_bm25sae_stage_a_pretrain.py` now supports
  optional `--coverage-train-root` candidate rows.
- Coverage loss is off by default and only activates when both a root and
  non-zero coverage weights are supplied.
- `scripts/run_m150_broad_stage_a_spark.sh` enables the auxiliary when the
  standard M130 candidate-row roots are present on Spark.

Canonical M150 Stage A v2 coverage defaults:

```text
--coverage-candidate-k 80
--coverage-batch-size 24
--coverage-every 4
--coverage-start-frac 0.05
--weight-coverage-recall 0.05
--weight-coverage-complement-ce 0.10
--weight-coverage-dense-kl 0.02
```

These values are intentionally conservative. Stage A should learn a
BM25-complement-aware sparse semantic space, not memorize a small candidate
surface. If full-corpus evaluation shows `SAE Recall@100` rises but
`BM25+SAE Recall@100` still falls, the next correction is per-query fusion
calibration in Stage B, not a stronger Stage A supervised loss.

Model selection should not use a single aggregate loss. It should rank
checkpoints by:

- dense-neighborhood overlap;
- official BEIR representative SAE-vs-dense gap;
- atom DF/fanout distribution;
- no collapse on `trec-covid`, `fiqa`, and `scidocs`.

## Evaluation Gates

### Stage-A Gate

Before Stage B, evaluate `SAE` standalone and `BM25+SAE` on representative
official BEIR full-corpus surfaces:

- `trec-covid`;
- `fiqa`;
- `scidocs`;
- one of `nq` or `hotpotqa`;
- `msmarco`.

Pass conditions:

- `SAE R@100` gap vs `dense` shrinks materially on `trec-covid`, `fiqa`, and
  `scidocs`;
- `BM25+SAE R@100` gets close to or exceeds `BM25+dense` on at least the first
  three representative datasets;
- postings/query does not rise materially above the M130 `doc64/query80`
  balanced profile without a clear quality gain;
- no single representative dataset collapses.

### Full15 Gate

Only after representative datasets pass:

- run official BEIR full15 all-test matrix;
- run strict-heldout matrix where train/dev query labels were used;
- compare `BM25s`, `BM25s+dense`, and `BM25s+SAE` on the same corpus, qrels,
  top-k, truncation, and metric surface.

## Execution Order

1. Stop old M130 canary/queue jobs. Freeze completed M130 canary results as
   diagnostics, not final verdict.
2. Build M150 full-corpus inventory.
3. Materialize PPLX embeddings for full BEIR in per-dataset shards.
4. Materialize or reuse neutral Wikipedia/arXiv/PubMed shards under the same
   PPLX/truncation policy.
5. Build a source manifest for the Stage A sampler.
6. Train `bm25sae-m150-pplx16384k96-stagea-v1` from scratch.
7. Run representative official BEIR Stage-A gate.
8. Only if Stage A passes, move to M151 Stage-B BM25-aware ranking refresh.

## Non-Goals

- No Stage B until Stage A standalone gap improves.
- No inherited checkpoint.
- No product SQL/API work.
- No claim that dense retrieval can be removed until full official BEIR gates
  pass.
- No final quality claim from query-sampled canaries.

## Immediate Engineering Tasks

- Full-BEIR inventory is complete at
  `/home/huoju/leask/runs/m150-stage-a-inventory.json`.
- Add a materialized-shard manifest that validates per-dataset row counts,
  roots, and PPLX policy before Stage A training.
- Add a Stage A source-manifest sampler or generate weighted training shards.
  This is now a hard pre-training blocker because BEIR materialization is
  sharded by dataset and split across Spark/HomeAI roots.
- Validate that the Stage A runner does not consume an old single-file
  M130-style corpus. It must read the M150 shard manifest or a verified merged
  M150 corpus with matching PPLX/truncation policy.
- Add a Spark runner for `bm25sae-m150-pplx16384k96-stagea-v1`.
- Keep old M130 canary outputs separate. Do not continue the old queue, and do
  not mix canary outputs into M150 training labels.

## Live Execution Notes

Current materialization status as of the latest review:

- Spark root `/home/huoju/leask/runs/m150-beir-full-pplx` has completed
  `trec-covid`, `nq`, `cqadupstack`, `webis-touche2020`, and
  `dbpedia-entity`; it is actively materializing `hotpotqa`.
- HomeAI root `/home/huoju/leask/runs/m150-beir-full-pplx-homeai` has completed
  `arguana`, `fiqa`, `nfcorpus`, `quora`, `scidocs`, `scifact`, and
  `dbpedia-entity`; GPU1 is actively materializing `msmarco`, while GPU0 must
  remain untouched.
- Asa root `/home/leask/dev/runs/m150-beir-full-pplx-asa` is actively
  materializing `fever` on the new RTX 3090 VM.
- xiaoni-mbp is actively materializing `climate-fever` with MPS.

The next safe action is not training. It is to finish enough materialization,
then build the manifest/merge layer that defines the broad Stage A training
surface.

Mac-side prepare workers are now part of the execution plan. They do not create
PPLX embeddings and therefore do not change the canonical embedding policy;
they only pre-generate `documents.input.jsonl`, `queries.input.jsonl`, and
prepare summaries for later GPU-host embedding. Spark will use
`SKIP_PREPARE_IF_EXISTS=1` for remaining datasets so these prepared shards are
reused instead of recomputed.
