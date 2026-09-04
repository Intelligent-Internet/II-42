# M1962 Frozen M190 Official BMP Contract

## Question

M1962 asks one engine-level question before M1961 is allowed to alter query
support:

> Are the unchanged raw M190 learned impacts practical when executed by the
> exact patched BMP engine, rather than PostgreSQL's exhaustive posting-union
> evaluator?

M1962 does not train, prune, recalibrate, fuse BM25, or inspect qrels while
constructing postings. It copies the complete official FiQA raw M190 sparse
matrices produced by M1960 and changes only the integer compilation and query
engine.

## Why This Test Precedes Query-DF Pruning

The exact inverted work of a query is

```text
T(q) = sum_{a in support(q)} df(a).
```

This is a useful upper bound for exhaustive posting traversal, but it is not
the work performed by a block-max dynamic-pruning engine. M1910 and M1912A
showed that high raw posting union and high maxDF can coexist with practical
patched-BMP latency. M1660 independently established exact top-100 score and
strict-boundary parity for the same implementation. Therefore, pruning M190
before measuring BMP would confound representation loss with an engine
assumption already contradicted by project evidence.

This ordering is consistent with:

- [Two-Step SPLADE](https://arxiv.org/abs/2404.13357), which separates sparse
  representation quality from efficient candidate generation;
- [Faster Learned Sparse Retrieval with Block-Max
  Pruning](https://arxiv.org/abs/2405.01117), which treats dynamic pruning as
  part of the learned-sparse execution contract;
- [DF-FLOPS](https://arxiv.org/abs/2505.15070), which warns that high-DF terms
  need explicit corpus-level treatment but does not imply that every high-DF
  term is dispensable;
- [Term Impact Decomposition](https://aclanthology.org/2022.findings-emnlp.205/),
  which motivates engine-side impact decomposition rather than changing a
  trained representation without first measuring the engine.

## Frozen Surface

- dataset: complete official FiQA, 57,638 documents and 648 test queries;
- checkpoint SHA-256:
  `49dfd1e8de9cbbe270c1f98793157101f1892cf3dd32d10033ead66d47e2360b`;
- PPLX source: `perplexity-ai/pplx-embed-v1-0.6B`;
- representation: raw M190 normalized learned impacts;
- document support: TopK-64;
- query support: TopK-80;
- vocabulary: 16,384 latent atoms;
- float reference: M1960 native raw-M190 FiQA ranking;
- qrels: identity validation and final quality measurement only.

The adapter must verify every M1960 output digest recorded in its source
manifest. Bare BEIR qrel IDs may only receive the deterministic
`beir15:fiqa:{q,d}:` namespace used by the native surface.

## Exact BMP Configuration

- upstream source commit: `c0a17ffc`;
- exact-topK patch: `bmp-0.2-exact-topk.patch`;
- block size: 16;
- source document order, no BP reordering;
- global u8 document quantization;
- per-query f32 scaling to max weight 32;
- u32 score accumulation;
- topK 100 and candidateK 1000.

## Gates

All gates were frozen before execution.

### Identity And Exactness

- all document/query/qrel IDs match exactly;
- checkpoint and M1960 output digests match;
- all sparse impacts are finite and positive;
- all 648 queries return 100 documents;
- known-ID, returned-score, score-multiset, and strict-boundary parity are all
  exactly 1.0 against exhaustive integer scoring.

### Quantized Quality

```text
quantized exhaustive Recall@100 / float Recall@100 >= 0.98
```

The other principal metrics are reported but are not independently optimized.

### Engine Cost

The primary historical controls are M1660 at 17.499 ms p95 and M1910 full at
17.905 ms p95. M1962 is engine-viable when:

- BMP p95 is at most 25 ms;
- index bytes per document are at most 5,500;
- BMP p95 is lower than exhaustive sparse-matrix p95;
- all index sizes and latency measurements are finite.

The 25 ms and 5,500-byte limits are approximately 1.4x and 1.25x the validated
controls. They are diagnostic product ceilings, not tuned search parameters.

## Decisions

- If exactness, quality, and cost pass, retain raw M190 unchanged as the
  semantic backup representation. M1961 query-DF pruning is not justified as a
  representation rescue; it may remain only as a separately labelled
  PostgreSQL-backend optimization.
- If exactness passes but cost fails, run exactly one frozen M1961 threshold
  (`max_df_ratio=0.12`) and compare it through the same engine.
- If quantized quality fails, diagnose quantization. Do not retrain M190 or
  alter support.
- If exactness fails, stop and repair the BMP binding before interpreting any
  quality or latency number.

No result from this single FiQA engine diagnostic can establish unseen-corpus
generalization. That remains the responsibility of M1960 full15 and the
held-out-corpus validation gate.
