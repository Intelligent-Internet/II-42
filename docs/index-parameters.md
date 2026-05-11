# Index Parameters

`psql_bm25s` exposes index parameters as PostgreSQL reloptions:

```sql
CREATE INDEX docs_body_bm25_idx
    ON docs USING psql_bm25s (body)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75
    );
```

This page is the compact reference for those `WITH (...)` options. Use it
together with [Supported Input Types](input-types.md) and
[Index Policy](index-policy.md).

## Policy Guide

This page lists every index reloption. For scenario-based presets, the behavior
of `consistency = 'realtime'`, `'eventual'`, and `'manual'`, and scheduler
guidance, see [Index Policy](index-policy.md).

## Scoring Parameters

These parameters affect the stored BM25 payload. Set them before building the
index. If you change them later with `ALTER INDEX ... SET (...)`, rebuild the
index before relying on the new scoring behavior.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `method` | `'lucene'` | BM25 scoring variant used for term weighting. Supported values are `'robertson'`, `'lucene'`, `'atire'`, `'bm25l'`, and `'bm25+'`. |
| `idf_method` | `'lucene'` | IDF variant. It accepts the same values as `method`. |
| `k1` | `1.5` | BM25 term-frequency saturation parameter. |
| `b` | `0.75` | BM25 document-length normalization parameter. |
| `delta` | `0.5` | Delta parameter used by the BM25L and BM25+ variants. |
| `create_empty_token` | `true` | Adds an empty token for integer-token indexes so empty documents can still be represented consistently. |

## Text Processing Parameters

These parameters apply to scalar `text` and `varchar` indexes, where the
extension tokenizes raw text at the index boundary. Pretokenized `text[]`,
`varchar[]`, and `int4[]` inputs are already token streams and do not need this
index-time scalar tokenizer path.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `text_lowercase` | `true` | Applies Unicode-aware case folding before indexing scalar text. |
| `text_stopwords` | `NULL` | Comma-separated stopword list for scalar text tokenization. `NULL` means no index-level stopword filter. |
| `text_stem_english` | `false` | Applies English Porter stemming to scalar text tokens. |
| `text_fold_diacritics` | `false` | Folds Latin diacritics before indexing scalar text. |

Index-bound query helpers such as `psql_bm25s_prepared_query(index_name, ...)`,
`psql_bm25s_query(index_name, ...)`, and
`psql_bm25s_query_prepared(...)` inherit omitted scalar text options
from the named index. Raw SQL expressions outside a real index scan do not
silently discover hidden index reloptions; use the index-bound helpers when
text-processing alignment matters.

## Multicolumn Parameters

These parameters apply only to multicolumn `text[]`, `varchar[]`, `text`,
and `varchar` indexes.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `field_aware` | `false` | When `false`, a multicolumn index is a fused-document index. When `true`, each indexed column gets a field-scoped internal token namespace. Generic direct APIs search all fields with equal weight for token and simple raw term queries; `psql_bm25s_field_aware_query_tokens(...)` and `psql_bm25s_field_aware_query(...)` can apply query-time field weights inside one multicolumn index. This is field-aware retrieval, not BM25F and not per-field length normalization. |

See [Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md) and
[Field-Aware Indexes](field-aware-indexes.md).

## Maintenance Parameters

These parameters control how mutable indexes react to `INSERT`, `UPDATE`,
`DELETE`, and `VACUUM`. They are validated against the selected `consistency`
policy. Parameters that do not apply to that policy must be removed instead of
being set to their default values.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `consistency` | `'realtime'` | Selects the maintenance consistency model. `'realtime'` preserves strong freshness and may maintain in foreground paths. `'eventual'` records automatic-maintenance debt and allows bounded stale results so foreground work can stay fast. `'manual'` marks writes stale, may update lightweight pending counts, and requires explicit maintenance. |
| `auto_rebuild_threshold` | `0` for `realtime`, `50000` for `eventual` | Valid with `realtime` and `eventual`. In realtime, pending write/delete counts at or above this value make consolidation eligible. In eventual, a positive value enables exact delta payloads for small pending changes before convergence. |
| `auto_rebuild_delta_bytes` | `0` | Valid with `realtime` and `eventual`. Pending delta-byte threshold for consolidation. In realtime this can trigger foreground maintenance; in eventual this is a background rebuild trigger. `0` disables the byte threshold. |
| `auto_rebuild_churn_ratio` | `0.0` | Valid only with `realtime`. Pending change count divided by indexed document count. A positive value triggers foreground maintenance when churn reaches that ratio. |

