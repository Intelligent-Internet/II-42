# M668 Output Blend Scale 0.3 Seed 6547

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
    "challenger_ceiling_count_mean": 26.970149253731343,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.016026671870295377,
    "fit_mse_delta": -0.00020388249610203194,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011233750108169886,
    "rejected_nonzero_count": 0.3880597014925373,
    "safe_nonzero_count": 5.611940298507463,
    "selected_scale": 0.008399253731343285,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492475,
    "teacher_dense_overlap_256_delta": 0.0003206623134328358,
    "teacher_support_cosine_delta": -7.205934666875583e-07,
    "top100_boundary_pair_count_mean": 3.8208955223880596,
    "top100_boundary_pair_query_rate": 0.47761194029850745
  },
  "test": {
    "challenger_ceiling_count_mean": 26.544776119402986,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.019907238727275615,
    "fit_mse_delta": -0.00026391575429171544,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011818592682772502,
    "rejected_nonzero_count": 0.35074626865671643,
    "safe_nonzero_count": 5.649253731343284,
    "selected_scale": 0.008815298507462686,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.0002985074626865666,
    "teacher_dense_overlap_256_delta": 0.0004664179104477612,
    "teacher_support_cosine_delta": -7.53064653766689e-07,
    "top100_boundary_pair_count_mean": 4.0,
    "top100_boundary_pair_query_rate": 0.5
  },
  "train": {
    "challenger_ceiling_count_mean": 26.575418994413408,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 1074.0,
    "fit_mse": 0.01789100685033746,
    "fit_mse_delta": -0.00022736937501355035,
    "moved_query_count": 1070.0,
    "moved_query_rate": 0.9962756052141527,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011358042772492095,
    "rejected_nonzero_count": 0.3985102420856611,
    "safe_nonzero_count": 5.601489757914339,
    "selected_scale": 0.00848417132216015,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778384,
    "teacher_dense_overlap_256_delta": 0.00027641992551210426,
    "teacher_support_cosine_delta": -7.186966013420005e-07,
    "top100_boundary_pair_count_mean": 4.037243947858473,
    "top100_boundary_pair_query_rate": 0.5046554934823091
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.961834 | 1.000000 | 1.000000 | 0.808277 | 0.704682 | 0.856420 | 0.886612 |
| `m668_blend030_seed6547` | 0.961664 | 0.943465 | 0.947901 | 0.809959 | 0.706039 | 0.856012 | 0.882718 |
| `p1_native` | 0.961664 | 0.943465 | 0.947851 | 0.809959 | 0.706039 | 0.856012 | 0.882718 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 4.9913194444473064e-05,
  "dense_overlap_at_50": -0.0001666666666665373,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 9.279515934546012e-07,
  "map_at_1000": 5.549144316274734e-07,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": 0.0,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 1.2598088653392736e-06,
  "ndcg_at_1000": 9.751745694286384e-07,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -2.781550090258378e-08
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
    "dev_gate_passed_for_selected_checkpoint": false,
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
    "dense_overlap_at_256": 4.9913194444473064e-05,
    "dense_overlap_at_50": -0.0001666666666665373,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 9.279515934546012e-07,
    "map_at_1000": 5.549144316274734e-07,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": 0.0,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": 1.2598088653392736e-06,
    "ndcg_at_1000": 9.751745694286384e-07,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -2.781550090258378e-08
  },
  "failed_checks": [
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
      "dense_overlap_256_safe": false,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 9.523809523803717e-05,
      "dense_overlap_at_256": -0.00017636551816246904,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 1.1847548832766286e-05,
      "map_at_1000": 1.4196514734998722e-05,
      "map_at_20": 6.349206349232084e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 3.5289052568998613e-08,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 7.0288378977156896e-06,
      "ndcg_at_1000": 8.500557421076138e-06,
      "ndcg_at_20": 5.305965714796912e-06,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -3.5762786843029915e-08
    },
    "failed_checks": [
      "dense_overlap_256_safe"
    ],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_failed"
  },
  "best_global_step": 17,
  "best_rejected_epoch": 1,
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
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": false,
      "recall_safe": true,
      "support_cosine_not_regressed": true,
      "swap_dense_hit_loss_query_safe": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": 0.0,
      "dense_overlap_at_100": 9.523809523803717e-05,
      "dense_overlap_at_256": -0.00017636551816246904,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 1.1847548832766286e-05,
      "map_at_1000": 1.4196514734998722e-05,
      "map_at_20": 6.349206349232084e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 3.5289052568998613e-08,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 7.0288378977156896e-06,
      "ndcg_at_1000": 8.500557421076138e-06,
      "ndcg_at_20": 5.305965714796912e-06,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -3.5762786843029915e-08
    },
    "failed_checks": [
      "dense_overlap_256_safe"
    ],
    "selected_epoch": 1,
    "selected_global_step": 17,
    "status": "dense_equivalence_gate_failed"
  },
  "best_rejected_global_step": 17,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -1.0,
    9.523809523803717e-05,
    -0.00017636551816246904,
    0.0,
    1.1847548832766286e-05,
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
