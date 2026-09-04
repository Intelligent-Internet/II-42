# SAE Milestone 6 PostgreSQL Evidence-Atom Surface Report

Date: 2026-05-13

## Purpose

Milestone 5 proved that the unified `EATMH001` evidence-atom payload can be
decoded and queried in standalone C with exact parity. Milestone 6 ports that
payload contract into PostgreSQL as a read-only SQL surface.

This is still not a mutable production index. The goal is narrower:

```text
EATMH001 bytea or generation table row
  + query atom ids
  + query atom weights
  + optional doc TID visibility map
  -> ranked doc_ord / ctid / score / diagnostics
```

## SQL Surface

Direct bytea query:

```sql
SELECT *
FROM ii42_evidence_atom_query(
    generation := $1::bytea,
    query_atoms := $2::int4[],
    query_weights := $3::real[],
    k := $4::int4,
    doc_tids := $5::tid[]
);
```

Generation-table query:

```sql
SELECT *
FROM ii42_evidence_atom_query_by_id(
    generation_table := 'sae_generations'::regclass,
    generation_id := 'evidence-smoke',
    query_atoms := $1::int4[],
    query_weights := $2::real[],
    k := $3::int4,
    doc_tids := $4::tid[]
);
```

The generation-table form intentionally reuses the existing experimental SAE
generation table helpers:

```text
ii42_sae_generation_create_table
ii42_sae_generation_upsert
```

That keeps the read-only resident storage contract shared while the payload
format evolves.

## Runtime Strategy

The PostgreSQL function loads the `EATMH001` payload into the existing
`SaePayload` runtime shape:

```text
doc ids              -> payload docs / tie order
doc-row atom pairs   -> doc_vector_starts + doc_vectors
impact-head pairs    -> impact_head_starts + impact_heads
atom ordinals        -> global_dim_id
```

The query path uses the same candidate-rerank shape as the standalone reader:

```text
query atoms
  -> impact-head candidate union
  -> exact doc-row scan rerank
  -> source-blind top-k sorting
```

The PG path currently uses a query-local atom weight array for the exact scan.
That matches the fastest standalone C strategy. The next production-shaped
iteration should reuse this query-local state or switch to the merge fallback
when the atom dictionary is too large.

## Verification

Commands:

```bash
python3 -m py_compile \
  scripts/research_sae_milestone4_evidence_payload.py \
  scripts/research_sae_milestone5_evidence_c_reader.py \
  scripts/test_research_sae_evidence_payload.py \
  scripts/test_research_sae_evidence_atom_pg_generation.py

cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  tests/research_sae_evidence_payload_reader.c \
  -o /tmp/research_sae_evidence_payload_reader

make PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT=$(xcrun --sdk macosx --show-sdk-path)

python3 scripts/test_research_sae_evidence_atom_pg_generation.py \
  --temp-postgres

python3 scripts/test_research_sae_evidence_payload.py

python3 scripts/test_research_sae_resident_generation_pg.py \
  --temp-postgres
```

Results:

```text
PostgreSQL EATMH001 evidence-atom smoke passed
evidence payload smoke passed
resident PostgreSQL SAE generation smoke passed
```

The PG smoke covers:

```text
direct bytea query
generation-table by-id query
direct/by-id result parity
TID visibility continuation
diagnostic counters
```

## Performance Guardrail

The current performance guardrail remains the standalone C reader matrix,
because the SQL surface still decodes the bytea payload per call. That decode
cost is not the final native-index model.

Current inner-loop guardrail:

| Path | Recall@100 | MRR@20 | Mean query ms |
| --- | ---: | ---: | ---: |
| Python `head16` payload | 0.7964 | 0.6790 | 7.6516 |
| C `head16_scan` payload | 0.7964 | 0.6790 | 0.2300 |

This means the candidate/rerank algorithm is no longer the bottleneck on the
research slice. PostgreSQL residency and decode/cache ownership are the next
systems bottlenecks.

## Current Limitations

1. The SQL API accepts atom ordinals, not atom names. The query encoder still
   owns atom-name to atom-id mapping.
2. The by-id SQL function fetches and decodes the bytea generation on each
   call. It is correct, but it is not a final resident-cache design.
3. The payload stores full doc-row sparse vectors without compression.
4. The function is read-only; there is no delta overlay or maintenance path.

## Follow-Up Status

The next milestone has implemented resident/cache ownership:

```text
sae-milestone7-pg-generation-cache-report.md
```

The by-id path now reuses parsed payloads when the generation row `xmin`
revision matches. Upsert/delete invalidate same-backend cache entries. The next
bottleneck is query-local workspace allocation/reset, not bytea decode.
