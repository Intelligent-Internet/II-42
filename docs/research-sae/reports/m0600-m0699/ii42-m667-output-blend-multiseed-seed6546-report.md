# M667 Output Blend Multiseed Seed6546

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
    "challenger_ceiling_count_mean": 26.53731343283582,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.01920453342956616,
    "fit_mse_delta": -0.00024845975159264324,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011854776221841361,
    "rejected_nonzero_count": 0.3208955223880597,
    "safe_nonzero_count": 5.67910447761194,
    "selected_scale": 0.00878171641791045,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492475,
    "teacher_dense_overlap_256_delta": 0.0003206623134328358,
    "teacher_support_cosine_delta": -7.566231400219362e-07,
    "top100_boundary_pair_count_mean": 3.8805970149253732,
    "top100_boundary_pair_query_rate": 0.48507462686567165
  },
  "test": {
    "challenger_ceiling_count_mean": 26.619402985074625,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.018357134420674905,
    "fit_mse_delta": -0.00023154750442021152,
    "moved_query_count": 134.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.001119140550959421,
    "rejected_nonzero_count": 0.3880597014925373,
    "safe_nonzero_count": 5.611940298507463,
    "selected_scale": 0.008352611940298509,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029850746268656744,
    "teacher_dense_overlap_256_delta": 0.000378964552238806,
    "teacher_support_cosine_delta": -7.112524402675344e-07,
    "top100_boundary_pair_count_mean": 4.059701492537314,
    "top100_boundary_pair_query_rate": 0.5074626865671642
  },
  "train": {
    "challenger_ceiling_count_mean": 26.620111731843576,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 1074.0,
    "fit_mse": 0.017687916094417732,
    "fit_mse_delta": -0.00022584609125846618,
    "moved_query_count": 1069.0,
    "moved_query_rate": 0.9953445065176909,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011358811470791829,
    "rejected_nonzero_count": 0.4022346368715084,
    "safe_nonzero_count": 5.597765363128492,
    "selected_scale": 0.008494180633147113,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778374,
    "teacher_dense_overlap_256_delta": 0.0002873312383612663,
    "teacher_support_cosine_delta": -7.194180728336952e-07,
    "top100_boundary_pair_count_mean": 4.022346368715084,
    "top100_boundary_pair_query_rate": 0.5027932960893855
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.955341 | 1.000000 | 1.000000 | 0.742692 | 0.652181 | 0.829456 | 0.823247 |
| `m667_output_blend_seed6546` | 0.953805 | 0.945189 | 0.947534 | 0.735580 | 0.648287 | 0.827488 | 0.818647 |
| `p1_native` | 0.953805 | 0.945189 | 0.947641 | 0.735580 | 0.648287 | 0.827488 | 0.818647 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": -0.00010686289983163899,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -6.743438115686118e-07,
  "map_at_1000": -7.991216715019789e-07,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": -1.0946821116242234e-08,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -3.220711676243937e-07,
  "ndcg_at_1000": -1.1638484416209849e-06,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -5.1657358723922187e-08
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
    "dense_overlap_at_100": 0.0,
    "dense_overlap_at_256": -0.00010686289983163899,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -6.743438115686118e-07,
    "map_at_1000": -7.991216715019789e-07,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": -1.0946821116242234e-08,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -3.220711676243937e-07,
    "ndcg_at_1000": -1.1638484416209849e-06,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -5.1657358723922187e-08
  },
  "failed_checks": [
    "dense_overlap_256_safe"
  ],
  "selected_epoch": 2,
  "selected_global_step": 34,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 2,
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
      "dense_overlap_at_100": 0.00033333333333340764,
      "dense_overlap_at_256": 5.963654401153207e-05,
      "dense_overlap_at_50": -9.523809523814819e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": -0.00011111111111106187,
      "map_at_100": -0.00011190344273181996,
      "map_at_1000": -0.00011355300523907541,
      "map_at_20": -0.00011111111111117289,
      "mrr_at_10": 0.0,
      "mrr_at_100": -2.5696371672712104e-06,
      "mrr_at_1000": -2.2590106335540483e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -8.257373516862287e-05,
      "ndcg_at_100": -8.461565091644196e-05,
      "ndcg_at_1000": -8.452838428385512e-05,
      "ndcg_at_20": -8.25737351687339e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -4.768371586472142e-08
    },
    "failed_checks": [],
    "selected_epoch": 2,
    "selected_global_step": 34,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 34,
  "best_rejected_epoch": 1,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 1.0,
      "dense_hit_loss_query_count": 1.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 1.0,
      "total_lost_dense_docs": 1.0,
      "total_net_dense_hit_delta": 0.0
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
      "dense_overlap_at_100": 0.0,
      "dense_overlap_at_256": 2.3674242424220893e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": -0.00011111111111106187,
      "map_at_100": -0.00011190344273181996,
      "map_at_1000": -0.00011216184147921915,
      "map_at_20": -0.00011111111111117289,
      "mrr_at_10": 0.0,
      "mrr_at_100": -2.5696371672712104e-06,
      "mrr_at_1000": -2.5696371672712104e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -8.257373516862287e-05,
      "ndcg_at_100": -8.461565091644196e-05,
      "ndcg_at_1000": -8.487907556631313e-05,
      "ndcg_at_20": -8.25737351687339e-05,
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
    0.0,
    2.3674242424220893e-05,
    0.0,
    -0.00011190344273181996,
    0.0,
    1.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
