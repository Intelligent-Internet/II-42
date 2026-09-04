# ii42 M1149 Boundary Atom Target Native Replay Report

## Purpose

M1148 showed heldout recovery of M1147 target atoms.  M1149 asks whether those
predicted atoms produce useful retrieval movement when added to the M1129 query
and replayed through the native SQL path.

This is a bounded replay only:

- only the 149 M1147 event queries are evaluated;
- no full shared15 promotion is attempted;
- no dataset-specific tuning is used.

## Inputs

- M1148 predictions:
  `runs/m1148_boundary_atom_target_recovery_v1/target_recovery.json`
- M1149 replay:
  `runs/m1149_boundary_atom_target_native_replay_v1/native_replay.json`
- M1149 summary:
  `runs/m1149_boundary_atom_target_native_replay_v1/summary.md`
- Script:
  `scripts/replay_m1149_boundary_atom_target_native.py`

## Method

For each M1147 event query:

1. Replay the original M1129 query through the native table-backed path.
2. Add top predicted atoms from M1148 to the M1129 query vector.
3. Test fixed variants:
   - top atoms: `8`, `16`, `32`;
   - scales: `0.02`, `0.05`, `0.10`.
4. Re-run native ordering and compare metrics against the original event-query
   baseline.

## Result

Event-query baseline:

| CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| 0.981217 | 0.468327 | 0.350403 | 0.461003 | 0.656877 |

Macro deltas vs event-query baseline:

| Variant | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `top8_s0.02` | -0.000983 | -0.001361 | -0.002989 | -0.001024 | -0.003566 |
| `top8_s0.05` | -0.001830 | -0.000184 | -0.004688 | -0.003576 | -0.005228 |
| `top8_s0.10` | -0.002532 | +0.015474 | -0.008662 | -0.007104 | -0.009746 |
| `top16_s0.02` | -0.000943 | -0.003699 | -0.004166 | -0.000837 | -0.001321 |
| `top16_s0.05` | -0.001883 | +0.006851 | -0.007934 | -0.005994 | -0.002702 |
| `top16_s0.10` | -0.002638 | +0.009817 | -0.016503 | -0.012017 | -0.009125 |
| `top32_s0.02` | -0.001090 | -0.001664 | -0.004943 | -0.000867 | -0.001208 |
| `top32_s0.05` | -0.002121 | +0.008348 | -0.010859 | -0.007073 | -0.003566 |
| `top32_s0.10` | -0.003298 | +0.004619 | -0.020394 | -0.012139 | -0.011712 |

## Interpretation

M1149 is a negative result for naive additive atom replay.

The important distinction:

- M1147 was positive: a same-surface boundary atom teacher exists.
- M1148 was positive: heldout proposal features can recover part of that
  teacher.
- M1149 is negative: simply adding recovered atoms to the query vector spends
  candidate upper bound and rank quality.

The best Recall variant, `top8_s0.10`, gains `+0.015474` Recall@100 but loses:

- `-0.002532` CUB;
- `-0.008662` MAP@100;
- `-0.007104` NDCG@10;
- `-0.009746` MRR@20.

This is not promotable.  The recovered target atoms can move boundary
documents, but the update is not rank-safe.  This matches older M686/M717
lessons: atom target recovery is not enough; admission must be constrained by
native rank and head preservation.

## Decision

Do not expand M1149 to full shared15.

Keep M1147/M1148 as useful teacher/recovery evidence, but stop the naive
additive query-update path.

## Next Direction

The next valid step is a rank-constrained atom admission audit, not deeper
training of the same additive update.

M1150 should test whether predicted atoms can be admitted one at a time only
when they improve or preserve a native boundary guard:

1. Start from M1148 predicted atoms.
2. For each query, greedily test candidate atoms against a small native
   boundary proxy:
   - protected head docs must not fall;
   - top100 relevant count must not decrease on train/event oracle surface;
   - score margin against displaced non-relevant docs must improve.
3. Keep the audit oracle-labeled first; do not claim deployability.
4. If a constrained admission oracle survives native replay, only then train a
   deployable approximation.

If M1150 still spends CUB/MAP/NDCG/MRR, the M1147/M1148 target route should be
kept as evidence but not continued as the main branch.
