# M680 Support Selector Seed 6545

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
    "challenger_ceiling_count_mean": 26.30597014925373,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 134.0,
    "fit_mse": 0.019705794343321738,
    "fit_mse_delta": -0.0002521289479501768,
    "moved_query_count": 133.0,
    "moved_query_rate": 0.9925373134328358,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.001154223436019696,
    "rejected_nonzero_count": 0.34328358208955223,
    "safe_nonzero_count": 5.656716417910448,
    "selected_scale": 0.00864179104477612,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00044776119402985113,
    "teacher_dense_overlap_256_delta": 0.0003206623134328358,
    "teacher_support_cosine_delta": -7.277104391980527e-07,
    "top100_boundary_pair_count_mean": 3.283582089552239,
    "top100_boundary_pair_query_rate": 0.41044776119402987
  },
  "test": {
    "challenger_ceiling_count_mean": 26.673621460506705,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 671.0,
    "fit_mse": 0.017416741827453574,
    "fit_mse_delta": -0.0002225626965473932,
    "moved_query_count": 670.0,
    "moved_query_rate": 0.9985096870342772,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011461330707201972,
    "rejected_nonzero_count": 0.38748137108792846,
    "safe_nonzero_count": 5.612518628912071,
    "selected_scale": 0.008552533532041728,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.00028315946348733174,
    "teacher_dense_overlap_256_delta": 0.0003376490312965723,
    "teacher_support_cosine_delta": -7.273365594946326e-07,
    "top100_boundary_pair_count_mean": 4.125186289120715,
    "top100_boundary_pair_query_rate": 0.5156482861400894
  },
  "train": {
    "challenger_ceiling_count_mean": 26.610800744878958,
    "challenger_ceiling_query_rate": 1.0,
    "example_count": 537.0,
    "fit_mse": 0.018068668843781163,
    "fit_mse_delta": -0.00023045591416888397,
    "moved_query_count": 534.0,
    "moved_query_rate": 0.994413407821229,
    "protected_doc_count_mean": 32.0,
    "protected_doc_query_rate": 1.0,
    "query_delta_l2": 0.0011266926560230678,
    "rejected_nonzero_count": 0.41154562383612664,
    "safe_nonzero_count": 5.588454376163873,
    "selected_scale": 0.008420856610800746,
    "swap_pair_count_mean": 128.0,
    "swap_pair_query_rate": 1.0,
    "teacher_active_support_delta": 0.0,
    "teacher_dense_overlap_100_delta": 0.000260707635009311,
    "teacher_dense_overlap_256_delta": 0.00024732309124767226,
    "teacher_support_cosine_delta": -7.147007592341532e-07,
    "top100_boundary_pair_count_mean": 4.052141527001862,
    "top100_boundary_pair_query_rate": 0.5065176908752328
  }
}
```

## Test Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.962644 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.783100 | 0.687432 | 0.854973 | 0.863128 |
| `m680_support_selector_m674_guard_seed6545` | 0.962369 | 0.935369 | 0.941366 | 0.944462 | 0.948118 | 0.783622 | 0.689414 | 0.853083 | 0.865933 |
| `p1_native` | 0.962369 | 0.935369 | 0.941366 | 0.944462 | 0.948072 | 0.783622 | 0.689413 | 0.853083 | 0.865933 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 4.6285840170412484e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 2.1085538326470754e-07,
  "map_at_1000": -2.586265870352378e-07,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": -1.2726846643218437e-07,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -6.954319986540725e-07,
  "ndcg_at_1000": -1.5892496200153872e-06,
  "ndcg_at_20": 0.0,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -1.1920928955078125e-07
}
```

## Full Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.960670 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.788058 | 0.694729 | 0.853308 | 0.872179 |
| `m680_support_selector_m674_guard_seed6545` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947975 | 0.786182 | 0.693946 | 0.851777 | 0.871168 |
| `p1_native` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947923 | 0.786182 | 0.693945 | 0.851777 | 0.871168 |

## Full Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 5.2295918367306626e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0,
  "map_at_100": 6.322320768026302e-07,
  "map_at_1000": 5.277769309364189e-07,
  "map_at_20": 0.0,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": -3.0880249579645636e-08,
  "mrr_at_20": 0.0,
  "ndcg_at_10": 0.0,
  "ndcg_at_100": -9.3647858112611e-07,
  "ndcg_at_1000": -1.0190048690406073e-06,
  "ndcg_at_20": -6.336415852015165e-07,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": -1.5894571947505653e-07
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
    "dense_overlap_at_256": 4.6285840170412484e-05,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0,
    "map_at_100": 2.1085538326470754e-07,
    "map_at_1000": -2.586265870352378e-07,
    "map_at_20": 0.0,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": -1.2726846643218437e-07,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0,
    "ndcg_at_100": -6.954319986540725e-07,
    "ndcg_at_1000": -1.5892496200153872e-06,
    "ndcg_at_20": 0.0,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -1.1920928955078125e-07
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
      "dense_overlap_at_256": 4.882812499995559e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": 5.748184439657855e-06,
      "map_at_1000": 5.428433455634263e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 2.823877579549716e-07,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 6.591426793711719e-06,
      "ndcg_at_1000": 6.278645322654874e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.390775045129189e-07
    },
    "failed_checks": [],
    "selected_epoch": 5,
    "selected_global_step": 45,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 45,
  "best_rejected_epoch": 11,
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
      "dense_overlap_at_256": 7.250236742417648e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -1.8142923274844236e-08,
      "map_at_1000": -1.4943231607666974e-06,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": -3.854455750795793e-06,
      "mrr_at_1000": -3.572067992951844e-06,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -2.6382271469671537e-06,
      "ndcg_at_1000": -3.4603843307445814e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -4.7286351523290193e-07
    },
    "failed_checks": [],
    "selected_epoch": 11,
    "selected_global_step": 99,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 99,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    7.250236742417648e-05,
    0.0,
    -1.8142923274844236e-08,
    0.0,
    0.0
  ],
  "best_score": [
    1.0,
    0.0,
    4.882812499995559e-05,
    -1.390775045129189e-07,
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
