# Unified Sparse Impact Index Design

Date: 2026-05-10

## Goal

Explore a deeper `BM25 + SAE` design for AI search, RAG, and memory
retrieval: one physical sparse index, one query path, and configurable source
weights.

The target is not late fusion of separate top-k lists. The target is:

```text
lexical BM25 impacts
  + SAE latent semantic impacts
  + optional future entity/facet/memory impacts
  -> one weighted sparse impact index
  -> one top-k retrieval engine
```

This is the vectorless path we should test seriously. Dense vectors can remain
as a baseline and fallback during research, but the production research
question is whether a single sparse impact engine can preserve enough semantic
recall for AI retrieval.

## Why Late Fusion Is Not Enough

The previous hybrid prototype did this:

```text
BM25 top-k
SAE top-k
dense top-k
  -> normalize each source
  -> weighted fusion
```

That is useful for research, but it has three structural limits:

1. Each source has its own candidate window, so good documents can be lost
   before fusion.
2. The database engine cannot prune using a unified upper bound.
3. It keeps BM25 and SAE as separate retrieval systems instead of one physical
   engine.

The deeper design stores both lexical and SAE signals in the same inverted
postings space and scores them in one pass.

## Model Boundary

The database does not train or run SAE models.

Offline pipeline:

```text
document text
  -> Snowflake/snowflake-arctic-embed-m-v2.0
  -> dense embedding
  -> SAE encoder
  -> top-k sparse latent activations
  -> index postings
```

Query pipeline:

```text
query text
  -> lexical query terms
  -> Snowflake dense embedding
  -> same SAE encoder
  -> top-k sparse latent query activations
  -> unified sparse query dimensions
```

PostgreSQL receives only sparse dimensions and weights.

## Dimension Namespace

Every feature is a weighted sparse dimension:

```text
source      dim_id                  example
bm25        token id / term id       cuda
field_bm25  field + token id         title:cuda
sae         latent id                8192:4310
entity      entity id                entity:nvidia
memory      memory facet id          project:ii42
```

The first prototype only implements:

```text
bm25 term dimensions
sae latent dimensions
```

## Physical Shape

Conceptual storage:

```text
dimension_dictionary
  source text
  dim_key text or int
  dim_id int32
  doc_freq int64
  idf float4

impact_postings
  dim_id int32
  doc_id
  source smallint
  impact float4
  optional field/source mask

block_max
  dim_id int32
  block_id int32
  max_impact float4
  source_mask
```

BM25 postings store precomputed per-document term impact:

```text
bm25_impact(term, doc) =
    idf(term) * tf * (k1 + 1)
    / (tf + k1 * (1 - b + b * doc_len / avg_doc_len))
```

SAE postings store normalized latent impact:

```text
sae_impact(latent, doc) =
    doc_latent_weight / doc_latent_norm * idf(latent)
```

At query time:

```text
bm25 query term weight = 1.0
sae query latent weight = query_latent_weight
```

## Scoring

The exact prototype uses separate source accumulators inside one scan:

```text
bm25_raw(doc) = sum(bm25_impact(term, doc))

sae_raw(doc) =
    sum(query_latent_weight[i] * sae_impact(i, doc))
```

Then it normalizes per source for the query and applies weights:

```text
score(doc) =
    bm25_weight * bm25_raw(doc) / max_bm25_raw
  + sae_weight  * sae_raw(doc)  / max_sae_raw
```

This keeps BM25 and SAE comparable without hard-coding global score ranges.
The C implementation should keep separate source accumulators until the final
top-k heap so that the formula can evolve without changing postings.

## Query Flow

The desired engine flow:

```text
1. Build sparse query dimensions:
   - lexical terms from query text
   - SAE latent dims from query embedding + SAE encoder

2. Map query dimensions to posting lists.

3. Scan one unified postings set:
   - add BM25 impacts into bm25 accumulator
   - add SAE impacts into SAE accumulator

4. Normalize and weight source accumulators.

5. Return top-k document ids and source breakdowns.
```

Unlike late fusion, there is no separate `bm25_candidate_k` or
`sae_candidate_k`. The candidate set is the union produced by matching sparse
dimensions in the single index.

## Efficiency Path

The first implementation can be exact. The production index needs pruning:

- impact-ordered postings for high-impact traversal;
- block-max upper bounds per dimension;
- source-aware upper bounds so BM25 and SAE can be weighted dynamically;
- WAND / Block-Max WAND to avoid scoring every matching document;
- optional stop-term or max-df guards for broad lexical dimensions;
- eventually per-field max impacts for field-aware BM25.

The important design constraint is that pruning must account for runtime
source weights:

```text
block_upper_bound =
    bm25_weight * bm25_block_max
  + sae_weight  * sae_block_max
```

That lets one physical index support different retrieval policies.

The native block-max design has now been split into a more implementation-
oriented draft:

```text
unified-sparse-block-max-native-index-design.md
```

That draft updates this earlier design in one important way: per-query max
normalization is useful for offline evaluation, but it is not the first safe
native pruning contract. A native block-max implementation should start with
raw weighted impacts or fixed source-level saturation so block upper bounds are
valid before opening candidate blocks.

