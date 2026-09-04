# M769 Deterministic Policy Stop and Next Route

## Decision

Stop the single-threshold deterministic policy line as a deployable
improvement route.

M758, M764, M766, M767, and M768 now give a consistent picture:

- deterministic native-context policies can extract small qrels-free gains;
- rank-trace feedback is a useful safety diagnostic;
- the high-gain rank-trace guard is split-sensitive;
- multi-seed calibration can make the guard safe, but only by shrinking it to
  diagnostic-scale gains.

The route should not continue by tuning more thresholds on the same proposal
pool.  The remaining work must change the structure.

## Evidence

| Run | Scope | Policy | Applied | Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M758 | original | deterministic context top16 | 224 | fiqa, scidocs | 1 | +0.000246 | +0.000332 | +0.000000 | +0.000032 | +0.000000 |
| M764 | original | rank-trace top16 | 64 | none | 1 | +0.000369 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| M766 | seed7642 | fixed M764 guard | 58 | trec-covid | 1 | +0.000225 | +0.000203 | +0.000185 | +0.000042 | +0.000000 |
| M767 | seed7643 | fixed M764 guard | 51 | dbpedia-entity, hotpotqa, nfcorpus, scidocs | 0 | -0.000288 | -0.000145 | +0.000000 | +0.000038 | +0.000000 |
| M768 | multi-seed | calibrated rank-trace guard | 18 | none | 1 | about +0.000005 | +0.000000 | +0.000000 | about +0.000005 | +0.000000 |

M768 best guard:

```json
{
  "feature": "trace_spearman_at_256",
  "mode": "ge",
  "threshold": 0.9999327649347676
}
```

M768 all-test result:

- original: applied 4, dMAP +0.000004, dCUB +0.000008;
- seed7642: applied 9, dMAP +0.000005, dCUB +0.000008;
- seed7643: applied 5, dMAP +0.000004, dCUB +0.000000;
- no negative tasks;
- no dense-overlap spend.

This is safe, but not meaningful as a product improvement.

## Interpretation

The problem is not that rank-trace is useless.  It is the first qrels-free
surface that directly measures the native rank damage caused by a candidate
posting delta.

The problem is that the current global proposal pool does not provide enough
robust, reusable safe-positive rows.  Once the policy is calibrated across
multiple seeds, almost all useful rows are rejected.

That means the next bottleneck is upstream of the threshold:

1. proposal generation quality;
2. query-local online accept/fallback mechanics;
3. native engineering cost of evaluating a candidate delta safely.

## What To Keep

Keep these artifacts as reusable diagnostics:

- M754 global proposal export;
- M758 native-context policy features;
- M764 rank-trace feature computation;
- M768 multi-seed calibration harness.

Do not keep M764's original fixed guard as a frozen policy.  It is useful only
as a split-positive witness.

## Stop Rule For This Line

Do not run another threshold search unless one of these changes first:

- the proposal generator changes;
- the native online execution path changes;
- the validation surface changes from offline replay to DB/plugin native query
  execution.

Any new policy must pass:

- multiple seeds or native held-out surfaces;
- no negative tasks;
- non-negative MAP, NDCG, MRR, CUB, and dense-overlap deltas;
- applied-query count high enough to be more than diagnostic.

## Next Route

### M770: Proposal Quality Audit

Goal: decide whether the current proposal generator is the ceiling.

Tasks:

1. Compare accepted vs rejected proposal rows across original, seed7642, and
   seed7643.
2. Measure whether safe rows share stable proposal atoms, scale values,
   margin features, fanout, or trace signatures.
3. Identify whether the near-zero M768 result is caused by:
   - too few safe proposals;
   - unstable safe proposals across seeds;
   - a policy feature that cannot separate safe rows;
   - task-specific proposal behavior.
4. If safe-positive rows are rare and unstable, stop policy work and rebuild
   proposal generation.

Acceptance:

- clear per-task and per-feature explanation of why rows survive or fail;
- recommendation: rebuild proposal generator, continue policy, or move online.

### M771: Native Online Accept/Fallback Smoke

Goal: test whether rank-trace is useful as an execution-time safety check,
not a fixed research threshold.

Tasks:

1. Use the native candidate context to score a small candidate delta.
2. Apply the delta only when cheap query-local trace checks are safe.
3. Fallback to baseline P1 when the candidate is unsafe or uncertain.
4. Evaluate through native shared15-style query execution, not a new offline
   scan-only path.

Acceptance:

- no dense-overlap spend;
- no negative tasks;
- measurable MAP/NDCG/MRR gain beyond M768 diagnostic scale;
- runtime cost small enough to fit the engineering index path.

## Recommendation

Run M770 first.  If the audit shows no stable safe-positive proposal region,
do not build M771 yet.  Rebuild the proposal generator around the failure
analysis.

If M770 finds stable proposal signatures that the current global threshold
cannot exploit, then M771 is worth implementing as a native online
accept/fallback layer.
