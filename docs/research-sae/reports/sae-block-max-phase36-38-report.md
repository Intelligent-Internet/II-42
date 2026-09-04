# SAE Block-Max Phase 3.6-3.8 Report

Date: 2026-05-12

## Purpose

This phase finishes the next three read-only native payload steps:

1. compress PostgreSQL resident packed-entry metadata;
2. preserve exact SQL/Python/C parity and the zero binary-lookup query path;
3. improve block-bound selectivity with a better document layout.

The scope remains read-only generation execution. Mutable deltas, PostgreSQL
access-method integration, and production maintenance are still deferred.

## Phase 3.6: Resident Packed-Entry Compression

The packed `SBMXM001` PostgreSQL reader no longer expands every micro-block
entry into the full `SaeBlockEntry` shape.

```text
previous packed resident state:
    SaeBlockEntry[] + packed_entry_masks[] + packed_impacts[]

current packed resident state:
    SaePackedBlockEntry[] + packed_impacts[]

SaePackedBlockEntry:
    block_id
    posting_start
    max_impact
    doc_mask
    posting_count
```

`global_dim_id` is no longer duplicated per packed entry. The dimension
directory owns `block_entry_start` and `block_entry_count`, and the query-time
range lookup maps each query dimension to that compact entry range.

This is a deliberate middle step. It does not yet keep the raw 4-byte
on-disk entry stream resident, because query traversal still benefits from
direct `block_id`, `posting_start`, and `max_impact` access. The next memory
target is to delta-pack those resident fields without reintroducing per-opened
block binary lookup.

## Phase 3.7: Parity And Runtime Checks

Commands run:

```bash
python3 -m py_compile scripts/research_sae_block_max_sim.py scripts/test_research_sae_packed_microblock_pg_generation.py scripts/research_sae_block_max_export_packed_microblock.py scripts/research_sae_packed_microblock_benchmark.py
make -B PG_CONFIG=/opt/homebrew/Cellar/postgresql@18/18.3/bin/pg_config PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
python3 scripts/test_research_sae_packed_microblock_reader.py
python3 scripts/test_research_sae_packed_microblock_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_block_max_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/research_sae_packed_microblock_benchmark.py --output-dir results/sae/phase36/resident-entry-compression
```

Results:

- packed standalone reader remains exact on all five prepared benchmark slices;
- packed PostgreSQL generation smoke passed;
- compact PostgreSQL generation smoke passed;
- packed PostgreSQL diagnostics still report `block_entry_binary_steps = 0`;
- real scifact packed generation query reports `memory_bytes = 2519928`,
  down from the previous diagnostic value of about `4531111`.

The memory reduction is meaningful but not final. The remaining resident cost
is mainly compact entry metadata, dimension ranges, document rows, and exact
float32 impacts.

## Phase 3.8: Layout Selectivity

The previous best layout was `sae_tree`. This phase adds
`sae_overlap_greedy`, a deterministic layout that forms each micro-block by
greedily grouping documents with overlapping high-impact latent dimensions.

The design goal is not to change scores. It reduces bound slack: a block's
safe upper bound is tighter when the documents inside that block share the
same latent dimensions instead of contributing unrelated per-dimension maxima.

Simulation command:

```bash
python3 scripts/research_sae_block_max_sim.py --work-dir /tmp/ii42_sae_quality_matrix --output-dir results/sae/phase38/overlap-layout --block-sizes 2,4,8 --layouts sae_tree,sae_overlap_greedy,sae_signature,simhash --top-k 100
```

Packed C-reader command:

```bash
python3 scripts/research_sae_packed_microblock_benchmark.py --layout sae_overlap_greedy --output-dir results/sae/phase38/overlap-packed
```

Best simulator rows:

| Dataset | Best layout | Block | Exact | Opened docs | Scored docs | Bound slack |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `sae_overlap_greedy` | 2 | `100/100` | `0.219` | `0.219` | `1.40` |
| `scidocs` | `sae_overlap_greedy` | 2 | `100/100` | `0.210` | `0.210` | `1.40` |
| `nfcorpus` | `sae_overlap_greedy` | 2 | `100/100` | `0.177` | `0.176` | `1.34` |
| `arguana` | `sae_overlap_greedy` | 2 | `100/100` | `0.153` | `0.153` | `1.33` |
| `fiqa` | `sae_overlap_greedy` | 2 | `100/100` | `0.181` | `0.181` | `1.38` |

Packed C-reader comparison:

| Dataset | Tree opened docs | Overlap opened docs | Tree memory bytes | Overlap memory bytes |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | `504.34` | `437.74` | `3081062` | `2932142` |
| `scidocs` | `476.10` | `420.00` | `3103448` | `2972648` |
| `nfcorpus` | `447.83` | `365.13` | `3134371` | `2929911` |
| `arguana` | `379.36` | `305.42` | `3033544` | `2811004` |
| `fiqa` | `405.84` | `362.46` | `3100511` | `2981431` |

All overlap packed runs remain exact (`100/100`) with
`block_entry_binary_steps = 0`.

## Interpretation

`sae_overlap_greedy` is a better default research layout than `sae_tree` for
the packed micro-block path. It improves pruning on every prepared benchmark
slice and slightly reduces entry count because documents sharing latent
dimensions generate fewer distinct dimension/block entries.

The result is still not the final "extreme" target. Even with block size `2`,
the current exact traversal opens about `15-22%` of documents. The next real
breakthrough has to come from representation rather than weight tuning:

- tighter resident delta-packed entry metadata;
- stronger learned document layout optimized for bound slack;
- optional two-level bounds where a cheap coarse page bound narrows the exact
  micro-block list;
- query-side sparse encoder improvements that reduce noisy latent fanout.

## Current Decision

For the next read-only native payload iteration, use:

```text
generation format: SBMXM001
micro_block_size: 2
layout: sae_overlap_greedy
query traversal: per-block query plan
resident representation: SaePackedBlockEntry[] + float32 impacts
```

This is exact, compact enough for the prototype, and directly portable toward
a PostgreSQL-resident generation table.

## Follow-Up

Phase 3.9 completed the PostgreSQL-resident generation table prototype and the
next resident memory compression step. See:

```text
sae-block-max-phase39-resident-generation-report.md
```
