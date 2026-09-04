# M675 Full Shared15 M674 Guard Seed 6548

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

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.963807 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.783514 | 0.690950 | 0.854685 | 0.864999 |
| `m675_full_m674_guard_seed6548` | 0.963369 | 0.935494 | 0.942160 | 0.944369 | 0.948116 | 0.781226 | 0.689652 | 0.852919 | 0.862492 |
| `p1_native` | 0.963369 | 0.935494 | 0.942160 | 0.944369 | 0.948005 | 0.781226 | 0.689659 | 0.852919 | 0.862492 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.00011061058194405948,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": -6.317827788038244e-06,
  "map_at_1000": -4.527825076539038e-06,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": -3.692625825113538e-07,
  "mrr_at_1000": -2.2395027976518378e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -4.2012273184788285e-06,
  "ndcg_at_1000": -3.167888114052886e-06,
  "ndcg_at_20": 5.756993743943895e-07,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -3.5762786865234375e-07
}
```

## Full Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.960670 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.788058 | 0.694729 | 0.853308 | 0.872179 |
| `m675_full_m674_guard_seed6548` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.948025 | 0.786182 | 0.693949 | 0.851777 | 0.871168 |
| `p1_native` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947923 | 0.786182 | 0.693945 | 0.851777 | 0.871168 |

## Full Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.00010166879251682914,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 3.947141297144796e-06,
  "map_at_1000": 5.108442524881518e-06,
  "map_at_20": 7.182694507723575e-06,
  "mrr_at_10": 0.0,
  "mrr_at_100": -4.96680377159997e-07,
  "mrr_at_1000": -4.3837207497343655e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": 4.116141083310154e-06,
  "ndcg_at_1000": 4.442996488163153e-06,
  "ndcg_at_20": 7.155277336745591e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -3.3378601083100534e-07
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
  "full": {
    "dense_hit_gain_query_count": 0.0,
    "dense_hit_loss_query_count": 0.0,
    "query_count": 1342.0,
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
    "dense_overlap_at_256": 0.00011061058194405948,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": -6.317827788038244e-06,
    "map_at_1000": -4.527825076539038e-06,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": -3.692625825113538e-07,
    "mrr_at_1000": -2.2395027976518378e-07,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -4.2012273184788285e-06,
    "ndcg_at_1000": -3.167888114052886e-06,
    "ndcg_at_20": 5.756993743943895e-07,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -3.5762786865234375e-07
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
      "dense_overlap_at_256": 0.00010935797930278213,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -3.374867579841734e-07,
      "map_at_1000": 1.3439108198509686e-06,
      "map_at_20": -1.7806267805786646e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": -1.813861529753602e-06,
      "mrr_at_1000": -1.813861529753602e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -3.6222223364079653e-06,
      "ndcg_at_1000": -2.742665801913624e-06,
      "ndcg_at_20": -5.628423774384217e-06,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -3.9339065549537366e-07
    },
    "failed_checks": [],
    "selected_epoch": 9,
    "selected_global_step": 81,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 81,
  "best_rejected_epoch": 6,
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
      "dense_overlap_at_256": 8.042279411757391e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 9.623176241424503e-07,
      "map_at_1000": 2.141853121528925e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": -1.813861529753602e-06,
      "mrr_at_1000": -1.813861529753602e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -2.321015688533379e-06,
      "ndcg_at_1000": -1.371953833650963e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -2.1060307819897872e-07
    },
    "failed_checks": [],
    "selected_epoch": 6,
    "selected_global_step": 54,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 54,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    8.042279411757391e-05,
    0.0,
    9.623176241424503e-07,
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
