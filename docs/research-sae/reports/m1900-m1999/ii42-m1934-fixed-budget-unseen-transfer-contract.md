# II-42 M1934 Fixed-Budget Unseen Transfer Contract

## Question

M1933 selected a global semantic posting budget of `1.125x` lexical postings
using FiQA, ArguAna, NFCorpus, and SciFact. The selected point improved all
macro quality metrics under leave-one-dataset-out validation and reproduced
exactly through the native PostgreSQL posting path on NFCorpus and SciFact.

M1934 asks whether the same frozen budget and M1931 query calibration transfer
to corpora that did not participate in either selection step.

## Frozen Configuration

- parent: IBM Granite 30M sparse checkpoint at revision
  `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`;
- fixed M1914 query/document power transform;
- document support: parent top-192 dimensions;
- query support: parent top-50 dimensions;
- query calibration: M1931 `rms_m4`;
- baseline semantic budget: `1.0x` lexical postings;
- candidate semantic budget: `1.125x` lexical postings;
- one disjoint lexical/semantic namespace and one additive sparse dot;
- candidate depth: 1,000.

No qrel-derived atom selection, corpus-specific scale, budget search, learned
gate, reranker, or new checkpoint is allowed.

## Unseen Rows

Run in this order:

1. SciDocs as the small semantic-domain canary;
2. Quora as the lexical/exact-match stress row;
3. TREC-COVID as the biomedical domain-shift row.

The larger rows may start only after SciDocs has no material row harm. All
model inference runs on Spark nodes. The local database may build exact
lexical matrices and run the native engineering canary, but it must not run
the model encoder.

## Gates

For each row, relative to the frozen `1.0x` baseline:

- Recall@100 delta must be at least `-0.01`;
- NDCG@10 and MRR@20 must retain at least `98.5%`;
- the semantic posting ratio must not exceed `1.125x`.

Across all three unseen rows:

- NDCG@10, MAP@100, and MRR@20 must retain at least `99.5%`;
- Recall@100 or candidate upper bound must improve by at least `0.002`;
- no dataset-specific configuration is permitted.

If the fixed transfer gate passes, emit the same one-index surface and run an
exact native replay on at least one unseen row. Offline and native metrics must
match within `1e-5`; compact-index or PostgreSQL storage and latency must be
reported separately from quality.

## Stop Rules

- Stop immediately if SciDocs has material row harm.
- Stop promotion if the full unseen macro gate fails.
- Do not respond to a failure with another scalar budget or multiplier sweep.
- If quality is safe but the fixed budget has no unseen gain, retain M1931 and
  diagnose corpus-dependent semantic allocation before any model training.
- Neural training is authorized only after a deterministic policy transfers;
  the deterministic publisher already implements this policy exactly.
