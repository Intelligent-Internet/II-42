# M654X Doc Swap Audit

Status: `doc_swap_audit_complete`

This audit reloads selected compiler checkpoints and compares
P1-native top100 against generated-query top100 under frozen P1
document postings.  Dense is used only as the overlap reference.

## Runs

### `m659_shared15_allq_swap1_top100replay1_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `cub_safe, dense_overlap_100_safe, dense_overlap_256_safe, active_support_floor, support_cosine_floor, swap_dense_hit_loss_query_safe, dev_gate_passed_for_selected_checkpoint`

Split: `dev`

| Rows | Swap classes | dDenseHits@100 | Lost dense docs | Gained dense docs | New nondense docs |
| ---: | --- | ---: | ---: | ---: | ---: |
| 134 | dense_hit_gain=12, dense_hit_loss=18, dense_hit_swap_flat=10, no_top100_change=78, nondense_churn_flat=16 | -0.000447761 | 30 | 24 | 38 |

| Dataset | Query | Class | Dense hit delta | Lost dense | Gained dense | New nondense |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `test-environment-opecewiahw-con02a` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `arguana` | `test-health-hpehwadvoee-con05a` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `climate-fever` | `198` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `climate-fever` | `98` | `dense_hit_gain` | 1 | 1 | 2 | 0 |
| `cqadupstack` | `android_33728` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `dbpedia-entity` | `INEX_LD-2012315` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fever` | `211022` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `fever` | `78516` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `fiqa` | `3830` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `fiqa` | `8102` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `hotpotqa` | `5a7166395542994082a3e814` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `hotpotqa` | `5a8e0a005542995085b373a1` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `hotpotqa` | `5aba5d2e55429901930fa799` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `msmarco` | `87181` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `nfcorpus` | `PLAIN-1050` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `nfcorpus` | `PLAIN-731` | `dense_hit_loss` | -1 | 1 | 0 | 2 |
| `nq` | `test30` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `nq` | `test31` | `dense_hit_loss` | -1 | 1 | 0 | 1 |
| `nq` | `test44` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |
| `quora` | `1331` | `dense_hit_swap_flat` | 0 | 1 | 1 | 0 |

## Conclusion

The observed failures are sparse doc-level boundary swaps.  The next training objective should target these exact swap pairs or constrain the adapter, rather than adding coarse overlap gates or the current protected-vs-negative hinge.
