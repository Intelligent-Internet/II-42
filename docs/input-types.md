# Supported Input Types

`psql_bm25s` supports five indexed source-column types:

- `int4[]`
- `text[]`
- `varchar[]`
- `text`
- `varchar`

They all feed the same BM25 index core, but they reach it through two
different input models:

- pretokenized inputs owned by the application:
  - `int4[]`
  - `text[]`
  - `varchar[]`
- scalar text inputs tokenized at the index boundary:
  - `text`
  - `varchar`

## Overview

| Source type | Input model | Best fit | Main trade-off |
| --- | --- | --- | --- |
| `int4[]` | pre-encoded token IDs | highest-throughput exact retrieval | requires an external vocabulary or token-ID pipeline |
| `text[]` | pretokenized text tokens | explicit token control with strong exact performance | application must materialize tokens |
| `varchar[]` | pretokenized text tokens | same use case as `text[]` for schemas that already use `varchar[]` | application must materialize tokens |
| `text` | raw scalar text | easiest onboarding from ordinary PostgreSQL schemas | indexing and some verification paths pay tokenization cost inside the extension |
| `varchar` | raw scalar text | same as `text` when schema already uses `varchar` | indexing and some verification paths pay tokenization cost inside the extension |

## How `psql_bm25s` Supports Them

For `int4[]`, `text[]`, and `varchar[]`, the index receives the caller's
token stream directly:

- `int4[]` passes pre-encoded token IDs
- `text[]` passes pretokenized text
- `varchar[]` follows the same text-token path after adapting each array
  element to the text-like token interface used by the index

For scalar `text` and `varchar`, the extension tokenizes at the index
boundary and then lowers the result to the same internal token-stream
model. That keeps one BM25 scoring and postings core while still
letting ordinary SQL schemas start from raw text columns.

The scalar text pipeline uses the same project text-processing path used
by the SQL helpers:

- Unicode-aware tokenization
- NFC normalization
- ICU word-break segmentation
- Unicode case folding
- optional stopword filtering
- optional English Porter stemming
- optional Latin-diacritic folding

For the index-level scalar text parameters, see
[Index Parameters](index-parameters.md#text-processing-parameters).

## Performance Characteristics

### `int4[]`

This is the fastest and most stable exact-retrieval path when the
application can own vocabulary management and token-ID assignment. It is
the basis of the published `psql_bm25s ids` benchmark line.

### `text[]`

This is the main pretokenized text path. It keeps the token stream
explicit, avoids scalar retokenization, and is the basis of the
published `psql_bm25s text[]` benchmark line.

Use it when:

- the application already tokenizes documents
- token boundaries must stay explicit
- phrase and verification-heavy paths should avoid retokenizing raw text

### `varchar[]`

This follows the same pretokenized path as `text[]`. It mainly exists so
existing schemas do not need to rewrite arrays just to use the index.
Its behavior and expected performance profile should track `text[]`
closely.

### `text` and `varchar`

These are the easiest way to get started because the schema can index an
ordinary text column directly. The trade-off is that the extension must
tokenize during indexing, refresh, and some exact verification paths.

That makes scalar text columns the best choice when:

- ease of adoption matters more than explicit token materialization
- the dataset is moderate enough that extra tokenization CPU is
  acceptable
- the application wants a direct SQL-column search surface

Pretokenized arrays remain the more performance-oriented choice when the
application already owns tokenization or wants the most stable
high-throughput benchmark path.

## Public Benchmark Scope

The main published PG18 cross-engine benchmark matrix in
[Performance and Benchmarks](performance/README.md) is currently based
on the pretokenized input paths:

- `psql_bm25s ids` uses `int4[]`
- `psql_bm25s text[]` uses `text[]`

Scalar `text` and `varchar` are supported for ordinary application
schemas, but they are not the basis of the current public cross-engine
matrix.

## Multicolumn Fusion

Multicolumn fusion indexes currently support:

- `text[]`
- `varchar[]`
- `text`
- `varchar`

Scalar multicolumn fusion tokenizes each indexed scalar column with the
index text options before fusing the resulting token stream. See
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md) for the
current rules and recommended usage.

## Practical Guidance

Use:

- `text` or `varchar` when you want the easiest schema-level start
- `text[]` or `varchar[]` when you already own tokenization and want the
  clearest text-token contract
- `int4[]` when you need the highest-throughput exact path and can own
  token IDs upstream

Related docs:

- [API Reference](api-reference.md)
- [Query Semantics](query-semantics.md)
- [Performance and Benchmarks](performance/README.md)
- [Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md)
