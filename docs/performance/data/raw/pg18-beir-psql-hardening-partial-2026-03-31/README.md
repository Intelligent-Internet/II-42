# PG18 BEIR `psql_bm25s` Hardening Partial Raw Archive

This directory contains the raw artifacts for the
`chore/bm25s-simd-pg-common-hardening` branch rerun that was executed
against the same PG18/GCP benchmark setup as the current authoritative
matrix, then stopped early once the direction of the results was clear.

Scope:

- 15 BEIR datasets
- 2 engines
  - `psql_bm25s_ids`
  - `psql_bm25s_text`
- 30 total branch-evaluation cells
- 25 locally recovered result JSONs
- 5 intentionally stopped cells

Contents:

- `results/<dataset>/<engine>.json`
  Raw per-task benchmark outputs recovered before the run was stopped.
- `manifest.json`
  Sanitized task manifest for the rerun.
- `launch.json`
  Launch-state tracking captured during orchestration.
- `summary.json`
  Final partial summary after local recovery and shutdown.
- `status-matrix.json`
  Machine-readable final partial matrix view.
- `status-matrix.md`
  Human-readable final partial matrix view.
- `comparison-current.json`
  Partial comparison against the current authority in
  `../../diagnostics/pg18-beir-extension-matrix-current-2026-03-31.json`.
- `comparison-so-far.md`
  Human-readable old/new/delta table for the completed subset.
- `progress.log`
  Orchestration progress log from the live run.

This archive is useful for branch evaluation and PR discussion, but it
is not a replacement for the project-wide authority in:

- `../../diagnostics/pg18-beir-extension-matrix-current-2026-03-31.json`

At archive finalization time:

- `25/30` cells had validated local results
- `5/30` cells were intentionally stopped
- the partial comparison covered `12` datasets with complete
  `ids + text[]` pairs, plus a partial `fever` `ids` result
