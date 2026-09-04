# M1520 URSI Measurement Contract

Date: 2026-07-10

## Decision Boundary

M1520 starts a new representation source. It does not continue the M1518
SPLARE/SAE-latent checkpoint and does not fine-tune the P1 output compiler.
P1.3/M549U remains a frozen baseline and rollback surface.

The candidate product shape is deterministic BM25 lexical postings plus
newly trained, non-negative semantic residual postings in the existing II42
sparse-index lifecycle. Dense is permitted as a training-time coverage teacher
but is absent from runtime.

M1520B defines measurement only. It does not authorize model training,
contextual routing, quantization, or native-index changes.

## Artifact Contract

Document and query semantic postings are JSONL rows with the following fields:

```json
{"id":"row-id","keys":["concept#route"],"weights":[1.0]}
```

`dimensions` and `atom_ids` are accepted as compatibility aliases for `keys`;
`atom_impacts` and `atom_weights` are accepted as aliases for `weights`. A key
may be an integer concept, string concept, `[concept, route]`, or an object
containing exactly `concept` and `route`. The evaluator canonicalizes routed
keys to `concept#route`.

Impacts must be finite and non-negative. Duplicate keys are invalid. The
reserved `__background__` and `__null__` keys are measured but never indexed
and do not consume document/query posting K.

Reference rankings use JSONL rows containing `query_id` and ordered `doc_ids`.
Qrels are a JSON mapping from query ID to document relevance. Qrels may affect
only final quality and recovery metrics; they cannot affect keys, weights,
budgets, checkpoints, or source construction.

## Exact Cost Metrics

The authoritative cost is the exact union of all posting lists reached by the
selected query keys after document and query TopK have been applied.

Required index metrics:

- total and estimated packed posting bytes;
- postings per document mean/p95;
- active vocabulary and optional declared vocabulary size;
- maximum document-frequency ratio;
- head 1% posting share;
- background/null assignments per document.

Required query metrics:

- active query keys mean/p95;
- touched postings mean/p95;
- touched documents mean;
- touched-document ratio mean/p95;
- semantic candidate and all-touched candidate upper bounds.

FLOPS, L0, average nonzero count, and per-key DF remain diagnostics. They may
not substitute for exact posting-union touch in a promotion gate.

## Residual Capacity Metric

For each query, let `B` be BM25 top candidate K, `D` be dense top candidate K,
`S` be semantic candidates, and `R` be relevant documents.

```text
dense_recoverable_bm25_miss = R intersect (D minus B)
residual_recovery =
    abs(R intersect (D minus B) intersect S)
    / abs(R intersect (D minus B))
unified_candidate_ub = abs(R intersect (B union S)) / abs(R)
```

The evaluator reports micro and macro residual recovery. Semantic-only quality
is diagnostic and is not a promotion requirement because the model is trained
to represent evidence missing from BM25.

## Data Isolation

- Representation construction and training use MS MARCO, corpus text,
  qrels-free corpus statistics, and teacher outputs generated from training
  data.
- A disjoint MS MARCO heldout split is the development surface.
- NFCorpus, SciFact, and FiQA are one-shot OOD capacity canaries. Their qrels
  may measure an already locked artifact but cannot choose its vocabulary,
  route, K, checkpoint, or threshold.
- The remaining BEIR15 rows, official1024, and MTEB retrieval surfaces remain
  sealed until the final validation stage.

## M1520C Capacity Gate

The initial fixed capacity surface uses 32K concepts, document K=96, query
K=24, non-negative impacts, and no contextual routes. A non-indexed background
channel is mandatory.

M1520C passes only when at least two of the three locked canaries satisfy all
of the following:

- exact mean touched-document ratio is at most 30%;
- unified candidate upper bound does not fall below 97% of the stronger BM25
  or dense reference;
- at least 90% of dense-recoverable BM25 misses are recovered;
- no indexed key has corpus-universal DF and background mass is not emitted as
  an indexed posting.

If the qrels-free representation cannot meet the oracle capacity gate, stop
the source. More epochs, a threshold grid, or contextual routing are not
authorized. Contextual routing is considered only after the non-routed source
passes quality capacity and medium-DF union remains the measured cost blocker.

## Reproducibility

Every audit output must include resolved input path, size, mtime, SHA-256,
configuration, per-query cost rows, summary metrics, and `qrels_usage`.

The stage is verified with focused unit tests, `py_compile`, Ruff, and
`git diff --check`. Runtime tests are required in addition to compilation.
