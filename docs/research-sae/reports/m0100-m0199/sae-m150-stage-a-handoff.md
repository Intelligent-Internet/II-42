# M150 Stage A Handoff

Last verified: 2026-06-01

This document records the current M150 Stage A checkpoint, data roots, and
runner entrypoints for handoff. It is intentionally limited to Stage A
representation pretraining. Later Stage B/C ranking and calibration experiments
should be treated as separate work.

## Purpose

M150 Stage A is the broad from-scratch PPLX DiffSAE representation run. The goal
is to train a `text -> sparse atoms` representation that preserves the dense
teacher neighborhood shape before applying BM25-aware ranking calibration.

This checkpoint is not the final retrieval model. It is the representation
checkpoint that later stages use as input.

## Main Checkpoint

Host:

```text
huoju@100.123.2.95
```

Run directory:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1
```

Primary files:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1/bm25sae_stagea_best.pt
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1/bm25sae_stagea_latest.pt
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1/bm25sae_stagea_summary.json
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1/events.jsonl
```

Recommended handoff checkpoint:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1/bm25sae_stagea_best.pt
```

`best` is selected by the Stage A objective. `latest` is the final training
state after the full run. Use `best` unless the follow-up experiment explicitly
needs the final state.

## Summary Metrics

From `bm25sae_stagea_summary.json`:

```json
{
  "run_name": "bm25sae-m150-pplx16384k96-stagea-v1",
  "best_metric": 0.9253296628594398,
  "best_event_epoch": 1,
  "best_event_step": 16000,
  "best_event_neighbor_overlap": 0.944531261920929,
  "examples": 107710905,
  "source_counts": {
    "document": 101583957,
    "query": 6126948
  },
  "steps": 280500
}
```

Important interpretation:

- `source_counts.document` and `source_counts.query` are consumed training
  examples over the full run, not unique document/query counts.
- The run consumed about 107.7M examples over 3 epochs.
- Final event still had `mean_feature_k = 96`, so this is the `k96` Stage A
  checkpoint.

## Materialized Training Data

This section describes what the current handoff checkpoint actually consumed
through its Stage A manifests. The broader M150 plan also called for
Wikipedia/arXiv/PubMed neutral supplements, but those files are not present in
the `bm25sae-m150-pplx16384k96-stagea-v1` manifest described below.

Canonical materialized root on Spark:

```text
/home/huoju/leask/runs/m150-beir-full-pplx
```

Datasets currently present:

```text
arguana
climate-fever
cqadupstack
dbpedia-entity
fever
fiqa
hotpotqa
msmarco
nfcorpus
nq
quora
scidocs
scifact
trec-covid
webis-touche2020
```

Each dataset directory follows this contract:

```text
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/documents.input.jsonl
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/queries.input.jsonl
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/documents.jsonl
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/queries.jsonl
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/m150_prepare_summary.json
/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/.materialized_done
```

The `documents.jsonl` and `queries.jsonl` files contain the materialized PPLX
embedding records consumed by Stage A.

## Planned Extra Data Vs This Checkpoint

M150 was designed to expand beyond earlier small corpora. The plan included:

- all official BEIR corpus documents;
- BEIR train/dev query-style rows where safe;
- high-volume neutral Wikipedia, arXiv, and PubMed documents;
- optional BM25-complement candidate rows as a light Stage A coverage auxiliary.

For this specific handoff checkpoint, the verified manifest contains only the
15 official BEIR materialized roots under:

```text
/home/huoju/leask/runs/m150-beir-full-pplx
```

The Stage A runner defaults to that root:

```text
CORPUS_ROOT=/home/huoju/leask/runs/m150-beir-full-pplx
```

No Wikipedia/arXiv/PubMed neutral shard path appears in the current
`documents.files` or `queries.files` manifests. If the follow-up experiment
wants the full planned M150 neutral-supplement route, it should create a new
manifest and run name rather than treating this checkpoint as already trained
on those neutral shards.

Optional candidate-row roots exist on Spark:

```text
/home/huoju/leask/runs/m130-pplx-all-data-candidate-rows/candidate_rows.jsonl
/home/huoju/leask/runs/bm25sae-pplx-all-data-eval-candidate-rows/candidate_rows.jsonl
```

However, the current checkpoint directory summary and `events.jsonl` do not
record `loss_coverage` or `candidate_eval` keys. Treat those candidate roots as
available auxiliary inputs for a follow-up Stage A variant, not as confirmed
training inputs for this exact `stagea-v1` checkpoint.

## Historical External-Data Route

The earlier all-data PPLX Stage A route did use external neutral data. This is
the source of the "generalization pretraining" memory, but it is not the same
input manifest as the current M150 `stagea-v1` checkpoint.

Historical all-data corpus root:

```text
/home/huoju/leask/runs/m130-pplx-all-data-corpus
```

Verified document source counts:

```text
beir15_non_heldout_corpus: 200,000
wikipedia:                 200,000
arxiv:                     199,178
pubmed:                    198,576
msmarco:                   124,100
trec-covid:                 34,654
fiqa:                       21,949
scifact:                     7,183
nfcorpus:                    5,696
arguana:                     2,000
total:                     993,336
```

Verified query/source-style counts:

```text
beir15_non_heldout_corpus: 50,000
wikipedia:                 50,000
arxiv:                     49,794
pubmed:                    49,644
nfcorpus:                   2,441
msmarco:                    2,224
fiqa:                         337
scifact:                      335
m39_broad_query_generator:    162
arguana:                       79
trec-covid:                    43
total:                    205,059
```

The relevant predecessor checkpoint family is:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagea-v2
```

