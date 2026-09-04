# SAE Latent Sparse Retrieval Literature and Project Report

Date: 2026-05-06

## Executive Summary

The current best research direction is SAE over dense embeddings, but the
database-facing design should be a generic weighted sparse retrieval index.

SAE over dense embeddings is attractive because it can reuse the current dense
embedding model and vector pipeline. It does not require replacing the text
encoder with a SPLADE-style sparse text model. The operational shape is:

```text
text -> existing dense embedding -> SAE encoder -> sparse latent vector
```

This is lighter than introducing a full new retrieval encoder, but it still
requires model training, calibration, and empirical validation. The main
unknown is not whether the representation can be indexed; it can. The main
unknown is whether the sparse latent features preserve enough retrieval quality
and add enough complementary recall to justify native index work.

## Taxonomy

### BM25 Sparse

The current ii42 index stores lexical sparse evidence. It is excellent
for exact terms, entities, titles, and explainable lexical matches. It is weak
when query and document vocabulary diverge.

### Dense Vector

Dense vector indexes such as VectorChord provide high semantic recall but are
less transparent and less naturally field-aware. They also need ANN-specific
tradeoffs around probes, candidate windows, and filter behavior.

### Learned Sparse Text Retrieval

SPLADE, uniCOIL, DeepImpact, BGE-M3 sparse mode, and similar models map text
directly to sparse lexical or vocabulary-weighted vectors. They usually work
well with inverted indexes and are mature enough to use as baselines.

### SAE Latent Sparse Retrieval

SAE latent retrieval maps dense embeddings to sparse latent dimensions. The
dimensions are not necessarily vocabulary terms. They may be semantic concepts,
clusters, or learned features. This is less mature than SPLADE, but it fits our
existing embedding pipeline better.

## Paper and Project Notes

## Interpret and Control Dense Retrieval with Sparse Latent Features

Link: <https://arxiv.org/abs/2411.00786>

Core idea:

- Train sparse autoencoders over dense retriever embeddings.
- Use latent features to interpret and control dense retrieval behavior.
- Treat sparse latent activations as semantic features inside a dense retrieval
  model's representation space.

Why it matters:

- This is the closest paper to our preferred direction.
- It treats SAE as a layer over an existing dense retriever, not as a separate
  text encoder.
- It suggests a path where dense retrieval can be made more interpretable and
  controllable without discarding the dense model.

Implications for ii42:

- The index should not need to know text tokens.
- It should accept integer latent ids and weights.
- Query and document vectors can be produced outside PostgreSQL.
- The retrieval score is likely closer to sparse inner product than BM25.

Risks:

- Retrieval-oriented SAE training details matter.
- A reconstruction-only SAE may not produce good retrieval features.
- Feature interpretability does not automatically imply retrieval quality.

## Decoding Dense Embeddings / CL-SR

Link: <https://aclanthology.org/anthology-files/pdf/emnlp/2025.emnlp-main.1345.pdf>

Core idea:

- Use sparse autoencoders to decode dense embeddings into sparse concept-level
  representations.
- Use concept activations for retrieval and interpretation.
- Explore sparse concept retrieval as a bridge between dense models and
  inverted-index behavior.

Why it matters:

- This is directly aligned with "latent concept retrieval".
- It makes the retrieval unit a latent concept rather than a raw term.
- It suggests that concept-level sparse scoring can be evaluated with standard
  retrieval benchmarks.

Implications for ii42:

- We should evaluate concept idf, concept frequency, and concept norm.
- The scoring function may need BM25-like saturation even if the dimensions are
  not words.
- A concept browser or feature-labeling workflow may become useful later, but
  it is not required for the first index.

Risks:

- Concept ids may be model-specific and unstable across retraining.
- Query concept sparsity may differ heavily from document concept sparsity.
- Feature labels may not be reliable enough for user-facing explanation.

## Towards Monosemanticity

Link: <https://www.anthropic.com/research/towards-monosemanticity-decomposing-language-models-with-dictionary-learning>

Core idea:

- Sparse autoencoders can decompose neural activations into more interpretable
  features.
- Dictionary learning may expose features that are more monosemantic than raw
  activations.

Why it matters:

- It provides the conceptual foundation for SAE feature interpretability.
- It motivates why latent sparse dimensions might be meaningful.

Implications for ii42:

- SAE dimensions may be useful for debugging and explaining retrieval.
- The index can preserve feature ids and scores for downstream explanation.

Risks:

- This work targets model activations, not primarily dense embedding retrieval.
- Feature interpretability is a research property, not a production guarantee.

## SAELens

Link: <https://github.com/decoderesearch/SAELens>

Core idea:

