# M653-F Tail-Preservation Grid Report

Status: `no_promotable_checkpoint`

This is a small first-stage follow-up after the M653-E shared15 replay
showed a strict-gate failure on `dense_overlap_at_256`.  The experiment keeps
the same frozen document postings and query-side compiler architecture.  It
does not use BM25, reranker, learned gate, or qrels-driven training.

## Question

Can the M653-E signal be made top256-safe by simply shrinking the query-side
boundary update?

## Configs

All configs use `tail_identity_dims=432`, `faithfulness_weight=3.0`, and the
same M653-D dense-boundary rows.  Only `delta_scale` changes.

| Run | delta_scale | selected epoch | selected step | Canary test status | Failed checks |
| --- | ---: | ---: | ---: | --- | --- |
| `m653f_id432_s004_seed6532` | 0.004 | 5 | 155 | `dense_equivalence_gate_failed` | `dense_overlap_256_safe`, `recall_safe` |
| `m653f_id432_s008_seed6532` | 0.008 | 2 | 62 | `dense_equivalence_gate_failed` | `dense_overlap_100_safe`, `dense_overlap_256_safe`, `recall_safe` |
| `m653f_id432_s012_seed6532` | 0.012 | 2 | 62 | `dense_equivalence_gate_failed` | `dense_overlap_100_safe`, `dense_overlap_256_safe` |

## Canary Test Delta

| Run | dO@10 | dO@100 | dO@256 | dR@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m653f_id432_s004_seed6532` | +0.002380952 | +0.000078760 | -0.000203852 | -0.000700280 | +0.000236518 | +0.000176944 | +0.000000000 |
| `m653f_id432_s008_seed6532` | +0.002380952 | -0.000172257 | -0.000194561 | -0.000700280 | -0.000040018 | -0.000048311 | +0.000000000 |
| `m653f_id432_s012_seed6532` | +0.002380952 | -0.000082877 | -0.000202106 | +0.001680672 | +0.000323887 | +0.000128633 | +0.000000000 |

## Interpretation

Shrinking `delta_scale` is not sufficient.  All three conservative configs
failed the canary test gate before full shared15 replay, and every failure
included `dense_overlap_256_safe`.  This confirms the M653-E shared15 failure
is not just an update-magnitude issue.

The important observation is that dev selection remained overly optimistic:
each run selected a trained checkpoint whose dev gate passed, but the held-out
canary test gate failed.  The next experiment therefore needs a stronger
selection/evaluation surface or an explicit top256/listwise tail-preservation
objective.  Continuing scalar shrinkage is not a useful direction.

## Decision

Do not run full shared15 replay for these configs.  They failed the smaller
canary test gate.

Next step: implement a real top256/listwise preservation objective or
full-replay checkpoint-selection gate.  Keep the frozen-document/query-side
setting and do not increase capacity until the top256 constraint is controlled.

