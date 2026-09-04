# SAE Block-Max Phase 3.1 Compact Generation and Layout Report

## Scope

This phase moves beyond the debug `SBMXP001` bytea payload:

- compact read-only generation format: `SBMXG001`;
- runtime query dimensions and weights passed from SQL;
- generation bytes stored in PostgreSQL and read by a SQL function;
- no expected rankings, query rows, or debug document strings in the generation;
- no delta overlay or mutable maintenance.

It also adds block-bound selectivity diagnostics to the simulator so the
representation/layout problem can be measured directly.

## Compact Generation

The compact generation stores only the read-only index arrays needed by the C
traversal:

```text
header
doc_ord -> block_id
doc_ord -> tie_ord
block_id -> first_doc_ord, doc_count
dimension block entries
impact postings
```

`tie_ord` is the compact replacement for debug document strings. It preserves
the same deterministic score tie-break as document-id ordering without storing
document ids in the generation.

Query-time data is no longer embedded. The SQL function receives:

```sql
generation bytea,
query_dims int4[],
query_weights real[],
k int4,
doc_tids tid[]
```

The result contract is intentionally narrow:

```text
rank
ctid
doc_ord
score
opened_blocks
opened_docs
scored_docs
block/posting diagnostics
memory_bytes
```

This is still a read-only prototype. The generation is stored in PostgreSQL as
`bytea` in the smoke test, then selected into the C function. The next native
step can replace that table row with a proper resident generation/cache, but
the query contract and traversal are already separated from debug payload data.

## Implementation

New files:

```text
scripts/research_sae_block_max_export_generation.py
scripts/test_research_sae_block_max_pg_generation.py
```

Updated files:

```text
src/ii42_sae_blockmax.c
sql/ii42--0.4.7.sql
sql/ii42--0.4.6--0.4.7.sql
scripts/research_sae_block_max_sim.py
```

New SQL function:

```sql
ii42_sae_block_max_query_generation(
    generation bytea,
    query_dims int4[],
    query_weights real[],
    k int4 DEFAULT 100,
    doc_tids tid[] DEFAULT NULL
)
```

## Verification

Build:

```bash
make -s \
  PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
```

Read-only debug payload smoke:

```bash
python3 scripts/test_research_sae_block_max_pg_readonly.py --temp-postgres
```

Compact resident-generation smoke:

```bash
python3 scripts/test_research_sae_block_max_pg_generation.py --temp-postgres
```

Standalone C reader regression:

```bash
python3 scripts/test_research_sae_block_max_c_reader.py
```

All three passed after this phase.

## Layout Diagnostics

The simulator now records:

- `mean_opened_block_fraction`;
- `mean_positive_bound_slack`;
- `mean_opened_bound_slack`.

`bound_slack = safe_block_upper / actual_best_doc_score_in_block`.

Large slack means the per-dimension block maxima are coming from different
documents and the summed safe upper bound overestimates any real document in
the block. That directly explains why traversal keeps opening blocks.

Command:

```bash
python3 scripts/research_sae_block_max_sim.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/sae_phase3_layout_diag \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --block-sizes 4,8,16,32,64 \
  --layouts sae_tree,sae_signature,simhash,sae_pair,natural \
  --score-mode normalized_idf_dot \
  --latent-dims 8192 \
  --active-dims 64
```

Best exact run by opened-doc fraction:

| Dataset | Layout | Block | Opened docs | Scored docs | Bound slack |
| --- | --- | ---: | ---: | ---: | ---: |
| `scifact` | `sae_tree` | 4 | 0.729 | 0.710 | 2.07 |
| `scidocs` | `sae_tree` | 4 | 0.734 | 0.710 | 2.06 |
| `nfcorpus` | `natural` | 4 | 0.623 | 0.603 | 1.96 |
| `arguana` | `simhash` | 4 | 0.559 | 0.546 | 1.93 |
| `fiqa` | `sae_tree` | 4 | 0.624 | 0.596 | 1.97 |

`block_size >= 8` mostly degenerates toward opening nearly the full corpus on
these 2k-document slices.

## Small-Block Sweep

Command:

```bash
python3 scripts/research_sae_block_max_sim.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/sae_phase3_layout_diag_small_blocks \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --block-sizes 1,2,4 \
  --layouts sae_tree,simhash,natural \
  --score-mode normalized_idf_dot \
  --latent-dims 8192 \
  --active-dims 64
```

Best exact run by block size:

| Dataset | Block 1 opened | Block 2 opened | Block 4 opened |
| --- | ---: | ---: | ---: |
| `scifact` | 0.050 | 0.252 | 0.729 |
| `scidocs` | 0.050 | 0.238 | 0.734 |
| `nfcorpus` | 0.049 | 0.207 | 0.623 |
| `arguana` | 0.050 | 0.180 | 0.559 |
| `fiqa` | 0.050 | 0.203 | 0.624 |

This is the main finding of the phase:

```text
lookup is not the bottleneck; block granularity and bound looseness are.
```

`block_size=1` is effectively exact document-level WAND over the sparse impacts
and opens about `top_k / docs`. It is the quality/performance lower bound but
has high metadata cost. `block_size=2` is a more realistic next candidate:
it opens roughly `18-25%` of these slices while keeping exact top-k parity.

## Next Direction

The next useful research step is not another SQL wrapper. It is a compact
physical representation that can afford smaller logical blocks:

- micro-blocks of size `1-2`;
- compressed block-entry arrays;
- impact-ordered postings or MaxScore/WAND-style dimension traversal;
- optional larger super-blocks only as I/O grouping, not as the pruning unit;
- layout optimized for bound tightness, not only latent signature locality.

The current C/PG path is ready to host that experiment because query execution
now consumes a compact generation plus runtime query dimensions.
