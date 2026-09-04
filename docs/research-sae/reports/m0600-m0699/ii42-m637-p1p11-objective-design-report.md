# M637 / P1.11 Objective Design

Status: `implemented_canary_probe`

M637 freezes document postings and trains only the query-side
generated posting compiler.  The objective combines qrels promotion
with dense-rank preservation so that retrieval gains cannot be bought
by destroying dense top-100 geometry.

This is a first-stage posting objective.  It uses no BM25, fixed
alpha, learned gate, document id, query id, or post-hoc reranker.
