# M766 Held-Out Rank-Trace Replay

## Decision

M764's rank-trace feedback surface is real, but a single fixed threshold is not
stable enough to promote.

The fixed top16 guard learned on the original split failed on an alternate
seed/split. Re-running the M764 guard search on the alternate split still found
a no-negative policy, but the useful gain became much smaller. This means the
rank-trace signal is valid, while threshold calibration is not yet robust.

## Held-Out Surface

M766 regenerated the top16 global proposal replay with a different seed:

```text
runs/m766_global_proposal_context_top16_seed7642_v1
```

This produced:

- dev attempts: 4272
- test attempts: 4336
- same shared15 root
- same qrels-free top16 global proposal budget

## Fixed Guard Replay

Fixed guard from M764 top16:

```json
{
  "feature": "trace_position_keep_at_128",
  "mode": "le",
  "threshold": 0.4765625
}
```

| Split | Applied | Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dev | 57 | dbpedia-entity, hotpotqa, scidocs | 0 | -0.000250 | -0.000102 | +0.000119 | +0.000000 | +0.000000 |
| test | 58 | trec-covid | 1 | +0.000225 | +0.000203 | +0.000185 | +0.000042 | +0.000000 |

The test macro is still positive, but the fixed guard violates the per-task
robustness requirement.

## Re-Search On Held-Out Split

When the M764 guard search is rerun on seed7642 dev, it finds a clean test
policy:

```json
{
  "feature": "trace_mean_drop_at_128",
  "mode": "ge",
  "threshold": 0.6875
}
```

| Split | Applied | Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dev | 8 | none | 1 | +0.000002 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| test | 7 | none | 1 | +0.000025 | +0.000022 | +0.000000 | +0.000000 | +0.000000 |

This confirms the rank-trace surface is useful, but the guard is currently
split-calibrated and conservative.

## Interpretation

M764 is not a false positive, but it is not ready as a fixed production policy.

What remains true:

- rank-trace feedback can remove M760 negative rows;
- dense overlap remains intact;
- qrels-free local replay adds information unavailable to M758/M762/M763;
- the signal survives an alternate seed if recalibrated.

What failed:

- a single fixed trace threshold did not transfer;
- independent recalibration reduced gains to near diagnostic size;
- top16 remains the only plausible budget, while top32/top64 collapse toward
  no-op under strict rank-trace guards.

## Next Step

Do not promote M764 directly.

The next useful route is M767 multi-seed calibration:

1. collect 3-5 alternate split replays for top16;
2. evaluate a small family of rank-trace guards across all splits;
3. choose only guards that pass every split without negative tasks;
4. measure whether any guard keeps non-trivial MAP/NDCG/MRR gain.

If no guard survives multi-seed calibration with non-trivial gain, the
deterministic policy line should stop as a diagnostic.  The next model work
would need a stronger proposal generator or true query-local native feedback
inside the engineering path.
