# M1945 All-Source Selector Canary Contract

Date: 2026-07-13

## Question

M1944 passed its predeclared cross-corpus separability and cost gates, but its
LODO AUC was measured on a label-aware diagnostic set: every visible target
and harm plus 32 hard neutral dimensions per query. M1945 asks the stricter
deployment question:

> Does the fixed M1944 signal still recover safe residual dimensions when it
> must rank every qrels-free source dimension for a held-out corpus?

This is a train-free selector audit. It does not train an encoder or change
the frozen posting index.

## Fixed Selector

- Frozen M1934 `b1` document postings and M1931 `rms_m4` calibration.
- Same ranks 96-1000, BM25 top 32, semantic top 32 and 31 qrels-free M1944
  features.
- One LODO logistic model for target versus rest and one for target versus
  harm. Each held-out corpus is excluded from fitting both models.
- No latent ID, query ID, dataset ID or qrels-derived feature.
- Fixed selector score:

  `sqrt(P(target_vs_rest) * P(target_vs_harm))`

- The selector ranks the complete source, typically 7,000-11,000 dimensions,
  not the sampled M1944 diagnostic set.
- Fixed output budgets: top 8 for the deployable action and top 64 for
  observability diagnosis.
- The original M1944 proposal score is reported as a fixed baseline.

Qrels are used only to recreate target/harm labels and score held-out ranking.
No classifier, threshold, feature set, budget or loss sweep is allowed.

## Predeclared Gate

A separate fixed-eight native replay is authorized only if all pass on the
four-corpus selection surface:

- at least eight residual queries in every row;
- selector target recall at top 8 at least `0.10` globally and `0.05` per row;
- selector any-target query rate at top 8 at least `0.25` globally and `0.15`
  per row;
- selector harm recall at top 8 at most `0.02` globally and `0.05` per row;
- selector target recall at top 64 at least `0.50` globally and `0.35` per row;
- selector any-target query rate at top 64 at least `0.60` globally and `0.50`
  per row;
- selector harm recall at top 64 at most `0.10` globally and `0.20` per row;
- selector target recall at both budgets must exceed the original proposal;
- all output scores and metrics must be finite.

Passing authorizes one native replay with the frozen top-eight selector. It
does not authorize neural training or product promotion.

## Staged Execution And Stop Rule

1. Run a 64-query FiQA/SciFact runtime and schema smoke.
2. If full-source scoring is finite and bounded, run all four corpora once.
3. Run native replay only if every full gate passes.

If full-source ranking fails, M1944 is recorded as sampled separability rather
than deployable observability. Close this selector family without classifier,
threshold or feature ablations. Retain M1934 `b1`/`b1.125` as the current
learned-sparse frontier.
