# Unified Sparse Block-Max Native Index Design

Date: 2026-05-12

## Purpose

This is the native-index design draft that follows the current SAE block-max
simulator results.

The target is a generic sparse-impact index, not an SAE-specific index:

```text
BM25 lexical impacts
SAE latent semantic impacts
future sparse sources
  -> one dimension namespace
  -> one block-max physical layout
  -> one exact top-k traversal
```

The database layer must remain model-agnostic. PostgreSQL receives sparse
query dimensions and weights. It does not run the embedding model or the SAE
encoder.

## Current Evidence

The strongest current result is the packed exact `micro_block_size=1` path
with overlap-greedy document layout and the doc-bound C fast path. It is exact
against brute-force sparse top-k over 500 total queries across five benchmark
corpora:

| Dataset | Layout | Block | Opened docs | Scored docs | Exact |
| --- | --- | ---: | ---: | ---: | ---: |
| `scifact` | `sae_overlap_greedy` | `1` | `0.050` | `0.050` | `100/100` |
| `scidocs` | `sae_overlap_greedy` | `1` | `0.050` | `0.050` | `100/100` |
| `nfcorpus` | `sae_overlap_greedy` | `1` | `0.048` | `0.048` | `100/100` |
| `arguana` | `sae_overlap_greedy` | `1` | `0.050` | `0.050` | `100/100` |
| `fiqa` | `sae_overlap_greedy` | `1` | `0.050` | `0.050` | `100/100` |

This is enough to justify the read-only native payload direction, but not
enough to call the access method production-ready. The opened-doc selectivity
target is now met for the read-only prototype; the remaining gap moves to
larger-scale posting-entry visitation, MVCC-visible continuation, and the
future mutable generation path.

The full design-exploration closure is recorded in:

```text
sae-native-index-design-exploration-summary.md
```

This file is the lower-level physical design. The summary file is the decision
record that explains which alternatives are accepted, rejected, or deferred.

## Non-Goals

- Do not replace the current canonical exact BM25 APIs in this phase.
- Do not run dense embedding or SAE inference inside PostgreSQL.
- Do not depend on per-query candidate windows from separate retrievers.
- Do not use per-query max normalization in the first pruned native scorer.
- Do not require realtime in-place updates to the block-max base generation.

## Score Contract

Safe pruning requires a score formula with an upper bound that can be computed
before opening a block.

The first native contract should be:

```text
raw_source_score(doc, source) =
    sum(query_weight(dim) * impact(dim, doc))

final_score(doc) =
    sum(source_weight(source) * raw_source_score(doc, source))
```

An optional fixed saturation contract is also boundable:

```text
final_score(doc) =
    bm25_weight * (1 - exp(-bm25_raw / bm25_tau))
  + sae_weight  * (1 - exp(-sae_raw  / sae_tau))
```

`bm25_tau` and `sae_tau` must be stored index-level or source-level constants,
not derived from the per-query candidate set.

Per-query max normalization is useful for offline evaluation, but it is not a
safe first native pruning contract because the source maxima are not known
until enough candidates have been scored. It can be revisited later as:

- an exact two-pass mode;
- a post-processing mode after candidate generation;
- an approximate retrieval mode with explicit documentation.

## Dimension Namespace

Every indexed signal is represented as a sparse dimension.

```text
source_id  source_name       dim_key
0          bm25              token id
1          field_bm25        field id + token id
2          sae               latent id
3+         future source     source-local key
```

Native storage should use compact numeric IDs:

```text
global_dim_id uint32
source_id     uint16
source_dim_id uint32
```

The dictionary maps `(source_id, source_dim_id)` to `global_dim_id` and stores
statistics needed by the scorer:

```text
doc_freq
idf
posting_count
block_entry_count
flags
```

## Generation Layout

The native payload should be an immutable generation, consistent with the
existing `ii42` generation model.

Conceptual sections:

```text
SparseImpactGenerationHeader
SourceDirectory
DimensionDirectory
DocumentTable
BlockDirectory
DimensionBlockEntries
ImpactPostings
OptionalSourceStats
```

### Generation Header

```c
typedef struct SparseImpactGenerationHeader
{
    uint32 magic;
    uint32 version;
    uint32 flags;
    uint32 active_generation;

    uint32 source_count;
    uint32 dimension_count;
    uint64 posting_count;

    uint32 doc_count;
    uint32 block_size;
    uint32 block_count;
    uint32 layout_kind;

    uint64 source_dir_offset;
    uint64 dimension_dir_offset;
    uint64 document_table_offset;
    uint64 block_dir_offset;
    uint64 block_entries_offset;
    uint64 postings_offset;
} SparseImpactGenerationHeader;
```

### Document Table

Document order is the physical layout. The current best result uses
overlap-greedy grouping over high-impact latent dimensions, so native storage
should preserve an explicit `doc_ord -> ctid` table.

```text
doc_ord uint32
ctid ItemPointerData
optional external document id offset
flags
```

The executor returns TIDs and performs normal PostgreSQL visibility rechecks.
If high-scoring rows are invisible under the snapshot, traversal continues
until the visible top-k threshold is safely bounded.

### Block Directory

```text
block_id uint32
first_doc_ord uint32
doc_count uint16
layout_signature optional uint64
```

