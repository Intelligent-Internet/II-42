# M654U Boundary-Constrained Teacher Smoke

Status: `dense_equivalence_gate_passed`

This run trains the query-side compiler to distill an M653G-style
ridge teacher.  It freezes document postings and uses no BM25,
reranker, learned gate, or qrels-driven loss.  Qrels are used only
for held-out metric reporting.

## Example Counts

```json
{
  "dev": 10,
  "test": 10,
  "train": 80
}
```

## Teacher Summary

```json
{
  "dev": {
    "example_count": 10.0,
    "fit_mse": 0.02386979004368186,
    "fit_mse_delta": -0.0002661664970219135,
    "moved_query_count": 10.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0011643729987554253,
    "rejected_nonzero_count": 0.3,
    "safe_nonzero_count": 5.7,
    "selected_scale": 0.008700000000000001,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0010000000000000009,
    "teacher_dense_overlap_256_delta": 0.00078125,
    "teacher_support_cosine_delta": -7.569789886474609e-07
  },
  "test": {
    "example_count": 10.0,
    "fit_mse": 0.021020275168120862,
    "fit_mse_delta": -0.0002997519448399544,
    "moved_query_count": 10.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0012194593902677298,
    "rejected_nonzero_count": 0.2,
    "safe_nonzero_count": 5.8,
    "selected_scale": 0.0092,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0010000000000000009,
    "teacher_dense_overlap_256_delta": 0.0,
    "teacher_support_cosine_delta": -8.106231689453125e-07
  },
  "train": {
    "example_count": 80.0,
    "fit_mse": 0.017919505306053907,
    "fit_mse_delta": -0.0002446553437039256,
    "moved_query_count": 80.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.001246749506390188,
    "rejected_nonzero_count": 0.15,
    "safe_nonzero_count": 5.85,
    "selected_scale": 0.0092875,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0006249999999999977,
    "teacher_dense_overlap_256_delta": 0.000146484375,
    "teacher_support_cosine_delta": -8.068978786468506e-07
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.983333 | 1.000000 | 1.000000 | 0.735956 | 0.668106 | 0.841667 | 0.766667 |
| `m654u_boundary_teacher` | 0.983333 | 0.939000 | 0.952734 | 0.724570 | 0.663573 | 0.854167 | 0.762500 |
| `p1_native` | 0.983333 | 0.939000 | 0.952734 | 0.724570 | 0.663573 | 0.854167 | 0.762500 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.0,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 0.0,
  "map_at_1000": 0.0,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 0.0,
  "ndcg_at_1000": 0.0,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": 0.0
}
```

## Decision

```json
{
  "checks": {
    "active_support_not_regressed": true,
    "cub_safe": true,
    "dense_overlap_100_safe": true,
    "dense_overlap_256_safe": true,
    "recall_safe": true,
    "support_cosine_not_regressed": true,
    "trained_checkpoint_selected": true
  },
  "deltas": {
    "accuracy": 0.0,
    "active_support_recall": 0.0,
    "dense_overlap_at_10": 0.0,
    "dense_overlap_at_100": 0.0,
    "dense_overlap_at_256": 0.0,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 0.0,
    "map_at_1000": 0.0,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": 0.0,
    "ndcg_at_1000": 0.0,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": 0.0
  },
  "failed_checks": [],
  "selected_epoch": 2,
  "selected_global_step": 4,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 2,
  "best_gate": {
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 0.0010000000000000009,
      "dense_overlap_at_256": 0.0003906249999999778,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 0.0,
      "map_at_1000": -4.082965866403221e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 0.0,
      "ndcg_at_1000": -1.0541677814668304e-05,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.7881393432617188e-07
    },
    "failed_checks": [],
    "selected_epoch": 2,
    "selected_global_step": 4,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 4,
  "global_steps": 12,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler passed the canary first-stage gate.  Replay this checkpoint on full shared15 before promotion.
