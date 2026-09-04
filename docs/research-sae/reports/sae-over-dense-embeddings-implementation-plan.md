# SAE over Dense Embeddings Implementation Plan

Date: 2026-05-06

## Goal

Build a practical SAE-over-dense-embeddings prototype before changing the
PostgreSQL extension. The goal is to answer one question with evidence:

> Can sparse latent activations derived from our existing dense embeddings
> become a useful third retrieval signal beside BM25 and dense vector search?

The first implementation should be offline and reproducible. Native index work
starts only after the offline and SQL-sidecar prototypes pass quality gates.

## Non-Goals

- Do not put ML training or model inference inside PostgreSQL.
- Do not make the database layer SAE-specific.
- Do not replace BM25 or VectorChord in the first phase.
- Do not promise user-facing latent feature explanations in the first phase.
- Do not start with approximate sparse indexing before exact retrieval is
  correct and measured.

## Target Architecture

```text
document text
  -> current dense embedding model
  -> dense embedding
  -> SAE encoder
  -> top-k sparse latent activations
  -> sparse latent postings

query text
  -> same dense embedding model
  -> dense embedding
  -> same SAE encoder
  -> top-k sparse latent activations
  -> sparse latent top-k retrieval

BM25 candidates
  + dense vector candidates
  + SAE sparse latent candidates
  -> SQL-visible hybrid fusion
  -> optional AI reranker
```

## Key Design Choice

Use SAE-first for the model experiment, but design all storage and retrieval
interfaces as generic weighted sparse vectors:

```text
dimensions: int32[]
weights: float32[]
model_id: text
encoder_version: text
norm: float32
```

This keeps the path open for SPLADE, BGE-M3 sparse weights, and future sparse
models.

## Stage 1: Data Inventory and Export

### Objective

Determine exactly what dense vectors we can use and export a clean research
dataset.

### Tasks

1. Inspect available dense vector columns for arxiv, pubmed, policy docs, and
   policy chunks.
2. Record vector type, dimension, quantization state, null rate, row count, and
   source embedding model.
3. Decide whether to use existing stored vectors, dequantized vectors, or
   recomputed original embeddings.
4. Export train/validation/test splits with stable ids.
5. Export a query set with known or approximate relevance signals.

### Preferred Input

Use original dense embeddings if available. If only `rabitq8` vectors are
stored, dequantized vectors are acceptable for a first prototype, but the
report must record that this may hurt SAE training quality.

### Deliverables

- `research/sae/data_inventory.json`
- `research/sae/datasets/{corpus}/documents.parquet`
- `research/sae/datasets/{corpus}/queries.parquet`
- `research/sae/datasets/{corpus}/qrels.parquet` where labels exist

### Script Sketch

```text
scripts/research_sae_export_embeddings.py
```

Required options:

- `--corpus arxiv|pubmed|policy_ca|policy_tx|policy_wa|policy_chunks`
- `--sample-size`
- `--output-dir`
- `--include-text-preview`
- `--source-vector-column`

## Stage 2: SAE Training Prototype

### Objective

Train a small sparse autoencoder over dense embeddings and produce sparse
latent activations.

### Recommended First Model

Start with a TopK SAE:

```text
x dense embedding
  -> encoder linear layer
  -> activation
  -> keep top-k positive activations
  -> decoder reconstruction
```

TopK is preferred because it gives a hard active-dimension budget, which maps
directly to inverted-index cost.

### Initial Config Grid

Embedding normalization:

- raw dense vector
- L2-normalized dense vector

Latent dimension count:

- 4096
- 8192
- 16384

Active dimensions:

- 16
- 32
- 64
- 128

Loss:

- reconstruction loss
- reconstruction plus light activation regularization

### Deliverables

- `research/sae/models/{run_id}/config.json`
- `research/sae/models/{run_id}/sae.pt`
- `research/sae/models/{run_id}/training_metrics.json`
- `research/sae/models/{run_id}/feature_stats.parquet`

### Script Sketch

```text
scripts/research_sae_train.py
```

Required options:

- `--input documents.parquet`
- `--latent-dims`
- `--active-dims`
- `--normalize l2|none`
- `--epochs`
- `--batch-size`
- `--output-dir`

## Stage 3: Sparse Latent Encoding

