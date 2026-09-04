# ii42 M1148 Boundary Atom Target Recovery Report

## Purpose

M1147 showed that M1143 recall-gain boundary events can be translated into
same-surface M1129 atom targets.  M1148 tests whether those targets are
recoverable from inference-time proposal features under leave-one-dataset-out
validation.

This is still not a retrieval result.  It is a target-recovery gate before
native replay.

## Inputs

- Teacher rows:
  `runs/m1147_single_surface_boundary_atom_teacher_v1/teacher_rows.json`
- M1148 output:
  `runs/m1148_boundary_atom_target_recovery_v1/target_recovery.json`
- M1148 summary:
  `runs/m1148_boundary_atom_target_recovery_v1/summary.md`
- Script:
  `scripts/train_m1148_boundary_atom_target_recovery.py`

## Method

For each M1147 event query:

1. Re-query the frozen M1129 native stream.
2. Use top 500 base proposal documents as the atom proposal surface.
3. Build atom features from inference-time signals only:
   - whether the atom is already in the query;
   - query atom weight;
   - support counts in top20/top100/top500;
   - impact sums and max impact;
   - rank-discounted support;
   - fused/P1/BM25 weighted support.
4. Use M1147 `target_atoms_top32` as labels, with `target_k=8`.
5. Train an SGD logistic classifier with leave-one-dataset-out validation.
6. Measure target atom recall at K.

## Result

Macro heldout target recovery:

| K | Recall |
| ---: | ---: |
| 8 | 0.241515 |
| 16 | 0.319599 |
| 32 | 0.425264 |
| 64 | 0.547036 |

Dataset breakdown:

| Dataset | Queries | R@8 | R@16 | R@32 | R@64 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 1 | 0.600000 | 0.800000 | 0.800000 | 0.800000 |
| `climate-fever` | 6 | 0.208333 | 0.333333 | 0.520833 | 0.562500 |
| `cqadupstack` | 8 | 0.192708 | 0.239583 | 0.348958 | 0.411458 |
| `dbpedia-entity` | 27 | 0.296296 | 0.412037 | 0.532407 | 0.666667 |
| `fiqa` | 7 | 0.202381 | 0.244048 | 0.327381 | 0.398810 |
| `msmarco` | 16 | 0.515625 | 0.625000 | 0.718750 | 0.773438 |
| `nfcorpus` | 41 | 0.135976 | 0.193902 | 0.303659 | 0.456707 |
| `quora` | 1 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| `scidocs` | 18 | 0.262632 | 0.298347 | 0.369775 | 0.494444 |
| `scifact` | 1 | 0.000000 | 0.000000 | 0.125000 | 0.625000 |
| `trec-covid` | 19 | 0.223684 | 0.335526 | 0.460526 | 0.618421 |
| `webis-touche2020` | 4 | 0.093750 | 0.093750 | 0.125000 | 0.218750 |

## Interpretation

M1148 is a real, but not yet sufficient, positive signal.

It avoids the M1144/M1145 failure mode because it no longer tries to predict
whether a protected-tail action is safe.  Instead, it predicts atoms from a
single-surface boundary teacher.  Heldout recovery is non-trivial:

- top8 recovers about 24% of target atoms;
- top32 recovers about 43%;
- top64 recovers about 55%.

This is enough to justify a bounded native replay, but not enough to claim a
new compiler.  The weak rows (`quora`, `webis-touche2020`, and some small
single-query rows) also show the current proposal features are incomplete.

## Decision

Proceed to a bounded M1149 native replay on the 149 M1147 event queries only.

Do not expand to full shared15 yet.  The next question is whether recovered
target atoms create useful ranking movement when added to M1129 query atoms.

## M1149 Gate

M1149 should:

1. Use M1148 heldout predictions.
2. Add top predicted atoms at small scales to the M1129 query atom vector.
3. Replay only the M1147 event queries first.
4. Compare against the original M1129 base order for those same queries.
5. Stop if Recall/MAP gains are just boundary-pair margin artifacts and do not
   survive native scoring.

If M1149 is positive, then scale to all shared15 queries with a high-precision
event detector.  If M1149 is negative, the target-recovery signal is not enough
and the next work should improve proposal features before any larger training.
