# II-42 M506d Adaptive Fanout

## Purpose

M506c proved that TRECCOVID's M506b failure is mainly a fixed-fanout coverage
problem. M506d tests the simplest non-cheating dynamic policy:

```text
if base-prefix candidate touched ratio is low, use a larger prefix
```

Policy inputs:

- base-prefix candidate count;
- corpus size;
- fixed thresholds.

The policy uses no qrels, no dataset name, and no BM25. Qrels remain
evaluation-only.

## Implementation

Script:

- `scripts/research_sae_m506d_adaptive_fanout.py`

Artifacts:

- `outputs/m506d/broad4_count_adaptive_seed5062_gate/`
- `outputs/m506d/broad4_count_adaptive_gate/`

Main reported run:

- seed: 5062;
- tasks: ArguAna, FiQA2018, SCIDOCS, TRECCOVID;
- support head: `rotation_residual`;
- base prefix: 256;
- mid prefix: 512;
- high prefix: 768;
- mid threshold: 0.30;
- high threshold: 0.20;
- eval queries: 25 per task;
- no BM25.

## Main Broad4 Result

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.46462 | 0.58257 | 0.53588 | 0.22951 | 1.00000 | 1.00000 | 1.00000 |
| `m506d_rotation_residual_fixed_p768` | 0.45285 | 0.57231 | 0.52387 | 0.21822 | 0.83470 | 0.92280 | 0.75818 |
| `m506d_rotation_residual_adaptive_count` | 0.45246 | 0.57631 | 0.52231 | 0.21885 | 0.81150 | 0.87850 | 0.59705 |
| `m506d_rotation_residual_fixed_p512` | 0.45156 | 0.56869 | 0.53485 | 0.21485 | 0.79840 | 0.87610 | 0.68362 |
| `m506d_rotation_residual_fixed_p256` | 0.42581 | 0.56685 | 0.53148 | 0.20918 | 0.73040 | 0.78740 | 0.54270 |
| `m506d_structural_fixed_p256` | 0.37460 | 0.52670 | 0.48006 | 0.17253 | 0.60170 | 0.63840 | 0.55523 |

Count-adaptive fanout recovers nearly all fixed-prefix 768 quality while
touching far fewer documents:

- adaptive vs fixed 256: +0.02665 NDCG@10, +0.09110 candidate recall,
  +0.05435 touch;
- adaptive vs fixed 512: +0.00090 NDCG@10, -0.08657 touch;
- adaptive vs fixed 768: -0.00039 NDCG@10, -0.16113 touch.

## Per-Task Adaptive Behavior

| Task | NDCG@10 | Dense O@100 | Cand R@100 | Touch | Prefix Hist |
| --- | ---: | ---: | ---: | ---: | --- |
| ArguAna | 0.38418 | 0.92800 | 0.99960 | 0.96578 | `{'256': 25}` |
| FiQA2018 | 0.43387 | 0.79400 | 0.85800 | 0.37869 | `{'256': 25}` |
| SCIDOCS | 0.20314 | 0.85880 | 0.95080 | 0.66856 | `{'256': 25}` |
| TRECCOVID | 0.78867 | 0.66520 | 0.70560 | 0.37517 | `{'768': 25}` |

The policy did the intended thing:

- it did not inflate already-covered ArguAna, FiQA, or SCIDOCS queries;
- it promoted all low-coverage TRECCOVID heldout queries to prefix 768;
- TRECCOVID reached NDCG@10 0.78867, effectively matching the row-int8 dense
  teacher for this sampled split.

## Replication Seed

A second seed produced the same direction:

| Source | NDCG@10 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.48652 | 1.00000 | 1.00000 | 1.00000 |
| `m506d_rotation_residual_fixed_p768` | 0.45429 | 0.83720 | 0.92310 | 0.75928 |
| `m506d_rotation_residual_fixed_p512` | 0.44975 | 0.80240 | 0.87810 | 0.68337 |
| `m506d_rotation_residual_adaptive_count` | 0.44842 | 0.80930 | 0.87460 | 0.59675 |
| `m506d_rotation_residual_fixed_p256` | 0.43014 | 0.72630 | 0.78160 | 0.54143 |

The replicated run shows the same tradeoff: adaptive is much better than fixed
256 and close to fixed 512/768 at lower touch.

## Interpretation

M506d is a real route improvement, but it is still a hard-threshold controller.
It proves that qrels-free dynamic fanout can recover dense-like quality without
blindly raising prefix for every query.

The route is not exhausted. The next bottleneck is policy calibration:

- detect low-coverage queries more smoothly than two fixed thresholds;
- avoid overusing prefix 768 when prefix 512 is enough;
- use richer qrels-free signals such as support entropy, score margin, and
  structural/sketch agreement.

## Decision

Promote:

- qrels-free dynamic fanout as the next mainline;
- support-head + structural-score split;
- count-based fallback as the first safe controller.

Do not promote yet:

- dataset-specific fanout;
- BM25 rescue;
- learned scorer replacement;
- RL/ranking-aware tuning.

## Next Step

M506e should replace the hard threshold with a calibrated qrels-free fanout
gate and run Broad10 sampled:

1. collect per-query signals from prefix 256, 512, and 768;
2. train or tune a small gate against dense-teacher coverage, not qrels;
3. target adaptive quality close to fixed 768 with touch closer to fixed 256;
4. only then move to M507 ranking-aware optimization.
