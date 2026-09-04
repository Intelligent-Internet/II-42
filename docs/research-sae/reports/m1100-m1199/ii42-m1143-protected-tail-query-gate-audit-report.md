# ii42 M1143 Protected-Tail Query Gate Audit Report

## Purpose

M1142 full shared15 proved two things at the same time:

1. M1129 protected base plus M1137 tail has real native table-backed signal.
2. A fixed `protect_k` policy is not safe enough for promotion.

M1143 checks whether the next step should be a query-level gate.  It uses the
existing M1142 k=5000 per-query eval JSON and computes an oracle over three
choices:

- `abstain`: keep the M1129 base ranking;
- `protect5`: use protected-tail with top 5 base ranks fixed;
- `protect20`: use protected-tail with top 20 base ranks fixed.

This is not a deployable gate.  It is a headroom audit: if this oracle has no
safe headroom, a learned native feature gate is not worth building.

## Inputs

- `runs/m1142_shared15_table_replay_partial_k5000_v1/`
- `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- `runs/m1143_protected_tail_query_gate_audit_v1/summary.md`

The M1142 root keeps its original `partial` name, but it now contains all 15
shared15 datasets.

## Macro Result

| policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| fixed20 | -0.000624 | +0.012743 | +0.004062 | +0.000000 | +0.000000 |
| fixed5 | -0.000628 | +0.012608 | +0.007468 | +0.002665 | +0.000379 |
| oracle_recall_gain | +0.000938 | +0.015239 | +0.005452 | +0.003472 | +0.000255 |
| oracle_safe | +0.000947 | +0.015239 | +0.010292 | +0.009900 | +0.001334 |

## Choice Distribution

| oracle | abstain | protect5 | protect20 | non-abstain |
| --- | ---: | ---: | ---: | ---: |
| oracle_safe | 959 | 295 | 88 | 383 |
| oracle_recall_gain | 1193 | 104 | 45 | 149 |

Total queries: 1342.

The useful action set is sparse.  A deployable gate should be high precision
and abstain-heavy.  Applying tail movement broadly is the wrong shape.

## Interpretation

- Query-level switching has enough headroom to justify the next stage.
- The oracle turns CUB positive, which directly addresses the `trec-covid`
  failure in fixed protect policies.
- The oracle improves NDCG/MRR instead of spending them, so the top-rank harm
  seen in `fiqa`, `msmarco`, `scidocs`, and `webis-touche2020` is avoidable in
  principle.
- The strongest safe oracle is not the recall-only oracle.  Some queries should
  move for MAP/NDCG/MRR even when Recall@100 does not change.
- `fever`, `hotpotqa`, and `nq` are mostly abstain rows.  A good gate must
  learn no-op/harm-risk detection, not only tail-helpfulness.

## Stop / Continue Decision

Continue, but only into a gate audit with deployable features.

M1143 does not justify training a broad reranker or doing another fixed
`protect_k` search.  It justifies M1144:

1. extract per-query native features for `abstain`, `protect5`, and
   `protect20`;
2. train or derive a high-precision selector for action vs abstain;
3. evaluate with held-out datasets or leave-one-dataset-out validation;
4. require macro CUB, NDCG@10, MRR@20, and Recall@100 to stay non-negative;
5. reject if gains collapse outside the oracle labels or require
   dataset-specific thresholds.

The target product shape remains native unified posting, not a separate
reranker.
