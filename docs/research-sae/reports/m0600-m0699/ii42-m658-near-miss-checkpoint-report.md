# M658 Near-Miss Rejected Checkpoint Shared15

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
    "challenger_ceiling_count_mean": 26.567164179104477,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.02011152272331598,
    "fit_mse_delta": -0.00023487343977608566,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011075920938038955,
    "rejected_nonzero_count": 0.417910447761194,
    "safe_nonzero_count": 5.582089552238806,
    "selected_scale": 0.008328358208955224,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0002985074626865666,
    "teacher_dense_overlap_256_delta": 0.0004955690298507463,
    "teacher_support_cosine_delta": -6.885670903903334e-07
  },
  "test": {
    "challenger_ceiling_count_mean": 26.73134328358209,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.018456742205698765,
    "fit_mse_delta": -0.00024204979886981978,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.001131124740022099,
    "rejected_nonzero_count": 0.3880597014925373,
    "safe_nonzero_count": 5.611940298507463,
    "selected_scale": 0.00848134328358209,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 7.462686567164186e-05,
    "teacher_dense_overlap_256_delta": 0.000378964552238806,
    "teacher_support_cosine_delta": -7.139213049589698e-07
  },
  "train": {
    "challenger_ceiling_count_mean": 26.602420856610802,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 1074.0,
    "fit_mse": 0.017562325769878,
    "fit_mse_delta": -0.0002262308755481973,
    "moved_query_count": 1070.0,
    "moved_query_rate": 0.9962756052141527,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011441034743310948,
    "rejected_nonzero_count": 0.39013035381750466,
    "safe_nonzero_count": 5.609869646182496,
    "selected_scale": 0.008534683426443205,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0003165735567970202,
    "teacher_dense_overlap_256_delta": 0.00026550861266294226,
    "teacher_support_cosine_delta": -7.275762504705504e-07
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.954660 | 1.000000 | 1.000000 | 0.746682 | 0.628131 | 0.828092 | 0.844676 |
| `m658_near_miss_swap` | 0.954421 | 0.945073 | 0.949188 | 0.744424 | 0.629197 | 0.828346 | 0.842907 |
| `p1_native` | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": -0.0004045214045214074,
  "dense_overlap_at_256": -2.334543350168694e-05,
  "dense_overlap_at_50": -5.555555555547542e-05,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 0.00014234514556688005,
  "map_at_1000": -2.556370006967512e-05,
  "map_at_20": -5.979181796234023e-06,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 0.00014830474343108246,
  "ndcg_at_1000": -5.74991654234136e-06,
  "ndcg_at_20": -1.0786687183461652e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.00016666666666667607,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0005952380952380931,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -3.8146972658470446e-07
}
```

## Split Boundary Summaries

```json
{
  "dev": {
    "dense_hit_gain_query_count": 4.0,
    "dense_hit_loss_query_count": 1.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 4.0,
    "total_lost_dense_docs": 1.0,
    "total_net_dense_hit_delta": 3.0
  },
  "test": {
    "dense_hit_gain_query_count": 2.0,
    "dense_hit_loss_query_count": 7.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 2.0,
    "total_lost_dense_docs": 7.0,
    "total_net_dense_hit_delta": -5.0
  }
}
```

## Decision

```json
{
  "boundary": {
    "dense_hit_gain_query_count": 2.0,
    "dense_hit_loss_query_count": 7.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 2.0,
    "total_lost_dense_docs": 7.0,
    "total_net_dense_hit_delta": -5.0
  },
  "checks": {
    "active_support_not_regressed": true,
    "cub_safe": true,
    "dense_overlap_100_safe": false,
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
    "dense_overlap_at_100": -0.0004045214045214074,
    "dense_overlap_at_256": -2.334543350168694e-05,
    "dense_overlap_at_50": -5.555555555547542e-05,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 0.00014234514556688005,
    "map_at_1000": -2.556370006967512e-05,
    "map_at_20": -5.979181796234023e-06,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": 0.00014830474343108246,
    "ndcg_at_1000": -5.74991654234136e-06,
    "ndcg_at_20": -1.0786687183461652e-06,
    "precision_at_10": 0.0,
    "precision_at_100": 0.00016666666666667607,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0005952380952380931,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -3.8146972658470446e-07
  },
  "failed_checks": [
    "dense_overlap_100_safe",
    "dense_overlap_256_safe",
    "swap_dense_hit_loss_query_safe",
    "dev_gate_passed_for_selected_checkpoint"
  ],
  "selected_epoch": 1,
  "selected_global_step": 17,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 1,
  "best_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 4.0,
      "dense_hit_loss_query_count": 1.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 4.0,
      "total_lost_dense_docs": 1.0,
      "total_net_dense_hit_delta": 3.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": false,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 0.00028333333333330213,
      "dense_overlap_at_256": 8.062394781149784e-05,
      "dense_overlap_at_50": 6.410256410249726e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.672863756970962e-06,
      "map_at_1000": -9.757498672491316e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -3.0048402737126167e-06,
      "ndcg_at_1000": -1.6134794558197996e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -4.0531158451706517e-07
    },
    "failed_checks": [
      "swap_dense_hit_loss_query_safe"
    ],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_failed"
  },
  "best_global_step": 17,
  "best_rejected_epoch": 1,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 4.0,
      "dense_hit_loss_query_count": 1.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 4.0,
      "total_lost_dense_docs": 1.0,
      "total_net_dense_hit_delta": 3.0
    },
    "checks": {
      "active_support_not_regressed": true,
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": false,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 0.00028333333333330213,
      "dense_overlap_at_256": 8.062394781149784e-05,
      "dense_overlap_at_50": 6.410256410249726e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.672863756970962e-06,
      "map_at_1000": -9.757498672491316e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -3.0048402737126167e-06,
      "ndcg_at_1000": -1.6134794558197996e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -4.0531158451706517e-07
    },
    "failed_checks": [
      "swap_dense_hit_loss_query_safe"
    ],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_failed"
  },
  "best_rejected_global_step": 17,
  "best_rejected_score": [
    -1.0,
    -1.0,
    -1.0,
    0.00028333333333330213,
    8.062394781149784e-05,
    0.0,
    -9.672863756970962e-06,
    3.0,
    4.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": true,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
