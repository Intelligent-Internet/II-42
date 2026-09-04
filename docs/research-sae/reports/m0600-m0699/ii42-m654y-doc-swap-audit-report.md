# M654X Doc Swap Audit

Status: `doc_swap_audit_complete`

This audit reloads selected compiler checkpoints and compares
P1-native top100 against generated-query top100 under frozen P1
document postings.  Dense is used only as the overlap reference.

## Runs

### `m654u_shared15_boundary_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe`

Split: `test`

| Rows | Swap classes | dDenseHits@100 | Lost dense docs | Gained dense docs | New nondense docs |
| ---: | --- | ---: | ---: | ---: | ---: |
| 134 | dense_hit_gain=1, dense_hit_loss=2, no_top100_change=129, nondense_churn_flat=2 | -0.000074627 | 2 | 1 | 4 |

| Dataset | Query | Dense hit delta | Lost dense | New nondense |
| --- | --- | ---: | ---: | ---: |
| `climate-fever` | `204` | -1 | 1 | 1 |
| `scidocs` | `c79b88a8d8ba491cead38b431703d84015153a8f` | -1 | 1 | 1 |

### `m654y_shared15_swap_w1_seed6545`

Decision: `dense_equivalence_gate_passed`

Failed checks: `none`

Split: `test`

| Rows | Swap classes | dDenseHits@100 | Lost dense docs | Gained dense docs | New nondense docs |
| ---: | --- | ---: | ---: | ---: | ---: |
| 134 | dense_hit_gain=5, dense_hit_loss=5, dense_hit_swap_flat=3, no_top100_change=119, nondense_churn_flat=2 | 0.000000000 | 8 | 8 | 7 |

| Dataset | Query | Dense hit delta | Lost dense | New nondense |
| --- | --- | ---: | ---: | ---: |
| `climate-fever` | `203` | -1 | 1 | 1 |
| `hotpotqa` | `5ae32e125542991a06ce9946` | -1 | 1 | 1 |
| `nq` | `test78` | -1 | 1 | 1 |
| `scidocs` | `c79b88a8d8ba491cead38b431703d84015153a8f` | -1 | 1 | 1 |
| `webis-touche2020` | `5` | -1 | 1 | 1 |

## Conclusion

The observed failures are sparse doc-level boundary swaps.  The next training objective should target these exact swap pairs or constrain the adapter, rather than adding coarse overlap gates or the current protected-vs-negative hinge.
