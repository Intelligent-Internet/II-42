# M1293 Query-Delta Stability Grid

## Question

M1251 showed that `signed_sum_s1` is macro-positive on full shared15, while
M1252/M1253 showed it is not row-stable enough to become a native default.
M1293 asks whether this is just a scale/geometry issue.

This is not a new selector.  It reuses the same native LODO replay path and
compares fixed query-delta modes/scales before any objective training.

## Setup

- Surface: M1252 risk datasets
- Datasets:
  `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- Query count: `599`
- Modes: `signed_sum`, `mean_teacher`
- Scales: `0.5`, `0.75`, `1.0`
- Output:
  `runs/m1293_query_delta_stability_grid_risk7_v1/m1293_stability.json`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `mean_teacher_s0.75` | 7.673 | 0 | +0.001291 | +0.001114 | +0.001019 | +0.000067 | +0.000037 | +0.012005 |
| `signed_sum_s1` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | +0.005682 |
| `mean_teacher_s1` | 7.673 | 1 | +0.000463 | +0.001098 | +0.000710 | -0.000260 | +0.000966 | +0.004869 |
| `signed_sum_s0.75` | 7.816 | 1 | +0.001663 | +0.000867 | +0.000828 | -0.000853 | +0.001084 | +0.003419 |
| `signed_sum_s0.5` | 7.816 | 1 | +0.000904 | +0.000604 | +0.000745 | -0.001142 | +0.000483 | -0.005400 |
| `mean_teacher_s0.5` | 7.673 | 2 | +0.000629 | +0.000542 | +0.000343 | -0.000232 | -0.000336 | -0.006078 |

`mean_teacher_s0.75` is the best fixed replay geometry on this risk surface.
It is macro-clean, unlike `signed_sum_s1`, which still loses MRR on this
surface.

## Row-Level Stability

The best variant still has row-level negative datasets:

| Variant | NegativeDatasets | Datasets |
| --- | ---: | --- |
| `mean_teacher_s0.75` | 4 | `cqadupstack`, `webis-touche2020`, `dbpedia-entity`, `trec-covid` |
| `signed_sum_s1` | 5 | `cqadupstack`, `fiqa`, `dbpedia-entity`, `scidocs`, `trec-covid` |
| `mean_teacher_s1` | 5 | `cqadupstack`, `webis-touche2020`, `nfcorpus`, `dbpedia-entity`, `trec-covid` |
| `signed_sum_s0.75` | 5 | `cqadupstack`, `fiqa`, `webis-touche2020`, `dbpedia-entity`, `trec-covid` |
| `signed_sum_s0.5` | 5 | `cqadupstack`, `fiqa`, `webis-touche2020`, `dbpedia-entity`, `trec-covid` |
| `mean_teacher_s0.5` | 5 | `cqadupstack`, `webis-touche2020`, `dbpedia-entity`, `scidocs`, `trec-covid` |

## Interpretation

M1293 changes the conclusion slightly but does not solve the branch.

The good news:

- fixed geometry matters;
- `mean_teacher_s0.75` removes macro-level negative metrics on the risk
  surface;
- the source remains useful as an objective signal.

The bad news:

- row-level harm remains broad;
- the remaining harm is not eliminated by scale choice;
- this still does not satisfy the engineering-default requirement.

## Decision

Do not promote any M1293 fixed replay policy as default.

Retain `mean_teacher_s0.75` and `signed_sum_s1` as objective/source signals.
Stop replay-policy tuning unless a new source changes the row-level
observability.

## Next Direction

The next useful branch should train or construct a constrained generated
posting objective with two explicit targets:

1. ranking movement from `signed_sum_s1` / `mean_teacher_s0.75`;
2. row-safety constraints from the M1252/M1293 negative datasets and M1288
   pair/witness source.

It should not be another fixed scale, threshold, or post-hoc guard.
