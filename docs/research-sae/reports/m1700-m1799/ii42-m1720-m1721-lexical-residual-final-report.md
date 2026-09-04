# M1720-M1721 Lexical-Residual Posting Final Report

Date: 2026-07-12

Final decision: **the lexical-residual insight is valid as an objective, but
both tested post-hoc representation forms are closed**.

## Executive Result

The research question was whether BM25's already efficient lexical evidence
could be removed from the semantic representation, leaving a cheaper posting
encoder that publishes only dense evidence BM25 cannot express.

Two natural and independently falsifiable definitions were tested:

1. **Embedding quotient:** remove dense directions predictable from corpus
   TF-IDF features, then index the orthogonal residual.
2. **Score residual:** factorize the qrels-free positive deficiency
   `relu(dense rank signal - BM25 rank signal)`.

Neither is a deployable source.

| Stage | Main positive evidence | Decisive failure | Decision |
| --- | --- | --- | --- |
| M1720A embedding quotient | exact decomposition, O@100 1.0 | effective rank and max DF increase; recovery 0.7406 -> 0.4211 | stop projection |
| M1721 free score factor | score cosine 0.84-0.88 at rank 32 | target O@100 only 0.67-0.70; recovery 0.23-0.60 | stop compact teacher |
| M1721 document projection | score cosine 0.82-0.91 | candidate recovery 0.14-0.64 | stop document factor |
| M1721 LODO transform | full linear map has some signal | cosine 0.18-0.45; recovery 0.14-0.37 | stop shared transform |
| M1721 sparse TopK 4 | one physical dot product is possible | max DF 0.277-0.597 | no posting source |

## What The Data Says

### Lexical complement is not the easy part of dense geometry

The lexical-correlated component carries about 70% to 74% of dense energy and
is more clusterable than the remaining component. Removing it leaves a
diffuse residual whose effective rank increases from roughly `193–217` to
`248–260`. Complementarity and compressibility are different properties.

### The useful residual is relational

The positive dense-over-BM25 deficit depends on the query, document, corpus
competition, and rank boundary. A free score field can partially fit it, but
one transform does not transfer across corpora. This matches the earlier
M1502, M1561/M1562, and M1600 observability failures: an oracle action does not
become a stable posting key merely because it is factorized.

### Ranking error is not reconstruction error

M1720 reconstructs the full dense score to `1e-7`, and M1721 document
projection reaches high score cosine, yet both lose candidate membership.
This repeats M600/M636, M1530, and M1710: KL, cosine, or vector reconstruction
cannot substitute for top-k and exact posting gates.

### The cost problem remains global DF

The residual dimensions do not become selective. M1720 route max DF rises to
2.9%–6.6%; M1721 TopK-4 factors reach 27.7%–59.7%. This aligns with
[DF-FLOPS](https://arxiv.org/abs/2505.15070): average activation does not
control a few expensive high-DF keys.

## Alignment With Prior Work

- [CLEAR](https://arxiv.org/abs/2004.13969) supports lexical-error-focused
  semantic training, but keeps lexical and dense retrieval paths. It does not
  show that the residual is a sparse shared dot product.
- [DrBoost](https://arxiv.org/abs/2112.07771) supports sequentially learning
  what the current retriever misses, not subtracting an embedding subspace.
- [RepCONC](https://arxiv.org/abs/2110.05789) supports jointly learning balanced
  discrete representations; M1720/M1721 show why post-hoc factors are not
  enough.
- [Searching Dense Representations with Inverted Indexes](https://arxiv.org/abs/2312.01556)
  independently finds that direct dense-to-inverted conversion is workable
  but impractically slow.
- [SPARTA](https://arxiv.org/abs/2009.13013) and SPLADE demonstrate that a
  retrieval-native sparse model can succeed, but their sparse space is learned
  as the retrieval representation rather than extracted as a dense residual.
- [An Efficiency Study for SPLADE](https://arxiv.org/abs/2207.03834) reinforces
  that query/document asymmetry and efficiency-aware training are part of the
  model, not a post-hoc threshold.

## Historical Non-Duplication

The next tempting fixes are already covered:

- graph/community conversion: M1560 improves document edge co-location but
  reduces query admission;
- query-document residual routes: M1561/M1562 overfit construction queries and
  regress every held-out split;
- exact lexical route additions: M1566-M1569 expose capacity only at excessive
  reads;
- balanced post-hoc codes: M1600 contains an oracle but its deployable query
  policy fails;
- signed dense blocks: M1630 is exact but opens every block;
- learned vocabulary sparse roots: M1640 is the strongest established
  single-index mechanism, but M1691/M1700 expose the quality/DF Pareto when
  pushed toward dense complementarity.

There is no justified projection, graph, factor-rank, ridge, or selector sweep
over the M1720/M1721 representations. This statement does not close a
high-dimensional nonlinear source trained on a different residual-native
objective. M1722 may proceed only as a causal comparison against the frozen
M1600 dense teacher, with the same source, capacity, schedule, and validation
surface.

## What Survives

The user's product insight should be retained in this exact form:

> Frozen BM25 terms publish lexical evidence. Semantic capacity is trained and
> evaluated only on dense/BM25 disagreement, and pays an explicit global DF
> cost.

The residual belongs in the **training objective and sampling distribution**,
not in a precomputed vector subtraction or score matrix.

The only scientifically distinct future model is a retrieval-native balanced
discrete backbone:

```text
text -> shared dense-root trunk
     -> frozen exact lexical publisher
     -> jointly learned semantic code publisher
     -> one unified posting map
```

Its semantic code assignments and query compatibility must be learned jointly
from large corpus-derived dense/BM25 disagreement pairs. The objective must
combine:

- CLEAR/DrBoost-style lexical-error weighting;
- candidate-set top-k/listwise dense neighborhood preservation;
- RepCONC-style balanced assignment or optimal-transport load control;
- direct DF-FLOPS/max-DF and exact posting-union constraints;
- cross-corpus heldout source capacity before any BEIR qrel evaluation.

This is not another output head on frozen dense vectors. The final geometry
must be allowed to move to become indexable while the dense root supplies the
initial knowledge. M1600 and M1691/M1700 make its risk high: a small canary is
justified only under a new reviewed contract, and failure at source
observability or exact cost should end the route.

## Final Boundary

M1720/M1721 complete the post-hoc lexical-residual analysis. They provide two
real negative baselines rather than evidence that every residual-trained
posting representation is impossible:

- query-independent orthogonal residualization is closed;
- fixed low-rank/global-linear score-residual distillation is closed;
- no compiler, codebook, native index, or broader qrel run is authorized from
  either failed representation;
- the reusable deliverables are the contracts, deterministic audits, exact
  gates, and the reframed residual-native training objective.

The authorized next question is narrower than the earlier conclusion: whether
the same balanced source becomes easier to route when the teacher removes
BM25-covered dense neighbors. That question is specified in M1722 and must
beat an equal-capacity dense-teacher control before scale-up.
