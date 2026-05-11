# Architecture and Design

`psql_bm25s` is organized around four layers.

## 1. BM25 Core

Core file:

- `src/psql_bm25s_core.c`

Responsibilities:

- BM25 scoring variants
- corpus statistics
- sparse postings and score storage
- top-k retrieval
- BM25S-aligned ranking semantics

This is the layer that stays closest to the Python reference
implementation `bm25s`.

## 2. Stable Binary Storage

Core file:

- `src/psql_bm25s_storage.c`

Responsibilities:

- stable little-endian serialization
- stored payload for the internal `psql_bm25s_index` SQL type
- stored payload for the PostgreSQL index relation

Current storage extends the earlier single-payload design with exact
stats required for maintainable mutable workloads:

- per-posting term frequency
- per-document document length
- per-term document frequency

## 3. PostgreSQL Type and Function Bindings

Core file:

- `src/psql_bm25s_pg.c`

Responsibilities:

- `psql_bm25s_index` I/O
- standalone builder and top-k functions
- SQL-visible helpers around the core storage type

## 4. PostgreSQL Access Method

Core file:

- `src/psql_bm25s_am.c`

Responsibilities:

- `CREATE INDEX USING psql_bm25s`
- ordered scans through `<=>`
- predicate scans through `@@`
- canonical retrieval APIs over stored indexes
- mutable-workload maintenance
- maintenance introspection and policy recommendation

## Main Design Rules

- canonical exact BM25 retrieval stays in native C
- no SQL/SPI fallback in the hot retrieval path
- PostgreSQL durability and physical replication are first-class
- write-side maintenance may be more expensive if needed to protect
  query throughput
- storage evolution may change maintenance mechanics, but not the BM25
  scoring contract

## Mainline Enhancement Over Original bm25s Storage

Original `bm25s` is optimized for fast retrieval over a static or
externally managed corpus. `psql_bm25s` adds the database-facing layer:

- persisted maintenance metadata
- transaction-aware batching
- threshold-bounded deferred maintenance
- exact base-plus-delta overlays
- vacuum-driven delete tombstones
- restart / crash / replication-safe storage inside PostgreSQL

This is the main architectural extension beyond the Python reference
implementation.
