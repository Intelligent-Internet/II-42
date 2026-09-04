# M679 Support-Aware Dense Selector Seed 6547

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

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.962701 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.778521 | 0.680410 | 0.846107 | 0.854542 |
| `m679_support_selector_m674_guard_seed6547` | 0.962435 | 0.936463 | 0.941552 | 0.943077 | 0.948438 | 0.777953 | 0.680315 | 0.845018 | 0.853765 |
| `p1_native` | 0.962435 | 0.936463 | 0.941552 | 0.943077 | 0.948337 | 0.777618 | 0.679944 | 0.845018 | 0.853024 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 0.00010156814111927925,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0003703703703704653,
  "map_at_100": 0.00037141383249539306,
  "map_at_1000": 0.0003711400203738613,
  "map_at_20": 0.0003674152322979962,
  "mrr_at_10": 0.0007407407407407085,
  "mrr_at_100": 0.0007407407407407085,
  "mrr_at_1000": 0.0007406497087083519,
  "mrr_at_20": 0.0007407407407409305,
  "ndcg_at_10": 0.0003352509415271987,
  "ndcg_at_100": 0.0003378254821255755,
  "ndcg_at_1000": 0.00033735724010808266,
  "ndcg_at_20": 0.0003351661854222865,
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

## Full Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.960670 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.788058 | 0.694729 | 0.853308 | 0.872179 |
| `m679_support_selector_m674_guard_seed6547` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.948007 | 0.786333 | 0.694110 | 0.851777 | 0.871502 |
| `p1_native` | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947923 | 0.786182 | 0.693945 | 0.851777 | 0.871168 |

## Full Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": 0.0,
  "dense_overlap_at_100": 0.0,
  "dense_overlap_at_256": 8.428749406741431e-05,
  "dense_overlap_at_50": 0.0,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0001666666666666483,
  "map_at_100": 0.0001644431742096053,
  "map_at_1000": 0.00016396170300725377,
  "map_at_20": 0.00016546310699627753,
  "mrr_at_10": 0.00033333333333340764,
  "mrr_at_100": 0.0003327731092437336,
  "mrr_at_1000": 0.0003327322357776774,
  "mrr_at_20": 0.0003333333333332966,
  "ndcg_at_10": 0.0001508629236873782,
  "ndcg_at_100": 0.00014979332330555817,
  "ndcg_at_1000": 0.00014937697623851331,
  "ndcg_at_20": 0.00015129102300981412,
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
    "dense_overlap_at_256": 0.00010156814111927925,
    "dense_overlap_at_50": 0.0,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0003703703703704653,
    "map_at_100": 0.00037141383249539306,
    "map_at_1000": 0.0003711400203738613,
    "map_at_20": 0.0003674152322979962,
    "mrr_at_10": 0.0007407407407407085,
    "mrr_at_100": 0.0007407407407407085,
    "mrr_at_1000": 0.0007406497087083519,
    "mrr_at_20": 0.0007407407407409305,
    "ndcg_at_10": 0.0003352509415271987,
    "ndcg_at_100": 0.0003378254821255755,
    "ndcg_at_1000": 0.00033735724010808266,
    "ndcg_at_20": 0.0003351661854222865,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": -1.5894571947505653e-07
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
      "dense_overlap_at_256": 9.114583333347248e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -4.293514910269636e-08,
      "map_at_1000": -3.480996760840682e-07,
      "map_at_20": 0.0,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -8.04230493978153e-07,
      "ndcg_at_1000": -1.201134858797026e-06,
      "ndcg_at_20": 0.0,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.5894571936403423e-07
    },
    "failed_checks": [],
    "selected_epoch": 5,
    "selected_global_step": 45,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 45,
  "best_rejected_epoch": 32,
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
      "dense_overlap_at_256": 9.868314555838698e-05,
      "dense_overlap_at_50": 0.0,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -8.711090438329183e-05,
      "map_at_1000": -8.633201983720973e-05,
      "map_at_20": -6.54622133099414e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": -3.774417073565406e-05,
      "ndcg_at_1000": -3.806046536780627e-05,
      "ndcg_at_20": -3.0208277241317028e-05,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.0,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": -1.0649363199055628e-06
    },
    "failed_checks": [],
    "selected_epoch": 32,
    "selected_global_step": 288,
    "status": "dense_equivalence_gate_passed"
  },
  "best_rejected_global_step": 288,
  "best_rejected_score": [
    -0.0,
    -0.0,
    -0.0,
    0.0,
    9.868314555838698e-05,
    0.0,
    -8.711090438329183e-05,
    0.0,
    0.0
  ],
  "best_score": [
    1.0,
    0.0,
    9.114583333347248e-05,
    -1.5894571936403423e-07,
    0.0
  ],
  "global_steps": 324,
  "selected_rejected_checkpoint": false,
  "selected_trained_checkpoint": true
}
```

## Conclusion

M679 validates the M678 diagnosis.  The support-aware dense-only checkpoint
selector chooses epoch `5` instead of the M677 legacy-selected epoch `32`.

This recovers the M675/M676 seed `6547` result while using a longer 36-epoch
training run:

- no O@10/O@50/O@100 regression;
- positive O@256;
- no Recall@100 or CUB regression;
- positive full shared15 MAP@100, NDCG@10, and MRR@20.

M679 should not be described as a new model-quality breakthrough.  It is a
selector correction that prevents deeper training from selecting a support-
drifted checkpoint.  The current first-stage candidate remains equivalent to
M675/M676 seed `6547`, now with an explicit dense-only selection rule that can
reproduce that choice.
