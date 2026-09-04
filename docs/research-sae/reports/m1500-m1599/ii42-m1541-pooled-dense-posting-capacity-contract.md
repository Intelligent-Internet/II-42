# M1541 Pooled-Dense Posting Capacity Contract

## Question

M1540A established that a paper-shaped token SAE can recover semantic
residuals, but its exact posting union touches most of each canary corpus.
M1541 asks a narrower question before any further training:

> Can the frozen BGE pooled embedding be compiled into signed coordinate
> postings plus a compact document tail sketch while preserving retrieval
> quality under an honest native traversal budget?

This is a capacity audit, not a trainable model experiment. A failure stops
the pooled-coordinate source; it does not authorize another projection or
loss search.

## Frozen Inputs

- encoder: `BAAI/bge-base-en-v1.5`;
- query instruction: the same instruction used by M1540A;
- canaries: exact M1520 NFCorpus, SciFact, and FiQA corpora and qrels;
- document-derived PCA rotation only;
- 128 signed active document coordinates;
- 256-dimensional INT8 document tail sketch;
- qrels are used only after all rankings have been produced.

No dataset identifier, qrel, relevance label, or row-specific threshold may
affect rotation, postings, admission, scoring, or quantization.

## Surfaces

### Honest 8% and 15% edge budgets

For each query, a max-impact merge traverses at most `budget * N` entries
across same-sign coordinate posting lists. Every reached document is reranked;
there is no hidden full-union scan followed by a budget cap.

The report must distinguish:

- posting entries read;
- unique documents in the raw posting union;
- documents reranked with the tail sketch.

### Historical M392 ceiling

The historical shape may inspect a prefix of 256 from every active query
coordinate and then retain 8% of documents by sparse score. It is reported
only to separate representation capacity from traversal cost. It cannot pass
the deployment gate regardless of quality if its raw union exceeds budget.

### Required baselines

- exact frozen-BGE dense retrieval;
- stored reference BM25;
- budgeted signed-coordinate sparse score;
- budgeted exact-dense rerank upper bound;
- budgeted 256-dimensional INT8 tail rerank;
- historical post-selection upper and tail rows.

## Metrics

Each dataset and macro row must report:

- NDCG@10, MAP@100, Recall@100, MRR@20;
- candidate upper bound;
- dense overlap@10 and overlap@100;
- mean and p95 posting-read ratio;
- mean and p95 raw-union ratio;
- mean and p95 rerank ratio;
- logical index bytes per document.

## Predeclared Gate

The 15% INT8 tail row passes a dataset only when all are true:

1. its Recall@100 is at least 95% of exact BGE;
2. its NDCG@10 is at least 95% of exact BGE;
3. its candidate upper bound is at least 95% of exact BGE;
4. its exact-dense candidate upper has overlap@100 at least 0.90;
5. mean posting-read and raw-union ratios are each at most 0.151.

Scale is authorized only when at least two of three datasets pass and no
dataset falls below 90% of exact BGE Recall@100 or NDCG@10.

## Decisions

- **Pass:** test deterministic lexical BM25 and semantic postings in one
  native index before training any residual compiler.
- **Admission fail:** stop the pooled-coordinate source; do not tune the tail
  sketch because the missing documents cannot be rescored.
- **Tail fail with admission pass:** test one larger deterministic sketch
  capacity, then stop if it still misses the gate.
- **Cost fail with historical quality pass:** preserve M392 as an offline
  quality ceiling, but reject it as an engineered posting route.
- **All fail:** return to candidate/source construction; do not add epochs or
  losses.

