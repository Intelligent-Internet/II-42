# M820 Coverage Veto And Proposal Stop Summary

M819-M820 tested whether the M816 residual failure can be fixed without
changing the proposal generator.

## Evidence

M816 floor 0.50 is the best non-clean selector tradeoff:

| Surface | Clean | Applied | Utility | Negative Tasks |
| --- | ---: | ---: | ---: | --- |
| original | 1 | 3 | +0.000076 | none |
| seed7642 | 1 | 3 | +0.000012 | none |
| seed7643 | 0 | 8 | +0.000007 | nfcorpus |

M819 same-pool best-safe replacement shows the residual blocker is one
no-safe selected query:

| Surface | Selected | Safe Rate | No Safe | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: |
| original | 3 | 1.000 | 0 | 1 | +0.000076 |
| seed7642 | 3 | 1.000 | 0 | 1 | +0.000012 |
| seed7643 | 8 | 0.875 | 1 | 1 | +0.000021 |

The no-safe row is `seed7643 / nfcorpus / PLAIN-660`.  Dropping it makes the
held-out replay clean, but the total gain remains small.

M820 trained a coverage veto over the M816 base-selected queries.  It did not
remove the no-safe residual:

| Surface | Clean | Applied | Vetoed | Utility | Negative Tasks |
| --- | ---: | ---: | ---: | ---: | --- |
| original | 1 | 3 | 0 | +0.000076 | none |
| seed7642 | 1 | 2 | 1 | +0.000010 | none |
| seed7643 | 0 | 8 | 0 | +0.000007 | nfcorpus |

Including crossing rows also did not solve coverage.  The residual shifted to
another nfcorpus no-safe query (`PLAIN-541`) and became worse:

| Surface | Selected | Safe Rate | No Safe | Baseline Utility | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: |
| seed7643 | 6 | 0.833 | 1 | -0.000034 | +0.000013 |

Raw feature-table inspection shows both residual nfcorpus queries have only
three generated candidates, all variants of the same two dimensions
`78/320`.  There is no meaningful proposal diversity for the selector to use.

## Decision

Stop selector-side repair for the current generated-bundle pool.

The current pool is too narrow on residual failure queries.  Selector,
abstention, veto, and safe-first ranking can only choose among nearly identical
candidates.  This explains why M815-M820 repeatedly found signal but failed to
produce a deployable recall-matrix breakthrough.

## Next Structural Probe

`M821 proposal-diversity rebuild smoke`

Goal: produce a broader candidate set for residual-style queries before
training any selector.

Requirements:

- Generate more than one dim-pair family per query.
- Include candidates from lexical/atom coverage, not only the current margin
  pair.
- Keep a strict native replay oracle gate: the expanded pool must improve the
  clean oracle ceiling before any learned selector is trained.
- First smoke on residual nfcorpus-style rows, then replay on the same
  original/seed7642/seed7643 held-out surfaces.

Stop condition: if a more diverse proposal pool cannot improve the clean
oracle ceiling over M819, generated-bundle expansion should pause and work
should return to the broader engineering-index P1/BM25 route.