### Objective

Encode documents and queries into deterministic sparse latent vectors.

### Tasks

1. Load the trained SAE.
2. Encode document embeddings.
3. Encode query embeddings.
4. Keep top-k non-zero positive activations.
5. Sort dimensions by id for deterministic storage.
6. Compute per-dimension document frequency and IDF.
7. Compute vector norms and density statistics.

### Deliverables

- `research/sae/runs/{run_id}/doc_latents.parquet`
- `research/sae/runs/{run_id}/query_latents.parquet`
- `research/sae/runs/{run_id}/latent_stats.parquet`
- `research/sae/runs/{run_id}/density_report.json`

### Script Sketch

```text
scripts/research_sae_encode.py
```

Required options:

- `--model-dir`
- `--documents`
- `--queries`
- `--doc-active-dims`
- `--query-active-dims`
- `--output-dir`

## Stage 4: Offline Retrieval Evaluation

### Objective

Measure whether SAE latent sparse retrieval is useful before involving
PostgreSQL.

### Candidate Scoring Functions

Plain dot product:

```text
score(d, q) = sum(q_w[i] * d_w[i])
```

IDF-weighted dot product:

```text
score(d, q) = sum(q_w[i] * d_w[i] * idf[i])
```

Normalized IDF dot product:

```text
score(d, q) = sum(q_w[i] * d_w[i] * idf[i]) / doc_norm
```

### Baselines

- Current BM25 top-k.
- Current dense vector top-k.
- Existing BM25/vector hybrid top-k.
- Optional learned sparse text baseline: SPLADE or BGE-M3 sparse weights.

### Metrics

- Recall@20, Recall@100, Recall@1000.
- nDCG@10 where labels exist.
- Candidate overlap with BM25 and vector.
- Unique useful candidate rate.
- Reranker input hit rate.
- Query latency for exact Python sparse retrieval.
- Estimated posting count and index size.

### Deliverables

- `research/sae/runs/{run_id}/retrieval_metrics.json`
- `research/sae/runs/{run_id}/per_query_examples.jsonl`
- `research/sae/runs/{run_id}/candidate_overlap.json`
- `research/sae/runs/{run_id}/summary.md`

### Script Sketch

```text
scripts/research_sae_retrieve.py
scripts/research_sae_report.py
```

## Stage 5: SQL Sidecar Prototype

### Objective

Validate that the sparse latent retrieval shape can work in PostgreSQL before
writing native C index code.

### Prototype Tables

```sql
CREATE TABLE sae_sparse_latent_postings (
    run_id text NOT NULL,
    dim_id integer NOT NULL,
    document_id text NOT NULL,
    weight real NOT NULL,
    PRIMARY KEY (run_id, dim_id, document_id)
);

CREATE TABLE sae_sparse_latent_stats (
    run_id text NOT NULL,
    dim_id integer NOT NULL,
    doc_freq integer NOT NULL,
    idf real NOT NULL,
    PRIMARY KEY (run_id, dim_id)
);
```

### Query Shape

```sql
WITH query_dims AS (
    SELECT *
    FROM unnest(
        %s::integer[],
        %s::real[]
    ) AS q(dim_id, query_weight)
),
candidate_scores AS (
    SELECT
        p.document_id,
        SUM(q.query_weight * p.weight * s.idf)::real AS sparse_score
    FROM query_dims AS q
    JOIN sae_sparse_latent_postings AS p
      ON p.run_id = %s
     AND p.dim_id = q.dim_id
    JOIN sae_sparse_latent_stats AS s
      ON s.run_id = p.run_id
     AND s.dim_id = p.dim_id
    GROUP BY p.document_id
)
SELECT document_id, sparse_score
FROM candidate_scores
ORDER BY sparse_score DESC, document_id
LIMIT %s;
```

### Deliverables

- `research/sae/sql/sidecar_schema.sql`
- `scripts/research_sae_load_sidecar.py`
- `scripts/research_sae_sql_benchmark.py`
- SQL latency and result-parity report

## Stage 6: Hybrid Fusion Integration Experiment

### Objective

Treat SAE latent sparse retrieval as a third candidate source in the existing
hybrid fusion model.

### Candidate Source Shape

