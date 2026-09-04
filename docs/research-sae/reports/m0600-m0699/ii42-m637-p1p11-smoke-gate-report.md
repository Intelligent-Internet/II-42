# M637 / P1.11 Smoke Gate Report

Status: `smoke_gate_failed`

- Selected epoch: `0`
- Selected global step: `0`

## Test Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.975391 | 1.000000 | 0.499620 | 0.400499 | 0.924674 | 0.475701 |
| `m549_frozen` | 0.975391 | 0.996484 | 0.499644 | 0.400598 | 0.924674 | 0.475766 |
| `m637_query` | 0.975391 | 0.996484 | 0.499644 | 0.400598 | 0.924674 | 0.475766 |

## Decision

```json
{
  "checks": {
    "dense_overlap_guard": true,
    "map_non_negative": true,
    "mrr_guard": true,
    "ndcg_guard": true,
    "recall_positive": false,
    "trained_checkpoint_selected": false
  },
  "deltas": {
    "candidate_upper_bound": 0.0,
    "dense_overlap_at_100": 0.0,
    "map_at_100": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "query_count": 0.0,
    "recall_at_100": 0.0
  },
  "failed_checks": [
    "recall_positive",
    "trained_checkpoint_selected"
  ],
  "selected_epoch": 0,
  "selected_global_step": 0,
  "status": "smoke_gate_failed"
}
```
