# SAE Native Index Design Exploration Summary

Date: 2026-05-12

## Purpose

This document closes the current SAE/native sparse index design exploration.
It consolidates the earlier research reports into one implementation-facing
decision record.

The design target is not "SAE as a separate retriever". The target is:

```text
BM25 lexical impacts
+ SAE latent semantic impacts
+ future weighted sparse sources
  -> one sparse-impact namespace
  -> one block-max native payload
  -> one exact top-k traversal
```

The PostgreSQL extension remains model-agnostic. Embedding inference, SAE
encoding, and any future learned sparse encoder run outside PostgreSQL. The
index stores and searches weighted sparse impacts.

## Final Current Position

The current evidence supports continuing a native sparse-impact prototype, but
not replacing the production BM25 or vector paths yet.

| Question | Current answer |
| --- | --- |
| Is SAE useful as a retrieval signal? | Yes. It adds semantic sparse evidence beyond BM25. |
| Can SAE replace dense everywhere today? | Not proven. Some datasets still need dense-like recall. |
| Is BM25+SAE better than BM25 alone? | Yes on the current real-qrels matrix. |
| Is BM25+SAE already better than BM25+dense? | Mixed. It slightly improves mean recall/NDCG/MAP, but BM25+dense is cheaper and slightly better on mean MRR@20 in the current Python matrix. |
| Is a single sparse-impact index plausible? | Yes, if the scorer is boundable and the layout prunes enough blocks. |
| Is late fusion the final architecture? | No. It is useful for validation, but the native path should score one unified sparse space. |
| Is per-query max normalization safe for pruning? | No for v1. It is an offline or two-pass feature, not a first native exact contract. |
| Should existing BM25 APIs be rewritten immediately? | No. Build a new payload family first and prove BM25-only parity. |

## Current Development State

The implemented branch state now has two validated layers:

1. A read-only PostgreSQL resident v4 candidate path:
   `impact_head_size = 32`, `active_dims = 12`,
   `postings_per_dim = 32`.
2. A current three-way research matrix comparing BM25, BM25+dense, and
   BM25+SAE over the five sampled BEIR-style datasets.

The PostgreSQL v4 candidate path is the current systems prototype. It is not a
mutable index yet. It proves that resident payload lookup, impact-head
candidate generation, and doc-vector rerank can run inside PostgreSQL with
stable diagnostics.

The three-way quality matrix is the current product-signal check:

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean query ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | `0.6025` | `0.7035` | `0.5904` | `0.5031` | `0.4134` | `1.9422` |
| `BM25+dense` | `0.6947` | `0.7817` | `0.6860` | `0.5984` | `0.5010` | `2.4125` |
| `BM25+SAE` | `0.7045` | `0.7947` | `0.6835` | `0.6036` | `0.5052` | `5.4446` |

This changes the interpretation. SAE is not merely better than BM25; it is
close to BM25+dense in this matrix and slightly stronger on recall/NDCG/MAP.
But the current full Python SAE scorer is much more expensive. Therefore the
native v4 candidate path is not optional: it is the mechanism that can make
the quality signal systems-relevant.

Phase 5 gap exploration is now recorded in:

```text
sae-phase5-gap-exploration-plan.md
sae-phase5-gap-exploration-report.md
scripts/research_sae_phase5_gap_exploration.py
```

The most important new finding is that fixed source-level saturation nearly
matches per-query normalization while remaining compatible with exact native
upper bounds:

| Contract | Recall@100 | MRR@20 | NDCG@10 |
| --- | ---: | ---: | ---: |
| `normalized` | `0.7947` | `0.6835` | `0.6036` |
| `raw` | `0.7470` | `0.6315` | `0.5457` |
| `fixed_saturation` | `0.7934` | `0.6742` | `0.5952` |

The same pass found that tri-hybrid BM25+dense+SAE still improves the sampled
matrix (`Recall@100 = 0.8019`, `MRR@20 = 0.6927`), so SAE should be treated as
a semantic sparse signal with replacement potential, not as a proven universal
dense-removal path yet.

