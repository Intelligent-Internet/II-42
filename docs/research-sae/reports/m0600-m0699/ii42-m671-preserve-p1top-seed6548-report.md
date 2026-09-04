# M671 Preserve Dense TopK and P1 TopK Seed 6548

Status: `dense_equivalence_gate_passed`

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
| `m671_preserve_p1top_seed6548` | 0.981680 | 0.945482 | 0.950402 | 0.793727 | 0.714728 | 0.869123 | 0.887965 |
| `p1_native` | 0.981680 | 0.945482 | 0.950341 | 0.793727 | 0.714731 | 0.869123 | 0.887965 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 6.087662337661559e-05,
  "dense_overlap_at_50": -0.000523809523809593,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -2.1093708800767885e-06,
  "map_at_1000": -2.018772486178655e-06,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -3.6163594350213657e-06,
  "ndcg_at_1000": -3.380900671645648e-06,
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

## Split Boundary Summaries

```json
{
  "dev": {
    "dense_hit_gain_query_count": 0.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 0.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 0.0
  },
  "test": {
    "dense_hit_gain_query_count": 0.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 0.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 0.0
  }
}
```

## Decision

```json
{
  "boundary": {
    "dense_hit_gain_query_count": 0.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 0.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 0.0
  },
  "checks": {
    "active_support_not_regressed": true,
    "cub_safe": true,
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
    "dense_overlap_at_100": 0.0,
    "dense_overlap_at_256": 6.087662337661559e-05,
    "dense_overlap_at_50": -0.000523809523809593,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -2.1093708800767885e-06,
    "map_at_1000": -2.018772486178655e-06,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -3.6163594350213657e-06,
    "ndcg_at_1000": -3.380900671645648e-06,
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
  "selected_epoch": 1,
  "selected_global_step": 17,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 1,
  "best_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 0.0,
      "dense_hit_loss_query_count": 0.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 0.0,
      "total_lost_dense_docs": 0.0,
      "total_net_dense_hit_delta": 0.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": true,
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
      "dense_overlap_at_100": 0.0,
      "dense_overlap_at_256": 0.0,
      "dense_overlap_at_50": 0.00012121212121207758,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 5.941770647610589e-06,
      "map_at_1000": 5.328681131011592e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 4.002868103203383e-06,
      "ndcg_at_1000": 3.9121346862014406e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -7.947285940446136e-09
    },
    "failed_checks": [],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 17,
  "best_rejected_epoch": 2,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 0.0,
      "dense_hit_loss_query_count": 0.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 0.0,
      "total_lost_dense_docs": 0.0,
      "total_net_dense_hit_delta": 0.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": true,
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
      "dense_overlap_at_100": 0.0,
      "dense_overlap_at_256": 0.0,
      "dense_overlap_at_50": 0.00021645021645011475,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -3.3952975129203367e-06,
      "map_at_1000": -4.665440124562714e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": -9.337068160641948e-06,
      "mrr_at_1000": -9.337068160641948e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -1.285824459940077e-05,
      "ndcg_at_1000": -1.2985779357377325e-05,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -3.1789143872806847e-08
    },
    "failed_checks": [],
    "selected_epoch": 2,
    "selected_global_step": 34,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 34,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    0.0,
    0.0,
    -3.3952975129203367e-06,
    0.0,
    0.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler passed the canary first-stage gate.  Replay this checkpoint on full shared15 before promotion.
