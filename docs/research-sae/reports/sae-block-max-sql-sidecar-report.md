# SAE Block-Max SQL Sidecar Report

Date: 2026-05-12

## Purpose

This report records the Phase 0/1 implementation for the native sparse-impact
index exploration.

The goal of this phase is not PostgreSQL access-method performance. The goal
is to prove that the simulator contract can be represented with SQL tables
that mirror the planned native payload:

```text
doc_table
dimension_dictionary
block_directory
dimension_block_entries
impact_postings
query_dimensions
expected_rankings
```

## Implemented Scripts

```text
scripts/research_sae_block_max_sql_sidecar.py
scripts/test_research_sae_block_max_sql_sidecar.py
```

`research_sae_block_max_sql_sidecar.py` builds a native-shaped SAE sparse
impact artifact, loads it into PostgreSQL, and runs SQL block-max traversal.

`test_research_sae_block_max_sql_sidecar.py` builds a small deterministic
synthetic corpus and verifies:

- brute-force exact top-k;
- Python block-max top-k;
- SQL block-max top-k.

## SQL Traversal Shape

The SQL query intentionally does not use a plain postings `GROUP BY` as the
retrieval algorithm.

It follows the native traversal contract:

```text
query_dimensions
  -> dimension_block_entries
  -> block upper bounds
  -> ordered block prefixes
  -> first prefix where next_bound <= kth_score
  -> final top-k from opened blocks
```

The prefix calculation is relational and not meant to be the final fast path.
It is a correctness bridge before implementing the same contract in a
standalone C reader.

## Smoke Result

### Deterministic Hand-Written Sparse Smoke

Command:

```bash
python3 scripts/test_research_sae_block_max_sql_sidecar.py \
  --dsn 'dbname=postgres' \
  --keep-output
```

Result:

```text
block-max SQL sidecar smoke passed
```

Summary:

| Metric | Value |
| --- | ---: |
| queries | `3` |
| SQL exact matches | `3` |
| Python exact matches | `3` |
| documents | `6` |
| dimensions | `6` |
| postings | `12` |
| blocks | `3` |
| block entries | `9` |
| mean query terms | `2.00` |
| mean positive blocks | `2.00` |
| mean opened blocks | `2.00` |
| mean opened docs | `4.00` |
| mean scored docs | `3.00` |
| mean decoded postings | `4.00` |

### End-to-End Synthetic SAE Smoke

Command shape:

```bash
python3 scripts/research_sae_smoke.py \
  --output-dir "$tmpdir" \
  --documents 60 \
  --queries 6 \
  --embedding-dim 32 \
  --clusters 6 \
  --latent-dims 64 \
  --active-dims 8 \
  --epochs 2

python3 scripts/research_sae_block_max_sql_sidecar.py \
  --dsn 'dbname=postgres' \
  --schema sae_research_block_max_e2e \
  --run-id e2e \
  --documents "$tmpdir/data/documents.jsonl" \
  --queries "$tmpdir/data/queries.jsonl" \
  --doc-latents "$tmpdir/run/doc_latents.jsonl" \
  --query-latents "$tmpdir/run/query_latents.jsonl" \
  --output-dir "$tmpdir/block_max_sql" \
  --score-mode normalized_idf_dot \
  --top-k 10 \
  --layout sae_tree \
  --block-size 4 \
  --reset
```

Result:

| Metric | Value |
| --- | ---: |
| queries | `6` |
| SQL exact matches | `6` |
| Python exact matches | `6` |
| documents | `60` |
| dimensions | `24` |
| postings | `480` |
| blocks | `15` |
| block entries | `161` |
| mean query terms | `7.83` |
| mean positive blocks | `15.00` |
| mean opened blocks | `4.33` |
| mean opened docs | `17.33` |
| mean scored docs | `17.33` |
| mean decoded postings | `97.67` |

## Current Boundary

This phase proves:

- the native block metadata can be materialized from current SAE artifacts;
- SQL can reproduce the exact block-max stopping condition;
- Python block-max and SQL block-max both match brute-force exact top-k;
- traversal diagnostics are available for future quality/cost reports.

This phase does not prove:

- C reader performance;
- PostgreSQL access-method integration;
- mutable delta overlay correctness;
- BM25-only parity with the current production BM25 payload;
- dense-vector replacement quality on large production corpora.

## Next Implementation Step

The next phase is a standalone C reader over the same logical payload:

```text
export native-shaped payload
-> decode in C
-> run exact block-max traversal
-> match Python and SQL top-k
```

Only after that should this move into a PostgreSQL read-only native payload.

Phase 2 implementation status is recorded in:

```text
sae-block-max-c-reader-report.md
```
