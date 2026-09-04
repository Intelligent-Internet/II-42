# SAE Block-Max Phase 3.5 PostgreSQL Query-Plan Traversal Report

Date: 2026-05-12

## Purpose

Phase 3.4 proved the packed `SBMXM001` generation and standalone per-block query-plan traversal. Phase 3.5 ports that traversal into the PostgreSQL generation path and reduces packed resident decode memory.

The goal is still read-only generation execution. Delta overlays, mutable maintenance, and access-method integration remain out of scope.

## Changes

PostgreSQL traversal now builds a per-query block plan while computing block upper bounds:

```text
for each query dimension:
    visit its dimension block-entry range
    accumulate block upper bound
    append the matched entry reference to block_plan[block_id]

for each opened block:
    score block_plan[block_id] directly
```

This removes the previous second lookup stage where scoring opened blocks repeated binary searches for every `(query_dim, opened_block)` pair.

Packed generation decode also changed:

```text
before:
    SBMXM001 -> full SaePostingRow[] expansion

now:
    SBMXM001 -> packed_entry_masks[] + packed_impacts[]
```

The block-entry table is still expanded for now. This is the next memory target after the traversal path is stable.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/test_research_sae_packed_microblock_pg_generation.py
python3 scripts/test_research_sae_packed_microblock_reader.py
make -B PG_CONFIG=/opt/homebrew/Cellar/postgresql@18/18.3/bin/pg_config PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
python3 scripts/test_research_sae_packed_microblock_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/test_research_sae_block_max_pg_generation.py --temp-postgres --library ./ii42.dylib
python3 scripts/research_sae_packed_microblock_benchmark.py --output-dir results/sae/phase34/current
```

Results:

- packed PostgreSQL generation smoke passed;
- compact PostgreSQL generation smoke passed;
- packed standalone benchmark remains exact on all five prepared benchmark slices;
- packed PostgreSQL smoke asserts `block_entry_binary_steps = 0`.

## Real Payload Diagnostic

Using the `scifact` packed payload from `results/sae/phase34/current` and the first query:

```text
rows: 100
memory_bytes: 4531111
block_entry_binary_steps: 0
decoded_postings: 3246
opened_docs: 600
```

The previous packed PostgreSQL decode would have expanded all postings to `SaePostingRow[]`. For this payload shape, avoiding that expansion saves roughly the difference between full posting rows and float4 impacts, while preserving exact scoring. The remaining resident memory is dominated by the expanded block-entry table and lookup ranges.

## Follow-Up Status

Phase 3.6 completed the memory-focused step by replacing the full expanded
packed block-entry table with compact resident packed-entry metadata:

```text
SaePackedBlockEntry[] + packed_impacts[]
```

Phase 3.8 also validated `sae_overlap_greedy` as a better block-2 layout than
`sae_tree` on the prepared benchmark slices. See:

```text
sae-block-max-phase36-38-report.md
```
