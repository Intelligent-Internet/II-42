# M1680B Disjoint Quantile APC Contract

## Why This Follow-Up Exists

M1680's 500-step frozen-backbone MLM adaptation is a real positive mechanism
result: heldout MLM loss improved by `33.8%`, new-token MLM by `32.2%`, and
overlap/new embedding cosine remained `0.9931/0.9614`. It nevertheless failed
the locked fixed-`c=5` activation gate: the unshifted model was already at
`31.7%` activation and subtracting five reduced it to `2.09%`.

That failure remains recorded. M1680B does not change its result. It corrects
one interpretation error in the experimental contract: Vocabulary Transfer
defines APC as a scalar determined by probing unlabeled training data. The
paper reports `c=5` for its particular 20k-step/global-batch run; it does not
prove that five is universal across token budgets.

## Single Test

Load the unshifted M1680 MLM model. Reconstruct the deterministic WikiText
ordering used by M1680 and skip all 32,064 validation/training texts consumed
by that run. Then:

1. use the next 64 texts as calibration;
2. set `c` to the empirical 60th percentile of valid, non-special logits so
   that `logit - c > 0` targets 40% activation;
3. use the following disjoint 64 texts as heldout validation;
4. independently validate on 64 unlabeled MS MARCO texts.

This is one closed-form quantile, not a shift or target-rate grid. No qrels,
BM25, BEIR row, retrieval metric, or dataset-specific choice enters it.

## Gate

Authorize one paired retrieval-distillation canary only if:

- the inherited M1680 MLM, new-token, and embedding-geometry checks pass;
- the fitted scalar is finite and has absolute magnitude at most `5`;
- calibration, disjoint WikiText, and MS MARCO token activation are each in
  the paper-supported `30%` to `50%` plateau;
- every surface has finite statistics and positive-logit p99/p50 at least
  `2.0`;
- the calibrated checkpoint preserves tied input/output weights.

## Stop Rules

- Do not try another target rate, quantile estimator, subset, or scalar.
- Do not use a different shift for each corpus.
- Do not launch retrieval training if either heldout corpus fails.
- A pass authorizes only a small paired standard SPLADE control. It is not a
  product or retrieval result.
