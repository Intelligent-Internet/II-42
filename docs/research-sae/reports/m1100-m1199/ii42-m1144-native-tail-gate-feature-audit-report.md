# ii42 M1144 Native Tail Gate Feature Audit Report

## Purpose

M1143 proved query-level oracle headroom for choosing among:

- abstain;
- `protect5`;
- `protect20`.

M1144 tests whether native stream features available at query time are already
sufficient to learn that choice.  This is the first deployability check after
the oracle audit.

## Inputs

- M1142 eval root:
  `runs/m1142_shared15_table_replay_partial_k5000_v1/`
- M1143 labels:
  `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- M1144 output:
  `runs/m1144_native_tail_gate_features_v1/features_predictions.json`
- M1144 summary:
  `runs/m1144_native_tail_gate_features_v1/summary.md`

## Method

For each query, the script re-queries the M1129 base stream and M1137 tail
stream through the existing table-backed native path, then extracts deployable
features:

- base/tail candidate counts;
- base/tail top-k overlap;
- top100 changes under `protect5` and `protect20`;
- top BM25/P1 score shape;
- tail-new top100 score and source-count summaries.

It trains a multinomial logistic regression with leave-one-dataset-out
validation.  Labels are M1143 `oracle_recall_gain` choices, so this is a hard
and sparse action-selection task.

## Result

| source | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| oracle_label | +0.000938 | +0.015239 | +0.005452 | +0.003472 | +0.000255 |
| raw_prediction | -0.000628 | +0.004049 | +0.003215 | +0.001228 | +0.000082 |
| best_guarded_threshold | +0.000021 | +0.001344 | +0.001451 | +0.001150 | +0.000000 |

Counts:

| item | abstain | protect5 | protect20 |
| --- | ---: | ---: | ---: |
| labels | 1193 | 104 | 45 |
| raw predictions | 943 | 226 | 173 |

Raw action precision:

| action | precision |
| --- | ---: |
| abstain | 0.928950 |
| protect5 | 0.084071 |
| protect20 | 0.144509 |

Best guarded confidence threshold:

- threshold: `0.66`
- non-abstain actions: `116`
- non-abstain action precision: `0.163793`
- macro Recall@100: `+0.001344`

## Interpretation

This feature set is not sufficient.

The classifier correctly learns that most queries should abstain, but it does
not learn a high-precision action boundary for `protect5` or `protect20`.
Without a confidence threshold, it over-actions and makes CUB negative.  With a
threshold, it can satisfy the guard, but the remaining Recall/MAP gain is too
small compared with the M1143 oracle.

This is not a failure of M1142/M1143.  It is a stop signal for this specific
feature family: top-k overlap and simple score-shape features do not explain
the oracle action boundary well enough.

## Decision

Do not keep tuning this logistic/overlap feature gate.

The next valid branch must add stronger, still-deployable evidence:

1. candidate-level boundary features, not only query-level summaries;
2. qrels-free lexical/entity/term coverage features for tail-new docs;
3. support-risk features derived from atom fanout and tail insertion depth;
4. a high-precision abstain-first training objective, likely pairwise or
   calibrated binary action/no-action before choosing `protect5` vs
   `protect20`;
5. leave-one-dataset-out validation remains mandatory.

If those stronger features still cannot recover a meaningful fraction of M1143
oracle gain, the protected-tail policy should remain an analysis tool rather
than a deployable native route.