```text
source_name = 'sae_latent'
ctid/document_id = document id
raw_value = sparse_score
source_rank = rank within SAE candidates
normalizer = minmax or source-max
direction = higher-is-better
```

### Experiment

Compare:

- BM25 only
- vector only
- BM25 + vector
- BM25 + SAE
- vector + SAE
- BM25 + vector + SAE

### Deliverables

- fusion metrics report;
- per-query examples where SAE helps;
- per-query examples where SAE hurts;
- recommended candidate count and source weight.

## Stage 7: Native Index Decision

Only start native C index work if the evidence is strong.

### Continue Criteria

- SAE sparse retrieval keeps at least 80% of dense vector Recall@1000 at a
  practical active-dimension budget.
- SAE adds useful candidates not already found by BM25/vector.
- SQL sidecar latency is promising enough to justify native indexing.
- Index-size estimate is acceptable.
- Score normalization is stable across at least two corpora.

### Stop Criteria

- SAE mostly duplicates dense vector candidates.
- SAE loses too much recall under top-k pruning.
- BGE-M3 or SPLADE sparse baselines dominate SAE with acceptable operational
  cost.
- Latent scores are unstable and cannot be fused reliably.

## Native Index Sketch

If the gates pass, build a new generic weighted sparse index family.

### First Native API

```sql
ii42_sparse_query(
    index_name regclass,
    dimensions integer[],
    weights real[],
    k integer
)
```

### Hybrid Helper

```sql
ii42_hybrid_sparse_candidates(
    source_name text,
    index_name regclass,
    dimensions integer[],
    weights real[],
    weight double precision,
    candidate_k integer,
    normalizer text
)
```

### Initial Implementation

- Exact inverted-list accumulation.
- Non-negative weights only.
- Impact-sorted lists later.
- WAND/block-max pruning later.
- Eventual maintenance compatible with current ii42 worker design.

## Immediate Next Work

1. Inspect commons dense vector availability and export feasibility.
2. Build the offline data export script.
3. Train a tiny TopK SAE on one corpus sample.
4. Encode documents and queries to sparse latent vectors.
5. Run exact sparse retrieval in Python.
6. Compare against BM25/vector candidates.
7. Write a first empirical report before touching PostgreSQL internals.

## Recommended First Corpus

Start with arxiv.

Reasons:

- document-level semantics are natural;
- current hybrid vector/BM25 behavior is already well understood;
- query examples are easier to inspect manually;
- failure cases are easier to classify than policy shell documents.

After arxiv, run pubmed. Policy should be third because policy documents have
more corpus-specific cleanup and chunk/document granularity complications.

## Recommended First Success Report

The first empirical report should answer:

1. What active-dimension budget works?
2. How much dense vector recall is retained?
3. Which candidates are unique to SAE?
4. Does SAE help reranker input quality?
5. How large would the index be?
6. Does IDF help or hurt latent sparse scoring?
7. Is SAE promising enough to justify a SQL sidecar prototype?

## Working Assumption

SAE over dense embeddings is likely the fastest research path because it
reuses our existing embedding model. The production database feature, however,
should be a generic weighted sparse retrieval primitive. This separation keeps
the first experiment focused without creating a model-specific database dead
end.

## Implemented First-Phase Prototype

The first offline prototype is now implemented under `scripts/`:

- `research_sae_common.py`: shared JSONL, SAE, encoding, sparse retrieval,
  dense retrieval, BM25 baseline, and metrics utilities.
- `research_sae_export_embeddings.py`: exports synthetic, JSONL, or PostgreSQL
  records into the research JSONL contract. It can read existing dense vectors
  or embed missing text with `Snowflake/snowflake-arctic-embed-m-v2.0` through
  SentenceTransformers.
- `research_sae_make_doc_queries.py`: creates self-retrieval query/qrels files
  from exported documents when no external qrels are available.
- `research_sae_train.py`: trains a TopK SAE over dense embeddings.
- `research_sae_encode.py`: encodes documents or queries into top-k sparse
  latent activations.
- `research_sae_retrieve.py`: evaluates sparse latent retrieval against dense
  vector and simple BM25 baselines.
- `research_sae_sql_sidecar.py`: loads sparse latent postings into PostgreSQL
  and runs exact SQL top-k retrieval over those postings.