In realtime mode, threshold-bounded deferral still preserves exact committed
query results because canonical reads use a bounded base-plus-delta overlay or
perform required foreground maintenance.

## Eventual Consistency Parameters

These parameters only affect foreground read behavior when
`consistency = 'eventual'`. That explicit consistency setting prevents
accidental weakening of realtime semantics. They do not decide when the
background worker rebuilds the full index; use `auto_rebuild_threshold` or
`auto_rebuild_delta_bytes` for that.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `query_overlay_max_records` | `50000` | Maximum pending delta records that a query may merge inline under the eventual policy. `0` disables inline overlay. |
| `query_overlay_max_bytes` | `16777216` | Maximum pending delta bytes that a query may merge inline under the eventual policy. `0` disables inline overlay. |

Query-time overlay is only used when the current generation records an
append-only delta start block. Older on-disk storage layouts are not
query-compatible in this test line; health checks mark them
`unsupported_storage_layout` / `rebuild_required`, and the index must be
rebuilt from the heap into the current append-only generation layout.

## Shared-Preload Parameters

These parameters only affect optional shared-preload residency. They do not
change BM25 scoring or consistency semantics.

| Parameter | Default | Meaning |
| --- | --- | --- |
| `auto_preload` | `0` | Best-effort automatic shared-preload priority. `0` disables background preload only. Positive values mark the index for background preload when `shared_preload_libraries` includes `psql_bm25s` and `psql_bm25s.shared_generation_cache_size` is positive. Larger values are attempted first; equal values have no ordering guarantee. When the shared-preload arena is configured, share-capable frontend queries require a shared generation: they wait for or create a shared-preload publication and only error if shared publication is not possible, instead of privately cold-loading one copy per backend. |

Background maintenance is controlled by global GUCs, not per-index reloptions:

| GUC | Default | Meaning |
| --- | --- | --- |
| `psql_bm25s.maintenance_worker_limit` | `1` | Extension-level background maintenance worker cap, clamped to PostgreSQL `max_worker_processes`. Each worker maintains at most one due index. |
| `psql_bm25s.preload_timer_interval_ms` | `1000` | Shared-preload warmup interval. This is independent from maintenance throttling so startup drains all currently due marked `auto_preload` indexes before rebuild catch-up. Values below `1000` are clamped to one second. |
| `psql_bm25s.maintenance_timer_interval_ms` | `60000` | Background rebuild/catch-up interval. Without `shared_preload_libraries`, frontend touch wakeups use the same value as their cooldown. Values below `1000` are clamped to one second. |
| `psql_bm25s.maintenance_rebuild_memory_budget` | `32768MB` | Automatic rebuild admission guard. A worker first admits the standard in-memory builder only when its estimate is comfortably below the budget and the active payload is small enough for the in-memory path, otherwise admits the compact in-memory builder for smaller medium payloads, then the spill builder. If none fit, it skips with `reason=memory_budget` instead of forcing the host into swap. `0` disables the guard. |

The shared-preload resident registry is sized automatically from
`psql_bm25s.shared_generation_cache_size`; it is not a separate preload count
limit. The arena size remains the real admission control for large indexes.
On Linux, the extension also gives the shared-preload arena a best-effort
`MADV_HUGEPAGE` hint. Set
`/sys/kernel/mm/transparent_hugepage/shmem_enabled=advise` if the deployment
should let resident generations use shared transparent huge pages and reduce
fresh-backend page-table fault cost.

