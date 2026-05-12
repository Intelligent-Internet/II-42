# 🐿️ psql_bm25s

`psql_bm25s` ([Technical Report](docs/technical-report.md)) is an
independent PostgreSQL extension for BM25-family lexical retrieval. It
is inspired by the public [`bm25s`](https://github.com/xhluca/bm25s)
work and implemented as a PostgreSQL-native access method.

<img width="1500" height="600" alt="commons-banner-github" src="https://github.com/user-attachments/assets/de036ebb-58f7-4d62-a362-ec50a693b402" />

The project keeps the BM25 contract explicit where it matters most:

- BM25-family semantics
- corpus-statistics-driven ranking
- query-first exact top-k retrieval

The mainline extension adds a PostgreSQL-specific storage and
maintenance design for database workloads with frequent `INSERT`,
`UPDATE`, and `DELETE`. No source code from the Python reference
implementation `bm25s` is vendored or copied into this repository.

## What It Does

Current mainline capabilities:

- native `CREATE INDEX USING psql_bm25s`
- indexed column types:
  - `text`
  - `varchar`
  - `text[]`
  - `varchar[]`
  - `int4[]`
- multicolumn fusion indexes over `text[]`, `varchar[]`, `text`, or
  `varchar` columns
- opt-in field-aware multicolumn indexes with query-time field
  weights
- canonical exact BM25 retrieval APIs:
  - `psql_bm25s_query_tokens(...)`
  - `psql_bm25s_query_ids(...)`
- SQL-facing convenience surfaces:
  - `psql_bm25s_query(...)`
  - `psql_bm25s_query_prepared(...)`
  - `tokens @@ 'query text'`
  - `ORDER BY tokens <=> ... ASC LIMIT k`
  - `ORDER BY token_ids <=> ... ASC LIMIT k`
- text preprocessing helpers:
  - tokenization
  - normalization
  - optional stopwords
  - optional English stemming
  - optional Latin-diacritic folding
- token-stream and raw-text highlight and snippet helpers
- automatic mutable-workload maintenance:
  - exact `INSERT` / `UPDATE`
  - exact delete cleanup through PostgreSQL heap cleanup and index
    maintenance
  - bounded deferred overlays for lower write cost
- maintenance introspection and recommendation helpers
- PostgreSQL-native durability and physical replication compatibility
- weighted multi-index query fusion helpers for field-aware retrieval
- C-backed hybrid BM25/vector late-fusion helpers without a hard vector
  extension dependency

## Relationship To BM25S

The Python reference implementation `bm25s` project is the main public
reference for the eager sparse scoring formulation used as technical
background:

- <https://github.com/xhluca/bm25s>

Related scoring concepts include:

- `robertson`
- `lucene`
- `atire`
- `bm25l`
- `bm25+`
- eager sparse scoring
- CSC-style postings layout
- top-k retrieval over precomputed sparse scores

The extension does not try to reproduce the `bm25s` Python package interface.
It implements its own C and PostgreSQL storage, access-method, and
maintenance layers around BM25-family retrieval semantics.

## Why This Extension Exists

`bm25s` is excellent for fast BM25 retrieval over static or externally
managed corpora. PostgreSQL adds different requirements:

- index persistence inside the database
- transactional writes
- crash recovery
- physical replication
- planner-visible SQL access
- operationally manageable index maintenance under row churn

`psql_bm25s` keeps BM25-family semantics explicit while adding the
storage and maintenance layers needed for the index to behave like a
database index rather than a one-shot offline artifact.

## Performance Status

Current high-level read:

- the canonical rowset query path is the main performance path
- the current public benchmark reference is the PG18 `15 x 5` BEIR
  matrix
- published BEIR reference targets are included in the project benchmark
  suite
- historical same-machine studies remain useful background, but they
  are no longer the default benchmark reference
- the mutable-maintenance design dramatically improves write-side
  maintenance cost relative to eager full rebuilds, while keeping exact
  reads and PostgreSQL durability/replication guarantees

Current performance matrix, with dataset size and QPS:

These numbers are benchmark context, not universal claims about any
project. They depend on the measured versions, configuration, hardware,
workload, and query settings. Third-party project names identify the
measured engines for reproducibility.

| Dataset | Docs | Queries | Python reference implementation `bm25s` QPS | `psql_bm25s ids` QPS | `psql_bm25s text[]` QPS | `pg_search` QPS | `vchord_bm25` QPS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 8,674 | 1,406 | 1158.34 | 1402.63 | 1112.01 | 115.94 | 78.77 |
| `climate-fever` | 5,416,593 | 1,535 | 3.04 | 57.78 | 50.75 | 2.84 | 5.25 |
| `cqadupstack` | 457,199 | 13,145 | 111.56 | 443.13 | 438.42 | 13.99 | 60.51 |
| `dbpedia-entity` | 4,635,922 | 467 | 3.47 | 128.19 | 91.19 | 5.21 | 23.66 |
| `fever` | 5,416,568 | 123,142 | 3.15 | 97.56 | 80.15 | 5.62 | 12.13 |
| `fiqa` | 57,638 | 6,648 | 810.51 | 1409.52 | 1186.41 | 17.76 | 190.57 |
| `hotpotqa` | 5,233,329 | 97,852 | 4.16 | 55.40 | 49.86 | 3.42 | 9.31 |
| `msmarco` | 8,841,823 | 509,962 | 1.61 | 96.67 | 82.13 | 4.44 | 18.20 |
| `nfcorpus` | 3,633 | 3,237 | 3155.35 | 3373.94 | 3326.96 | 1132.17 | 1252.75 |
| `nq` | 2,681,468 | 3,452 | 10.55 | 174.34 | 176.69 | 6.28 | 21.96 |
| `quora` | 522,931 | 15,000 | 90.56 | 637.98 | 619.64 | 13.26 | 154.36 |
| `scidocs` | 25,657 | 1,000 | 1203.09 | 1835.85 | 1614.92 | 17.89 | 367.04 |
| `scifact` | 5,183 | 1,109 | 2964.86 | 2557.47 | 2240.18 | 500.04 | 629.42 |
| `trec-covid` | 171,332 | 50 | 210.50 | 191.94 | 154.48 | 8.75 | 75.66 |
| `webis-touche2020` | 382,545 | 49 | 240.36 | 82.97 | 74.10 | 8.14 | 86.04 |

Trend view by dataset scale:

![QPS vs dataset scale](docs/performance/reports/pg18-qps-vs-dataset-scale-2026-04-02.svg)

Current matrix readout:

- Median QPS ratios versus Python reference implementation `bm25s` are `3.97x` for
  `psql_bm25s ids`, `3.93x` for `psql_bm25s text[]`, `0.54x` for
  `vchord_bm25`, and `0.17x` for `pg_search`.
- Dataset counts at or above Python reference implementation `bm25s` are `12/15` for
  `psql_bm25s ids`, `11/15` for `psql_bm25s text[]`, `7/15` for
  `vchord_bm25`, and `3/15` for `pg_search`.
- On the largest workload, `msmarco`,
  the measured QPS was `96.67` for `psql_bm25s ids`, `82.13` for
  `psql_bm25s text[]`, `18.20` for `vchord_bm25`, `4.44` for
  `pg_search`, and `1.61` for the Python reference implementation
  `bm25s`.
- Mutable-maintenance results are still important, but they are a
  separate dimension from this cross-engine read-performance matrix.

[More Performance and Benchmark detail](docs/performance/README.md)

## Quick Start

You can get started in three common ways:

### 1. Install from GitHub Releases

Download the package that matches your PostgreSQL major, OS, and
architecture from:

- <https://github.com/Intelligent-Internet/psql_bm25s/releases>

Each release `.zip` is built with the PostgreSQL extension files staged
under their final install paths. A simple install flow is:

```bash
curl -L -o psql_bm25s.zip \
  https://github.com/Intelligent-Internet/psql_bm25s/releases/latest/download/<package>.zip
unzip psql_bm25s.zip
sudo rsync -a <package>/ /
```

Replace `<package>` with the actual archive directory name from the
release, for example `psql_bm25s-vX.Y.Z-linux-x86_64-pg18`.

After copying the files, restart PostgreSQL if needed, then enable the
extension in the target database:

```sql
CREATE EXTENSION psql_bm25s;
```

If you want a non-`public` extension schema, choose it at creation time with
`CREATE EXTENSION psql_bm25s WITH SCHEMA ext;`. The extension is not
relocatable after creation because SQL helper functions capture the extension
schema for safe wrapper resolution.

### 2. Run the Docker image from GitHub Packages

Prebuilt PostgreSQL 18 images are published to:

- <https://github.com/orgs/Intelligent-Internet/packages?repo_name=psql_bm25s>

Pull either the floating PG18 tag or a versioned release tag:

```bash
docker pull ghcr.io/intelligent-internet/psql_bm25s:pg18
# or:
docker pull ghcr.io/intelligent-internet/psql_bm25s:pg18-vX.Y.Z
```

Start PostgreSQL with the extension preinstalled:

```bash
docker run -d \
  --name psql-bm25s-pg18 \
  -e POSTGRES_PASSWORD=postgres \
  -p 5432:5432 \
  ghcr.io/intelligent-internet/psql_bm25s:pg18
```

The image init scripts create `psql_bm25s` in `postgres`, `template1`,
and the optional `POSTGRES_DB` database on first boot.

### 3. Build from source

For source builds, local validation, and release workflow notes, see
[Contribution](docs/contribution.md).

After installing the extension, open `psql` and run the smallest useful
example: insert rows, index a `text` column with default options, and
run a top-k BM25 query.

```sql
CREATE EXTENSION psql_bm25s;

DROP TABLE IF EXISTS docs;

CREATE TABLE docs (
    id integer primary key,
    title text not null,
    body text not null
);

INSERT INTO docs (id, title, body) VALUES
    (1, 'red apple', 'fresh red apple fruit'),
    (2, 'green apple', 'green apple slices'),
    (3, 'orange citrus', 'orange citrus fruit'),
    (4, 'cat guide', 'small cat animal care');

CREATE INDEX docs_bm25_idx
    ON docs USING psql_bm25s (body);

SELECT d.id, d.title, h.score
FROM psql_bm25s_query(
    'docs_bm25_idx'::regclass,
    'apple fruit',
    5
) AS h
JOIN docs AS d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;
```

With no `WITH (...)` options, the index uses Lucene-style BM25 and IDF
defaults with the `realtime` consistency policy.

The extension also supports `varchar`, pretokenized `text[]` and
`varchar[]`, and integer token id arrays for applications that own
tokenization. Eventual and manual consistency are available for write-heavy
or batch-maintained corpora.
For SQL operators, multi-field search, hybrid vector/BM25 fusion, and
maintenance policy examples, start with:

- [Supported Input Types](docs/input-types.md)
- [Query Semantics](docs/query-semantics.md)
- [Multi-Field Search](docs/multi-field-search.md)
- [Field-Aware Indexes](docs/field-aware-indexes.md)
- [Hybrid Vector/BM25 Search](docs/hybrid-search.md)
- [Index Policy](docs/index-policy.md)

## Canonical Retrieval APIs

Use these as the default exact BM25 entry points:

- `psql_bm25s_query_ids(...)`
- `psql_bm25s_query_tokens(...)`

These are the clearest semantic surface and the main benchmark path.

## Query Model

The extension exposes three layers of query surface:

1. Canonical exact BM25 APIs
   - `psql_bm25s_query_ids(...)`
   - `psql_bm25s_query_tokens(...)`
2. SQL convenience retrieval
   - `psql_bm25s_query(...)`
   - `psql_bm25s_query_prepared(...)`
3. Planner/operator integration
   - `@@`
   - `<=>`

The canonical exact BM25 contract is defined by rowset retrieval APIs
that return `psql_bm25s_result_hit` rows. They are the main benchmark path and
the clearest semantic surface.

Important semantic boundaries:

- `@@` is a document-match predicate, not a ranking API
- `<=>` aligns with true BM25 ordering only when PostgreSQL executes a
  real `psql_bm25s` index scan
- raw-query retrieval is exact for the supported surface, but grouped
  boolean and phrase queries may use bounded heap verification and are
  intentionally slower than simple array-based retrieval

## Hybrid BM25/Vector Fusion

For RAG-style ranking, `psql_bm25s` can combine BM25 candidates, vector
candidates, and other ranked SQL candidate sources into one weighted top-k
inside PostgreSQL. The fusion path is C-backed and keeps vector support
optional: `pgvector`, VectorChord, or another vector extension can own vector
retrieval, while `psql_bm25s` owns normalization, weighting, de-duplication,
and final ordering.

Use [Hybrid Vector/BM25 Search](docs/hybrid-search.md) for query examples and
[Hybrid Fusion Engine](docs/hybrid-fusion-engine.md) for implementation
boundaries, performance guidance, and validation coverage.

## Mutable-Workload Index Design

`psql_bm25s` extends the original static `bm25s` storage idea with
PostgreSQL-native mutable-index maintenance. The goal is to keep BM25
semantics stable while making `INSERT`, `UPDATE`, `DELETE`, `VACUUM`,
restart, crash recovery, and physical replication operationally manageable.

The public maintenance switch is `consistency`:

- `realtime`: default strong-freshness behavior for ordinary mutable tables
- `eventual`: query-first behavior for large knowledge bases that can
  tolerate short-term stale BM25 results while maintenance converges
- `manual`: explicit-refresh behavior for static corpora, benchmarks, and
  externally scheduled maintenance

The implementation records maintenance debt on the index, batches repeated
writes, supports bounded overlays where appropriate, follows PostgreSQL's
normal delete-cleanup lifecycle, and exposes automatic worker plus explicit
maintenance helpers for operators.

For details:

- [Index Policy](docs/index-policy.md) explains the three consistency modes,
  write/query behavior, maintenance helpers, and scheduler guidance.
- [Shared Generation Cache](docs/shared-generation-cache.md) explains the
  shared immutable cache tiers for large connection-pool deployments,
  including the zero-configuration DSM path and the optional
  `shared_preload_libraries` arena.
- [Index Parameters](docs/index-parameters.md) is the compact reference for
  `CREATE INDEX ... WITH (...)` reloptions.
- [Architecture and Design](docs/architecture-and-design.md) summarizes the
  storage and access-method design behind mutable indexes.
- [Online Maintenance Future Plan](docs/online-maintenance-future-plan.md)
  records the larger future direction for generationed publish semantics.

## Durability and Replication

The current implementation is a native PostgreSQL index relation.

That means:

- index pages live in ordinary PostgreSQL index storage
- writes are WAL-logged
- the index survives restart and crash recovery
- the index participates in physical replication like other PostgreSQL
  indexes

Logical replication follows normal PostgreSQL behavior:

- table rows replicate
- index relations do not replicate as logical data objects
- indexes should be created or rebuilt on the subscriber side

## Documentation

Check these links for detailed documentation:

- [Documentation Root](docs/)
- [Contribution](docs/contribution.md)
- [Architecture and Design](docs/architecture-and-design.md)
- [Technical Report](docs/technical-report.md)
- [API Reference](docs/api-reference.md)
- [Functions](docs/functions.md)
- [Index Parameters](docs/index-parameters.md)
- [Supported Input Types](docs/input-types.md)
- [Hybrid Vector/BM25 Search](docs/hybrid-search.md)
- [Hybrid Vector/BM25 Search Use-Case Design](docs/hybrid-vector-bm25-use-case-design.md)
- [Hybrid Fusion Engine](docs/hybrid-fusion-engine.md)
- [Query Semantics](docs/query-semantics.md)
- [Multi-Field Search](docs/multi-field-search.md)
- [Field-Aware Indexes](docs/field-aware-indexes.md)
- [Index Policy](docs/index-policy.md)
- [Shared Generation Cache](docs/shared-generation-cache.md)
- [Online Maintenance Future Plan](docs/online-maintenance-future-plan.md)
- [Performance and Benchmarks](docs/performance/README.md)
- [Testing and Validation](docs/testing-and-validation.md)

## Future Work

The remaining roadmap is broader project work rather than another round of
local collector tweaks:

See also:

- [Online Maintenance Future Plan](docs/online-maintenance-future-plan.md)

1. deeper storage and representation work for lower read-path overhead
2. stronger heap and `text[]` fetch redesign, especially for phrase- and
   verification-heavy paths
3. more architectural query-state reuse where it can be made exact and
   stable
4. stronger differential fuzzing and randomized correctness checks
5. prepared and index-bound query values
6. broader filtered ranked scans and clearer top-k diagnostics
7. better native APIs for pretokenized corpora
8. richer tokenizer and normalization profiles
9. longer-run maintenance policy tuning and workload replay
10. richer presentation helpers beyond token-stream output
