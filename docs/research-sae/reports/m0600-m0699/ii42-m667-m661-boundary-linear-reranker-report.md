# M667 / M661 Boundary Linear Reranker

Status: `m667_test_gate_passed`

M667 keeps the M658 top1000 candidate set fixed and trains only a
global linear scorer over dense/M549/M658/M661 score geometry.  It is
a second-stage smoke test for the `present_below_top100` pool found by
M666.

## Selected Dev Grid

- alpha: `0.75`
- preserve_top_k: `80`
- rerank_max_rank: `300`
- dev status: `accepted`
- dev deltas: `{"candidate_upper_bound": 0.0, "dense_overlap_at_100": -0.06328859060402692, "map_at_100": 7.243058233841193e-05, "mrr_at_20": 0.0, "ndcg_at_10": 0.0, "recall_at_100": 0.003973101395112311}`

## Test Gate

- status: `accepted`
- failed checks: `[]`
- deltas: `{"candidate_upper_bound": 0.0, "dense_overlap_at_100": -0.062309644670050957, "map_at_100": 2.0314800157517077e-05, "mrr_at_20": 0.0, "ndcg_at_10": 0.0, "recall_at_100": 0.0019903371855316543}`

## Test Matrix

`O@100` is top100 overlap against the fixed M658 baseline order.

| Source | CUB | O@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m658_baseline` | 0.899800 | 1.000000 | 0.761723 | 0.331258 | 0.427089 | 0.446442 |
| `m667_reranker` | 0.899800 | 0.937690 | 0.763713 | 0.331278 | 0.427089 | 0.446442 |
| `oracle_candidate_set` | 0.899800 | 0.986345 | 0.891704 | 0.891704 | 0.932420 | 0.994924 |

## Oracle Gap

The oracle candidate-set row keeps the same M658 top1000 candidates but
orders known positives first.  It measures ranking headroom, not a
deployable model.

## Decision

M667 is a positive second-stage signal.  The next step should scale
this scorer to a broader native-engineering surface and add BM25/P1
features before returning to full matrix evaluation.

## Artifacts

- JSON: `runs/ii42-m667-m661-boundary-linear-reranker-v1/m667_m661_boundary_linear_seed6671/m667_m661_boundary_linear_seed6671.json`
- Model JSON: `runs/ii42-m667-m661-boundary-linear-reranker-v1/m667_m661_boundary_linear_seed6671/m667_m661_boundary_linear_seed6671_model.json`

