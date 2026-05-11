# Benchmark Data Layout

This directory contains only the current public PG18 benchmark matrix
and the raw data needed to verify it.

## Canonical

Canonical `psql_bm25s` result files:

- `canonical/official-beir-ids-current-2026-04-02.json`
- `canonical/official-beir-text-current-2026-04-02.json`

These are the current PG18 BEIR runs for the two published
high-throughput paths:

- `psql_bm25s ids`
- `psql_bm25s text[]`

## Diagnostics

Current public rollups:

- `diagnostics/official-beir-python-reference-current-2026-04-02.json`
- `diagnostics/official-beir-pg18-comparison-current-2026-04-02.json`
- `diagnostics/pg18-beir-extension-matrix-current-2026-04-02.json`
- `diagnostics/pg18-beir-quality-matrix-current-2026-05-06.json`

The main public comparison is
`pg18-beir-extension-matrix-current-2026-04-02.json`, a PG18 `15 x 5`
BEIR matrix over:

- Python reference implementation `bm25s`
- `psql_bm25s ids`
- `psql_bm25s text[]`
- ParadeDB `pg_search`
- TensorChord `vchord_bm25`

## Raw Cells

The raw per-dataset result cells behind the current public matrix are
stored under:

- `raw/pg18-beir-current-matrix-2026-04-02/results/<dataset>/<engine>.json`

That directory contains `75` result cells:

- `15` datasets
- `5` engines per dataset

The rolled-up current matrix was checked against these raw cells on:

- `stats`
- `build_ms`
- `query.count`
- `query.qps`

No other benchmark artifacts are retained in this public performance
data directory.
