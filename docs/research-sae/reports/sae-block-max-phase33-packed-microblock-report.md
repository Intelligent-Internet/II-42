# SAE Block-Max Phase 3.3 Packed Micro-Block Report

## Scope

Phase 3.3 estimates the physical size of the design implied by Phase 3.1 and
Phase 3.2:

```text
micro-block size 2 for pruning
+ compact inline postings for metadata efficiency
```

Implemented script:

```text
scripts/research_sae_packed_microblock_estimator.py
```

This is a size and layout prototype, not a mutable PostgreSQL AM path.

## Encoding Estimate

The estimate keeps exact float32 impacts:

```text
doc table:          docs * 12 bytes
dimension ranges:   dimensions * 16 bytes
micro-block entry:  block delta + doc mask + count = 4 bytes
impact payload:     postings * 4 bytes
```

For `micro_block_size=2`, the doc mask identifies which of the two docs in the
micro-block has an impact for that dimension. This removes the need to store a
full `doc_ord`, `block_id`, and posting row per impact.

The estimate intentionally remains conservative:

- no float16 or quantized impact compression;
- no varint byte shaving in the reported totals;
- no cross-dimension page compression;
- no production TID packing assumptions beyond doc table budget.

## Command

```bash
python3 scripts/research_sae_packed_microblock_estimator.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/sae_phase33_packed_microblock \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --score-mode normalized_idf_dot \
  --latent-dims 8192 \
  --active-dims 64 \
  --layout sae_tree \
  --micro-block-size 2
```

## Results

| Dataset | Blocks | Entries | Postings | Current bytes | Packed bytes | WAND bytes | Packed/current |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 1000 | 118667 | 128000 | 5,986,820 | 1,035,324 | 1,064,656 | 0.173 |
| `scidocs` | 1000 | 119411 | 128000 | 6,007,836 | 1,040,828 | 1,067,184 | 0.173 |
| `nfcorpus` | 1032 | 120482 | 132032 | 6,128,028 | 1,059,260 | 1,097,208 | 0.173 |
| `arguana` | 1000 | 116209 | 128000 | 5,926,888 | 1,024,740 | 1,063,904 | 0.173 |
| `fiqa` | 1000 | 119251 | 128000 | 6,007,216 | 1,042,764 | 1,069,760 | 0.174 |

Bytes per posting:

| Dataset | Current | Packed | WAND |
| --- | ---: | ---: | ---: |
| `scifact` | 46.77 | 8.09 | 8.32 |
| `scidocs` | 46.94 | 8.13 | 8.34 |
| `nfcorpus` | 46.41 | 8.02 | 8.31 |
| `arguana` | 46.30 | 8.01 | 8.31 |
| `fiqa` | 46.93 | 8.15 | 8.36 |

## Interpretation

This is the first result that combines both requirements:

```text
candidate pruning target: block-size 2
metadata target: near WAND postings
```

The packed micro-block estimate is only about `17.3%` of the current
block-entry-plus-posting representation and is slightly smaller than the WAND
posting estimate, while preserving the block-2 pruning strategy from Phase 3.1.

This makes packed micro-blocks the best next implementation target.

## Next Step

Phase 3.4 should implement a read-only packed micro-block reader:

- binary generation magic for the packed format;
- per-dimension entry ranges;
- fixed `micro_block_size=2`;
- entry stream with block delta/doc mask/count;
- exact float32 impact stream;
- C reader parity against the current block-max reader;
- PostgreSQL read-only function with the same TID/score/diagnostics contract.

Do not add mutable maintenance yet. The next proof should be read-only
correctness and traversal diagnostics.
