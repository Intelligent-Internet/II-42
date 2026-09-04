# M1228 Native Rank-Context Observability

## Question

M1227 showed that current source/delta features cannot safely distinguish
M1225 query movements that help from movements that harm.  M1228 tests the
review-driven hypothesis that the missing signal is not in the atom source
features, but in native retrieval rank/score context.

This is an observability audit only.  It does not train or deploy a replay
policy.

## Method

M1228 keeps the M1225/M1226 replay labels fixed and adds only qrels-free native
features from the current query result shape:

- fused score margins around rank boundaries
- top10/top50/top100/top256 score mean/std
- P1/BM25 source coverage in top windows
- P1/BM25 raw score stats
- top100 vs tail score shape

It compares three feature groups:

- `source_only`: previous source/delta features
- `native_only`: new native rank-context features only
- `combined`: both groups

The main gate is LODO classifier AUC, not replay performance.

## Result

Full `shared15`, `1342` queries:

| Group | all_safe LODO | rank_safe LODO | cub_safe LODO | recall_safe LODO |
| --- | ---: | ---: | ---: | ---: |
| `source_only` | 0.5586 | 0.5420 | 0.4462 | 0.6299 |
| `native_only` | 0.7899 | 0.7597 | 0.6964 | 0.5920 |
| `combined` | 0.7724 | 0.7499 | 0.6355 | 0.6165 |

Best individual native features:

| Label | BestFeature | Individual AUC |
| --- | --- | ---: |
| `all_safe` | `native_top10_fused_std` | 0.7842 |
| `rank_safe` | `native_top10_fused_std` | 0.7663 |
| `cub_safe` | `native_all_fused_mean` | 0.8583 |
| `recall_safe` | `native_all_fused_mean` | 0.8586 |

## Interpretation

This is a real signal.

M1227's negative result was not that safe/harm is inherently invisible.  It was
that current source/delta features did not expose the right state.  Native
rank-context features make the safe/harm boundary visible enough to justify a
bounded trained guard or objective.

The surprising detail is that `native_only` is stronger than `combined`.
Source/delta features appear to add noise for this gate.  The next experiment
should therefore use native context as the primary guard input, not append it
blindly to the old source feature set.

## Decision

Proceed to one narrow replay validation:

- train a LODO native-context guard on `all_safe` and/or `rank_safe`
- use fixed threshold `0.5`
- no threshold sweep
- no dataset-specific tuning
- accept only if row-level failures decrease without erasing the M1225 macro
  gain

If this works, the route becomes:

1. CUB-specific atom teacher
2. native rank-context guard
3. then a real atom-level generated-posting objective

If it fails, the next change should be atom-doc overlap/fanout features or
candidate/source construction, not more query-level gate tuning.

## Artifacts

- Script: `scripts/audit_m1228_native_rank_context_observability.py`
- Smoke JSON:
  `runs/m1228_native_rank_context_observability_smoke_v1/m1228_native_rank_context_observability.json`
- Full JSON:
  `runs/m1228_native_rank_context_observability_v1/m1228_native_rank_context_observability.json`
- Query records:
  `runs/m1228_native_rank_context_observability_v1/m1228_query_records.jsonl`
- Generated markdown:
  `runs/m1228_native_rank_context_observability_v1/m1228_native_rank_context_observability.md`
