# M1555 Unique Lexical Boundary Report

## Question

M1553 showed a strong macro gain from preserving P1 semantic ranks 1-99 and
using lexical evidence at rank 100, but arguana regressed.  M1555 tests the
narrow implementation hypothesis that the regression came from promoting a
document already present in the semantic tail rather than a genuinely new
lexical-residual document.

The test changes only the action source: rank 100 may be replaced by the best
BM25 document absent from the semantic top256.  The P1 semantic head remains
unchanged.

## Result

The hypothesis is rejected on full official arguana.

| Policy | Recall@100 | MAP@100 | CUB | O@100 |
| --- | ---: | ---: | ---: | ---: |
| P1 semantic | 0.990007 | 0.276755 | 0.998572 | 1.000000 |
| M1553 unrestricted | 0.989293 | 0.276748 | 0.998572 | 0.990064 |
| M1555 lexical-unique | 0.988580 | 0.276741 | 0.998572 | 0.990000 |

Relative to P1 semantic, lexical-unique admission changes:

- Recall@100: `-0.001428`;
- MAP@100: `-0.000014`;
- CUB: unchanged;
- NDCG@10 and MRR@20: unchanged by construction.

## Interpretation

Arguana is already saturated at this boundary: P1 semantic CUB is `0.998572`
and the lexical union adds no relevant candidate capacity.  Under that
condition, forcing any rank-100 replacement has negative expected value.
Restricting the source to newly admitted lexical documents does not make the
action safe; it makes the observed regression larger.

This is not evidence against the unified residual architecture.  It is
evidence against an unconditional boundary action.  The next prerequisite is
an observability audit: determine whether gain and harm actions can be
separated from qrels-free query-time boundary features.  A learned guard is
not authorized unless that signal transfers leave-one-corpus-out and rejects
the harmful arguana actions.

## Decision

**Stop M1555.**  Do not run the same lexical-unique policy over the remaining
official rows and do not tune another source threshold.  Preserve M1553 as the
capacity result and proceed to M1556 observability before any guard training.
