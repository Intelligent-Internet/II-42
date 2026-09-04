# SAE Latent Sparse Retrieval Direction Report

Date: 2026-05-06

## Decision

Use SAE over dense embeddings as the first research direction, but design the
database-facing layer as a generic weighted sparse retrieval index.

This means:

- SAE is the preferred first model experiment.
- The PostgreSQL extension should not be SAE-specific.
- The index should accept sparse dimension ids and weights.
- SPLADE, BGE-M3 sparse weights, and other learned sparse models should remain
  compatible future inputs.

## Why SAE-First Is Reasonable

The current system already has dense embedding pipelines and vector indexes.
SAE over dense embeddings can reuse that path:

```text
query/document text
  -> current dense embedding model
  -> SAE encoder
  -> sparse latent activations
  -> sparse retrieval index
```

This is lighter than adopting a new text sparse encoder because it does not
force the product to change its embedding model. It also creates a potentially
interpretable semantic sparse signal that can sit between BM25 and dense vector
retrieval.

## What This Could Add

## Candidate Recall

SAE latent sparse retrieval may recover documents that BM25 misses because of
vocabulary mismatch, while remaining more discrete and controllable than dense
vector ANN.

## SQL-Friendly Semantics

Sparse latent candidates can be represented as SQL-visible ids and scores. That
fits the existing ii42 hybrid fusion model.

## Explainability

Latent dimensions may eventually be labeled or inspected. Even if labels are
not user-facing, feature ids and weights can help debug why a document matched.

## Memory and RAG

For AI memory/RAG retrieval, latent concepts could become a useful middle
layer:

- BM25 handles exact names, identifiers, terms, and titles.
- Dense vector handles broad semantic similarity.
- SAE sparse latent handles concept-level semantic evidence with inverted
  index behavior.

## Main Product Fit

The strongest near-term use cases are:

- arxiv and pubmed semantic candidate expansion;
- policy chunk retrieval where concepts matter and lexical phrasing varies;
- memory extraction where repeated semantic concepts should be easy to locate;
- reranker candidate widening without relying only on dense ANN.

## What This Should Not Promise Yet

SAE sparse retrieval should not be marketed as a dense-vector replacement in
the first phase.

It cannot yet promise:

- better quality than dense vector retrieval;
- stable human-readable concepts;
- cross-model portability;
- no retraining when the embedding model changes;
- production-grade latency before index experiments.

## Recommended Architecture

## Model Layer

The model layer stays outside PostgreSQL.

Responsibilities:

- produce dense embeddings;
- run SAE encoder;
- prune top-k active latents;
- write sparse latent vectors to PostgreSQL;
- encode query vectors at request time.

The first prototype can be Python-only.

## Storage Contract

Store sparse latent vectors as model output, not raw text:

```text
doc_id / ctid
model_id
encoder_version
dimension_ids[]
weights[]
norm
created_at
```

For the native index, the core posting representation should be:

```text
dim_id -> [(ctid, weight)]
```

Optional future fields:

- field id;
- section id;
- timestamp;
- source corpus id;
- quantized weight.

## Query Contract

The query API should accept sparse query dimensions directly:

```sql
ii42_sparse_query(
    index_name regclass,
    dimensions integer[],
    weights real[],
    k integer
)
```

Hybrid integration should be a candidate helper:

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

## Scoring Contract

Start simple:

```text
score = sum(q_weight * d_weight * idf(dim))
```

Why include IDF early:

- common latent dimensions may be broad and unhelpful;
- rare dimensions may carry stronger semantic evidence;
- this mirrors why BM25 works well for sparse lexical retrieval.

Later variants:

- no-idf dot product;
- document norm normalization;
- BM25-like saturation on document weights;
- field-aware sparse latent score;
- signed weights only if we prove non-negative activations are insufficient.

## Index Implementation Direction

## Stage 1: SQL Prototype

Use plain SQL tables to validate retrieval quality and data distribution:

```text
sparse_latent_postings(index_id, dim_id, ctid, weight)
sparse_latent_stats(index_id, dim_id, doc_freq, idf)
```

This lets us test scoring and fusion without changing the C access method.

## Stage 2: Native Exact Index

Implement a simple exact weighted sparse inverted index.

Properties:

- exact score accumulation;
- top-k heap;
- MVCC-safe ctid visibility handling;
- eventually consistent maintenance, consistent with the current extension;
- no approximate pruning until correctness is boring.

## Stage 3: Pruned Index

Add acceleration only after exact behavior is validated:

- impact-sorted lists;
- block max metadata;
- WAND/BMW-style safe pruning;
- Seismic/SINDI-inspired approximate sparse MIPS if exact top-k is too slow.

