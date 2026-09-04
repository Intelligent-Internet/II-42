# M659 Top100 Boundary Replay Shared15

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
| `m659_top100_boundary_replay` | 0.954413 | 0.945259 | 0.948841 | 0.744068 | 0.628645 | 0.828319 | 0.842907 |
| `p1_native` | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": -0.01273471320346331,
  "dense_overlap_at_10": -0.0022222222222221255,
  "dense_overlap_at_100": -0.00021885521885534054,
  "dense_overlap_at_256": -0.00037017308501674506,
  "dense_overlap_at_50": 0.0001707551707552657,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": -0.0004644326866548587,
  "map_at_100": -0.00040968698958110394,
  "map_at_1000": -0.0005602619802961284,
  "map_at_20": -0.0005369148179090732,
  "mrr_at_10": 0.0,
  "mrr_at_100": 0.0,
  "mrr_at_1000": -1.0667349379378521e-06,
  "mrr_at_20": 0.0,
  "ndcg_at_10": -0.00035587580451756917,
  "ndcg_at_100": -0.000286905516065028,
  "ndcg_at_1000": -0.00035364989284136783,
  "ndcg_at_20": -0.0003575640672184299,
  "precision_at_10": 0.0,
  "precision_at_100": -2.7755575615628914e-17,
  "precision_at_1000": 2.7777777777779344e-05,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0005681818181818565,
  "recall_at_1000": -7.615840949326547e-06,
  "recall_at_20": 0.0,
  "support_cosine": -9.477933247892256e-05
}
```

## Split Boundary Summaries

```json
{
  "dev": {
    "dense_hit_gain_query_count": 22.0,
    "dense_hit_loss_query_count": 30.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 24.0,
    "total_lost_dense_docs": 30.0,
    "total_net_dense_hit_delta": -6.0
  },
  "test": {
    "dense_hit_gain_query_count": 24.0,
    "dense_hit_loss_query_count": 29.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 24.0,
    "total_lost_dense_docs": 30.0,
    "total_net_dense_hit_delta": -6.0
  }
}
```

## Decision

```json
{
  "boundary": {
    "dense_hit_gain_query_count": 24.0,
    "dense_hit_loss_query_count": 29.0,
    "query_count": 134.0,
    "total_gained_dense_docs": 24.0,
    "total_lost_dense_docs": 30.0,
    "total_net_dense_hit_delta": -6.0
  },
  "checks": {
    "active_support_floor": false,
    "cub_safe": false,
    "dense_overlap_100_safe": false,
    "dense_overlap_256_safe": false,
    "dev_gate_passed_for_selected_checkpoint": false,
    "recall_safe": true,
    "support_cosine_floor": false,
    "swap_dense_hit_loss_query_safe": false,
    "trained_checkpoint_selected": true
  },
  "deltas": {
    "accuracy": 0.0,
    "active_support_recall": -0.01273471320346331,
    "dense_overlap_at_10": -0.0022222222222221255,
    "dense_overlap_at_100": -0.00021885521885534054,
    "dense_overlap_at_256": -0.00037017308501674506,
    "dense_overlap_at_50": 0.0001707551707552657,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": -0.0004644326866548587,
    "map_at_100": -0.00040968698958110394,
    "map_at_1000": -0.0005602619802961284,
    "map_at_20": -0.0005369148179090732,
    "mrr_at_10": 0.0,
    "mrr_at_100": 0.0,
    "mrr_at_1000": -1.0667349379378521e-06,
    "mrr_at_20": 0.0,
    "ndcg_at_10": -0.00035587580451756917,
    "ndcg_at_100": -0.000286905516065028,
    "ndcg_at_1000": -0.00035364989284136783,
    "ndcg_at_20": -0.0003575640672184299,
    "precision_at_10": 0.0,
    "precision_at_100": -2.7755575615628914e-17,
    "precision_at_1000": 2.7777777777779344e-05,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0005681818181818565,
    "recall_at_1000": -7.615840949326547e-06,
    "recall_at_20": 0.0,
    "support_cosine": -9.477933247892256e-05
  },
  "failed_checks": [
    "cub_safe",
    "dense_overlap_100_safe",
    "dense_overlap_256_safe",
    "active_support_floor",
    "support_cosine_floor",
    "swap_dense_hit_loss_query_safe",
    "dev_gate_passed_for_selected_checkpoint"
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
      "dense_hit_gain_query_count": 22.0,
      "dense_hit_loss_query_count": 30.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 24.0,
      "total_lost_dense_docs": 30.0,
      "total_net_dense_hit_delta": -6.0
    },
    "checks": {
      "active_support_floor": false,
      "cub_safe": false,
      "dense_overlap_100_safe": false,
      "dense_overlap_256_safe": false,
      "recall_safe": true,
      "support_cosine_floor": false,
      "swap_dense_hit_loss_query_safe": false,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": -0.01244084330021833,
      "dense_overlap_at_10": 0.0016168091168092813,
      "dense_overlap_at_100": -0.0005048100048101745,
      "dense_overlap_at_256": -0.0003014021568709113,
      "dense_overlap_at_50": -0.0008842009842009446,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 3.7237948760537876e-05,
      "map_at_100": 1.647967044593468e-05,
      "map_at_1000": -1.680619876776035e-05,
      "map_at_20": -8.998718405539563e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 8.218693283634781e-05,
      "mrr_at_1000": 8.164022702883411e-05,
      "mrr_at_20": 8.476372112731223e-05,
      "ndcg_at_10": 5.1755350753301954e-05,
      "ndcg_at_100": 0.0002549949376079441,
      "ndcg_at_1000": -4.6302350487525956e-05,
      "ndcg_at_20": -0.0002929979867193344,
      "precision_at_10": 0.0,
      "precision_at_100": -6.0606060606205325e-06,
      "precision_at_1000": -6.0606060606205325e-06,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0012000219985701666,
      "recall_at_1000": -6.489345580995565e-05,
      "recall_at_20": 0.0,
      "support_cosine": -9.612242380774294e-05
    },
    "failed_checks": [
      "cub_safe",
      "dense_overlap_100_safe",
      "dense_overlap_256_safe",
      "active_support_floor",
      "support_cosine_floor",
      "swap_dense_hit_loss_query_safe"
    ],
    "selected_epoch": 2,
    "selected_global_step": 34,
    "status": "dense_equivalence_gate_failed"
  },
  "best_global_step": 34,
  "best_rejected_epoch": 2,
  "best_rejected_gate": {
    "boundary": {
      "dense_hit_gain_query_count": 22.0,
      "dense_hit_loss_query_count": 30.0,
      "query_count": 134.0,
      "total_gained_dense_docs": 24.0,
      "total_lost_dense_docs": 30.0,
      "total_net_dense_hit_delta": -6.0
    },
    "checks": {
      "active_support_floor": false,
      "cub_safe": false,
      "dense_overlap_100_safe": false,
      "dense_overlap_256_safe": false,
      "recall_safe": true,
      "support_cosine_floor": false,
      "swap_dense_hit_loss_query_safe": false,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": -0.01244084330021833,
      "dense_overlap_at_10": 0.0016168091168092813,
      "dense_overlap_at_100": -0.0005048100048101745,
      "dense_overlap_at_256": -0.0003014021568709113,
      "dense_overlap_at_50": -0.0008842009842009446,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 3.7237948760537876e-05,
      "map_at_100": 1.647967044593468e-05,
      "map_at_1000": -1.680619876776035e-05,
      "map_at_20": -8.998718405539563e-06,
      "mrr_at_10": 0.0,
      "mrr_at_100": 8.218693283634781e-05,
      "mrr_at_1000": 8.164022702883411e-05,
      "mrr_at_20": 8.476372112731223e-05,
      "ndcg_at_10": 5.1755350753301954e-05,
      "ndcg_at_100": 0.0002549949376079441,
      "ndcg_at_1000": -4.6302350487525956e-05,
      "ndcg_at_20": -0.0002929979867193344,
      "precision_at_10": 0.0,
      "precision_at_100": -6.0606060606205325e-06,
      "precision_at_1000": -6.0606060606205325e-06,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0012000219985701666,
      "recall_at_1000": -6.489345580995565e-05,
      "recall_at_20": 0.0,
      "support_cosine": -9.612242380774294e-05
    },
    "failed_checks": [
      "cub_safe",
      "dense_overlap_100_safe",
      "dense_overlap_256_safe",
      "active_support_floor",
      "support_cosine_floor",
      "swap_dense_hit_loss_query_safe"
    ],
    "selected_epoch": 2,
    "selected_global_step": 34,
    "status": "dense_equivalence_gate_failed"
  },
  "best_rejected_global_step": 34,
  "best_rejected_score": [
    -30.0,
    -30.0,
    -6.0,
    -0.0005048100048101745,
    -0.0003014021568709113,
    0.0012000219985701666,
    1.647967044593468e-05,
    -6.0,
    24.0
  ],
  "global_steps": 204,
  "selected_rejected_checkpoint": true,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The boundary_constrained teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
