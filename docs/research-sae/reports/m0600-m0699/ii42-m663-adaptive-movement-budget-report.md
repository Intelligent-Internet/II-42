# M663 Adaptive Movement Budget Report

Status: `adaptive_budget_gate_passed`

M663 evaluates post-hoc movement budgets on the trained M658
near-miss checkpoint. It does not use BM25, reranking, learned gates,
or qrels loss. The only variable is how much of the generated query
movement is allowed before scoring against frozen P1 document postings.

## Summary

| Policy | Status | Failed checks | Test gain/lossQ | O@100 | O@256 | R@100 | MAP@100 | support | active |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw_m658` | `dense_equivalence_gate_failed` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | 2.0/7.0 | -0.000404521 | -0.000023345 | 0.000595238 | 0.000142345 | -0.000000366 | 0.000000000 |
| `fixed_scale_0_5` | `dense_equivalence_gate_failed` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | 2.0/2.0 | -0.000005051 | -0.000023345 | 0.000000000 | -0.000005049 | -0.000000083 | 0.000000000 |
| `fixed_scale_0_25` | `dense_equivalence_gate_passed` | `` | 1.0/0.0 | 0.000055556 | 0.000030908 | 0.000000000 | -0.000004691 | -0.000000016 | 0.000000000 |
| `fixed_scale_0_1` | `dense_equivalence_gate_failed` | `dense_overlap_256_safe` | 1.0/0.0 | 0.000055556 | -0.000021701 | 0.000000000 | 0.000003600 | 0.000000040 | 0.000000000 |
| `adaptive_1e_6_to_1e_5` | `dense_equivalence_gate_failed` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | 2.0/7.0 | -0.000404521 | -0.000045047 | 0.000595238 | 0.000142345 | -0.000000362 | 0.000000000 |
| `adaptive_3e_6_to_3e_5` | `dense_equivalence_gate_failed` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | 1.0/6.0 | -0.000293410 | -0.000045047 | 0.000000000 | -0.000020336 | -0.000000330 | 0.000000000 |
| `adaptive_1e_5_to_1e_4` | `dense_equivalence_gate_failed` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | 0.0/2.0 | -0.000111111 | -0.000045047 | 0.000000000 | -0.000020210 | -0.000000314 | 0.000000000 |

## Best Policy

```json
{
  "active_support_recall_delta": 0.0,
  "dense_overlap_at_100_delta": 5.555555555547542e-05,
  "dense_overlap_at_256_delta": 3.0908038720522946e-05,
  "failed_checks": [],
  "map_at_100_delta": -4.690680695307314e-06,
  "policy": "fixed_scale_0_25",
  "recall_at_100_delta": 0.0,
  "status": "dense_equivalence_gate_passed",
  "support_cosine_delta": -1.5894571991914574e-08,
  "test_gain_query_count": 1.0,
  "test_loss_query_count": 0.0
}
```

## Interpretation

At least one post-hoc movement budget passed the strict dense-equivalence gate. The passing policy is a fixed damped movement, not a margin-threshold policy. This suggests the useful shape is train with enough movement to learn the direction, then apply a conservative output projection at inference.
