# SAE M150 Execution Control Plan

Date: 2026-05-26

## Objective

Execute M150 without drifting back into M130-style small-surface experiments.
The only active objective is:

```text
Stage A from scratch -> broad PPLX dense-neighborhood retention -> close SAE
standalone gap to dense before Stage B.
```

## Current State

Stopped legacy M130 canary/queue jobs before starting M150 execution. Completed
M130 canary outputs remain diagnostic artifacts only.

M150 inventory completed on Spark:

```text
/home/huoju/leask/runs/m150-stage-a-inventory.json
```

Key inventory totals:

| Item | Count |
| --- | ---: |
| Official BEIR datasets | 15 |
| Official BEIR corpus documents | 33,860,495 |
| Official BEIR queries | 778,054 |
| Train qrel queries | 706,648 |
| Dev qrel queries | 24,984 |
| Test qrel queries | 46,422 |

Materialization is active and remains on the M150 route. Current progress:

| Host | Root | State |
| --- | --- | --- |
| Spark | `/home/huoju/leask/runs/m150-beir-full-pplx` | `trec-covid`, `nq`, `cqadupstack`, `webis-touche2020`, and `dbpedia-entity` complete; `hotpotqa` running |
| HomeAI | `/home/huoju/leask/runs/m150-beir-full-pplx-homeai` | `arguana`, `fiqa`, `nfcorpus`, `quora`, `scidocs`, `scifact`, and `dbpedia-entity` complete; GPU1 embedding `msmarco`; GPU0 remains untouched |
| Asa | `/home/leask/dev/runs/m150-beir-full-pplx-asa` | RTX 3090 VM added for M150; `fever` input synced and GPU materialization job launched |
| Flora | `/Volumes/Betty/Tmp/m150-beir-embedded-local` | local MPS `fever` was stopped after Asa took ownership; local MPS queue excludes `fever` to avoid duplicate restarts |
| xiaoni-mbp | `/Users/leask/Tmp/m150-beir-embedded-xiaoni` | MPS embedding `climate-fever`; `webis-touche2020` complete |

Latest checked large-shard progress:

```text
hotpotqa on Spark:       3,509,220 / 5,233,329 documents
msmarco on HomeAI GPU1:  1,771,567 / 8,841,823 documents
fever on Asa:            180,815 / 5,416,568 documents
climate-fever on xiaoni: 262,474 / 5,416,593 documents
```

HomeAI GPU1 is allowed for M150 only when free. GPU0 must remain untouched.
`dbpedia-entity` has completed on HomeAI and Spark.
HomeAI GPU1 is currently assigned to `msmarco`. HomeAI does not have the
private `leask/ii42-sae:pytorch26.03` image, so GPU materialization
jobs there use `nvcr.io/nvidia/pytorch:26.03-py3` and install the embedding
Python dependencies in the container before running the shard.

Mac-side prepare acceleration is enabled:

| Host | Session | Work |
| --- | --- | --- |
| Flora | `m150_prepare_inputs_local` | prepare `msmarco`, `fever`, `climate-fever` input shards |
| Flora | `m150_sync_prepared_local_to_spark` | sync completed local input shards to Spark |
| xiaoni-mbp | `m150_prepare_inputs_xiaoni` | prepare `hotpotqa`, `dbpedia-entity`, `cqadupstack`, `webis-touche2020` input shards |
| Flora | `m150_sync_prepared_xiaoni_to_spark` | stage xiaoni shards through local disk, then sync to Spark |

Mac-side MPS embedding acceleration is also enabled for smaller shards that
would otherwise wait behind large Spark jobs:

| Host | Session | Work |
| --- | --- | --- |
| Flora | `m150_embed_cqadupstack_local_mps` | embed `cqadupstack`, then sync to Spark |
| xiaoni-mbp | `m150_embed_webis_touche2020_xiaoni_mps` | embed `webis-touche2020`, then sync to Spark |
| Asa | `bm25sae_m150_materialize_fever` | embed `fever` on the new RTX 3090 VM, then sync to Spark |
| Flora | `m150_sync_cqadupstack_to_spark` | wait for local MPS completion and publish to Spark root |
| Flora | `m150_sync_webis_from_xiaoni_to_spark` | wait for xiaoni MPS completion and publish to Spark root |
| Flora | `m150_sync_dbpedia_from_homeai_to_spark` | completed `dbpedia-entity` sync from HomeAI to Spark |
| Flora | `m150_autopilot` | every 10 minutes, ensure active watchers exist, restart missing safe watchers, and snapshot progress |

