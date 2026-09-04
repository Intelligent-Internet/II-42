# M654X Doc Swap Audit

Status: `doc_swap_audit_complete`

This audit reloads selected compiler checkpoints and compares
P1-native top100 against generated-query top100 under frozen P1
document postings.  Dense is used only as the overlap reference.

## Runs

### `m658_shared15_swap1_nearmiss_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe, dense_overlap_256_safe, swap_dense_hit_loss_query_safe, dev_gate_passed_for_selected_checkpoint`

Split: `test`

| Rows | Swap classes | dDenseHits@100 | Lost dense docs | Gained dense docs | New nondense docs |
| ---: | --- | ---: | ---: | ---: | ---: |
| 134 | dense_hit_gain=1, dense_hit_loss=6, dense_hit_swap_flat=1, no_top100_change=125, nondense_churn_flat=1 | -0.000373134 | 7 | 2 | 7 |

| Dataset | Query | Class | Dense hit delta | Lost dense | Gained dense | New nondense |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `climate-fever` | `203` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `climate-fever` | `27` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fever` | `75311` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `nq` | `test78` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `scidocs` | `30c9a7660281ad8e4538ff9beb20282c74fac810` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `scidocs` | `c79b88a8d8ba491cead38b431703d84015153a8f` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `webis-touche2020` | `33` | `dense_hit_loss` | -1 | 1 | 0 | 1 |

## Conclusion

The observed failures are sparse doc-level boundary swaps.  The next training objective should target these exact swap pairs or constrain the adapter, rather than adding coarse overlap gates or the current protected-vs-negative hinge.
