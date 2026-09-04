# M1720 Lexical-Quotient Residual Posting Contract

## Objective

M1520-M1710 establish two facts that must be kept separate:

1. exact lexical postings are already a selective and efficient publisher;
2. copying all dense information into another posting vocabulary repeatedly
   recreates high document frequency, global traversal, or weak admission.

M1720 therefore tests a narrower representation hypothesis:

> Publish lexical evidence once through frozen BM25 terms, and spend semantic
> posting capacity only on dense evidence that lexical features cannot
> explain.

The proposed product remains one encoder family, one physical inverted index,
and one additive posting accumulator. Lexical keys and residual semantic keys
occupy separate namespaces in that index. ANN search, a forward dense vector,
exact-dense reranking, qrels-selected routes, and dataset-specific thresholds
are outside the product boundary.

M1720 is not a continuation of SPLADE loss search. It starts with a
deterministic representation audit. No output head or text encoder may be
trained until the residual representation proves that it is more selective
than the raw dense source at the same retrieval budget.

## Research Basis

The design combines a known retrieval objective with a new factorization
hypothesis.

- [CLEAR](https://arxiv.org/abs/2004.13969) trains a semantic branch on the
  residual mistakes of a lexical retriever. It supports the complementarity
  objective, but its runtime is BM25 plus a dense index rather than one sparse
  index.
- [DrBoost](https://arxiv.org/abs/2112.07771) trains later compact components
  on mistakes left by the current retriever. It supports staged residual
  learning rather than asking one representation to relearn solved evidence.
- [RepCONC](https://arxiv.org/abs/2110.05789) shows that balanced discrete
  assignment must be part of representation learning, not an afterthought.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) shows why average activation is
  an insufficient production cost proxy: a small number of high-DF keys can
  dominate inverted-index traversal.
- M1520 defines the relevant product metric as recovery of
  `relevant intersect (dense minus BM25)`, not semantic-only score.
- M1540, M1541, M1542, M1566, M1691, and M1700 show that copying broad dense
  or expansion evidence into postings can improve quality while violating
  exact touch or DF limits.
- M1710 shows that reconstruction improvement does not imply top-k
  preservation under an additive posting score.

The new hypothesis is not claimed by those papers. M1720 proposes learning a
corpus-only dense subspace whose directions are predictable from hashed
TF-IDF features, then removing that subspace from both document and query
dense vectors:

```text
U_lex = left singular directions of covariance(dense_doc, tfidf_doc)
z_lex = U_lex U_lex^T z
z_res = (I - U_lex U_lex^T) z
```

Because `U_lex` is orthonormal, dense score decomposes exactly:

```text
dot(q, d) = dot(q_lex, d_lex) + dot(q_res, d_res)
```

BM25 is not identical to `dot(q_lex, d_lex)`. The projection is only a
qrels-free diagnostic for dense directions predictable from lexical content.
The experiment must therefore measure retrieval complementarity directly; a
low reconstruction error alone cannot pass.

## Frozen Surface

M1720A reuses the locked M1541/M1542 canary:

- dense root: `BAAI/bge-base-en-v1.5`;
- datasets: NFCorpus, SciFact, and FiQA;
- corpus and queries: the existing BEIR15 shared root;
- BM25 rankings and qrels: the frozen M1520 references;
- candidate depth: 1,000;
- semantic route budgets: 8% and 15% of corpus documents;
- route target cell size: 16 documents;
- one semantic route assignment per document;
- lexical hashing: 2,048 dimensions;
- lexical quotient rank: 128;
- seed: 1720.

Corpus text may fit IDF statistics, the lexical covariance basis, and route
centroids. Query text, qrels, BM25 scores, dataset identity, and retrieval
labels may not choose the basis, rank, route count, budget, or checkpoint.
Qrels are evaluation-only. BM25 rankings are used only to define the already
solved lexical candidate set and the residual recovery metric.

## M1720A: Representation Audit

M1720A does not train a model. For each corpus it compares two sources under
identical route construction and traversal budgets:

- `raw_dense_route`: spherical corpus routes over the normalized BGE vector;
- `lexical_quotient_route`: spherical routes over `z_res`.

Each route is scored twice:

- direct centroid-posting score, as the deployable additive approximation;
- exact source-vector score over the same touched set, as a non-product
  admission upper bound.

The BM25 candidate set is then unioned with each semantic touched set for
capacity measurements only. No alpha, learned gate, qrels, or dense reranker
may alter that set.

### Structural Measurements

M1720A records:

- lexical cross-covariance energy captured by the fixed rank;
- dense energy in lexical and residual components;
- raw and residual effective rank;
- raw and residual spherical-cluster cosine;
- exact score-decomposition maximum error;
- semantic posting edges per document;
- maximum and p99 semantic-key DF;
- mean and p95 posting reads divided by corpus documents;
- touched-document ratio and route probes.

### Retrieval Measurements

For each query and source, M1720A records:

- dense top100 and top1000 admission recall;
- qrels-free recovery of `dense_top1000 minus BM25_top1000`;
- qrels-based recovery of relevant dense-recoverable BM25 misses;
- BM25 plus semantic unified candidate upper bound;
- direct and exact-upper NDCG@10, MAP@100, Recall@100, and MRR@20.

## Gates

The gates are conjunctive and predeclared.

### Evaluator Integrity

- the exact orthogonal score decomposition error is at most `1e-5`;
- the raw exact-dense control has overlap at 100 of at least `0.999` when it
  scans the full corpus;
- input identities and the complete configuration are written to JSON.

Any failure stops the experiment as an evaluator failure.

### Residual Structure Gate

The lexical quotient is structurally useful only if, on at least two of the
three rows:

- residual effective rank is at most `0.90` of raw dense effective rank, or
  residual route mean cluster cosine exceeds raw by at least `0.02`; and
- the 15% residual route maximum DF is at most `0.02`;
- mean semantic posting reads are at most `0.30N`.

This gate does not use qrels.

### Complementarity Gate

At the fixed 15% budget, at least two of three rows must satisfy all of:

- qrels-free dense-minus-BM25 recovery is no lower than the raw dense route;
- relevant dense-recoverable BM25-miss recovery is at least `0.90`;
- unified candidate upper bound is at least `0.97` of the stronger BM25 or
  dense reference;
- no row loses more than `0.02` unified candidate upper bound relative to the
  raw dense route at the same budget.

The macro residual route must also improve relevant BM25-miss recovery or
semantic read cost over the raw route; equality everywhere is not a new
representation result.

## Follow-On Decisions

- If structural and complementarity gates pass, authorize M1720B: fit one
  balanced residual codebook on a disjoint MS MARCO corpus, freeze it, and
  repeat the same canary without per-corpus semantic routes.
- If only the exact residual upper passes, the residual geometry has capacity
  but the route publisher fails. M1720B may test a balanced residual codebook,
  but no text encoder training is authorized.
- If residual geometry is not more compressible than raw dense, stop the
  lexical-quotient projection. Do not sweep rank, hash size, seed, or loss.
- If M1720B passes, authorize M1720C: freeze the dense trunk and train only a
  query-independent document/query residual-code compiler to reproduce the
  deterministic code assignment and impact.
- Retrieval training is allowed only after compiler equivalence. It must use
  a CLEAR-style lexical-error objective plus DF/load constraints and preserve
  the frozen BM25 publisher.

## Final Product Gate

The route may become a product candidate only after one unchanged artifact
passes native shared15 and official evaluation through the existing II42
index lifecycle with:

- one physical posting index and one query accumulator;
- no external ANN or forward-vector reranker;
- at least 90% recovery of dense-recoverable BM25 misses;
- unified CUB at least 97% of the stronger reference per locked gate;
- semantic mean reads at most 30% of corpus documents;
- maximum semantic-key DF at most 2%;
- no dataset-specific tuning or qrels-dependent source selection.

M1720 must end with a conclusion-bearing report. A local positive signal,
oracle upper bound, or canary-only gain cannot be promoted as the default.