- A mature open-source library for training and analyzing sparse autoencoders.
- Provides tooling around activation stores, SAE training, and feature
  analysis.

Why it matters:

- Good starting point for understanding SAE training mechanics.
- Useful for feature analysis and experiment reproducibility.

Implications for ii42:

- Useful as a research tool, not a production dependency for the extension.
- We can borrow training and evaluation patterns.

Risks:

- Oriented toward transformer activations more than dense embedding vectors.
- Production inference path may need a much smaller custom encoder.

## SAEBench

Link: <https://github.com/adamkarvonen/SAEBench>

Core idea:

- Benchmark and evaluation tooling for sparse autoencoders.

Why it matters:

- Helps avoid judging SAE quality only by reconstruction loss.
- Encourages feature-level and downstream evaluation.

Implications for ii42:

- We should define retrieval-specific SAE benchmarks instead of relying only on
  generic SAE metrics.
- Feature sparsity, stability, and downstream retrieval quality all matter.

## EleutherAI Sparsify

Link: <https://github.com/EleutherAI/sparsify>

Core idea:

- Tools for training and experimenting with sparse autoencoders.

Why it matters:

- Another practical SAE training reference.
- Useful for comparing TopK, JumpReLU, and related sparse activation designs.

Implications for ii42:

- TopK SAE is especially attractive because it gives a hard active-dimension
  bound, which is valuable for indexing.

## Neuronpedia

Link: <https://docs.neuronpedia.org/>

Core idea:

- Browsing, labeling, and analyzing sparse autoencoder features.

Why it matters:

- It shows what feature-level observability could look like.

Implications for ii42:

- A later product could expose "latent features responsible for this match".
- This should not block the first retrieval prototype.

## SPLADE

Paper: <https://arxiv.org/abs/2107.05720>

Repository: <https://github.com/naver/splade>

Core idea:

- A neural sparse retrieval model that expands and weights vocabulary terms.
- It keeps inverted-index compatibility while improving semantic matching.

Why it matters:

- SPLADE is the strongest baseline for learned sparse retrieval.
- It is closer to production retrieval than SAE latent sparse.

Implications for ii42:

- The same weighted sparse index could support SPLADE outputs.
- SPLADE can validate whether our sparse index design is general enough.

Risks:

- SPLADE requires a separate text encoder.
- Model inference may be heavier than applying a small SAE to an embedding we
  already produce.
- Vocabulary-level sparse vectors can be dense unless aggressively pruned.

## BGE-M3

Paper: <https://arxiv.org/abs/2402.03216>

Model card: <https://huggingface.co/BAAI/bge-m3>

Core idea:

- A multilingual, multi-function embedding model that supports dense retrieval,
  sparse lexical weights, and multi-vector retrieval.

Why it matters:

- It is highly relevant to multilingual policy and AI-search use cases.
- It can produce both dense and sparse signals from one model family.

Implications for ii42:

- Strong baseline for multilingual learned sparse retrieval.
- If the project later changes embedding models, BGE-M3 can be a candidate
  end-to-end embedding/sparse provider.

Risks:

- Adopting it may require changing or duplicating the current embedding model.
- Sparse lexical weights are not SAE latent concepts.
- Multi-vector mode introduces another retrieval family and should be separate
  from the first SAE experiment.

## SentenceTransformers SparseEncoder

Link: <https://www.sbert.net/docs/package_reference/sparse_encoder/model.html>

Core idea:

- Provides sparse encoder model support in a familiar Python ecosystem.

Why it matters:

- Practical path for running SPLADE-like baselines quickly.

Implications for ii42:

- Useful for the baseline phase.
- Not the preferred first integration path if we want to reuse current dense
  embeddings.

## OpenSearch Neural Sparse Search

Link: <https://docs.opensearch.org/docs/2.15/search-plugins/neural-sparse-search/>

Core idea:

- Production system support for neural sparse retrieval.

Why it matters:

- Shows how mature search systems expose sparse model inference and sparse
  retrieval.

Implications for ii42:

- Useful reference for SQL/API design and operational expectations.
- Confirms that neural sparse retrieval is production-relevant.

Risks:

- OpenSearch owns the whole search stack; PostgreSQL extension constraints are
  different.

## Qdrant Sparse Vectors

Link: <https://qdrant.tech/articles/sparse-vectors/>

Core idea:

- Vector database support for sparse vectors and hybrid sparse/dense search.

Why it matters:

- Shows a simple model-agnostic sparse vector abstraction:
  indices plus values.

Implications for ii42:

- The external API should probably accept sparse vectors as dimension/value
  arrays.
- The model should remain outside the database.

## pgvector sparsevec

