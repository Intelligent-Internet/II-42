# PG18 PSQL-Only Tantivy Branch Rerun (2026-04-02)

This archive stores the Google Cloud PG18 rerun for the current
`codex/tantivy-followup-planning` branch.

Scope:

- datasets: `15` official BEIR subsets
- engines: `psql_bm25s_ids`, `psql_bm25s_text`
- cloud: Google Cloud `us-east4-a`
- machine type: `n2-standard-16`
- PostgreSQL: `18`
- comparison baseline: the project `2026-03-31` PG18 `15 x 5` matrix

Files:

- `results/`: raw per-dataset result JSON (`30` files)
- `manifest.json`: orchestrator manifest with project id redacted
- `launch.json`: task-level launch state
- `summary.json`: run summary
- `status-matrix.json` / `status-matrix.md`: final task matrix
- `comparison-vs-doc-baseline.json` / `.md`: per-dataset comparison against
  the documented `2026-03-31` baseline

Aggregate outcome versus the documented baseline:

- `psql_bm25s_ids`
  - at or above upstream: `12/15`
  - median vs upstream: `3.97x`
- `psql_bm25s_text[]`
  - at or above upstream: `11/15`
  - median vs upstream: `3.93x`
