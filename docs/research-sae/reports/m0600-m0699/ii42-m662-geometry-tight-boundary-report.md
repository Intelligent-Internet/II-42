# M662 / Geometry-Tight Boundary Report

Status: `geometry_tightening_negative_test_regression`

M662 tests whether the M661 positive retrieval signal can be made acceptable by
simple parameter tightening:

```text
same missing-positive boundary objective as M661
lower learning rate / fewer steps
higher faithfulness weight
higher support-preservation weight
```

This is not a new objective.  It is a stability check for M661.

## Setup

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m662_geometry_tight_boundary_spark.sh` |
| Shared root | `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Init checkpoint | M658 `m658_support_geometry_smoke_seed6581.compiler.pt` |
| Device | `cpu` in Docker |

Artifact root:

```text
runs/ii42-m662-geometry-tight-boundary-v1/
  m662_geometry_tight_seed6621/
```

CUDA failed to initialize inside Docker again, so this ran on CPU.

## Parameter Changes vs M661

| Parameter | M661 | M662 |
| --- | ---: | ---: |
| max train steps | `220` | `160` |
| learning rate | `0.00035` | `0.00020` |
| faithfulness weight | `2.0` | `4.0` |
| support-preservation weight | `0.60` | `1.20` |
| rank mode | `missing_positive` | `missing_positive` |
| support top/core k | `100 / 100` | `100 / 100` |

## Dev Signal

The dev gate passed strongly.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | +0.00045001 |
| Recall@100 | +0.00004058 |
| dense overlap@100 | -0.00026042 |
| MAP@100 | +0.00030501 |
| MRR@20 | +0.00028973 |
| NDCG@10 | +0.00097089 |

This looks better than M661 on dev, but it does not hold on test.

## Test Signal

The final test decision failed because MAP regressed.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| candidate upper bound | +0.00000000 |
| Recall@100 | +0.00031101 |
| dense overlap@100 | -0.00058594 |
| MAP@100 | -0.00039698 |
| MRR@20 | -0.00046862 |
| NDCG@10 | -0.00018390 |

M662 still recovers Recall, but it gives up the MAP/MRR stability that made
M661 more promising.

## Test Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.836677 | 1.000000 | 0.434924 | 0.251049 | 0.630678 | 0.460447 |
| M549 target | 0.836562 | 0.995336 | 0.434247 | 0.251167 | 0.630127 | 0.460439 |
| M658 baseline | 0.836632 | 0.994770 | 0.434188 | 0.251100 | 0.630127 | 0.460367 |
| M662 compiler | 0.836632 | 0.994184 | 0.434004 | 0.250703 | 0.630438 | 0.459899 |

## Geometry Gate

The strict geometry gate failed and was worse than M661.

| Metric | Delta vs M658 baseline |
| --- | ---: |
| doc active recall | +0.00000000 |
| query active recall | +0.00000000 |
| doc support cosine | -0.00000405 |
| query support cosine | -0.00000422 |
| top100 overlap vs dense | +0.00000000 |
| top100 overlap vs M549 | -0.00069062 |

M662 kept active recall and dense top100 overlap, but it did not preserve M549
geometry.  Increasing generic faithfulness/support weights is not sufficient.

## Decision

Do not promote M662.  Keep M661 as the more valuable positive signal.

M662's main lesson:

```text
parameter-level tightening does not fix the M661 geometry blocker
```

The likely reason is that the current support-preservation loss protects score
ordering inside a candidate set, but the failing geometry floors are M549
support-shape and M549 top100-overlap properties.  The loss is not directly
optimizing the failing quantities.

## Next Step

M663 should not continue simple weight tuning.  It should add one direct
constraint:

1. either an explicit M549 topK overlap / boundary-pair preservation loss;
2. or a support-cosine floor surrogate computed against the M658 checkpoint;
3. or a selection gate that requires geometry checker pass before writing a
   promoted checkpoint.

The most direct next probe is M663:

```text
M661 missing-positive boundary
+ explicit M549 top100 boundary preservation penalty
+ same retrieval gate
+ same strict geometry checker
```

If M663 preserves M661's Recall gain and passes geometry, expand to a broader
smoke.  If it fails, stop this output-head retrieval-pressure sub-branch and
move to candidate-upper-bound shape analysis.

## Verification

- `python3 -m py_compile scripts/train_m636_retrieval_constrained_compiler.py scripts/check_m659_bounded_retrieval_geometry.py`
- `python3 -m pytest tests/test_m636_retrieval_constrained_compiler.py tests/test_check_m659_bounded_retrieval_geometry.py -q`
- `bash -n scripts/run_m662_geometry_tight_boundary_spark.sh scripts/run_m661_missing_positive_boundary_spark.sh`
- `git diff --check`
