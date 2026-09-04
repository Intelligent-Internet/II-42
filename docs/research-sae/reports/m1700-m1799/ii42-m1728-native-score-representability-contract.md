# M1728 Frozen Additive Native-Score Representability Contract

## Question

M1727 proves that frozen query logits can add useful dense-minus-BM25
candidates at `0.179909x` reads. Its ranking still uses exact dense scores.
M1728 asks:

> Can a score that decomposes into the same posting accumulator move those
> candidates into the dense top100 without another model or controller?

## Frozen Surface

- exact M1727A-v2 candidate policy and fixed incremental budget;
- frozen M1600 document keys, query logits, and BM25 candidates;
- M1600 full 1,000-query validation pool;
- no qrels, fitted weights, score normalization, rank features, or threshold
  search.

## Native Scores

For each candidate document, evaluate two independently fixed scores:

1. `key_hit_count`: number of document keys present in the selected query-key
   set;
2. `query_logit_sum`: sum of frozen query logits for those matching document
   keys.

Both scores are additive posting contributions. BM25 is used for candidate
admission in this audit but its score is not mixed in; lexical calibration is
the next conditional stage.

For each score, compare frozen base keys with base plus M1727 expansion keys.
Also retain the exact dense candidate upper only as the denominator for gain
capture.

## Gate

A score is representable enough to authorize global lexical/semantic
calibration only if:

- expanded O@10 is no lower than base O@10 by more than `0.001`;
- expanded O@100 and O@256 are both higher than the matched base;
- it captures at least 50% of M1727's exact-dense O@100 and O@256 gains;
- reads remain `<=0.18x` and max DF `<=0.02`.

If neither score passes, stop scoring this expansion. Do not respond with a
score transform, alpha sweep, nonlinear reranker, or deeper encoder training.

If one passes, train one global qrels-free linear posting calibration on the
disjoint M1600 train pool, then evaluate it unchanged here. The fitted score
must remain a sum of BM25 and semantic posting contributions.
