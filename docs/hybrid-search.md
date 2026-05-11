# Hybrid Vector/BM25 Search

Hybrid search combines lexical BM25 candidates with semantic vector
candidates inside PostgreSQL. The implementation is intentionally a late
fusion layer: each source keeps its own best index access path, and
`psql_bm25s` only combines already-retrieved candidates.

This keeps the core extension independent from `pgvector`, VectorChord, and
other vector extensions. Vector candidates are supplied as ordinary SQL rows
containing `ctid`, a raw distance or score, rank, weight, and normalization
metadata.

## When To Use It

Use hybrid search when a RAG workload needs both:

- lexical precision from exact BM25 fields such as title, abstract, or body
- semantic recall from vector embeddings
- one final top-k ordering inside PostgreSQL
- explicit weights and explainable per-source contributions

Do not use hybrid search when a single BM25 index or a single vector index is
already the complete ranking signal. In that case, the direct index path is
simpler and faster.

## Candidate Model

The fusion layer operates on `psql_bm25s_result_hybrid_candidate` values:

- `source_name`: stable source label such as `title`, `body`, or `embedding`
- `ctid`: row identity for same-table fusion within one statement
- `raw_value`: BM25 score, vector distance, or another source-local value
- `source_rank`: one-based rank inside that source
- `weight`: source weight
- `normalizer`: score normalization strategy
- `direction`: whether higher or lower raw values are better

The output is a set of `psql_bm25s_result_hybrid_hit` rows with:

- final fused score
- source count
- source names
- raw values
- normalized scores
- weighted scores
- source ranks

The debug fields are part of the API because hybrid ranking is only useful
when the final order can be inspected and tuned.

`psql_bm25s_hybrid_fuse_candidates(...)` uses a C fast path for the actual
normalization, de-duplication, grouping, and final ordering. Application
queries should use that public C-backed function directly.

For implementation boundaries, performance expectations, and validation
coverage, see [Hybrid Fusion Engine](hybrid-fusion-engine.md).

## Default Fusion: RRF

The default fusion method is reciprocal rank fusion:

```text
source contribution = weight / (rrf_k + source_rank)
```

The default `rrf_k` is `60`.

RRF is the safest default because it does not compare raw BM25 scores with
raw vector distances. BM25 and vector distances live on different scales, and
vector distance is usually lower-is-better while BM25 is higher-is-better.

## Score Fusion

Advanced users can use `fusion => 'score'` with explicit normalization.

Supported normalizers:

- `identity`: use the raw score directly; with `lower_is_better`, the value is
  negated
- `negative_distance`: use `-distance`
- `inverse_distance`: use `1 / (epsilon + distance)`
- `minmax`: normalize each source over its retrieved candidates
- `zscore`: z-score each source over its retrieved candidates
- `rank`: use `1 / source_rank`

Degenerate cases are handled deliberately:

- empty inputs return no rows
- single-row `minmax` sources normalize to `1.0`
- zero-variance `zscore` sources normalize to `0.0`
- NULL, NaN, and infinite raw values are ignored

For mixed BM25/vector workloads, prefer `rrf` first. Use score fusion only
after measuring whether a specific normalization improves result quality.

For a concrete SQL-first use-case design where vector is the primary signal
and BM25 acts as a lexical boost, see
[Hybrid Vector/BM25 Search Use-Case Design](hybrid-vector-bm25-use-case-design.md).

## BM25 Sources

BM25 candidates can be created directly from a `psql_bm25s` index:

```sql
WITH bm25_candidates AS (
    SELECT c
    FROM psql_bm25s_hybrid_bm25_candidates(
        'title',
        'docs_title_bm25_idx'::regclass,
        'how to use a computer',
        2.0,
        1000
    ) AS c
)
SELECT h.*
FROM psql_bm25s_hybrid_fuse_candidates(
    ARRAY(SELECT c FROM bm25_candidates),
    20,
    'rrf'
) AS h;
```

This is still late fusion. The BM25 index produces candidates first; the
hybrid layer only combines the resulting rows.

## Vector Sources

Vector candidates are supplied by SQL. The core extension does not require
the `vector` type.

