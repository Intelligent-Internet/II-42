# Connection Memory and Index Prewarming

This page describes how `psql_bm25s` manages memory across PostgreSQL
backends, how to size connection pools, and how to actively prewarm large BM25
indexes.

The important split is:

- Immutable generation payloads are the decoded BM25 index contents. Large
  payloads are shared through DSM by default and can use an optional
  `shared_preload_libraries` arena for the lowest fresh-backend cost.
- Mutable query workspace is backend-local scratch memory used while scoring
  and ranking. It is not shared, because it is tied to one query execution and
  one backend snapshot.

## Memory Model

For a large index, the ideal production shape is:

```text
shared immutable generation payload  +  small bounded per-backend workspace
```

The shared payload avoids `index_size * connection_count` memory growth. The
remaining per-backend workspace is intentionally bounded by default so a large
connection pool does not keep all query scratch buffers resident forever.

Resident memory tools can be misleading:

- RSS can count a shared DSM or shared-preload mapping in every backend.
- PSS is usually better for understanding proportional shared-memory cost.
- Private dirty/anonymous memory is the best signal for backend-local
  workspace growth.

## Workspace Retention Settings

These settings are ordinary PostgreSQL GUCs. Users do not need to configure
them for normal deployments.

| Setting | Default | Meaning |
| --- | --- | --- |
| `psql_bm25s.workspace_cache_bytes` | `32MB` | Maximum mutable query workspace retained by one backend after a query finishes. Workspaces larger than this are released at the end of the query. |
| `psql_bm25s.workspace_idle_timeout` | `60s` | How long an idle backend may keep retained mutable workspace. Reclamation is lazy and happens the next time that backend touches a `psql_bm25s` index. |

`workspace_cache_bytes = 0` releases mutable query workspace after every query.
`workspace_cache_bytes = -1` keeps workspace without a size cap in the current
backend. The uncapped mode is mainly useful for controlled benchmark runs or
small fixed-size connection pools.

`workspace_idle_timeout = 0ms` makes retained workspace expire on the next
`psql_bm25s` cache entry after it was used. It does not interrupt an active
query.

`workspace_idle_timeout = -1` disables idle-time workspace release. If both
`workspace_cache_bytes` and `workspace_idle_timeout` are set to `-1`, the
backend keeps mutable workspace until the cached index generation is
invalidated, the backend exits, or `psql_bm25s_generation_cache_clear()` is
called.

These settings only affect backend-local scratch buffers such as score arrays,
candidate bitmaps, and touched-document lists. They do not evict immutable DSM
or shared-preload generation payloads.

## Shared Generation Tiers

The immutable generation cache has three tiers:

1. Optional shared-preload arena, configured at PostgreSQL start.
2. Zero-configuration DSM cache.
3. Backend-local selected path for small indexes and query-sensitive overlay
   materializations.

For very large connection pools, the optional shared-preload arena can remove
most fresh-backend mapping overhead:

```conf
shared_preload_libraries = 'psql_bm25s'
psql_bm25s.shared_generation_cache_size = '32GB'
```

The arena must be sized for the resident hot indexes that should stay warm.
If the arena is not configured, DSM sharing still prevents every backend from
privately decoding and copying the same large immutable generation.

See [Shared Generation Cache](shared-generation-cache.md) for the cache-tier
design and failure behavior.

## Active Prewarming

Use `psql_bm25s_generation_cache_preload(index regclass)` to warm the best
available cache tier for one index:

```sql
SELECT public.psql_bm25s_generation_cache_preload(
    'commons.data_pubmed__introduction__bm25_idx'::regclass
);
```

The function returns a diagnostic string describing the warmed generation. In
a shared-preload deployment it can populate the main shared-memory arena before
application traffic reaches the index. This is an optional deployment hook:
if a share-capable index is first queried before manual warmup, psql_bm25s
still uses shared publication instead of privately loading one copy per backend.
Without shared-preload it warms the DSM tier for share-eligible large
generations.

A simple warmup script can preload selected indexes after deploy or restart:

```sql
SELECT public.psql_bm25s_generation_cache_preload(indexrelid)
FROM pg_index
WHERE indexrelid::regclass::text IN (
    'commons.data_arxiv__title__bm25_idx',
    'commons.data_pubmed__abstract__bm25_idx',
    'commons.data_pubmed__introduction__bm25_idx'
);
```

For large deployments, run warmup from one administrative session before
opening the full application connection pool. That avoids a cold connection
stampede and lets later backends attach or inherit the resident generation.

## Sizing Guidance

For a service with many PostgreSQL backends:

```text
required RAM ~= PostgreSQL baseline
             + hot shared BM25 generation bytes
             + connection_count * retained workspace budget
             + OS page cache and safety margin
```

With defaults, the retained workspace term is bounded around:

```text
connection_count * 32MB
```

The actual active-query peak can be higher while a query is executing on a
very large index, but the backend releases workspace above the configured
budget after the query finishes.

Practical guidance:

- Keep `workspace_cache_bytes` at the default for general connection pools.
- Lower it to `0` or a small value when connection count is very high and
  first-query scratch allocation cost is acceptable.
- Raise it only for fixed-size pools where repeat-query latency matters more
  than private memory.
- Use `psql_bm25s_generation_cache_preload(...)` for hot large indexes after
  deploy, restart, or major index maintenance.
- Use shared-preload when fresh-backend latency is important and operators can
  change PostgreSQL configuration.

## Diagnostics

Inspect the immutable generation cache:

```sql
SELECT public.psql_bm25s_generation_cache_state(
    'commons.data_pubmed__introduction__bm25_idx'::regclass
);
```

Clear volatile generation-cache state when testing cold-load behavior:

```sql
SELECT public.psql_bm25s_generation_cache_clear();
```

This does not change durable index contents. It only clears backend-local
state and best-effort volatile generation descriptors.
