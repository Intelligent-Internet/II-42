# M1282 Query-Prototype Source Audit

## Question

M1235 and M1238 showed that CUB-specific target atoms are mostly added atoms
and mostly query-locally safe, but ordinary atom-level selectors cannot find
them safely.  M1282 tests a different source interface:

1. Build query-level profiles from candidate atom rows.
2. Find nearest training queries under the profile space.
3. Propose atoms that were safe targets in similar training queries.
4. Intersect those atoms with the current query candidate rows.

This is a non-parametric prototype compiler smoke, not a native replay.

## Result

Hard-row smoke:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Top frontier:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `source_abs_top1` | 0.377880 | 0.337449 | 0.012346 | 0.325103 | 1.000 |
| `source_abs_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `source_abs_top8` | 0.976959 | 0.184991 | 0.006108 | 0.178883 | 4.716 |
| `proto_k20_top3` | 0.211982 | 0.183267 | 0.015936 | 0.167331 | 1.033 |
| `proto_x_source_k20_top3` | 0.216590 | 0.187251 | 0.015936 | 0.171315 | 1.033 |
| `proto_k20_top8` | 0.225806 | 0.182156 | 0.014870 | 0.167286 | 1.107 |

## Interpretation

M1282 rejects the prototype-memory interface.

The prototype source is much lower recall than `source_abs`, and its
target/harm gap is also worse than `source_abs_top3`.  This means similar-query
target transfer does not recover the missing CUB-specific atoms in a deployable
way, at least within the current action-delta candidate universe.

The failure is informative:

- the missing signal is not a simple nearest-query memory effect;
- global recurring atom identities are not enough;
- the current candidate universe remains too constrained or too mixed.

## Decision

Do not replay M1282.

Do not deepen this prototype memory shape.  It fails before native replay.

## Updated Route Control

Recent evidence now rules out several tempting next moves:

- M1280: direct scalar composition of source/model/signed signals.
- M1281: simple CUB-target minus CUB-harm two-head objective.
- M1282: nearest-query prototype target transfer.

Together with M1236/M1239/M1243/M1273, this suggests the next valid step must
change the candidate source more substantially.  The current action-delta
candidate rows are useful for diagnosis, but not sufficient as the deployable
compiler interface.

The next research branch should audit candidate source construction outside
the existing action-delta rows, or return to the model/output compiler layer
with a stronger generated target that is not limited to the current action
candidate universe.

## Artifacts

- Script: `scripts/audit_m1282_query_prototype_source.py`
- JSON: `runs/m1282_query_prototype_source_smoke_v1/m1282_query_prototype_source.json`
- Markdown: `runs/m1282_query_prototype_source_smoke_v1/m1282_query_prototype_source.md`
