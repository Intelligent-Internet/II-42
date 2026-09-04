# SAE Block-Max Standalone C Reader Report

Date: 2026-05-12

## Purpose

This report records Phase 2 of the native sparse-impact index exploration.

The goal is to move the exact block-max traversal out of Python and SQL into a
standalone C reader while still avoiding PostgreSQL access-method complexity.
The reader consumes a binary payload exported from the same logical artifact
used by the SQL sidecar.

## Implemented Files

```text
scripts/research_sae_block_max_export_payload.py
tests/research_sae_block_max_reader.c
scripts/test_research_sae_block_max_c_reader.py
```

## Payload Contract

The exporter writes a little-endian binary payload:

```text
magic/version/header counts
doc_table
block_directory
dimension_dictionary
dimension_block_entries
impact_postings
query dimensions
expected top-k rows
```

The C reader does not connect to PostgreSQL. It only:

- reads the payload;
- computes block upper bounds;
- opens blocks in descending bound order;
- accumulates document scores;
- stops when the next block cannot enter top-k;
- compares the result with expected top-k rows.

## Smoke Result

### Deterministic Hand-Written Sparse Smoke

Command:

```bash
python3 scripts/test_research_sae_block_max_c_reader.py
```

Result:

```text
block-max C reader smoke passed
```

The deterministic smoke uses the same hand-written sparse corpus as the SQL
sidecar smoke. It verifies that the standalone C traversal matches the Python
brute-force expected rows.

Output shape:

```text
queries=3 exact_matches=3
mean_opened_blocks=2.00
mean_opened_docs=4.00
mean_scored_docs=3.00
mean_decoded_postings=4.00
block-max C reader smoke passed
```

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

python3 scripts/research_sae_block_max_export_payload.py \
  --run-id c-e2e \
  --documents "$tmpdir/data/documents.jsonl" \
  --queries "$tmpdir/data/queries.jsonl" \
  --doc-latents "$tmpdir/run/doc_latents.jsonl" \
  --query-latents "$tmpdir/run/query_latents.jsonl" \
  --payload "$tmpdir/payload.bin" \
  --summary "$tmpdir/payload_summary.json" \
  --score-mode normalized_idf_dot \
  --top-k 10 \
  --layout sae_tree \
  --block-size 4

cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  tests/research_sae_block_max_reader.c \
  -o "$tmpdir/reader"

"$tmpdir/reader" "$tmpdir/payload.bin"
```

Result:

| Metric | Value |
| --- | ---: |
| queries | `6` |
| exact matches | `6` |
| documents | `60` |
| dimensions | `31` |
| postings | `480` |
| blocks | `15` |
| block entries | `199` |
| query dimensions | `48` |
| mean opened blocks | `4.67` |
| mean opened docs | `18.67` |
| mean scored docs | `18.67` |
| mean decoded postings | `88.83` |

## Current Boundary

This phase proves:

- the native-shaped payload can be exported as binary;
- C can decode the payload independently;
- exact block-max traversal can be implemented without SQL;
- the Phase 2 reader is ready to become the inner loop for larger benchmark
  payloads.

This phase does not prove:

- PostgreSQL memory-context integration;
- MVCC visibility handling;
- on-disk page format stability;
- mutable delta overlay behavior.

## Next Implementation Step

The immediate follow-up was Phase 2.5:

```text
larger benchmark payload
-> dimension range lookup instead of full scans
-> binary parity against SQL sidecar
```

Phase 2.5 has now implemented the first hardening step. See:

```text
sae-block-max-phase25-report.md
```

The remaining step before PostgreSQL integration is to repeat this benchmark
on real benchmark artifacts and document lookup memory footprint.

That step is now recorded in:

```text
sae-block-max-phase26-real-benchmark-report.md
```
