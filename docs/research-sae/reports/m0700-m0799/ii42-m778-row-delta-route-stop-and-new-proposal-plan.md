# M778 Row-Delta Route Stop and New Proposal Plan

## Decision

Stop the current M754/M758 row-delta route as a main improvement path.

M777 fast smoke confirms the failure mode predicted by M776:

- adding qrels-free damage bounds before deterministic row selection can make
  policies clean;
- but the clean policies collapse to no-op or diagnostic scale;
- the M771 oracle ceiling is not preserved by simple damage-bounding over the
  existing top16 proposal surface.

This means the next stage should not be another filter, selector, or veto over
the same row pool.  It needs a new proposal mechanism.

## Evidence Chain

| Run | Question | Result |
| --- | --- | --- |
| M768 | Can multi-seed rank-trace guard be clean? | yes, but only diagnostic gain |
| M770 | Are safe-positive rows present? | yes, 78 strict-positive rows in test |
| M771 | Is there oracle ceiling in current pool? | yes, dMAP about +0.000261 |
| M772 | Can a generic trained selector capture it? | no, task-local regressions |
| M774 | Do richer native damage features fix selector? | no, still not clean |
| M775 | Does a global damage veto fix selector? | no, failures move tasks |
| M777 | Can damage bounds before row selection preserve gain? | no, clean result is no-op |

M777 best clean test policy:

- score: `margin_bundle`;
- bound: `trace_mean_abs_at_100 <= 0.5`;
- applied: 1.7 queries;
- dMAP: about +0.00000027;
- dNDCG/MRR/CUB/O@100: zero.

This is far below both M771 oracle ceiling and M768 diagnostic guard.

## Interpretation

The existing top16 row-delta surface is not a good substrate for a deployable
improvement.  It contains useful rows, but the useful rows are not separable
from task-damaging rows by the available qrels-free native context and damage
features.

The problem is now upstream:

- candidate deltas are too local and brittle;
- dimension/scale rows are selected from historical accepted counts, not from
  stable damage-bounded utility;
- useful gains require task-local decisions that current global features do not
  expose.

## What To Keep

Keep:

- P1/P1.3 native unified posting substrate;
- M764 rank-trace diagnostics;
- M770 proposal-quality audit;
- M771 oracle ceiling harness;
- M774 damage-witness feature code.

Do not keep:

- M772/M774/M775 selector policies;
- M777 damage-bounded deterministic policies;
- the current M754/M758 top16 proposal surface as the main route.

## Next Proposal Mechanism

The next route should generate proposals from a different unit than
`single dim x direction x scale`.

Recommended M779 direction: stable bundle proposal generation.

Core idea:

- instead of single-coordinate deltas, propose small bundles of coordinates;
- bundle construction is qrels-free and based on native score geometry:
  co-moving coordinates, low crossing damage, stable score lift, and low
  reciprocal-rank loss;
- evaluate bundles through the same native shared15 harness;
- only after bundle proposals show an oracle ceiling should selector training
  resume.

M779 minimal smoke:

1. Build candidate bundles from top M754/M749 coordinate signatures:
   - pair or triple coordinates with same direction;
   - require low per-coordinate damage witness;
   - cap total scale.
2. Replay bundle deltas on shared15 original + seed7642 + seed7643.
3. Measure:
   - strict-positive mass;
   - utility-positive mass;
   - task-damaging rows;
   - oracle ceiling;
   - dense overlap and CUB floors.
4. Stop immediately if bundle oracle ceiling is not above M768 by 5x.
5. Only if the bundle ceiling is real, train a selector.

Acceptance:

- clean oracle ceiling remains at least M771/M768-scale, preferably above
  dMAP +0.0001;
- task-damaging row rate is lower than M754/M758;
- no dense-overlap spend;
- no per-dataset tuning.

Stop condition:

If bundle proposals do not produce a clean oracle ceiling, the deterministic
query-local row/bundle route should be stopped.  The next serious route would
then be a learned posting compiler that changes the proposal mechanism at the
model/output level, not another native row filter.
