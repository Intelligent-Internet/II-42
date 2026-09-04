# SAE Block-Max Phase 4.0-4.4 Read-Only Index Surface Report

Date: 2026-05-12

## Purpose

Phase 4 keeps the native SAE block-max line read-only. The goal is to make the
PostgreSQL resident generation path feel like a future native index without
adding mutable delta overlay, background maintenance, or access-method write
paths too early.

The implemented steps are:

1. stable resident generation management API;
2. block-bound selectivity sweep for physical-layout decisions;
3. read-only index-facade query function over resident generations.
4. doc-bound fast path for one-document micro-block generations;
5. larger-scale entry-visitation profiling and MVCC-visible continuation.

## Resident Generation API

The resident table contract is now created and managed through experimental C
helpers:

```sql
SELECT ii42_sae_generation_create_table('schema_name', 'sae_generations');

SELECT ii42_sae_generation_upsert(
    'schema_name.sae_generations'::regclass,
    'generation-or-index-id',
    generation_bytes,
    '{"layout": "sae_overlap_greedy"}'::jsonb
);

SELECT *
FROM ii42_sae_generation_list(
    'schema_name.sae_generations'::regclass
);

SELECT ii42_sae_generation_delete(
    'schema_name.sae_generations'::regclass,
    'generation-or-index-id'
);
```

The table contract remains intentionally small:

```sql
generation_id text PRIMARY KEY,
generation bytea NOT NULL,
metadata jsonb NOT NULL DEFAULT '{}'::jsonb,
created_at timestamptz NOT NULL DEFAULT now()
```

The C helpers use `regclass` for already-created generation tables wherever a
table reference is needed. This keeps dynamic SQL table naming narrow and makes
call sites closer to a future relation-backed index surface.

## Read-Only Index Facade

Phase 4.2 adds a semantic facade over the by-id resident generation query:

```sql
SELECT *
FROM ii42_sae_readonly_index_query(
    'schema_name.sae_generations'::regclass,
    'index-id',
    ARRAY[1, 2, 3]::int4[],
    ARRAY[0.8, 0.6, 0.4]::real[],
    100,
    NULL::tid[]
);
```

It currently calls the same C traversal as
`ii42_sae_block_max_query_generation_by_id(...)`. The difference is the
API intent: callers pass an `index_id`, not a raw payload, and PostgreSQL
returns TID, score, rank, and traversal diagnostics. That is the minimum shape
needed before a future access method can hide the resident generation table.

## Selectivity Sweep

The new script is:

```text
scripts/research_sae_bound_selectivity_sweep.py
```

It fixes the resident PostgreSQL facade path and changes only physical layout
parameters. The first sweep covered:

```text
datasets: scifact, scidocs, nfcorpus, arguana, fiqa
layouts: sae_overlap_greedy, sae_signature
micro_block_sizes: 2, 4
score_mode: normalized_idf_dot
top_k: 100
```

Results:

| Dataset | Layout | Micro block | Exact | Query ms | Opened docs | Opened ratio | P95 opened | Payload KB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `sae_overlap_greedy` | 2 | 100/100 | 2.195 | 437.74 | 0.219 | 562.00 | 1111.6 |
| `scifact` | `sae_overlap_greedy` | 4 | 100/100 | 2.506 | 1216.80 | 0.608 | 1484.00 | 1048.8 |
| `scifact` | `sae_signature` | 2 | 100/100 | 2.097 | 504.42 | 0.252 | 652.00 | 1144.0 |
| `scifact` | `sae_signature` | 4 | 100/100 | 3.080 | 1488.36 | 0.744 | 1736.00 | 1101.5 |
| `scidocs` | `sae_overlap_greedy` | 2 | 100/100 | 2.073 | 420.00 | 0.210 | 536.00 | 1124.1 |
| `scidocs` | `sae_overlap_greedy` | 4 | 100/100 | 2.557 | 1207.32 | 0.604 | 1452.00 | 1064.6 |
| `scidocs` | `sae_signature` | 2 | 100/100 | 3.227 | 480.08 | 0.240 | 608.00 | 1152.3 |
| `scidocs` | `sae_signature` | 4 | 100/100 | 2.745 | 1479.64 | 0.740 | 1716.00 | 1114.6 |
| `nfcorpus` | `sae_overlap_greedy` | 2 | 100/100 | 2.064 | 365.13 | 0.177 | 440.00 | 1124.5 |
| `nfcorpus` | `sae_overlap_greedy` | 4 | 100/100 | 2.429 | 1053.85 | 0.511 | 1284.00 | 1051.3 |
| `nfcorpus` | `sae_signature` | 2 | 100/100 | 2.089 | 434.30 | 0.211 | 545.00 | 1159.6 |
| `nfcorpus` | `sae_signature` | 4 | 100/100 | 2.553 | 1348.19 | 0.654 | 1555.00 | 1108.4 |
| `arguana` | `sae_overlap_greedy` | 2 | 100/100 | 2.316 | 305.42 | 0.153 | 400.00 | 1089.6 |
| `arguana` | `sae_overlap_greedy` | 4 | 100/100 | 2.396 | 863.96 | 0.432 | 1092.00 | 1014.9 |
| `arguana` | `sae_signature` | 2 | 100/100 | 2.439 | 372.54 | 0.186 | 470.00 | 1127.3 |
| `arguana` | `sae_signature` | 4 | 100/100 | 2.638 | 1174.36 | 0.587 | 1456.00 | 1076.3 |
| `fiqa` | `sae_overlap_greedy` | 2 | 100/100 | 7.966 | 362.46 | 0.181 | 464.00 | 1124.8 |
| `fiqa` | `sae_overlap_greedy` | 4 | 100/100 | 2.712 | 1014.56 | 0.507 | 1360.00 | 1066.6 |
| `fiqa` | `sae_signature` | 2 | 100/100 | 2.528 | 411.26 | 0.206 | 548.00 | 1152.0 |
| `fiqa` | `sae_signature` | 4 | 100/100 | 2.744 | 1282.52 | 0.641 | 1588.00 | 1116.8 |

