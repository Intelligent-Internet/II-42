# M823 Pairwise Selector Stop And M824 Plan

## What Was Tested

M823 kept the M821 proposal surface fixed:

- old generated-bundle pool
- mixed-direction expanded proposal pool
- unioned as `mixed_direction_old_plus_expanded`

The only change from M822 was the selector objective.  Instead of pointwise
safe-positive classification, M823 trained within-query winner-vs-loser
pairwise models and ranked candidates by predicted pairwise wins.

## Results

HGB pairwise selector:

| Held-out | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 0 | +0.000072 | 1 | +0.000077 |
| seed7642 | 1 | +0.000010 | 0 | +0.000000 | 1 | +0.000278 |
| seed7643 | 0 | +0.000004 | 0 | +0.000014 | 1 | +0.000019 |

Logistic pairwise selector:

| Held-out | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 0 | +0.000071 | 1 | +0.000077 |
| seed7642 | 1 | +0.000010 | 0 | -0.000005 | 1 | +0.000278 |
| seed7643 | 0 | +0.000004 | 0 | +0.000014 | 1 | +0.000019 |

## Interpretation

The M821 oracle signal is real, but the M822/M823 selector surface is
under-observed.

What changed:

- M821 proved proposal coverage can be fixed.  The union pool has clean oracle
  candidates, including a safe replacement for the old no-safe
  `seed7643 / nfcorpus / PLAIN-660`.
- M822 showed pointwise safe-positive classification cannot deploy that pool.
- M823 showed pairwise/listwise-style ranking over the same weak feature set
  also cannot deploy it.

The failure is therefore not worth more model swaps or threshold tuning.

## Likely Blocker

The expanded candidates currently do not carry the same rich inference-time
interaction features that earlier M798/M802 rows used:

- query/document lexical overlap
- IDF-weighted coverage
- top-boundary interaction features
- text-token coverage signals

M822/M823 only used a reduced common feature set derived from bundle geometry
and native score-damage fields.  That is enough to show the oracle gap, but
not enough to identify the safe candidate reliably across surfaces.

## Decision

Stop selector-objective work on the weak common feature surface.

Do not continue with:

- more pointwise model swaps
- more pairwise model swaps
- threshold-only tuning
- further proposal expansion before selector observability improves

## M824 Next Probe

Build a rich-feature union table for the M821 union pool, then rerun the same
selector gates.

Requirements:

1. Keep the proposal pool fixed to `mixed_direction_old_plus_expanded`.
2. Compute M798-style interaction features for both old and expanded
   candidates.
3. First rerun pointwise and pairwise selector smokes on the richer feature
   table.
4. Use the same leave-surface-out protocol and native replay gates.
5. Stop if rich features still cannot recover the clean oracle ceiling.

Pass condition:

- all held-out surfaces clean,
- mean utility improves over M816/M822 baseline,
- seed7642 does not regress `webis-touche2020`,
- seed7643 fixes `nfcorpus`.

If M824 fails, the blocker is not proposal coverage or selector objective; it
is either missing inference-time observability or the generated-bundle route is
too brittle for deployment.  In that case pause this route and return to the
broader native P1/BM25 engineering path.
