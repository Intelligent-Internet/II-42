# M654W Boundary-Loss Teacher Shared15

Status: `dense_equivalence_gate_failed`

This run trains the query-side compiler to distill an M653G-style
ridge teacher.  It freezes document postings and uses no BM25,
reranker, learned gate, or qrels-driven loss.  Qrels are used only
for held-out metric reporting.

## Example Counts

```json
{
  "dev": 134,
  "test": 134,
  "train": 1074
}
```

## Teacher Summary

```json
{
  "dev": {
    "example_count": 134.0,
    "fit_mse": 0.016026671870295377,
    "fit_mse_delta": -0.00020388249610203194,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0011233750108169886,
    "rejected_nonzero_count": 0.3880597014925373,
    "safe_nonzero_count": 5.611940298507463,
    "selected_scale": 0.008399253731343285,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492475,
    "teacher_dense_overlap_256_delta": 0.0003206623134328358,
    "teacher_support_cosine_delta": -7.205934666875583e-07
  },
  "test": {
    "example_count": 134.0,
    "fit_mse": 0.019907238727275615,
    "fit_mse_delta": -0.00026391575429171544,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "query_delta_l2": 0.0011818592682772502,
    "rejected_nonzero_count": 0.35074626865671643,
    "safe_nonzero_count": 5.649253731343284,
    "selected_scale": 0.008815298507462686,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0002985074626865666,
    "teacher_dense_overlap_256_delta": 0.0004664179104477612,
    "teacher_support_cosine_delta": -7.53064653766689e-07
  },
  "train": {
    "example_count": 1074.0,
    "fit_mse": 0.01789100685033746,
    "fit_mse_delta": -0.00022736937501355035,
    "moved_query_count": 1070.0,
    "moved_query_rate": 0.9962756052141527,
    "query_delta_l2": 0.0011358042772492095,
    "rejected_nonzero_count": 0.3985102420856611,
    "safe_nonzero_count": 5.601489757914339,
    "selected_scale": 0.00848417132216015,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778384,
    "teacher_dense_overlap_256_delta": 0.00027641992551210426,
    "teacher_support_cosine_delta": -7.186966013420005e-07
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.961834 | 1.000000 | 1.000000 | 0.808277 | 0.704682 | 0.856420 | 0.886612 |
| `m654w_boundary_loss` | 0.961664 | 0.943405 | 0.947820 | 0.809959 | 0.706047 | 0.856012 | 0.882718 |
| `p1_native` | 0.961664 | 0.943465 | 0.947851 | 0.809959 | 0.706039 | 0.856012 | 0.882718 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": -6.060606060598328e-05,
  "dense_overlap_at_256": -3.1550480769193534e-05,
  "dense_overlap_at_50": -0.00010256410256404003,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 8.69664682989768e-06,
  "map_at_1000": 8.310268515110408e-06,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 8.010612104314419e-06,
  "ndcg_at_1000": 7.477213672446581e-06,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -2.0265579225853259e-07
}
```

## Decision

```json
{
  "checks": {
    "active_support_not_regressed": true,
    "cub_safe": true,
    "dense_overlap_100_safe": false,
    "dense_overlap_256_safe": false,
    "recall_safe": true,
    "support_cosine_not_regressed": true,
    "trained_checkpoint_selected": true
  },
  "deltas": {
    "accuracy": 0.0,
    "active_support_recall": 0.0,
    "dense_overlap_at_10": 0.0,
    "dense_overlap_at_100": -6.060606060598328e-05,
    "dense_overlap_at_256": -3.1550480769193534e-05,
    "dense_overlap_at_50": -0.00010256410256404003,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 8.69664682989768e-06,
    "map_at_1000": 8.310268515110408e-06,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": 8.010612104314419e-06,
    "ndcg_at_1000": 7.477213672446581e-06,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -2.0265579225853259e-07
  },
  "failed_checks": [
    "dense_overlap_100_safe",
    "dense_overlap_256_safe"
  ],
  "selected_epoch": 7,
  "selected_global_step": 119,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 7,
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
      "dense_overlap_at_100": 1.190476190460199e-05,
      "dense_overlap_at_256": 3.956330128196406e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -7.641911382361144e-06,
      "map_at_1000": -5.449122353118341e-06,
      "map_at_20": -8.733749910283528e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 2.9628811404336375e-08,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 1.5559506959528235e-06,
      "ndcg_at_1000": 2.2832542612150064e-06,
      "ndcg_at_20": 1.9226079521761363e-06,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.9868214928830952e-07
    },
    "failed_checks": [],
    "selected_epoch": 7,
    "selected_global_step": 119,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 119,
  "global_steps": 204,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