- `research_sae_smoke.py`: runs a complete synthetic end-to-end smoke test.

The prototype deliberately uses JSONL and JSON outputs instead of Parquet so it
can run in the current development environment without requiring `pyarrow`.

## Prototype Commands

Synthetic smoke:

```bash
python3 scripts/research_sae_smoke.py \
  --output-dir /tmp/ii42_sae_smoke \
  --documents 180 \
  --queries 18 \
  --embedding-dim 48 \
  --clusters 6 \
  --latent-dims 128 \
  --active-dims 12 \
  --epochs 8
```

Manual pipeline:

```bash
python3 scripts/research_sae_export_embeddings.py \
  --source synthetic \
  --output-dir /tmp/ii42_sae_pipeline/data \
  --documents 180 \
  --queries 18 \
  --embedding-dim 48 \
  --clusters 6

python3 scripts/research_sae_train.py \
  --input /tmp/ii42_sae_pipeline/data/documents.jsonl \
  --output-dir /tmp/ii42_sae_pipeline/model \
  --latent-dims 128 \
  --active-dims 12 \
  --epochs 8

python3 scripts/research_sae_encode.py \
  --input /tmp/ii42_sae_pipeline/data/documents.jsonl \
  --model-dir /tmp/ii42_sae_pipeline/model \
  --output /tmp/ii42_sae_pipeline/run/doc_latents.jsonl \
  --stats-output /tmp/ii42_sae_pipeline/run/latent_stats.json \
  --density-output /tmp/ii42_sae_pipeline/run/doc_density.json

python3 scripts/research_sae_encode.py \
  --input /tmp/ii42_sae_pipeline/data/queries.jsonl \
  --model-dir /tmp/ii42_sae_pipeline/model \
  --output /tmp/ii42_sae_pipeline/run/query_latents.jsonl \
  --density-output /tmp/ii42_sae_pipeline/run/query_density.json

python3 scripts/research_sae_retrieve.py \
  --documents /tmp/ii42_sae_pipeline/data/documents.jsonl \
  --queries /tmp/ii42_sae_pipeline/data/queries.jsonl \
  --doc-latents /tmp/ii42_sae_pipeline/run/doc_latents.jsonl \
  --query-latents /tmp/ii42_sae_pipeline/run/query_latents.jsonl \
  --qrels /tmp/ii42_sae_pipeline/data/qrels.jsonl \
  --output-dir /tmp/ii42_sae_pipeline/report \
  --top-k 100
```

PostgreSQL arxiv sample using stored Snowflake vectors:

```bash
python3 scripts/research_sae_export_embeddings.py \
  --source postgres \
  --dsn "$SAE_RESEARCH_DSN" \
  --table commons.data_arxiv \
  --id-column id \
  --text-columns title,abstract \
  --vector-column vector \
  --dequantize-vector \
  --where "vector IS NOT NULL" \
  --limit 500 \
  --output-dir /tmp/ii42_sae_arxiv_real/data
```

Then create self-retrieval queries:

```bash
python3 scripts/research_sae_make_doc_queries.py \
  --documents /tmp/ii42_sae_arxiv_real/data/documents.jsonl \
  --output /tmp/ii42_sae_arxiv_real/data/queries.jsonl \
  --qrels-output /tmp/ii42_sae_arxiv_real/data/qrels.jsonl \
  --count 50
```

SQL sidecar:

```bash
python3 scripts/research_sae_sql_sidecar.py \
  --dsn "dbname=postgres" \
  --schema sae_research_smoke \
  --run-id arxiv-smoke \
  --doc-latents /tmp/ii42_sae_arxiv_real/run/doc_latents.jsonl \
  --query-latents /tmp/ii42_sae_arxiv_real/run/query_latents.jsonl \
  --qrels /tmp/ii42_sae_arxiv_real/data/qrels.jsonl \
  --output-dir /tmp/ii42_sae_arxiv_real/sql_report \
  --top-k 100 \
  --reset
```

## First Verification Results

Synthetic smoke, 180 documents and 18 queries:

- sparse latent `Recall@20`: `0.6667`
- sparse latent `Recall@100`: `1.0`
- dense `Recall@20`: `0.6667`
- dense `Recall@100`: `1.0`

