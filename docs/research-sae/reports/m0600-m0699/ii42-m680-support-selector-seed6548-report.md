# M680 Support Selector Seed 6548

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
| `m680_support_selector_m674_guard_seed6548` | 0.963369 | 0.935494 | 0.942160 | 0.944369 | 0.948091 | 0.781186 | 0.689551 | 0.852919 | 0.862492 |
| `p1_native` | 0.963369 | 0.935494 | 0.942160 | 0.944369 | 0.948005 | 0.781226 | 0.689659 | 0.852919 | 0.862492 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 8.619495778106057e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": -0.00010101010101004615,
  "map_at_100": -0.00010780438286939198,
  "map_at_1000": -0.00010634523918517935,
  "map_at_20": -0.00010333399353001926,
  "mrr_at_10": 0.0,
  "mrr_at_100": -3.692625825113538e-07,
  "mrr_at_1000": -2.676111534594128e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": -3.9432747285905734e-05,
  "ndcg_at_100": -4.439654794619052e-05,
  "ndcg_at_1000": -4.33240755458586e-05,
  "ndcg_at_20": -4.04344629780784e-05,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -1.4702479045336503e-07
}
```

## Full Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.960670 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.788058 | 0.694729 | 0.853308 | 0.872179 |
| `m680_support_selector_m674_guard_seed6548` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.948010 | 0.786181 | 0.693885 | 0.851777 | 0.871168 |
| `p1_native` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947923 | 0.786182 | 0.693945 | 0.851777 | 0.871168 |

## Full Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 8.678536821704608e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": -5.5555555555586444e-05,
  "map_at_100": -6.034277389488363e-05,
  "map_at_1000": -5.896185458464931e-05,
  "map_at_20": -5.803639918200432e-05,
  "mrr_at_10": 0.0,
  "mrr_at_100": -4.96680377159997e-07,
  "mrr_at_1000": -4.5083297617320994e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": -7.988548976767262e-07,
  "ndcg_at_100": -1.8329507374903464e-05,
  "ndcg_at_1000": -2.068839273916545e-05,
  "ndcg_at_20": -7.1717447981400895e-06,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -1.629193624452796e-07
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
    "dense_overlap_at_256": 8.619495778106057e-05,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": -0.00010101010101004615,
    "map_at_100": -0.00010780438286939198,
    "map_at_1000": -0.00010634523918517935,
    "map_at_20": -0.00010333399353001926,
    "mrr_at_10": 0.0,
    "mrr_at_100": -3.692625825113538e-07,
    "mrr_at_1000": -2.676111534594128e-07,
    "mrr_at_20": 0.0,
    "ndcg_at_10": -3.9432747285905734e-05,
    "ndcg_at_100": -4.439654794619052e-05,
    "ndcg_at_1000": -4.33240755458586e-05,
    "ndcg_at_20": -4.04344629780784e-05,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -1.4702479045336503e-07
  },
  "failed_checks": [],
  "selected_epoch": 5,
  "selected_global_step": 45,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 5,
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
      "dense_overlap_at_256": 8.042279411757391e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 1.3148782942451476e-06,
      "map_at_1000": 3.957935631682119e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": -1.813861529753602e-06,
      "mrr_at_1000": -1.813861529753602e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -2.2467162019701448e-06,
      "ndcg_at_1000": -1.345980432665428e-07,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.5497207639381116e-07
    },
    "failed_checks": [],
    "selected_epoch": 5,
    "selected_global_step": 45,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 45,
  "best_rejected_epoch": 31,
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
      "dense_overlap_at_256": 0.0002154278693800027,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 5.6809117138234555e-05,
      "map_at_1000": 5.7007769363903726e-05,
      "map_at_20": 5.7997557997624405e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 1.8576815550952475e-05,
      "ndcg_at_1000": 1.89612988513721e-05,
      "ndcg_at_20": 2.0866486423964936e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -9.377797445253577e-07
    },
    "failed_checks": [],
    "selected_epoch": 31,
    "selected_global_step": 279,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 279,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    0.0002154278693800027,
    0.0,
    5.6809117138234555e-05,
    0.0,
    0.0
  ],
  "best_score": [
    1.0,
    0.0,
    8.042279411757391e-05,
    -1.5497207639381116e-07,
    0.0
  ],
  "global_steps": 324,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

This seed passes the first-stage dense-equivalence gate.  See
`docs/research-sae/reports/m0600-m0699/ii42-m680-support-selector-multiseed-report.md` for the aggregate decision:
the support-aware selector is useful diagnostically but is not promoted as the
final default selector.
