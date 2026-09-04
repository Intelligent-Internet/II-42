# Unified BM25+SAE PostgreSQL Read-Only Payload Report

Date: 2026-05-14

## Scope

This phase ports the `UBMXM001` unified BM25+SAE payload from the standalone C
reader into PostgreSQL as a read-only SQL-visible query surface.

The payload keeps BM25 token dimensions and SAE atom dimensions in one physical
structure:

```text
source-tagged dimensions
doc-row sparse vectors
impact-head candidate lists
embedded query dimensions
embedded expected rankings for parity tests
```

The PostgreSQL reader follows the same path as the Python exporter and C smoke
reader:

```text
SAE impact heads
+ BM25 impact heads
  -> candidate union
  -> source-aware normalized rerank through doc sparse vectors
  -> top-k rows with diagnostics
```

## SQL Surface

Two experimental read-only functions are now exposed:

```sql
SELECT *
FROM ii42_unified_payload_query(
    payload => $1,
    query_filter => NULL,
    doc_tids => NULL
);

SELECT *
FROM ii42_unified_payload_query_by_id(
    generation_table => 'schema.unified_generations'::regclass,
    generation_id => 'smoke',
    query_filter => NULL,
    doc_tids => NULL
);
```

Both functions return:

```text
query_id
rank
ctid
doc_ord
score
selected_sae_terms
bm25_terms
sae_candidate_docs
bm25_candidate_docs
candidate_overlap_docs
candidate_docs
scored_docs
sae_postings
bm25_postings
bm25_docs_touched
rerank_doc_terms
rerank_score_terms
memory_bytes
```

This is deliberately a read-only research surface. It does not add mutable
maintenance, an access method, or a shared resident cache entry for `UBMXM001`.
The by-id function fetches the stored generation and decodes it in the query
memory context, which is simpler and safer for the first PostgreSQL integration
checkpoint.

## Validation

Commands run:

```bash
PATH="/opt/homebrew/opt/postgresql@18/bin:$PATH" \
make PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
     PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

python3 -m py_compile \
    scripts/research_sae_unified_payload_export.py \
    scripts/research_sae_unified_payload_c_benchmark.py \
    scripts/research_sae_unified_native_payload_simulator.py \
    scripts/test_research_sae_unified_payload_pg.py

cc -std=c11 -O2 -Wall -Wextra -Werror \
    -o /tmp/research_sae_unified_payload_reader \
    tests/research_sae_unified_payload_reader.c

python3 scripts/test_research_sae_unified_payload_pg.py --temp-postgres
```

Result:

```text
unified BM25+SAE PostgreSQL payload smoke passed
```

The smoke test covers:

- direct `bytea` query parity against embedded expected rankings;
- by-id resident-table query parity against the same embedded expected rankings;
- `query_filter` behavior;
- `tid[]` mapping to non-null SQL-visible TIDs;
- diagnostic counters populated for candidate generation and rerank.

## Current Limitations

- No backend-local parsed cache for `UBMXM001` yet. This avoids changing the
  existing SAE/evidence generation cache contract before the unified payload
  shape stabilizes.
- The first smoke uses the synthetic research fixture. Larger BEIR payloads
  should be run next to separate decode cost from traversal cost.
- The SQL surface still uses embedded query dimensions from the payload. A
  production-shaped API must later accept runtime query atoms from the encoder.

## Next Step

The next integration step should be:

```text
larger payload benchmark
-> add optional backend-local UBMX parsed cache
-> define runtime-query form
-> compare SQL path with standalone C on the same BEIR payloads
```

Mutable delta overlays and access-method integration should remain out of scope
until the read-only runtime-query contract and larger-payload costs are stable.

The model-side follow-up is now tracked separately in:

```text
sae-milestone22-sae-splade-concept-roadmap.md
```

That milestone incorporates the SAE-SPLADE paper direction: learned concept
latents should be evaluated as a sparse output vocabulary, with TopK and
QD-FLOPs-style cost gates, before we freeze a product-shaped query API around
the current Snowflake-derived atom pipeline.
