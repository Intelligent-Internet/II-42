# M1944 Full-Tail Latent Separability Contract

Date: 2026-07-13

## Question

M1943 found a diverse, low-DF oracle target but rejected the fixed top-64
proposal. The target was not absent: native ranks 96-1000 exposed every target,
while positive proposal ranks had medians 884 and 1,253. M1944 therefore asks
one independent question:

> Can qrels-free first-pass candidate context separate target residual latents
> from harm and hard neutral latents across corpora at a supportable cost?

This is a train-free representation/observability audit. It does not reopen or
relax M1943.

## Literature Basis

Pseudo-relevance feedback uses first-pass corpus response to reformulate a
query. Work comparing feedback across sparse, dense and learned-sparse
retrieval reports that PRF grounds expansion in the target corpus, while also
warning that different feedback sources have different failure modes:
[Generative and Pseudo-Relevant Feedback for Sparse, Dense and Learned Sparse
Retrieval](https://arxiv.org/abs/2305.07477).

M1944 tests only whether this interface exposes the missing residual signal. It
does not assume that first-pass candidates are relevant.

## Fixed Surface And Candidate Construction

- Frozen M1934 `b1` document postings and M1931 `rms_m4` calibration.
- Selection corpora: FiQA, ArguAna, NFCorpus and SciFact.
- First pass: the same one-index lexical plus semantic score.
- Context: combined ranks 96-1000, plus BM25 top 32 and semantic top 32.
- Oracle target/harm labels: unchanged from M1943 and qrels-only.
- Model features: qrels-free source impact, rank-weighted impact, first
  occurrence, boundary/head contrast, DF and score margins.
- Latent ID, query ID and dataset ID are excluded from features.

For each target-bearing query, the diagnostic record set contains every visible
target and harm latent plus exactly 32 highest-scoring non-target hard neutral
latents. Label-aware sampling is allowed only to create the diagnostic set; no
label enters a feature.

## Predeclared Gates

One retrieval-conditioned selector canary is authorized only if all pass:

- all four corpora and at least eight target-bearing residual queries per row;
- full-tail source target recall at least `0.95` globally and `0.90` per row;
- at least three valid LODO folds for both tasks;
- target-vs-harm pooled LODO AUC at least `0.60`, minimum fold `0.55`;
- target-vs-rest pooled LODO AUC at least `0.65`, minimum fold `0.55`;
- mean source dimensions at most `20,000`, row p95 at most `30,000`;
- row p95 source document postings read at most `150,000`.

Passing authorizes only one selector canary with a fixed eight-posting output.
It does not authorize native promotion.

## Staged Execution And Stop Rule

1. Run a 64-query FiQA/SciFact runtime and schema smoke.
2. If records contain target, harm and neutral classes, run all four selection
   corpora once.
3. Do not run unseen transfer unless every gate passes.

If either LODO task fails, or source cost exceeds its floor, close the
retrieval-conditioned residual family. Do not sweep tail depth, negative count,
classifier, thresholds or loss weights. Retain M1934 `b1`/`b1.125` as the
learned-sparse frontier.