Small blocks were consistently best in the simulator. For SAE-heavy indexes,
the current read-only default is `micro_block_size=1`, because one-document
micro-blocks make the block upper bound equal to the exact document score.
Larger super-blocks may still be useful as I/O grouping, but they should not
replace the exact pruning unit unless later evidence shows equivalent
opened-doc behavior.

### Dimension Block Entries

For each dimension, postings are grouped by block. Each dimension has a compact
list of block entries:

```text
global_dim_id uint32
block_id uint32
max_impact float4
posting_start uint64
posting_count uint32
```

This structure supports both phases of query execution:

1. compute block upper bounds from `max_impact`;
2. when a block is opened, fetch only the posting slices for query dimensions
   that occur in that block.

### Impact Postings

Postings are sorted by `(global_dim_id, block_id, doc_ord)`.

```text
doc_ord uint32
impact float4
```

`impact` is source-local:

- BM25 impact is the precomputed BM25 term impact for that document;
- SAE impact is normalized latent impact, for example
  `doc_weight / doc_norm * idf(latent)`.

Query-time weights multiply these impacts.

## Build Flow

The build process should have explicit source adapters.

```text
heap scan
  -> BM25 source adapter
  -> SAE source adapter
  -> sparse document rows
  -> layout builder
  -> dimension dictionary
  -> dim/block posting slices
  -> block max metadata
  -> immutable generation pages
  -> metapage generation swap
```

### Source Adapters

BM25 adapter:

- tokenize the configured text / multicolumn fields;
- compute BM25 corpus statistics;
- emit `(source_id=bm25, source_dim_id=token_id, doc_ord, impact)`.

SAE adapter:

- read sparse latent dimension and weight arrays from indexed columns or a
  precomputed side table;
- normalize document weights according to the selected score mode;
- compute latent `doc_freq` and `idf`;
- emit `(source_id=sae, source_dim_id=latent_id, doc_ord, impact)`.

The database should not know how the SAE vectors were produced. Model version,
embedding model, and SAE checkpoint identity should be metadata attached to
the index or source adapter, not executable inference logic.

### Layout Builder

The first native layouts should be:

```text
natural
sae_primary
sae_pair
sae_signature
sae_tree
```

`simhash` was not competitive in the current smoke test, so it can remain a
research-only layout for now.

Recommended first default:

```text
layout = sae_tree
block_size = 8
```

`sae_tree` recursively chooses a latent dimension that both:

- splits the current document set with acceptable balance;
- has high aggregate activation weight in the current partition.

Leaves fall back to top-latent signature order. This approximates the current
simulator behavior and is deterministic if ties use dimension id and TID.

For mixed BM25+SAE indexes, layout should be source-weighted. The first
implementation can use SAE-only layout for semantic indexes because the
simulator evidence is SAE-driven. A later version can include BM25 terms in
the layout objective with a lower weight.

## Query Flow

Inputs:

```text
index regclass
query dimensions: (source_id, source_dim_id, query_weight)[]
source weights
k
score contract
optional max_df / stop-dim policy
```

Execution:

```text
resolve query dims to global_dim_id
drop unknown or blocked dimensions
initialize touched block list
for each query dim:
    for each dimension block entry:
        block_upper[source] += query_weight * max_impact
        mark block as touched
convert source upper scores to final block upper score
sort or heap touched blocks by upper desc

for each block by descending upper:
    if visible_heap has k rows and block_upper <= kth_score:
        stop
    for each query dim:
        find posting slice for this block
        accumulate source raw score by doc_ord
    compute final score for docs in this block
    visibility-check candidate TIDs
    update visible top-k heap and threshold

return top-k rows with source score breakdowns
```

Important invariant:

```text
block_upper(block) >= final_score(doc)
for every visible or invisible doc in that block
```

This invariant is what makes pruning exact.

## MVCC and Visibility

The index stores physical TIDs. Query execution must recheck heap visibility
under the active snapshot.

If an opened block contains invisible high-scoring rows:

- those rows are ignored for the visible top-k heap;
- the threshold may remain lower;
- traversal continues until the next block upper is below the visible kth
  score.

This is the same correctness shape as other PostgreSQL index scans: the index
can overproduce candidates, but the heap snapshot decides visibility.

Delete cleanup should reuse the current VACUUM/tombstone direction. Dead rows
in the immutable generation remain harmless but can weaken pruning until the
next rebuild.

## Updates and Maintenance

The block-max base generation should be immutable. In-place updates are not a
good first target because they can affect:

- document order;
- block assignment;
- block maxima;
- corpus statistics;
- BM25 and SAE IDF.

Recommended first contract:

```text
base generation: immutable block-max payload
delta tail: exact append/update/delete overlay
maintenance: rebuild and atomically swap a new generation
```

Policy mapping:

- `manual`: mark stale; query base generation only.
- `eventual`: record exact delta when affordable; query may overlay bounded
  deltas; background worker rebuilds the block-max generation.
- `realtime`: use exact overlay or foreground rebuild only when necessary; do
  not weaken committed-read semantics.

For large RAG/AI corpora, `eventual` is the recommended policy. The current
timer-based catch-up worker and `auto_preload` model are a good operational
fit: rebuilds can publish a new resident immutable generation while old
queries keep using the previous one.

## Delta Overlay Strategy

