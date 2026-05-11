# API Reference

This page groups the SQL surface by role instead of listing every
function in implementation order.

For the full signature inventory, result types, operational helpers, and
catalog-visible support functions, see [Functions](functions.md).

## Canonical Exact Retrieval

Preferred exact BM25 APIs:

- `psql_bm25s_query_ids(regclass, int4[], k, weight_mask)`
- `psql_bm25s_query_tokens(regclass, text[], k, weight_mask)`

These are the canonical exact BM25 interfaces and the main performance
path.

For supported source-column types and their trade-offs, see
[Supported Input Types](input-types.md).

They are also the recommended exact retrieval surface for multicolumn
fusion indexes. See
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md).

## Raw Query Retrieval

Convenience retrieval over text-backed indexes:

- `text[]`
- `varchar[]`
- `text`
- `varchar`

- `psql_bm25s_query(...)`

The optional tail arguments expose explicit query-time normalization:

- `lowercase`
- `stopwords`
- `stem_english`
- `fold_diacritics`

Prepared/index-bound query values:

- `psql_bm25s_prepared_query(...)`
- `psql_bm25s_ranked_query(...)`
- `psql_bm25s_fusion_weighted_query(...)`
- `psql_bm25s_fusion_field_query(...)`
- `psql_bm25s_fusion_weighted_queries(index_names[], query_text, weights, ...)`
- `psql_bm25s_fusion_field_queries(field_names[], index_names[], query_text, weights, ...)`
- `psql_bm25s_query_prepared(...)`
- `psql_bm25s_filter_query(ranked_query)`
- `psql_bm25s_fusion_query_weighted(weighted_queries[], k, candidate_k, weight_mask)`
- `psql_bm25s_fusion_query_fields(field_queries[], k, candidate_k, weight_mask)`
- `psql_bm25s_fusion_query(index_names[], query_text, weights, k, candidate_k, weight_mask, ...)`
- `psql_bm25s_fusion_query(field_names[], index_names[], query_text, weights, k, candidate_k, weight_mask, ...)`
- `psql_bm25s_field_aware_query_tokens(index_name, query_tokens, field_names, weights, k)`
- `psql_bm25s_field_aware_query(index_name, query_text, field_names, weights, k)`
- `psql_bm25s_order_tokens(prepared_query)`
- `psql_bm25s_order_tokens(ranked_query)`
- `psql_bm25s_order_tokens(index_name, query_text, ...)`

These let SQL bind a parsed query configuration to an explicit index
context without changing the canonical execution path.

For scalar `text` and `varchar` indexes, omitted query options in
index-bound raw-query helpers inherit the named index's text reloptions.
That includes `psql_bm25s_prepared_query(index_name, ...)`,
`psql_bm25s_query(...)`, and
`psql_bm25s_query_prepared(...)`. This
makes prepared-query predicates and helper functions stable even when the
surrounding SQL does not use a real index scan.

`psql_bm25s_ranked_query(...)` packages:

- a prepared/index-bound query
- the matching `<=>` order tokens
- the intended top-k
- an optional `weight_mask`

This is a convenience bundle for common filtered/ranked SQL shapes. It
does not change ranking semantics and it does not introduce a second
retrieval implementation.

For a full advanced example of weighted title/abstract/body search, see
[Multi-Field Search](multi-field-search.md).

For one-index fused `text[]` retrieval across multiple indexed columns,
see [Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md).

For one-index field-aware retrieval over a multicolumn index
created with `field_aware = true`, generic direct APIs search all fields
with equal weight for token and simple raw term queries. Use
`psql_bm25s_field_aware_query_tokens(...)` or
`psql_bm25s_field_aware_query(...)` when a query needs custom
field weights or a field subset. The field-aware engine stores
field-scoped terms in one BM25 payload, but it does not implement BM25F
or per-field length normalization. See
[Field-Aware Indexes](field-aware-indexes.md).

For BM25/vector late fusion that keeps vector extensions optional, see
[Hybrid Vector/BM25 Search](hybrid-search.md) and
[Hybrid Fusion Engine](hybrid-fusion-engine.md).

For type-specific usage guidance, see
[Supported Input Types](input-types.md).

Recommended score-carrying SQL surface:

