# M654X Doc Swap Audit

Status: `doc_swap_audit_complete`

This audit reloads selected compiler checkpoints and compares
P1-native top100 against generated-query top100 under frozen P1
document postings.  Dense is used only as the overlap reference.

## Runs

### `m659_shared15_allq_swap1_top100replay1_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `cub_safe, dense_overlap_100_safe, dense_overlap_256_safe, active_support_floor, support_cosine_floor, swap_dense_hit_loss_query_safe, dev_gate_passed_for_selected_checkpoint`

Split: `test`

| Rows | Swap classes | dDenseHits@100 | Lost dense docs | Gained dense docs | New nondense docs |
| ---: | --- | ---: | ---: | ---: | ---: |
| 134 | dense_hit_gain=15, dense_hit_loss=21, dense_hit_swap_flat=8, no_top100_change=74, nondense_churn_flat=16 | -0.000447761 | 30 | 24 | 41 |

| Dataset | Query | Class | Dense hit delta | Lost dense | Gained dense | New nondense |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `test-environment-opecewiahw-con01a` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `arguana` | `test-health-hdond-con01a` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `arguana` | `test-health-hdond-pro03a` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `arguana` | `test-health-hgwhwbjfs-pro02a` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `climate-fever` | `113` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `climate-fever` | `204` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `climate-fever` | `5` | `dense_hit_loss` | -1 | 1 | 0 | 2 |
| `cqadupstack` | `android_48012` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `dbpedia-entity` | `INEX_LD-2010004` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `dbpedia-entity` | `INEX_LD-2012305` | `dense_hit_swap_flat` | 0 | 1 | 1 | 1 |
| `fever` | `202314` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fever` | `49775` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fever` | `75311` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fiqa` | `1159` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `hotpotqa` | `5a77724455429972597f153e` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `hotpotqa` | `5a77cb335542997042120b3a` | `dense_hit_loss` | -1 | 2 | 1 | 1 |
| `hotpotqa` | `5ae32e125542991a06ce9946` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `nfcorpus` | `PLAIN-23` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `nfcorpus` | `PLAIN-468` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `nq` | `test13` | `dense_hit_loss` | -1 | 1 | 0 | 1 |

## Conclusion

The observed failures are sparse doc-level boundary swaps.  The next training objective should target these exact swap pairs or constrain the adapter, rather than adding coarse overlap gates or the current protected-vs-negative hinge.