The v1 delta path should be exact but conservative.

Base query:

```text
block-max traversal over immutable generation
```

Delta query:

```text
exact sparse scoring over pending changed rows
```

Merge:

```text
base top-k candidates
+ delta scored candidates
- delete tombstones
-> final visible top-k
```

If the delta is too large for the foreground budget, use the existing policy
rules:

- `eventual`: return the base generation and leave maintenance debt;
- `realtime`: wait, overlay, or trigger foreground consolidation;
- `manual`: report stale state.

Future work can add mini block-max delta segments, but v1 should avoid that
complexity.

## Coexistence With Current BM25

This design should not disturb the current exact BM25 engine first.

Recommended implementation shape:

1. Keep existing `ii42_query(...)` and field-aware BM25 functions on the
   current mature payload.
2. Add a new sparse-impact payload family and query function.
3. Prove BM25-only parity before routing canonical BM25 queries through the
   block-max path.
4. Add BM25+SAE unified query after BM25-only and SAE-only exactness are both
   boring.

Possible SQL shape:

```sql
SELECT *
FROM ii42_sparse_impact_query(
    index_name => 'docs_sparse_impact_idx'::regclass,
    sources => ARRAY['bm25', 'sae']::text[],
    dimensions => ARRAY[42, 4310]::int4[],
    weights => ARRAY[1.0, 0.37]::real[],
    source_weights => ARRAY[1.0, 1.0]::real[],
    k => 20
);
```

Convenience wrapper:

```sql
SELECT *
FROM ii42_unified_sparse_query(
    'docs_sparse_impact_idx'::regclass,
    bm25_query => 'cuda graph neural network optimization',
    sae_dimensions => ARRAY[4310, 917, 22]::int4[],
    sae_weights => ARRAY[0.42, 0.31, 0.18]::real[],
    bm25_weight => 1.0,
    sae_weight => 1.0,
    k => 20
);
```

Returned rows should expose:

```text
ctid
score
bm25_raw
sae_raw
matched_bm25_dims
matched_sae_dims
opened_blocks
scored_docs
```

The diagnostic fields are important during research and can later be hidden
behind an `explain => true` option.

## Shared Cache and Preload

The immutable sparse-impact generation should use the same shared-generation
cache direction as current large BM25 payloads.

Why:

- block directories and postings are large;
- many PostgreSQL backends will query the same hot RAG indexes;
- repeated per-backend decode would hide the benefit of block pruning.

`auto_preload > 0` should be supported once the payload can be decoded into a
resident generation. A rebuild worker should publish the new generation into
shared-preload when possible, then retire the previous generation after active
readers drain.

## Build-Time Reloptions

First draft reloptions:

```text
sparse_impact = true
sources = 'bm25,sae'
score_contract = 'raw' | 'sat'
layout = 'natural' | 'sae_primary' | 'sae_pair' | 'sae_signature' | 'sae_tree'
block_size = 8 | 16 | 32 | 64 | 128
sae_score_mode = 'normalized_idf_dot'
sae_dims_column = '<column name>'
sae_weights_column = '<column name>'
```

The exact SQL syntax needs more design because PostgreSQL index access methods
do not naturally accept arbitrary named source adapters. For the first native
prototype, a narrow expression-index shape is acceptable.

## Implementation Phases

### Phase 0: Lock the simulator contract

- Keep `research_sae_block_max_sim.py` as the reference.
- Add simulator checks for BM25+SAE boundable score contracts.
- Add tests that fail if any block-max run is not exact against brute force.

### Phase 1: SQL block-max sidecar

Before C storage, build a SQL sidecar with:

```text
dimension_dictionary
document_layout
dimension_block_entries
impact_postings
```

This validates the physical shape and query algorithm on real SQL joins.

Current implementation:

```text
scripts/research_sae_block_max_sql_sidecar.py
scripts/test_research_sae_block_max_sql_sidecar.py
sae-block-max-sql-sidecar-report.md
```

The SQL sidecar computes block upper bounds and exact prefix stopping. It is a
correctness bridge, not the final fast executor.

### Phase 2: C in-memory reader

Build a standalone C reader over an exported binary sparse-impact payload:

- load generation;
- resolve dimensions;
- run block-max traversal;
- compare against Python exact output.

No PostgreSQL AM integration yet.

Current implementation:

```text
scripts/research_sae_block_max_export_payload.py
tests/research_sae_block_max_reader.c
scripts/test_research_sae_block_max_c_reader.py
sae-block-max-c-reader-report.md
```

Phase 2.5 has replaced the first C reader's simple scans with dimension/block
range lookup. The implementation and benchmark are recorded in:

```text
scripts/research_sae_block_max_phase25_benchmark.py
sae-block-max-phase25-report.md
```

Phase 2.6 repeated the same path on prepared real benchmark artifacts and
added memory/lookup diagnostics:

```text
scripts/research_sae_block_max_phase26_real_benchmark.py
sae-block-max-phase26-real-benchmark-report.md
```

The Phase 3 scope is therefore read-only: port traversal into PostgreSQL first,
while keeping mutable maintenance and delta overlay out of scope.

### Phase 3: Read-only native payload

The first Phase 3 cut is implemented as a read-only PostgreSQL payload function:

```text
src/ii42_sae_blockmax.c
scripts/test_research_sae_block_max_pg_readonly.py
sae-block-max-phase3-pg-readonly-report.md
```

It accepts the Phase 2 binary payload as `bytea`, runs the same C traversal in
PostgreSQL, and returns SQL rows with:

```text
ctid
rank
score
doc_ord
document_id
opened_blocks
opened_docs
scored_docs
block/posting lookup diagnostics
```

The optional `doc_tids tid[]` argument maps `doc_ord` back to heap TID without
making the payload mutable. This keeps the first PostgreSQL integration useful
for SQL/RAG experiments while avoiding premature delta-overlay complexity.

Remaining Phase 3 work is to replace the debug bytea payload with a compact
read-only PostgreSQL-resident generation:

- build or load from heap/offline artifacts;
- query through the same SQL result contract;
- return TIDs and score diagnostics;
- no mutable delta overlay yet.

The compact generation cut is now implemented:

```text
scripts/research_sae_block_max_export_generation.py
scripts/test_research_sae_block_max_pg_generation.py
sae-block-max-phase31-compact-generation-layout-report.md
```

The `SBMXG001` generation removes embedded query rows, expected rows, and debug
document strings. Query dimensions and weights are passed at runtime through
`ii42_sae_block_max_query_generation(...)`.

The layout diagnostics also show that the next bottleneck is block granularity:

```text
block_size=1 -> about 5% opened docs on 2k-document / top-100 slices
block_size=2 -> about 18-25% opened docs
block_size=4 -> about 56-73% opened docs
```

This led to the packed micro-block work. Phase 4.3 later confirms that
one-document micro-blocks are practical after resident metadata compression and
the doc-bound fast path.

Phase 3.2 tested the postings-native alternatives:

```text
scripts/research_sae_wand_sim.py
scripts/research_sae_impact_ordered_sim.py
sae-block-max-phase32-wand-impact-report.md
```

The result is mixed but useful:

```text
document-level WAND: compact metadata, but scores about 39-47% of docs
impact-ordered threshold: exact, but touches about 91-95% of docs
block-size 2: still the best pruning target at about 18-25% opened docs
```

The initial packed design combined the winning parts:

```text
micro-block size 2 for bound pruning
+ compact postings/super-block storage for metadata efficiency
+ impact ordering only as an in-page scoring optimization
```

Phase 4.3 supersedes the `micro-block size 2` default for the read-only path:
`micro_block_size=1` now gives exact doc-level bounds with only a small payload
increase on the prepared slices.

Phase 3.3 validates the size side of that design:

```text
scripts/research_sae_packed_microblock_estimator.py
sae-block-max-phase33-packed-microblock-report.md
```

On the five 2k-document benchmark slices, the packed exact `micro_block_size=2`
estimate is about `17.3%` of the current block-entry-plus-posting generation:

```text
current block-max: about 46-47 bytes/posting
packed micro-block: about 8.0-8.2 bytes/posting
WAND postings: about 8.3 bytes/posting
```

This gives the next concrete implementation target: a read-only packed
micro-block reader with exact float32 impacts and the same SQL
TID/score/diagnostics contract.

Phase 3.4 implements that packed reader and PostgreSQL generation dispatch:

```text
scripts/research_sae_block_max_export_packed_microblock.py
tests/research_sae_packed_microblock_reader.c
scripts/test_research_sae_packed_microblock_reader.py
scripts/test_research_sae_packed_microblock_pg_generation.py
scripts/research_sae_packed_microblock_benchmark.py
sae-block-max-phase34-packed-reader-report.md
```

The packed `SBMXM001` generation uses:

```text
doc table: doc_ord, block_id, tie_ord
dimension directory: global_dim_id, entry range, posting start
micro-block entry: uint16 block_delta, uint8 doc_mask, uint8 posting_count
impact stream: float32 impact per posting
```

On the five prepared benchmark slices it remains exact (`500/500` queries)
and stores the generation at about `8.0-8.15 bytes/posting`. The same
`ii42_sae_block_max_query_generation(...)` function now accepts both the
older `SBMXG001` compact generation and the new `SBMXM001` packed generation,
so query dimensions and weights still arrive at runtime.

The first Phase 3.5 traversal improvement is implemented in both the
standalone packed reader and the PostgreSQL generation path. It builds a
per-query opened-block plan:

```text
while building block bounds:
    append matched entry reference to a per-block query plan
when opening a block:
    score the already collected entry references
```

This preserves exactness while reducing binary lookup steps to zero on the
standalone benchmark and the packed PostgreSQL smoke test.

Phase 3.5 also stops expanding packed generations into full posting rows:

```text
SBMXM001 resident decode:
    SaeBlockEntry[]
    packed_entry_masks[]
    packed_impacts[]
```

The implementation and diagnostic are recorded in:

```text
sae-block-max-phase35-pg-query-plan-report.md
```

Phase 3.6 compresses the resident packed block-entry state:

```text
SBMXM001 resident decode:
    SaePackedBlockEntry[]
    packed_impacts[]
```

The PostgreSQL generation path keeps exactness and still reports
`block_entry_binary_steps = 0`. The scifact diagnostic memory dropped from
about `4531111` bytes to `2519928` bytes for the first-query generation path.

