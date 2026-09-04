# M1943 Cross-Corpus Latent Observability Contract

Date: 2026-07-13

## Question

M1942 proved that a frozen learned-sparse parent can admit a small, monotonic
query residual without harming its existing support. It did not pass the
promotion gate because the learned residual collapsed onto latents used by
almost every query. More steps increased query load without finding additional
rescues.

M1943 asks the prerequisite question before any further training:

> Does the frozen M1934 `b1` native surface expose a diverse, qrels-free and
> cross-corpus observable latent target for safe top-100 boundary movement?

This is a train-free audit. It cannot promote a model or index.

## Frozen Surface

- Parent: M1914 Granite 30M learned-sparse encoder.
- Document postings: deterministic M1933 `b1` balanced pruning.
- Lexical postings: frozen M1930 BM25 matrix.
- Query calibration: exact M1931 `rms_m4` artifact, SHA256
  `0f3e5a7a0f11a3d199e05ff09a3ba7f38a468892be8d7511cb22d73e5ceed591`.
- Selection corpora: FiQA, ArguAna, NFCorpus and SciFact.
- Unseen corpora, only after the selection gate: SciDocs, Quora and TREC-COVID.

Every run records absolute source paths, file sizes, mtimes and SHA256 hashes.
No surface is regenerated.

## Oracle Labels And Deployable Inputs

Qrels are used only to define research labels. For each query whose relevant
document is present at ranks 101-1000 but absent from top 100:

1. collect semantic latents on the missed relevant documents;
2. exclude the frozen query's existing support;
3. compare each latent with non-relevant documents in the fixed rank 96-256
   competition window;
4. label the strongest positive contrasts as targets and the strongest
   negative contrasts as harms.

The proposal is strictly qrels-free. It uses only latent evidence from:

- BM25 top 32 documents;
- semantic top 32 documents;
- combined ranks 96-256;
- combined top-95 protection context;
- corpus document frequency and native score margins.

The fixed proposal admits at most 64 new dimensions with positive
source-versus-head contrast. Latent identity and dataset identity are not model
features.

## Predeclared Gates

The selection run authorizes one load-balanced residual-head canary only if all
conditions pass:

- at least four corpora and at least eight residual queries per corpus;
- at least 64 residual queries with a non-empty oracle target;
- oracle target normalized entropy at least `0.50`;
- no target latent appears in more than `25%` of target queries;
- proposal target recall at least `0.50`;
- proposal any-target-hit rate at least `0.25`;
- every corpus proposal target recall at least `0.25`;
- every corpus proposal any-target-hit rate at least `0.10`;
- at least three valid LODO folds;
- pooled target-versus-harm LODO AUC at least `0.60`;
- minimum held-out-fold AUC at least `0.55`.

These floors are intentionally above random but below a promotion claim. They
only establish that the target is sufficiently visible to justify training.

## Staged Execution

1. Run FiQA plus SciFact as a structural canary. Validate source identity,
   residual-row construction, target diversity and proposal coverage.
2. If valid, run all four selection corpora and apply the full gate.
3. Only if the selection gate passes, run the three unseen corpora and repeat
   cross-corpus/LODO analysis.
4. Only if both surfaces remain observable, design one load-balanced residual
   objective. Do not sweep rank, budget, steps or loss weights before then.

## Stop Rule

Stop before training if target latents are universal, qrels-free proposals
cannot see them, or LODO separability fails. In that case M1942's collapse is
an observability/source limitation rather than a shallow optimization problem.
Retain M1934 `b1`/`b1.125` and close the bounded query-residual family.
