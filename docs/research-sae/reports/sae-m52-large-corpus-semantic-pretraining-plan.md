# SAE M52 Large-Corpus Semantic Pretraining And Supervised Ranking Plan

Status: active.

## Execution Update: 2026-05-19

M52 is now split into an evidence-backed pipeline rather than a single
training command:

1. `xiaoni-mbp.local` has the BEIR15 shared artifacts, BEIR official data, and
   a fresh arXiv/PubMed commons sample staged under
   `/Users/leask/Tmp/ii42_sae_data`.
2. The first M52A shard validated the end-to-end teacher materialization and
   student training path, but arXiv/PubMed coverage was too small in the pilot
   dump. It produced `47,992` docs and `11,998` queries.
3. The arXiv/PubMed dump has been regenerated with PostgreSQL JSON output
   instead of text-mode `COPY`, because text-mode `COPY` escaped backslashes
   and produced invalid JSONL for LaTeX-heavy rows.
4. A balanced no-Wikipedia 100k shard is now the stable Stage-A corpus:
   `79,999` docs and `20,001` queries, split evenly across arXiv, PubMed, and
   BEIR15 non-heldout corpus. Wikipedia will be added only after a local JSONL
   is staged; Hugging Face streaming left Arrow shutdown threads hanging on
   macOS and is not reliable enough for the main builder.
5. The training loop now has a teacher-fidelity evaluator and a vectorized
   support-ranking loss. The first support-ranking smoke confirmed the
   bottleneck is teacher atom support selection, not active-value regression.
6. The plain M52D pretrain on the balanced no-Wikipedia 100k shard completed
   successfully and improved teacher-support fidelity versus the first M52A
   pilot. This makes broader neutral corpus coverage the current positive
   direction.
7. The next no-Wikipedia medium shard, M52E, has been built and teacher
   materialization and the follow-up plain pretrain have started on
   `xiaoni-mbp.local`. The shard has `597,754` docs and `149,438` queries. It
   uses almost all currently staged arXiv and PubMed rows plus a 250k BEIR
   sample.
8. Spark/GB10 is prepared as the monitored GPU host for follow-up runs. The
   training image is derived from NVIDIA PyTorch `26.03`, includes
   `flash_attn`, and adds optional Aim monitoring. Aim runs are written under
   `/home/huoju/leask/aim` and served at `http://100.123.2.95:43800/`.

Current M52A fidelity baseline on the first shard:

| Run | Docs teacher recall | Queries teacher recall | Notes |
| --- | ---: | ---: | --- |
| M52A 3-epoch pretrain | `0.1889` | `0.1855` | support imitation remains weak |
| M52C 1-epoch rank-loss smoke | `0.0965` | `0.1234` | speed smoke only; not a promoted model |
| M52C 3-epoch rank-loss | `0.1664` | `0.1416` | below M52A; support-ranking loss is not promoted |
| M52D balanced 100k pretrain | `0.2219` | `0.1993` | positive data-scaling signal |

The 3-epoch vectorized rank-loss result did not beat the M52A support recall.
This closes the current support-ranking-loss direction. It is useful as a
diagnostic, but not as the next training objective.

The balanced arXiv/PubMed/BEIR 100k pretrain did lift teacher support recall.
M52D ended at loss `1.6984` and improved support fidelity to docs `0.2219` and
queries `0.1993`. This is still far from teacher-level atom preservation, but
it is a clean positive signal that the earlier training data was too narrow.
The next M52 step should scale Stage A to a medium corpus with the same plain
teacher-imitation objective before adding more ranking loss. If the medium run
continues the fidelity curve, Stage B should start from that checkpoint; if it
plateaus, the blocker becomes encoder/support-selection capacity.

Current M52E medium corpus:

| Source | Selected units | Docs | Queries | Shortfall |
| --- | ---: | ---: | ---: | ---: |
| arXiv | `248,972` | `199,178` | `49,794` | `1,028` |
| PubMed | `248,220` | `198,576` | `49,644` | `1,780` |
| BEIR15 non-heldout corpus | `250,000` | `200,000` | `50,000` | `0` |

M52E teacher materialization has completed on `xiaoni-mbp.local`, and detached
watcher PID `48227` has started the same plain Stage-A pretrain configuration
used for M52D against:

```text
/Users/leask/Tmp/ii42_sae_m52/teacher/m52e-balanced-no-wiki-750k
```

The current M52E xiaoni pretrain was launched before Aim was added and should
not be interrupted only to retrofit tracking. Future Spark or xiaoni runs can
enable tracking with:

```bash
--aim-repo /workspace/leask/aim \
--aim-experiment sae-m52 \
--aim-run-name <run-name> \
--aim-tags m52 spark gb10
```

## Strategic Reset

M50/M51 showed that the current encoder has useful semantic signal, but the
training setup is too narrow and too entangled:

