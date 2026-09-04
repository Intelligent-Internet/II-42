# PG18 BEIR Extension Matrix Raw Archive

This directory contains the raw artifacts for the final
`2026-03-31` PG18 benchmark matrix.

Scope:

- 15 BEIR datasets
- 5 engines
  - `upstream_bm25s`
  - `psql_bm25s_ids`
  - `psql_bm25s_text`
  - `pg_search`
  - `vchord_bm25`

Contents:

- `results/<dataset>/<engine>.json`
  The raw per-task benchmark outputs.
- `manifest.json`
  The matrix task manifest used for the run.
- `launch.json`
  Launch-state tracking for the task fleet.
- `summary.json`
  Aggregated status snapshot from the orchestration pass.
- `status-matrix.json`
  Final machine-readable matrix status view.
- `status-matrix.md`
  Final human-readable matrix status view.

This directory is the raw archive behind the rolled-up current files in:

- `../../canonical/`
- `../../diagnostics/`

It is also the authoritative raw source for current cross-engine
PostgreSQL benchmark comparisons in this repository.