## Relationship to Existing ii42 Work

This direction fits current ii42 principles:

- PostgreSQL-native retrieval primitive;
- SQL-visible candidates and scores;
- no hard dependency on vector extensions;
- hybrid fusion stays explicit;
- background maintenance and preload can reuse existing operational patterns.

It also fits the BM25v exploration conceptually: both are about multiple
candidate sources and bounded top-k scoring. The difference is that SAE sparse
retrieval introduces a new learned sparse source rather than a different
fusion policy over existing sources.

## First Prototype Recommendation

Build `scripts/research_sae_latent_sparse_retrieval.py` later, not now, with
the following flow:

1. load dense embeddings for a small corpus sample;
2. train a TopK SAE;
3. export top-k latent activations for documents and queries;
4. compute exact sparse dot/idf scores;
5. compare to BM25 and dense vector candidates;
6. write per-query failure examples;
7. produce an index-size estimate.

The first prototype should be offline and file-based. It should not require a
PostgreSQL extension change.

## Evaluation Gates

Continue to native index design only if:

- SAE sparse Recall@1000 is close enough to dense vector retrieval;
- SAE candidates add non-trivial unique relevant documents beyond BM25 and
  dense vector;
- query active dimensions can be capped under a practical budget;
- document active dimensions can be capped without destroying recall;
- score normalization is stable across arxiv, pubmed, and policy;
- retraining/reindexing cost is operationally acceptable.

Recommended initial thresholds:

- Query active dims: 16-64.
- Document active dims: 32-128.
- Dense recall retention: at least 80% before fusion, higher if possible.
- Unique useful candidates: at least one clear workload bucket where SAE
  improves reranker input quality.
- Index expansion: no more than a few times the BM25 posting footprint for the
  first production candidate.

## Risks and Mitigations

## Risk: SAE Reconstruction Does Not Equal Retrieval Quality

Mitigation:

- evaluate retrieval metrics directly;
- try retrieval-oriented training objectives if reconstruction-only SAE is too
  weak;
- keep SPLADE/BGE-M3 as strong baselines.

## Risk: Latent Dimensions Are Too Dense

Mitigation:

- use TopK SAE;
- prune document activations aggressively;
- measure recall loss at every active-dimension budget.

## Risk: Scores Are Hard to Fuse

Mitigation:

- start with per-source normalization inside hybrid fusion;
- compare dot, idf-dot, and normalized dot;
- expose debug fields for score calibration.

## Risk: Embedding Model Changes Force Reindexing

Mitigation:

- version every latent index by dense model and SAE model;
- treat model changes as full reindex events;
- avoid promising cross-model compatibility.

## Risk: No Advantage Over Existing Vector Search

Mitigation:

- define complementarity metrics, not only standalone nDCG;
- evaluate reranker input hit rate and final answer support quality;
- stop native index work if it only duplicates vector candidates.

## Directional Roadmap

## Milestone A: Research Harness

Deliverables:

- Python offline SAE training and retrieval harness.
- Data export path for dense embeddings.
- Exact sparse retrieval evaluator.
- Baseline comparison against BM25 and dense vector.

## Milestone B: SQL Prototype

Deliverables:

- SQL postings table prototype.
- IDF and dot-product scoring.
- Hybrid candidate integration through SQL.
- Latency and storage estimate.

## Milestone C: Native Index Design

Deliverables:

- access method design document;
- on-disk posting format;
- exact top-k algorithm;
- MVCC and maintenance plan;
- benchmark plan.

## Milestone D: Native Prototype

Deliverables:

- exact weighted sparse query function;
- insert/update/delete maintenance;
- correctness tests;
- BEIR and commons-style benchmark.

## Final Recommendation

Proceed SAE-first, but keep the database layer generic.

The likely winning product shape is not "SAE replaces BM25/vector". It is:

```text
BM25 lexical candidates
  + dense vector candidates
  + SAE latent sparse candidates
  -> SQL-visible hybrid fusion
  -> optional AI reranker
```

This aligns with ii42 as a PostgreSQL-native retrieval and fusion
extension. The first implementation step should be an offline SAE retrieval
harness, not C index work.

## Design Exploration Closure: 2026-05-12

This early recommendation has now been refined by the follow-up prototypes.
The offline harness, SQL sidecar, unified sparse scorer, candidate-budget
training, and block-max simulator have all been explored far enough to define
the next native index target.

The current target is no longer a third late-fusion candidate source. It is a
generic sparse-impact native index where BM25 and SAE impacts share one
dimension namespace and one exact block-max top-k traversal.

The current decision record is:

```text
sae-native-index-design-exploration-summary.md
sae-pre-sql-unified-exploration-report.md
```