SPLADE v2-distil has also been tested before SQL integration:

```text
sae-splade-baseline-report.md
scripts/research_splade_baseline_matrix.py
```

The result keeps SPLADE as a reference learned-sparse source rather than the
next native implementation target. It underperforms BM25+SAE on mean
Recall@100 and MRR@20, while BM25+SAE+SPLADE only improves NDCG/MAP slightly.
This does not change the native path: first implement fixed-saturation
BM25+SAE with budgeted sparse candidate generation.

## Evidence Chain

The exploration produced these durable results:

- Literature and project review showed that learned sparse retrieval, SPLADE,
  neural sparse search, and WAND/MaxScore systems are the closest references.
- The first SAE-over-dense prototype established the correct boundary:
  dense embedding first, SAE encoder second, sparse index third.
- Snowflake `8192/32` and later `8192/64` showed that SAE capacity matters.
- Unified BM25+SAE sparse scoring improved BM25 and in some SciDocs profiles
  matched or exceeded dense vector metrics.
- Query top-k reduction, naive SAE DF stoplists, and active-128 brute-force
  expansion were not enough as system-level solutions.
- Candidate-budget gating improved cost/quality tradeoffs without becoming
  a replacement for a physical pruning engine.
- Learned SAE tree document layout plus block-max metadata produced the best
  exact physical simulator result so far.

The strongest physical simulator point is recorded in
`unified-sparse-block-max-native-index-design.md`:

| Path | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact |
| --- | ---: | ---: | ---: | ---: | ---: |
| baseline, learned layout | `0.9260` | `0.7737` | `0.471` | `0.403` | `500/500` |
| retention `50`, learned layout | `0.9234` | `0.7676` | `0.435` | `0.358` | `500/500` |

This is good enough to design the native payload. It is not yet good enough
to claim an extreme-performance replacement for dense vector search.

## Native Index Decision Record

### D1: Generic Sparse-Impact Payload

The native payload must not be SAE-specific. It should store:

```text
source_id
source_dim_id
global_dim_id
doc_ord
impact
```

SAE is one source. BM25 and field-aware BM25 are other sources. Future SPLADE,
BGE-M3 sparse weights, learned memory tags, or agent-memory signals should
fit the same representation.

### D2: Immutable Base Generation

The first production-shaped payload should use an immutable generation:

```text
metapage -> active generation
generation -> headers, dictionaries, doc table, block metadata, postings
```

This matches the current `ii42` generation and preload model, keeps
readers simple, and lets background maintenance publish a new resident
generation atomically.

### D3: Exact Delta Overlay, Not In-Place Block Updates

In-place updates are the wrong first target because one row update may affect:

- BM25 corpus statistics;
- SAE latent document frequency;
- document layout;
- block maxima;
- posting slices.

The first mutable shape should be:

```text
immutable base generation
+ exact changed-row delta overlay
+ delete tombstones
-> background rebuild and generation swap
```

Large AI/RAG indexes should default to eventual consistency.

### D4: Boundable Score Contract

Native exact pruning requires a block upper bound that is never lower than any
document score in that block.

The v1 exact contracts are:

```text
raw weighted impact sum
fixed source-level saturation
```

The v1 exact contracts are not:

```text
per-query max normalization
candidate-window normalization
post-hoc fusion normalization
```

Those can still exist as evaluation, approximate, or two-pass modes, but not
as the first exact block-max pruning contract.

### D5: Document Order Is a First-Class Index Asset

The simulator shows that physical document order changes pruning efficiency.
The native payload should store a logical `doc_ord -> ctid` table instead of
assuming heap order is enough.

First layout set:

```text
natural
sae_primary
sae_pair
sae_signature
sae_tree
```

Default first native research layout:

```text
layout = sae_tree
block_size = 8
```

### D6: Block Metadata Must Be Source-Aware Enough for Pruning

Each query dimension needs compact block entries:

