# ii42 M1141 Protected-Tail Cross-Surface Summary

## Purpose

M1140 found a structural replay that improves shared15: keep the current best
base run's top-rank band fixed, then use the dual-gamma run only as a tail
expansion source.

M1141 checks whether the same policy transfers across existing shared5 and
shared8 ranking exports before spending compute on native replay or training.

## Inputs

| Surface | Base | Tail | Base shape | Tail shape |
| --- | --- | --- | --- | --- |
| shared5 | M1126 top-rank listwise | M1135 dual-gamma | `a0.35,g1.00` | `a0.50,g0.75` |
| shared8 | M1128 listwise control | M1136 dual-gamma | `a0.50,g1.00` | `a0.60,g0.50` |
| shared15 | M1129 listwise control | M1137 dual-gamma | `a0.60,g1.00` | `a0.50,g0.50` |

All rows are replayed on heldout ranking exports with the same deterministic
policy family:

1. append base top `K`;
2. append tail ranking without duplicates;
3. append the remaining base ranking.

No new model was trained.

## Cross-Surface Result

### Baseline and tail-only global runs

| Surface | Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| shared5 | base | 0.717658 | 0.578506 | 0.515893 | 0.411211 |
| shared5 | tail global | 0.724981 | 0.598489 | 0.526144 | 0.423195 |
| shared8 | base | 0.658735 | 0.673472 | 0.578677 | 0.441150 |
| shared8 | tail global | 0.665262 | 0.680417 | 0.582149 | 0.444132 |
| shared15 | base | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| shared15 | tail global | 0.782350 | 0.762162 | 0.698423 | 0.598732 |

The global tail run is mixed: positive on shared5/shared8, but it damages
rank-quality on shared15. This is why a protected composition is needed.

### `protect20_then_tail` delta versus base

| Surface | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| shared5 | +0.013871 | +0.000000 | +0.000000 | +0.002117 |
| shared8 | +0.012925 | +0.000000 | +0.000000 | +0.001568 |
| shared15 | +0.010714 | +0.000000 | +0.000245 | +0.001672 |

`protect20_then_tail` is stable across all three surfaces. It improves Recall
and MAP, while preserving MRR/NDCG. This is materially different from the prior
gamma/alpha selector attempts: the win comes from rank-region separation, not
from a global score-shape swap.

## Protect-K Behavior

The broad pattern is consistent:

- `protect5` can be stronger on some heldout rows but is more aggressive and
  was not stable on train sanity.
- `protect10` is good but still leaves tiny top-rank risk on train sanity.
- `protect20` is the conservative point where top-rank metrics are preserved
  and tail recall still improves.
- larger `protectK` values keep rank safety but gradually give up tail recall.

## Interpretation

This is the first post-M1129 route with a clean structural explanation and
cross-surface validation:

- M1129/Listwise controls top-rank precision.
- M1135/M1136/M1137 dual-gamma exposes additional useful tail candidates.
- Global use of the dual-gamma scorer can damage rank geometry.
- Protected-tail composition recovers the candidate value without spending the
  top-rank band.

This directly addresses the M1137-M1139 failure: the useful M1137 signal was
not separable by query scalar or simple candidate-pair classifiers, but it is
usable when constrained to the tail region.

## Verdict

Promote `protect20_then_tail` to the next validation route.

This does not yet complete the goal. The current evidence is export-replay
evidence, not native DB/plugin evidence. The next step should be engineering
validation:

1. implement or reuse a deterministic protected-tail composer on native
   candidate streams;
2. replay at least shared15 through the native path;
3. compare against BM25, dense, base P1/M1129, and the tail run;
4. only then consider distilling the policy into a model target.

## Stop Conditions

Stop this route if any of the following happens:

- native replay cannot reproduce the export replay;
- gains vanish on another seed/split;
- gains are dominated by one dataset while macro still hides losses;
- implementation requires dataset-specific thresholds or per-dataset tuning;
- top-rank preservation fails outside the current ranking-export surface.
