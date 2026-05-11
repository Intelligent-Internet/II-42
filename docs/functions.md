# Functions

It is ordered by what application users normally need first:

- top-k retrieval APIs
- SQL composition and operator helpers
- field-aware single-index multicolumn search
- multi-index, multi-field, and hybrid fusion
- index maintenance and operational diagnostics
- local scalar/token helpers
- result types and PostgreSQL catalog support functions

Exposure levels used below:

- `public`: supported user-facing SQL API.
- `ops`: supported administrative or diagnostic API.
- `support`: PostgreSQL operator, type, or access-method support surface. It
  may still be SQL-visible because PostgreSQL resolves it through catalogs, but
  users should not call it directly.

For behavior-level guidance, start with [API Reference](api-reference.md) and
[Query Semantics](query-semantics.md). This page is the full function inventory.

## Exact BM25 Retrieval

These are the main rowset-returning query APIs. They are the primary surface
applications should use for top-k retrieval.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_query` | `(index_name regclass, query_text text, k int4 = 10, weight_mask real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `SETOF psql_bm25s_result_hit` | Raw text query using index text options, or explicit query-time options when supplied. | Text-backed table indexes. |
| `psql_bm25s_query_tokens` | `(index_name regclass, query_tokens text[], k int4 = 10, weight_mask real[] = NULL)` | `SETOF psql_bm25s_result_hit` | Exact BM25 top-k over token text. | `text[]` / `varchar[]` table indexes and simple fusion indexes. |
| `psql_bm25s_query_ids` | `(index_name regclass, query_ids int4[], k int4 = 10, weight_mask real[] = NULL)` | `SETOF psql_bm25s_result_hit` | Exact BM25 top-k over token ids. | `int4[]` table indexes. |
| `psql_bm25s_query_prepared` | `(prepared_query psql_bm25s_result_prepared_query, k int4 = 10, weight_mask real[] = NULL)` | `SETOF psql_bm25s_result_hit` | Execute a prepared query value. | Prepared-query retrieval. |

## Operators And Prepared Query Builders

Use these when SQL needs index-bound query values, `@@@` predicates, or
`<=>` ordering composition.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_prepared_query` | `(index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_prepared_query` | Build an index-bound prepared query value. | Query construction. |
| `psql_bm25s_ranked_query` | `(prepared_query psql_bm25s_result_prepared_query, k int4 = 10, weight_mask real[] = NULL)`<br>`(index_name regclass, query_text text, k int4 = 10, weight_mask real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_ranked_query` | Package prepared query, order tokens, top-k, and mask.<br>Build ranked query from raw text. | Filtered/ranked SQL. |
| `psql_bm25s_filter_query` | `(ranked_query psql_bm25s_result_ranked_query)` | `psql_bm25s_result_prepared_query` | Extract the predicate side of a ranked query. | `@@@` predicate helpers. |
| `psql_bm25s_order_tokens` | `(prepared_query psql_bm25s_result_prepared_query)`<br>`(ranked_query psql_bm25s_result_ranked_query)`<br>`(index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `text[]` | Produce order tokens from a prepared query.<br>Extract stored order tokens.<br>Produce order tokens from raw text and index options. | `<=>` ordering. |

Operator-facing SQL usually looks like:

- `tokens @@ 'query text'`
- `tokens @@@ psql_bm25s_prepared_query(...)`
- `tokens @@@ psql_bm25s_filter_query(psql_bm25s_ranked_query(...))`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(...) ASC LIMIT k`

## Field-Aware Query

These functions target `field_aware = true` multicolumn indexes. They
search one BM25 payload with field-scoped terms and query-time field weights.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_field_aware_query_tokens` | `(index_name regclass, query_tokens text[], field_names text[], weights real[] = NULL, k int4 = 10)` | `SETOF psql_bm25s_result_hit` | Search a field-aware index with field subset and weights. | `field_aware = true` multicolumn indexes. |
| `psql_bm25s_field_aware_query` | `(index_name regclass, query_text text, field_names text[], weights real[] = NULL, k int4 = 10)` | `SETOF psql_bm25s_result_hit` | Raw-text wrapper over field-aware search. | `field_aware = true` multicolumn indexes. |

## Multi-Index / Multi-Field Fusion

These functions combine result sets from multiple BM25 indexes or fields.
Fusion happens after each source has produced query-scoped hits.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_fusion` | `(left_hits psql_bm25s_result_hit[], left_weight real, right_hits psql_bm25s_result_hit[], right_weight real, k int4 = NULL)`<br>`(hits psql_bm25s_result_hit[], k int4 = NULL)` | `SETOF psql_bm25s_result_hit` | Weighted score fusion of two materialized hit sets.<br>Sort and truncate one hit set. | Late fusion. |
| `psql_bm25s_fusion_weighted_query` | `(prepared_query psql_bm25s_result_prepared_query, weight real = 1.0)`<br>`(index_name regclass, query_text text, weight real = 1.0, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_fusion_weighted_query` | Attach a weight to a prepared query.<br>Build weighted query from raw text. | Multi-query fusion. |
| `psql_bm25s_fusion_weighted_queries` | `(index_names regclass[], query_text text, weights real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_fusion_weighted_query[]` | Build weighted queries for multiple indexes. | Multi-query fusion. |
| `psql_bm25s_fusion_field_query` | `(field_name text, weighted_query psql_bm25s_result_fusion_weighted_query)`<br>`(field_name text, prepared_query psql_bm25s_result_prepared_query, weight real = 1.0)`<br>`(field_name text, index_name regclass, query_text text, weight real = 1.0, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_fusion_field_query` | Attach a field name to a weighted query.<br>Build field query from prepared query.<br>Build field query from raw text. | Field-labeled fusion. |
| `psql_bm25s_fusion_field_queries` | `(field_names text[], index_names regclass[], query_text text, weights real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `psql_bm25s_result_fusion_field_query[]` | Build field queries for multiple indexes. | Field-labeled fusion. |
| `psql_bm25s_fusion_query_weighted` | `(weighted_queries psql_bm25s_result_fusion_weighted_query[], k int4 = 10, candidate_k int4 = NULL, weight_mask real[] = NULL)`<br>`(weighted_queries psql_bm25s_result_fusion_weighted_query[], query_text text, k int4 = 10, candidate_k int4 = NULL, weight_mask real[] = NULL)` | `SETOF psql_bm25s_result_hit` | Execute weighted queries and fuse hits.<br>Execute weighted queries with replaced query text. | Multi-index search. |
| `psql_bm25s_fusion_query_fields` | `(field_queries psql_bm25s_result_fusion_field_query[], k int4 = 10, candidate_k int4 = NULL, weight_mask real[] = NULL)` | `SETOF psql_bm25s_result_hit` | Execute field-labeled queries and fuse hits. | Multi-field search. |
| `psql_bm25s_fusion_query` | `(index_names regclass[], query_text text, weights real[] = NULL, k int4 = 10, candidate_k int4 = NULL, weight_mask real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(field_names text[], index_names regclass[], query_text text, weights real[] = NULL, k int4 = 10, candidate_k int4 = NULL, weight_mask real[] = NULL, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `SETOF psql_bm25s_result_hit` | Convenience multi-index search for shared query text.<br>Convenience multi-index search with field labels. | Multi-index search. |

## Hybrid Vector / BM25 Fusion

Hybrid fusion accepts generic candidate rows and therefore does not require
`pgvector`, VectorChord, or any vector type at extension install time.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_hybrid_candidate` | `(source_name text, ctid tid, raw_value real, source_rank int4, weight real = 1.0, normalizer text = 'identity', direction text = 'higher_is_better')` | `psql_bm25s_result_hybrid_candidate` | Build a generic hybrid candidate. | Hybrid fusion. |
| `psql_bm25s_hybrid_bm25_candidate` | `(source_name text, ctid tid, score real, source_rank int4, weight real = 1.0, normalizer text = 'identity')` | `psql_bm25s_result_hybrid_candidate` | Build a BM25 score candidate. | Hybrid fusion. |
| `psql_bm25s_hybrid_vector_candidate` | `(source_name text, ctid tid, distance real, source_rank int4, weight real = 1.0, normalizer text = 'negative_distance')` | `psql_bm25s_result_hybrid_candidate` | Build a vector distance candidate. | Hybrid fusion. |
| `psql_bm25s_hybrid_bm25_candidates` | `(source_name text, index_name regclass, query_text text, weight real = 1.0, candidate_k int4 = 100, normalizer text = 'identity', lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `SETOF psql_bm25s_result_hybrid_candidate` | Generate hybrid candidates from BM25 retrieval. | Hybrid fusion. |
| `psql_bm25s_hybrid_fuse_candidates` | `(candidates psql_bm25s_result_hybrid_candidate[], k int4 = 10, fusion text = 'rrf', rrf_k real = 60.0, epsilon real = 0.000001)` | `SETOF psql_bm25s_result_hybrid_hit` | Fuse BM25/vector/rank candidates. | Hybrid fusion. |

## Index Maintenance / Operational Diagnostics

Use these to inspect index state, maintain mutable indexes, or prewarm shared
generation cache state for connection pools.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_index_details` | `(index_name regclass)` | `TABLE(index_name regclass, source_type text, docs int8, index_bytes int8, pages int8, stale bool, consistency text, rebuilds int8, pending_writes int8, pending_deletes int8, delta_records int8, delta_bytes int8, auto_rebuild_threshold int4, auto_rebuild_delta_bytes int4, auto_rebuild_churn_ratio float8, query_overlay_max_records int4, query_overlay_max_bytes int4)` | Inspect physical index state, consistency mode, pending maintenance debt, and active policy parameters. | Diagnostics/ops. |
| `psql_bm25s_index_policy_recommend` | `(index_name regclass, profile text = 'balanced')` | `TABLE(index_name regclass, profile text, confidence text, recommended_options text, recommended_consistency text, recommended_auto_rebuild_threshold int4, recommended_auto_rebuild_delta_bytes int4, recommended_auto_rebuild_churn_ratio float8, matches_current bool, refresh_now bool, docs int8, pending_total int8, reason text)` | Recommend policy settings for a named workload profile. | Ops helper. |
| `psql_bm25s_index_refresh` | `(index_name regclass)` | `text` | Force refresh/rebuild. | Manual maintenance. |
| `psql_bm25s_index_maintain` | `(index_name regclass)` | `text` | Run maintenance. | Manual/scheduled maintenance. |
| `psql_bm25s_index_try_maintain` | `(index_name regclass)` | `text` | Try-lock maintenance primitive. | Scheduler-safe maintenance. |
| `psql_bm25s_index_maintain_due` | `(max_indexes integer = 1)` | `TABLE(index_oid regclass, result text)` | Maintain due indexes. | Scheduler/pg_cron integration. |
| `psql_bm25s_generation_cache_preload` | `(index_name regclass)` | `text` | Preload index payload into generation cache. | Connection-pool warmup. |
| `psql_bm25s_generation_cache_state` | `(index_name regclass)` | `text` | Inspect generation cache state. | Cache diagnostics. |
| `psql_bm25s_generation_cache_clear` | `()` | `int4` | Clear generation cache. | Cache ops/debugging. |

## Local Match / Score Helpers

These helpers evaluate one supplied document value at a time. They are useful
for diagnostics, SQL composition, and scalar text handling outside an index
scan, but they do not run index top-k retrieval.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_match_query` | `(doc_tokens text[], index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_tokens varchar[], index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_text text, index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_text varchar, index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `boolean` | Match one document value with index-bound query options. | Local non-top-k helper. |
| `psql_bm25s_match_prepared_query` | `(doc_tokens text[], prepared_query)`<br>`(doc_tokens varchar[], prepared_query)`<br>`(doc_text text, prepared_query)`<br>`(doc_text varchar, prepared_query)` | `boolean` | Match one document value against a prepared query. | Local non-top-k helper. |
| `psql_bm25s_score_query` | `(doc_tokens text[], index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_tokens varchar[], index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_text text, index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)`<br>`(doc_text varchar, index_name regclass, query_text text, lowercase bool = NULL, stopwords text[] = NULL, stem_english bool = NULL, fold_diacritics bool = NULL)` | `float8` | Score one document value with index-bound query options. | Local non-top-k helper. |
| `psql_bm25s_score_prepared_query` | `(doc_tokens text[], prepared_query)`<br>`(doc_tokens varchar[], prepared_query)`<br>`(doc_text text, prepared_query)`<br>`(doc_text varchar, prepared_query)` | `float8` | Score one document value against a prepared query. | Local non-top-k helper. |

## Tokenization / Normalization

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_tokenize_text` | `(input_text text, lowercase bool = true, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)` | `text[]` | Tokenize scalar text. | Tokenization helper. |
| `psql_bm25s_normalize_tokens` | `(tokens text[], lowercase bool = true, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(tokens varchar[], lowercase bool = true, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)` | `text[]` | Normalize token arrays.<br>Normalize varchar token arrays. | Tokenization helper. |

## Highlight / Snippet

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_highlight` | `(doc_tokens text[], query_text text, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_tokens varchar[], query_text text, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_text text, query_text text, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_text varchar, query_text text, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)` | `text` | Highlight matches for token arrays or scalar text. | UI helper. |
| `psql_bm25s_snippet` | `(doc_tokens text[], query_text text, max_tokens int4 = 24, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_tokens varchar[], query_text text, max_tokens int4 = 24, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_text text, query_text text, max_tokens int4 = 24, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)`<br>`(doc_text varchar, query_text text, max_tokens int4 = 24, start_tag text = '<b>', end_tag text = '</b>', lowercase bool = false, stopwords text[] = NULL, stem_english bool = false, fold_diacritics bool = false)` | `text` | Build highlighted snippets for token arrays or scalar text. | UI helper. |

## Result Types

These composite types are visible because the SQL APIs return or accept them.

| Type | Structure | Used By | Notes |
|---|---|---|---|
| `psql_bm25s_result_hit` | `(ctid tid, doc_id int4, score real)` | table query and fusion | Main rowset query result type. |
| `psql_bm25s_result_prepared_query` | `(index_name regclass, query_text text, lowercase bool, stopwords text[], stem_english bool, fold_diacritics bool)` | prepared query helpers | Carries query text plus index-bound tokenizer options. |
| `psql_bm25s_result_ranked_query` | `(prepared_query, order_tokens text[], k int4, weight_mask real[])` | filtered/ranked helpers | Bundles filter and order state. |
| `psql_bm25s_result_fusion_weighted_query` | `(prepared_query, weight real)` | weighted fusion | Query plus weight. |
| `psql_bm25s_result_fusion_field_query` | `(field_name text, weighted_query)` | field-labeled fusion helpers | Field label plus weighted query. |
| `psql_bm25s_result_hybrid_candidate` | `(source_name text, ctid tid, raw_value real, source_rank int4, weight real, normalizer text, direction text)` | hybrid fusion | Generic BM25/vector/rank candidate. |
| `psql_bm25s_result_hybrid_hit` | `(ctid tid, score real, source_count int4, source_names text[], raw_values real[], normalized_scores real[], weighted_scores real[], ranks int4[])` | hybrid fusion | Fused result plus source details. |

## Advanced Planner / Fast-Path Diagnostics

These functions are user-visible because they inspect SQL plans and index
eligibility, but they are diagnostics. They should not be treated as part of
the ordinary retrieval API flow.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_fast_path_advice` | `(index_name regclass)` | `jsonb` | Summarize supported API/operator paths for one index. | Advanced diagnostics. |
| `psql_bm25s_fast_path_plan` | `(index_name regclass, explain_plan jsonb)` | `jsonb` | Analyze an existing EXPLAIN JSON plan. | Advanced diagnostics. |
| `psql_bm25s_fast_path_explain` | `(index_name regclass, sql_text text)` | `jsonb` | Run EXPLAIN and analyze fast-path usage. | Advanced diagnostics. |

## Operator Support Functions

These functions back PostgreSQL operators and opclasses. They remain
catalog-visible, but users should normally call the operators or public query
APIs instead.

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_op_score_ids` | `(doc_ids int4[], query_ids int4[])` | `float8` | Score function behind ids `<=>`. | Operator support. |
| `psql_bm25s_op_score_tokens` | `(doc_tokens text[], query_tokens text[])`<br>`(doc_tokens text[], query_tokens varchar[])`<br>`(doc_tokens varchar[], query_tokens text[])`<br>`(doc_tokens varchar[], query_tokens varchar[])` | `float8` | Score function behind token `<=>`. | Operator support. |
| `psql_bm25s_op_score_scalar_text` | `(doc_text text, query_tokens text[])`<br>`(doc_text text, query_tokens varchar[])`<br>`(doc_text varchar, query_tokens text[])`<br>`(doc_text varchar, query_tokens varchar[])` | `float8` | Score function behind scalar text/varchar `<=>`. | Operator support. |
| `psql_bm25s_op_match_query_tokens` | `(doc_tokens text[], query_text text)`<br>`(doc_tokens varchar[], query_text text)` | `boolean` | Match function behind token-array `@@`. | Operator support. |
| `psql_bm25s_op_match_query_scalar` | `(doc_text text, query_text text)`<br>`(doc_text varchar, query_text text)` | `boolean` | Match function behind scalar text/varchar `@@`. | Operator support. |
| `psql_bm25s_op_match_prepared_query` | `(doc_tokens text[], prepared_query)`<br>`(doc_tokens varchar[], prepared_query)` | `boolean` | Match function behind token-array `@@@`. | Operator support. |
| `psql_bm25s_op_match_prepared_query_scalar` | `(doc_text text, prepared_query)`<br>`(doc_text varchar, prepared_query)` | `boolean` | Match function behind scalar text/varchar `@@@`. | Operator support. |

## Extension / Catalog Internals

Some objects cannot be hidden completely without a larger storage/type or
operator redesign. They remain catalog-visible because PostgreSQL resolves
access methods, custom types, operators, and opclasses through catalog entries,
but they are not user-facing APIs.

| Object | Current Status | Why It Remains | Future Plan |
|---|---|---|---|
| `psql_bm25s_handler` | `support` | PostgreSQL needs the access-method handler to create and use `USING psql_bm25s` indexes. | Keep catalog-visible; do not document as user API. |
| `psql_bm25s_in`, `psql_bm25s_out`, `psql_bm25s_recv`, `psql_bm25s_send` | `support` | PostgreSQL custom type I/O functions must be catalog-visible for `psql_bm25s_index`. | Keep until a larger storage/type cleanup removes or replaces the SQL type. |
| Operator support functions | `support` | `<=>`, `@@`, `@@@`, and opclasses resolve their backing functions through PostgreSQL catalogs. | Keep catalog-visible for now; consider internal naming/schema cleanup only if it does not complicate operator resolution. |
| `psql_bm25s_index` type | `support` | The serialized payload representation is still used by type I/O and historical upgrade paths. | Keep as an internal catalog type for now; revisit after raw payload SQL functions stay removed for a release cycle. |

Catalog-visible internal functions:

| Function | Parameters | Returns | Purpose | Scope |
|---|---|---|---|---|
| `psql_bm25s_handler` | `(internal)` | `index_am_handler` | PostgreSQL index access method handler. | Extension internal. |
| `psql_bm25s_in` | `(cstring)` | `psql_bm25s_index` | Text input for custom type. | Type internal. |
| `psql_bm25s_out` | `(psql_bm25s_index)` | `cstring` | Text output for custom type. | Type internal. |
| `psql_bm25s_recv` | `(internal)` | `psql_bm25s_index` | Binary input for custom type. | Type internal. |
| `psql_bm25s_send` | `(psql_bm25s_index)` | `bytea` | Binary output for custom type. | Type internal. |