The overall design exploration has now been closed in:

```text
sae-native-index-design-exploration-summary.md
```

Treat this document as the historical unified sparse-impact design and the
summary file as the current decision record for implementation sequencing.

## PostgreSQL API Sketch

Initial SQL-facing function:

```sql
SELECT *
FROM ii42_unified_sparse_query(
    'commons.data_arxiv__unified_sparse_idx'::regclass,
    bm25_query => 'CUDA graph neural network optimization',
    sae_dimensions => ARRAY[4310, 917, 22]::int4[],
    sae_weights => ARRAY[0.42, 0.31, 0.18]::real[],
    bm25_weight => 1.0,
    sae_weight => 1.0,
    k => 20
);
```

Returned rows should expose score breakdowns:

```text
ctid / document id
score
bm25_score
sae_score
bm25_norm
sae_norm
matched_bm25_dims
matched_sae_dims
```

The score breakdown is important for debugging AI retrieval and for later
reranker feature logging.

## Prototype Implemented

The first offline single-index prototype is:

```text
scripts/research_sae_unified_sparse_eval.py
```

It builds one in-memory sparse postings dictionary containing:

- BM25 term impact postings;
- SAE latent impact postings.

It does not call separate BM25 and SAE top-k retrievers. It scans the shared
postings space once per query, accumulates per-source raw scores, normalizes
per source, and ranks the configured source combination.

Run command:

```bash
python3 scripts/research_sae_unified_sparse_eval.py \
  --documents /tmp/ii42_sae_scifact/data/documents.jsonl \
  --queries /tmp/ii42_sae_scifact/data/queries.jsonl \
  --doc-latents /tmp/ii42_sae_scifact_mps_8192/latent8192_active32/doc_latents.jsonl \
  --query-latents /tmp/ii42_sae_scifact_mps_8192/latent8192_active32/query_latents.jsonl \
  --output-dir /tmp/ii42_sae_scifact_mps_8192/unified_sparse \
  --score-mode normalized_idf_dot \
  --final-k 100 \
  --bm25-weight 1.0 \
  --sae-weight 1.0
```

## Current SciFact Result

Input:

- documents: `400`
- queries: `40`
- SAE model: Snowflake input, `latent_dims=8192`, `active_dims=32`
- SAE score mode: `normalized_idf_dot`

Unified sparse index size:

| source | dimensions | postings |
| --- | ---: | ---: |
| BM25 | `9889` | `49797` |
| SAE | `1362` | `12800` |
| total | `11251` | `62597` |

Unified `BM25 + SAE` weight sweep:

| SAE weight | Recall@20 | Recall@100 | MRR@20 | MRR@100 |
| ---: | ---: | ---: | ---: | ---: |
| `0.10` | `0.9500` | `0.9750` | `0.8986` | `0.8996` |
| `0.25` | `0.9500` | `0.9750` | `0.9115` | `0.9126` |
| `0.50` | `0.9750` | `0.9750` | `0.9127` | `0.9127` |
| `0.75` | `0.9750` | `0.9750` | `0.9169` | `0.9169` |
| `1.00` | `0.9750` | `0.9750` | `0.9297` | `0.9297` |
| `1.25` | `0.9750` | `1.0000` | `0.9005` | `0.9008` |

For vectorless `BM25 + SAE`, the best current balance is:

```text
bm25_weight = 1.0
sae_weight = 1.0
```

This is different from the earlier three-way `dense + BM25 + SAE` experiment,
where SAE should remain a light auxiliary signal.

## SQL Sidecar Prototype

The first SQL sidecar prototype is also implemented:

```text
scripts/research_sae_unified_sql_sidecar.py
```

It loads both sources into one PostgreSQL table:

```sql
CREATE TABLE unified_sparse_postings (
    run_id text NOT NULL,
    source text NOT NULL,
    dim_key text NOT NULL,
    document_id text NOT NULL,
    impact real NOT NULL
);

CREATE INDEX unified_sparse_postings_lookup_idx
ON unified_sparse_postings (run_id, source, dim_key);
```

The query uses one `query_dims` relation for both BM25 terms and SAE latents,
then computes raw source scores and weighted normalized score in SQL:

```sql
WITH query_dims AS (
    SELECT *
    FROM unnest(
        $1::text[],
        $2::text[],
        $3::real[]
    ) AS q(source, dim_key, query_weight)
),
raw_scores AS (
    SELECT
        p.document_id,
        p.source,
        SUM(q.query_weight * p.impact)::real AS raw_score
    FROM query_dims AS q
    JOIN unified_sparse_postings AS p
      ON p.run_id = $4
     AND p.source = q.source
     AND p.dim_key = q.dim_key
    GROUP BY p.document_id, p.source
),
source_max AS (
    SELECT source, MAX(raw_score)::real AS max_score
    FROM raw_scores
    GROUP BY source
),
combined AS (
    SELECT
        r.document_id,
        SUM(source_weight * r.raw_score / NULLIF(m.max_score, 0.0)) AS score
    FROM raw_scores AS r
    JOIN source_max AS m ON m.source = r.source
    GROUP BY r.document_id
)
SELECT document_id, score
FROM combined
ORDER BY score DESC, document_id
LIMIT $5;
```

