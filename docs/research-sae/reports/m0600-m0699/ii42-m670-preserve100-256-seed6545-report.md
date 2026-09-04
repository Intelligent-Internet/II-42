# M670 Preserve Dense Top100/256 Seed 6545

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
    "teacher_support_cosine_delta": -6.885670903903334e-07,
    "top100_boundary_pair_count_mean": 4.0,
    "top100_boundary_pair_query_rate": 0.5
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
    "teacher_support_cosine_delta": -7.139213049589698e-07,
    "top100_boundary_pair_count_mean": 4.298507462686567,
    "top100_boundary_pair_query_rate": 0.5373134328358209
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
    "teacher_support_cosine_delta": -7.275762504705504e-07,
    "top100_boundary_pair_count_mean": 3.977653631284916,
    "top100_boundary_pair_query_rate": 0.4972067039106145
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.954660 | 1.000000 | 1.000000 | 0.746682 | 0.628131 | 0.828092 | 0.844676 |
| `m670_preserve100_256_seed6545` | 0.954421 | 0.945774 | 0.949508 | 0.744424 | 0.629030 | 0.827751 | 0.842907 |
| `p1_native` | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.00029629629629635,
  "dense_overlap_at_256": 0.00029690506252999516,
  "dense_overlap_at_50": -1.0101010101015717e-05,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -2.496137835039658e-05,
  "map_at_1000": -2.8071584881161904e-05,
  "map_at_20": -5.979181796234023e-06,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 5.463764315427611e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -1.6480762380366443e-06,
  "ndcg_at_1000": -3.781179929940315e-06,
  "ndcg_at_20": 5.9736486160977265e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -1.0371208191140013e-06
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
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 3.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 3.0
  }
}
```

## Decision

```json
{
  "boundary": {
    "dense_hit_gain_query_count": 3.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 3.0,
    "total_lost_dense_docs": 0.0,
    "total_net_dense_hit_delta": 3.0
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
    "dense_overlap_at_100": 0.00029629629629635,
    "dense_overlap_at_256": 0.00029690506252999516,
    "dense_overlap_at_50": -1.0101010101015717e-05,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -2.496137835039658e-05,
    "map_at_1000": -2.8071584881161904e-05,
    "map_at_20": -5.979181796234023e-06,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 5.463764315427611e-07,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -1.6480762380366443e-06,
    "ndcg_at_1000": -3.781179929940315e-06,
    "ndcg_at_20": 5.9736486160977265e-06,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -1.0371208191140013e-06
  },
  "failed_checks": [],
  "selected_epoch": 11,
  "selected_global_step": 187,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 11,
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
      "dense_overlap_at_100": 7.407407407400424e-05,
      "dense_overlap_at_256": 0.00032127739158993585,
      "dense_overlap_at_50": 6.410256410249726e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.102525371340597e-06,
      "map_at_1000": -9.866573660755051e-06,
      "map_at_20": 5.585812037467441e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": -2.711680562317298e-07,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -6.353897214150805e-05,
      "ndcg_at_100": -2.902222002165722e-05,
      "ndcg_at_1000": -3.006238418257201e-05,
      "ndcg_at_20": -3.8216058766527006e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.0848045348677005e-06
    },
    "failed_checks": [],
    "selected_epoch": 11,
    "selected_global_step": 187,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 187,
  "best_rejected_epoch": 7,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 2.0,
      "dense_hit_loss_query_count": 0.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 2.0,
      "total_lost_dense_docs": 0.0,
      "total_net_dense_hit_delta": 2.0
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
      "dense_overlap_at_100": 0.00014074074074066356,
      "dense_overlap_at_256": 0.00023003472222227206,
      "dense_overlap_at_50": 6.410256410249726e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -7.141364568807873e-06,
      "map_at_1000": -4.0513888059923175e-06,
      "map_at_20": 6.589487295816099e-07,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -6.353897214150805e-05,
      "ndcg_at_100": -2.8136683328239265e-05,
      "ndcg_at_1000": -2.418636515610917e-05,
      "ndcg_at_20": -3.917514435280989e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 6.66666666665483e-06,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 7.85237534350891e-06,
      "recall_at_20": 0.0,
      "support_cosine": -5.563100179406533e-07
    },
    "failed_checks": [],
    "selected_epoch": 7,
    "selected_global_step": 119,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 119,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.00014074074074066356,
    0.00023003472222227206,
    0.0,
    -7.141364568807873e-06,
    2.0,
    2.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler passed the canary first-stage gate.  Replay this checkpoint on full shared15 before promotion.
