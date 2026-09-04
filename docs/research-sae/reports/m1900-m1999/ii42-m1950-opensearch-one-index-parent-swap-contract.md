# M1950 OpenSearch One-Index Parent Swap Contract

Date: 2026-07-13

## Question

M1917 selected frozen OpenSearch sparse-v2 as the strongest mature ranking
parent on the common four-corpus native surface. M1930-M1934 later proved the
exact product form `BM25 + bounded semantic postings -> one additive index`,
but only with the compact M1914 parent. M1950 asks:

> Does the mature OpenSearch parent produce a stronger lexical-semantic
> one-index frontier under the same deterministic publisher and gates?

This is a parent swap, not a new loss, model training run or two-engine fusion.

## Frozen Inputs

- Model: `opensearch-project/opensearch-neural-sparse-encoding-v2-distill`.
- Revision: `269e6638b2c4f648996691f6d751495285d8f330`.
- Surfaces: the exact M1917 FiQA, ArguAna, NFCorpus and SciFact artifacts.
- Lexical substrate: exact M1930 native-token BM25 postings.
- Score: one sparse dot product over disjoint lexical and semantic namespaces.
- Candidate depth: 1,000.
- Qrels are evaluation-only.

Every surface manifest, model identity, file size and SHA-256 must validate
before evaluation.

The FiQA OpenSearch cache contains the same document-ID set as the lexical
surface but a different row order. Semantic rows must be permuted in memory to
the frozen M1916 reference IDs before scoring. The permutation is exact and
recorded; missing, duplicate or unequal IDs are fatal.

## Fixed Search And Gate

Reproduce the existing M1930B global grid exactly:

- semantic document budgets: `0.25x`, `0.5x`, `1.0x` lexical postings;
- semantic query budgets: top 16, top 32 and all;
- global unlabeled median-top1 scale multipliers: `0.5`, `1.0`, `2.0`.

There is no dataset-specific configuration. Selection and LODO row-safety
gates are unchanged from M1930B. This is a controlled reproduction on a new
frozen parent, not a new hyperparameter search.

## Stop And Expansion Rules

- If no global configuration passes, close OpenSearch as a parent for this
  bounded additive product path.
- If selection passes but LODO fails, report the row failure and do not train.
- If both pass, authorize one exact native canary on NFCorpus and SciFact.
- Do not start neural continuation from this result. Training requires a
  separate corpus-disjoint objective and native-cost contract.
