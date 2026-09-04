# Tantivy Follow-up Local Main-vs-Branch Comparison

Date: 2026-04-02

This archive contains the final local `main` vs
`codex/tantivy-followup-planning` comparison that closed out the
Tantivy-inspired optimization branch.

Comparison scope:

- local PostgreSQL 17 benchmark
- official 15-dataset BEIR list
- `psql_bm25s_ids` and `psql_bm25s_text` only
- `benchmark_beir_official.py`
- `--path-mode both`
- `--skip-upstream`
- `--auto-maintain false`

Compared code states:

- baseline `main`: `f200342`
- candidate branch: `47f811f`

Files:

- `main.json`
  Baseline local run on `main`.
- `branch.json`
  Local run on `codex/tantivy-followup-planning`.
- `summary.json`
  Machine-readable per-dataset comparison summary.
- `summary.md`
  Human-readable comparison table.

Headline result:

- `psql_bm25s_ids`
  - median QPS delta: `+18.60%`
  - wins: `14/15`
- `psql_bm25s_text`
  - median QPS delta: `+24.52%`
  - wins: `13/15`

The main exceptions are the smallest or noisiest benchmark slices:

- `arguana`
  Tiny corpus (`8,674` docs) where fixed PostgreSQL and `text[]`
  overhead becomes a larger fraction of total latency.
- `trec-covid`
  Only `50` queries, so a few slower samples can move the percentage
  materially.

For the larger and more representative corpora, including
`cqadupstack`, `fever`, `msmarco`, `nq`, and `quora`, the branch stayed
positive on both indexed paths.
