# Query Semantics

This page explains which surface defines the real BM25 contract and
which surfaces are convenience or planner integration layers.

For a complete SQL function inventory, see [Functions](functions.md).

## Canonical BM25 Surface

The canonical exact BM25 contract is defined by:

- `psql_bm25s_query_ids(...)`
- `psql_bm25s_query_tokens(...)`

These should be treated as the ground truth for:

- ranking semantics
- benchmark comparisons
- precise behavioral expectations

For supported source-column types and their trade-offs, see
[Supported Input Types](input-types.md).

## Raw Query Retrieval

`psql_bm25s_query(...)` is the SQL convenience surface for
text-backed indexes:

- `text[]`
- `varchar[]`
- `text`
- `varchar`

Supported behavior:

- exact token matching
- legacy `term`, `+required`, `-excluded`, `prefix*`
- precedence-aware `AND`, `OR`, `NOT`
- parenthesized grouping
- optional query-time normalization through helper arguments

Grouped boolean and phrase queries are exact, but may be slower because
they can use bounded verification over ranked candidates.

## Prepared Query Values

`psql_bm25s_prepared_query(...)` creates an explicit index-bound prepared query
value for the same raw-query surface used by
`psql_bm25s_query(...)`.

That prepared value can then be executed as top-k retrieval through:

- `psql_bm25s_query_prepared(...)`

It can also be reused by document-local helpers:

- `psql_bm25s_match_query(...)`
- `psql_bm25s_match_prepared_query(...)`
- `psql_bm25s_score_query(...)`
- `psql_bm25s_score_prepared_query(...)`

This is a structured SQL wrapper, not a new scoring model. Top-k
execution still belongs to the query functions. The local match and
score helpers reuse the same query parsing and normalization controls,
but they evaluate one provided document value at a time.

The local match and score helpers are document-local helpers. They are
useful for SQL composition and diagnostics, but they are not a
replacement for the canonical top-k retrieval path.

Use `psql_bm25s_query(...)` for direct raw-text retrieval, or
`psql_bm25s_query_prepared(...)` when a prepared query value is
already available.

For scalar `text` and `varchar`, these helper overloads are also the
clearest non-index-scan way to apply explicit query normalization
options to a raw document value. When a scalar index is named and those
options are omitted, index-bound helpers resolve them from the index
reloptions. `psql_bm25s_prepared_query(index_name, ...)` stores the concrete
values in the prepared-query payload, and
`psql_bm25s_query_prepared(...)` routes through that same
prepared-query path.

In particular, the local score helpers should be read as:

- explicit local scoring conveniences
- based on the current document-local token score path
- not a promise of exact corpus-level BM25 ranking semantics
- not a replacement for indexed top-k retrieval on large tables

## @@ Predicate

`tokens @@ 'query text'` is:

- a boolean document-match predicate
- local to one document
- not a BM25 ranking API

It supports:

- term
- required and excluded terms
- prefix
- phrase
- grouped boolean syntax

It can participate in index-aware filtering when PostgreSQL plans a
matching index path.

For scalar `text` and `varchar`, plain `@@` outside a real index scan is
still a local predicate over the operator's explicit query options. It
does not discover hidden index reloptions on its own. When those text
options matter, prefer:

- `column @@@ psql_bm25s_prepared_query(...)`
- `psql_bm25s_match_query(column, index_name, query_text, ...)`

Future scalar ergonomics may add an explicit default index binding for
plain `column @@ 'query text'`. That binding would be opt-in, attached to
one scalar `text` or `varchar` column, and would name exactly one
concrete `psql_bm25s` index. With such a binding, raw scalar `@@` could
reuse the existing prepared-query path and behave like:

- `column @@@ psql_bm25s_prepared_query(bound_index, 'query text')`

The important design constraint is that this must not become hidden
index guessing. The same column can have multiple `psql_bm25s` indexes
with different text reloptions, so any future binding must reject
ambiguous defaults, reject multicolumn fusion indexes, and remain
catalog-visible or metadata-visible. Without an explicit binding, raw
scalar `@@` should keep its current convenience-predicate semantics.

## @@@ Prepared Predicate

`tokens @@@ psql_bm25s_prepared_query(...)` is:

