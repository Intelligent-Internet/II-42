# M817 Risk-First Top-Bundle Veto Summary

M817 tested the next logical repair after M816:

- M816A proved query-level safe-existence is learnable.
- M816B showed existence guarding improves utility but still lets unsafe top
  bundles through.
- M817 adds a risk-first veto over the already selected top bundle.

The goal was narrow: keep the useful M816 floor-0.50 behavior while removing
the seed7643/nfcorpus failure.

## Result

M817 did not pass.

| Variant | All Clean | Applied | Sum Utility | Remaining Failure |
| --- | ---: | ---: | ---: | --- |
| M816 floor 0.50 | 0 | 14 | +0.000094 | seed7643 nfcorpus |
| M817 HGB risk veto | 0 | 13 | +0.000094 | seed7643 nfcorpus |
| M817 logistic risk veto | 0 | 13 | +0.000094 | seed7643 nfcorpus |
| M816 floor 0.60 | 1 | 4 | +0.000006 | none |

HGB and logistic vetoes converged to the same held-out behavior:

- original remains clean with 3 applied queries.
- seed7642 remains clean with 3 applied queries.
- seed7643 still selects 7 queries and remains non-clean because nfcorpus
  survives the veto.

The risk model only vetoed one candidate relative to M816 floor 0.50.  It did
not identify the residual harmful nfcorpus top bundle.

## Interpretation

The failure is not threshold calibration anymore.  It is not solved by a
low-capacity vs high-capacity risk model either.

The current top-bundle representation has a blind spot:

- query-level existence can tell whether some safe bundle exists;
- pairwise winner ranking can find oracle-like bundles often enough;
- but the chosen top-bundle features do not reliably separate a small harmful
  nfcorpus-style top bundle from safe bundles.

This means continuing with selector/veto patching is likely to loop.

## Decision

Pause the current generated-bundle selector repair line.

The next useful step is structural: rebuild the top-bundle teacher/objective so
that harmful top-bundle avoidance is part of the ranking objective, not a
post-hoc veto.

Recommended next probe:

`M818 safe-first top-bundle ranker`

- Train pairwise ranking pairs where safe-positive bundles must outrank unsafe
  or neutral bundles.
- Use harmful/risky candidates as explicit hard negatives.
- Evaluate whether the top selected bundle itself becomes safe before adding
  existence guards or thresholds.
- Stop if safe-first ranking still cannot remove seed7643/nfcorpus while
  keeping nontrivial positive utility.
