# II-42 M506e Calibrated Fanout

## Purpose

M506d hand-count fanout was strong, but hand thresholds are not a final policy.
M506e tested whether thresholds can be calibrated from train-query dense
teacher coverage:

```text
train queries + dense Top100 coverage -> count thresholds
heldout queries + qrels evaluation -> report only
```

No qrels, dataset names, or BM25 are used for gate calibration.

## Implementation

Script:

- `scripts/research_sae_m506e_calibrated_fanout.py`

Artifacts:

- `outputs/m506e/broad4_seed5062_gate/`
- `outputs/m506e/broad4_seed5062_gap015_gate/`
- `outputs/m506e/broad4_seed5062_touch065_gate/`
- `outputs/m506e/broad10_seed5062_touch065_sampled_v2/`

The final Broad10 run uses:

- seed 5062;
- 10 sampled MTEB tasks;
- `rotation_residual` support head;
- base/mid/high prefixes 256/512/768;
- max calibrated train touch 0.65;
- max train groups 96;
- max eval queries 25 per task.

## Broad4 Result

| Source | NDCG@10 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.46462 | 1.00000 | 1.00000 | 1.00000 |
| `m506e_rotation_residual_fixed_p768` | 0.45285 | 0.83470 | 0.92280 | 0.75818 |
| `m506e_rotation_residual_hand_count` | 0.45246 | 0.81150 | 0.87850 | 0.59705 |
| `m506e_rotation_residual_calibrated_count` | 0.45222 | 0.81880 | 0.89820 | 0.65941 |
| `m506e_rotation_residual_fixed_p512` | 0.45156 | 0.79840 | 0.87610 | 0.68362 |
| `m506e_rotation_residual_fixed_p256` | 0.42581 | 0.73040 | 0.78740 | 0.54270 |

The calibrated gate improves candidate recall over hand-count, but costs more
touch and does not improve NDCG@10.

## Broad10 Sampled Result

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.54998 | 0.71907 | 0.63308 | 0.40987 | 1.00000 | 1.00000 | 1.00000 |
| `m506e_rotation_residual_fixed_p768` | 0.50778 | 0.68397 | 0.59269 | 0.36061 | 0.79648 | 0.89860 | 0.64421 |
| `m506e_rotation_residual_hand_count` | 0.49956 | 0.66154 | 0.58598 | 0.35247 | 0.76016 | 0.83376 | 0.47409 |
| `m506e_rotation_residual_calibrated_count` | 0.49718 | 0.66355 | 0.58368 | 0.34822 | 0.75912 | 0.84508 | 0.53848 |
| `m506e_rotation_residual_fixed_p512` | 0.49713 | 0.67137 | 0.58844 | 0.34827 | 0.75432 | 0.83836 | 0.55034 |
| `m506e_rotation_residual_fixed_p256` | 0.46188 | 0.60927 | 0.56445 | 0.32480 | 0.65240 | 0.70224 | 0.39368 |
| `m506e_structural_fixed_p256` | 0.39706 | 0.53972 | 0.49705 | 0.26185 | 0.55556 | 0.58468 | 0.41155 |

Broad10 confirms the Broad4 finding:

- fixed 768 is the quality upper point but costs more touch;
- hand-count is the best current efficiency/quality point;
- calibrated-count increases candidate recall but does not improve qrels
  ranking quality enough to justify its extra touch.

## Per-Task Notes

Calibrated-count helps or changes coverage on several tasks, but the qrels
impact is inconsistent:

- CQADupstackGaming: calibrated NDCG@10 0.46877 vs hand-count 0.47047;
- CQADupstackUnix: calibrated 0.34176 vs hand-count 0.33651;
- FiQA2018: calibrated 0.44114 vs hand-count 0.42873;
- Touche2020: calibrated 0.58200 vs hand-count 0.61905.

Touche2020 is the important counterexample: calibration lowered qrels quality
despite using more dense-coverage-driven routing. Coverage alone is therefore
not the right gate objective.

## Interpretation

M506e is a useful negative result.

It shows that:

- dense teacher coverage is necessary but not sufficient;
- optimizing candidate recall alone can over-expand fanout;
- higher Dense O@100 or Candidate R@100 does not always translate to better
  qrels ranking;
- M506d hand-count remains the promoted policy for now.

The next gate should optimize dense teacher ranking agreement or rerank
stability, not just Top100 candidate coverage.

## Decision

Promote:

- M506d hand-count as current best dynamic fanout policy;
- fixed prefix 768 as the higher-quality upper point;
- Broad10 sampled matrix as the current route evidence.

Reject for now:

- M506e coverage-only calibrated-count as a promoted policy;
- qrels-aware gate tuning;
- BM25 rescue;
- learned scorer replacement.

## Next Step

M506f should train/tune a qrels-free gate against dense ranking agreement:

1. for train queries, evaluate prefix 256/512/768 against dense teacher rank
   overlap or dense-score order stability;
2. select the minimal prefix that matches high-prefix ranking agreement within
   a small tolerance;
3. learn a compact gate from query-time signals such as base touched ratio,
   top score margin, support entropy, and prefix-stability indicators;
4. keep hand-count as the fallback baseline;
5. evaluate Broad10 sampled before any ranking-aware or RL stage.
