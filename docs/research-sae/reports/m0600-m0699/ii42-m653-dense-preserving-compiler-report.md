# M653 Dense-Preserving Compiler Report

Status: `dense_equivalence_gate_passed`

This canary trains only the query-side output compiler from M653-D
dense-boundary rows.  Training rows explicitly mark `qrels_used=false`
and `bm25_used=false`; qrels are used only for held-out metric
reporting.

## Boundary Rows

```json
{
  "dev": 1842,
  "test": 2491,
  "train": 7853
}
```

## Test Ranking Macro

| Source | CUB | O@10 | O@50 | O@100 | O@256 | Support cos | Active recall | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.983333 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | n/a | n/a | 0.659914 | 0.575559 | 0.863747 | 0.766896 |
| `m653_dense_boundary` | 0.983333 | 0.942662 | 0.943188 | 0.947799 | 0.950880 | 0.999996 | 1.000000 | 0.661969 | 0.580774 | 0.864728 | 0.775430 |
| `p1_native` | 0.983333 | 0.940281 | 0.943517 | 0.947674 | 0.950880 | 0.999996 | 1.000000 | 0.661792 | 0.580396 | 0.863047 | 0.775430 |

## Test Boundary Rows

| Dataset | Pairs | Target margin | Generated margin | Margin error | Positive-margin rate |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 761 | 0.008702 | -0.008245 | 0.016948 | 0.002628 |
| `cqadupstack` | 424 | 0.009556 | -0.007767 | 0.017324 | 0.007075 |
| `fiqa` | 684 | 0.009617 | -0.007277 | 0.016895 | 0.020468 |
| `scidocs` | 622 | 0.008173 | -0.007788 | 0.015961 | 0.012862 |

## Decision

```json
{
  "checks": {
    "active_support_floor": true,
    "boundary_positive_margin_improved": true,
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
    "dense_overlap_at_10": 0.0023809523809522615,
    "dense_overlap_at_100": 0.00012581902326158634,
    "dense_overlap_at_256": 0.0,
    "dense_overlap_at_50": -0.000328900255754383,
    "hit_rate_at_10": 0.0,
    "hit_rate_at_100": 0.0,
    "hit_rate_at_1000": 0.0,
    "hit_rate_at_20": 0.0,
    "map_at_10": 0.00023809523809525945,
    "map_at_100": 0.0003779472523834926,
    "map_at_1000": 0.00039664099205394265,
    "map_at_20": 0.00033848563580018975,
    "mrr_at_10": 0.0,
    "mrr_at_100": 2.3274216822577465e-05,
    "mrr_at_1000": 2.32732436320493e-05,
    "mrr_at_20": 0.0,
    "ndcg_at_10": 0.0001769437182186362,
    "ndcg_at_100": 0.0005687106264575803,
    "ndcg_at_1000": 0.00026255063480973284,
    "ndcg_at_20": 0.00022264465249866028,
    "precision_at_10": 0.0,
    "precision_at_100": -2.8011204481790536e-05,
    "precision_at_1000": 0.0,
    "precision_at_20": 0.0,
    "recall_at_10": 0.0,
    "recall_at_100": 0.001680672268907557,
    "recall_at_1000": 0.0,
    "recall_at_20": 0.0,
    "support_cosine": 0.0
  },
  "failed_checks": [],
  "selected_epoch": 2,
  "selected_global_step": 62,
  "status": "dense_equivalence_gate_passed"
}
```

## Training

```json
{
  "best_epoch": 2,
  "best_gate": {
    "checks": {
      "active_support_floor": true,
      "boundary_positive_margin_improved": true,
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
      "dense_overlap_at_10": -0.00011904761904757422,
      "dense_overlap_at_100": 0.00031398809523808247,
      "dense_overlap_at_256": 0.0004999069940476719,
      "dense_overlap_at_50": -0.0007142857142857784,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": 0.0,
      "map_at_100": -6.259783687068143e-06,
      "map_at_1000": -8.556095751788284e-06,
      "map_at_20": -5.41125541125842e-05,
      "mrr_at_10": 0.0,
      "mrr_at_100": 0.0,
      "mrr_at_1000": 0.0,
      "mrr_at_20": 0.0,
      "ndcg_at_10": 0.0,
      "ndcg_at_100": 1.000428835862266e-05,
      "ndcg_at_1000": 2.2413691203038155e-06,
      "ndcg_at_20": -2.954582678249551e-05,
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
    "failed_checks": [],
    "selected_epoch": 2,
    "selected_global_step": 62,
    "status": "dense_equivalence_gate_passed"
  },
  "best_global_step": 62,
  "global_steps": 248,
  "selected_trained_checkpoint": true
}
```

## Conclusion

A trained dense-boundary compiler passed the dense-equivalence gate.  The next step is to replay it on the full shared15/native matrix before considering any retrieval-expanded objective.