Phase 3.8 adds the current best layout, `sae_overlap_greedy`. It groups
documents by overlapping high-impact latent dimensions, reducing upper-bound
slack while preserving exact scores. On the packed C benchmark this lowers
mean opened docs on every prepared dataset:

| Dataset | `sae_tree` opened docs | `sae_overlap_greedy` opened docs |
| --- | ---: | ---: |
| `scifact` | `504.34` | `437.74` |
| `scidocs` | `476.10` | `420.00` |
| `nfcorpus` | `447.83` | `365.13` |
| `arguana` | `379.36` | `305.42` |
| `fiqa` | `405.84` | `362.46` |

The implementation and diagnostics are recorded in:

```text
sae-block-max-phase36-38-report.md
```

Phase 3.9 adds the first PostgreSQL-resident generation table path. Instead of
passing the packed generation as a query `bytea`, the caller stores the
generation once and queries it by id:

```text
sae_generations(generation_id text primary key, generation bytea, metadata jsonb)
ii42_sae_block_max_query_generation_by_id(regclass, text, dims, weights)
```

The packed resident decode also moves closer to the on-disk `SBMXM001` shape:

```text
SaePackedEntry[]           -- uint16 block_delta, uint8 doc_mask, uint8 count
packed_entry_max_impacts[] -- float4
packed_impacts[]           -- float4
```

The per-block query plan captures the exact posting slice metadata during
bound construction, so scoring opened blocks still uses
`block_entry_binary_steps = 0`.

The five-dataset resident benchmark remains exact (`500/500`) with mean
PostgreSQL by-id query time around `2.2-2.6 ms` on the local scalar build:

| Dataset | Exact | PG mean query ms | Memory bytes | Opened docs |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | `100/100` | `2.624` | `1523340` | `437.74` |
| `scidocs` | `100/100` | `2.197` | `1542860` | `420.00` |
| `nfcorpus` | `100/100` | `2.249` | `1533148` | `365.13` |
| `arguana` | `100/100` | `2.245` | `1472348` | `305.42` |
| `fiqa` | `100/100` | `2.199` | `1552708` | `362.46` |

The implementation and diagnostics are recorded in:

```text
scripts/test_research_sae_resident_generation_pg.py
scripts/research_sae_resident_generation_benchmark.py
sae-block-max-phase39-resident-generation-report.md
```

### Phase 4: Read-only resident index surface

Phase 4 deliberately stays read-only. It turns the Phase 3.9 resident
generation prototype into a small index-shaped API surface:

- `ii42_sae_generation_create_table(schema, table)`;
- `ii42_sae_generation_upsert(regclass, generation_id, bytea, metadata)`;
- `ii42_sae_generation_list(regclass)`;
- `ii42_sae_generation_delete(regclass, generation_id)`;
- `ii42_sae_readonly_index_query(regclass, index_id, dims, weights)`.

The facade still uses the same exact C traversal as the by-id generation
query, but the caller no longer treats the payload as a per-query value. The
runtime shape is now:

```text
resident generation table
  -> index_id / generation_id
  -> read-only block-max traversal
  -> TID + score + diagnostics
```

Phase 4.1 adds a PostgreSQL-backed selectivity sweep that fixes the read-only
facade path and changes only layout parameters. Among multi-document
micro-blocks, `sae_overlap_greedy` with `micro_block_size = 2` was the best
default on all five prepared datasets.

Phase 4.3 then enables the doc-bound path. With `micro_block_size = 1`, each
block contains one document, so the block upper bound is the exact document
score. The C traversal can rank directly from sorted bounds and skip the
second postings-scoring pass:

| Dataset | Best layout | Block | Opened ratio | Exact |
| --- | --- | ---: | ---: | ---: |
| `scifact` | `sae_overlap_greedy` | `1` | `0.050` | `100/100` |
| `scidocs` | `sae_overlap_greedy` | `1` | `0.050` | `100/100` |
| `nfcorpus` | `sae_overlap_greedy` | `1` | `0.048` | `100/100` |
| `arguana` | `sae_overlap_greedy` | `1` | `0.050` | `100/100` |
| `fiqa` | `sae_overlap_greedy` | `1` | `0.050` | `100/100` |

The implementation and diagnostics are recorded in:

```text
scripts/research_sae_bound_selectivity_sweep.py
sae-block-max-phase40-readonly-index-surface-report.md
```

Phase 4.4 validates the next bottleneck with deterministic 1x/2x/4x scaled
resident profiles. The result is exact across all tested configurations and
`opened_docs` remains near `top_k`, but `block_entry_visits` scales almost
linearly with corpus size:

| Dataset | Docs at 4x | Exact | Mean query ms | Visits/term | Opened docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `8000` | `100/100` | `7.001` | `485.12` | `100.00` |
| `scidocs` | `8000` | `100/100` | `6.918` | `457.45` | `100.00` |
| `nfcorpus` | `8252` | `100/100` | `6.806` | `424.14` | `100.00` |
| `arguana` | `8000` | `100/100` | `7.040` | `482.48` | `100.00` |
| `fiqa` | `8000` | `100/100` | `6.770` | `413.80` | `100.00` |

The read-only facade also now simulates MVCC continuation by treating NULL
`doc_tids` entries as invisible documents. Top-k filling skips invisible docs
and keeps traversing until the visible threshold is safe.