With VectorChord or pgvector installed, the vector side can look like this:

```sql
WITH vector_candidates AS (
    SELECT psql_bm25s_hybrid_vector_candidate(
        'embedding',
        d.ctid,
        (d.embedding <-> '[0.1,0.2,0.3]'::vector)::real,
        row_number() OVER (
            ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
        )::int4,
        1.0,
        'minmax'
    ) AS c
    FROM docs AS d
    ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
    LIMIT 1000
)
SELECT h.*
FROM psql_bm25s_hybrid_fuse_candidates(
    ARRAY(SELECT c FROM vector_candidates),
    20,
    'rrf'
) AS h;
```

For RRF, the vector normalizer is not used; only rank and weight matter. For
score fusion, vector distances should normally use `lower_is_better` through
`psql_bm25s_hybrid_vector_candidate(...)`, with `minmax`,
`negative_distance`, or `inverse_distance`.

## Mixed BM25 And Vector Example

```sql
WITH title_candidates AS (
    SELECT c
    FROM psql_bm25s_hybrid_bm25_candidates(
        'title',
        'docs_title_bm25_idx'::regclass,
        'how to use a computer',
        2.0,
        1000
    ) AS c
),
body_candidates AS (
    SELECT c
    FROM psql_bm25s_hybrid_bm25_candidates(
        'body',
        'docs_body_bm25_idx'::regclass,
        'how to use a computer hahhaha',
        1.8,
        1000
    ) AS c
),
vector_candidates AS (
    SELECT psql_bm25s_hybrid_vector_candidate(
        'embedding',
        d.ctid,
        (d.embedding <-> '[0.1,0.2,0.3]'::vector)::real,
        row_number() OVER (
            ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
        )::int4,
        1.0,
        'minmax'
    ) AS c
    FROM docs AS d
    WHERE d.published_at >= DATE '2022-01-01'
      AND d.published_at < DATE '2024-01-01'
    ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
    LIMIT 1000
),
hybrid_hits AS (
    SELECT *
    FROM psql_bm25s_hybrid_fuse_candidates(
        ARRAY(
            SELECT c FROM title_candidates
            UNION ALL
            SELECT c FROM body_candidates
            UNION ALL
            SELECT c FROM vector_candidates
        ),
        20,
        'rrf'
    )
)
SELECT d.*, h.score, h.source_names, h.normalized_scores
FROM hybrid_hits AS h
JOIN docs AS d ON d.ctid = h.ctid
WHERE d.published_at >= DATE '2022-01-01'
  AND d.published_at < DATE '2024-01-01'
ORDER BY h.score DESC, d.id;
```

The final `WHERE` keeps the returned rows aligned with the requested time
window. If the time predicate is highly selective, increase each source's
`candidate_k` or use table partitioning so PostgreSQL can restrict each
source before retrieval.

## Filtering And Recall

Late fusion has a known recall tradeoff: filters applied after each source
has produced candidates can remove many candidates. If the filter is narrow,
the final result may miss rows that would have appeared with a larger
candidate pool.

For large time-windowed knowledge bases, prefer partitioning by time. Then
the vector and BM25 indexes operate on the relevant partitions before fusion.

## Optional VectorChord Smoke

Run this only in a database that already has VectorChord or another
pgvector-compatible index installed:

```sql
CREATE EXTENSION IF NOT EXISTS vector;
CREATE EXTENSION IF NOT EXISTS vchord;

EXPLAIN
WITH vector_candidates AS (
    SELECT psql_bm25s_hybrid_vector_candidate(
        'embedding',
        d.ctid,
        (d.embedding <-> '[0.1,0.2,0.3]'::vector)::real,
        row_number() OVER (
            ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
        )::int4,
        1.0,
        'minmax'
    ) AS c
    FROM docs AS d
    ORDER BY d.embedding <-> '[0.1,0.2,0.3]'::vector
    LIMIT 1000
)
SELECT *
FROM psql_bm25s_hybrid_fuse_candidates(
    ARRAY(SELECT c FROM vector_candidates),
    20,
    'rrf'
);
```

The plan should show the vector index path for the candidate CTE. The hybrid
function should only appear after the vector candidate query has produced its
top-k rows.
