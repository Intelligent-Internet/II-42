# M665 / Candidate Upper-Bound Shape Audit

Status: `candidate_shape_audit_complete`

M665 is a read-only audit. It does not train a model and it does not
reinterpret fallback query rows as trained-checkpoint evidence.

## Run-Level Gate Evidence

| Run | Signal | Trained selected | Trace dCUB | Trace dR@100 | Trace dO@100 | Trace dMAP | Final status |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| `M659` | `rejected_candidate_shape_signal` | False | +0.00052083 | +0.00004058 | -0.00123958 | -0.00054291 | `smoke_gate_failed` |
| `M660` | `rejected_no_useful_signal` | False | -0.00008409 | +0.00000000 | -0.00155208 | +0.00060100 | `smoke_gate_failed` |
| `M661` | `selected_ranking_signal_cub_flat` | True | -0.00009804 | +0.00004058 | -0.00011458 | +0.00042188 | `smoke_gate_passed` |
| `M662` | `selected_candidate_shape_signal` | True | +0.00045001 | +0.00004058 | -0.00026042 | +0.00030501 | `smoke_gate_failed` |
| `M663` | `rejected_ranking_signal_cub_flat` | False | +0.00000000 | +0.00056064 | -0.00110938 | +0.00019952 | `smoke_gate_failed` |
| `M664` | `rejected_no_useful_signal` | False | -0.00009804 | +0.00000000 | -0.00026042 | -0.00010995 | `smoke_gate_failed` |

## Final Query-Row Split Audit

Rows marked fallback-only are useful as safety baselines, not as
evidence that the trained epoch improved retrieval.

| Run | Split | Rows trained | Matched queries | dCUB | dR@100 | dO@100 | dMAP | Recall gain no CUB | CUB gain no Recall |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M659` | `dev` | False | 298 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M659` | `test` | False | 394 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M660` | `dev` | False | 298 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M660` | `test` | False | 394 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M661` | `dev` | True | 298 | -0.00001316 | +0.00000545 | -0.00043624 | +0.00052357 | 1 | 0 |
| `M661` | `test` | True | 394 | -0.00000185 | +0.00050761 | -0.00065990 | +0.00000899 | 1 | 1 |
| `M662` | `dev` | True | 298 | +0.00066163 | +0.00000545 | -0.00033557 | +0.00037316 | 1 | 1 |
| `M662` | `test` | True | 394 | +0.00000000 | +0.00049953 | -0.00076142 | -0.00045785 | 1 | 0 |
| `M663` | `dev` | False | 298 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M663` | `test` | False | 394 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M664` | `dev` | False | 298 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |
| `M664` | `test` | False | 394 | +0.00000000 | +0.00000000 | +0.00000000 | +0.00000000 | 0 | 0 |

## Diagnosis

1. M659 and M662 are the only runs with a trace-level CUB gain, but
   both fail downstream gates: M659 loses dense overlap/MAP and M662
   regresses final MAP/geometry.
2. M661 is the only selected trained checkpoint with positive final
   Recall@100, but its CUB delta is effectively flat. That is a
   ranking/boundary movement signal, not a candidate-shape breakthrough.
3. M663 increases Recall/MAP in the trace while CUB stays flat, then
   fails dense overlap. M664 protects dense overlap but suppresses
   Recall/MAP. This confirms the boundary-preservation weight family
   is trading stability against ranking movement.

## Limitation

Current query rows contain per-query aggregate metrics, not per-positive
ranks. This audit can separate CUB movement from ranking movement, but
it cannot yet say exactly which positive document crossed rank 100.

## Decision

Do not continue boundary-loss weight tuning. The next useful branch is
a candidate-shape objective or a per-positive rank exporter. If the next
stage stays in first-stage model training, it must target CUB movement
directly and gate on CUB + Recall + dense-overlap together.

## Next Step

M666 should export per-positive ranks for dense root, M549 target,
M658/M661 candidates, and optionally BM25/P1 scorer rows. That enables
hard classification into candidate miss, present-below-top100, and
support-damaging swap. Training should resume only after that exporter
shows whether the bottleneck is candidate generation or ranking.

## Artifacts

- JSON: `runs/ii42-m665-candidate-upper-bound-shape-audit-v1/m665_candidate_upper_bound_shape_seed6651/m665_candidate_upper_bound_shape_seed6651.json`

