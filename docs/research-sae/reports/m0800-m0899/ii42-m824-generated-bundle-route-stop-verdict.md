# M824 Generated-Bundle Route Stop Verdict

## Summary

M821-M824 tested whether the generated-bundle line can produce a deployable
improvement after the M816/M820 residual failures.

The answer is mixed but decisive:

- Proposal coverage can be repaired.
- Deployable selection over that repaired proposal pool still fails.
- The failure persists after changing selector objective and adding rich
  interaction features.

This is a stop signal for the current generated-bundle selector branch.

## Evidence Chain

### M821: Proposal Coverage Reopened

`mixed_direction_old_plus_expanded` produced a clean useful oracle ceiling:

| Held-out | Baseline Clean | Baseline Utility | Union Oracle Clean | Union Oracle Utility |
| --- | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 1 | +0.000077 |
| seed7642 | 1 | +0.000012 | 1 | +0.000280 |
| seed7643 | 0 | +0.000007 | 1 | +0.000059 |

This fixed the old no-safe `seed7643 / nfcorpus / PLAIN-660` residual by
creating a safe candidate with utility `+0.002158`.

### M822: Pointwise Selector Failed

Both pointwise models preserved the oracle gap:

| Model | original | seed7642 | seed7643 |
| --- | --- | --- | --- |
| HGB | clean, small loss | not clean, webis-touche2020 loss | not clean, nfcorpus unchanged |
| logistic | clean fallback | not clean, webis-touche2020 loss | not clean, nfcorpus unchanged |

### M823: Pairwise Selector Failed

Changing objective from pointwise safe-positive classification to within-query
winner-vs-loser ranking did not solve it:

| Model | original | seed7642 | seed7643 |
| --- | --- | --- | --- |
| HGB | not clean, trec-covid loss | not clean, climate-fever loss | not clean, nfcorpus unchanged |
| logistic | not clean, trec-covid loss | not clean, climate-fever/webis loss | not clean, nfcorpus unchanged |

### M824: Rich Interaction Features Still Failed

M824 added M798-style lexical/native interaction features for expanded
candidates.  The failure remained:

| Model | original | seed7642 | seed7643 |
| --- | --- | --- | --- |
| HGB | clean fallback | not clean, webis-touche2020 loss | not clean, nfcorpus/trec-covid loss |
| logistic | clean fallback | not clean, webis-touche2020 loss | not clean, nfcorpus unchanged |

## Interpretation

The current route is not blocked by proposal coverage anymore.  It is blocked
by reliable selector observability under leave-surface-out replay.

The repeated pattern matters:

- The oracle can pick safe candidates.
- Learned selectors repeatedly choose harmful replacements or abstain back to
  a harmful baseline.
- Adding lexical interaction features did not remove this behavior.
- Failures are not confined to one model family or one selector objective.

This makes more local tuning low value.

## Decision

Pause the generated-bundle selector branch.

Do not continue with:

- more threshold tuning,
- more pointwise/pairwise model swaps,
- more same-family feature tweaks,
- further proposal expansion before selector reliability is solved.

## Recommended Next Direction

Return to the broader native P1/BM25 engineering route as the main line.

If this generated-bundle route is revisited, it should not be another local
selector over tiny held-out surfaces.  It needs one of:

1. a stronger query-time policy with explicit safety constraints and enough
   cross-surface training data,
2. a native-index-level reranking objective over real candidate lists, or
3. a larger supervised surface where selector reliability can be measured
   without overfitting three seeds.

Until then, M821 remains useful evidence that mixed-direction expansion can
repair proposal coverage, but M822-M824 show the current route is not
deployable.
