# M1540-M1545 Recall Breakthrough Review

## Executive Conclusion

The investigation did not produce a new recall winner, but it did produce a
high-confidence architectural conclusion and stopped an unjustified training
loop.

**Dense-equivalent recall and low-touch unified postings are not jointly
supported by the tested pooled-dense/latent-term constructions.** The best
honest posting approximation is M1542 dual corpus routing: about 94.3% of
exact-BGE Recall and 97.5% of NDCG at 15% unique candidates. M1545 raises
ranking quality to nearly dense at about 19.3% candidates, but still does not
exceed dense candidate capacity or Recall.

The practical recall route is therefore a hybrid physical index: lexical
postings plus an ANN/vector access path under one native index lifecycle.

## What Was Actually Learned

### Representation is not the main failure

M1541's full-union historical row proves that signed PCA coordinates plus a
256-dimensional INT8 tail sketch can reproduce exact-BGE ranking:

| Macro row | NDCG@10 | Recall@100 | Dense O@100 |
| --- | ---: | ---: | ---: |
| exact BGE | 0.667473 | 0.767131 | 1.000000 |
| full-union select8% + tail256 | 0.667459 | 0.766407 | 0.921333 |

The failure is candidate access: that row reads 13.85 posting entries per
corpus document and unions the whole corpus before its 8% selection.

### Candidate geometry matters more than edge strength

At the same honest 15% union:

| Source | NDCG@10 | Recall@100 | Dense O@100 |
| --- | ---: | ---: | ---: |
| coordinate max-edge | 0.166898 | 0.159931 | 0.281800 |
| corpus route single | 0.646490 | 0.708464 | 0.713700 |
| corpus route dual | 0.650569 | 0.723467 | 0.712067 |
| coordinate clustered blocks | 0.629322 | 0.688932 | 0.704533 |

Dense neighbors are selected by neighborhood geometry and accumulated moderate
evidence, not the largest independent coordinate impacts.

### The scorer is already adequate after admission

Across M1541-M1545, exact-dense upper and tail256 differ little after they
receive the same candidate set. The final M1545 macro rows are:

| Scorer over fixed union | NDCG@10 | MAP@100 | Recall@100 |
| --- | ---: | ---: | ---: |
| tail256 INT8 | 0.669063 | 0.562879 | 0.759776 |
| exact dense | 0.669331 | 0.562849 | 0.760748 |

Training a deeper scorer cannot create missing candidates.

### Lexical rescue is row-dependent

SciFact benefits from semantic/lexical union, while NFCorpus and FiQA mostly
lose semantic coverage when lexical candidates consume a fixed budget. The
additive 20% policy removes the substitution problem but still fails to exceed
equal-budget dense CUB on every row. This reproduces the historical
action-source observability bottleneck instead of solving it.

## Literature Alignment

- [Latent Terms](https://arxiv.org/html/2605.29384v1) supports the finding that
  frozen retriever states contain useful BM25-ready semantic features. M1540
  confirms the source signal but exposes its exact fanout cost on our canaries.
- [Seismic](https://arxiv.org/html/2404.18812v1) motivates clustered block
  summaries for sparse retrieval. M1544 shows that a direct coordinate-block
  adaptation is neither compact nor dense-equivalent on this representation.
- [Why Advanced Encoders Lag on Sparse Retrieval](https://arxiv.org/html/2607.00004v1)
  emphasizes vocabulary and normalization compatibility. Our results agree
  that changing the access vocabulary is structural; scorer/loss tuning alone
  cannot repair an incompatible candidate source.

## Stop Conditions Reached

The following are now rejected as next steps:

- more SAE reconstruction epochs or latent dimensions;
- learned projection/output heads over the same pooled source;
- more coordinate prefix, route-count, centroid-count, or budget grids;
- fixed alpha/source allocation search;
- selector/gate training before candidate capacity exists;
- listwise reranking of M1543/M1545 unions.

## Recommended Next Milestone

Define the next milestone as an engineering and recall milestone, not another
posting-equivalence experiment:

> Build one native hybrid index lifecycle containing BM25 postings and a
> VectorChord ANN field, run a fixed global candidate union and auditable
> scorer, and produce the official held-out recall matrix.

Acceptance should require:

- native DB/plugin execution only;
- held-out official rows, with training/evaluation provenance explicit;
- candidate capacity reported at equal budget;
- Recall@100 and MAP@100 above both dense and BM25;
- no material NDCG@10 or MRR@20 regression;
- latency, index size, and candidates scored reported beside quality.

This changes the physical access architecture while retaining the project's
core unified lifecycle and auditable scoring goals. It is the only next step
consistent with all M1540-M1545 evidence.

