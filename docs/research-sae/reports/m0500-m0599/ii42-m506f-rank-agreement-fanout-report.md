# II-42 M506f Rank-Agreement Fanout Report

## Summary

M506f tested a qrels-free dense-rank-agreement fanout gate for the PPLX-root
posting route.  The gate calibrates prefix thresholds on train queries by
matching the route ranking to the dense teacher Top100, rather than by matching
candidate coverage only.

Result: M506f is a useful diagnostic but is not promoted.  On Broad10 sampled,
it improves candidate recall over M506d hand-count, but loses a small amount of
NDCG and uses more fanout.  M506d hand-count remains the best dynamic policy for
now, while fixed prefix 768 remains the quality upper point.

## Method

- Teacher: `row_int8` PPLX dense surface.
- Candidate/support head: `rotation_residual`.
- Scorer: unchanged structural dense-tail scorer from M506.
- Prefixes: `256`, `512`, `768`.
- Gate fitting signal: train-query overlap between route-ranked Top100 and
  dense-teacher Top100.
- Heldout evaluation: qrels metrics are used only after gate fitting.

Outputs:

- `outputs/m506f/broad4_seed5062_gate/m506f_broad4_seed5062_gate.json`
- `outputs/m506f/broad4_seed5062_gate/m506f_broad4_seed5062_gate.md`
- `outputs/m506f/broad10_seed5062_sampled/m506f_broad10_seed5062_sampled.json`
- `outputs/m506f/broad10_seed5062_sampled/m506f_broad10_seed5062_sampled.md`

## Broad10 Sampled Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_materialized_dense` | 0.55000 | 0.71905 | 0.63344 | 0.41006 | 0.99608 | 1.00000 | 1.00000 |
| `teacher_row_int8_dense` | 0.54998 | 0.71907 | 0.63308 | 0.40987 | 1.00000 | 1.00000 | 1.00000 |
| `m506f_rotation_residual_fixed_p768` | 0.50778 | 0.68397 | 0.59269 | 0.36061 | 0.79648 | 0.89860 | 0.64421 |
| `m506f_rotation_residual_hand_count` | 0.49956 | 0.66154 | 0.58598 | 0.35247 | 0.76016 | 0.83376 | 0.47409 |
| `m506f_rotation_residual_fixed_p512` | 0.49713 | 0.67137 | 0.58844 | 0.34827 | 0.75432 | 0.83836 | 0.55034 |
| `m506f_rotation_residual_rank_agreement_count` | 0.49711 | 0.66355 | 0.58358 | 0.34813 | 0.75948 | 0.84552 | 0.53849 |
| `m506f_rotation_residual_fixed_p256` | 0.46188 | 0.60927 | 0.56445 | 0.32480 | 0.65240 | 0.70224 | 0.39368 |
| `m506f_structural_fixed_p256` | 0.39706 | 0.53972 | 0.49705 | 0.26185 | 0.55556 | 0.58468 | 0.41155 |

## Broad4 Gate Check

| Source | NDCG@10 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.46462 | 1.00000 | 1.00000 | 1.00000 |
| `m506f_rotation_residual_fixed_p768` | 0.45285 | 0.83470 | 0.92280 | 0.75818 |
| `m506f_rotation_residual_hand_count` | 0.45246 | 0.81150 | 0.87850 | 0.59705 |
| `m506f_rotation_residual_rank_agreement_count` | 0.45222 | 0.81880 | 0.89820 | 0.65941 |
| `m506f_rotation_residual_fixed_p512` | 0.45156 | 0.79840 | 0.87610 | 0.68362 |
| `m506f_rotation_residual_fixed_p256` | 0.42581 | 0.73040 | 0.78740 | 0.54270 |

## Interpretation

The rank-agreement gate is better aligned with the intended failure mode than
M506e coverage calibration.  It chooses larger prefixes on sparse-support
queries and smaller prefixes where route agreement remains stable.

However, the broad10 matrix shows that agreement-calibrated fanout still does
not beat the hand-count dynamic policy:

- M506f rank-agreement count: NDCG@10 `0.49711`, Touch `0.53849`.
- M506d/M506f hand-count baseline: NDCG@10 `0.49956`, Touch `0.47409`.
- Fixed p768 quality point: NDCG@10 `0.50778`, Touch `0.64421`.

This means the current dynamic policy family is still missing a ranking-quality
signal.  Dense-rank agreement is useful, but optimizing it through only
base-ratio thresholds is too weak.  The next stage should not keep tuning
thresholds.  It should train or score the admission/ranking decision directly
under hard preservation constraints.

## Decision

- Do not promote M506f as the default dynamic fanout policy.
- Keep M506d hand-count as the current efficiency/quality default.
- Keep fixed p768 as the quality upper point for this route.
- Move M507 forward only if it keeps dense teacher preservation and fanout caps
  explicit in the objective.

## Recommended Next Step

M507 should test ranking-aware fine-tuning/admission with hard constraints:

- preserve dense-teacher Top100 overlap;
- keep fanout no worse than p768 and preferably near M506d;
- optimize route ranking agreement or listwise dense score on sampled groups;
- use qrels only for heldout reporting unless running a clearly separated
  train/dev qrels-aware experiment.

The immediate experiment should be a small learned admission/rerank gate over
the existing candidate/support head, not another hand-crafted threshold sweep.
