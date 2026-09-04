# SAE Block-Max Phase 3.9 Resident Generation Report

Date: 2026-05-12

## Purpose

Phase 3.9 turns the packed read-only generation from a SQL-passed `bytea`
prototype into a PostgreSQL-resident generation table prototype.

The scope is still read-only:

- no mutable delta overlay;
- no PostgreSQL access method integration;
- no background maintenance;
- no inference inside PostgreSQL.

The goal is to validate the API shape and runtime contract that the later
native index can preserve.

## Resident Table Contract

The minimal table used by the benchmark is:

```sql
CREATE TABLE sae_generations (
    generation_id text PRIMARY KEY,
    generation bytea NOT NULL,
    metadata jsonb NOT NULL DEFAULT '{}'::jsonb,
    created_at timestamptz NOT NULL DEFAULT now()
);
```

The new C entrypoint is:

```sql
ii42_sae_block_max_query_generation_by_id(
    generation_table regclass,
    generation_id text,
    query_dims int4[],
    query_weights real[],
    k int4 DEFAULT 100,
    doc_tids tid[] DEFAULT NULL
)
```

It returns the same result contract as
`ii42_sae_block_max_query_generation(...)`:

```text
rank
ctid
doc_ord
score
opened_blocks
opened_docs
scored_docs
block_entry_visits
block_entry_binary_steps
posting_slice_hits
decoded_postings
memory_bytes
```

The function reads exactly one row from the generation table by
`generation_id`, copies the `bytea` payload into the caller query memory
context, and runs the same read-only traversal.

## Resident Memory Compression

Phase 3.6 used:

```text
SaePackedBlockEntry[] + packed_impacts[]
```

Phase 3.9 moves closer to the on-disk `SBMXM001` layout:

```text
SaePackedEntry[]              -- uint16 block_delta, uint8 doc_mask, uint8 count
packed_entry_max_impacts[]    -- float4, computed once on load
packed_impacts[]              -- float4 exact impact stream
```

The query-plan builder decodes block deltas sequentially while computing block
bounds. It captures `posting_start`, `doc_mask`, and `posting_count` inside
the per-block query plan, so opened-block scoring does not need random lookup
back into a decoded block-entry row.

This preserves:

```text
block_entry_binary_steps = 0
```

and reduces PostgreSQL resident memory for the packed path.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/test_research_sae_resident_generation_pg.py scripts/research_sae_resident_generation_benchmark.py scripts/test_research_sae_block_max_pg_generation.py scripts/test_research_sae_packed_microblock_pg_generation.py scripts/research_sae_block_max_sim.py
make -B PG_CONFIG=/opt/homebrew/Cellar/postgresql@18/18.3/bin/pg_config PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
python3 scripts/test_research_sae_resident_generation_pg.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_packed_microblock_reader.py
python3 scripts/test_research_sae_block_max_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_packed_microblock_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/research_sae_resident_generation_benchmark.py --temp-postgres --library ./ii42.dylib --output-dir results/sae/phase39/resident-generation
```

Results:

- direct compact generation smoke passed;
- direct packed generation smoke passed;
- resident by-id packed generation smoke passed;
- resident by-id result rows match direct `bytea` result rows;
- standalone packed C reader smoke passed;
- five-dataset resident matrix remains exact.

## Benchmark

The resident benchmark uses:

```text
generation format: SBMXM001
layout: sae_overlap_greedy
micro_block_size: 2
score_mode: normalized_idf_dot
top_k: 100
```

| Dataset | Exact | PG mean query ms | Memory bytes | Opened docs | Decoded postings | Binary steps |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `100/100` | `2.624` | `1523340` | `437.74` | `3086.11` | `0.00` |
| `scidocs` | `100/100` | `2.197` | `1542860` | `420.00` | `2855.56` | `0.00` |
| `nfcorpus` | `100/100` | `2.249` | `1533148` | `365.13` | `2302.50` | `0.00` |
| `arguana` | `100/100` | `2.245` | `1472348` | `305.42` | `2595.86` | `0.00` |
| `fiqa` | `100/100` | `2.199` | `1552708` | `362.46` | `2508.90` | `0.00` |

The benchmark output is stored under:

```text
results/sae/phase39/resident-generation
```

Those result files are generated artifacts, not source-of-truth design files.

## Interpretation

Phase 3.9 validates the next API boundary:

```text
application stores generation once
query passes generation_id + sparse query dims/weights
PostgreSQL returns exact top-k rows with diagnostics
```

This is much closer to a native index surface than passing a large `bytea` in
every query. It also catches an important implementation detail: generation
payloads fetched through SPI must be copied into the caller query memory
context before `SPI_finish()`.

The remaining bottleneck is still opened-doc selectivity, not lookup mechanics
or resident memory. The current exact path opens about `15-22%` of documents on
the prepared 2k-document slices. Further progress needs a stronger physical
layout or multi-level bound representation.

## Current Decision

The Phase 4 read-only surface should build on:

```text
resident table: generation_id -> SBMXM001 generation
query API: generation_id + sparse dims/weights
layout: sae_overlap_greedy
micro_block_size: 2
resident decode: delta entry + max impact + exact impact stream
```

Do not add delta overlay or mutable maintenance until the read-only resident
surface is stable and the opened-doc selectivity target is clearer.
