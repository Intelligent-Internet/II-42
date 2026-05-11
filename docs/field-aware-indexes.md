# Field-Aware Indexes

`psql_bm25s` supports field-aware retrieval in two production shapes:

- separate single-column indexes fused at query time
- one multicolumn index with `field_aware = true`

The second shape is the single-index field-aware mode. It preserves field
identity inside one BM25 payload and allows query-time field weighting without
building one index per field.

## When To Use This

Use a field-aware index when:

- the table has several searchable text-like fields
- one index object is operationally simpler than several field indexes
- queries need weights such as `title^3 + body^1`
- BM25F-style per-field length normalization is not required

For the strongest field-isolated model, keep separate field indexes and use
the multi-index helpers documented in [Multi-Field Search](multi-field-search.md).
For columns that are simply one logical document, use the default fused
multicolumn index documented in
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md).

## Supported Column Types

Field-aware indexes are available for multicolumn indexes whose
indexed columns are all one of these homogeneous text-like types:

- `text[]`
- `varchar[]`
- `text`
- `varchar`

Scalar `text` and `varchar` fields are tokenized with the index text options
before the field-scoped tokens are stored.

## Build An Index

Pretokenized fields:

```sql
CREATE INDEX docs_field_bm25_idx
    ON docs USING psql_bm25s (title_tokens, body_tokens)
    WITH (field_aware = true);
```

Scalar text fields:

```sql
CREATE INDEX docs_text_field_bm25_idx
    ON docs USING psql_bm25s (title, body)
    WITH (
        field_aware = true,
        text_lowercase = true,
        text_fold_diacritics = true
    );
```

`field_aware = true` is opt-in. Without it, the same multicolumn index is a
fused-document index: columns are appended into one logical token stream and
field identity is not preserved.

## Query With Field Weights

For token arrays, use `psql_bm25s_field_aware_query_tokens(...)`:

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

For scalar `text` or `varchar` fields, use
`psql_bm25s_field_aware_query(...)` so the query text is tokenized with
the index text options before field-aware retrieval:

```sql
SELECT d.id, h.score
FROM psql_bm25s_field_aware_query(
    'docs_text_field_bm25_idx'::regclass,
    'bird migration',
    ARRAY['title', 'body'],
    ARRAY[3.0, 1.0]::real[],
    10
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

Pass `NULL` for weights, or omit the argument, to use equal weights.

## Generic Query Behavior

Generic direct APIs work on field-aware indexes:

- `psql_bm25s_query_tokens(...)`
- `psql_bm25s_query(...)`

On `field_aware = true` indexes, these APIs search every indexed field with
equal weight for token queries and simple raw term queries. Use the explicit
field-aware APIs when the query needs custom weights or only a subset of
fields.

Complex raw-query semantics such as phrase and boolean filters remain
available on non-field-aware fused indexes. Field-aware generic raw queries
intentionally reject complex raw query forms instead of silently applying
non-field-aware planning to field-scoped terms.

## Internal Semantics

The field-aware engine stores field-scoped internal tokens in one BM25 payload.
For example:

- `title_tokens = ARRAY['bird']` becomes one internal token namespace
- `body_tokens = ARRAY['bird']` becomes a different internal token namespace
- a query over both fields scores both streams and sums the weighted scores

This gives one multicolumn BM25 index with query-time field weighting. It is
not BM25F:

- document length normalization uses the combined indexed document length
- there is no per-field length normalization
- there is no per-field explain output
- phrase and boolean verification are not field-isolated in the generic path

The implementation keeps the single-column hot path and the default
multicolumn fused-document path separate. Field-aware behavior is used only
when `field_aware = true` is set on a multicolumn text-like index.

## Choosing A Multi-Field Shape

Use separate field indexes when:

- field-local phrase or verification semantics are required
- per-field explainability matters
- isolated rebuild and maintenance per field is more important than having
  one index object

Use a default multicolumn fused-document index when:

- the columns are really one logical search document
- field weights are not needed
- phrase matching across column boundaries is acceptable

Use `field_aware = true` when:

- one multicolumn index object is preferred
- query-time field weights are required
- BM25F-style per-field normalization is not required

## Current Limits

The current field-aware engine deliberately does not implement:

- BM25F scoring
- per-field length normalization
- per-field explain output
- SQL operator scans over multicolumn indexes
- field-isolated complex raw-query planning

Those limits are part of the current contract, not unfinished work in this
feature.