```text
teacher imitation
+ human relevance ranking
+ physical export/cost control
```

have been trained in one compact loop. M52 splits the work into two explicit
stages:

1. **Large-corpus teacher imitation pretraining**:
   train `text -> SAE atoms` to preserve the Snowflake-SAE teacher's semantic
   representation over broad, neutral corpora.
2. **Multi-source supervised ranking fine-tuning**:
   train the same encoder to respect human relevance labels from many
   retrieval benchmarks, without allowing one dataset or qrel style to dominate.

This is a model reset, not a SQL/API productization step.

## Stage A: Semantic Preservation Pretraining

### Goal

Train the encoder to approximate:

```text
text
-> Snowflake embedding
-> SAE latent atoms
```

before asking it to learn ranking labels. This stage can be large and expensive
because it is offline. Query-time cost is controlled later by export profiles
and runtime selectors.

### Primary Corpus Mix

Use a broad and intentionally balanced document corpus:

| Source | Initial Share | Role |
| --- | ---: | --- |
| Wikipedia | 25% | neutral encyclopedic language and entities |
| arXiv | 25% | long scientific and technical discourse |
| PubMed | 25% | biomedical terminology and evidence-heavy abstracts |
| BEIR15 non-heldout corpus | 25% | retrieval-domain variety without using held-out query labels |

The share is by sampled text units, not by raw corpus size. Large sources must
be capped so they do not dominate the teacher-imitation objective.

### Scale Plan

| Tier | Text units | Purpose |
| --- | ---: | --- |
| M52A-smoke | 100k | validate data loader, teacher materialization, loss shape |
| M52A-medium | 1M | first meaningful semantic-preservation run |
| M52A-large | 2M-5M | primary run if medium improves teacher gap |
| M52A-stretch | 10M+ | only if scaling curve remains positive |

The first implementation should not start from full-corpus ingestion. It should
build resumable shards with per-source quotas and deterministic seeds.

### Stage-A Corpus Builder

The concrete builder is:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m52_corpus_builder.py \
  --output-root /Users/leask/Tmp/ii42_sae_m52/stage_a \
  --beir-official-root /Users/leask/Tmp/ii42_sae_data/beir_official \
  --real-corpus-root /Users/leask/Tmp/ii42_sae_data/real_m20_pilot \
  --target-units 100000 \
  --allow-hf-wikipedia
```

The output is a BEIR-style dataset directory:

```text
documents.jsonl
queries.jsonl
quality_qrels.json
qrels.jsonl
manifest.json
summary.md
```

`documents.jsonl` and `queries.jsonl` deliberately have empty embeddings at
this point. The next step is teacher materialization:

```text
text -> Snowflake embedding -> SAE teacher atoms
```

This keeps data sampling separate from expensive teacher generation. It also
lets us rebuild the same shard against the current 8192 teacher first and the
16384 control teacher later.

### Teacher Targets

Primary target:

```text
Snowflake/snowflake-arctic-embed-m-v2.0
-> current 8192-dim SAE teacher
```

Reason: the current runtime payload, M40-M49 evidence, and M52 evaluation
baseline all use the 8192-dim teacher path.

Control target:

```text
Snowflake/snowflake-arctic-embed-m-v2.0
-> 16384-dim SAE teacher
```

Run this only after the data pipeline is stable. The latent dimension is not
fixed forever; it must win on quality/cost Pareto, not just teacher fidelity.

### Losses

Stage A should optimize representation preservation:

- atom support BCE / sampled softmax over active teacher atoms;
- atom value regression for active teacher atoms;
- teacher top-k neighborhood coverage when document collections are available;
- fanout-aware penalty only as a regularizer, not as the main objective;
- optional query/document asymmetric heads after the plain document encoder
  reaches a better teacher-fidelity baseline.

Do not use BEIR test qrels or product-quality labels in Stage A. The goal is
general semantic preservation.

## Stage B: Supervised Ranking Fine-Tuning

### Goal

Use human relevance judgments to teach final retrieval behavior:

```text
BM25 token atoms
+ SAE latent atoms
-> final sparse ranking
```

Stage B should start from the best Stage A checkpoint. It should not train a
weak encoder from scratch on qrels.

### Supervised Sources

Use all available supervised retrieval sources, but keep train/validation
boundaries explicit:

| Source | Use |
| --- | --- |
| BEIR15 official train/dev/test where available | broad retrieval regression and holdout |
| MS MARCO passage ranking | dense web-query supervision and hard negatives |
| TREC Deep Learning | high-quality judged ranking labels over MS MARCO style queries |
| Natural Questions retrieval | Wikipedia QA retrieval supervision |
| LoTTE | long-tail topic retrieval and domain robustness |
| TREC-COVID / CORD-19 | biomedical broad-query relevance; use folds or heldout splits, not leakage |
| Existing arXiv/PubMed/commons corpora | distribution/fanout/proxy diagnostics, not quality proof unless qrels exist |

If a source has no clean train/test split, create explicit query-level folds and
mark reports as cross-validation, not held-out product quality.

### Current BEIR Split Problem

The current canonical M40 checkpoint trained on this merged root:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded
```

