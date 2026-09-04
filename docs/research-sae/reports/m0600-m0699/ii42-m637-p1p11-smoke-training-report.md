# M637 / P1.11 Smoke Training Report

Status: `smoke_gate_failed`

- Run: `m637_p1p11_query_boundary_seed6371`
- Tasks: `FiQA2018, ArguAna`
- Baseline source: `m549_frozen`
- Checkpoint: `/home/huoju/leask/runs/ii42-m637-p1p11-rank-preserving-query-v1/m637_p1p11_query_boundary_seed6371/m637_p1p11_query_boundary_seed6371.query_compiler.pt`
- Query rows: `/home/huoju/leask/runs/ii42-m637-p1p11-rank-preserving-query-v1/m637_p1p11_query_boundary_seed6371/m637_p1p11_query_boundary_seed6371_query_rows.jsonl`
- Case counts: `{'dev': 96, 'test': 128, 'train': 192}`
- Selected trained checkpoint: `False`

## Trace

```json
[
  {
    "epoch": 1,
    "gate": {
      "checks": {
        "dense_overlap_guard": true,
        "map_non_negative": true,
        "mrr_guard": true,
        "ndcg_guard": true,
        "recall_positive": false,
        "trained_checkpoint_selected": true
      },
      "deltas": {
        "candidate_upper_bound": 0.0,
        "dense_overlap_at_100": -0.00020833333333314386,
        "map_at_100": 0.002182752144284761,
        "mrr_at_20": 0.0012493191721132946,
        "ndcg_at_10": 0.0015420590793205347,
        "query_count": 0.0,
        "recall_at_100": 0.0
      },
      "failed_checks": [
        "recall_positive"
      ],
      "selected_epoch": 1,
      "selected_global_step": 180,
      "status": "smoke_gate_failed"
    },
    "global_step": 180,
    "losses": {
      "boundary": 0.14422129,
      "dense_margin": 0.65731455,
      "dense_rank": 2.25516562,
      "faithfulness": 0.00517953,
      "loss": 4.24154612,
      "regularizer": 0.00119103,
      "retrieval": 5.78287253
    },
    "recall_at_100": 0.8703125
  }
]
```
