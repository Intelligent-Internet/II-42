# Multi-Column Fusion Indexes

This page describes the current multicolumn `psql_bm25s` index feature.

The goal is to support one BM25 index over several homogeneous
text-like columns without turning the feature into a hidden tax on the
existing single-column hot path.

## What This Feature Is

A multicolumn fusion index lets one `psql_bm25s` index treat multiple
`text[]`, `varchar[]`, `text`, or `varchar` columns as one BM25
document.

Example:

```sql
CREATE TABLE docs (
    id bigint PRIMARY KEY,
    title_tokens text[],
    body_tokens text[]
);

CREATE INDEX docs_fused_bm25_idx
    ON docs USING psql_bm25s (title_tokens, body_tokens);
```

In the current implementation:

- the indexed columns must all share one type:
  - `text[]`
  - `varchar[]`
  - `text`
  - `varchar`
- the columns are fused in index-definition order
- `NULL` indexed columns are skipped
- the fused token stream is what BM25 sees for build, refresh, and
  direct regclass query APIs
- scalar `text` and `varchar` columns are tokenized with the index text
  options before fusion

This means the feature behaves like a single BM25 document assembled
from multiple tokenized sources.

## What This Feature Is Not

The default multicolumn index is not a full field-aware BM25F design.

It does not keep separate:

- per-field scores
- per-field length normalization
- per-field explain output
- per-field query masks inside one index

If you need per-field weighting or per-field retrieval semantics, use
the stable multi-index fusion helpers documented in
[Multi-Field Search](multi-field-search.md), or use the single-index
field-aware mode enabled by `field_aware = true` and documented in
[Field-Aware Indexes](field-aware-indexes.md).

## Field-Aware Index

For multicolumn `text[]`, `varchar[]`, `text`, or `varchar` indexes, the
index can preserve field identity inside one BM25 payload:

```sql
CREATE INDEX docs_field_bm25_idx
    ON docs USING psql_bm25s (title_tokens, body_tokens)
    WITH (field_aware = true);
```

This mode stores field-scoped internal tokens and can be queried with
query-time field weights:

```sql
SELECT d.id, h.score
FROM psql_bm25s_field_aware_query_tokens(
    'docs_field_bm25_idx'::regclass,
    ARRAY['bird'],
    ARRAY['title_tokens', 'body_tokens'],
    ARRAY[3.0, 1.0]::real[],
    10
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

For scalar text columns, `psql_bm25s_field_aware_query(...)`
tokenizes the query text with the index text options before the
field-aware search.

Generic text retrieval APIs search all indexed fields with equal weight
on `field_aware = true` indexes for token and simple raw term queries.
Use the field-aware APIs above when the query needs a field subset or
custom per-field weights. Complex raw-query semantics such as phrases
and boolean filters remain available on non-field-aware fused indexes.

This is one BM25 payload with field-scoped terms, not BM25F and not
multiple internal sub-indexes. Combined document length
normalization still applies.

## Current Query Surface

Recommended APIs for multicolumn fusion indexes:

- `psql_bm25s_query_tokens(...)`
- `psql_bm25s_query(...)`
- prepared-query helpers built on the same regclass retrieval path

Example token query:

```sql
SELECT d.id, h.score
FROM psql_bm25s_query_tokens(
    'docs_fused_bm25_idx'::regclass,
    ARRAY['cat', 'bird'],
    10,
    NULL
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

Example raw query:

```sql
SELECT d.id, h.score
FROM psql_bm25s_query(
    'docs_fused_bm25_idx'::regclass,
    'cat AND bird',
    10,
    NULL
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

## Current Operator Limitation

Multicolumn fusion indexes do not currently support the SQL operator
scan surfaces:

- `@@`
- `@@@`
- `<=>`

Those operator paths remain single-column surfaces in this version.

Reason:

- the operator scan path still assumes one indexed heap attribute for
  planner-visible filtering and ordering
- keeping those paths single-column avoids pushing extra complexity into
  the existing fast path

So the practical rule is:

- use direct regclass retrieval APIs for multicolumn fusion indexes
- use operator scans on single-column indexes
- on `field_aware = true` multicolumn indexes, generic direct APIs
  search all fields with equal weight for token and simple raw term
  queries
- use `psql_bm25s_field_aware_query_tokens(...)` or
  `psql_bm25s_field_aware_query(...)` when a `field_aware = true`
  query needs custom field weights or a field subset

## Build And Refresh Semantics

The fused token stream is materialized during:

- initial index build
- auto-maintain delta recording
- index refresh / reindex

That keeps the storage format aligned with the existing single-column
core design. The current implementation does not introduce a second
posting format just for multicolumn indexes.

For scalar `text` and `varchar` columns:

- each non-NULL indexed column is tokenized with the index text options
- the resulting token streams are appended in index-column order
- the fused token stream is what gets indexed, refreshed, and verified

## Phrase And Boolean Semantics

Phrase and boolean matching operate over the fused token stream.

That means:

- phrase matching sees the fused token order
- phrase boundaries are not field boundaries
- `"cat bird"` can match if `cat` is at the end of one indexed column
  and `bird` is at the start of the next one in the fused order

This is a deliberate tradeoff. The feature is a fused-document index,
not a field-isolated query model.

If that phrase behavior is not acceptable for your application, keep
separate field indexes and use weighted multi-index fusion instead.

## When To Use It

Use a multicolumn fusion index when:

- one logical search document is already spread across several tokenized
  columns
- you want one index object instead of several field indexes
- you do not need separate field weights inside that one index
- you are happy with fused-document phrase semantics

Typical examples:

- `title_tokens + body_tokens`
- `title_text + body_text`
- `headline_tokens + summary_tokens + body_tokens`
- several chunked text arrays that are already conceptually one search
  document

## When Not To Use It

Do not use the default fused-document multicolumn index when:

- you need `title^3 + body^1` style weighting inside one query
- you need field-isolated phrase semantics
- you want SQL operator scans over the same index

For the exact reference model, keep separate single-column indexes and
use:

- `psql_bm25s_fusion_query_weighted(...)`
- `psql_bm25s_fusion_query_fields(...)`
- `psql_bm25s_fusion_query(...)`

If one index object is more important than BM25F-style field
normalization, use `field_aware = true` and the field-aware
helpers instead. See [Multi-Field Search](multi-field-search.md) and
[Field-Aware Indexes](field-aware-indexes.md).
