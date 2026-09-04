# M672 Broad50 M671 Guard Seed 6548

Status: `dense_equivalence_gate_failed`

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
    "challenger_ceiling_count_mean": 26.71641791044776,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.0189920849950442,
    "fit_mse_delta": -0.00024732585072016983,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011650991252838085,
    "rejected_nonzero_count": 0.3656716417910448,
    "safe_nonzero_count": 5.634328358208955,
    "selected_scale": 0.008649253731343285,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00022388059701492557,
    "teacher_dense_overlap_256_delta": 0.0002915111940298507,
    "teacher_support_cosine_delta": -7.468373028200064e-07,
    "top100_boundary_pair_count_mean": 4.059701492537314,
    "top100_boundary_pair_query_rate": 0.5074626865671642
  },
  "test": {
    "challenger_ceiling_count_mean": 26.634873323397912,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 671.0,
    "fit_mse": 0.01797585225993373,
    "fit_mse_delta": -0.00022411544046652032,
    "moved_query_count": 671.0,
    "moved_query_rate": 1.0,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011201714006278047,
    "rejected_nonzero_count": 0.43219076005961254,
    "safe_nonzero_count": 5.567809239940387,
    "selected_scale": 0.008389344262295081,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029806259314456,
    "teacher_dense_overlap_256_delta": 0.0002968982861400894,
    "teacher_support_cosine_delta": -7.097483036592714e-07,
    "top100_boundary_pair_count_mean": 4.065573770491803,
    "top100_boundary_pair_query_rate": 0.5081967213114754
  },
  "train": {
    "challenger_ceiling_count_mean": 26.556797020484172,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 537.0,
    "fit_mse": 0.017548136166825865,
    "fit_mse_delta": -0.0002297142453776114,
    "moved_query_count": 533.0,
    "moved_query_rate": 0.9925512104283054,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011564187980539883,
    "rejected_nonzero_count": 0.3500931098696462,
    "safe_nonzero_count": 5.649906890130354,
    "selected_scale": 0.00862290502793296,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00029795158286778384,
    "teacher_dense_overlap_256_delta": 0.0003055167597765363,
    "teacher_support_cosine_delta": -7.319050794207184e-07,
    "top100_boundary_pair_count_mean": 3.9329608938547485,
    "top100_boundary_pair_query_rate": 0.49162011173184356
  }
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.963807 | 1.000000 | 1.000000 | 0.783514 | 0.690950 | 0.854685 | 0.864999 |
| `m672_m671_guard_seed6548` | 0.963346 | 0.944369 | 0.948200 | 0.781537 | 0.689930 | 0.852919 | 0.863267 |
| `p1_native` | 0.963369 | 0.944369 | 0.948005 | 0.781226 | 0.689659 | 0.852919 | 0.862492 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.0001953473341319567,
  "dense_overlap_at_50": -3.192981773392223e-05,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.00028658679821480515,
  "map_at_100": 0.0002710772058860389,
  "map_at_1000": 0.0002723754638542797,
  "map_at_20": 0.0002835303049620874,
  "mrr_at_10": 0.0007751937984494806,
  "mrr_at_100": 0.0007748245358669692,
  "mrr_at_1000": 0.0007750260061674119,
  "mrr_at_20": 0.0007751937984497026,
  "ndcg_at_10": 0.0003114112612893072,
  "ndcg_at_100": 0.00030477283699004154,
  "ndcg_at_1000": 0.0002983174171504732,
  "ndcg_at_20": 0.00030931418490431906,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": -1.3333333333392927e-06,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": -2.292711955431681e-05,
  "recall_at_20": 0.0,
  "support_cosine": -7.629394531694089e-07
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
    "dense_overlap_at_100": 0.0,
    "dense_overlap_at_256": 0.0001953473341319567,
    "dense_overlap_at_50": -3.192981773392223e-05,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.00028658679821480515,
    "map_at_100": 0.0002710772058860389,
    "map_at_1000": 0.0002723754638542797,
    "map_at_20": 0.0002835303049620874,
    "mrr_at_10": 0.0007751937984494806,
    "mrr_at_100": 0.0007748245358669692,
    "mrr_at_1000": 0.0007750260061674119,
    "mrr_at_20": 0.0007751937984497026,
    "ndcg_at_10": 0.0003114112612893072,
    "ndcg_at_100": 0.00030477283699004154,
    "ndcg_at_1000": 0.0002983174171504732,
    "ndcg_at_20": 0.00030931418490431906,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": -1.3333333333392927e-06,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": -2.292711955431681e-05,
    "recall_at_20": 0.0,
    "support_cosine": -7.629394531694089e-07
  },
  "failed_checks": [
    "cub_safe"
  ],
  "selected_epoch": 11,
  "selected_global_step": 99,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 11,
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
      "dense_overlap_at_256": 0.00019081417289124047,
      "dense_overlap_at_50": -0.00014814814814823052,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": -7.843137254892607e-05,
      "map_at_100": -8.174251902370866e-05,
      "map_at_1000": -7.855850171123446e-05,
      "map_at_20": -8.021199932961576e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": -3.552145495744341e-06,
      "mrr_at_1000": -3.5521454958553633e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -5.8287342471929904e-05,
      "ndcg_at_100": -7.017676049125843e-05,
      "ndcg_at_1000": -6.760193233323886e-05,
      "ndcg_at_20": -6.39157662462031e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -7.589658101991859e-07
    },
    "failed_checks": [],
    "selected_epoch": 11,
    "selected_global_step": 99,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 99,
  "best_rejected_epoch": 12,
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
      "dense_overlap_at_256": 0.00017549554544027401,
      "dense_overlap_at_50": -0.0002693602693603081,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": -7.843137254892607e-05,
      "map_at_100": -8.12616966251456e-05,
      "map_at_1000": -7.870515464214467e-05,
      "map_at_20": -8.021199932961576e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": -3.552145495744341e-06,
      "mrr_at_1000": -3.5521454958553633e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": -5.8287342471929904e-05,
      "ndcg_at_100": -6.865007211787066e-05,
      "ndcg_at_1000": -6.741083762384914e-05,
      "ndcg_at_20": -6.39157662462031e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -8.225440979447995e-07
    },
    "failed_checks": [],
    "selected_epoch": 12,
    "selected_global_step": 108,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 108,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    0.00017549554544027401,
    0.0,
    -8.12616966251456e-05,
    0.0,
    0.0
  ],
  "global_steps": 108,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
