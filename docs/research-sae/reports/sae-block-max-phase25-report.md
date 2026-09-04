# SAE Block-Max Phase 2.5 Report

Date: 2026-05-12

## Purpose

This report records the Phase 2.5 hardening step between the standalone C
reader and a future PostgreSQL read-only native payload.

The goal was to remove the first C reader's full-scan lookup behavior and
verify a larger payload against Python and SQL.

## Implemented Files

```text
tests/research_sae_block_max_reader.c
scripts/research_sae_block_max_phase25_benchmark.py
```

## Reader Change

The C reader now builds a range index after loading the payload:

```text
global_dim_id -> block_entry_range
```

Query-time traversal now uses that range index in two places:

- block upper bounds are built by expanding only the query dimensions'
  block-entry ranges;
- scoring an opened block finds the `(dimension, block)` block entry and then
  reads only that entry's posting slice.

This replaces the previous prototype behavior that scanned all block entries
and all postings for every query dimension.

## Benchmark Command

```bash
python3 scripts/research_sae_block_max_phase25_benchmark.py \
  --dsn 'dbname=postgres' \
  --output-dir /tmp/sae_phase25_benchmark \
  --documents 180 \
  --queries 12 \
  --embedding-dim 48 \
  --clusters 12 \
  --latent-dims 128 \
  --active-dims 12 \
  --epochs 3 \
  --top-k 20 \
  --block-size 8 \
  --seed 23
```

## Benchmark Result

Parity:

| Path | Exact matches |
| --- | ---: |
| Python block-max | `12/12` |
| SQL sidecar | `12/12` |
| C reader | `12/12` |

Payload:

| Metric | Value |
| --- | ---: |
| documents | `180` |
| queries | `12` |
| dimensions | `75` |
| postings | `2160` |
| blocks | `23` |
| block entries | `752` |
| query dimensions | `143` |
| block size | `8` |

C reader traversal:

| Metric | Value |
| --- | ---: |
| mean opened blocks | `20.00` |
| mean opened docs | `156.00` |
| mean scored docs | `154.58` |
| mean decoded postings | `585.50` |
| wall time | `0.148135s` |

Pipeline timings:

| Step | Seconds |
| --- | ---: |
| SAE smoke train/encode | `1.457304` |
| SQL sidecar | `1.168874` |
| payload export | `0.909035` |
| C compile | `0.171065` |
| C reader | `0.148135` |

## Interpretation

Phase 2.5 validates the reader architecture:

- the payload can be decoded into compact dimension ranges;
- upper-bound traversal no longer depends on full block-entry scans;
- opened-block scoring no longer depends on full posting scans;
- C, SQL, and Python still produce exact top-k parity.

The synthetic benchmark is larger than the deterministic smoke, but it is not
yet a production-scale performance claim. The opened-doc fraction is still
high on this synthetic setup, so future quality work should continue improving
latent selectivity and block layout.

## Next Step

Before Phase 3 PostgreSQL integration, the useful final Phase 2.x step is:

```text
real benchmark payload
-> same binary exporter
-> same C reader
-> compare C diagnostics against SQL/Python
-> document lookup complexity and memory footprint
```

After that, Phase 3 should introduce a read-only PostgreSQL payload that reuses
the C traversal logic and adds PostgreSQL memory-context and TID return
plumbing.

This real-artifact step is now recorded in:

```text
sae-block-max-phase26-real-benchmark-report.md
```
