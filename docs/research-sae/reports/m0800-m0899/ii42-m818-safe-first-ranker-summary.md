# M818 Safe-First Ranker Summary

M818 tested whether the M817 failure could be solved by changing the
top-bundle ranking objective itself.

Instead of ranking the M813 oracle winner against high-utility losers, M818
trains safe-positive generated bundles to outrank unsafe or neutral bundles.
This moves harmful avoidance into the ranking teacher rather than applying a
post-hoc veto.

## Result

M818 did not pass.

| Variant | All Clean | Applied | Sum Utility | Main Failure |
| --- | ---: | ---: | ---: | --- |
| M818 HGB | 0 | 9 | +0.000007 | seed7642 nfcorpus/trec-covid |
| M818 logistic | 0 | 16 | +0.000284 | original dbpedia, seed7643 cqadupstack/webis |

HGB becomes too conservative and moves the failure from seed7643/nfcorpus to
seed7642.  Logistic finds larger utility but is not clean, and its held-out
oracle threshold cannot clean original.

## Comparison To M816/M817

| Route | All Clean | Applied | Sum Utility |
| --- | ---: | ---: | ---: |
| M816 floor 0.50 | 0 | 14 | +0.000094 |
| M817 risk-first veto | 0 | 13 | +0.000094 |
| M816 floor 0.60 | 1 | 4 | +0.000006 |
| M818 HGB safe-first | 0 | 9 | +0.000007 |
| M818 logistic safe-first | 0 | 16 | +0.000284 |

The safe-first objective did not dominate the previous best tradeoff.  It can
move where failures happen, but it does not produce a deployable common clean
selector.

## Decision

Stop the current generated-bundle selector/ranker repair loop.

Evidence across M815-M818:

- M815A: oracle winners are separable.
- M816A: query-level safe-existence is separable.
- M816B: existence guard improves utility but leaves harmful top bundles.
- M817: post-hoc risk veto cannot remove the residual harmful bundle.
- M818: safe-first ranker objective does not make top-bundle selection clean.

This points away from more selector thresholds, risk caps, or pairwise loss
variants on the same generated-bundle feature table.

## Next Structural Direction

The next useful probe should rebuild the proposal/top-bundle teacher, not
continue repairing the current selector.

Recommended `M819`:

- Inspect harmful residual rows from M816/M817/M818 and identify what candidate
  is missing from the proposal set or why the safe candidate is not promoted.
- Build a new proposal family that emits multiple safe alternatives per query,
  not just one high-scoring top bundle.
- Score proposal families by native replay before training a selector.
- Only after a proposal family shows a clean oracle ceiling should a selector
  be trained.

Stop condition: if new proposal families cannot produce a better clean oracle
ceiling than the current M813/M816 surface, pause generated-bundle expansion
and return to the base P1/BM25 engineering-index route.