Link: <https://github.com/pgvector/pgvector>

Core idea:

- pgvector supports a `sparsevec` type with vector operations.

Why it matters:

- Good reference for PostgreSQL type design.
- Useful for interop and maybe early SQL prototypes.

Implications for ii42:

- We may not need a custom sparse type at first.
- But top-k retrieval over sparse vectors still needs an inverted index, not
  only a type.

## Seismic

Paper: <https://arxiv.org/abs/2404.18812>

Repository: <https://github.com/TusKANNy/seismic>

Core idea:

- Efficient inverted indexes for approximate retrieval over learned sparse
  representations.

Why it matters:

- Learned sparse vectors have different distributions from classic term
  indexes.
- Seismic studies indexing specifically for learned sparse representations.

Implications for ii42:

- We should start exact, but Seismic is an important reference for the
  approximate/pruned version.
- It may inform clustering, block metadata, and pruning strategy.

Risks:

- Approximate learned sparse indexing is more complex than the first prototype
  should attempt.

## SINDI

Link: <https://arxiv.org/abs/2509.08395>

Core idea:

- Sparse approximate maximum inner product search.

Why it matters:

- SAE latent retrieval is likely a sparse MIPS problem.
- SINDI is relevant if exact inverted-list scoring is too slow.

Implications for ii42:

- Useful second-stage algorithm reference after exact correctness is proven.

## PISA

Link: <https://github.com/pisa-engine/pisa>

Core idea:

- Research search engine with modern inverted-index algorithms such as WAND
  and block-max variants.

Why it matters:

- The top-k retrieval problem over weighted sparse features will need similar
  pruning concepts.

Implications for ii42:

- WAND/BMW-style safe pruning should be studied before inventing a custom
  pruning model.

## Design Implications for ii42

## Model Boundary

The extension should not train or run SAE models. It should index already
materialized sparse vectors and query sparse vectors.

This keeps the database extension stable and avoids depending on Python or ML
runtimes in PostgreSQL.

## Data Type Boundary

The minimal input contract is:

```sql
dimensions integer[]
weights real[]
```

Optional metadata:

- model id;
- encoder version;
- source dense model id;
- top-k pruning policy;
- vector norm;
- feature labeling table.

## Scoring Boundary

Start with exact sparse dot product:

```text
sum(q_weight * d_weight)
```

Then evaluate:

- IDF-weighted dot product;
- normalized dot product;
- BM25-like saturation on document weights;
- field-aware sparse latent scoring.

## Index Boundary

Do not force SAE-specific storage. Store weighted postings:

```text
dim_id -> [(ctid, weight, optional field_id)]
```

This supports:

- SAE latent concepts;
- SPLADE vocabulary weights;
- BGE-M3 lexical weights;
- future custom sparse models.

## Integration with Existing Hybrid Fusion

The sparse latent candidate source should look like another hybrid source:

```text
BM25 candidates
vector candidates
sparse latent candidates
-> ii42_hybrid_fuse_candidates(...)
```

This matches the current direction of ii42: SQL-visible candidate scores
and database-side fusion.

## Main Open Problems

1. Training data: whether current stored embeddings are sufficient or original
   unquantized embeddings must be recomputed.
2. Active dimension budget: whether 32-64 active dimensions preserve useful
   recall.
3. Score calibration: whether latent sparse scores are stable enough to fuse.
4. Refresh policy: whether SAE retraining requires full reindexing.
5. Feature stability: whether latent ids stay semantically stable across
   versions.
6. Query latency: whether exact accumulation is fast enough before WAND-like
   pruning.
7. Explanation: whether latent features can be labeled well enough to show to
   users.

## Recommended Experimental Order

1. SAE over existing dense embeddings.
2. SPLADE/BGE-M3 sparse baselines.
3. SQL table prototype for exact sparse top-k.
4. Native exact inverted index prototype.
5. Impact-sorted or WAND-like pruning.
6. Seismic/SINDI-inspired approximate sparse MIPS only if needed.

## Preliminary Conclusion

SAE latent sparse retrieval is not yet the most mature retrieval method, but it
is the most strategically aligned with our current architecture. It should be
the first research focus. The implementation should remain a generic weighted
sparse index so that SPLADE, BGE-M3, and future sparse encoders can reuse the
same database layer.

## Design Exploration Closure: 2026-05-12

The later prototypes confirmed the most important systems lesson from this
survey: the database layer should be a generic weighted sparse-impact engine,
not an SAE-specific runtime.

The current design record is:

```text
sae-native-index-design-exploration-summary.md
```

The reference projects remain useful, but implementation should now prioritize
the concrete native payload and exact block-max traversal defined there.
