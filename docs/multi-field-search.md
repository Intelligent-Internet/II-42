# Multi-Field Search

This page shows the intended advanced SQL patterns for multi-field
search on top of the current stable helper surface.

If you want one BM25 index over several `text[]` columns instead, see
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md).

The goal is not drop-in compatibility with another extension. The goal
is to make the common application shape easy to express while keeping
the exact retrieval path explicit.

## When To Use This

Use this pattern when:

- one logical document has multiple indexed fields
- fields should contribute with different weights
- the application wants one final ranked result set
- the query-time score should stay attached to the current search

Typical examples:

- `title` gets the highest weight
- `abstract` gets a medium weight
- `body` gets a lower weight

## Core Model

The stable contract is:

1. each field has its own `psql_bm25s` index
2. each field runs its own exact top-k retrieval
3. scores are fused only after those top-k result sets exist

That means:

- there is no hidden cross-field scoring engine
- field names are metadata, not implicit scoring signals
- the canonical exact retrieval path is still the foundation
- one single-index fusion mode is a different feature with different
  semantics; see [Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md)

## Schema Shape

One practical schema shape is:

```sql
CREATE TABLE docs (
    id bigint PRIMARY KEY,
    title_tokens text[] NOT NULL,
    abstract_tokens text[] NOT NULL,
    body_tokens text[] NOT NULL
);

CREATE INDEX docs_title_bm25_idx
    ON docs USING psql_bm25s (title_tokens);

CREATE INDEX docs_abstract_bm25_idx
    ON docs USING psql_bm25s (abstract_tokens);

CREATE INDEX docs_body_bm25_idx
    ON docs USING psql_bm25s (body_tokens);
```

Each field is indexed independently. That keeps the storage and
retrieval contract simple and makes weighting explicit at query time.

## Level 1: Explicit Baseline Fusion

This is the most explicit advanced shape. It is also the easiest one to
reason about.

```sql
WITH fused AS (
    SELECT *
    FROM psql_bm25s_fusion(
        ARRAY(
            SELECT h
            FROM psql_bm25s_query(
                'docs_title_bm25_idx'::regclass,
                'cancer therapy',
                20,
                NULL
            ) AS h
        ),
        3.0,
        ARRAY(
            SELECT h
            FROM psql_bm25s_query(
                'docs_abstract_bm25_idx'::regclass,
                'cancer therapy',
                20,
                NULL
            ) AS h
        ),
        1.5,
        10
    )
)
SELECT d.id, d.title, h.score
FROM fused AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

Logic:

- retrieve exact top-k from one field
- retrieve exact top-k from another field
- weight the scores explicitly
- fuse the result sets
- join back to rows through `ctid`

This is the reference mental model for the higher-level helpers below.

## Level 2: Structured Field Queries

When the application has several weighted fields, the more structured
surface is:

- `psql_bm25s_fusion_field_query(...)`
- `psql_bm25s_fusion_query_fields(...)`

```sql
SELECT d.id, d.title, h.score
FROM psql_bm25s_fusion_query_fields(
    ARRAY[
        psql_bm25s_fusion_field_query(
            'title',
            'docs_title_bm25_idx'::regclass,
            'cancer therapy',
            3.0
        ),
        psql_bm25s_fusion_field_query(
            'abstract',
            'docs_abstract_bm25_idx'::regclass,
            'cancer therapy',
            1.5
        ),
        psql_bm25s_fusion_field_query(
            'body',
            'docs_body_bm25_idx'::regclass,
            'cancer therapy',
            1.0
        )
    ]::psql_bm25s_result_fusion_field_query[],
    10,
    30,
    NULL
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

What this buys you:

- field names are carried explicitly
- each field still owns one weighted prepared query
- the array is structured instead of being only positional

What it does not change:

- exact retrieval still happens per field/index
- fusion still happens after retrieval
- the returned score is still query-scoped

## Level 3: Convenience Search Fields

If several fields share the same raw query text and normalization
options, the shortest surface is:

- `psql_bm25s_fusion_query(...)`

Named form:

```sql
SELECT d.id, d.title, h.score
FROM psql_bm25s_fusion_query(
    ARRAY['title', 'abstract', 'body']::text[],
    ARRAY[
        'docs_title_bm25_idx'::regclass,
        'docs_abstract_bm25_idx'::regclass,
        'docs_body_bm25_idx'::regclass
    ],
    'cancer therapy',
    ARRAY[3.0, 1.5, 1.0]::real[],
    10,
    30,
    NULL,
    true,
    ARRAY['and', 'or']::text[],
    true,
    false
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

Short form:

```sql
SELECT d.id, d.title, h.score
FROM psql_bm25s_fusion_query(
    ARRAY[
        'docs_title_bm25_idx'::regclass,
        'docs_abstract_bm25_idx'::regclass,
        'docs_body_bm25_idx'::regclass
    ],
    'cancer therapy',
    ARRAY[3.0, 1.5, 1.0]::real[],
    10,
    30,
    NULL
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

Recommendation:

- use the named form in real applications
- keep the short form for smaller SQL snippets or quick experiments

## Understanding `k` And `candidate_k`

The field-aware helpers expose two important limits:

- `k`
  - final result size
- `candidate_k`
  - per-field retrieval size before fusion

Practical rule:

- start with `candidate_k >= k`
- use a larger `candidate_k` when several fields may surface different
  documents that need to survive into the fused top-k

If `candidate_k` is too small, fusion can miss documents that would have
scored well after combining fields.

## Getting Back Rows And Scores

The intended application pattern is:

1. get query-scoped hit rows
2. join hits back to rows

That is why the examples use:

- `psql_bm25s_fusion_query_fields(...)`
- `psql_bm25s_fusion_query(...)`
- `JOIN ... ON d.ctid = h.ctid`

This keeps score attached to the current query execution. It also avoids
the high-risk pattern of a generic scalar `score(id)` call evaluated row
by row later.

## Syntax Summary

Use these in increasing order of abstraction:

- `psql_bm25s_fusion(...)`
  - most explicit
- `psql_bm25s_fusion_weighted_query(...)`
  - weighted prepared query
- `psql_bm25s_fusion_field_query(...)`
  - weighted prepared query with a field name
- `psql_bm25s_fusion_query_weighted(...)`
  - fusion from weighted prepared queries
- `psql_bm25s_fusion_query_fields(...)`
  - fusion from named field queries
- `psql_bm25s_fusion_query(...)`
  - convenience form for shared query text

## Design Notes

The important design boundaries are:

- no hidden planner magic
- no row-by-row generic rescoring
- no silent change to BM25 ranking semantics
- no requirement that field-aware helpers behave like another
  extension's API

This is an advanced convenience layer built on top of the exact
retrieval core, not a second search engine inside the extension.