- `psql_bm25s_query(...)`
- `psql_bm25s_query_prepared(...)`
- `psql_bm25s_fusion(left_hits, left_weight, right_hits, right_weight, k)`

These return `psql_bm25s_result_hit`, which carries:

- `ctid`
- `doc_id`
- `score`

The intended pattern is to join `ctid` back to application rows when a
query needs both row data and the query-time score.

This is preferred over a hypothetical scalar `score(id)` API because:

- retrieval and scoring stay on the same exact top-k path
- scores remain attached to the current query execution
- SQL avoids row-by-row re-scoring outside the retrieval executor
- planner behavior stays easier to reason about

For limited multi-field or multi-index score fusion, use:

- `psql_bm25s_fusion(...)`
- `psql_bm25s_fusion_weighted_query(...)`
- `psql_bm25s_fusion_field_query(...)`
- `psql_bm25s_fusion_query_weighted(...)`
- `psql_bm25s_fusion_query_fields(...)`

These helpers return `SETOF psql_bm25s_result_hit`. Fusion is post-retrieval
composition of query-scoped hit rows, not a replacement for the
underlying exact retrieval path.

`psql_bm25s_fusion_query_weighted(...)` is the more structured helper
for a small set of field- or index-specific prepared queries with
weights. It runs exact retrieval per query, then fuses the top-k result
sets by weighted score sum.

`psql_bm25s_fusion_field_query(...)` and
`psql_bm25s_fusion_query_fields(...)` make the field-labeled fusion contract
more explicit:

- each field has a stable field name
- each field still owns one weighted prepared query
- retrieval stays per field/index
- fusion still happens only after those top-k results exist

`psql_bm25s_fusion_weighted_queries(...)`, `psql_bm25s_fusion_field_queries(...)`,
and `psql_bm25s_fusion_query(...)` are the convenience layer for the
common case where multiple field indexes share the same query text and
normalization options.

For end-to-end examples, see
[Multi-Field Search](multi-field-search.md) and
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md).

For the opt-in single-index field-aware engine, see
[Field-Aware Indexes](field-aware-indexes.md).

## Hybrid Vector/BM25 Fusion

Hybrid fusion accepts generic candidate rows and therefore does not require
`pgvector`, VectorChord, or any vector type at extension install time.

Types:

- `psql_bm25s_result_hybrid_candidate`
- `psql_bm25s_result_hybrid_hit`

Candidate constructors:

- `psql_bm25s_hybrid_candidate(source_name, ctid, raw_value, source_rank, weight, normalizer, direction)`
- `psql_bm25s_hybrid_bm25_candidate(source_name, ctid, score, source_rank, weight, normalizer)`
- `psql_bm25s_hybrid_vector_candidate(source_name, ctid, distance, source_rank, weight, normalizer)`
- `psql_bm25s_hybrid_bm25_candidates(source_name, index_name, query_text, weight, candidate_k, normalizer, ...)`

Fusion:

- `psql_bm25s_hybrid_fuse_candidates(candidates, k, fusion, rrf_k, epsilon)`

The public fusion function is C-backed and is the supported API surface.

See [Hybrid Fusion Engine](hybrid-fusion-engine.md) for the execution model,
performance boundary, and validation coverage.

Supported fusion methods:

- `rrf`
- `score`

Supported score normalizers:

- `identity`
- `negative_distance`
- `inverse_distance`
- `minmax`
- `zscore`
- `rank`

Use `rrf` as the default for mixed BM25/vector retrieval. Use `score` only
when the application has chosen and benchmarked a normalization strategy.

## Operators

Document predicate:

- `tokens @@ 'query text'`
- `tokens @@@ psql_bm25s_prepared_query(...)`
- `tokens @@@ psql_bm25s_filter_query(psql_bm25s_ranked_query(...))`
- `column @@ 'query text'`
- `column @@@ psql_bm25s_prepared_query(...)`

Ordered retrieval surface:

