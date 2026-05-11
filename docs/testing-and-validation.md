# Testing and Validation

For source-build commands and release workflow notes, see
[Contribution](contribution.md).

## Core Test Layers

### Unit tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### PostgreSQL integration tests

```bash
make installcheck
```

## What Integration Tests Cover

- extension installation
- `CREATE INDEX USING psql_bm25s`
- `text[]`, `varchar[]`, `text`, `varchar`, and `int4[]` indexing
- canonical BM25 search APIs
- raw-query retrieval
- `@@` predicate behavior
- `<=>` ordered scans
- table-index diagnostics through supported SQL helpers
- automatic maintenance
- manual stale mode
- maintenance-state introspection
- maintenance-policy introspection
- maintenance-policy recommendation
- shared generation cache diagnostics and invalidation checks

## Mutable-Maintenance Smoke Tests

Dedicated scripts in the main repository cover:

- restart
- crash recovery
- local physical replication
- concurrent maintenance stability smoke
- query-first eventual clean-but-stale tail convergence smoke
- query-first eventual staged-maintenance cancellation smoke
- query-first eventual self-triggered background maintenance smoke
- payload-health corruption and repair smoke
- non-public extension schema wrapper smoke
- broader concurrent stress
- broader family soak

The concrete smoke scripts live in `scripts/` in the main repository.
They cover restart, crash recovery, local physical replication,
concurrent maintenance stability, query-first eventual clean-but-stale tail
convergence, staged-maintenance cancellation, payload-health corruption repair,
broader concurrent stress, non-public extension-schema wrapper resolution, and
family soak behavior.

Focused extension schema smoke:

```bash
python3 scripts/test_extension_schema_smoke.py \
    --psql /path/to/postgresql/bin/psql
```

This script verifies fresh install and `0.2.0` to current-version upgrade when the
extension is created in a non-`public` schema. It also verifies that
`ALTER EXTENSION ... SET SCHEMA` is rejected, because SQL helper functions
capture the extension schema for safe wrapper resolution.

Hybrid search coverage is part of the main regression test. It uses
synthetic vector-like candidates so the test suite does not require
`pgvector` or VectorChord:

- RRF fusion
- score fusion with `minmax`, `zscore`, `rank`, and `inverse_distance`
- mixed BM25-like and vector-distance-like candidates
- BM25 adapter output from `psql_bm25s_query(...)`

The schema smoke test also calls the hybrid fusion API from a non-`public`
extension schema, including the `0.2.0` to current-version upgrade path.

Shared generation diagnostics are covered by the main integration regression.
The regression checks that small indexes stay off the shared-cache descriptor
path, that `cache_epoch` advances after explicit refresh, and that
maintenance/REINDEX paths produce a new observable generation key.

The cache deployment model and connection-pool benchmark requirements are
documented in [Shared Generation Cache](shared-generation-cache.md). Runtime
memory sizing, workspace retention, and active warmup guidance are documented
in [Connection Memory and Index Prewarming](connection-memory.md).

Focused generation cache lifecycle smoke:

```bash
python3 scripts/test_generation_cache_smoke.py \
    --psql /path/to/postgresql/bin/psql \
    --dbname contrib_regression
```

This script verifies that operational cache clear removes corrupt descriptor
files, interrupted descriptor temp files, stale lock files, and failure
markers. It also uses independent psql backends to verify that maintenance
publishes a new observable generation key and that the new generation is
searchable.

Focused shared-preload generation cache smoke:

```bash
python3 scripts/test_shared_preload_generation_cache.py \
    --bindir /path/to/postgresql/bin
```

This script starts a temporary PostgreSQL cluster with
`shared_preload_libraries = 'psql_bm25s'`, configures
`psql_bm25s.shared_generation_cache_size`, creates a BM25 index, prewarms it,
queries it from independent `psql` backends, and verifies that the main
shared-memory arena has one ready resident generation.

Focused shared-preload automatic preload smoke:

```bash
python3 scripts/test_shared_preload_auto_preload.py \
    --bindir /path/to/postgresql/bin
```

This script verifies the `auto_preload` reloption validation, priority order,
background preload through the shared-preload worker, and the no-op behavior for
unmarked indexes.

Focused shared-preload standby smoke:

```bash
python3 scripts/test_shared_preload_standby_auto_preload.py \
    --bindir /path/to/postgresql/bin
```

This script starts a temporary primary/standby pair, verifies that the standby
auto-preloads a marked index without performing maintenance, then rebuilds the
index on the primary and verifies that the standby reloads the replicated
current generation.

Focused generation cache connection-churn benchmark:

```bash
python3 scripts/benchmark_generation_cache_churn.py \
    --dataset webis-touche2020 \
    --max-cases 50 \
    --connections 4 \
    --parallelism 4 \
    --cases-per-connection 1
```