The timing column is useful for smoke-level regression only. The selectivity
signal is the important result here: among multi-document micro-block layouts,
`sae_overlap_greedy` with `micro_block_size = 2` was the best default.

## Doc-Bound Fast Path

Phase 4.3 revisits the `micro_block_size = 1` result now that the packed
resident format is compact enough to make doc-level blocks practical. With one
document per micro-block, the block upper bound is the exact document score:

```text
block_upper_bound(query, one_doc_block) == score(query, doc)
```

The C traversal now detects packed generations with `micro_block_size = 1` and
ranks directly from sorted block bounds. It still visits block entries to build
the exact bounds, but it does not reopen postings for a second scoring pass.
This makes `decoded_postings = 0` and `posting_slice_hits = 0` for the
doc-bound path.

Resident benchmark with the new default:

| Dataset | Exact | PG mean query ms | Memory bytes | Opened docs | Decoded postings |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `100/100` | `1.336` | `1669572` | `100.00` | `0.00` |
| `scidocs` | `100/100` | `1.426` | `1675892` | `100.00` | `0.00` |
| `nfcorpus` | `100/100` | `1.327` | `1719704` | `100.00` | `0.00` |
| `arguana` | `100/100` | `2.691` | `1667692` | `100.00` | `0.00` |
| `fiqa` | `100/100` | `1.301` | `1682332` | `100.01` | `0.00` |

The doc-bound selectivity sweep confirms the tradeoff:

| Dataset | Block 1 opened ratio | Block 2 opened ratio | Payload delta |
| --- | ---: | ---: | ---: |
| `scifact` | `0.050` | `0.219` | `+5.9%` |
| `scidocs` | `0.050` | `0.210` | `+5.3%` |
| `nfcorpus` | `0.048` | `0.177` | `+7.6%` |
| `arguana` | `0.050` | `0.153` | `+8.2%` |
| `fiqa` | `0.050` | `0.181` | `+5.1%` |

## Decision

The current read-only default remains:

```text
generation format: SBMXM001
layout: sae_overlap_greedy
micro_block_size: 1
score_mode: normalized_idf_dot
query API: ii42_sae_readonly_index_query(...)
```

The previous upper-bound selectivity bottleneck is resolved for the read-only
resident prototype: the exact path now opens roughly `top_k` documents instead
of `15-22%` of the corpus on these 2k-document slices. Do not move to mutable
delta overlay yet; the next bottleneck is now large-scale posting-entry
visitation and MVCC-visible continuation, not opened-doc selectivity.

Phase 4.4 completes that follow-up. A deterministic 1x/2x/4x scaled profile
over `scifact`, `scidocs`, `nfcorpus`, `arguana`, and `fiqa` remains exact and
keeps `opened_docs` near `top_k`, but `block_entry_visits` scales almost
linearly with corpus size. The read-only facade also now treats NULL `doc_tids`
entries as invisible documents and continues traversal until visible top-k is
safe. The resulting next step is a read-only high-DF dimension skip layer, not
mutable delta overlay.

Detailed results are recorded in:

```text
scripts/research_sae_large_scale_entry_profile.py
results/sae/phase44/entry-visitation/summary.md
sae-block-max-phase44-entry-visitation-report.md
```

Phase 4.5 implements that super-block idea as an opt-in `SBMXM001` v2 payload,
then rejects it as a default. The prototype is exact, but document-range
max-impact bounds are too loose and open most documents. The default path
therefore remains the Phase 4.3 doc-bound generation with
`--doc-super-block-size 0`. The next read-only direction is impact-ordered
skipping with residual tail upper bounds.

```text
sae-block-max-phase45-superblock-skip-report.md
```

## Verification

Commands run:

```bash
python3 -m py_compile scripts/test_research_sae_resident_generation_pg.py scripts/research_sae_resident_generation_benchmark.py scripts/research_sae_bound_selectivity_sweep.py scripts/test_research_sae_block_max_pg_generation.py
make -B PG_CONFIG=/opt/homebrew/Cellar/postgresql@18/18.3/bin/pg_config PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
python3 scripts/test_research_sae_resident_generation_pg.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_block_max_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_packed_microblock_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_packed_microblock_reader.py
python3 scripts/research_sae_bound_selectivity_sweep.py --temp-postgres --library ./ii42.dylib --datasets scifact scidocs nfcorpus arguana fiqa --layouts sae_overlap_greedy,sae_signature --micro-block-sizes 2,4 --output-dir results/sae/phase40/bound-selectivity
python3 scripts/research_sae_bound_selectivity_sweep.py --temp-postgres --library ./ii42.dylib --datasets scifact scidocs nfcorpus arguana fiqa --layouts sae_overlap_greedy --micro-block-sizes 1,2 --output-dir results/sae/phase43/doc-bound-selectivity
python3 scripts/research_sae_resident_generation_benchmark.py --temp-postgres --library ./ii42.dylib --datasets scifact scidocs nfcorpus arguana fiqa --output-dir results/sae/phase43/doc-bound-resident-generation
```
