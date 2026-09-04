# SAE M81 Large Stage-A Surface Plan

Status: active first layer. M81 follows the M80-A2.3 finding that the current
`36,867` document focused surface is still too narrow. The goal is to stabilize
large Stage-A sparse preservation before returning to Stage B.

## Machine Split

Current availability:

| Machine | Role | Current data state |
| --- | --- | --- |
| `spark` / `huoju@100.123.2.95` | primary data build and training | has M52, M39, M60, BEIR15 data; 121 GiB RAM; about 560 GiB free |
| `Flora` local Mac | source control, script validation, light BEIR checks | has repo and partial BEIR cache; large M52/M39 not present locally |
| `xiaoni-mbp.local` | optional light validation/shard worker | reachable with 96 GiB RAM, but large M52/M39/BEIR data is not present |

Decision: first stabilize on Spark. Distributing heavy data prep to xiaoni would
require moving large artifacts first, which adds more risk than it removes. Once
the Spark pipeline is stable, small shardable jobs can be copied to xiaoni.

## M81-A0: Large Surface Build

First stable target:

```text
/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0
```

Inputs:

| Source | Role |
| --- | --- |
| BEIR15 shared | fixed supervised candidate surface and eval sanity |
| M39 broad generated | broad query supervision, especially TREC-COVID/MSMARCO style rows |
| M60 MS MARCO split | larger supervised query/document signal |
| M52 balanced Stage-A | large neutral representation surface, no quality claim |

Initial scale:

| Component | Target |
| --- | ---: |
| M52 representation docs | 200,000 |
| M52 representation queries | 50,000 |
| BEIR15 supervised docs/queries | all cached rows |
| M39 supervised docs/queries | all cached rows |
| M60 supervised docs/queries | 20,000 docs / 300 queries for first stable run |
| Candidate depth | 128 |

This is intentionally smaller than full M52 and full M60. The initial attempt
to run full M60 (`100k docs x 2k queries`) exposed that the current candidate
builder still performs expensive dense candidate construction per query. The
stable first layer therefore caps M60 at `20k/300` while keeping M52 large
enough to test representation scale. After this completes, the next scaling
step should either cache dense candidates or vectorize per-source dense scoring
before uncapping M60.

## M81-A1: Dense Cache Materialization

M80 repeatedly re-encoded texts, which is acceptable for V0 but not for M81.
After M81-A0 builds, materialize dense vectors from the current M80-A1
checkpoint into a persistent cache:

```text
/home/huoju/leask/data/ii42_sae_m81/dense-cache-v0
```

The cache should contain ids, source metadata, and float32 vectors for documents
and queries. A2 must read this cache instead of repeatedly loading Snowflake and
encoding large JSONL files.

New entrypoint:

```text
scripts/research_sae_m81_materialize_dense_cache.py
```

It writes:

```text
documents.ids.jsonl
documents.vectors.npy
queries.ids.jsonl
queries.vectors.npy
manifest.json
```

The first cache should run on Spark after `large-stage-a-v0` completes. Flora
and xiaoni can validate code and small slices, but Spark is the only machine
that currently has the full M52/M39/M60 data stack.

## M81-P1: Distributed Preprocessing Shards

Before training, split the completed M81 surface into deterministic shards:

```text
/home/huoju/leask/data/ii42_sae_m81/preprocess-shards-v0-stable
```

New entrypoint:

```text
scripts/research_sae_m81_make_preprocess_shards.py
```

It writes:

```text
documents.shardNNN.jsonl
queries.shardNNN.jsonl
candidate_rows.shardNNN.jsonl
manifest.json
```

Documents and queries are assigned by stable id hash. Candidate rows are
assigned by `query_id`, so query-side workers can keep their candidate training
rows local. This is a preprocessing layout, not a quality-changing step.

Current execution policy:

- Spark owns the first full shard build because it has all source corpora and
  the dense student checkpoint.
- Flora and xiaoni should receive explicit shard artifacts only. They should
  not independently rebuild M52/M39/M60 from partial local caches.
- Dense cache materialization can run per shard through
  `research_sae_m81_materialize_dense_cache.py --documents-path ... --queries-path ...`.
- The sparse trainer can consume cache shards through repeated
  `--dense-cache-dir` arguments. Training remains centralized on Spark after
  shard caches are verified, so the final model is produced from one
  reproducible cache layout.

## M81-A2: Sparse Preservation Rerun

First rerun:

```text
latent_dims = 8192
active_dims = 256
candidate_kl_weight = 1.0
pairwise_rank_weight = 0.1
selection = balanced_gate
```

Do not optimize low-k first. k256 is the high-quality ceiling check. Only if
k256 is stable on the large surface should we test k160/k192/k224 again.

## Stage-B Gate

Stage B remains blocked until M81-A2 proves:

- broad generated query tax is no longer collapsing;
- k256 sparse tax is stable on the larger surface;
- low-k degradation can be addressed by representation/training changes, not
  post-hoc scoring alone.

## First Execution

New entrypoint:

```text
scripts/research_sae_m81_large_surface.py
```

This script extends the M80 neutral corpus builder with an additional M60
supervised source and larger default limits. It writes:

```text
documents.jsonl
queries.jsonl
candidate_rows.jsonl
manifest.json
summary.md
```

The first Spark run should build `large-stage-a-v0`; next steps are dense cache
materialization and k256 sparse preservation rerun.
