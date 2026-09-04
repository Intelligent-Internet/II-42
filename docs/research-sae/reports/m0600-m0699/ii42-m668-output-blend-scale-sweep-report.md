# M668 Output Blend Scale Sweep Report

## Summary

M668 tested whether M666's positive result can be made stable by choosing a
single global `output_blend_scale`. It keeps the first-stage scope unchanged:

- frozen P1 document postings;
- query-side compiler only;
- output active-lock;
- relative support gate;
- no BM25;
- no reranker;
- no learned gate;
- no qrels loss.

Scales tested:

- `0.15`
- `0.20`
- `0.25`
- `0.30`

Seeds tested:

- `6545`
- `6546`
- `6547`
- `6548`

Result: no single global blend is robust enough to promote. The route remains
useful, but fixed scalar damping is not sufficient.

## Full Matrix

| Scale | Seed | Pass | Selected | Rejected | Failed checks | Dev gain/lossQ | Test gain/lossQ | O@100 | O@256 | R@100 | MAP@100 | CUB | support |
| ---: | ---: | --- | --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.15 | 6545 | `false` | `e2/s34` | `false` | `dense_overlap_100_safe,swap_dense_hit_loss_query_safe` | `1/0` | `2/2` | `-0.000005051` | `+0.000030908` | `0.000000000` | `-0.000006083` | `0.000000000` | `+0.000000004` |
| 0.15 | 6546 | `false` | `e2/s34` | `false` | `dense_overlap_256_safe` | `0/0` | `0/0` | `0.000000000` | `-0.000077928` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000016` |
| 0.15 | 6547 | `true` | `e1/s17` | `false` | `` | `1/0` | `0/0` | `0.000000000` | `0.000000000` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000008` |
| 0.15 | 6548 | `false` | `e7/s119` | `true` | `dense_overlap_256_safe,swap_dense_hit_loss_query_safe,dev_gate_passed_for_selected_checkpoint` | `0/0` | `3/1` | `+0.000097222` | `-0.000011109` | `0.000000000` | `-0.000008680` | `0.000000000` | `-0.000000203` |
| 0.20 | 6545 | `false` | `e2/s34` | `false` | `dense_overlap_100_safe,dense_overlap_256_safe,swap_dense_hit_loss_query_safe` | `2/0` | `3/3` | `-0.000005051` | `-0.000023345` | `0.000000000` | `-0.000005195` | `0.000000000` | `-0.000000032` |
| 0.20 | 6546 | `false` | `e2/s34` | `false` | `dense_overlap_256_safe` | `0/0` | `0/0` | `0.000000000` | `-0.000106863` | `0.000000000` | `-0.000000674` | `0.000000000` | `-0.000000040` |
| 0.20 | 6547 | `false` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `1/0` | `0/0` | `0.000000000` | `0.000000000` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000016` |
| 0.20 | 6548 | `false` | `e7/s119` | `true` | `dense_overlap_256_safe,swap_dense_hit_loss_query_safe,dev_gate_passed_for_selected_checkpoint` | `1/0` | `3/1` | `+0.000097222` | `-0.000056485` | `0.000000000` | `-0.000007987` | `0.000000000` | `-0.000000397` |
| 0.25 | 6545 | `true` | `e1/s17` | `false` | `` | `1/0` | `1/0` | `+0.000055556` | `+0.000030908` | `0.000000000` | `-0.000004691` | `0.000000000` | `-0.000000004` |
| 0.25 | 6546 | `false` | `e2/s34` | `false` | `dense_overlap_256_safe` | `1/0` | `0/0` | `0.000000000` | `-0.000106863` | `0.000000000` | `-0.000000674` | `0.000000000` | `-0.000000052` |
| 0.25 | 6547 | `false` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `1/0` | `0/0` | `0.000000000` | `+0.000017361` | `0.000000000` | `+0.000000146` | `0.000000000` | `-0.000000020` |
| 0.25 | 6548 | `false` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `0/0` | `0/0` | `0.000000000` | `+0.000037202` | `0.000000000` | `-0.000002109` | `0.000000000` | `0.000000000` |
| 0.30 | 6545 | `false` | `e1/s17` | `false` | `dense_overlap_100_safe,swap_dense_hit_loss_query_safe` | `1/0` | `2/2` | `-0.000005051` | `+0.000030908` | `0.000000000` | `-0.000004691` | `0.000000000` | `-0.000000024` |
| 0.30 | 6546 | `false` | `e2/s34` | `true` | `dense_overlap_256_safe,dev_gate_passed_for_selected_checkpoint` | `3/1` | `0/0` | `0.000000000` | `-0.000080821` | `0.000000000` | `-0.000000674` | `0.000000000` | `-0.000000091` |
| 0.30 | 6547 | `false` | `e1/s17` | `true` | `dev_gate_passed_for_selected_checkpoint` | `1/0` | `0/0` | `0.000000000` | `+0.000049913` | `0.000000000` | `+0.000000928` | `0.000000000` | `-0.000000028` |
| 0.30 | 6548 | `false` | `e2/s34` | `false` | `dense_overlap_256_safe` | `0/0` | `1/0` | `+0.000055556` | `-0.000066725` | `0.000000000` | `-0.000001572` | `0.000000000` | `-0.000000091` |

## Aggregate

| Scale | Pass count | Rejected count | Test loss docs | Test gain docs | min O@100 | min O@256 | min R@100 | min CUB | mean MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.15 | `1/4` | `1/4` | `3` | `5` | `-0.000005051` | `-0.000077928` | `0.000000000` | `0.000000000` | `-0.000003691` |
| 0.20 | `0/4` | `2/4` | `4` | `6` | `-0.000005051` | `-0.000106863` | `0.000000000` | `0.000000000` | `-0.000003464` |
| 0.25 | `1/4` | `2/4` | `0` | `1` | `0.000000000` | `-0.000106863` | `0.000000000` | `0.000000000` | `-0.000001832` |
| 0.30 | `0/4` | `2/4` | `2` | `3` | `-0.000005051` | `-0.000080821` | `0.000000000` | `0.000000000` | `-0.000001502` |

## Interpretation

The fixed-scale sweep gives a useful stop signal:

- `0.25` is still the best tested fixed blend for exact dense-hit preservation:
  it has zero test dense-hit loss across four seeds.
- no fixed blend satisfies the complete dev/test first-stage gate across all
  seeds;
- O@256 is the recurring weak gate, especially for seed `6546`;
- lower scales can reduce movement but can still create top100 loss on some
  seeds;
- higher scales recover slightly more movement but reintroduce dense-hit loss.

This means fixed scalar damping is not enough. The first-stage route should
not be abandoned, because M666/M667/M668 repeatedly show no Recall/CUB
regression and mostly controlled dense-hit movement. But promotion requires a
more structured projection than one global scalar.

## Conclusion

Keep:

- train-high / infer-damped query compiler;
- output active-lock;
- relative support gate;
- strict no-loss selection;
- `0.25` as the current safest fixed blend diagnostic.

Do not promote:

- any fixed `output_blend_scale` from this sweep as the final first-stage
  default.

Next step:

- investigate the O@256 regression source directly, especially seed `6546`;
- constrain or project the top256 tail rather than only top100/support;
- keep BM25/reranker/qrels out of scope until O@256 is stable.