```text
global_dim_id
block_id
max_impact
posting_start
posting_count
```

Query execution uses these entries to compute block upper bounds before
opening postings. Source-specific raw scores are accumulated after a block is
opened.

### D7: Query Traversal Is Exact Block-Max Top-K

The v1 query algorithm is:

```text
resolve sparse query dims
compute touched block upper bounds
open blocks in descending upper-bound order
score postings inside opened blocks
heap-visibility recheck TIDs
stop only when next block upper <= visible kth score
```

This is exact as long as the score contract is boundable and visibility
rechecks continue after invisible high-score rows.

### D8: Existing BM25 Stays Canonical During Bring-Up

The native sparse-impact payload should be introduced beside the existing
BM25 payload.

Bring-up order:

1. SAE-only exact parity against simulator.
2. BM25-only exact parity against current BM25 APIs.
3. BM25+SAE exact parity against unified sparse scorer.
4. Only then consider routing existing BM25 helper APIs through the new path.

### D9: Shared Preload Is Part of the Design

The payload is large and query-hot. Once native decode exists, it should reuse
the existing shared generation cache and `auto_preload` direction.

The preload target is the active immutable generation, not the external SAE
model or embedding model.

### D10: Diagnostics Stay Visible During Research

The native function should initially return diagnostics:

```text
score
bm25_raw
sae_raw
opened_blocks
scored_docs
decoded_postings
matched_bm25_dims
matched_sae_dims
```

These fields are required to decide whether a quality win is also a systems
win.

## Rejected or Deferred Directions

### SAE-Only as the Primary Product

Rejected for now. SAE-only retrieval is useful, but the most stable path is
BM25+SAE in one sparse-impact space.

### Naive Query Top-K Latent Truncation

Rejected as a general solution. It reduces cost but loses too much recall on
some datasets.

### Naive SAE DF Stoplist

Rejected as a default. Latent stopwords exist, but a simple DF cutoff does not
separate noise from useful broad semantic concepts reliably enough.

### Active-128 as the Default Systems Profile

Deferred. Active-128 improves quality, but increases postings and weakens the
systems argument. It remains a quality profile, not the first systems profile.

### Per-Query Max Normalization in Native v1

Rejected for the first exact native scorer. It breaks simple block upper-bound
reasoning unless implemented as an exact two-pass algorithm.

### Immediate C Access Method Rewrite

Rejected. The next implementation should be staged:

```text
SQL sidecar block-max
-> standalone C reader
-> read-only native payload
-> mutable generation and delta overlay
-> production AM integration
```

## Implementation Contract for the Next Coding Phase

The next coding phase should not be another scoring-weight sweep. It should
turn the design into a minimal exact native-shaped prototype.

### Phase 0: Freeze Simulator Contract

Required outputs:

- corpus sparse-impact export;
- query sparse-impact export;
- brute-force exact top-k;
- block-max exact top-k;
- per-query opened block/doc/posting diagnostics.

Pass condition:

```text
block-max top-k == brute-force top-k for every query
```

### Phase 1: SQL Block-Max Sidecar

Create SQL tables that mimic the native layout:

```text
doc_table
dimension_dictionary
block_directory
dimension_block_entries
impact_postings
```

The SQL sidecar proves the layout and traversal before C code.

Implemented research harness:

```text
scripts/research_sae_block_max_sql_sidecar.py
scripts/test_research_sae_block_max_sql_sidecar.py
sae-block-max-sql-sidecar-report.md
```

The sidecar materializes `doc_table`, `dimension_dictionary`,
`block_directory`, `dimension_block_entries`, `impact_postings`,
`query_dimensions`, and `expected_rankings`. SQL traversal computes block upper
bounds, finds the first safe prefix where `next_bound <= kth_score`, and then
returns exact top-k rows plus traversal diagnostics. This keeps the SQL phase
native-shaped instead of falling back to a plain postings `GROUP BY`.

### Phase 2: Standalone C Reader

Export the same payload to a binary file and implement:

