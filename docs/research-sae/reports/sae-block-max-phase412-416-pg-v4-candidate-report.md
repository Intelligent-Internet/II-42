# SAE Block-Max Phase 4.12-4.16 PostgreSQL V4 Candidate Report

Date: 2026-05-12

## Purpose

Phase 4.11 selected the first native-compatible candidate budget:

```text
impact_head_size = 32
active_dims = 12
postings_per_dim = 32
```

Phase 4.12-4.16 completes the short-term read-only exploration by moving that
path from the standalone C harness into PostgreSQL, validating the by-id resident
generation API, exposing diagnostics, and recording the remaining storage
decision.

Mutable delta overlay, delete tombstones, and maintenance workers remain out of
scope for Phase 4.

## Implementation

The PostgreSQL loader now accepts packed `SBMXM001` versions `3` and `4`:

```text
v3 = packed micro-block generation + row-wise doc vectors
v4 = v3 + dimension-local impact-head directory
```

New C/SQL functions:

```text
ii42_sae_candidate_rerank_generation(...)
ii42_sae_candidate_rerank_generation_by_id(...)
ii42_sae_readonly_index_candidate_query(...)
```

The query path is:

```text
query dims
  -> select top active_dims by query weight
  -> read impact_head[dim][:postings_per_dim]
  -> union candidate doc_ord values
  -> rerank candidates through doc_vector[doc_ord]
  -> return rank, ctid, doc_ord, score, diagnostics
```

The old exact `ii42_sae_block_max_query_generation(...)` path is unchanged.
It can still query v1/v2 packed generations, and can now also read v3/v4
payloads while ignoring the candidate-specific sections.

## API Surface

The read-only surface is now enough for the next prototype:

- Create resident generation table:
  `ii42_sae_generation_create_table(schema_name, table_name)`
- Upsert/list/delete generation payloads:
  `ii42_sae_generation_upsert`, `..._list`, `..._delete`
- Exact read-only traversal:
  `ii42_sae_readonly_index_query`
- Selected Phase 4 candidate traversal:
  `ii42_sae_readonly_index_candidate_query`

The candidate function defaults encode the selected policy:

```text
k = 100
active_dims = 12
postings_per_dim = 32
```

## PostgreSQL Benchmark

Command:

```bash
python3 scripts/research_sae_v4_pg_candidate_benchmark.py \
  --temp-postgres \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --output-dir results/sae/phase412/pg-v4-candidate-rerank
```

Results:

| Dataset | Queries | PG ms/query | Exact overlap | Candidates | Gen visits | Gen decoded | Rerank doc terms | Generation MB | Memory MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 100 | `1.701` | `0.6739` | `307.8` | `12.0` | `374.9` | `36932.7` | `2.45` | `3.04` |
| `scidocs` | 100 | `1.664` | `0.6692` | `305.7` | `12.0` | `372.4` | `36719.7` | `2.47` | `3.07` |
| `nfcorpus` | 100 | `1.816` | `0.6749` | `302.2` | `12.0` | `368.3` | `36416.5` | `2.52` | `3.14` |
| `arguana` | 100 | `1.593` | `0.7151` | `254.6` | `12.0` | `378.1` | `30243.0` | `2.45` | `3.04` |
| `fiqa` | 100 | `1.664` | `0.7037` | `294.5` | `12.0` | `374.0` | `35368.6` | `2.49` | `3.10` |

Five-dataset mean:

```text
PG ms/query = 1.688
Exact@100 overlap = 0.68736
Candidate docs = 292.96
Generator visits = 12
Generator decoded postings = 373.52
Rerank doc-vector terms = 35136.11
Resident memory = 3.08 MB
```

The PostgreSQL counters match the Phase 4.11 native-selected `d12_pdim32` shape:
impact-head candidate generation opens exactly 12 generator lists, and rerank
uses doc-vector scans with zero block-range binary lookup.

## Storage Decision

Phase 4.14 does not need another physical format before the PostgreSQL prototype.
The current v4 payload is acceptable for read-only exploration:

```text
generation bytes: about 2.45-2.52 MB per 2k-doc slice
resident memory: about 3.04-3.14 MB per 2k-doc slice
```

The remaining storage issue is real but not blocking: v4 duplicates the sparse
impact stream in document-major order for fast candidate rerank. Compression
should be the first Phase 5 storage task, not another Phase 4 blocker, because
the query policy and API surface now need larger-scale validation.

Candidate compression directions to revisit:

- varint or delta-coded doc-vector dimension ids;
- optional float16 or quantized impacts for doc-vector rerank;
- per-document vector starts compressed by fixed active-dim assumptions;
- keeping impact heads as a small uncompressed hot directory.

## Phase 4 Closeout

Phase 4 is now complete as a read-only exploration stage:

1. Exact packed micro-block traversal works in PostgreSQL.
2. V4 impact-head candidate generation works in PostgreSQL.
3. Row-wise doc-vector rerank works in PostgreSQL.
4. The selected default is encoded in SQL defaults.
5. Diagnostics expose candidate count, generator visits, decoded postings, and rerank doc-vector terms.
6. Full five-dataset PG benchmark is automated.

The next phase should stop changing the read-only query policy by default and
move to production-shape questions: larger payload scale, resident cache
ownership, optional compression, then mutable maintenance.

## Verification

Commands run:

```bash
python3 -m py_compile \
  scripts/test_research_sae_block_max_pg_generation.py \
  scripts/test_research_sae_packed_microblock_pg_generation.py \
  scripts/research_sae_v4_pg_candidate_benchmark.py

make PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT=$(xcrun --show-sdk-path)

python3 scripts/test_research_sae_packed_microblock_pg_generation.py \
  --temp-postgres

python3 scripts/research_sae_v4_pg_candidate_benchmark.py \
  --temp-postgres \
  --datasets scifact \
  --max-queries 5 \
  --output-dir results/sae/phase412/pg-v4-candidate-smoke

python3 scripts/research_sae_v4_pg_candidate_benchmark.py \
  --temp-postgres \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --output-dir results/sae/phase412/pg-v4-candidate-rerank
```
