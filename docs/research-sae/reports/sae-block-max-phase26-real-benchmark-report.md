# SAE Block-Max Phase 2.6 Real Artifact Benchmark Report

Date: 2026-05-12

## Purpose

This report records Phase 2.6 of the native sparse-impact index exploration.

Phase 2.6 moves from synthetic data to prepared real benchmark artifacts under:

```text
/tmp/ii42_sae_quality_matrix
```

The goals were:

- run the same binary C reader on real BEIR-derived SAE artifacts;
- compare C results with SQL/Python sidecar parity on representative datasets;
- measure resident payload memory and lookup complexity;
- define the minimum Phase 3 read-only PostgreSQL payload boundary.

## Implemented Files

```text
scripts/research_sae_block_max_phase26_real_benchmark.py
tests/research_sae_block_max_reader.c
```

The C reader now reports:

```text
memory_bytes
block_entries
postings
max_global_dim_id
mean_block_entry_visits
mean_block_entry_binary_steps
mean_posting_slice_hits
mean_decoded_postings
```

It also uses an incremental top-k threshold buffer. This replaced the previous
prototype behavior that sorted all seen documents after each opened block.

## SQL/Python/C Parity Runs

Command shape:

```bash
python3 scripts/research_sae_block_max_phase26_real_benchmark.py \
  --dsn 'dbname=postgres' \
  --output-dir /tmp/sae_phase26_real_scifact2 \
  --datasets scifact \
  --top-k 100 \
  --block-size 8
```

Representative completed parity runs:

| Dataset | Docs | Queries | Python exact | SQL exact | C exact | C seconds | SQL seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `2000` | `100` | `100/100` | `100/100` | `100/100` | `0.314913` | `85.116496` |
| `scidocs` | `2000` | `100` | `100/100` | `100/100` | `100/100` | `1.610853` | `84.231991` |

The `scidocs` C timing above was captured before the incremental top-k buffer
optimization. The all-dataset C-only run below reflects the current optimized
reader.

The SQL sidecar remains a correctness bridge. It is intentionally much slower
than the C reader because it expresses prefix-stop traversal relationally.

## All-Dataset C Reader Run

Command:

```bash
python3 scripts/research_sae_block_max_phase26_real_benchmark.py \
  --output-dir /tmp/sae_phase26_real_all_c2 \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --block-size 8 \
  --skip-sql
```

Results:

| Dataset | Docs | Queries | C exact | Memory MiB | C seconds | Mean opened docs | Mean decoded postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `2000` | `100` | `100/100` | `6.011` | `0.251742` | `1957.04` | `7703.87` |
| `scidocs` | `2000` | `100` | `100/100` | `6.155` | `0.112016` | `1974.48` | `7284.59` |
| `nfcorpus` | `2063` | `100` | `100/100` | `6.112` | `0.104452` | `2007.32` | `6684.14` |
| `arguana` | `2000` | `100` | `100/100` | `5.986` | `0.104080` | `1919.84` | `7589.82` |
| `fiqa` | `2000` | `100` | `100/100` | `6.094` | `0.104738` | `1919.52` | `6516.60` |

Lookup complexity:

| Dataset | Mean block-entry visits | Mean binary steps | Mean posting slice hits |
| --- | ---: | ---: | ---: |
| `scifact` | `5433.21` | `98553.34` | `5388.59` |
| `scidocs` | `5263.51` | `98420.81` | `5237.62` |
| `nfcorpus` | `4693.50` | `96648.06` | `4639.75` |
| `arguana` | `5235.37` | `96463.13` | `5142.95` |
| `fiqa` | `4795.14` | `93595.87` | `4713.56` |

## Memory Interpretation

The current in-memory binary reader uses about `6 MiB` for a `~2000` document,
`128k` posting SAE payload:

```text
about 49-50 bytes per posting
about 66-69 bytes per dimension-block entry
```

This includes research-reader overhead such as document/query strings and
expected rows. A PostgreSQL read-only payload can be smaller because it should
store TIDs and compact offsets instead of debug strings and expected answers.

## Performance Interpretation

The range-lookup reader is now fast enough to justify Phase 3 scaffolding:

- C exactness is stable on five real benchmark artifacts.
- C traversal is around `0.10-0.31s` for `100` queries on `~2000` documents.
- SQL/Python/C diagnostics match on `scifact`; earlier `scidocs` SQL parity
  also matched.
- The dominant quality/system issue is no longer C lookup mechanics. It is
  that current SAE block bounds still open most documents on these datasets.

The high opened-doc fraction is a representation/layout issue:

```text
opened docs: roughly 1920-2007 out of 2000
```

That means Phase 3 should be read-only and diagnostic-first. It should not be
marketed as a production breakthrough until the learned sparse representation
or physical layout becomes more selective.

## Phase 3 Minimum Cut

The minimum useful PostgreSQL read-only payload should include:

- resident generation header;
- document table storing TIDs instead of debug document ids;
- block directory;
- dimension dictionary with `global_dim_id -> block_entry_range`;
- dimension block entries;
- impact postings;
- SQL-facing function that accepts query dimensions and weights;
- returned TIDs, scores, and diagnostics.

Do not add mutable delta overlay in Phase 3. The next PostgreSQL step should
only prove:

```text
binary reader traversal
-> resident PostgreSQL generation
-> read-only SQL function returning TIDs
-> exact parity with the standalone C reader
```

MVCC, generation swap, delta overlay, and BM25 coexistence should remain Phase
4+ concerns.
