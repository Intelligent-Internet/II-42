# M1172 Oracle Class Tail Router

M1172 tests the upper bound of a typed surface router by using the M1166
witness class as an oracle feature.  If oracle class routing cannot pass the
row-floor gate, a learned blind router should not be pursued as a runtime
surface switch.

## Inputs

- Rows: 138 M1170 selected queries.
- Decision: switch selected classes from direct_protect to tail_full.
- Denominator: full M1143/M1171 shared15 query universe, not only selected
  queries.
- Output: `runs/m1172_oracle_class_tail_router_v1/oracle_class_tail_router.json`.

## Best Policies

No row-floor-clean oracle class policy was found.

| Policy | Clean | Accepted | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `clean_relevant_entry+rank_gain_no_top100_recall+relevant_swap` | false | 99 | +0.000001 | +0.000001 | +0.000546 | +0.001340 | +0.002692 |
| `clean_relevant_entry+rank_gain_no_top100_recall+relevant_exit_only+relevant_swap` | false | 105 | +0.000001 | -0.000011 | +0.000563 | +0.001269 | +0.002692 |
| `clean_relevant_entry+relevant_swap` | false | 83 | +0.000001 | +0.000001 | +0.000430 | +0.001121 | +0.002546 |

The best policy has useful macro gains, but it still violates dataset-level
row floor:

| Metric | Min Dataset Tail-Gate Minus Direct |
| --- | ---: |
| CUB | +0.000000 |
| Recall@100 | +0.000000 |
| MAP@100 | -0.000294 |
| NDCG@10 | -0.019946 |
| MRR@20 | +0.000000 |

## Interpretation

This is a stronger stop signal than M1171.  M1171 showed current query-time
features cannot learn a clean router.  M1172 shows even an oracle class router
is not row-floor-clean.  Therefore the issue is not only weak deployable
features; direct tail routing itself has localized rank harm that must be
absorbed as a training/reference signal rather than deployed as a raw runtime
surface switch.

## Decision

Stop pursuing tail_full as a direct runtime route on selected queries.

Keep M1137 tail_full as a teacher/reference:

1. Use tail_full to identify ranking improvements and useful doc movement.
2. Do not switch the runtime surface directly unless a stronger local native
   guard is present.
3. Next diagnostic should localize which dataset/class rows create MAP/NDCG
   harm, then decide whether this can become a training loss for a unified
   surface.
