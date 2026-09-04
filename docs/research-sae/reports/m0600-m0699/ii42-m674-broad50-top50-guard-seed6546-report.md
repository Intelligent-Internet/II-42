# M674 Broad50 Top50 Guard Seed 6546

Status: `dense_equivalence_gate_passed`

This run trains the query-side compiler to distill an M653G-style
ridge teacher.  It freezes document postings and uses no BM25,
reranker, learned gate, or qrels-driven loss.  Qrels are used only
for held-out metric reporting.

## Example Counts

```json
{
  "dev": 134,
  "test": 671,
  "train": 537
}
```

## Teacher Summary

```json
{
  "dev": {
    "challenger_ceiling_count_mean": 26.83582089552239,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.018311151156528618,
    "fit_mse_delta": -0.00024013729581831773,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011544330867655477,
    "rejected_nonzero_count": 0.3805970149253731,
    "safe_nonzero_count": 5.619402985074627,
    "selected_scale": 0.008675373134328361,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0005223880597014922,
    "teacher_dense_overlap_256_delta": 0.0003498134328358209,
    "teacher_support_cosine_delta": -7.321585470171117e-07,
    "top100_boundary_pair_count_mean": 3.701492537313433,
    "top100_boundary_pair_query_rate": 0.4626865671641791
  },
  "test": {
    "challenger_ceiling_count_mean": 26.61251862891207,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 671.0,
    "fit_mse": 0.01827547834738853,
    "fit_mse_delta": -0.00023727813973639788,
    "moved_query_count": 668.0,
    "moved_query_rate": 0.9955290611028316,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.001161958434487023,
    "rejected_nonzero_count": 0.3502235469448584,
    "safe_nonzero_count": 5.649776453055142,
    "selected_scale": 0.00864269746646796,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00034277198211624387,
    "teacher_dense_overlap_256_delta": 0.00032018442622950817,
    "teacher_support_cosine_delta": -7.463460885288108e-07,
    "top100_boundary_pair_count_mean": 4.089418777943368,
    "top100_boundary_pair_query_rate": 0.511177347242921
  },
  "train": {
    "challenger_ceiling_count_mean": 26.554934823091248,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 537.0,
    "fit_mse": 0.01734365949956128,
    "fit_mse_delta": -0.00021506079127474426,
    "moved_query_count": 536.0,
    "moved_query_rate": 0.9981378026070763,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.001106866003718628,
    "rejected_nonzero_count": 0.44878957169459965,
    "safe_nonzero_count": 5.5512104283054,
    "selected_scale": 0.008299813780260708,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00016759776536312863,
    "teacher_dense_overlap_256_delta": 0.00026187150837988826,
    "teacher_support_cosine_delta": -6.898377416742136e-07,
    "top100_boundary_pair_count_mean": 3.992551210428305,
    "top100_boundary_pair_query_rate": 0.49906890130353815
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.962133 | 1.000000 | 1.000000 | 0.774388 | 0.696245 | 0.860852 | 0.856505 |
| `m674_top50_guard_seed6546` | 0.961238 | 0.944159 | 0.947030 | 0.772241 | 0.695164 | 0.858761 | 0.854072 |
| `p1_native` | 0.961238 | 0.944159 | 0.946945 | 0.772241 | 0.695168 | 0.858761 | 0.854066 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 8.460859439485269e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -3.817976454323357e-06,
  "map_at_1000": -4.643712668506161e-06,
  "map_at_20": 3.654970760158527e-06,
  "mrr_at_10": 0.0,
  "mrr_at_100": 4.563217856246027e-06,
  "mrr_at_1000": 4.606365740844609e-06,
  "mrr_at_20": 5.341880341847016e-06,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -4.591255154950957e-07,
  "ndcg_at_1000": -3.359437003025434e-07,
  "ndcg_at_20": 3.1236200079831633e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -3.019968668471762e-07
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
    "query_count": 671.0,
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
    "query_count": 671.0,
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
    "dense_overlap_at_256": 8.460859439485269e-05,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -3.817976454323357e-06,
    "map_at_1000": -4.643712668506161e-06,
    "map_at_20": 3.654970760158527e-06,
    "mrr_at_10": 0.0,
    "mrr_at_100": 4.563217856246027e-06,
    "mrr_at_1000": 4.606365740844609e-06,
    "mrr_at_20": 5.341880341847016e-06,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -4.591255154950957e-07,
    "ndcg_at_1000": -3.359437003025434e-07,
    "ndcg_at_20": 3.1236200079831633e-06,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -3.019968668471762e-07
  },
  "failed_checks": [],
  "selected_epoch": 8,
  "selected_global_step": 72,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 8,
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
      "dense_overlap_at_256": 0.00015388257575754682,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.32280553200826e-07,
      "map_at_1000": -1.3641565288491364e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -6.704924849776361e-07,
      "ndcg_at_1000": -5.187967360154033e-07,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -2.90075937936507e-07
    },
    "failed_checks": [],
    "selected_epoch": 8,
    "selected_global_step": 72,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 72,
  "best_rejected_epoch": 10,
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
      "dense_overlap_at_256": 0.00015388257575754682,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.32280553200826e-07,
      "map_at_1000": -6.862749230007736e-07,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -6.236134115766845e-07,
      "ndcg_at_1000": -1.259181160451206e-07,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -3.894170125251506e-07
    },
    "failed_checks": [],
    "selected_epoch": 10,
    "selected_global_step": 90,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 90,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    0.00015388257575754682,
    0.0,
    -9.32280553200826e-07,
    0.0,
    0.0
  ],
  "global_steps": 108,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler passed the canary first-stage gate.  Replay this checkpoint on full shared15 before promotion.
