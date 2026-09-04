# M667 Output Blend Multiseed Report

## Summary

M667 replayed the M666 first-stage candidate across additional random
all-query splits/seeds. It keeps the same first-stage scope:

- frozen P1 document postings;
- query-side compiler only;
- no BM25;
- no reranker;
- no learned gate;
- no qrels loss;
- strict dense-equivalence selection.

M666 showed that train-high / infer-damped output projection can pass the
strict gate for seed `6545`. M667 tests whether that is stable enough to
promote.

Result: positive but not stable enough. The mechanism remains useful, but
`output_blend_scale=0.25` is not yet a robust default across seeds.

## Runs

Shared configuration:

- `output_blend_scale=0.25`
- `output_lock_active=true`
- `relative_support_gate=true`
- `support_regression_tolerance=1e-5`
- `delta_scale=0.001`
- `swap_preservation_weight=1.0`
- `swap_gate_max_dense_hit_loss_queries=0`

| Seed | Status | Selected | Rejected fallback | Failed checks | Dev gain/lossQ | Test gain/lossQ |
| --- | --- | --- | --- | --- | --- | --- |
| 6545 | `dense_equivalence_gate_passed` | `e1/s17` | `false` | `` | `1/0` | `1/0` |
| 6546 | `dense_equivalence_gate_failed` | `e2/s34` | `false` | `dense_overlap_256_safe` | `1/0` | `0/0` |
| 6547 | `dense_equivalence_gate_failed` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `1/0` | `0/0` |
| 6548 | `dense_equivalence_gate_failed` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `0/0` | `0/0` |

Test deltas versus P1:

| Seed | O@100 | O@256 | R@100 | MAP@100 | CUB | support | active |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `+0.000055556` | `+0.000030908` | `0.000000000` | `-0.000004691` | `0.000000000` | `-0.000000004` | `0.000000000` |
| 6546 | `0.000000000` | `-0.000106863` | `0.000000000` | `-0.000000674` | `0.000000000` | `-0.000000052` | `0.000000000` |
| 6547 | `0.000000000` | `+0.000017361` | `0.000000000` | `+0.000000146` | `0.000000000` | `-0.000000020` | `0.000000000` |
| 6548 | `0.000000000` | `+0.000037202` | `0.000000000` | `-0.000002109` | `0.000000000` | `0.000000000` | `0.000000000` |

## Interpretation

M667 preserves the important part of M666:

- no seed shows test Recall@100 or CUB regression;
- no seed shows test dense-hit loss queries;
- active support is stable after output active-lock;
- most failures are very small gate-specific issues, not broad collapse.

But promotion is not justified yet:

- only one of four seeds fully passes the strict dev/test gate;
- seed `6546` fails test O@256 despite clean lossQ;
- seeds `6547` and `6548` rely on rejected fallback because dev did not pass.

This means M666 is a real first-stage signal, but the fixed `0.25` output
blend is not robust enough.

## Conclusion

Keep the route:

- train-high / infer-damped output projection;
- output active-lock;
- relative support gate;
- strict no-loss selection.

Do not promote `output_blend_scale=0.25` as a frozen default yet.

Next step:

- run a global output-blend sweep around this mechanism, for example
  `0.15`, `0.20`, `0.25`, `0.30`;
- select only if a single global blend improves seed stability without
  dataset-specific tuning;
- keep the acceptance gate identical: no dense-hit loss, O@100/O@256 not
  regressed, Recall/CUB not regressed, support not regressed.
