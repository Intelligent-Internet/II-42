# M672 Broad50 M671 Guard Seed 6547

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
    "challenger_ceiling_count_mean": 26.865671641791046,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.018769628354417745,
    "fit_mse_delta": -0.000234725377730914,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011231271412576297,
    "rejected_nonzero_count": 0.373134328358209,
    "safe_nonzero_count": 5.626865671641791,
    "selected_scale": 0.008421641791044779,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 7.462686567164186e-05,
    "teacher_dense_overlap_256_delta": 0.00020405783582089552,
    "teacher_support_cosine_delta": -7.103628187037226e-07,
    "top100_boundary_pair_count_mean": 4.7164179104477615,
    "top100_boundary_pair_query_rate": 0.5895522388059702
  },
  "test": {
    "challenger_ceiling_count_mean": 26.58569299552906,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 671.0,
    "fit_mse": 0.017723883294424765,
    "fit_mse_delta": -0.00022846153047596468,
    "moved_query_count": 670.0,
    "moved_query_rate": 0.9985096870342772,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011371211156470209,
    "rejected_nonzero_count": 0.38152011922503726,
    "safe_nonzero_count": 5.618479880774963,
    "selected_scale": 0.008535022354694487,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00032786885245901585,
    "teacher_dense_overlap_256_delta": 0.00036675670640834576,
    "teacher_support_cosine_delta": -7.1587754432917e-07,
    "top100_boundary_pair_count_mean": 3.9225037257824145,
    "top100_boundary_pair_query_rate": 0.4903129657228018
  },
  "train": {
    "challenger_ceiling_count_mean": 26.58100558659218,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 537.0,
    "fit_mse": 0.017918490665336965,
    "fit_mse_delta": -0.00022742790829050364,
    "moved_query_count": 533.0,
    "moved_query_rate": 0.9925512104283054,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011457130011854853,
    "rejected_nonzero_count": 0.41154562383612664,
    "safe_nonzero_count": 5.588454376163873,
    "selected_scale": 0.008497672253258846,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778406,
    "teacher_dense_overlap_256_delta": 0.00024004888268156424,
    "teacher_support_cosine_delta": -7.333480224041077e-07,
    "top100_boundary_pair_count_mean": 3.9478584729981376,
    "top100_boundary_pair_query_rate": 0.4934823091247672
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.962701 | 1.000000 | 1.000000 | 0.778521 | 0.680410 | 0.846107 | 0.854542 |
| `m672_m671_guard_seed6547` | 0.962435 | 0.943077 | 0.948551 | 0.777594 | 0.679921 | 0.845018 | 0.853031 |
| `p1_native` | 0.962435 | 0.943077 | 0.948337 | 0.777618 | 0.679944 | 0.845018 | 0.853024 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.00021387789735538565,
  "dense_overlap_at_50": -0.00025574480501322494,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": -2.777777777773771e-05,
  "map_at_100": -2.2881095408266994e-05,
  "map_at_1000": -2.3592197632926748e-05,
  "map_at_20": -2.3587677905112514e-05,
  "mrr_at_10": 0.0,
  "mrr_at_100": 6.412264429656034e-06,
  "mrr_at_1000": 6.329096615020902e-06,
  "mrr_at_20": 7.054673721418325e-06,
  "ndcg_at_10": -2.451978359852003e-05,
  "ndcg_at_100": -1.4652848871699042e-05,
  "ndcg_at_1000": -1.5465952876492217e-05,
  "ndcg_at_20": -1.6409684710172456e-05,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -5.722045898215455e-07
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
    "dense_overlap_at_256": 0.00021387789735538565,
    "dense_overlap_at_50": -0.00025574480501322494,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": -2.777777777773771e-05,
    "map_at_100": -2.2881095408266994e-05,
    "map_at_1000": -2.3592197632926748e-05,
    "map_at_20": -2.3587677905112514e-05,
    "mrr_at_10": 0.0,
    "mrr_at_100": 6.412264429656034e-06,
    "mrr_at_1000": 6.329096615020902e-06,
    "mrr_at_20": 7.054673721418325e-06,
    "ndcg_at_10": -2.451978359852003e-05,
    "ndcg_at_100": -1.4652848871699042e-05,
    "ndcg_at_1000": -1.5465952876492217e-05,
    "ndcg_at_20": -1.6409684710172456e-05,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -5.722045898215455e-07
  },
  "failed_checks": [],
  "selected_epoch": 9,
  "selected_global_step": 81,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 9,
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
      "dense_overlap_at_256": 0.00014178240740747583,
      "dense_overlap_at_50": -0.0002878787878787259,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 6.666674478017498e-07,
      "map_at_1000": 1.5781639587375551e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.00012006900301875234,
      "ndcg_at_100": 2.6510861028228305e-05,
      "ndcg_at_1000": 9.290002542461373e-06,
      "ndcg_at_20": 7.748861411061014e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -5.841255187322147e-07
    },
    "failed_checks": [],
    "selected_epoch": 9,
    "selected_global_step": 81,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 81,
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
      "dense_overlap_at_256": 0.00013888888888902162,
      "dense_overlap_at_50": -0.0002878787878787259,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 1.3404227004798486e-05,
      "map_at_1000": 1.3763797107269049e-05,
      "map_at_20": 2.269426289025489e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 1.2441273598007285e-06,
      "ndcg_at_1000": 2.063679718555811e-06,
      "ndcg_at_20": 5.33006462921648e-06,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -6.874402364021037e-07
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
    0.00013888888888902162,
    0.0,
    1.3404227004798486e-05,
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
