# M1300 Post-Selector/Source Synthesis

## Scope

M1300 summarizes the M1295-M1299 branch and defines the next valid research
move.  The goal is to prevent another loop of threshold, gate, source merge, or
scalar selector variants after the latest evidence.

## Recent Evidence

| Run | Question | Result | Decision |
| --- | --- | --- | --- |
| M1295 | Can hand-scored movement/support terms improve pair/witness? | No. Pair-only remains best. | Stop hand-scored soft objective variants. |
| M1296 | Can train-split native-outcome labels train a scalar atom ranker? | No. Train label AUC is high, eval transfer fails. | Stop row-level scalar selector over current features. |
| M1297 | Can a global CUB-specific prior improve pair/witness source construction? | No. It lowers pair success and does not reduce risk. | Stop global CUB prior + pair. |
| M1298 | Can query-specific CUB target atoms improve pair/witness as an oracle? | No. CUB targets are too sparse and mostly no-op. | Stop CUB/pair merge. |
| M1299 | Can protected-head support act as a rank-moving source? | No. It only behaves as a conservative constraint. | Do not use head support as primary source ordering. |

## Current Diagnosis

The branch has not produced a deployable default, but it has narrowed the
problem:

- M1288 pair/witness remains the strongest rank-moving teacher.
- M1225 CUB-specific teacher remains useful for support-aware objective shape.
- Protected-head support is useful as a constraint, not as a source.
- Current row-level observable features cannot safely choose effective atoms.
- Simple source composition does not create a better frontier.

The repeated failure mode is structural:

> Source A carries movement but is teacher-side; Source B carries safety but is
> sparse or conservative; combining them after the fact does not produce a
> stronger native policy.

## Stop Rules From This Branch

Do not continue with:

- threshold/grid tuning over pair, CUB, or head-support features;
- hand-scored additive movement/support weights;
- scalar classifier/regressor over the same candidate rows;
- global atom priors as a primary safety signal;
- CUB/pair intersection, union, or front-loading without a new objective;
- protected-head support as source ordering.

These paths have now been tested enough at small scale.

## Next Valid Direction

The next branch should change the training interface, not the selector:

1. Construct training examples at the query level, not independent atom rows.
2. Optimize a set-valued generated-posting action under two explicit losses:
   rank movement on dense-boundary pairs and protected-head preservation.
3. Treat CUB-specific and pair/witness teachers as diagnostics/supervision
   terms, not as direct merged sources.
4. Evaluate first with native pair-success/top95 gates, then only move to
   full metric matrices if the small gate beats M1288 pair-only.

This means the next experiment should be a true joint objective prototype:

- input: candidate atom set plus query-native context;
- output: selected atom set or small delta vector;
- loss: fixed/regressed pair utility, target recall, negative-only penalty,
  top95/top100 protected overlap penalty;
- replay gate: native P1 scorer, same M1288/M1299 metrics.

## Practical Next Step

M1301 should not be another selector.  It should be a tiny set-level policy
prototype:

- start with the same four-dataset `limit25` surface;
- train only on train split;
- use a differentiable or greedy set objective that scores the whole selected
  set rather than individual atoms;
- compare against `pair_risk0.5_b192_s0.05`;
- stop immediately if it cannot beat pair-only on pair success or protected
  overlap.

If M1301 fails, the evidence points away from the current expanded-atom
compiler family and toward a larger redesign of the encoder/posting training
objective.
