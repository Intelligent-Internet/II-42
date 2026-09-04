# M636 / P1.10 Objective Design

Status: `implemented_smoke_probe`

M636 changes the M635 boundary from dense target fitting to retrieval
constrained generated posting.  The model still receives only frozen
dense-root embeddings and emits generated posting vectors.  It is not
a frozen-row reranker and does not use BM25, fixed alpha, dataset ids,
query ids, or document ids as model inputs.

Training uses query-level qrels splits only for retrieval constraints.
Dense faithfulness remains a guard through support cosine, KL, active
membership, and dense overlap metrics.
