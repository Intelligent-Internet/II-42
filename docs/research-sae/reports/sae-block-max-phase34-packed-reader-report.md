# SAE Block-Max Phase 3.4 Packed Micro-Block Reader Report

Date: 2026-05-12

## Purpose

Phase 3.3 showed that exact `micro_block_size=2` storage can be close to WAND-style posting storage size while preserving block-max pruning. Phase 3.4 turns that estimate into an executable read-only prototype.

The new `SBMXM001` format stores:

- document table: `doc_ord`, `block_id`, `tie_ord`;
- dimension directory: `global_dim_id`, entry range, posting start;
- 4-byte micro-block entries: `uint16 block_delta`, `uint8 doc_mask`, `uint8 posting_count`;
- float32 impact stream, one impact per posting;
- optional query/expected rows only for standalone parity harnesses.

The PostgreSQL generation function now also accepts `SBMXM001`; query dimensions and weights still arrive at runtime through the existing `ii42_sae_block_max_query_generation(...)` SQL path.

## Implementation

New files:

```text
scripts/research_sae_block_max_export_packed_microblock.py
tests/research_sae_packed_microblock_reader.c
scripts/test_research_sae_packed_microblock_reader.py
scripts/test_research_sae_packed_microblock_pg_generation.py
scripts/research_sae_packed_microblock_benchmark.py
```

Updated file:

```text
src/ii42_sae_blockmax.c
```

The standalone reader decodes the compact entry stream into an in-memory block-entry table so that the query traversal remains comparable to the current C path. This is intentional for this phase: the goal is to validate the compact generation bytes and SQL runtime contract before optimizing resident-memory layout.

The reader also includes the first Phase 3.5 traversal improvement: while computing block upper bounds, it builds a per-block query plan containing the matched entry references. When a block is opened, scoring reuses those references instead of binary-searching every `(query_dim, opened_block)` pair again.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/research_sae_block_max_export_packed_microblock.py scripts/research_sae_packed_microblock_benchmark.py scripts/test_research_sae_packed_microblock_reader.py scripts/test_research_sae_packed_microblock_pg_generation.py
python3 scripts/test_research_sae_packed_microblock_reader.py
make PG_CONFIG=/opt/homebrew/Cellar/postgresql@18/18.3/bin/pg_config PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
python3 scripts/test_research_sae_packed_microblock_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/research_sae_packed_microblock_benchmark.py --output-dir results/sae/phase34/current
```

The default `make` command could not find `pg_config` in `PATH`, and Homebrew PostgreSQL was compiled with a stale CLT SDK path. Passing explicit `PG_CONFIG` and `PG_SYSROOT` fixed the local build.

## Real-Artifact Results

The benchmark used `/tmp/ii42_sae_quality_matrix`, run `sae_8192_64`, `top_k=100`, `micro_block_size=2`, `layout=sae_tree`, and `score_mode=normalized_idf_dot`.

| Dataset | Docs | Queries | Exact | Generation bytes/posting | Payload bytes/posting | C seconds | Opened docs | Scored docs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 2000 | 100 | 100/100 | 8.089 | 9.125 | 0.186282 | 504.34 | 501.78 |
| `scidocs` | 2000 | 100 | 100/100 | 8.132 | 9.197 | 0.036792 | 476.10 | 472.91 |
| `nfcorpus` | 2063 | 100 | 100/100 | 8.023 | 9.031 | 0.036274 | 447.83 | 443.90 |
| `arguana` | 2000 | 100 | 100/100 | 8.006 | 9.064 | 0.035073 | 379.36 | 376.57 |
| `fiqa` | 2000 | 100 | 100/100 | 8.147 | 9.184 | 0.034754 | 405.84 | 400.77 |

Lookup diagnostics:

| Dataset | Mean block-entry visits | Mean binary steps | Mean posting slice hits | Mean decoded postings |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | 7097.76 | 0.00 | 2994.73 | 3304.92 |
| `scidocs` | 6744.33 | 0.00 | 2741.80 | 3006.07 |
| `nfcorpus` | 6130.23 | 0.00 | 2322.29 | 2571.93 |
| `arguana` | 6894.80 | 0.00 | 2512.16 | 2866.33 |
| `fiqa` | 6101.07 | 0.00 | 2349.56 | 2585.65 |

## Interpretation

This phase validates the storage direction:

- exact top-k parity holds on all five prepared benchmark slices;
- packed generation bytes are stable around `8.0-8.15 bytes/posting`;
- the full parity payload is around `9.0-9.2 bytes/posting` because it also contains test query rows;
- PostgreSQL can read the packed generation through the existing runtime query function.

The remaining bottleneck is not correctness or on-disk generation size. It is query-time lookup shape:

- the reader currently expands packed entries into a normal in-memory block-entry table;
- opened-doc ratios are still governed by the representation/layout problem;
- per-opened-block binary lookup is removable; the standalone packed reader now proves this by reducing binary lookup steps to zero with a per-block query plan.

## Follow-Up

Phase 3.5 ported the per-block query plan into the PostgreSQL generation path and reduced packed resident decode memory by avoiding full posting-row expansion. Phase 3.6-3.8 then compressed packed resident entry metadata and validated the overlap-greedy layout. See:

```text
sae-block-max-phase35-pg-query-plan-report.md
sae-block-max-phase36-38-report.md
```
