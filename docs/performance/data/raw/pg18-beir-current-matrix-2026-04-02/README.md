# PG18 BEIR Current Matrix Raw Cells

This directory stores the raw per-dataset result cells behind the current
public PG18 `15 x 5` BEIR matrix.

Scope:

- datasets: `15` official BEIR subsets
- engines per dataset: `5`
- raw cells: `75`
- PostgreSQL: `18`
- machine type: Google Cloud `n2-standard-16`
- `top_k = 1000`

Engines:

- Python reference implementation `bm25s`
- `psql_bm25s ids`
- `psql_bm25s text[]`
- ParadeDB `pg_search`
- TensorChord `vchord_bm25`

The current rollup is:

- `../../diagnostics/pg18-beir-extension-matrix-current-2026-04-02.json`

The published summary is:

- `../../../README.md`