M52/M54 also had neutral representation pretraining lines, including the
`m52f-balanced-wiki-1m-*` runs. Those are useful historical references, but the
current M150 handoff checkpoint should not be described as having consumed that
same external neutral corpus unless a new manifest explicitly includes it.

## Stage A Manifests

Manifest root:

```text
/home/huoju/leask/runs/m150-broad-stage-a-manifest
```

Manifest files:

```text
/home/huoju/leask/runs/m150-broad-stage-a-manifest/documents.files
/home/huoju/leask/runs/m150-broad-stage-a-manifest/queries.files
```

These list the 15 per-dataset `documents.jsonl` and `queries.jsonl` files. The
runner should consume these manifests rather than an old single-file M130-style
corpus.

## Embedding Policy

Stage A uses the PPLX teacher surface:

```text
model: perplexity-ai/pplx-embed-v1-0.6B
embedding_dim: 1024
normalize: false
similarity: cosine
max_seq_length: 512
max_text_chars: 6000
dtype: fp32
```

Do not mix this checkpoint with Snowflake or Stella embedding surfaces unless
the experiment is explicitly an ablation.

## Training Configuration

Canonical Stage A run:

```text
run_name: bm25sae-m150-pplx16384k96-stagea-v1
n_features: 16384
feature_k: 96
batch_size: 384
epochs: 3
query_repeat: 2
topk_iters: 24
device: cuda
clearml_project: ii42_sae/M150
```

Coverage auxiliary defaults supported by the runner:

```text
coverage_every: 4
coverage_batch_size: 24
coverage_recall_weight: 0.05
coverage_complement_weight: 0.10
coverage_dense_kl_weight: 0.02
```

For this exact checkpoint, rely on `bm25sae_stagea_summary.json` as the source
of truth. It does not include coverage/candidate metrics.

## Runner Entrypoints

Local source files:

```text
/Users/leask/Documents/II/ii42_sae/scripts/run_m150_broad_stage_a_spark.sh
/Users/leask/Documents/II/ii42_sae/scripts/research_sae_m130_bm25sae_stage_a_pretrain.py
/Users/leask/Documents/II/ii42_sae/sae-m150-broad-stage-a-from-scratch-plan.md
/Users/leask/Documents/II/ii42_sae/sae-m150-execution-control-plan.md
```

Spark copy of the runner:

```text
/home/huoju/leask/ii42_sae/scripts/run_m150_broad_stage_a_spark.sh
```

The Spark path is a working copy used for execution, not a confirmed Git
checkout. Prefer editing from the local repo and syncing the script to Spark
when changing the runner.

## Reproduce Or Fork A New Attempt

SSH to Spark:

```bash
ssh huoju@100.123.2.95
```

Run the same Stage A job:

```bash
cd /home/huoju/leask/ii42_sae
bash scripts/run_m150_broad_stage_a_spark.sh
```

For a colleague experiment, do not overwrite the existing run. Use a new run
name and output directory:

```bash
cd /home/huoju/leask/ii42_sae
RUN_NAME=bm25sae-m150-colleague-stagea-v1 \
OUTPUT_DIR=/home/huoju/leask/runs/bm25sae-m150-colleague-stagea-v1 \
bash scripts/run_m150_broad_stage_a_spark.sh
```

The runner will move an existing output directory to a timestamped backup if
the target exists, but using a new name is still safer.

## ClearML

The runner expects:

```text
/home/huoju/leask/clearml_env.sh
```

and logs tasks under:

```text
ii42_sae/M150
```

Current ClearML endpoints should be taken from `clearml_env.sh`; do not hardcode
old IPs into new scripts.

## Handoff Notes

- Treat this as Stage A only. It is suitable as a base checkpoint for Stage B/C
  training, candidate-surface diagnostics, or atom-coverage experiments.
- Do not claim final retrieval quality from Stage A alone.
- Do not train on official test qrel labels when evaluating BEIR promotion
  gates.
- Keep BM25-aware ranking calibration separate from representation pretraining
  when comparing with older M130/M128 runs.
- If the colleague changes embedding model, feature count, or `feature_k`, the
  result is a new branch of experiments and should not be compared as the same
  M150 Stage A checkpoint.
