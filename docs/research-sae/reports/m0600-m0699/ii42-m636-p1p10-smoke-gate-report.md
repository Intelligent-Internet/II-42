# M636 / P1.10 Smoke Gate Report

Status: `smoke_gate_failed`

- Selected epoch: `0`
- Selected global step: `0`

## Test Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.822281 | 1.000000 | 0.492065 | 0.277892 | 0.636744 | 0.528240 |
| `m549_target` | 0.823029 | 0.995875 | 0.491719 | 0.277238 | 0.636607 | 0.527862 |
| `m635_baseline` | 0.822970 | 0.994438 | 0.491698 | 0.277138 | 0.636468 | 0.527862 |
| `m636_compiler` | 0.822281 | 1.000000 | 0.492065 | 0.277892 | 0.636744 | 0.528240 |
| `m636_initial` | 0.822281 | 1.000000 | 0.492065 | 0.277892 | 0.636744 | 0.528240 |

## Decision

```json
{
  "checks": {
    "dense_overlap_guard": true,
    "map_non_negative": true,
    "mrr_guard": true,
    "ndcg_guard": true,
    "recall_positive": true,
    "trained_checkpoint_selected": false
  },
  "deltas": {
    "candidate_upper_bound": -0.0006895818987778801,
    "dense_overlap_at_100": 0.005562499999999915,
    "map_at_100": 0.0007543959567026248,
    "mrr_at_20": 0.0003778249607451212,
    "ndcg_at_10": 0.0003672108137096064,
    "query_count": 0.0,
    "recall_at_100": 0.00027608212660390485
  },
  "failed_checks": [
    "trained_checkpoint_selected"
  ],
  "selected_epoch": 0,
  "selected_global_step": 0,
  "status": "smoke_gate_failed"
}
```
