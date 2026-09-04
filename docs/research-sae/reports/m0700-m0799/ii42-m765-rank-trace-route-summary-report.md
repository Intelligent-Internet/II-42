# M765 Rank-Trace Route Summary

## Decision

M764 is a real improvement over M758/M760/M762/M763.

The useful route is now:

```text
M754 qrels-free global proposal pool
  -> M758 deterministic native-context policy
  -> M764 query-local rank-trace feedback guard
```

This is not another global selector.  The new signal is qrels-free query-local
rank movement inside the native scorer after applying a candidate posting
delta.  It directly targets the M760 failure mode: small MAP/NDCG/MRR
regressions caused by rank geometry changes that do not show up as top100
membership loss.

## Result Matrix

| Run | Budget | Guard | Applied | Negative Tasks | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M758 | top16 | none | 224 | fiqa, scidocs | +0.000246 | +0.000332 | +0.000000 | +0.000032 | +0.000000 | 1 |
| M764 | top16 | `trace_position_keep_at_128 <= 0.4765625` | 64 | none | +0.000369 | +0.000280 | +0.000185 | +0.000004 | +0.000000 | 1 |
| M764 | top32 | `trace_position_keep_at_128 >= 0.7109375` | 4 | none | +0.000000 | +0.000078 | +0.000000 | +0.000000 | +0.000000 | 1 |
| M764 | top64 | `trace_max_drop_at_100 <= 1.0` | 35 | none | +0.000008 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |

## Interpretation

The route should concentrate on top16.

Top16 with rank-trace feedback:

- removes the M760 negative dataset rows;
- improves MAP more than raw M758 top16;
- adds positive MRR movement;
- keeps dense overlap unchanged;
- keeps a non-zero query count, unlike top32/top64 strict guards.

Top32/top64 are no longer attractive in their current form.  Their rank-trace
guards can remove negative datasets, but only by shrinking the accepted set to
near no-op.

## What Changed

M762 and M763 failed because their features were too coarse:

- aggregate score/margin deltas;
- selected-row risk classifier;
- geometry clusters.

M764 uses a different surface:

- baseline top-k internal rank displacement;
- max rank drop;
- position-keep ratio;
- rank correlation.

This explains why it can catch regressions that M758/M762 missed while still
preserving the positive rows.

## Next Step

Do not tune more thresholds on the same split.

M766 should be an independent replay:

1. freeze the top16 proposal budget;
2. freeze the M758 top16 deterministic policy;
3. freeze the M764 top16 rank-trace guard;
4. rerun on a different shared15 seed/split or an equivalent held-out native
   replay surface;
5. accept only if:
   - no negative datasets;
   - dO@100 remains zero;
   - MAP/NDCG/MRR stay non-negative;
   - gain is not near-zero.

If M766 passes, promote M764 top16 to the next broader native validation.  If
M766 fails, keep M764 as a split-positive diagnostic and stop the deterministic
policy line until a stronger proposal generator exists.