This benchmark prepares a large enough index to use the DSM generation cache,
clears volatile cache state, then launches fresh PostgreSQL backends against
the same index. The expected DSM V2 behavior is single-flight publish,
serialized attach, and a valid descriptor after the run.

Focused query-first eventual maintenance smoke:

```bash
python3 scripts/test_query_first_eventual_tail_smoke.py \
    --psql /path/to/postgresql/bin/psql \
    --dbname contrib_regression
```

This script verifies that query-first eventual maintenance can tolerate
concurrent tail inserts by publishing a complete clean-but-stale generation,
then retrying maintenance to converge the index to a clean state with the
inserted documents searchable.

Focused cancellation smoke:

```bash
python3 scripts/test_query_first_eventual_cancel_smoke.py \
    --psql /path/to/postgresql/bin/psql \
    --dbname contrib_regression
```

This script terminates a backend during the long staged-build phase, before the
final publish. It verifies the old base index remains readable with retryable
debt, then reruns maintenance to converge the index.

Focused self-triggered background maintenance smoke:

```bash
python3 scripts/test_query_first_eventual_background_smoke.py \
    --psql /path/to/postgresql/bin/psql \
    --dbname contrib_regression
```

This script verifies that committed pending debt can wake a dynamic background
worker and converge without an explicit pg_cron call.

Focused payload-health corruption smoke:

```bash
python3 scripts/test_payload_health_corruption_smoke.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin
```

This script truncates a temporary index relation after build, verifies cheap
payload-health diagnostics report `corrupt/rebuild_required`, verifies query
fails fast instead of cold-loading a bad payload, then verifies maintenance can
rebuild a new generation from the heap.

Focused compact maintenance builder smoke:

```bash
python3 scripts/test_compact_maintenance_builder.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin \
    --builder compact
```

This script starts a temporary shared-preload cluster, creates an eventual
`text[]` index, sets a rebuild memory budget that rejects the standard builder
under the `60%` headroom rule but admits the compact builder under the `75%`
headroom rule, runs due-index maintenance, and verifies the result reports
`builder=compact` with the new row searchable after publish.

Focused spill maintenance builder smoke:

```bash
python3 scripts/test_compact_maintenance_builder.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin \
    --builder spill
```

This runs the same online maintenance path with a tighter memory budget that
rejects standard and compact builders under their headroom rules but admits the
spill builder. It verifies `builder=spill` and confirms the streamed
append-only generation is searchable.

## Benchmark Validation

Benchmark tooling is also part of validation because maintenance policy
is benchmark-backed.

Important benchmark scripts also live in `scripts/` in the main
repository. They cover automatic maintenance, churn benchmarks, policy
sweeps, policy matrices, long-run profiles, and production-shaped trace
profiles.

For query-first eventual backfill behavior, use:

```bash
PSQL_BM25S_BENCH_DSN='dbname=postgres' \
python3 scripts/benchmark_query_first_eventual_backfill.py \
    --output /tmp/psql_bm25s_eventual_backfill.json
```

This benchmark compares eager exact maintenance with query-first eventual
maintenance while a non-indexed text column is backfilled and concurrent
queries hit the indexed title-token column.

## Filtered Ranked SQL Validation Gate

For SQL-surface changes that affect filtered/ranked query composition,
validation should include all of the following:

- unit tests and `make installcheck`
- at least one integration case for:
  - `@@` plus `<=>`
  - `@@@` plus `<=>`
  - `psql_bm25s_ranked_query(...)`
- `EXPLAIN (FORMAT JSON)` inspection through:
  - `psql_bm25s_fast_path_plan(...)`
  - or `psql_bm25s_fast_path_explain(...)`

The goal is not just functional correctness. The goal is to confirm
that the intended SQL shape still uses a `psql_bm25s` index-aware plan
instead of silently degrading into a broader executor path.

If a new helper only wraps an existing exact surface, the benchmark gate
can stay narrow. At minimum it should cover:

- one canonical `rowset` case
- one combined filter/rank SQL case

If a new helper changes how filtered/ranked SQL is composed, it should
also be checked against a representative benchmark slice before the
surface is treated as stable.

## Field-Aware Helper Validation Slice

The structured field-aware helpers have one focused benchmark slice:

- [`../scripts/benchmark_sql_field_helpers.py`](../scripts/benchmark_sql_field_helpers.py)

It validates:

- result equivalence against explicit `psql_bm25s_fusion(...)`
- latency overhead for:
  - `psql_bm25s_fusion_query_weighted(...)`
  - `psql_bm25s_fusion_query_fields(...)`
  - `psql_bm25s_fusion_query(...)`

Current benchmark summary:

- all helper variants matched the explicit baseline exactly
- all variants stayed sub-millisecond on the focused synthetic slice

## Practical Rule

For code changes:

- run unit and integration tests

For maintenance-path changes:

- run unit and integration tests
- run the dedicated smoke scripts
- run the relevant benchmark slice again