- `ORDER BY tokens <=> ... ASC LIMIT k`
- `ORDER BY token_ids <=> ... ASC LIMIT k`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(psql_bm25s_prepared_query(...))`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(psql_bm25s_ranked_query(...))`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(index_name, query_text, ...)`

Important:

- `@@` is a boolean document-match predicate
- `@@@` is the prepared-query boolean predicate
- if you want prepared-query filtering and SQL-native ranking together,
  use `@@@` plus `psql_bm25s_order_tokens(prepared_query)`
- `@@` is useful for filtering, not for ranking
- for scalar `text` and `varchar`, raw `@@` outside a real index scan
  does not discover hidden index reloptions on its own
- for scalar `text` and `varchar`, `@@@ psql_bm25s_prepared_query(...)`
  inherits omitted query options from the named index reloptions
- `<=>` is only true BM25 ordering when PostgreSQL chooses a real
  `psql_bm25s` index scan
- if you need the clearest exact BM25 contract regardless of planner
  shape, prefer `psql_bm25s_query_tokens(...)` or
  `psql_bm25s_query_ids(...)`

## Local Scalar / Token Helpers

These helpers operate on one provided document value at a time. They are
useful for SQL composition, diagnostics, UI rendering, and explicit scalar
text handling outside an index scan. They do not run index top-k retrieval
and should not be used as the primary search path for large tables.

- `psql_bm25s_tokenize_text(text, ...)`
- `psql_bm25s_normalize_tokens(text[], ...)`
- `psql_bm25s_match_prepared_query(text[], psql_bm25s_result_prepared_query)`
- `psql_bm25s_match_prepared_query(text, psql_bm25s_result_prepared_query)`
- `psql_bm25s_match_prepared_query(varchar, psql_bm25s_result_prepared_query)`
- `psql_bm25s_match_query(text, index_name, query_text, ...)`
- `psql_bm25s_match_query(varchar, index_name, query_text, ...)`
- `psql_bm25s_score_prepared_query(text[], psql_bm25s_result_prepared_query)`
- `psql_bm25s_score_prepared_query(text, psql_bm25s_result_prepared_query)`
- `psql_bm25s_score_prepared_query(varchar, psql_bm25s_result_prepared_query)`
- `psql_bm25s_score_query(text, index_name, query_text, ...)`
- `psql_bm25s_score_query(varchar, index_name, query_text, ...)`
- `psql_bm25s_highlight(text[], text, ...)`
- `psql_bm25s_highlight(text, text, ...)`
- `psql_bm25s_highlight(varchar, text, ...)`
- `psql_bm25s_snippet(text[], text, ...)`
- `psql_bm25s_snippet(text, text, ...)`
- `psql_bm25s_snippet(varchar, text, ...)`

Important:

- the local match and score helpers are document-local convenience helpers
- they use the existing local token match/score paths
- they do not access an index payload for top-k retrieval
- they are not the canonical exact BM25 retrieval contract
- wrappers that take `index_name, query_text, ...` are thin convenience
  layers over prepared-query helpers, not a separate scoring model
- when scalar text options are omitted, `index_name` overloads resolve them
  from the named index reloptions and then apply the local helper path

## Introspection and Maintenance

- `psql_bm25s_index_details(regclass)`
- `psql_bm25s_index_policy_recommend(regclass, profile text)`
- `psql_bm25s_index_refresh(regclass)`
- `psql_bm25s_index_maintain(regclass)`
- `psql_bm25s_index_try_maintain(regclass)`
- `psql_bm25s_index_maintain_due(max_indexes integer DEFAULT 1)`
- `psql_bm25s_generation_cache_clear()`
- `psql_bm25s_generation_cache_state(regclass)`
- `psql_bm25s_generation_cache_preload(regclass)`

`psql_bm25s_index_maintain(...)` is intended for scheduled convergence of
pending or stale indexes. It no-ops when no maintenance is needed.

`psql_bm25s_index_try_maintain(...)` is the non-blocking scheduler primitive.
For query-first eventual indexes, it builds a replacement payload first and
takes only a short non-blocking swap lock. If the lock is busy, it returns a
retryable no-op result. Append-only delta records created while the replacement
was building are carried forward after the swap; non-tail-compatible concurrent
changes still return a retryable no-op result.

`psql_bm25s_index_maintain_due(...)` scans due `eventual` indexes owned by
the current role, then calls the same try-maintenance primitive. It is suitable
for pg_cron, systemd timers, or application schedulers. It prioritizes stale
indexes first, then larger record and byte maintenance debt.

`psql_bm25s_index_details(...)` is the structured inspection surface for index
metadata, persisted maintenance state, consistency mode, and active reloptions.

`psql_bm25s_index_policy_recommend(...)` returns a structured recommendation
for a workload profile. Use it as planning guidance; it does not change the
index.

`psql_bm25s_generation_cache_state(...)` reports the observable immutable
generation key and shared-cache state for one index, including DSM descriptor
state, optional shared-preload arena counters, and whether that specific index
is resident or currently loading in the shared-preload arena. Use it for
debugging shared generation reuse and invalidation, not in latency-sensitive
query paths.
The shared-preload counters intentionally separate background worker slots from
current phase: `active_background_workers` is the total worker-slot usage,
while `active_preload_workers` and `active_index_maintenance_workers` show
whether active workers are warming resident generations or rebuilding indexes.

`psql_bm25s_generation_cache_clear()` clears backend-local cache state and
best-effort volatile shared-generation descriptors, failure markers,
interrupted temp descriptors, old lock files, and shared-preload registry
entries. It does not modify durable index contents; later readers can rebuild
from the index relation.

## Advanced Diagnostics

These functions are for inspecting planner behavior and index eligibility.
They are not retrieval APIs and should not be presented as part of the
ordinary query flow:

- `psql_bm25s_fast_path_advice(index_name)`
- `psql_bm25s_fast_path_plan(index_name, explain_plan_json)`
- `psql_bm25s_fast_path_explain(index_name, sql_text)`

`psql_bm25s_fast_path_advice(index_name)` returns a JSON summary of:

- the index key type
- which predicate/order surfaces are supported
- whether filtered ranked SQL is eligible
- the canonical API
- the recommended SQL filter/order shape

`psql_bm25s_fast_path_plan(...)` and
`psql_bm25s_fast_path_explain(...)` report whether a concrete plan
actually used:

- a `psql_bm25s` index node
- bitmap versus ordered index scan
- `@@` / `@@@`-style match predicates
- `<=>` ordering inside the plan

`psql_bm25s_generation_cache_preload(...)` opens the index and warms the best
available generation-cache tier for the current deployment. In a configured
shared-preload deployment it can populate the main shared-memory arena before
application traffic reaches an index; otherwise it warms the DSM tier for
share-eligible generations or the selected backend-local path for small
indexes.

Indexes can also set `auto_preload = <priority>` as a reloption. The default
priority is `0`, which disables automatic preload. Positive priorities are
best-effort hints consumed by the shared-preload background worker; larger
values are attempted first.
Warmup uses `psql_bm25s.preload_timer_interval_ms`, drains all currently due
marked indexes per cycle, and is intentionally independent from
`psql_bm25s.maintenance_timer_interval_ms` so rebuild throttling does not slow
startup residency.
Automatic rebuilds are separately guarded by
`psql_bm25s.maintenance_rebuild_memory_budget`. Maintenance reports
`builder=standard`, `builder=compact`, or `builder=spill` when a rebuild is
admitted. Automatic workers choose `standard` only when
`standard_estimated_bytes <= budget_bytes * 0.60` and the active payload is
below the standard payload cap, choose `compact` only when
`compact_estimated_bytes <= budget_bytes * 0.75` and the active payload is
below the compact payload cap, and otherwise choose `spill` when
`spill_estimated_bytes <= budget_bytes`. If even spill does not fit,
maintenance reports `reason=memory_budget` with the estimate fields and leaves
the readable resident generation in place instead of risking swap pressure.
Explicit `CREATE INDEX` / `REINDEX` uses the same estimates for builder choice,
but falls through to `spill` with a `NOTICE` because the operator asked for a
controlled rebuild.

See [Index Policy](index-policy.md) for consistency modes, maintenance
behavior, and scheduler guidance.
See [Shared Generation Cache](shared-generation-cache.md) for cache tiers,
large connection-pool deployment guidance, and the optional shared-preload
arena.
See [Connection Memory and Index Prewarming](connection-memory.md) for
workspace retention settings, memory sizing, and active warmup examples.
See [Index Parameters](index-parameters.md) for the complete
`CREATE INDEX ... WITH (...)` option reference.

## Policy Recommendation Profiles

Current helper profiles:

- `query_first`
- `balanced`
- `small_mixed_churn`
- `heavy_mixed_churn`
- `heavy_insert_skew`
- `longrun_mixed_churn`
- `write_tolerant_query_first`
