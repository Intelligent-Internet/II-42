# M654V Overlap50-Gated Boundary Teacher Shared15

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
    "fit_mse": 0.01920453342956616,
    "fit_mse_delta": -0.00024845975159264324,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.0011854776221841361,
    "rejected_nonzero_count": 0.3208955223880597,
    "safe_nonzero_count": 5.67910447761194,
    "selected_scale": 0.00878171641791045,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492475,
    "teacher_dense_overlap_256_delta": 0.0003206623134328358,
    "teacher_support_cosine_delta": -7.566231400219362e-07
  },
  "test": {
    "example_count": 134.0,
    "fit_mse": 0.018357134420674905,
    "fit_mse_delta": -0.00023154750442021152,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "query_delta_l2": 0.001119140550959421,
    "rejected_nonzero_count": 0.3880597014925373,
    "safe_nonzero_count": 5.611940298507463,
    "selected_scale": 0.008352611940298509,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029850746268656744,
    "teacher_dense_overlap_256_delta": 0.000378964552238806,
    "teacher_support_cosine_delta": -7.112524402675344e-07
  },
  "train": {
    "example_count": 1074.0,
    "fit_mse": 0.017687916094417732,
    "fit_mse_delta": -0.00022584609125846618,
    "moved_query_count": 1069.0,
    "moved_query_rate": 0.9953445065176909,
    "query_delta_l2": 0.0011358811470791829,
    "rejected_nonzero_count": 0.4022346368715084,
    "safe_nonzero_count": 5.597765363128492,
    "selected_scale": 0.008494180633147113,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778374,
    "teacher_dense_overlap_256_delta": 0.0002873312383612663,
    "teacher_support_cosine_delta": -7.194180728336952e-07
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.955341 | 1.000000 | 1.000000 | 0.742692 | 0.652181 | 0.829456 | 0.823247 |
| `m654v_overlap50_teacher` | 0.953805 | 0.945189 | 0.947641 | 0.735580 | 0.648287 | 0.827488 | 0.818647 |
| `p1_native` | 0.953805 | 0.945189 | 0.947641 | 0.735580 | 0.648287 | 0.827488 | 0.818647 |

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
  "support_cosine": 1.1920929021691506e-08
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
    "dense_overlap_50_safe": true,
    "recall_safe": true,
    "support_cosine_not_regressed": true,
    "trained_checkpoint_selected": false
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
    "support_cosine": 1.1920929021691506e-08
  },
  "failed_checks": [
    "trained_checkpoint_selected"
  ],
  "selected_epoch": 0,
  "selected_global_step": 0,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 0,
  "best_gate": null,
  "best_global_step": 0,
  "global_steps": 204,
  "selected_trained_checkpoint": false
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