These are global `SIGHUP` GUCs. Configure them in PostgreSQL configuration and
reload or restart the server; they are intentionally not per-index knobs.

Example configuration for the recommended shared-preload deployment:

```conf
shared_preload_libraries = 'psql_bm25s'
psql_bm25s.shared_generation_cache_size = '64GB'
psql_bm25s.maintenance_worker_limit = 1
psql_bm25s.preload_timer_interval_ms = 1000
psql_bm25s.maintenance_timer_interval_ms = 60000
psql_bm25s.maintenance_rebuild_memory_budget = '32768MB'
```

Mark hot indexes for proactive startup residency with:

```sql
ALTER INDEX docs_bm25_idx SET (auto_preload = 1);
```

`auto_preload` affects preload priority only. It does not change freshness,
scoring, or the rebuild builder selection.

The memory budget is a safety gate, not a guarantee that large indexes will be
maintained automatically. The current automatic choices are:

| Builder | Automatic condition | Notes |
| --- | --- | --- |
| `standard` | `standard_estimated_bytes <= budget_bytes * 0.60` and active payload is below the standard payload cap. | Fast path for genuinely small indexes with large headroom. |
| `compact` | Standard was rejected, `compact_estimated_bytes <= budget_bytes * 0.75`, and active payload is below the compact payload cap. | Lower-memory in-memory path for smaller medium indexes. |
| `spill` | Compact was rejected and `spill_estimated_bytes <= budget_bytes`. | Safe large-index path; spills term entries to PostgreSQL temp files and streams publish. |
| skip | Spill estimate also exceeds `budget_bytes`. | Returns `reason=memory_budget` and leaves the readable resident generation in place. |

The standard and compact payload caps are hard-coded safety rails. The current
code uses a `1GB` active-payload cap for standard and a `512MB` cap for
compact. Larger payloads go directly to spill when the spill estimate fits.

The estimate fields use coarse multipliers over active payload bytes:

| Estimate field | Formula |
| --- | --- |
| `standard_estimated_bytes` | `active_payload_bytes * 6` |
| `compact_estimated_bytes` | `active_payload_bytes * 4` |
| `spill_estimated_bytes` | `active_payload_bytes * 2` |

The extra `40%` standard-builder and `25%` compact-builder budget headroom is
intentional: rebuild RSS includes allocator fragmentation, PostgreSQL executor
state, shared-buffer effects, and resident shared-preload pressure that are
not perfectly captured by the coarse payload multipliers. The spill builder
writes compact `(term_id, doc_id, tf)` entries to PostgreSQL temporary files
and streams the final serialized payload directly into staged generation pages,
so the full `index_bytes` copy is not allocated during online maintenance or
explicit `CREATE INDEX` / `REINDEX` once a low-memory builder is selected.
With the default `32768MB` budget, PubMed/arXiv-sized indexes are expected to
use spill; if even the spill estimate exceeds the budget, automatic workers
skip safely.

Explicit `CREATE INDEX` / `REINDEX` uses the same estimates and headroom for
builder choice, but it does not skip permanently when no builder fits. It emits
a `NOTICE` and uses the spill builder because the operator explicitly requested
a controlled rebuild.

Use the `standard_estimated_bytes`, `compact_estimated_bytes`, and
`spill_estimated_bytes` fields in a `psql_bm25s_index_try_maintain(...)` result
to size the setting. A budget below all estimates means automatic maintenance
will not rebuild that index; choose a larger value only if the host has enough
headroom after shared buffers, the shared generation arena, active query memory,
and the operating system cache.

The same fields are also visible through:

```sql
SELECT psql_bm25s_generation_cache_state('docs_bm25_idx'::regclass);
```

See [Index Policy](index-policy.md) for the operational behavior of these
parameters, their policy-specific validity rules, and minimal scheduler
guidance.