The next Phase 4 step should add a read-only dimension-local skip directory
over doc-bound entries:

```text
dimension range
  -> high-DF super-block directory with max impact bounds
     -> doc-bound entries
        -> exact one-doc contribution
```

Phase 4.5 implements and measures this document-range super-block directory as
an opt-in `SBMXM001` v2 payload. The result is exact but not a viable default:
the range-level bound is too loose, opens most documents, and is slower than
the Phase 4.3 doc-bound fast path. The stable default therefore remains the v1
doc-bound generation with no super-block directory.

Five-dataset mean:

| Path | Mean query ms | Super-entry visits | Block-entry visits | Opened docs |
| --- | ---: | ---: | ---: | ---: |
| doc-bound default | `1.616` | `0.0` | `7236.5` | `100.0` |
| super size 4 | `6.152` | `5958.5` | `5834.4` | `1306.4` |
| super size 64 | `3.910` | `1776.4` | `7236.5` | `1877.0` |

Phase 4.6 tests exact impact-ordered threshold skipping rather than another
document-range size sweep:

```text
dimension range
  -> impact-ordered side directory or postings order
  -> high-impact doc-bound entries first
  -> residual tail upper bounds
  -> exact visible top-k stop condition
```

The result is also negative as a direct native-reader target. It is exact, but
the residual frontier remains too loose with broad 64-dimension queries:

| Query dims | Mean visit ratio | Mean unique docs |
| ---: | ---: | ---: |
| 16 | `0.960` | `1045.1` |
| 32 | `0.967` | `1517.0` |
| 48 | `0.972` | `1745.1` |
| 64 | `0.975` | `1863.8` |

Do not start delta overlay or mutable maintenance yet, and do not port this
exact impact-ordered threshold algorithm to C as-is. The next useful step is a
representation-aware candidate-budget design: train or encode queries so they
activate fewer, sharper high-value latent dimensions, then use approximate
high-impact candidate generation plus exact doc-bound rerank.

Implementation and diagnostics:

```text
scripts/research_sae_large_scale_entry_profile.py
sae-block-max-phase44-entry-visitation-report.md
sae-block-max-phase45-superblock-skip-report.md
sae-block-max-phase46-impact-ordered-skip-report.md
```

Phase 4.7 implements that approximate candidate-budget experiment in simulator
form. The path selects a small subset of query dimensions, reads only the
impact-ordered posting heads for those dimensions, unions candidate documents,
then reranks candidates with the full original SAE query dimensions.

This is intentionally not an exact top-k parity path. It is accepted only if
qrels quality stays close while the candidate set shrinks enough to justify a
native candidate-generator stage.

Five-dataset general-purpose candidates:

| Config | Mean R@100 delta | Mean MRR@20 delta | Candidates | Exact@100 overlap |
| --- | ---: | ---: | ---: | ---: |
| `top_weight d=16 pdim=32` | `-0.0050` | `+0.0008` | `377.8` | `0.7727` |
| `incremental d=16 pdim=32 p=0.25` | `-0.0063` | `+0.0005` | `335.3` | `0.7575` |
| `top_weight d=12 pdim=32` | `-0.0099` | `+0.0010` | `292.8` | `0.6873` |
| `incremental d=16 pdim=24 p=1.00` | `-0.0132` | `+0.0005` | `244.6` | `0.6595` |

The default exact read-only path remains the correctness reference. The next
prototype should add a separate approximate mode:

```text
candidate generator
  -> dimension-local impact-ordered posting heads
  -> compact candidate accumulator
  -> budget or score-concentration stop

exact candidate rerank
  -> full query dimensions
  -> doc-bound exact score lookup
  -> visible top-k
```

Implementation and diagnostics:

```text
scripts/research_sae_candidate_budget_rerank.py
sae-block-max-phase47-candidate-budget-rerank-report.md
```

Phase 4.8 ports the Phase 4.7 candidate-generator shape into the standalone C
packed micro-block reader. The exact reader remains the default. The
approximate path is enabled explicitly:

```text
packed_payload.bin
  -> selected top-weight query dimensions
  -> top impact postings per selected dimension
  -> candidate doc union
  -> full-query exact rerank over candidates
```

The C prototype reproduces the Phase 4.7 candidate counts and full-SAE overlap,
but it also shows that the existing payload is not enough for a fast
approximate mode. Repeat-3 five-dataset means:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Rerank binary steps |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound | `0.049164` | `100 opened` | `1.0000` | `7236.47` | `0.00` |
| `d16_pdim32` | `0.091059` | `377.93` | `0.7727` | `1845.36` | `159744.81` |
| `d12_pdim32` | `0.073004` | `292.96` | `0.6874` | `1383.06` | `123821.21` |
| `d16_pdim24` | `0.072920` | `296.02` | `0.6915` | `1845.36` | `125092.10` |

The next Phase 4 step should add row-wise compact document vectors to the
payload so candidate rerank can scan a candidate document's active SAE
dimensions directly instead of doing `candidate_count * query_dim_count`
dimension-range binary lookups. Impact-head posting slices should follow so
candidate generation can avoid scanning each selected dimension's full
block-id ordered range.

Implementation and diagnostics:

```text
tests/research_sae_packed_microblock_reader.c
scripts/research_sae_native_candidate_rerank_benchmark.py
sae-block-max-phase48-native-candidate-rerank-report.md
```