- payload decode;
- dimension lookup;
- block upper-bound traversal;
- exact top-k heap;
- visibility-free parity with the simulator.

This phase isolates index-engine correctness from PostgreSQL access-method
complexity.

Implemented Phase 2 harness:

```text
scripts/research_sae_block_max_export_payload.py
tests/research_sae_block_max_reader.c
scripts/test_research_sae_block_max_c_reader.py
sae-block-max-c-reader-report.md
```

The reader is deliberately standalone. It validates payload decode and exact
block-max traversal before any PostgreSQL memory-context, page-format, or MVCC
work is introduced.

Phase 2.5 range-lookup hardening is recorded in:

```text
scripts/research_sae_block_max_phase25_benchmark.py
sae-block-max-phase25-report.md
```

The C reader now indexes `global_dim_id -> block_entry_range`, builds touched
blocks from query-dimension ranges, and scores opened blocks through posting
slices instead of full scans.

Phase 2.6 real-artifact validation is recorded in:

```text
scripts/research_sae_block_max_phase26_real_benchmark.py
sae-block-max-phase26-real-benchmark-report.md
```

The result supports a read-only PostgreSQL Phase 3, but only as a diagnostic
prototype: exactness and C lookup mechanics are solid, while current SAE block
bounds still open most documents on the real benchmark artifacts.

### Phase 3: PostgreSQL Read-Only Payload

Add a new read-only sparse-impact payload family and SQL function that returns
TIDs and diagnostics. Do not support mutable updates yet.

### Phase 4: Mutable/Eventual Integration

Add:

- delta overlay;
- delete tombstones;
- rebuild scheduling;
- shared preload publish;
- old generation retirement.

### Phase 5: BM25 Coexistence and Parity

Only after SAE-only and unified sparse exactness are boring, add BM25-only
parity tests against the current mature BM25 payload.

## Required Tests Before Production Claims

- exact equality against brute force on random sparse corpora;
- exact equality against all current benchmark sparse exports;
- BM25-only parity with existing BM25 functions;
- BM25+SAE parity with the unified sparse Python scorer;
- MVCC behavior with invisible high-score rows;
- tombstone and VACUUM behavior;
- delta overlay merge correctness;
- interrupted rebuild keeps old generation valid;
- shared-preload generation swap is atomic for readers;
- benchmark does not regress current BM25 hot paths.

## Current Default Research Profile

Use this profile until new evidence beats it:

```text
dense model: Snowflake/snowflake-arctic-embed-m-v2.0
input_dim: 768
SAE latent_dims: 8192
SAE document active_dims: 64
SAE score mode: normalized_idf_dot
query gate: post-training incremental candidate-budget gate
native score contract: raw weighted impact or fixed saturation
layout: sae_tree
block_size: 8
```

Quality profile for stress tests:

```text
SAE document active_dims: 128
```

## Open Research Risks

- The current learned sparse representation may still be too broad for an
  extreme-pruning index.
- Dense retrieval may remain necessary for some low lexical-overlap semantic
  workloads.
- BM25 and SAE may prefer different document layouts.
- A WAND/MaxScore posting traversal may outperform row-block traversal, but it
  has not replaced the current simulator yet.
- Retrieval-aware training may improve selectivity, but it can also overfit
  benchmark corpora if not validated broadly.

## Final Design Exploration Decision

The exploration is complete enough to stop debating the architecture and start
the native-shaped implementation sequence.

The next branch should implement the sparse-impact block-max prototype without
modifying the current production BM25 path. The success criterion is not just
quality. It is simultaneous:

```text
exactness
+ competitive recall/MRR
+ materially lower decoded/scored work
+ no regression to current BM25 APIs
```

If the native sparse-impact path reaches those criteria, it becomes a credible
next-generation retrieval engine for RAG, agent memory, and AI knowledge-base
search. If it fails, the result is still valuable: SAE remains an auxiliary
signal for hybrid retrieval, and the generic sparse-impact payload can still
serve learned sparse text encoders.
