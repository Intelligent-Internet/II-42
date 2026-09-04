# M668 Output Blend Scale 0.2 Seed 6548

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
    "challenger_ceiling_count_mean": 26.559701492537314,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.018182226573117077,
    "fit_mse_delta": -0.00023830725304059574,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011737723089298476,
    "rejected_nonzero_count": 0.31343283582089554,
    "safe_nonzero_count": 5.686567164179104,
    "selected_scale": 0.008671641791044777,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029850746268656576,
    "teacher_dense_overlap_256_delta": 0.0002915111940298507,
    "teacher_support_cosine_delta": -7.597368154952776e-07,
    "top100_boundary_pair_count_mean": 4.17910447761194,
    "top100_boundary_pair_query_rate": 0.5223880597014925
  },
  "test": {
    "challenger_ceiling_count_mean": 26.53731343283582,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.017779717006400894,
    "fit_mse_delta": -0.0002240876444796128,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011340201644047942,
    "rejected_nonzero_count": 0.3582089552238806,
    "safe_nonzero_count": 5.641791044776119,
    "selected_scale": 0.008447761194029851,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492557,
    "teacher_dense_overlap_256_delta": 0.0002915111940298507,
    "teacher_support_cosine_delta": -7.17479791214217e-07,
    "top100_boundary_pair_count_mean": 3.8208955223880596,
    "top100_boundary_pair_query_rate": 0.47761194029850745
  },
  "train": {
    "challenger_ceiling_count_mean": 26.62756052141527,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 1074.0,
    "fit_mse": 0.017887509252962324,
    "fit_mse_delta": -0.00022804353635903844,
    "moved_query_count": 1069.0,
    "moved_query_rate": 0.9953445065176909,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011354850984519897,
    "rejected_nonzero_count": 0.4068901303538175,
    "safe_nonzero_count": 5.593109869646183,
    "selected_scale": 0.008496042830540037,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778384,
    "teacher_dense_overlap_256_delta": 0.0003018796554934823,
    "teacher_support_cosine_delta": -7.182526188855731e-07,
    "top100_boundary_pair_count_mean": 4.01489757914339,
    "top100_boundary_pair_query_rate": 0.5018621973929237
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.981859 | 1.000000 | 1.000000 | 0.800534 | 0.720802 | 0.873601 | 0.892956 |
| `m668_blend020_seed6548` | 0.981680 | 0.945579 | 0.950285 | 0.793727 | 0.714723 | 0.869123 | 0.887965 |
| `p1_native` | 0.981680 | 0.945482 | 0.950341 | 0.793727 | 0.714731 | 0.869123 | 0.887965 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 9.722222222230403e-05,
  "dense_overlap_at_256": -5.648467562524573e-05,
  "dense_overlap_at_50": -0.00018037518037539169,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -7.986549670824239e-06,
  "map_at_1000": -7.468211185135409e-06,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -7.329654519994122e-07,
  "ndcg_at_1000": -1.1380053182019623e-06,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -3.9736429857661903e-07
}
```

## Split Boundary Summaries

```json
{
  "dev": {
    "dense_hit_gain_query_count": 1.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 1.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 1.0
  },
  "test": {
    "dense_hit_gain_query_count": 3.0,
    "dense_hit_loss_query_count": 1.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 3.0,
    "total_lost_dense_docs": 1.0,
    "total_net_dense_hit_delta": 2.0
  }
}
```

## Decision

```json
{
  "boundary": {
    "dense_hit_gain_query_count": 3.0,
    "dense_hit_loss_query_count": 1.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 3.0,
    "total_lost_dense_docs": 1.0,
    "total_net_dense_hit_delta": 2.0
  },
  "checks": {
    "active_support_not_regressed": true,
    "cub_safe": true,
    "dense_overlap_100_safe": true,
    "dense_overlap_256_safe": false,
    "dev_gate_passed_for_selected_checkpoint": false,
    "recall_safe": true,
    "support_cosine_not_regressed": true,
    "swap_dense_hit_loss_query_safe": false,
    "trained_checkpoint_selected": true
  },
  "deltas": {
    "accuracy": 0.0,
    "active_support_recall": 0.0,
    "dense_overlap_at_10": 0.0,
    "dense_overlap_at_100": 9.722222222230403e-05,
    "dense_overlap_at_256": -5.648467562524573e-05,
    "dense_overlap_at_50": -0.00018037518037539169,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -7.986549670824239e-06,
    "map_at_1000": -7.468211185135409e-06,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -7.329654519994122e-07,
    "ndcg_at_1000": -1.1380053182019623e-06,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -3.9736429857661903e-07
  },
  "failed_checks": [
    "dense_overlap_256_safe",
    "swap_dense_hit_loss_query_safe",
    "dev_gate_passed_for_selected_checkpoint"
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
    "boundary": {
      "dense_hit_gain_query_count": 1.0,
      "dense_hit_loss_query_count": 0.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 1.0,
      "total_lost_dense_docs": 0.0,
      "total_net_dense_hit_delta": 1.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": false,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 4.4444444444513564e-05,
      "dense_overlap_at_256": 6.511356120730838e-05,
      "dense_overlap_at_50": 6.3492063491210615e-06,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 9.434907629612965e-07,
      "map_at_1000": -1.2033561893609601e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 6.33081664869195e-09,
      "ndcg_at_1000": -3.42189168146545e-05,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": -6.060606060613594e-06,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": -9.935419771489595e-05,
      "recall_at_20": 0.0,
      "support_cosine": -4.0531158440604287e-07
    },
    "failed_checks": [
      "cub_safe"
    ],
    "selected_epoch": 7,
    "selected_global_step": 119,
    "status": "dense_equivalence_gate_failed"
  },
  "best_global_step": 119,
  "best_rejected_epoch": 7,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 1.0,
      "dense_hit_loss_query_count": 0.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 1.0,
      "total_lost_dense_docs": 0.0,
      "total_net_dense_hit_delta": 1.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": false,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 4.4444444444513564e-05,
      "dense_overlap_at_256": 6.511356120730838e-05,
      "dense_overlap_at_50": 6.3492063491210615e-06,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 9.434907629612965e-07,
      "map_at_1000": -1.2033561893609601e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 6.33081664869195e-09,
      "ndcg_at_1000": -3.42189168146545e-05,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": -6.060606060613594e-06,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": -9.935419771489595e-05,
      "recall_at_20": 0.0,
      "support_cosine": -4.0531158440604287e-07
    },
    "failed_checks": [
      "cub_safe"
    ],
    "selected_epoch": 7,
    "selected_global_step": 119,
    "status": "dense_equivalence_gate_failed"
  },
  "best_rejected_global_step": 119,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -1.0,
    4.4444444444513564e-05,
    6.511356120730838e-05,
    0.0,
    9.434907629612965e-07,
    1.0,
    1.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": true,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
