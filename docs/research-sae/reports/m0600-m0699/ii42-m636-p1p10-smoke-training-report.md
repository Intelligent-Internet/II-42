# M636 / P1.10 Smoke Training Report

Status: `smoke_gate_failed`

- Run: `m636_p1p10_smoke_gatefix_seed6361`
- Tasks: `FiQA2018, SCIDOCS, TRECCOVID, ArguAna`
- Baseline source: `m635_baseline`
- Checkpoint: `/home/huoju/leask/runs/ii42-m636-p1p10-retrieval-compiler-v1/m636_p1p10_smoke_gatefix_seed6361/m636_p1p10_smoke_gatefix_seed6361.compiler.pt`
- Query rows: `/home/huoju/leask/runs/ii42-m636-p1p10-retrieval-compiler-v1/m636_p1p10_smoke_gatefix_seed6361/m636_p1p10_smoke_gatefix_seed6361_query_rows.jsonl`
- Case counts: `{'dev': 154, 'test': 202, 'train': 318}`
- Selected trained checkpoint: `False`

## Trace

```json
[
  {
    "epoch": 1,
    "gate": {
      "checks": {
        "dense_overlap_guard": false,
        "map_non_negative": true,
        "mrr_guard": true,
        "ndcg_guard": true,
        "recall_positive": false
      },
      "deltas": {
        "candidate_upper_bound": -0.0009717204521441092,
        "dense_overlap_at_100": -0.012343749999999987,
        "map_at_100": 0.00048736452704878364,
        "mrr_at_20": 0.0005761310448810786,
        "ndcg_at_10": 0.0012644565847129141,
        "query_count": 0.0,
        "recall_at_100": -7.482271847081279e-05
      },
      "failed_checks": [
        "dense_overlap_guard",
        "recall_positive"
      ],
      "status": "smoke_gate_failed"
    },
    "global_step": 180,
    "losses": {
      "faithfulness": 0.00442849,
      "listwise": 5.56141416,
      "loss": 5.80343023,
      "pairwise": 0.68641293,
      "regularizer": 0.00187528
    },
    "recall_at_100": 0.60446482
  }
]
```
