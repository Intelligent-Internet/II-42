# M654 / Coordinate Feasibility Report

Status: `coordinate_not_feasible`

M654 audits whether individual posting coordinates can promote
boundary positives without harming protected dense/P1 top100 support.
It is a feasibility audit, not a trained compiler.

## Counters

```json
{
  "dev": {
    "accepted": 30,
    "attempted": 67,
    "boundary_positive_queries": 67,
    "queries_with_positive_gain": 66,
    "queries_with_safe_coord": 0,
    "queries_without_boundary_positive": 200
  },
  "dev_boundary": {
    "accepted": 30,
    "attempted": 67,
    "boundary_positive_queries": 67,
    "queries_with_positive_gain": 66,
    "queries_with_safe_coord": 0,
    "queries_without_boundary_positive": 0
  },
  "test": {
    "accepted": 29,
    "attempted": 71,
    "boundary_positive_queries": 71,
    "queries_with_positive_gain": 67,
    "queries_with_safe_coord": 2,
    "queries_without_boundary_positive": 200
  },
  "test_boundary": {
    "accepted": 29,
    "attempted": 71,
    "boundary_positive_queries": 71,
    "queries_with_positive_gain": 67,
    "queries_with_safe_coord": 2,
    "queries_without_boundary_positive": 0
  }
}
```

## Test Boundary Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.93990 | 1.00000 | 0.58747 | 0.42113 | 0.62731 | 0.77023 |
| `m549_frozen` | 0.93990 | 1.00000 | 0.58747 | 0.42113 | 0.62731 | 0.77023 |
| `m654_coord_safe` | 0.94032 | 1.00000 | 0.58895 | 0.42225 | 0.62731 | 0.77371 |

## Test All Macro

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.96665 | 1.00000 | 0.78901 | 0.69680 | 0.86147 | 0.87512 |
| `m549_frozen` | 0.96665 | 1.00000 | 0.78901 | 0.69680 | 0.86147 | 0.87512 |
| `m654_coord_safe` | 0.96687 | 1.00000 | 0.78968 | 0.69718 | 0.86147 | 0.87679 |

## Top Candidate Dimensions

```json
{
  "170": 20,
  "192": 20,
  "276": 18,
  "331": 18,
  "380": 30,
  "381": 20,
  "383": 22,
  "39": 18,
  "391": 18,
  "400": 18,
  "405": 18,
  "418": 38,
  "450": 18,
  "504": 18,
  "507": 18,
  "522": 18,
  "538": 18,
  "540": 18,
  "548": 20,
  "625": 18,
  "664": 112,
  "691": 20,
  "719": 20,
  "76": 16,
  "91": 20
}
```

## Decision

```json
{
  "checks": {
    "accepted_safe_update": true,
    "all_dense_overlap_safe": true,
    "all_map_non_negative": true,
    "all_ndcg_non_negative": true,
    "boundary_recall_positive": false,
    "safe_coordinate_exists": true
  },
  "deltas": {
    "all": {
      "candidate_upper_bound": 0.00022178895395363885,
      "dense_overlap_at_100": 0.0,
      "map_at_100": 0.00038049604833423345,
      "mrr_at_20": 0.0016666666666668162,
      "ndcg_at_10": 0.0006712507922165267,
      "query_count": 0.0,
      "recall_at_100": 0.0
    },
    "boundary": {
      "candidate_upper_bound": 0.0004130510343676175,
      "dense_overlap_at_100": 0.0,
      "map_at_100": 0.0011262458285946075,
      "mrr_at_20": 0.00347222222222221,
      "ndcg_at_10": 0.001481588462896588,
      "query_count": 0.0,
      "recall_at_100": 0.0
    }
  },
  "failed_checks": [
    "boundary_recall_positive"
  ],
  "status": "coordinate_not_feasible"
}
```

## Conclusion

M654 found some safe-looking coordinates, but single-coordinate ranking simulation still failed the aggregate gate.  The next step would need support-set composition, not scalar coordinate movement.
