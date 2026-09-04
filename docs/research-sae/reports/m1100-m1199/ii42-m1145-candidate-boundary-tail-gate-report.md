# ii42 M1145 Candidate Boundary Tail Gate Report

## Purpose

M1144 showed that query-level score/overlap summaries are too weak to learn
the M1143 query-level oracle.  M1145 tests a stronger, still-deployable feature
family:

- compare tail documents inserted into top100 by `protect5` or `protect20`;
- compare base top100 documents displaced by that action;
- add lexical coverage from query terms against inserted/displaced document
  text;
- train action-specific binary classifiers under leave-one-dataset-out
  validation.

The goal is to see whether a candidate-boundary view can recover a useful
fraction of the M1143 oracle without dataset-specific thresholds.

## Inputs

- M1142 eval root:
  `runs/m1142_shared15_table_replay_partial_k5000_v1/`
- M1143 labels:
  `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- M1145 output:
  `runs/m1145_candidate_boundary_tail_gate_v1/features_predictions.json`
- M1145 summary:
  `runs/m1145_candidate_boundary_tail_gate_v1/summary.md`

## Method

For each query and each action (`protect5`, `protect20`), M1145 builds
action-specific boundary features:

- inserted top100 count;
- displaced top100 count;
- inserted tail rank min/mean;
- inserted/displaced fused score, P1 score, BM25 score;
- inserted/displaced source-count;
- query-term lexical coverage and hit counts;
- inserted-minus-displaced score and lexical deltas.

It then trains a binary action/no-action logistic model.  At inference time it
predicts probabilities for `protect5` and `protect20`, picks the higher score,
and abstains unless the confidence threshold is met.

## Result

| source | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| oracle_label | +0.000938 | +0.015239 | +0.005452 | +0.003472 | +0.000255 |
| raw_prediction | -0.000628 | +0.012608 | +0.007452 | +0.002650 | +0.000364 |

Prediction distribution:

| prediction | count |
| --- | ---: |
| protect5 | 1338 |
| protect20 | 4 |
| abstain | 0 |

Threshold sweep result:

- no threshold satisfied the full guard;
- low thresholds act almost everywhere and keep CUB negative;
- high thresholds reduce action count but still leave NDCG negative or Recall
  too small;
- best-looking high threshold still fails guard.

## Interpretation

This is a stronger negative than M1144.

The model learned the high-recall `protect5` shape, but did not learn when to
abstain.  Candidate-boundary score and lexical coverage features explain the
direction of potential movement better than M1144, but they still do not
predict safety.  The remaining oracle boundary depends on information not
captured by these local deployable features.

The protected-tail oracle is real, but the deployable gate is not currently
recoverable from:

- top-k overlap;
- inserted/displaced rank and score summaries;
- BM25/P1/source-count comparisons;
- query-term lexical coverage.

## Decision

Stop this deployable protected-tail gate branch for now.

M1142 and M1143 should be kept as evidence and analysis tools:

- M1142 proves the native bridge and full shared15 signal;
- M1143 proves query-level oracle headroom;
- M1144/M1145 show current deployable feature families cannot recover that
  oracle safely.

Do not keep adding thresholds or shallow classifiers on the same local feature
surface.  The next viable route needs a different signal source:

1. a stronger teacher that directly labels safe tail insertions;
2. candidate-level dense/qrels-derived boundary supervision distilled into
   posting generation, not post-hoc local gating;
3. or a deterministic policy with an explicit proof-style guard inside native
   query execution, not learned from these features.

Until then, protected-tail should not be promoted as a production native route.
