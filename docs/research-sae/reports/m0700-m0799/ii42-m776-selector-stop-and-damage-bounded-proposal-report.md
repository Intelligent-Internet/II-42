# M776 Selector Stop and Damage-Bounded Proposal Route

## Decision

Stop the current accept-selector route.

M771 showed a real oracle accept ceiling inside the M758 proposal pool, but
M772, M774, and M775 show that the current inference-available feature surface
cannot learn a clean global selector.  The failure is not caused by a missing
single threshold or a weak model.  Negative tasks move between surfaces and
tasks when we add features or vetoes.

The next route should return upstream to proposal generation, but with a new
constraint: generated proposals must be damage-bounded by construction.

## Evidence

| Run | Change | Clean | Gate | Applied | dMAP | dNDCG | dMRR | Main Failure |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M768 | multi-seed rank-trace guard | 1 | 1 | 6.0 | +0.000005 | +0.000000 | +0.000000 | diagnostic-scale only |
| M771 | oracle accept ceiling | 1 | 1 | 26.0 | +0.000261 | +0.000227 | +0.000123 | not deployable oracle |
| M772 | trained selector | 0 | 1 | 6.0 | +0.000028 | +0.000073 | +0.000000 | `trec-covid` |
| M774 | failure-aware features | 0 | 1 | 2.7 | +0.000018 | +0.000034 | +0.000000 | `trec-covid` |
| M775 | selector + global damage veto | 0 | 0 | 22.7 | +0.000029 | +0.000044 | -0.000062 | task damage moves |

M775 tested one global qrels-free damage veto on top of the M774 selector.
The best candidates still failed:

- HGB strict-positive: `dbpedia-entity`, `scidocs`, `trec-covid`,
  `webis-touche2020`;
- Logistic strict-positive: `climate-fever`, `scidocs`, `dbpedia-entity`;
- Logistic utility-positive: `scidocs`, `trec-covid`;
- HGB utility-positive with crossing veto: `cqadupstack`, `dbpedia-entity`,
  `scidocs`, `trec-covid`, `webis-touche2020`.

This is a structural failure: the selector is choosing rows that are locally
plausible but not task-robust.

## What We Keep

Keep these pieces:

- M754/M758 proposal rows as the current baseline proposal surface;
- M764 rank-trace features as a cheap qrels-free damage diagnostic;
- M770 proposal quality audit;
- M771 oracle ceiling as proof that useful rows exist;
- M774 native damage-witness feature code for future proposal filtering.

Do not keep M772/M774/M775 trained selectors as deployable policies.

## Diagnosis

The current proposal pool mixes three row types:

1. safe-positive rows that improve ranking without dense-overlap spend;
2. no-op rows that survive robust guards but give almost no gain;
3. task-damaging rows that look safe in macro or query-local features.

The selector sees query-local qrels-free features only.  It cannot reliably
separate type 1 from type 3 across seeds and tasks.  Adding more generic
damage features shrinks or shifts the failures, but does not eliminate them.

That means the next useful change is not a better selector over the same row
pool.  The proposal generator itself must produce fewer type-3 rows.

## Next Route: Damage-Bounded Proposal Generator

M777 should generate a new proposal surface with safety constraints applied
before selector training.

Principles:

- keep P1/P1.3 native unified posting substrate frozen;
- do not use per-dataset tuning;
- generate candidate deltas only when qrels-free native damage is bounded;
- favor rows with stable positive evidence across seeds or bootstrap subsets;
- preserve dense overlap and candidate upper bound floors.

Concrete M777 plan:

1. Build proposal rows with native damage bounds:
   - max crossing at top20/top50/top100;
   - reciprocal-rank loss floor;
   - trace displacement ceiling;
   - top-k entropy/score-shape shift ceiling;
   - candidate upper-bound and dense-overlap floor.
2. Rank proposals by positive native context evidence:
   - top-k score lift;
   - margin lift;
   - repeated acceptance across bootstrap train slices;
   - stable dimension/direction signatures across seeds.
3. Export a new top16/top32 proposal surface.
4. Replay M770/M771:
   - if oracle ceiling is still real and type-3 rows shrink, retry selector;
   - if oracle ceiling collapses, proposal constraints are too strict;
   - if type-3 rows remain common, abandon row-level delta selection.

Acceptance:

- M777 proposal surface has strict-positive mass comparable to M770;
- M777 reduces task-damaging rows relative to M754/M758;
- M771-style oracle ceiling remains above M768 by at least 5x;
- M772-style selector has a realistic chance to pass clean.

Stop Condition:

If damage-bounded proposal generation cannot preserve oracle ceiling while
reducing task-damaging rows, stop this entire deterministic row-delta route.
At that point the next real route is a different posting compiler/proposal
mechanism, not more filtering.