Generic continuation watchers are now disabled. After introducing explicit
Spark, HomeAI, xiaoni, and Asa assignments, legacy generic continuation
watchers were stopped because they could duplicate already assigned work. In
particular, the old Spark continuation must not auto-start `msmarco` or
`fever`, and the old HomeAI dbpedia watcher must not rerun `dbpedia-entity`
after the dataset has already materialized.

`climate-fever` is the current slowest shard on xiaoni MPS. A dedicated
watcher, `m150_spark_takeover_climate_after_hotpotqa`, waits for Spark to
finish `hotpotqa`, then stops the xiaoni MPS job, copies the partial
`climate-fever/documents.jsonl` to the Spark root, and starts Spark
materialization with `SKIP_PREPARE_IF_EXISTS=1` so the job resumes rather than
starting from zero.

Important execution correction: materialization is intentionally sharded by
dataset and currently split across two roots. The Stage A runner must not fall
back to a single small `documents.jsonl`/`queries.jsonl` corpus. Before Stage A
starts, build either a source manifest or a verified merged training surface
from all completed shards.

## Autopilot Timer

The unattended maintenance timer is:

```text
tmux session: m150_autopilot
script: scripts/run_m150_autopilot_local.sh
interval: 600 seconds
log: results/sae/m150/m150_autopilot.log
runner log: results/sae/m150/m150_autopilot_runner.log
```

The autopilot is deliberately conservative. It does not kill active embedding
jobs. Each cycle:

- verifies explicit Spark/HomeAI/Asa/xiaoni assignments and sync watchers;
- restarts missing watcher sessions when the corresponding shard is not marked
  complete;
- keeps the local MPS worker on a conservative queue. `fever` has been removed
  from this queue after assignment to Asa, so the local timer will not restart
  a duplicate `fever` MPS job;
- keeps xiaoni available for independent MPS shards; current assignment is
  `climate-fever`;
- keeps HomeAI GPU1 on `msmarco` and leaves GPU0 untouched;
- keeps the new Asa RTX 3090 assignment alive for `fever`, including input
  transfer, remote materialization, and sync-back to Spark;
- snapshots Spark counts, local sessions, remote sessions, and HomeAI GPU
  state;
- reports recent active-log error markers.

This keeps materialization moving without silently changing the M150 plan or
stealing HomeAI GPU0.

## Non-Drift Rules

1. No inherited checkpoint. M150 Stage A starts from random initialization.
2. No final claim from canary/sample query results.
3. No Stage B until SAE standalone gap to dense improves on representative
   official BEIR datasets.
4. No test qrel labels in training.
5. No shrinking BEIR corpus back to `200k` rows.
6. No mixed embedding policy. PPLX truncation/model settings must match across
   dense baselines, Stage A training rows, and representative gates.
7. No silent infrastructure fallback. If embedding/training fails, fix the
   blocker and resume the same plan.
8. Long training must log to ClearML under project `ii42_sae/M150`.

## Materialization Contract

Root:

```text
/home/huoju/leask/runs/m150-beir-full-pplx
```

This Spark root is the canonical consolidated root. HomeAI, Asa, xiaoni, and
local Mac roots are temporary worker roots; completed shards must sync back to
the Spark root with the same file contract and `.materialized_done` marker.

Per dataset:

```text
<root>/<dataset>/documents.input.jsonl
<root>/<dataset>/queries.input.jsonl
<root>/<dataset>/documents.jsonl
<root>/<dataset>/queries.jsonl
<root>/<dataset>/m150_prepare_summary.json
<root>/<dataset>/.materialized_done
```

Document rows:

- include every official BEIR corpus document;
- stable id: `beir15:<dataset>:d:<raw_id>`;
- text policy: title + body, capped by canonical embedding truncation;
- metadata includes `dataset`, `raw_id`, and `m150_source`.

Query rows:

- include train/dev qrel queries where available;
- exclude official test qrel-only queries;
- add synthetic document-derived prompts where train/dev query coverage is too
  small;
- stable id: `beir15:<dataset>:q:<raw_id>` for official train/dev queries;
- synthetic ids use `beir15:<dataset>:synq:<n>`.

