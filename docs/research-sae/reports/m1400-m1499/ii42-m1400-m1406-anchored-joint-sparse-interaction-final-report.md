# M1400-M1406 Anchored Joint Sparse Interaction Final Report

Date: 2026-07-10
Decision: no-go at M1401 Gate A

## Outcome

The route was designed as a bounded falsification program. M1400 froze P1.3
and M549U, separated their historical embedding lineages, stopped the
query-only selector lineage, and required an equal-budget representation test
before any new encoder training.

M1401 then tested P1-style single sparse dot, pooled joint token dot, sparse
MaxSim, and magnitude-routed Signed MaxSim on a common frozen PPLX root.
Same-forward root parity and exact signed-score parity passed, but the
quality/cost capacity gate failed twice.

The consistent result is:

> Raw PPLX token late interaction can move oracle-inserted tail positives, but
> it cannot preserve PPLX/P1-style head geometry at bounded posting cost.

## Stage Disposition

| Stage | Disposition | Reason |
| --- | --- | --- |
| M1400 route contract | complete | anchors, data isolation, gates, and stop rules frozen |
| M1401 capacity audit | complete, failed | no Signed MaxSim Pareto point on two independent surfaces |
| M1402 frozen-root projector | not run by design | Gate A prerequisite failed |
| M1403 hard-row seeds | not run by design | no eligible M1402 checkpoint |
| M1404 shared15 LODO | not run by design | no eligible trained route |
| M1405 scaling | not run by design | earlier gates failed |
| M1406 native grouped scorer | not run by design | offline capacity did not justify index work |

## What Was Learned

1. The historical P1.3 shared15 proof and M549U PPLX teacher were incorrectly
   treated as one root in the initial smoke. The corrected contract keeps them
   as separate frozen anchors.
2. PPLX SentenceTransformer exposes both the quantized 1024-dimensional
   sentence embedding and unpooled 1024-dimensional token states. Their pooled
   parity is above 0.9996 mean on both formal surfaces.
3. Signed MaxSim score transport is numerically correct to below 3e-8 error.
   The failure is therefore semantic/ranking capacity, not implementation
   arithmetic.
4. Full token MaxSim shows a tail-recall signal only with severe fanout and a
   head-quality tradeoff. Sparse MaxSim retains the tradeoff rather than
   resolving it.
5. A more expressive scorer is not automatically a safer retrieval model.
   This matches the M1000+ observation that oracle movement exists while
   deployable target/harm separation remains absent.

## Engineering Decision

Retain P1.3 as the native engineering baseline. Do not add grouped Signed
MaxSim to UnifiedPosting and do not train M1402 under this objective.

Any future late-interaction proposal must be treated as a new route, not a
continuation of M1400. Before training, it must explain how token retrieval
semantics are pretrained, how head ordering is anchored, and how grouped
posting fanout remains below the failed M1401 frontier.

## Verification

- M1401 runner compiles with `py_compile`.
- Seven focused unit tests cover sparse coding, signed routing, exact parity,
  root alignment, one-dimensional cost accounting, and grouped posting
  touches.
- `git diff --check` passes for the M1400/M1401 files.
- No Spark training process was started; spark-2's existing workload was not
  disturbed.
