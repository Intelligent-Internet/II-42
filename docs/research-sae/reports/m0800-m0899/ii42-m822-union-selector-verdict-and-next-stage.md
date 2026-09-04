# M822 Union Selector Verdict And Next Stage

## Current Evidence

M821 changed the proposal question from selector repair to proposal coverage.
The expanded-only pool was not a safe replacement for the old generated-bundle
pool, but `mixed_direction_old_plus_expanded` opened a clean useful oracle
ceiling.

Key M821 replay:

| Held-out | Baseline Clean | Baseline Utility | Union Oracle Clean | Union Oracle Utility |
| --- | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 1 | +0.000077 |
| seed7642 | 1 | +0.000012 | 1 | +0.000280 |
| seed7643 | 0 | +0.000007 | 1 | +0.000059 |

Important residual fix:

- `seed7643 / nfcorpus / PLAIN-660` was no-safe in the old pool.
- The mixed-direction expanded pool produced one safe candidate for it.
- The best safe utility for that query was `+0.002158`.

This means the generated-bundle route is not dead at the proposal layer.  The
old pool must remain as a coverage base, and mixed-direction expansion can add
missing safe candidates.

## M822 Selector Smoke

M822 trained leave-surface-out pointwise selectors over the
`mixed_direction_old_plus_expanded` pool using common inference-time bundle
features only.

HGB result:

| Held-out | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 1 | +0.000073 | 1 | +0.000077 |
| seed7642 | 1 | +0.000012 | 0 | -0.000008 | 1 | +0.000280 |
| seed7643 | 0 | +0.000007 | 0 | +0.000007 | 1 | +0.000059 |

Logistic result:

| Held-out | Base Clean | Base Utility | Policy Clean | Policy Utility | Oracle Clean | Oracle Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | 1 | +0.000076 | 1 | +0.000076 | 1 | +0.000077 |
| seed7642 | 1 | +0.000012 | 0 | -0.000003 | 1 | +0.000280 |
| seed7643 | 0 | +0.000007 | 0 | +0.000007 | 1 | +0.000059 |

## Interpretation

The positive signal is real but not yet deployable.

M821 shows the right candidate exists after unioning old and mixed expanded
proposals.  M822 shows a pointwise safe-candidate classifier cannot reliably
select it across held-out surfaces.

The failure is not worth more threshold tuning:

- seed7642 fails by choosing a harmful `webis-touche2020` candidate.
- seed7643 often chooses `inf` threshold and falls back to the harmful
  baseline selected query, so it never recovers `PLAIN-660`.
- Both HGB and logistic preserve the oracle gap, which points to objective and
  ranking form rather than a single model-family issue.

## Decision

Stop pointwise selector work over this union pool.

Do not expand the proposal pool again until a selector can recover the M821
oracle ceiling.  The next useful experiment is a pairwise/listwise winner
selector over the union pool:

1. Train on within-query winner-vs-loser comparisons, not independent
   safe-positive classification.
2. Rank candidates per selected query by predicted pairwise wins.
3. Tune only a conservative abstention threshold on held-in surfaces.
4. Replay held-out surfaces through the native evaluator with fallback to the
   old selected candidate.
5. Stop if pairwise/listwise selection cannot recover seed7642 and seed7643
   without hurting original.

If pairwise/listwise also fails while the oracle remains clean, the blocker is
selector observability, not proposal generation.  At that point this route
should either add richer inference-time interaction features or pause in favor
of the broader native P1/BM25 engineering route.
