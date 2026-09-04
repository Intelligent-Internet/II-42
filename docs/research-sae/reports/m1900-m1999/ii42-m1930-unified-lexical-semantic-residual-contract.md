# II-42 M1930 Unified Lexical-Semantic Residual Contract

## Product Question

Can one physical inverted index combine a conventional lexical namespace with
a learned sparse semantic namespace so that the semantic postings recover
relevant documents missed by BM25, without paying again for evidence that BM25
already represents well?

The target is not ANN plus BM25, score fusion between two external engines, or
a traditional SAE reconstruction experiment. The target remains one encoder
contract and one posting index:

```text
text
  -> lexical postings
  -> learned semantic residual postings
  -> one additive posting map in disjoint namespaces
  -> one native inverted-index traversal
```

## Evidence Reset

M1930 starts from the strongest established surfaces instead of training a new
model blindly:

- native BM25 is the lexical substrate;
- P1/M549U is the dense-root product reference;
- M1914 is the calibrated small Granite learned-sparse parent;
- OpenSearch sparse-v2 is the mature learned-sparse control;
- M1911 is a latent-SAE secondary control where an equal native surface exists.

The experiment must not assume that semantic quality alone identifies the best
parent. Parent selection is based on BM25-miss recovery, row stability, and
measured native posting cost.

## M1930A: No-Training Complementarity Gate

Use the same full official FiQA, ArguAna, NFCorpus, and SciFact rows already
materialized by M1917. Export BM25 top-1000 rankings from the existing II42
indexes. For every qrel-positive pair and at depths 100 and 1,000, measure:

- shared hits, BM25-only hits, semantic-only hits, and misses by both;
- conditional recovery of BM25 misses;
- per-query BM25, semantic, and oracle-union Recall;
- candidate upper-bound gain at 1,000;
- native semantic relation bytes and indexed latency;
- semantic-only positive pairs recovered per unit of measured storage.

Qrels are evaluation-only. They cannot choose a query policy, threshold,
dataset-specific parameter, or checkpoint.

Authorize M1930B only when a trainable semantic parent satisfies all of:

1. semantic-only share at 100 is at least 1% on at least three of four rows;
2. equal-dataset macro semantic-only share at 100 is at least 1%;
3. at least 5% of BM25 misses at 100 are recovered;
4. macro candidate upper bound at 1,000 improves over BM25 by at least 1%.

These are signal-existence gates, not product promotion gates. Standalone parent
storage is an upper-bound proxy because the residual namespace has not yet been
pruned.

## M1930B: Deterministic One-Index Closure

Publish lexical and semantic postings into disjoint namespaces of one II42
generation. Use one additive query and one native posting traversal. No ANN,
post-hoc reranker, or two-engine RRF is allowed.

Before training, sweep deterministic semantic residual budgets at 0.25, 0.5,
and 1.0 times the lexical posting bytes. Report the exact quality/cost Pareto
frontier. Do not select a budget per dataset.

M1930C is authorized only if one global budget improves macro Recall@100 or
candidate upper bound while keeping NDCG@10 and MRR@20 within 1% of the better
single parent, with no row losing more than 2% Recall@100.

## M1930C: Residual Training

Freeze the selected mature sparse parent first. Train only a residual output
head using corpus-derived, qrels-free query/document samples. Sampling and loss
must emphasize evidence not already explained by lexical retrieval:

- lexical hard negatives and lexical misses from teacher competition;
- parent-faithfulness on shared evidence;
- listwise ordering among fixed candidate sets;
- explicit document-frequency and posting-byte budgets;
- query-disjoint and corpus-disjoint heldout gates.

Residual utility belongs in candidate construction and the objective. It is not
defined as literal subtraction of incomparable BM25 and semantic scores.

Only after the frozen-head route passes heldout and native gates may low-rate
joint tuning be attempted. All training runs must use ClearML and run on an
available remote GPU node.

## Scale And Stop Rules

Scale training depth only when the selected checkpoint improves both heldout
quality and native cost shape. Loss reduction alone is never sufficient.

Stop a family when any of the following repeats across two independent seeds
or heldout corpora:

- the no-training complementarity gate fails;
- gains come only from high-DF terms or exceed the declared posting budget;
- heldout improvement disappears in the one-index native replay;
- macro gains require dataset-specific thresholds;
- Recall gains exchange more than 1% NDCG@10 or MRR@20;
- longer training improves teacher fit while worsening maxDF, bytes, or row
  safety.

The final promotion surface is broader native evaluation, not a canary or an
offline scan.