Real arxiv sanity check, 500 documents and 50 self-retrieval queries using
dequantized stored Snowflake vectors:

- sparse latent `Recall@20`: `1.0`
- sparse latent `Recall@100`: `1.0`
- dense `Recall@20`: `1.0`
- BM25 `Recall@20`: `1.0`
- document mean active dimensions: `32.0`
- document unique latent dimensions: `263`
- sparse-vs-dense top20 mean overlap: `0.541`
- sparse-vs-BM25 top20 mean overlap: `0.296`

SQL sidecar over the same arxiv sample:

- documents: `500`
- postings: `16000`
- latent dimensions: `263`
- SQL sparse latent `Recall@20`: `1.0`
- SQL sparse latent `Recall@100`: `1.0`

The arxiv check is only a pipeline sanity test because self-retrieval qrels are
easy. The useful signal is that sparse latent ranking is not identical to dense
or BM25 ranking, which makes it worth moving to real qrels and query sets.

## Real-Qrels SciFact Sweep

The first real-qrels sweep is implemented and recorded in
`sae-scifact-first-phase-report.md`.

Additions:

- `research_sae_prepare_beir.py` prepares BEIR SciFact or SciDocs data with
  Snowflake dense embeddings.
- `research_sae_sweep.py` trains several TopK SAE configurations and compares
  sparse latent retrieval against dense vector and simple BM25 baselines.
- `research_sae_sql_sidecar.py` now has result parity on the best SciFact
  `idf_dot` run.

SciFact sample:

- documents: `400`
- queries: `40`
- embedding dimension: `256`
- query encoding: `query: ` prefix

Best isolated SAE result in this sweep:

- run: `latent256_active16`
- score mode: `idf_dot`
- sparse latent `Recall@20`: `0.835`
- sparse latent `Recall@100`: `0.9075`
- dense top-20 overlap: `0.3525`
- BM25 top-20 overlap: `0.2225`

SQL sidecar parity for the same run:

- postings: `6400`
- dimensions with postings: `117`
- SQL sparse latent `Recall@20`: `0.835`
- SQL sparse latent `Recall@100`: `0.9075`

Conclusion: SAE is not strong enough as a standalone retriever yet, but it is
non-duplicative enough to justify the next hybrid-contribution experiment. Do
not start native index work until the hybrid experiment shows unique relevant
candidate gain at practical candidate budgets.

## Snowflake MPS Prototype

The first larger local-Mac prototype is recorded in
`sae-snowflake-mps-prototype-report.md`.

Configuration:

- input dense model: `Snowflake/snowflake-arctic-embed-m-v2.0`
- input dimension used by research data: `256`
- SAE latent dimensions: `8192`
- active dimensions per record: `32`
- score mode: `normalized_idf_dot`
- training device: `mps`
- epochs: `20`

Result on the same SciFact sample:

- SAE `Recall@20`: `0.925`
- SAE `Recall@100`: `0.975`
- SQL sidecar parity: exact match with offline SAE metrics
- postings: `12800` for `400` documents
- unique latent dimensions with postings: `1362`

The new hybrid evaluator is implemented as:

```text
scripts/research_sae_hybrid_eval.py
```

Equal-weight `BM25 + SAE` matched `dense + BM25` recall and improved MRR on the
small SciFact slice:

- `dense + BM25` MRR@20: `0.9225`
- `BM25 + SAE` MRR@20: `0.9297`

Three-way equal fusion was worse, so the next default for SAE as an auxiliary
signal should be conservative:

```text
dense_weight = 1.0
bm25_weight = 1.0
sae_weight = 0.10
```

This is promising enough to continue the vectorless `BM25 + SAE` experiment,
but not enough to remove dense vector retrieval from the production design.

## Unified BM25 + SAE Sparse Index Prototype

The next-stage single-index design is recorded in
`unified-sparse-impact-index-design.md`.

The new offline prototype is:

```text
scripts/research_sae_unified_sparse_eval.py
scripts/research_sae_unified_sql_sidecar.py
```

It is different from the earlier hybrid evaluator. Instead of computing
separate BM25 and SAE top-k lists and fusing them afterward, it builds one
shared sparse postings space:

```text
BM25 term impact postings
SAE latent impact postings
```

Each query scans the unified postings space once, accumulates per-source raw
scores, normalizes source scores for the query, and ranks the weighted
combination.