Phase 4.9 adds the row-wise compact document-vector section proposed by Phase
4.8:

```text
doc_ord -> sorted (global_dim_id, impact) pairs
```

Packed micro-block payloads now support `SBMXM001` v3. The exact path remains
compatible with v1, while the approximate candidate-rerank path uses v3 doc
vectors when present. This removes the `candidate_count * query_dim_count`
dimension-range binary lookup from rerank.

Repeat-3 five-dataset means:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Rerank binary steps |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound v3 | `0.055006` | `100 opened` | `1.0000` | `7236.47` | `0.00` |
| `d16_pdim32` | `0.056798` | `377.93` | `0.7727` | `1845.36` | `0.00` |
| `d12_pdim32` | `0.050509` | `292.96` | `0.6874` | `1383.06` | `0.00` |
| `d16_pdim24` | `0.051059` | `296.02` | `0.6915` | `1845.36` | `0.00` |

This is a successful payload step. `d16_pdim32` improves from about `0.091s`
in Phase 4.8 to about `0.057s`, and the measured rerank binary lookup cost
drops to zero. The cost is storage: v3 duplicates the impact stream in
document-major order. On the scifact slice, generation bytes increase from
about `1.08 MB` to about `2.10 MB`.

The next Phase 4 step should add an impact-head directory:

```text
global_dim_id -> top impact doc_ord/impact pairs
```

Candidate generation should then read only the requested posting head instead
of scanning the selected dimension's block-id ordered range.

Implementation and diagnostics:

```text
scripts/research_sae_block_max_export_packed_microblock.py
tests/research_sae_packed_microblock_reader.c
scripts/research_sae_native_candidate_rerank_benchmark.py
sae-block-max-phase49-doc-vector-rerank-report.md
```

Phase 4.10 adds the impact-head directory proposed by Phase 4.9:

```text
global_dim_id -> top impact doc_ord/impact pairs
```

Packed micro-block payloads now support `SBMXM001` v4. Version 4 keeps the
row-wise document vectors and adds a dense `impact_head_starts` directory plus
impact-sorted `(doc_ord, impact)` pairs. The approximate candidate generator
uses the head when `postings_per_dim > 0`; otherwise it can still fall back to
the full block-id ordered dimension range.

Repeat-3 five-dataset means with `impact_head_size = 32`:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Generator decoded |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound v4 | `0.057153` | `100 opened` | `1.0000` | `7236.47` | `1085.19` |
| `d16_pdim32` | `0.052813` | `377.93` | `0.7727` | `16.00` | `497.84` |
| `d12_pdim32` | `0.047180` | `292.96` | `0.6874` | `12.00` | `373.52` |
| `d16_pdim24` | `0.058429` | `296.02` | `0.6915` | `16.00` | `378.05` |

This completes the first physically plausible approximate native path: v4
avoids both Phase 4.8 rerank binary lookups and Phase 4.9 dimension-range
candidate-generation scans. The next step should be a quality/cost sweep over
the v4 native path using qrels metrics, then a storage decision for doc-vector
and impact-head duplication.

Implementation and diagnostics:

```text
scripts/research_sae_block_max_export_packed_microblock.py
tests/research_sae_packed_microblock_reader.c
scripts/research_sae_native_candidate_rerank_benchmark.py
sae-block-max-phase410-impact-head-rerank-report.md
```

Phase 4.11 connects that v4 physical path back to qrels quality. The new
combined sweep keeps the Python candidate-budget evaluator as the quality
oracle and uses the standalone C reader for native timing/counters:

```text
scripts/research_sae_v4_quality_cost_sweep.py
```

Five-dataset repeat-3 means:

| Head | Config | dR@100 | dMRR@20 | C seconds | Candidates | Generation MB |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 32 | `d16_pdim32` | `-0.0050` | `+0.0008` | `0.053887` | `377.9` | `2.47` |
| 32 | `d12_pdim32` | `-0.0099` | `+0.0010` | `0.047881` | `293.0` | `2.47` |
| 32 | `d16_pdim24` | `-0.0133` | `+0.0010` | `0.048395` | `296.0` | `2.47` |
| 16 | `d16_pdim16` | `-0.0307` | `-0.0014` | `0.042484` | `206.9` | `2.31` |

The next read-only PostgreSQL target is:

```text
impact_head_size = 32
active_dims = 12
postings_per_dim = 32
```

This row is the current best efficiency/quality tradeoff: average R@100 falls
by about one point versus full exact SAE, while MRR@20 is slightly higher and
candidate docs fall from about `378` to about `293` compared with the
conservative `d16_pdim32` row. Keep `d16_pdim32` as the fallback if later
domain workloads need the extra recall margin.

Implementation and diagnostics:

```text
scripts/research_sae_v4_quality_cost_sweep.py
sae-block-max-phase411-v4-quality-cost-sweep-report.md
```

Phase 4.12-4.16 ports the selected v4 path into PostgreSQL and closes the
read-only exploration stage. PostgreSQL now accepts `SBMXM001` v3/v4 payloads,
including row-wise document vectors and dimension-local impact heads. The
candidate-rerank API is exposed as:

```text
ii42_sae_candidate_rerank_generation(...)
ii42_sae_candidate_rerank_generation_by_id(...)
ii42_sae_readonly_index_candidate_query(...)
```

The selected defaults are encoded in SQL:

```text
active_dims = 12
postings_per_dim = 32
```

Five-dataset PostgreSQL by-id benchmark:

| Metric | Mean |
| --- | ---: |
| PG ms/query | `1.688` |
| Exact@100 overlap | `0.68736` |
| Candidate docs | `292.96` |
| Generator visits | `12.00` |
| Generator decoded postings | `373.52` |
| Rerank doc-vector terms | `35136.11` |
| Resident memory | `3.08 MB` |

This completes Phase 4 as a read-only prototype. The storage duplication in v4
is acceptable for the next stage, but should be compressed before production
mutable maintenance: row-wise doc vectors are the main extra resident-memory
cost, while impact heads are a small hot candidate directory.

Implementation and diagnostics:

```text
src/ii42_sae_blockmax.c
scripts/research_sae_v4_pg_candidate_benchmark.py
scripts/test_research_sae_packed_microblock_pg_generation.py
sae-block-max-phase412-416-pg-v4-candidate-report.md
```

Phase 4.17 adds the missing BM25+dense comparison without rerunning the
existing BM25 and BM25+SAE rows. The script appends BM25+dense to the previous
matrix, producing a three-way view:

```text
scripts/research_sae_append_bm25_dense_matrix.py
sae-bm25-dense-sae-current-matrix-report.md
results/sae/phase417/bm25-dense-sae-current/summary.md
```

Five-dataset mean:

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean query ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | `0.6025` | `0.7035` | `0.5904` | `0.5031` | `0.4134` | `1.9422` |
| `BM25+dense` | `0.6947` | `0.7817` | `0.6860` | `0.5984` | `0.5010` | `2.4125` |
| `BM25+SAE` | `0.7045` | `0.7947` | `0.6835` | `0.6036` | `0.5052` | `5.4446` |

This result confirms both sides of the design tension. SAE is a real sparse
semantic signal and slightly beats BM25+dense on mean recall/NDCG/MAP, but the
full Python SAE scorer is too expensive to be the production path. The v4
impact-head candidate design remains the correct route for Phase 5.

Phase 5.1 closes the pre-SQL unified questions:

```text
scripts/research_sae_pre_sql_unified_exploration.py
sae-pre-sql-unified-exploration-report.md
results/sae/phase5/pre-sql-unified-exploration/summary.md
```

The important update is that fixed-saturation BM25+SAE remains the right
native score contract, but simple mixed-source block layouts are not selective
enough. The corrected five-dataset probe reaches about `0.7934..0.7937`
Recall@100 in the `sae_weight=2.0..2.5` range, while the tested layouts still
open about `98%` of documents.

Therefore the next implementation stage should not be a naive SQL port of the
current block layout. It should focus on source-aware impact-ordered traversal,
tighter upper bounds, and candidate-budget profiles such as `q32_d64` or
`q64_d48`.

A post-milestone branch point now explores a more radical evidence-atom model:

```text
bm25-sae-deep-fusion-exploration-report.md
```

That line asks whether token atoms, SAE latent atoms, and token-latent mixed
atoms can share one source-blind scorer. If it succeeds, the native payload
should become a generic evidence-atom index rather than a BM25+SAE-specific
source-fusion index.

### Phase 5: Mutable policy integration

Only after the read-only surface, opened-doc selectivity, and
candidate-budget-aware query representation are stable, add:

- resident generation cache ownership;
- doc-vector compression;
- larger resident payload validation;
- delta records and delete tombstones;
- background rebuild and shared-preload publish.

### Phase 6: Unified BM25+SAE production research

Add source adapters and source-aware score contracts:

- BM25-only parity with current BM25 engine;
- SAE-only parity with simulator;
- BM25+SAE exactness;
- performance benchmark against current BM25, hybrid SQL, and vector search.

## Correctness Tests

Required before any production claim:

- exact top-k equality against brute force for random sparse corpora;
- exact top-k equality for all current five benchmark SAE exports;
- BM25-only score parity against current canonical BM25 APIs;
- mixed BM25+SAE exactness under raw score contract;
- MVCC visibility with invisible high-score rows;
- delete tombstone and VACUUM behavior;
- delta overlay merge correctness;
- interrupted rebuild leaves old generation usable;
- shared-preload generation swap does not expose partial data.

## Open Risks

- SAE query generation still requires an external embedding and SAE pipeline.
- `micro_block_size=1` solves opened-doc selectivity for the read-only
  prototype, but it increases block-entry metadata and may shift the next
  bottleneck to high-frequency dimension entry visitation at larger scale.
- Per-query max normalization is not a safe pruning contract.
- BM25 and SAE may prefer different physical layouts.
- Rebuilding large semantic indexes may be expensive enough that `eventual`
  consistency is the only practical production policy.

## Current Recommendation

Proceed with a native prototype only as a staged research branch:

```text
SQL sidecar block-max
  -> C in-memory block-max reader
  -> read-only packed sparse-impact generation
  -> PostgreSQL-resident generation table
  -> read-only resident index facade
  -> mutable/eventual integration
```

Do not attempt to merge this into the current BM25 access method hot path
until BM25-only parity and SAE block-max exactness are both proven in C.
