# M1542 Corpus-Route Posting Capacity Audit

## Decision

**Do not promote corpus routes as a dense-equivalent first-stage source.**

M1542 nevertheless produced the first major honest-access improvement after
M1541. At the same 15% raw candidate union, dual spherical route atoms improve
macro Recall@100 from `0.159931` to `0.723467` and macro NDCG@10 from
`0.166898` to `0.650569`. The route is a strong retrieval candidate source,
but it is not a faithful replacement for the complete dense neighborhood.

Commit under test: `2abc844f`.

ClearML task: `c7673d9fcfd74dbe9074361a225ecf74`.

## Experiment

- Frozen `BAAI/bge-base-en-v1.5` document/query vectors.
- Document-only spherical k-means, no qrels or query fitting.
- Global centroid-count rule: `ceil(document_count / 16)`.
- One or two route atoms per document.
- Exact 8% and 15% unique candidate unions.
- Every duplicate dual-route posting read counted.
- Frozen M1541 PCA active component and tail256 INT8 scorer.
- Exact M1520 NFCorpus, SciFact, and FiQA canaries.

## 15% Results

| Dataset | Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | Dense O@100 | Reads | Union |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | exact BGE | 0.458739 | 0.232774 | 0.386586 | 0.697777 | 0.812651 | 1.000000 | 1.0000x | 1.0000 |
| nfcorpus | single tail | 0.453620 | 0.220350 | 0.336461 | 0.695325 | 0.484133 | 0.663900 | 0.1503x | 0.1503 |
| nfcorpus | dual tail | 0.458297 | 0.225704 | 0.352677 | 0.703512 | 0.492528 | 0.643700 | 0.1730x | 0.1503 |
| scifact | exact BGE | 0.848783 | 0.812670 | 0.980000 | 0.822607 | 1.000000 | 1.000000 | 1.0000x | 1.0000 |
| scifact | single tail | 0.806099 | 0.778139 | 0.900000 | 0.794017 | 0.910000 | 0.727500 | 0.1500x | 0.1500 |
| scifact | dual tail | 0.815018 | 0.791433 | 0.915000 | 0.806833 | 0.915000 | 0.735500 | 0.1831x | 0.1500 |
| fiqa | exact BGE | 0.694897 | 0.641505 | 0.934806 | 0.752176 | 0.996667 | 1.000000 | 1.0000x | 1.0000 |
| fiqa | single tail | 0.679750 | 0.623180 | 0.888933 | 0.733659 | 0.937599 | 0.749700 | 0.1500x | 0.1500 |
| fiqa | dual tail | 0.678391 | 0.621368 | 0.902722 | 0.726338 | 0.945833 | 0.757000 | 0.1839x | 0.1500 |
| macro | exact BGE | 0.667473 | 0.562316 | 0.767131 | 0.757520 | 0.936439 | 1.000000 | 1.0000x | 1.0000 |
| macro | single tail | 0.646490 | 0.540556 | 0.708464 | 0.741000 | 0.777244 | 0.713700 | 0.1501x | 0.1501 |
| macro | dual tail | 0.650569 | 0.546168 | 0.723467 | 0.745561 | 0.784454 | 0.712067 | 0.1800x | 0.1501 |

Both predeclared gates are `0/3`. Dual assignment has no catastrophic row by
the 90% Recall/NDCG floor, but it misses the 95% Recall/CUB and 0.90 dense
O@100 requirements.

## What Changed Relative To M1541

| 15% macro source | NDCG@10 | Recall@100 | CUB | Dense O@100 | Reads |
| --- | ---: | ---: | ---: | ---: | ---: |
| coordinate max-edge + tail256 | 0.166898 | 0.159931 | 0.192809 | 0.281800 | 0.1501x |
| single corpus route + tail256 | 0.646490 | 0.708464 | 0.777244 | 0.713700 | 0.1501x |
| dual corpus route + tail256 | 0.650569 | 0.723467 | 0.784454 | 0.712067 | 0.1800x |

This is a structural gain, not a threshold adjustment. Candidate construction
that preserves neighborhoods is far more important than individual posting
impact. It validates the clustered-inverted-index idea while rejecting the
claim that 15% route probing is dense-equivalent.

## Scoring Diagnosis

The exact-dense upper and tail256 rows remain close:

- dual upper macro Recall@100: `0.727668`;
- dual tail256 macro Recall@100: `0.723467`;
- dual upper macro NDCG@10: `0.647860`;
- dual tail256 macro NDCG@10: `0.650569`.

The remaining deficit is almost entirely candidate admission, not the INT8
tail scorer. No larger sketch or output-head training is authorized.

## Interpretation

M1542 resolves an important ambiguity:

1. pooled dense geometry can be exposed through sparse route atoms with honest
   bounded access;
2. those atoms preserve retrieval usefulness much better than coordinate-edge
   truncation;
3. at 15% union they still discard too much of the dense neighborhood and its
   deeper relevant-document capacity;
4. therefore they cannot replace VectorChord/ANN as the first-stage dense
   equivalence mechanism.

The dual route reaches about 97.5% of exact-BGE NDCG and 94.3% of Recall with
15% unique candidates. That is useful enough for one separately framed
retrieval-complement audit, but not enough to override the failed first-stage
gate.

## Next Authorized Question

Do not tune centroid count, budget, assignments, or epochs. The next bounded
probe should ask whether the missing route candidates are lexical complements:

- keep dual route construction frozen;
- split a fixed 15% candidate budget between semantic route and reference
  BM25 candidates using one predeclared 10%/5% split;
- measure exact union CUB and exact-dense upper before any scorer work;
- use a fixed rank-only fusion for a non-trained lower bound;
- reject the route entirely if the lexical union does not exceed both dense
  and BM25 candidate capacity on at least two rows without a catastrophic row.

This is not a rescue of first-stage dense equivalence. It is a final capacity
test for a constrained retrieval source. Only a positive complement result
would justify a later native scorer or compiler experiment.

## Artifacts

- `runs/m1542a_corpus_route_posting_capacity_v1/summary.json`
- `runs/m1542a_corpus_route_posting_capacity_v1/summary.md`