Canonical PPLX policy:

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

Embedding acceleration check, 2026-05-26:

- Spark's active NVIDIA PyTorch 26.03 container has `flash_attn 2.7.4.post1`
  and `torch.compile` available.
- `torch.compile` on the current fp32/sdpa `SentenceTransformer.encode()` path
  did not improve measured throughput and increased peak memory, so it is not
  used for M150.
- `flash_attention_2` cannot run with fp32 PPLX weights. With bf16 it was much
  faster, but bf16 changes the embedding distribution slightly and must not be
  mixed into the active fp32/sdpa M150 shards.
- Decision: keep the current M150 materialization unchanged. For the next clean
  materialization generation, use an explicit new embedding policy such as
  `pplx-v1-0.6b-bf16-flash2` and rebuild from zero instead of resuming into the
  fp32 shards.

## Stage A Training Contract

Run:

```text
bm25sae-m150-pplx16384k96-stagea-v1
```

Training starts only after the M150 corpus root contains enough materialized
rows for a broad epoch. The preferred final training surface is all M150
materialized rows plus neutral Wikipedia/arXiv/PubMed shards.

The runner must use:

```text
--run-name bm25sae-m150-pplx16384k96-stagea-v1
--n-features 16384
--feature-k 96
--query-repeat >= 2
--clearml-project ii42_sae/M150
```

Objective update after the first M150/M130 same-surface comparison:

- Stage A must not optimize dense-neighborhood overlap alone. M150 improved
  overlap and SAE-only average Recall@100, but BM25+SAE Recall@100 regressed.
- The corrected objective keeps dense-retention as the base loss and adds a
  conservative BM25-complement coverage auxiliary when candidate rows are
  available.
- The auxiliary trains SAE-only coverage for qrel positives that BM25 scores
  weakly. It does not train the final BM25+SAE ranker; that remains Stage B.
- Checkpoint review must include the Stage A recall decomposition:
  `SAE-only`, `BM25+SAE`, relevant-hit totals, qrel-count buckets, and
  dataset-level deltas.

The Spark runner now enables the auxiliary automatically if the standard
candidate-row roots exist:

```text
--coverage-train-root $RUNS_ROOT/m130-pplx-all-data-candidate-rows
--candidate-eval-root $RUNS_ROOT/bm25sae-pplx-all-data-eval-candidate-rows
--coverage-every 4
--weight-coverage-recall 0.05
--weight-coverage-complement-ce 0.10
--weight-coverage-dense-kl 0.02
```

The current shell runner still expects one `documents.jsonl` and one
`queries.jsonl`. That is not sufficient for the actual M150 materialization
layout. Update the runner before training so it consumes a manifest of
per-dataset shards, or generate a checked merged corpus with row-count and
policy validation.

Checkpoint selection must include:

- dense-neighborhood overlap;
- atom DF/fanout distribution;
- representative official BEIR SAE-vs-dense gap;
- no collapse on `trec-covid`, `fiqa`, and `scidocs`.

## Execution Steps

1. Stop M130 legacy queue and GPU jobs.
2. Run `research_sae_m150_stage_a_inventory.py`.
3. Start full BEIR materialization queue.
4. While materialization runs, build the Stage A source-manifest sampler.
5. Add neutral Wikipedia/arXiv/PubMed shards under the same PPLX policy.
6. Validate every shard policy and row count before training.
7. Start Stage A from scratch with ClearML tracking.
8. Run representative Stage-A gate.
9. Write M150 results report.

## First Representative Gate

Evaluate before Stage B:

- `trec-covid`;
- `fiqa`;
- `scidocs`;
- `nq` or `hotpotqa`;
- `msmarco`.

Report:

- `dense`, `SAE`, `BM25`, `BM25+dense`, `BM25+SAE`;
- Recall@20/100, MRR@20, NDCG@10, MAP@100;
- SAE postings/query, accumulator entries/query, elapsed/query;
- dense-hit/SAE-miss taxonomy.

## Stop Conditions

Stop and debug before continuing if:

- a materialized shard uses different PPLX/truncation settings;
- test qrel labels enter training rows;
- Stage A starts from a non-random checkpoint;
- Stage A starts from only one shard or one old merged M130-style corpus;
- ClearML logging fails for long training;
- SAE postings explode without representative quality gain.
