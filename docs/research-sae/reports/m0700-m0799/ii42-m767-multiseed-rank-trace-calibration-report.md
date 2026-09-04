# M767 Multi-Seed Rank-Trace Calibration

## Decision

The M764 rank-trace feedback surface is valid, but the high-gain guard does not
generalize as a fixed policy.

Two alternate seed replays show the same pattern:

- fixed original guard fails;
- re-searching a rank-trace guard on the alternate dev split can find a clean
  test policy;
- the clean held-out gain is much smaller than the original M764 top16 result.

This means the line should not be promoted as a fixed deterministic policy yet.

## Fixed Guard

Original M764 top16 guard:

```json
{
  "feature": "trace_position_keep_at_128",
  "mode": "le",
  "threshold": 0.4765625
}
```

## Results

| Surface | Mode | Applied | Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original split | search | 64 | none | 1 | +0.000369 | +0.000280 | +0.000185 | +0.000004 | +0.000000 |
| seed7642 | fixed | 58 | trec-covid | 1 | +0.000225 | +0.000203 | +0.000185 | +0.000042 | +0.000000 |
| seed7642 | search | 7 | none | 1 | +0.000025 | +0.000022 | +0.000000 | +0.000000 | +0.000000 |
| seed7643 | fixed | 51 | dbpedia-entity, hotpotqa, nfcorpus, scidocs | 0 | -0.000288 | -0.000145 | +0.000000 | +0.000038 | +0.000000 |
| seed7643 | search | 14 | none | 1 | +0.000021 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

## Interpretation

Rank-trace features are the first feedback surface that directly removes
M760-style negative tasks on the original split while preserving non-zero
ranking gains.  However, the threshold is split-sensitive.

The fixed guard does not survive multi-seed validation:

- seed7642 keeps macro gains but has a trec-covid negative row;
- seed7643 fails the global gate and regresses several datasets.

Re-searching per split shows the surface is not noise, but the safe version is
too conservative.  The robust gain becomes diagnostic rather than deployable.

## Updated Stop/Continue Rule

Stop doing single-threshold deterministic policy tuning on this line.

Continue only if the next experiment changes one of these structural pieces:

1. learn or derive a multi-seed calibrated rank-trace guard from several splits,
   not one dev split;
2. improve the proposal generator so the trace guard has safer rows to choose
   from;
3. move the rank-trace check into the native engineering path as an online
   accept/fallback diagnostic, not as a fixed research threshold.

The most pragmatic next step is option 1 as a bounded calibration experiment.
If multi-seed calibration still collapses to near-zero, stop this route and
return to proposal generation.
