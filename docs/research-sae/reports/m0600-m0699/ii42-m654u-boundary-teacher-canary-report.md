# M654U Boundary-Constrained Teacher Canary

Status: `dense_equivalence_gate_passed`

This run trains the query-side compiler to distill an M653G-style
ridge teacher.  It freezes document postings and uses no BM25,
reranker, learned gate, or qrels-driven loss.  Qrels are used only
for held-out metric reporting.

## Example Counts

```json
{
  "dev": 40,
  "test": 40,
  "train": 320
}
```

## Teacher Summary

```json
{
  "dev": {
    "example_count": 40.0,
    "fit_mse": 0.015741711657028645,
    "fit_mse_delta": -0.00020816953619942069,
    "moved_query_count": 40.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0011941223732719664,
    "rejected_nonzero_count": 0.2,
    "safe_nonzero_count": 5.8,
    "selected_scale": 0.009049999999999999,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0005000000000000004,
    "teacher_dense_overlap_256_delta": 0.00029296875,
    "teacher_support_cosine_delta": -7.271766662597656e-07
  },
  "test": {
    "example_count": 40.0,
    "fit_mse": 0.015426829206990077,
    "fit_mse_delta": -0.00020018437062390148,
    "moved_query_count": 40.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.00110611419131601,
    "rejected_nonzero_count": 0.475,
    "safe_nonzero_count": 5.525,
    "selected_scale": 0.0082875,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0,
    "teacher_dense_overlap_256_delta": 0.00029296875,
    "teacher_support_cosine_delta": -6.86943531036377e-07
  },
  "train": {
    "example_count": 320.0,
    "fit_mse": 0.017090699099935592,
    "fit_mse_delta": -0.00022152851088321767,
    "moved_query_count": 320.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0011734406332152502,
    "rejected_nonzero_count": 0.28125,
    "safe_nonzero_count": 5.71875,
    "selected_scale": 0.008878906249999999,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00040624999999999966,
    "teacher_dense_overlap_256_delta": 0.00029296875,
    "teacher_support_cosine_delta": -7.463619112968445e-07
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.975000 | 1.000000 | 1.000000 | 0.678162 | 0.612167 | 0.922672 | 0.731617 |
| `m654u_boundary_teacher` | 0.975000 | 0.947560 | 0.947666 | 0.670993 | 0.605650 | 0.906005 | 0.712582 |
| `p1_native` | 0.975000 | 0.947560 | 0.947433 | 0.670993 | 0.605641 | 0.906005 | 0.712582 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.00023251488095232808,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 8.735150244687517e-06,
  "map_at_1000": 1.1315884510865182e-05,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 4.675364042361352e-06,
  "ndcg_at_1000": 1.0259069797458054e-05,
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
    "dense_overlap_at_256": 0.00023251488095232808,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 8.735150244687517e-06,
    "map_at_1000": 1.1315884510865182e-05,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": 4.675364042361352e-06,
    "ndcg_at_1000": 1.0259069797458054e-05,
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
  "selected_epoch": 3,
  "selected_global_step": 15,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 3,
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
      "dense_overlap_at_100": 0.0,
      "dense_overlap_at_256": 0.00016741071428572063,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 0.0,
      "map_at_1000": -1.8404004020755593e-07,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 0.0,
      "ndcg_at_1000": -4.580279590049585e-07,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.1920928955078125e-07
    },
    "failed_checks": [],
    "selected_epoch": 3,
    "selected_global_step": 15,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 15,
  "global_steps": 60,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler passed the canary first-stage gate.  Replay this checkpoint on full shared15 before promotion.
