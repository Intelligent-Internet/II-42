# SAE Milestone 7 PostgreSQL Generation Cache Report

Date: 2026-05-13

## Purpose

Milestone 6 made `EATMH001` queryable from PostgreSQL, but the by-id path still
fetched and decoded the generation bytea on every SQL call. Milestone 7 removes
that bottleneck with a backend-local parsed payload cache.

The target is:

```text
first by-id query:
  fetch generation bytea
  parse EATMH001 / SBMXM001
  store parsed payload in backend-local cache

later by-id query:
  fetch only row xmin revision
  reuse parsed payload if revision matches
  run the same exact scorer
```

This remains read-only. It does not add mutable deltas or index maintenance.

## Cache Contract

Cache key:

```text
generation table OID
generation_id
payload kind
row xmin revision
```

Payload kinds:

```text
block_max      -> existing SBMXG/SBMXM SAE payloads
evidence_atom  -> new EATMH001 payloads
```

Invalidation:

```text
same backend upsert/delete:
  ii42_sae_generation_upsert/delete invalidates matching cache entries

other backend upsert/delete:
  by-id query compares cached revision with current row xmin
  stale cache entry is discarded before parsing the new generation
```

This is intentionally backend-local. It avoids shared-memory ownership and
locking while proving that per-query bytea decode is no longer required.

## SQL Diagnostics

Two helper functions were added:

```sql
SELECT ii42_sae_generation_cache_clear();

SELECT *
FROM ii42_sae_generation_cache_state();
```

`cache_state` returns:

```text
relation_oid
generation_id
payload_kind
revision
memory_bytes
hits
doc_count
```

These helpers are experimental diagnostics for the research path.

## Benchmark

Runner:

```text
scripts/research_sae_milestone7_pg_cache_benchmark.py
```

Artifact:

```text
results/sae/milestone7/pg-generation-cache/summary.md
```

Configuration:

```text
dataset       = scifact
queries       = 50
head_size     = 16
payload_bytes = 4,375,236
documents     = 2,000
```

Timing:

| Path | Calls | Mean ms | Total ms |
| --- | ---: | ---: | ---: |
| `direct_bytea` | 50 | 2.8116 | 140.5800 |
| `by_id_cold_clear_each` | 50 | 1.4916 | 74.5813 |
| `by_id_cached` | 50 | 0.5180 | 25.8979 |

Parity:

```text
direct_vs_cached_exact = true
cold_vs_cached_exact   = true
```

Cache state after the benchmark:

```text
[('bench', 'evidence_atom', 100, 5028068, 2000)]
```

The hit count is 100 because the benchmark warms the cache and then executes a
timed cached pass plus a parity pass.

## Interpretation

The by-id cached path is about:

```text
2.9x faster than by-id cold decode
5.4x faster than direct bytea query from Python
```

This confirms the previous bottleneck has been removed for repeated by-id
queries. The SQL path still has SPI revision-check overhead, but it no longer
pays bytea fetch/decode cost per query.

The result also preserves ranking exactly. The cache changes ownership and
decode frequency, not scoring semantics.

## Verification

Commands:

```bash
python3 -m py_compile \
  scripts/research_sae_milestone7_pg_cache_benchmark.py \
  scripts/test_research_sae_evidence_atom_pg_generation.py

make PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT=$(xcrun --sdk macosx --show-sdk-path)

python3 scripts/test_research_sae_evidence_atom_pg_generation.py \
  --temp-postgres

python3 scripts/test_research_sae_resident_generation_pg.py \
  --temp-postgres

python3 scripts/test_research_sae_packed_microblock_pg_generation.py \
  --temp-postgres

python3 scripts/research_sae_milestone7_pg_cache_benchmark.py \
  --output-dir results/sae/milestone7/pg-generation-cache
```

The EATMH smoke covers:

```text
cache creation
cache hit accounting
upsert invalidation
delete invalidation
direct/by-id result parity
TID visibility continuation
```

The SBMXM resident and packed generation smokes still pass, so the shared
cache did not break the existing SAE read-only path.

## Next Step

The next bottleneck is no longer bytea decode. It is the query-local working
set:

```text
candidate_seen[doc_count]
candidate_docs[doc_count]
scores[doc_count]
scored_seen[doc_count]
query_weights[atom_count]
```

For larger payloads, the next milestone should replace per-query full-size
zeroed arrays with reusable cache-local workspaces and touched-list reset.
That should keep the fast `scan` scorer without paying O(doc_count +
atom_count) allocation/zeroing on every SQL call.