The sidecar matches the Python exact unified scorer:

```text
Recall@20: 0.975
Recall@100: 0.975
MRR@20: 0.9297
MRR@100: 0.9297
```

## Interpretation

The first result supports continuing the single-index path:

- unified `BM25 + SAE` reaches the same `Recall@20` as dense-based hybrid on
  this small sample;
- `BM25 + SAE` improves MRR over BM25 alone;
- the index shape is simple enough to map to one physical inverted engine.

It is not enough to claim dense vector retrieval can be removed. SciFact is
small and lexically strong. The next proof must use larger and harder semantic
queries.

## Larger SciDocs Validation

The larger validation is recorded in
`unified-sparse-larger-validation-report.md`.

BEIR SciDocs sample:

- documents: `2000`
- queries: `100`
- SAE model: Snowflake input, `latent_dims=8192`, `active_dims=32`
- SAE score mode: `normalized_idf_dot`

Single-source metrics:

| source | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: |
| dense vector | `0.5195` | `0.6740` | `0.6544` |
| BM25 | `0.4315` | `0.5720` | `0.5455` |
| SAE | `0.4440` | `0.6400` | `0.5664` |

Unified index size:

| source | dimensions | postings |
| --- | ---: | ---: |
| BM25 | `19471` | `207188` |
| SAE | `1461` | `64000` |
| total | `20932` | `271188` |

Best default `BM25 + SAE` result with `sae_weight=1.0`:

```text
Recall@20: 0.4885
Recall@100: 0.6470
MRR@20: 0.6280
MRR@100: 0.6293
```

The SQL sidecar matched the Python scorer exactly with `0` ranking diffs over
`100` queries.

The important change from SciFact is that dense vector retrieval remains
stronger on SciDocs. This is the right warning: unified `BM25 + SAE` is
clearly better than BM25, but it is not ready to replace dense vector search
without more model and field-aware lexical work.

Follow-up analysis changed the quality/cost picture. Increasing SAE
`active_dims` improves the semantic sparse signal substantially:

| active dims | SAE weight | total postings | Recall@20 | Recall@100 | MRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `32` | `1.0` | `271188` | `0.4885` | `0.6470` | `0.6280` |
| `64` | `1.5` | `335188` | `0.5035` | `0.6775` | `0.6654` |
| `128` | `1.5` | `463188` | `0.5280` | `0.6750` | `0.6629` |

Dense vector baseline on the same slice:

```text
Recall@20: 0.5195
Recall@100: 0.6740
MRR@20: 0.6544
```

The `active_dims=128` profile surpassed dense on this SciDocs slice, while the
`active_dims=64` profile reached a strong quality/cost balance. This supports
continuing the vectorless unified sparse path, but the active-dimension budget
is now a first-class index-size knob.

## Updated Implementation Stages

The first follow-up batch is complete:

- field-aware BM25 dimensions are implemented in the Python and SQL sidecar
  prototypes, but did not improve the SciDocs 2k qrels benchmark;
- `input_dim=768` improved the systems profile enough that
  `BM25 + SAE 8192/64` beat dense vector retrieval on MRR@20 and Recall@100
  in the latest SciDocs run;
- SPLADE SparseEncoder was tested as a neural sparse baseline and underperformed
  the current BM25+SAE prototype on this slice;
- query-adaptive source weighting has modest oracle headroom, but is not yet a
  blocker for the physical index design;
- a 2k arxiv production-field sanity slice can be ingested, but only
  self-retrieval labels were available in this pass.

The revised implementation stages are:

1. Use `input_dim=768`, `latent_dims=8192`, `active_dims=64` as the next
   systems profile.
2. Keep field-aware BM25 in the model, but treat it as dataset-sensitive rather
   than a default quality win.
3. Add exact source-breakdown output to the SQL sidecar.
4. Build a real arxiv semantic evaluation set before judging vector
   replacement.
5. Only after quality holds, design the C index access method:
   exact accumulator first, then Block-Max WAND.

## Open Risks

- SAE is trained on dense embeddings, so query-time still needs the embedding
  model and SAE encoder.
- Score normalization currently depends on candidate-set max scores. Native
  pruning must preserve correctness or define an approximate mode explicitly.
- Broad lexical terms can create large scans unless the engine adds WAND,
  stop-term handling, or max-df policy.
- Small-corpus results can overestimate quality. The next benchmark must be
  larger and less lexically easy.
- The current arxiv sanity test uses self-retrieval labels and is only a
  pipeline check. It must not be used as quality evidence.

## Design Exploration Closure

The current exploration phase is complete. The next work is no longer another
late-fusion or static weight sweep. The accepted implementation direction is:

```text
generic sparse-impact payload
+ immutable block-max generation
+ exact delta overlay
+ boundable raw or fixed-saturation score contract
+ BM25-only parity before production API routing
```

The detailed closure and implementation contract are in
`sae-native-index-design-exploration-summary.md`.