Current data scope:

| Scope | Value |
| --- | ---: |
| Train datasets | 9 |
| Train documents | 146,353 |
| Train queries | 4,981 |
| Train qrel pairs | 146,212 |
| Eval datasets | 15 |
| Eval queries | 1,342 |
| Eval qrel pairs | 39,742 |

Important imbalance:

- `nfcorpus` contributes `121,960 / 146,212` train qrel pairs.
- `msmarco` has only `317` train qrel pairs but `4,102` eval qrels.
- official `trec-covid` contributes no train qrels in the current split.
- six full15 datasets have no current train queries.

M52 Stage B must use balanced sampling by source, query family, and qrel style,
not raw qrel-pair count.

## Sampling And Balancing

Training batches should be source-balanced:

```text
source -> dataset -> query family -> query
```

Do not let any source exceed a configured fraction of update steps. Initial
limits:

| Group | Max step share |
| --- | ---: |
| any single dataset | 15% |
| biomedical sources combined | 25% |
| MS MARCO/TREC DL web ranking combined | 25% |
| BEIR holdout-style mixed sources | 30% |
| unsupervised teacher-imitation replay during Stage B | 20% |

These are starting limits; they should be tuned by validation collapse, not by
aggregate full15 mean alone.

## Evaluation Gates

Stage A teacher-fidelity gate:

- atom support F1 / recall on active teacher atoms;
- value regression error on active teacher atoms;
- teacher-neighborhood overlap on sampled corpora;
- full15 teacher-gap matrix after export.

Stage B relevance gate:

- full15 Recall@100, MRR@20, NDCG@10, MAP@100;
- hard dataset table for `trec-covid`, `msmarco`, `dbpedia-entity`,
  `nfcorpus`;
- source-family holdout;
- supervised-source holdout where labels exist;
- physical matrix: candidate docs, BM25 postings, SAE postings, payload MB.

Promotion requires improving hard datasets without losing the M46/M49 aggregate
frontier or exploding SAE postings.

## xiaoni Execution Plan

`xiaoni-mbp.local` is reachable and has about 96GB unified memory. It should be
used for the larger CPU/MPS-heavy data and training jobs, but it does not have
Flora's `/Volumes/Betty/Tmp` mounted.

Execution layout on `xiaoni`:

```text
/Users/leask/Tmp/ii42_sae_data
/Users/leask/Tmp/ii42_sae_m52
```

First steps:

1. Generate the M52 data manifest on Flora. Completed for the first pass.
2. Rsync BEIR/shared and existing SAE artifacts to `xiaoni`. Completed for:
   `beir15_shared`, `real_m20_pilot`, and `beir_official`.
3. Materialize or download large external corpora into `xiaoni` local storage.
4. Build M52A-smoke shards.
5. Run teacher materialization and semantic-pretraining smoke.
6. Scale to M52A-medium only after smoke metrics are valid.

Current `xiaoni` staging roots:

```text
/Users/leask/Tmp/ii42_sae_data/beir15_shared
/Users/leask/Tmp/ii42_sae_data/beir_official
/Users/leask/Tmp/ii42_sae_data/real_m20_pilot
```

## Spark/GB10 Monitored Training Plan

Spark is reachable at `huoju@100.123.2.95` and all project-owned files are kept
inside `/home/huoju/leask`.

Current Spark layout:

```text
/home/huoju/leask/ii42_sae
/home/huoju/leask/aim
/home/huoju/leask/logs
```

Current container images:

```text
leask/ii42-sae:pytorch26.03
leask/ii42-sae:pytorch26.03-aim
```

The base image uses NVIDIA's `nvcr.io/nvidia/pytorch:26.03-py3` image, because
that is the reachable NVIDIA registry in this environment. The user-provided
`vcr.io` spelling did not respond to registry probing. The image has CUDA,
GB10 GPU access, `flash_attn`, `transformers`, `sentence-transformers`, `beir`,
`psycopg`, `bm25s`, `PyStemmer`, and optional Aim tracking.

Aim status:

```text
repo: /home/huoju/leask/aim
ui:   http://100.123.2.95:43800/
mode: read-only UI container, opt-in training logging
```

Training scripts remain unchanged unless `--aim-repo` is provided. This is
intentional: monitoring must not change the loss path, exported atoms, or
evaluation results.

## Non-Goals

- Do not freeze SQL/API.
- Do not claim dense-removal product readiness.
- Do not train on evaluation query IDs as ordinary train data.
- Do not allow dataset ID routing in the runtime selector.
- Do not use real arXiv/PubMed/commons efficiency-only corpora as quality
  proof without qrels.
