# SAE Block-Max Phase 4.5 Super-Block Skip Report

Date: 2026-05-12

## Purpose

Phase 4.5 implements the read-only super-block skip idea from Phase 4.4 as an
opt-in packed generation extension, then measures whether it should become the
default path.

The implementation deliberately remains read-only:

- no delta overlay;
- no mutable maintenance;
- no heap visibility callback changes;
- exact top-k parity remains the gate.

## Implemented Prototype

`SBMXM001` now supports a version-2 packed generation when
`--doc-super-block-size > 0` and `micro_block_size = 1`.

The v2 payload adds:

```text
generation header:
  super_block_size
  super_block_count
  super_entry_count

per dimension:
  super_entry_start
  super_entry_count

super entry:
  super_block_id
  first_block_id
  entry_start
  entry_count
  posting_start
  max_impact
```

At query time, the C reader builds upper bounds over document-range
super-blocks:

```text
query dims
  -> dimension-local super entries
  -> super-block upper bounds
  -> open only ranges whose bound can beat visible top-k
  -> exact doc-bound entry scoring inside opened ranges
```

The old v1 packed generation remains the default. Super-block generations are
opt-in through:

```bash
--doc-super-block-size 64
```

`block_entry_binary_steps` is reused as the experimental super-entry visit
counter in the current research result shape. The stable path keeps it at zero.

## Result

The prototype is correct but not useful as a default. All tested configurations
remain exact, but document-range max-impact bounds are too loose: the traversal
still opens most documents, while adding directory work and payload bytes.

Five-dataset mean results:

| Path | Query ms | Super-entry visits | Block-entry visits | Opened docs | Payload bytes | Resident memory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| doc-bound default | 1.616 | 0.0 | 7236.5 | 100.0 | 1214702 | 1683038 |
| super size 4 | 6.152 | 5958.5 | 5834.4 | 1306.4 | 3810971 | 4298777 |
| super size 8 | 5.797 | 5145.1 | 7160.4 | 1838.1 | 3495290 | 3983095 |
| super size 16 | 5.453 | 4110.3 | 7236.3 | 1876.9 | 3091259 | 3579065 |
| super size 32 | 4.574 | 2897.9 | 7236.5 | 1877.0 | 2602984 | 3090790 |
| super size 64 | 3.910 | 1776.4 | 7236.5 | 1877.0 | 2121837 | 2609642 |

The smaller super-blocks have tighter bounds but too many directory entries.
The larger super-blocks reduce directory work but open almost the full corpus.
Neither side beats the Phase 4.3 doc-bound path.

## Decision

Keep v2 super-block support as an opt-in research path, but do not enable it by
default. The default exporter and benchmark now use `--doc-super-block-size 0`,
which preserves the Phase 4.3 doc-bound fast path.

This is a useful negative result. The failure mode is not implementation
overhead alone; it is the bound shape. A document-range bound sums one maximum
per query dimension over a broad doc range, so the upper bound remains high
until most ranges have been opened.

## Next Direction

The next attempt should not be another doc-range size sweep. Phase 4.6 tests
impact-ordered skipping:

- keep doc-bound entries as the exact scoring unit;
- store per-dimension entries ordered by impact, or add a compact impact-order
  side directory;
- maintain a residual upper bound for unvisited tails;
- update candidates from high-impact entries first;
- stop only when current visible top-k beats the residual global upper bound.

That is closer to MaxScore / WAND over latent sparse postings and attacks the
real Phase 4.4 bottleneck: high-DF dimensions with many low-impact entries.

Phase 4.6 shows this exact threshold route is also not enough as-is. The
residual frontier remains too loose with broad query fanout. The next direction
should combine candidate-budget-aware query representation with approximate
high-impact candidate generation and exact rerank.

## Verification

Commands run:

```bash
python3 -m py_compile \
  scripts/research_sae_block_max_export_packed_microblock.py \
  scripts/research_sae_resident_generation_benchmark.py \
  scripts/research_sae_large_scale_entry_profile.py \
  scripts/test_research_sae_resident_generation_pg.py

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
make -B \
  PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/test_research_sae_resident_generation_pg.py \
  --temp-postgres \
  --library ./ii42.dylib

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/test_research_sae_resident_generation_pg.py \
  --temp-postgres \
  --library ./ii42.dylib \
  --doc-super-block-size 64

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/test_research_sae_packed_microblock_pg_generation.py \
  --temp-postgres \
  --library ./ii42.dylib

for size in 4 8 16 32 64; do
  PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
  python3 scripts/research_sae_resident_generation_benchmark.py \
    --temp-postgres \
    --library ./ii42.dylib \
    --datasets scifact scidocs nfcorpus arguana fiqa \
    --doc-super-block-size "$size" \
    --output-dir "results/sae/phase45/superblock-size-${size}"
done

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/research_sae_resident_generation_benchmark.py \
  --temp-postgres \
  --library ./ii42.dylib \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --output-dir results/sae/phase45/docbound-default-resident-generation
```