- a prepared-query boolean predicate
- bound to an explicit `psql_bm25s` index
- a more SQL-native raw-query entry surface than direct helper calls

For scalar `text` and `varchar`, omitted query options inherit the named
index's text reloptions at `psql_bm25s_prepared_query(index_name, ...)` creation
time. That makes the prepared-query path stable even when PostgreSQL
does not use a real index scan for the surrounding SQL.

Unlike `@@`, the right-hand side is not plain text. It is a
`psql_bm25s_result_prepared_query` value, which keeps the operator family
distinct and preserves index-aware filtering without pretending to be a
drop-in text predicate.

## <=> Ordered Retrieval

`<=>` is an ordered retrieval surface, not the canonical BM25 API in all
contexts.

When PostgreSQL chooses a real `psql_bm25s` index scan:

- `<=>` aligns to true BM25 ordering

Outside that access-method path:

- the fallback operator implementation is only a simplified
  overlap-based distance
- it is not the same thing as exact BM25 scoring

For scalar `text` and `varchar`, use:

- `ORDER BY column <=> psql_bm25s_order_tokens(...)`
- `psql_bm25s_score_query(column, index_name, query_text, ...)`

when you need an explicit SQL-local score with caller-specified text
options and are not relying on a real index scan.

When the caller already has a `psql_bm25s_result_prepared_query`, the intended
SQL-native bridge into ordered retrieval is:

