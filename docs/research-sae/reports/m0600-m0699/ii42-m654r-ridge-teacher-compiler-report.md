# M654R Ridge-Teacher Query Compiler

Status: `dense_equivalence_gate_failed`

M654R trains the query-side compiler to distill the M653G `ridge_10`
teacher.  It freezes document postings and uses no BM25, reranker,
learned gate, or qrels-driven loss.  Qrels are used only for held-out
metric reporting.

## Example Counts

```json
{
  "dev": 40,
  "test": 40,
  "train": 320
}
```

## Test Macro

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.980000 | 1.000000 | 1.000000 | 0.772234 | 0.697093 | 0.960000 | 0.829854 |
| `m654r_ridge_teacher` | 0.980000 | 0.956107 | 0.952753 | 0.763688 | 0.695081 | 0.960000 | 0.828980 |
| `p1_native` | 0.980000 | 0.955238 | 0.953185 | 0.762908 | 0.693685 | 0.960000 | 0.828366 |

## Test Deltas vs P1

```json
{
  "accuracy": 0.0,
  "active_support_recall": 0.0,
  "dense_overlap_at_10": -0.0024999999999999467,
  "dense_overlap_at_100": 0.0008690476190474916,
  "dense_overlap_at_256": -0.00043247767857135244,
  "dense_overlap_at_50": 0.001071428571428612,
  "hit_rate_at_10": 0.0,
  "hit_rate_at_100": 0.0,
  "hit_rate_at_1000": 0.0,
  "hit_rate_at_20": 0.0,
  "map_at_10": 0.0014251700680272261,
  "map_at_100": 0.0013960078970751688,
  "map_at_1000": 0.001392201247018221,
  "map_at_20": 0.0014291244469815112,
  "mrr_at_10": 0.0004251700680271142,
  "mrr_at_100": 0.0006145640074212144,
  "mrr_at_1000": 0.0006145640074212144,
  "mrr_at_20": 0.0006145640074212144,
  "ndcg_at_10": 0.0007800434828071356,
  "ndcg_at_100": 0.0008599394223501866,
  "ndcg_at_1000": 0.0008601153953256002,
  "ndcg_at_20": 0.0008756201114175077,
  "precision_at_10": 0.0,
  "precision_at_100": 0.0,
  "precision_at_1000": 0.0,
  "precision_at_20": 0.0,
  "recall_at_10": 0.0,
  "recall_at_100": 0.0,
  "recall_at_1000": 0.0,
  "recall_at_20": 0.0,
  "support_cosine": 0.0
}
```

## Decision

```json
{
  "checks": {
    "active_support_floor": true,
    "cub_safe": true,
    "dense_overlap_100_safe": true,
    "dense_overlap_256_safe": false,
    "recall_safe": true,
    "support_cosine_floor": true,
    "trained_checkpoint_selected": true
  },
  "deltas": {
    "accuracy": 0.0,
    "active_support_recall": 0.0,
    "dense_overlap_at_10": -0.0024999999999999467,
    "dense_overlap_at_100": 0.0008690476190474916,
    "dense_overlap_at_256": -0.00043247767857135244,
    "dense_overlap_at_50": 0.001071428571428612,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.0014251700680272261,
    "map_at_100": 0.0013960078970751688,
    "map_at_1000": 0.001392201247018221,
    "map_at_20": 0.0014291244469815112,
    "mrr_at_10": 0.0004251700680271142,
    "mrr_at_100": 0.0006145640074212144,
    "mrr_at_1000": 0.0006145640074212144,
    "mrr_at_20": 0.0006145640074212144,
    "ndcg_at_10": 0.0007800434828071356,
    "ndcg_at_100": 0.0008599394223501866,
    "ndcg_at_1000": 0.0008601153953256002,
    "ndcg_at_20": 0.0008756201114175077,
    "precision_at_10": 0.0,
    "precision_at_100": 0.0,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.0,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": 0.0
  },
  "failed_checks": [
    "dense_overlap_256_safe"
  ],
  "selected_epoch": 7,
  "selected_global_step": 35,
  "status": "dense_equivalence_gate_failed"
}
```

## Training

```json
{
  "best_epoch": 7,
  "best_gate": {
    "checks": {
      "active_support_floor": true,
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true,
      "support_cosine_floor": true,
      "trained_checkpoint_selected": true
    },
    "deltas": {
      "accuracy": 0.0,
      "active_support_recall": 0.0,
      "dense_overlap_at_10": -0.002083333333333326,
      "dense_overlap_at_100": 0.00016666666666675933,
      "dense_overlap_at_256": 0.0005208333333334147,
      "dense_overlap_at_50": 8.333333333332416e-05,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0017460317460317176,
      "map_at_100": 0.001825056169048378,
      "map_at_1000": 0.001626713446706618,
      "map_at_20": 0.0017080487165626046,
      "mrr_at_10": 0.0,
      "mrr_at_100": -6.808278867098672e-05,
      "mrr_at_1000": -6.808278867098672e-05,
      "mrr_at_20": -6.808278867098672e-05,
      "ndcg_at_10": 0.0011190759005763606,
      "ndcg_at_100": 0.002059321873777442,
      "ndcg_at_1000": 0.0010207927595592414,
      "ndcg_at_20": 0.0010594808356322316,
      "precision_at_10": 0.0,
      "precision_at_100": 0.0002083333333333312,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": 0.0,
      "recall_at_100": 0.004166666666666652,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0,
      "support_cosine": 0.0
    },
    "failed_checks": [],
    "selected_epoch": 7,
    "selected_global_step": 35,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 35,
  "global_steps": 60,
  "selected_trained_checkpoint": true
}
```

## Conclusion

The ridge-teacher compiler did not pass the canary first-stage gate.  This means the M653G oracle capacity has not yet transferred into the current global compiler architecture.
