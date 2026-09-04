# M1238 Query-Local Target/Harm Overlap Report

M1238 audited whether the current CUB-specific target atom teacher is
query-locally contradictory.  This directly tests the common failure mode from
M1231-M1237: useful atoms exist, but qrels-free features cannot safely choose
them.

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Evaluation: leave-one-dataset-out over shared15.
- Output:
  `runs/m1238_query_local_target_harm_overlap_v1/m1238_query_local_target_harm_overlap.json`

## Full Shared15 Result

| TargetRows | SafeTargetShare | ConflictTargetShare | TargetQueries | AllSafeQueryShare | MixedQueryShare | ZeroSafeQueryShare |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2936 | 0.9854 | 0.0146 | 420 | 0.9571 | 0.0357 | 0.0071 |

Oracle selectors:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `oracle_safe_targets` | 0.9854 | 1.0000 | 0.0000 | 1.0000 | 1.278 |
| `oracle_all_targets` | 1.0000 | 1.0000 | 0.0146 | 0.9854 | 1.297 |
| `oracle_conflict_targets` | 0.0146 | 1.0000 | 1.0000 | 0.0000 | 0.019 |

## Interpretation

This is a useful positive diagnosis.  The CUB-specific target surface is not
mostly self-contradictory.  Almost all target atoms are safe within their own
query: only `1.46%` of target rows are also query-local harm atoms, and `95.7%`
of target queries have no target/harm conflict at all.

Therefore the bottleneck is not primarily label design.  The failure is that
current query-time selectors cannot identify when safe target atoms should be
added, or which safe added atoms to choose.

## Consequence

The next step should split the problem:

1. Query-level gate: identify queries that actually need safe added atoms.
2. Atom proposal/source: choose atoms only inside gated queries.

This avoids the current failure mode where every query receives atom proposals,
causing low precision even though the true target atoms are mostly safe.

M1231-M1237 should not be repeated as more atom-level threshold/classifier
variants.  The evidence now points to missing query-level routing and proposal
source design, not to a contradictory CUB teacher.
