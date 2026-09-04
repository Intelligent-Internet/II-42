# SAE Latent Sparse Retrieval Research Plan

Date: 2026-05-06

## Purpose

This plan explores whether ii42 should support a new sparse retrieval
family based on Sparse Autoencoders (SAEs) over existing dense embeddings.
The first research phase intentionally leans toward SAE and latent-concept
retrieval because that path can reuse the current dense embedding pipeline
while producing an inverted-index-friendly sparse representation.

The index direction should remain model-agnostic: the extension should think
in terms of weighted sparse dimensions, not in terms of one specific SAE,
SPLADE, or BGE-M3 model.

## Core Hypothesis

Existing text embedding models already encode useful semantic structure. If a
small SAE can project those dense vectors into a sparse latent concept space,
then ii42 may be able to add a third retrieval signal:

```text
text
  -> existing dense embedding model
  -> dense vector
  -> SAE encoder
  -> top-k latent activations
  -> weighted sparse inverted index
```

This could provide a candidate source that is:

- more semantic than BM25;
- more indexable and explainable than dense ANN;
- compatible with SQL-side filtering and hybrid fusion;
- cheaper to scan than dense vector search when active dimensions are small.

## Research Questions

1. Does SAE latent sparse retrieval preserve enough of the dense embedding
   model's retrieval quality?
2. Does it add recall beyond BM25 and dense vector candidates on arxiv,
   pubmed, and policy-style data?
3. Can latent sparse scores be calibrated with BM25 and vector scores inside
   the existing hybrid fusion layer?
4. What active-dimension budget is practical for document and query vectors?
5. Does IDF or BM25-like saturation improve latent concept retrieval?
6. Can the representation be maintained incrementally inside PostgreSQL?
7. Is a native ii42 sparse-latent index justified, or should this remain
   a sidecar experimental feature?

## Reference Scope

### SAE and Latent Concept Retrieval

