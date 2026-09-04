# SAE M36 Query-Distribution Reset Plan

Status: closed after M36 audit, synthetic validation, and expanded real-query
training.

## Summary

M35 showed that teacher-neighborhood labels are not enough if the query
distribution is wrong. The sentence-extracted pseudo queries were too
definition-like and document-specific, especially for `trec-covid`.

M36 therefore resets the training source:

```text
real or validated query distribution
+ teacher-neighborhood ranking target
+ fixed teacher doc atoms
  -> query text -> atoms
```

M36 still does not productize SQL/API or mutable index work. It only answers
whether the direct text-to-atoms route can learn robust broad-query semantic
neighborhoods without relying on query-time dense embedding.

## Goals

1. Separate query-distribution failure from encoder-capacity failure.
2. Train only on non-test query sources unless an artifact is explicitly marked
   `quality_claim_allowed=false`.
3. Preserve the current product direction:

```text
query text -> sparse atoms -> unified evidence-atom engine
```

4. Continue to use fixed teacher doc atoms until query-side gates pass.

## M36.0 Query Distribution Audit

Build a query-distribution audit before new training:

- real train/dev query pool by dataset;
- held-out/test query pool by dataset for comparison only;
- query length distribution;
- question-word distribution;
- content-token document-frequency distribution;
- BM25 score concentration;
- teacher-vs-BM25 top-k gap;
- teacher top-k entropy / neighborhood width;
- bucket labels:
  - broad high-DF;
  - lexical-heavy;
  - semantic-heavy;
  - short keyword;
  - long natural-language;
  - many-positive neighborhood.

Acceptance:

- The audit must show whether available train/dev queries cover the hard
  `trec-covid`-like bucket.
- If they do not, synthetic queries are only allowed after passing M36.2
  distribution validation.

## M36.1 Real-Query Teacher-Neighborhood Replay

Use real non-test queries first.

Training data:

- official train/dev queries where available;
- qrels positives;
- BM25 hard negatives;
- teacher top-k neighborhoods;
- teacher near-misses;
- current student misses.

Objective:

- fixed teacher doc atoms;
- final BM25+SAE ranking loss;
- teacher listwise target;
- qrels residual target;
- BM25-preservation term;
- candidate-budget/fanout term.

This is not the same as M32. M32 established that the larger official split
helps aggregate quality. M36.1 must explicitly bucket and weight real queries
by broad-query diagnostics, and must report whether the hard buckets improve
without dataset-specific rules.

Pass gate:

- improve `trec-covid` NDCG/MAP versus M32/M33/M35 without hurting
  `msmarco` or `dbpedia-entity`;
- no full15 aggregate regression;
- physical cost near the M32/M35 profile.

## M36.2 Validated Synthetic Query Generator

Only run this if M36.0 shows real train/dev queries do not cover the hard
broad-query bucket.

Synthetic queries must pass a validation gate before training:

- content-token DF distribution close to hard real-query buckets;
- BM25 concentration close to hard real-query buckets;
- teacher-vs-BM25 gap close to hard real-query buckets;
- topic diversity high enough to avoid boilerplate corpus-theme phrases;
- generated query text resembles information needs, not document titles or
  definitions.

Candidate generation options:

- template transformations learned from real query shapes;
- title-to-information-need conversion with strict filtering;
- multi-document cluster summaries turned into one query-like intent;
- optional LLM-assisted query generation only if the artifact is marked as
  synthetic and never used as direct quality evidence.

Stop condition:

- If synthetic-query validation fails, do not train on it.

## M36.3 Stronger Query Encoder Gate

Only reopen encoder capacity after M36.1/M36.2 clarify that the query
distribution is adequate.

Allowed changes:

- stronger token/char encoder;
- lightweight pretrained text encoder if it outputs atoms directly and does
  not require query-time dense vector retrieval;
- query-style conditioning from runtime-safe text features;
- ranking-first objective unchanged.

Not allowed:

- frozen dense embedding -> atom projection as the main product path;
- dataset-id features;
- test-qrel leakage;
- aggregate-only promotion.

Pass gate:

- product-research pass versus teacher:
  - Recall@100 gap >= `-0.010`;
  - MRR@20 gap >= `-0.015`;
  - NDCG@10 / MAP@100 gap >= `-0.025`;
- no individual hard-dataset collapse;
- physical cost no worse than the current EATMH profile unless the
  quality/cost Pareto frontier is clearly better.

## M36 Execution Order

1. Implement the query-distribution audit runner.
2. Run the audit on full15 real train/dev/test query pools.
3. Build the M36.1 real-query replay artifact.
4. Train one primary M36.1 model from the current best M32/M35-compatible
   initialization.
5. Compare M36.1 against M32, M33, M35, and teacher fixed-doc.
6. If M36.1 fails because hard buckets are absent, implement M36.2 synthetic
   validation before any new training.
7. If M36.1 has hard-bucket coverage but still fails, reopen M36.3 stronger
   query encoder under the same gate.

## Reporting

M36 results must include:

- full15 quality matrix;
- hard-dataset collapse table;
- query-bucket before/after metrics;
- physical cost matrix;
- distribution audit summary;
- final decision:
  - `promoted`;
  - `needs_encoder_capacity`;
  - `needs_validated_synthetic_queries`;
  - `closed_failed_gate`.

## Result

M36 completed:

- M36.0 query-distribution audit;
- M36.2 validation of the existing M35/M35b synthetic sources;
- M36.1 expanded real-query replay using all available clean `nfcorpus`
  train/dev queries.

Final decision:

```text
closed_failed_gate
```

The audit showed M32 train has only 3 TREC-like
`broad_high_df + many_positive` queries versus 50 in current eval. The
synthetic validation showed the existing M35/M35b default artifacts should not
be reused. Expanded `nfcorpus` real-query replay lowered SAE postings but did
not fix `trec-covid`, `msmarco`, or `dbpedia-entity` collapse.

See `sae-m36-query-distribution-reset-results-report.md`.
