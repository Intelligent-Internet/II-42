# II-42 M1916 P1 versus M1914 Common Native Contract

## Question

Before adding a second-stage lexical or ranking objective to M1914, determine
whether its first-stage learned sparse representation is competitive with the
frozen P1/M549U first stage on the same official corpora and the same indexed
scoring path.

This is a representation and product-cost comparison. It is not a BM25 fusion
experiment and it does not tune either route with qrels.

## Locked Routes

- `P1`: canonical M549U active-128 product atoms, alpha zero, from the frozen
  M603 official atom export.
- `M1914`: calibrated Granite 30M learned-sparse surface selected on disjoint
  MS MARCO teacher rows. The fixed support from M1913 is unchanged.

P1.3 active-512 is not part of the primary test. It may be added only if this
comparison is ambiguous and a larger P1 cost/quality point is needed.

## Common Surface

- Official full-corpus rows: `fiqa`, `arguana`, `nfcorpus`, and `scifact`.
- The exact same native PostgreSQL document IDs and qrels are used by both
  routes.
- Candidate depth is 1,000.
- Scoring uses one normalized PostgreSQL inverted representation:
  `(atom_id, doc_ord, impact)` with a B-tree on `(atom_id, doc_ord)`.
- Query execution is an indexed posting join and score aggregation. No
  document-by-document scan or materialized query-document matrix is allowed.
- No BM25, reranker, learned gate, per-dataset threshold, or qrels-derived
  parameter is allowed.

FiQA is the canary. The other three rows run only after FiQA proves identity,
metric parity with the frozen M1914 reference, and practical indexed runtime.

## Metrics

For every dataset and equal-weight macro:

- NDCG@10
- MAP@100
- Recall@100
- MRR@20
- candidate upper bound at depth 1,000
- dense overlap@100 against the same frozen dense ranking
- normalized posting relation bytes
- measured indexed query p50/p95/p99

Latency is a local engineering comparison, not a production benchmark. Each
route receives the same warm-up query count, database, hardware, SQL scorer,
candidate depth, and output materialization.

## Decision Gate

Authorize M1914 second-stage retrieval-residual work only when all conditions
hold on the completed four-row matrix:

- macro Recall@100 and candidate upper bound are no worse than P1 by `0.002`;
- macro NDCG@10, MAP@100, and MRR@20 are each no worse by `0.01`;
- no row loses more than `0.01` Recall@100 or at least two head metrics by
  more than `0.02`;
- normalized storage and indexed p95 are each no more than `1.60x` P1.

If Recall and candidate upper bound pass but head ranking fails, repair the
M1914 first stage before any second-stage work. If either candidate gate fails,
retain P1 and do not start M1914 second-stage optimization.

## Artifacts

- `runs/m1916_p1_m1914_common_native_v1/`: immutable inputs, manifests,
  warm-ups, and per-route native results.
- `ii42-m1916-p1-m1914-common-native-matrix.json`: machine-readable matrix.
- `docs/research-sae/reports/m1900-m1999/ii42-m1916-p1-m1914-common-native-report.md`: decision report.
