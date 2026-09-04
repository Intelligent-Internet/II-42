# M654Y Swap-Pair Preservation Shared15

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
    "fit_mse": 0.02011152272331598,
    "fit_mse_delta": -0.00023487343977608566,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
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
    "example_count": 134.0,
    "fit_mse": 0.018456742205698765,
    "fit_mse_delta": -0.00024204979886981978,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
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
    "example_count": 1074.0,
    "fit_mse": 0.017562325769878,
    "fit_mse_delta": -0.0002262308755481973,
    "moved_query_count": 1070.0,
    "moved_query_rate": 0.9962756052141527,
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
| `m654y_swap_pair` | 0.954421 | 0.945430 | 0.949181 | 0.744424 | 0.629029 | 0.827751 | 0.842907 |
| `p1_native` | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": -4.7619047619074095e-05,
  "dense_overlap_at_256": -3.0579229798100016e-05,
  "dense_overlap_at_50": 0.00011111111111128391,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -2.6153852769605912e-05,
  "map_at_1000": -2.660373091079027e-05,
  "map_at_20": -5.979181796234023e-06,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 5.463764315427611e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -4.707932772629597e-06,
  "ndcg_at_1000": -4.299853843203749e-06,
  "ndcg_at_20": -1.0786687183461652e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -2.4636586515303094e-07
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
    "dense_overlap_at_100": -4.7619047619074095e-05,
    "dense_overlap_at_256": -3.0579229798100016e-05,
    "dense_overlap_at_50": 0.00011111111111128391,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -2.6153852769605912e-05,
    "map_at_1000": -2.660373091079027e-05,
    "map_at_20": -5.979181796234023e-06,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 5.463764315427611e-07,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -4.707932772629597e-06,
    "ndcg_at_1000": -4.299853843203749e-06,
    "ndcg_at_20": -1.0786687183461652e-06,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -2.4636586515303094e-07
  },
  "failed_checks": [
    "dense_overlap_100_safe",
    "dense_overlap_256_safe"
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
      "dense_overlap_at_100": 2.7777777777848733e-05,
      "dense_overlap_at_256": 6.811041967302867e-05,
      "dense_overlap_at_50": -0.00010256410256415105,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -9.636122809686398e-07,
      "map_at_1000": -9.329422006043586e-07,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -6.432177619419832e-07,
      "ndcg_at_1000": -9.846570548921463e-07,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -2.622604370339232e-07
    },
    "failed_checks": [],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 17,
  "global_steps": 204,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
