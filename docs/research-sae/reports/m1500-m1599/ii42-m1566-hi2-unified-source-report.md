# M1566 HI2 Unified Source Capacity Report

## Decision

**Do not train or promote the tested full-term or term15 source.**

The combined semantic-cluster and lexical-term index contains enough
information to recover dense retrieval only when every lexical posting is
available to a dense-teacher oracle. That source violates the read budget by
more than 8x. The fixed HI2 unsupervised term15 compression is efficient but
destroys the required membership even under the oracle.

The experiment isolates a narrower, actionable bottleneck: preserve the
low-document-frequency lexical capacity of the full source without scanning
its universal/high-DF postings. Do not continue with score fusion, selector
thresholds, or training against the failed term15 target.

## Surface And Integrity

- Official FiQA: 57,638 documents, 648 queries, and 1,706 qrel edges.
- M1565 route basis SHA-256:
  `46c2a0fe2b0b453c91d7bf40cecbab3e3c2bcbc02289ee88fd05400a91e4f9ee`.
- Qrels-free candidate surface SHA-256:
  `25e77e27628bda030eb7a028f8d4a5818e15610ce738f72e0685c124194eb685`.
- Route1000 maximum parity delta versus M1565: `0.0`.
- ClearML task: `97cc3f06ea744ce1bfc8b176df3ed266`.
- Runtime: 128.3 seconds.

The index has one semantic route namespace and one lexical term namespace.
Dense vectors are used only as a qrels-free oracle and exact candidate codec;
there is no ANN candidate path or external BM25 engine.

## Source Statistics

| Source | Dimensions | Postings | Postings/doc | Max DF ratio | Mean reads/query |
| --- | ---: | ---: | ---: | ---: | ---: |
| full terms | 75,189 | 4,838,599 | 83.9481 | 0.927340 | 2.531509x |
| term15 | 69,003 | 862,374 | 14.9619 | 0.010774 | 0.023459x |

Queries use 10.71 unique terms on average and 17.65 at p95. The full source
cost is therefore dominated by common-term lists, not by candidate reranking
or route probes.

## Capacity Matrix

| Variant | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| route1000 | 0.921759 | 0.808503 | 0.702311 | 0.683089 | 0.383019 | 0.322550 | 0.468258 | 0.822706 | 0.021159x |
| route + full terms | 0.956327 | 0.857392 | 0.757409 | 0.721033 | 0.400766 | 0.337479 | 0.486899 | 0.878106 | 2.552668x |
| route + full-term oracle | 0.997840 | 0.992485 | 0.985448 | 0.731927 | 0.400680 | 0.337622 | 0.485211 | 0.905573 | 2.552668x |
| route + term15 | 0.949383 | 0.843287 | 0.741000 | 0.710759 | 0.395886 | 0.333553 | 0.482150 | 0.862693 | 0.044619x |
| route + term15 oracle | 0.958488 | 0.859645 | 0.763847 | 0.714375 | 0.397032 | 0.334209 | 0.483197 | 0.868109 | 0.044619x |

Exact dense has O@100/O@256 `1.0`, Recall@100 `0.733984`, NDCG@10
`0.400339`, MAP@100 `0.337638`, MRR@20 `0.485244`, and CUB `0.902807`.

## Interpretation

The full-term oracle is the first low-candidate unified source in this branch
to recover both dense top100 and top256: O@100 is `0.992485`, O@256 is
`0.985448`, and CUB exceeds exact dense by `0.002766`. This proves that
semantic routes and exact lexical membership are complementary in one index.

It is not deployable as measured. Mean reads are `2.552668x` corpus size and
p95 exceeds `4.96x`, so the source violates the 30% read cap despite returning
only about 1,256 unique candidates.

Term15 solves cost but not capacity. Its oracle improves O@100 only from
`0.843287` to `0.859645`; no scorer or deeper ranking loss can recover terms
that were never indexed. Training against this source would repeat the old
capacity-before-loss error.

The deployable full-term order is nevertheless quality-positive: NDCG and
MRR slightly exceed exact dense, MAP retains 99.95%, and Recall retains 98.23%.
Its remaining problem is dense-faithful admission plus posting-list cost, not
absence of useful retrieval signal.

## Next Source Gate

Before any neural term selector, test one analytically derived background
channel. The combined read budget is 30%; reserving the M1565 route cap of 5%
leaves 25% for at most 32 lexical query terms. An indexed term's DF ceiling is:

```text
max_df_ratio = (0.30 - 0.05) / 32 = 0.0078125
```

Keep every exact term below that ceiling and make higher-DF terms non-indexed
background. This differs from term15: it removes terms because of access cost,
not because they fall outside a per-document impact prefix. If the resulting
qrels-free oracle preserves the full source's dense coverage, it becomes a
valid fixed target for HI2-style selector/impact distillation. If it does not,
stop lexical source compression rather than searching more thresholds.