- `ORDER BY tokens <=> psql_bm25s_order_tokens(prepared_query) ASC`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(index_name, query_text, ...) ASC`

That keeps the actual ordering operator on the existing `text[]` query
token path, instead of introducing a new order-only query type inside
the operator family.

So `<=>` should be understood as planner/operator integration, not the
definition of BM25 semantics everywhere.

Advanced planner diagnostics are available when a caller wants to inspect
whether a SQL shape can use, or did use, the intended access path. These
helpers are not search APIs and should not be part of normal application query
construction.

For a compact summary of which filter/rank shape is eligible for a given index,
use:

- `psql_bm25s_fast_path_advice(index_name)`

It reports the key type, the supported SQL surfaces, and the
recommended filter/order shape without changing planner behavior.

To inspect one concrete SQL shape, use:

- `psql_bm25s_fast_path_plan(index_name, explain_plan_json)`
- `psql_bm25s_fast_path_explain(index_name, sql_text)`

These helpers report whether the plan actually used a `psql_bm25s`
index node, whether the access path was ordered or bitmap-based, and
whether the plan observed `@@` / `@@@` / `<=>` conditions.

If a caller wants one structured SQL value that can drive the common
filtered/ranked shape, use:

- `psql_bm25s_ranked_query(index_name, query_text, ...)`

It bundles:

- a prepared filter query
- the corresponding order tokens
- the intended `k`
- the optional `weight_mask`

The intended SQL pattern is:

- `WHERE tokens @@@ psql_bm25s_filter_query(ranked_query)`
- `ORDER BY tokens <=> psql_bm25s_order_tokens(ranked_query) ASC`
- `LIMIT (ranked_query).k`

This is still a convenience layer over the existing exact retrieval and
ordering surfaces. It does not define a new scoring contract.

## Recommended Filtered Ranked SQL Shapes

For text-backed indexes, the recommended filtered/ranked SQL shapes are:

- plain-text filter plus order:
  - `WHERE tokens @@ 'query text'`
  - `ORDER BY tokens <=> psql_bm25s_order_tokens(index_name, query_text) ASC`
- prepared filter plus order:
  - `WHERE tokens @@@ psql_bm25s_prepared_query(index_name, query_text, ...)`
  - `ORDER BY tokens <=> psql_bm25s_order_tokens(index_name, query_text, ...) ASC`
- ranked bundle:
  - `WHERE tokens @@@ psql_bm25s_filter_query(ranked_query)`
  - `ORDER BY tokens <=> psql_bm25s_order_tokens(ranked_query) ASC`
  - `LIMIT (ranked_query).k`

For `int4[]` indexes, the recommended surface is ordered retrieval only:

- `ORDER BY ids <=> query_ids ASC`

The diagnostic way to confirm which shape applies to one index is:

- `psql_bm25s_fast_path_advice(index_name)`

The diagnostic way to confirm that one concrete query actually used the
expected index-aware shape is:

- `psql_bm25s_fast_path_explain(index_name, sql_text)`

The main anti-patterns are:

- row-by-row local scoring in place of canonical retrieval
- generic scalar score evaluation after a broad table scan
- SQL wrappers that hide whether the query still uses the `psql_bm25s`
  access path

Those anti-patterns are rejected because they make filtered/ranked SQL
look more compatible while weakening the exact fast path that defines
the extension's intended behavior.

## Score-Carrying SQL Results

When a caller needs "the rows for this query" and "the score for each
row" together, the preferred SQL surface is:

- `psql_bm25s_query(index_name, query_text, ...)`
- `psql_bm25s_query_prepared(prepared_query, ...)`

These return `psql_bm25s_result_hit` rows with:

- `ctid`
- `doc_id`
- `score`

That keeps scoring tied to the canonical top-k retrieval path instead of
reconstructing scores row by row later.

This is preferred over a hypothetical scalar `score(id)` surface
because:

- the score stays attached to the same exact retrieval path
- SQL does not have to reconstruct scores row by row
- planner behavior stays easier to reason about
- the API shape makes it clearer that the score is query-scoped

When an application needs to combine two query-scoped result sets, the
intended low-risk helper is:

- `psql_bm25s_fusion(left_hits, left_weight, right_hits, right_weight, k)`

This helper performs weighted score fusion after exact retrieval. It
operates only on already-materialized top-k hit rows, so it does
not alter the underlying index access path.

For a small number of field- or index-specific prepared queries, the
more structured helper is:

- `psql_bm25s_fusion_weighted_query(...)`
- `psql_bm25s_fusion_field_query(...)`
- `psql_bm25s_fusion_query_weighted(weighted_queries[], ...)`
- `psql_bm25s_fusion_query_fields(field_queries[], ...)`
- `psql_bm25s_fusion_weighted_queries(index_names[], query_text, weights, ...)`
- `psql_bm25s_fusion_field_queries(field_names[], index_names[], query_text, weights, ...)`
- `psql_bm25s_fusion_query(index_names[], query_text, weights, ...)`
- `psql_bm25s_fusion_query(field_names[], index_names[], query_text, weights, ...)`

This keeps field weighting explicit at the SQL surface:

- each field/index gets its own prepared query
- each query gets its own weight
- exact retrieval still happens per query
- fusion happens only after those top-k result sets have been produced

The `psql_bm25s_fusion_field_query(...)` family tightens that contract one step
further:

- each field keeps an explicit field name
- the field name is metadata only, not a hidden scoring signal
- field-aware composition stays structured instead of relying on
  positional parallel arrays alone

For concrete SQL examples of weighted multi-field search, see
[Multi-Field Search](multi-field-search.md).

## Hybrid Vector/BM25 Fusion

Hybrid search extends the same late-fusion model to non-BM25 sources. The
core extension still does not depend on vector extensions. Instead, vector
queries supply ordinary candidates with:

- `ctid`
- raw distance or similarity
- source rank
- source weight
- normalization metadata

`psql_bm25s_hybrid_fuse_candidates(...)` then combines those candidates with
BM25 candidates inside PostgreSQL.

The default fusion method is `rrf`, because it uses only source-local ranks
and weights. This avoids comparing raw BM25 scores with vector distances.
Advanced score fusion is available through explicit normalizers such as
`minmax`, `zscore`, `negative_distance`, and `inverse_distance`.

For the full API, VectorChord-style examples, and engine-level behavior, see
[Hybrid Vector/BM25 Search](hybrid-search.md) and
[Hybrid Fusion Engine](hybrid-fusion-engine.md).

## Query-Time Normalization

Optional normalization controls are explicit, not silent global defaults:

- `lowercase`
- `stopwords`
- `stem_english`
- `fold_diacritics`

That is intentional:

- index-bound helpers inherit the named index's text reloptions when
  these options are omitted
- callers opt into normalization policy explicitly
- benchmark semantics do not silently drift

For index-level scalar text defaults and reloptions, see
[Index Parameters](index-parameters.md#text-processing-parameters).