- [Interpret and Control Dense Retrieval with Sparse Latent Features](https://arxiv.org/abs/2411.00786)
- [Decoding Dense Embeddings: Sparse Autoencoders for Interpreting and Discretizing Dense Retrieval](https://aclanthology.org/anthology-files/pdf/emnlp/2025.emnlp-main.1345.pdf)
- [Towards Monosemanticity](https://www.anthropic.com/research/towards-monosemanticity-decomposing-language-models-with-dictionary-learning)
- [SAELens](https://github.com/decoderesearch/SAELens)
- [SAEBench](https://github.com/adamkarvonen/SAEBench)
- [EleutherAI Sparsify](https://github.com/EleutherAI/sparsify)
- [Neuronpedia](https://docs.neuronpedia.org/)

### Learned Sparse Retrieval Baselines

- [SPLADE](https://arxiv.org/abs/2107.05720)
- [SPLADE repository](https://github.com/naver/splade)
- [BGE-M3](https://arxiv.org/abs/2402.03216)
- [BGE-M3 model card](https://huggingface.co/BAAI/bge-m3)
- [SentenceTransformers SparseEncoder](https://www.sbert.net/docs/package_reference/sparse_encoder/model.html)
- [OpenSearch Neural Sparse Search](https://docs.opensearch.org/docs/2.15/search-plugins/neural-sparse-search/)

### Sparse Vector Indexing and Systems

- [Seismic](https://github.com/TusKANNy/seismic)
- [Efficient Inverted Indexes for Approximate Retrieval over Learned Sparse Representations](https://arxiv.org/abs/2404.18812)
- [SINDI](https://arxiv.org/abs/2509.08395)
- [VSAG](https://github.com/antgroup/vsag)
- [Qdrant Sparse Vectors](https://qdrant.tech/articles/sparse-vectors/)
- [pgvector sparsevec](https://github.com/pgvector/pgvector)

## Phase 0: Terminology and Data Contract

Define the common data shape that every candidate model must produce:

```text
SparseLatentVector {
    dimensions: int32[]
    weights: float32[]
    original_dimension_count: int32
    model_id: text
    encoder_version: text
    norm: float32
}
```

Open design points:

- Dimensions may be vocabulary ids, latent ids, or model-specific sparse ids.
- Weights must be non-negative for the first prototype.
- Vectors must be sorted by dimension id.
- Top-k pruning must be explicit and deterministic.
- Empty vectors are valid but should not be indexed.

## Phase 1: Offline SAE Prototype

Goal: prove or falsify the SAE-first direction before touching PostgreSQL C
code.

Tasks:

- Select one current dense embedding model as the initial target.
- Export or recompute a representative training sample from arxiv, pubmed, and
  policy chunks.
- Train a small TopK or gated SAE over dense embeddings.
- Produce document sparse latent vectors at several active-dimension budgets:
  16, 24, 32, 64, 128.
- Encode queries through the same dense model and SAE.
- Score with plain dot product and IDF-weighted dot product.
- Compare against BM25, dense vector, and existing hybrid results.

Success criteria:

- Top-k candidates are stable across repeated runs.
- SAE sparse retrieval recovers at least 80-90% of dense vector Recall@1000 on
  generic semantic queries.
- It improves recall on at least one failure bucket where BM25 misses and
  vector candidates are weak or too broad.
- Average document active dimensions stay under 64 for the efficient profile.

## Phase 2: Strong Baselines

Goal: prevent SAE enthusiasm from hiding simpler options.

Tasks:

- Run SPLADE or SentenceTransformers SparseEncoder on the same text samples.
- Run BGE-M3 sparse lexical weights on the same samples if practical.
- Compare model inference cost, sparse-vector density, index size, and
  retrieval quality.
- Treat these as baselines, not the preferred integration path.

Decision rule:

- If BGE-M3/SPLADE beats SAE by a wide margin and is operationally acceptable,
  keep SAE as research and implement a generic weighted sparse index first.
- If SAE is close to learned sparse text encoders while using the existing
  dense embedding model, continue SAE-first.

## Phase 3: PostgreSQL-Side Simulation

Goal: approximate extension behavior using SQL tables before writing a new
access method.

Prototype schema:

```sql
CREATE TABLE sparse_latent_postings (
    index_id oid NOT NULL,
    dim_id integer NOT NULL,
    doc_tid tid NOT NULL,
    weight real NOT NULL,
    PRIMARY KEY (index_id, dim_id, doc_tid)
);

CREATE TABLE sparse_latent_stats (
    index_id oid NOT NULL,
    dim_id integer NOT NULL,
    doc_freq integer NOT NULL,
    idf real NOT NULL,
    PRIMARY KEY (index_id, dim_id)
);
```

Prototype scoring:

```sql
score(doc) = sum(q_weight * d_weight * idf(dim))
```

Optional later scoring:

```sql
score(doc) =
    sum(q_tf_sat * d_tf_sat * idf(dim)) / doc_latent_norm
```

Success criteria:

- A SQL prototype can produce top-k candidates with acceptable latency on a
  medium corpus subset.
- Scores can be normalized and fused through the existing hybrid fusion API.
- Field-aware variants can be represented without changing the model contract.

## Phase 4: Native Index Design

Only start this phase after the offline and SQL prototypes justify it.

Candidate implementation path:

1. Start exact and simple: dimension-sorted inverted lists with weights.
2. Add top-k accumulation with a bounded heap.
3. Add impact-sorted postings or WAND-like pruning.
4. Add block max metadata for safe pruning.
5. Evaluate learned-sparse-specific approximations inspired by Seismic/SINDI.
6. Integrate eventual maintenance and shared preload patterns from the current
   ii42 design.

Native SQL surface sketch:

```sql
ii42_sparse_latent_query(
    index_name regclass,
    dimensions integer[],
    weights real[],
    k integer
)
```

Potential hybrid helper:

```sql
ii42_hybrid_sparse_latent_candidates(
    source_name text,
    index_name regclass,
    dimensions integer[],
    weights real[],
    weight double precision,
    candidate_k integer,
    normalizer text
)
```

## Evaluation Matrix

Datasets:

- BEIR SciFact and SciDocs for continuity with existing benchmark work.
- arxiv document-level search.
- pubmed document-level search.
- policy chunk and document search.
- Synthetic adversarial queries for exact entity names, abbreviations,
  multilingual terms, and synonym-only semantic matches.

Metrics:

- Recall@20, Recall@100, Recall@1000.
- nDCG@10 where labels exist.
- Reranker input hit rate.
- Answer-support citation quality for RAG.
- Query latency p50/p95.
- Index size per million rows.
- Active dimensions per document and per query.
- Incremental update cost.

Failure buckets:

- BM25 fails because of vocabulary mismatch.
- Dense vector returns broad semantic neighbors but misses exact entities.
- SAE/unified sparse misses low lexical-overlap semantic neighbors because the
  active latent budget is too small or the SAE was trained on too little
  domain data.
- Legal/policy shell documents pollute results.
- Query includes rare identifiers, equations, acronyms, or names.
- Multilingual or cross-lingual queries.

## Current Progress Update: 2026-05-10

Implemented research harness:

- BEIR preparation with Snowflake embeddings.
- TopK SAE training on local Mac MPS.
- SAE sparse retrieval and score-mode sweeps.
- Hybrid dense/BM25/SAE evaluator.
- Unified `BM25 + SAE` sparse impact evaluator.
- Unified SQL sidecar with single postings table and Python parity checks.
- Dense-vs-unified gap analysis.

Validated configurations:

| dataset | documents | queries | profile | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| SciFact | `400` | `40` | `BM25 + SAE32` | `0.9750` | `0.9750` | `0.9297` |
| SciDocs | `2000` | `100` | `BM25 + SAE32` | `0.4885` | `0.6470` | `0.6280` |
| SciDocs | `2000` | `100` | `BM25 + SAE64` | `0.5035` | `0.6775` | `0.6654` |
| SciDocs | `2000` | `100` | `BM25 + SAE128` | `0.5280` | `0.6750` | `0.6629` |

SciDocs dense baseline:

```text
Recall@20 = 0.5195
Recall@100 = 0.6740
MRR@20 = 0.6544
```

Key observation:

- SAE32 proves the direction but is not enough.
- SAE64 is a strong systems profile and nearly matches or exceeds dense on
  important metrics.
- SAE128 is a quality profile that beat dense on the current SciDocs slice,
  at the cost of `4x` SAE postings compared with SAE32.

Updated hypothesis:

> SAE-over-Snowflake can be competitive with dense retrieval when active
> dimensions are high enough. The core research problem is now the quality/cost
> frontier of active dimensions, not basic feasibility.

Updated next experiments:

1. Add field-aware BM25 dimensions for title/abstract-like corpora.
2. Run arxiv semantic queries with SAE64 and SAE128.
3. Train on larger domain samples instead of the small qrels slice.
4. Compare `input_dim=256` against full Snowflake `768` dimensions.
5. Add a learned calibration or source-gating layer instead of fixed
   `sae_weight`.
6. Keep BGE-M3/SPLADE as baselines, but do not switch the main prototype until
   they prove a decisive quality/cost advantage.

## First Milestone

Build a Python research harness that:

1. loads a sample of existing dense embeddings;
2. trains a small SAE;
3. exports top-k latent activations;
4. runs exact sparse dot-product retrieval;
5. compares against BM25 and dense vector candidate files;
6. writes a report with recall, latency, index-size, and failure examples.

The milestone should not require PostgreSQL extension changes.

## Early Decision Gates

Continue SAE-first if:

- SAE sparse retrieval is close to dense vector recall at a practical active
  dimension budget;
- SAE candidates are complementary to BM25 and vector candidates;
- training and inference can be made stable enough for our data refresh cycle;
- score normalization is understandable enough to expose in SQL.

Stop SAE-first if:

- sparse latent retrieval is consistently worse than BGE-M3/SPLADE;
- active dimensions are too dense for efficient inverted indexing;
- SAE must be retrained too often for our operational model;
- latent scores are too unstable to calibrate.

## Recommended Posture

Use SAE as the first research focus. Keep the extension design generic enough
to support SPLADE/BGE-M3-style sparse vectors later. This gives us the best
chance to reuse the current embedding pipeline while avoiding a model-specific
dead end.

## Design Exploration Closure: 2026-05-12

The first research loop has reached its intended design output. The current
implementation-facing closure is:

```text
sae-native-index-design-exploration-summary.md
unified-sparse-block-max-native-index-design.md
```

The posture above remains correct, but the concrete target has narrowed:

```text
generic sparse-impact native payload
+ BM25 and SAE source adapters
+ exact block-max traversal
+ immutable generation plus exact delta overlay
```

Future model work should now be judged by whether it improves the native
payload's quality/cost frontier, not by isolated SAE reconstruction quality.