Current SciFact result for `BM25 + SAE` with the Snowflake `8192/32` SAE:

- BM25 dimensions: `9889`
- BM25 postings: `49797`
- SAE dimensions: `1362`
- SAE postings: `12800`
- total dimensions: `11251`
- total postings: `62597`
- best current vectorless weights: `bm25_weight=1.0`, `sae_weight=1.0`
- `Recall@20`: `0.975`
- `Recall@100`: `0.975`
- `MRR@20`: `0.9297`

This validates the single-index direction well enough to build a SQL sidecar
unified postings table, which is now implemented and parity-checked against
the Python scorer. It still does not justify removing dense vector retrieval
from production until larger SciDocs/arxiv evaluations pass.

## Larger Unified Sparse Validation

The larger SciDocs validation is recorded in
`unified-sparse-larger-validation-report.md`.

SciDocs sample:

- documents: `2000`
- queries: `100`
- BM25 dimensions/postings: `19471` / `207188`
- SAE dimensions/postings: `1461` / `64000`
- total dimensions/postings: `20932` / `271188`

Result:

- dense vector `Recall@20`: `0.5195`
- BM25 `Recall@20`: `0.4315`
- SAE `Recall@20`: `0.4440`
- unified `BM25 + SAE` `Recall@20`: `0.4885`
- unified `BM25 + SAE` `Recall@100`: `0.6470`
- unified `BM25 + SAE` `MRR@20`: `0.6280`

The SQL sidecar matched the Python unified scorer with `0` ranking diffs over
`100` queries. The conclusion is that unified `BM25 + SAE` reliably improves
BM25, but dense vector retrieval is still stronger on harder semantic data.
The next useful improvement is field-aware BM25 dimensions before moving toward
native C index work.

Follow-up gap analysis and active-dimension scaling refined that conclusion:

- dense still finds more unique relevant hits than unified sparse at top 20,
  but unified sparse also finds relevant hits that dense misses;
- dense-over-unified examples often have low query/document lexical overlap,
  which is the failure mode SAE should target;
- increasing SAE active dimensions from `32` to `64` and `128` substantially
  improves retrieval quality.

SciDocs active-dimension scaling:

| profile | Recall@20 | Recall@100 | MRR@20 | total postings |
| --- | ---: | ---: | ---: | ---: |
| `BM25 + SAE32` | `0.4885` | `0.6470` | `0.6280` | `271188` |
| `BM25 + SAE64` | `0.5035` | `0.6775` | `0.6654` | `335188` |
| `BM25 + SAE128` | `0.5280` | `0.6750` | `0.6629` | `463188` |
| `dense` | `0.5195` | `0.6740` | `0.6544` | vector index |

This means SAE can still improve materially. The likely next default profiles
are:

```text
systems profile: active_dims=64, sae_weight=1.5
quality profile: active_dims=128, sae_weight=1.5
```

The next research priority is no longer "does SAE work at all"; it is the
quality/cost frontier:

1. field-aware lexical dimensions;
2. active-dimension cost model;
3. arxiv-style semantic evaluation;
4. testing whether full `768`-dimensional Snowflake input improves SAE enough
   to justify the extra model size.

## Native Design Exploration Closure

The SAE-over-dense exploration has now reached a native-index design decision.
The current implementation-facing record is:

```text
sae-native-index-design-exploration-summary.md
unified-sparse-block-max-native-index-design.md
```

The accepted direction is a generic sparse-impact block-max payload, not an
SAE-specific index and not another Python late-fusion layer. SAE remains an
external encoder over dense embeddings. PostgreSQL receives weighted sparse
dimensions and searches them together with BM25 impacts.

The first native implementation should use the current systems profile:

```text
Snowflake 768-dimensional input
SAE 8192/64 normalized_idf_dot
post-training incremental candidate-budget query gate
raw weighted impact or fixed source-level saturation scoring
sae_tree layout
block_size = 8
```

The current production BM25 path should remain untouched until the new payload
proves BM25-only parity, SAE-only exactness, and BM25+SAE exactness.

## Current Development Status

The project has moved beyond the first offline prototype and SQL sidecar. The
current `sae` branch contains:

- offline Snowflake 768 + SAE 8192/64 quality artifacts;
- unified sparse BM25+SAE research scorer;
- read-only packed sparse-impact generations;
- PostgreSQL resident generation table helpers;
- PostgreSQL read-only exact query path;
- PostgreSQL v4 impact-head candidate query path;
- three-way BM25, BM25+dense, BM25+SAE quality/performance matrix.

The latest three-way matrix is recorded in:

```text
sae-bm25-dense-sae-current-matrix-report.md
results/sae/phase417/bm25-dense-sae-current/summary.md
```

Mean sampled-dataset result:

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean query ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | `0.6025` | `0.7035` | `0.5904` | `0.5031` | `0.4134` | `1.9422` |
| `BM25+dense` | `0.6947` | `0.7817` | `0.6860` | `0.5984` | `0.5010` | `2.4125` |
| `BM25+SAE` | `0.7045` | `0.7947` | `0.6835` | `0.6036` | `0.5052` | `5.4446` |

This should guide the next implementation choice:

- BM25+SAE is a real semantic sparse signal and slightly beats BM25+dense on
  mean Recall@20, Recall@100, NDCG@10, and MAP@100.
- BM25+dense is much cheaper in the current Python full-scan matrix and
  slightly better on mean MRR@20.
- The value of SAE depends on the native sparse-impact implementation becoming
  materially cheaper than a separate dense retrieval layer at production scale.

## Updated Phase 5 Plan

Phase 5 should not continue tuning Python fusion weights. The current planning
record is:

```text
sae-phase5-gap-exploration-plan.md
sae-phase5-gap-exploration-report.md
scripts/research_sae_phase5_gap_exploration.py
```

The first unattended gap exploration completed the nine short targets:

1. BGE-M3/SPLADE feasibility probe.
2. Boundable score-contract comparison.
3. BM25+dense+SAE tri-hybrid comparison.
4. Query active-dimension budget sweep.
5. Document active-dimension budget sweep.
6. Corpus-specific unigram+bigram lexical vocabulary probe.
7. Seismic-style posting fanout and block-summary probe.
8. SINDI-style resident doc-vector/decode cost probe.
9. Real arxiv/pubmed/commons qrels readiness check.

The follow-up SPLADE baseline is recorded in:

```text
sae-splade-baseline-report.md
scripts/research_splade_baseline_matrix.py
results/sae/phase5/splade-baseline/
```

`naver/splade_v2_distil` is useful as a learned sparse reference, but it does
not beat BM25+SAE on the current five-dataset mean:

| Source | Recall@100 | MRR@20 | NDCG@10 |
| --- | ---: | ---: | ---: |
| `bm25_sae` | 0.7947 | 0.6835 | 0.6036 |
| `splade` | 0.7519 | 0.6324 | 0.5559 |
| `bm25_splade` | 0.7445 | 0.6402 | 0.5570 |
| `bm25_sae_splade` | 0.7926 | 0.6827 | 0.6087 |

Therefore SPLADE v2-distil should stay archived as an offline baseline for
now, not move into SQL sidecar or native payload integration.

The key result is that fixed source-level saturation is close to current
per-query normalization:

| Run | Recall@100 | MRR@20 | NDCG@10 |
| --- | ---: | ---: | ---: |
| `normalized` | 0.7947 | 0.6835 | 0.6036 |
| `raw` | 0.7470 | 0.6315 | 0.5457 |
| `fixed_saturation` | 0.7934 | 0.6742 | 0.5952 |

This changes the next implementation target. Phase 5 should first make the
native read-only path support the fixed-saturation contract and budgeted SAE
candidate generation, because this is the first scorer that is both practical
and compatible with exact native upper bounds.

The next implementation sequence is:

1. Implement fixed-saturation scoring in the native simulator and PostgreSQL
   read-only candidate path.
2. Add query top-16/top-32 budget modes and compare exact overlap against full
   BM25+SAE.
3. Add document top-48 or compressed resident doc-vector payload generation.
4. Improve impact-head candidate selectivity before mutable maintenance.
5. Build arxiv/pubmed/commons qrels before making production dense-removal
   claims.

The current BM25 path should remain untouched until BM25-only, SAE-only, and
BM25+SAE parity are all proven against the simulator and current BM25 APIs.
