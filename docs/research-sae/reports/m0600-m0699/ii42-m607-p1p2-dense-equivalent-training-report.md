# II-42 M607 P1.2 Dense-Equivalent Training Report

Date: 2026-07-05

## Objective

M607 tests whether the current P1 bottleneck can be fixed in the first
stage, before BM25 or a downstream scorer is reintroduced.  The target is
not qrels tuning.  The target is a P1.2 posting/score surface that preserves
frozen dense teacher membership and ranking geometry better than P1-a0125.

This report should be read together with
`docs/research-sae/reports/m0600-m0699/ii42-m606-p1-dense-surface-diagnostic-report.md`.  M606 showed that P1
candidate coverage is not collapsed, but dense top-100 documents are often
under-ranked by the P1 score surface.  M607 therefore started with bounded
dense-only canaries.

## Artifacts

All runs below used the shared3 bounded surface:

- `nfcorpus`
- `cqadupstack`
- `webis-touche2020`

The source root was:

`/home/huoju/leask/runs/ii42-m599-m551-beir15-shared15-root-v1/_shared/tasks`

Remote execution used spark-1 Docker CUDA through a temporary `/dev/shm`
workspace because spark-1 root filesystem was full.  Results were copied back
under:

`runs/m607_p1p2_dense_equivalent_remote/`

## M607-A Residual Training Canaries

These runs used the existing M551-style identity-initialized residual output
layer.  BM25 and qrels were not part of the training objective.  Qrels appear
only in the held-out report metrics.

### A1 Conservative Residual

Run:

`runs/m607_p1p2_dense_equivalent_remote/m607a_canary_shared3_conservative_seed6071/`

Macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense teacher | 0.69110 | 0.59815 | 0.75414 | 0.86124 | 1.00000 |
| dense top-256 sparse | 0.66962 | 0.57763 | 0.74755 | 0.86944 | 0.81017 |
| guarded source | 0.66715 | 0.57750 | 0.74657 | 0.87778 | 0.80917 |
| trained residual | 0.66425 | 0.56940 | 0.75054 | 0.85972 | 0.80317 |

Decision: reject as a promoted first-stage surface.  Recall moved slightly in
one direction, but dense overlap fell and NDCG/MAP worsened.

### A2 Pairwise/Top-K Guard

Run:

`runs/m607_p1p2_dense_equivalent_remote/m607a2_canary_shared3_pairwise_guard_seed6072/`

Macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense teacher | 0.71523 | 0.63317 | 0.77485 | 0.84643 | 1.00000 |
| dense top-256 sparse | 0.71140 | 0.63327 | 0.76474 | 0.85416 | 0.80333 |
| guarded source | 0.71140 | 0.63327 | 0.76474 | 0.85416 | 0.80333 |
| trained residual | 0.71608 | 0.62970 | 0.75891 | 0.86230 | 0.80033 |

Decision: reject as a first-stage dense-equivalent improvement.  NDCG and MRR
looked positive, but O@100 dropped and Recall@100 dropped.  This is exactly
the failure mode we wanted to avoid: qrels-facing movement without denser
dense-faithful geometry.

## Capacity Diagnostic

After A1/A2, the next question was whether the residual loss was weak or
whether the support itself was too narrow.  To isolate this, the next runs set
`RESIDUAL_SCALE=0.0` and only changed `ACTIVE_DIMS`.  All rows below use the
same seed and split (`SEED=6074`), so the capacity curve is directly
comparable.

Run roots:

- `runs/m607_p1p2_dense_equivalent_remote/m607_capacity256_shared3_seed6074/`
- `runs/m607_p1p2_dense_equivalent_remote/m607_capacity384_shared3_seed6074/`
- `runs/m607_p1p2_dense_equivalent_remote/m607_capacity512_shared3_seed6074/`

Macro:

| Active dims | Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| dense | exact dense teacher | 0.69695 | 0.59209 | 0.74050 | 0.87639 | 1.00000 |
| 256 | dense top-k sparse | 0.68527 | 0.58294 | 0.73871 | 0.86643 | 0.79233 |
| 384 | dense top-k sparse | 0.69116 | 0.58780 | 0.73733 | 0.87713 | 0.88233 |
| 512 | dense top-k sparse | 0.69608 | 0.59100 | 0.73875 | 0.88855 | 0.94317 |

Per-task dense overlap:

| Active dims | nfcorpus O@100 | cqadupstack O@100 | webis O@100 |
| ---: | ---: | ---: | ---: |
| 256 | 0.72150 | 0.82750 | 0.82800 |
| 384 | 0.84250 | 0.90050 | 0.90400 |
| 512 | 0.92600 | 0.94750 | 0.95600 |

An additional active-256 `row_abs` check produced the same macro surface as
active-256 `teacher_interaction` under `RESIDUAL_SCALE=0.0`:

`runs/m607_p1p2_dense_equivalent_remote/m607_rowabs256_shared3_seed6074/`

This makes the immediate diagnosis sharper: the current miss is not explained
by the existing `row_abs` versus `teacher_interaction` selector switch.  The
dominant first-stage lever is active support capacity, or a new selection rule
that can approximate the active-512 surface with fewer postings.

## M607-B / M608 Support Weighting Probes

After the capacity result, I tested whether a stronger dense-teacher support
weight can recover active-512 geometry at active-256/384 capacity without
changing the native posting format.

Code changes:

- `--support-importance-power`
- `--support-importance-clip-min`
- `--support-importance-clip-max`
- `--support-score-mode teacher_topk_uniform`

The new knobs still use frozen dense teacher signals only.  They do not use
BM25, qrels, dataset-specific thresholds, or the M605 scorer.

Run roots:

- `runs/m608_p1p3_capacity_aware_remote/m608_supportpower2_active256_shared3_seed6074/`
- `runs/m608_p1p3_capacity_aware_remote/m608_uniformtopk_active256_shared3_seed6074/`
- `runs/m608_p1p3_capacity_aware_remote/m608_uniformtopk_active384_shared3_seed6074/`

Macro:

| Active dims | Support mode | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | Gate |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 256 | baseline dense top-k sparse | 0.68527 | 0.58294 | 0.73871 | 0.86643 | 0.79233 | baseline |
| 256 | interaction power 2.0 | 0.68929 | 0.57921 | 0.73428 | 0.85246 | 0.78367 | reject |
| 256 | topK uniform | 0.68408 | 0.57822 | 0.74672 | 0.85327 | 0.79050 | reject |
| 384 | baseline dense top-k sparse | 0.69116 | 0.58780 | 0.73733 | 0.87713 | 0.88233 | baseline |
| 384 | topK uniform | 0.69220 | 0.59023 | 0.74277 | 0.87449 | 0.87883 | reject |
| 512 | baseline dense top-k sparse | 0.69608 | 0.59100 | 0.73875 | 0.88855 | 0.94317 | ceiling |

Interpretation:

- Sharpening `teacher_interaction` with power 2.0 worsened dense overlap and
  MRR.  I did not run power 4.0 because the dense gate already failed.
- `teacher_topk_uniform` improves qrels-facing Recall@100 on active-256 and
  active-384, but dense overlap still regresses versus the corresponding
  baseline.  This is exactly the failure mode the M607 gate is designed to
  catch.
- Global support weighting is therefore not enough.  It can move retrieval
  metrics, but it does not repair dense membership geometry.

Decision:

Do not promote these support-weighted checkpoints.  Do not return to native
DB/plugin benchmark from them.  Do not restart M605 from them.

The remaining valid first-stage directions are:

1. Treat active-384 or active-512 as explicit P1.2 capacity candidates and test
   them on broader dense-only surfaces.
2. Replace global support weights with a per-vector posting compiler that
   learns a local packing rule, because the global weight vector is too blunt.
3. If active-512 holds broader dense-faithfulness but active-384 does not, make
   active-512 the engineering cost point instead of continuing threshold
   micro-tuning.

## M607-C Shared15 Capacity Validation

I then expanded the valid capacity line from shared3 to the full shared15
dense-only surface.  This is still first-stage evaluation: no BM25, no qrels in
training, no learned gate, and no native reranker.

Run roots:

- `runs/m608_p1p3_capacity_aware_remote/m608_capacity384_shared15_seed6075/`
- `runs/m608_p1p3_capacity_aware_remote/m608_capacity512_shared15_seed6075/`

Macro:

| Active dims | Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | Decision |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| dense | exact dense teacher | 0.77290 | 0.68609 | 0.86018 | 0.87150 | 1.00000 | teacher |
| 384 | dense top-k sparse | 0.77210 | 0.69019 | 0.85652 | 0.87160 | 0.89353 | cost candidate, below gate |
| 512 | dense top-k sparse | 0.77760 | 0.68949 | 0.85698 | 0.87662 | 0.94500 | P1.2 capacity candidate |

Active-512 per-task dense overlap:

| Dataset | O@100 |
| --- | ---: |
| arguana | 0.95100 |
| climate-fever | 0.95650 |
| cqadupstack | 0.95050 |
| dbpedia-entity | 0.94150 |
| fever | 0.94700 |
| fiqa | 0.95400 |
| hotpotqa | 0.93350 |
| msmarco | 0.97000 |
| nfcorpus | 0.92400 |
| nq | 0.93650 |
| quora | 0.94550 |
| scidocs | 0.93900 |
| scifact | 0.93200 |
| trec-covid | 0.94000 |
| webis-touche2020 | 0.95400 |

Interpretation:

- The active-384 surface is useful but not dense-equivalent enough.  It misses
  the 0.91 O@100 gate and should remain a cost candidate, not the new default.
- Active-512 validates the capacity hypothesis on all 15 shared datasets.  Its
  NDCG@10, MAP@100, and MRR@20 are slightly above the exact dense teacher on
  this bounded surface, while Recall@100 is only slightly lower.
- This is the first M607 branch that justifies returning to the engineering
  benchmark.  The next step is to build/replay native P1.2 active-512 atoms and
  rerun the native DB/plugin matrix plus M604 scorer-gap audit.

Decision:

Promote active-512 to the next P1.2 capacity candidate for native benchmark
replay.  Do not promote the residual/checkpoint variants; the promoted surface
is the auditable dense top-k sparse surface at active 512.

## M607-D Native Replay Bridge

The native replay path now has an explicit P1.2 dense-root atom bridge.  This
keeps the existing M603 database/plugin lifecycle and changes only the
first-stage posting surface.

Code changes:

- `scripts/compile_m603_p1_atoms_from_root_jsonl.py` now supports
  `--support-mode root_identity`.  This mode emits signed-coordinate product
  atoms directly from the normalized dense root vector and rejects compiler
  checkpoints, so P1.2 active512 cannot be accidentally mixed with the M549U
  compiler surface.
- `scripts/run_m608_p1p2_root_identity_atoms_spark.sh` batch-compiles
  `documents.p1_atoms.jsonl` and `queries.p1_atoms.jsonl` with
  `PRODUCT_ATOM_ACTIVE_DIMS=512` by default.  The output filenames match the
  existing M603 native matrix runner.
- `scripts/run_m603_p1_native_surface_matrix.py` now accepts
  `--source-prefix` and `--zero-source-name`, allowing the same native matrix
  path to label active512 runs as `P1.2`, `P1.2-a010`, and `P1.2-a0125`
  instead of conflating them with frozen P1-a010/a0125.
- The native matrix runner now forwards `--query-ids-file` into
  `evaluate_p1_native_atoms_pg.py`; the missing field was an existing native
  path interface bug exposed by the smoke tests.

Validation:

- `python3 -m py_compile` passed for the changed Python scripts and tests.
- `bash -n` passed for the new P1.2 atom runner and existing M607/M603
  runners.
- Targeted tests passed:
  `tests/test_compile_m603_p1_atoms_from_root_jsonl.py`,
  `tests/test_run_m608_p1p2_root_identity_atoms_spark.py`,
  `tests/test_run_m603_p1_native_surface_matrix_labels.py`, and
  `tests/test_publish_p1_atoms_to_ii42_pg.py`.

Next engineering command shape:

```bash
SOURCE_ROOT=/path/to/dense-root-jsonl \
ATOM_ROOT=/path/to/p1p2-active512-atoms \
DATASETS=nfcorpus,scifact,... \
bash scripts/run_m608_p1p2_root_identity_atoms_spark.sh

python3 scripts/run_m603_p1_native_surface_matrix.py \
  --surface p1p2_active512_shared15 \
  --source-prefix P1.2 \
  --zero-source-name P1.2 \
  --doc-atom-root /path/to/p1p2-active512-atoms \
  --semantic-backend postings \
  --ensure-atom-postings-table \
  --alphas 0,0.10,0.125
```

Decision:

The first-stage dense gate has justified native replay.  The next step is no
longer more residual training; it is to generate active512 root-identity atom
surfaces and run the native DB/plugin benchmark.  Only after that matrix should
M604 scorer-gap audit be repeated.

## Interpretation

The strongest M607 signal is not residual training.  It is support capacity.

With the same data split, increasing active dims from 256 to 384 improved
O@100 by about 9.0 points.  Increasing from 384 to 512 added another 6.1
points.  At 512, the sparse surface is already very close to the exact dense
teacher on NDCG@10, MAP@100, and MRR@20 on this bounded shared3 surface.

This means the current P1 first-stage distortion is largely caused by a too
narrow active support / posting budget.  The residual MLP can move qrels-facing
metrics, but in A1/A2 it did not improve dense-equivalent overlap and should
not be promoted.

## Decision

Do not proceed to M604/M605 or native DB evaluation from A1/A2 residual
checkpoints.

Promote the capacity finding as the next M607 direction:

1. Keep P1-a0125 / active-256 as frozen baseline.
2. Treat active-512 as the current P1.2 capacity candidate.
3. Keep active-384 as a lower-cost candidate only; it did not pass the shared15
   dense-overlap gate.
4. Do not add BM25 or qrels-aware scorer yet.
5. Return to native DB/plugin benchmark and M604 scorer-gap audit with the
   active-512 surface before restarting M605.

## Next Step

M607-B should be a capacity-aware support-selection line, not another residual
reranker:

- Primary gate: O@100 and dense rank/order metrics versus exact dense teacher.
- Secondary gate: NDCG@10, MAP@100, Recall@100, MRR@20 must not materially
  regress versus active-256.
- Candidate settings: active 512 as the dense-faithful capacity candidate;
  active 384 remains a cost candidate only.
- Training target: choose/weight support coordinates so active-384 approaches
  active-512 overlap, while keeping the native posting format auditable.

Active-512 held on shared15, and the native atom bridge is now implemented.
The next required step is to build the P1.2 active512 atom root and run the
native benchmark.  Further active-384 compression work should wait until the
active-512 native replay confirms that the dense-only gain survives the
engineering path.

## M607-E Native Shared15 Replay

The active-512 dense-root atom surface was replayed through the native
PostgreSQL/plugin path on the local shared15 schema.

Artifacts:

- Atom root:
  `runs/m608_p1p2_root_identity_atoms_shared15_v1/`
- Atom gate:
  `runs/m608_p1p2_root_identity_atoms_shared15_v1/m608_p1p2_shared15_atom_gate.json`
- Native matrix JSON:
  `runs/m608_p1p2_native_shared15_v1/m603_p1_native_p1p2_active512_shared15_matrix.json`
- Native matrix Markdown:
  `runs/m608_p1p2_native_shared15_v1/m603_p1_native_p1p2_active512_shared15_matrix.md`

The atom gate passed for all 15 shared datasets, with row counts matching the
shared15 root manifest.

Native shared15 macro:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.2 | 0.94003 | 0.80235 | 0.77406 | 0.69253 | 0.85016 | 0.87240 |
| P1.2-a010 | 0.94180 | 0.79784 | 0.77748 | 0.69676 | 0.85327 | 0.86968 |
| P1.2-a0125 | 0.94206 | 0.79425 | 0.77869 | 0.69805 | 0.85385 | 0.87068 |

Reference macro from the previous native shared15 matrix:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1-a0125 | 0.93731 | 0.41668 | 0.74494 | 0.66274 | 0.84349 | 0.83975 |
| M549U | 0.94084 | 0.40435 | 0.73078 | 0.64769 | 0.83595 | 0.82700 |
| BM25 | 0.90617 |  | 0.67006 | 0.59398 | 0.78372 | 0.78811 |
| dense | 0.86900 |  | 0.72854 | 0.64042 | 0.77817 | 0.83541 |

Delta versus old P1-a0125:

| Metric | Delta |
| --- | ---: |
| CUB | +0.00475 |
| O@100 | +0.37758 |
| NDCG@10 | +0.03375 |
| MAP@100 | +0.03531 |
| R@100 | +0.01037 |
| MRR@20 | +0.03094 |

Interpretation:

- The active-512 P1.2 gain survives the native DB/plugin path.  This is now a
  real engineering-path improvement, not just an offline scan result.
- P1.2-a0125 is the best fixed-alpha row on this surface and is the correct
  first-stage replay candidate for the next audit.
- Native O@100 is still lower than the dense-only active-512 sparse-dot gate
  (`0.79425` native versus `0.94500` dense-only).  This means the native
  signed-atom scoring interface is still not perfectly equivalent to the
  offline dense-topK sparse dot.  However, the native ranking metrics improved
  strongly, so this mismatch is not a stop condition.

Decision:

Promote P1.2 active512 as the current first-stage candidate and run M604
scorer-gap audit against P1.2-a0125.  Do not return to residual training until
the scorer-gap evidence is reviewed.

## M607-F P1.2 M604 Audit Readout

The M604 scorer-gap audit was repeated for all 15 shared datasets using the
P1.2 active512 native postings tables and fixed alpha 0.125.

Artifact root:

`runs/m608_p1p2_m604_scorer_gap_shared15_v1/`

Weighted by qrels positives:

| Category | Rate |
| --- | ---: |
| top100 hit | 0.34188 |
| candidate present but under-ranked | 0.48724 |
| candidate miss | 0.17088 |

Macro over datasets:

| Category | Rate |
| --- | ---: |
| top100 hit | 0.80592 |
| candidate present but under-ranked | 0.16369 |
| candidate miss | 0.03039 |

Decision:

P1.2 fixed the largest first-stage deficit, and the remaining gap is now
auditable as scorer/ranking recovery.  Since under-ranked positives exceed
candidate misses on the weighted view, a bounded M605-style global scorer is
eligible again.  It should remain a second-stage experiment and must use this
P1.2-a0125 native candidate pool as the frozen baseline.

## M607-G M605.2 Stop Signal

I then ran the bounded M605.2 scorer replay on the P1.2 active512 M604 exports.
The purpose was only to check whether the newly stronger candidate pool makes
the old scorer family viable again.

Eval-split deltas against the fixed P1.2 baseline:

| Probe | dCUB | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| free logistic | +0.000000 | -0.004108 | -0.004698 | -0.023301 | -0.007156 | reject |
| free pairwise tail | +0.000000 | -0.760783 | -0.690746 | -0.861086 | -0.840451 | reject |
| top50 logistic alpha 0.10 | +0.000000 | +0.000000 | -0.000206 | -0.011460 | +0.000000 | reject |
| top50 GBDT d1/e16 alpha 0.10 | +0.000000 | +0.000000 | +0.000398 | -0.000916 | +0.000000 | reject |

This closes the current M605.2 feature family.  P1.2 exposed a better native
candidate pool, but a small global scorer over the current M604 features still
does not safely recover the remaining tail.  The next iteration should return
to the first-stage score interface rather than expand scorer grids.

Updated recommendation:

1. Keep P1.2 active512 as the strongest current first-stage candidate.
2. Keep P1.2-a0125 as the fixed native baseline.
3. Stop the current M605.2 scorer family.
4. Start an M608/P1.3 score-interface recovery pass: explain and reduce the gap
   between offline active512 O@100 (`0.94500`) and native P1.2 O@100
   (`0.79425`) before trying another reranker.

## M607-H M608/P1.3 Signed-Dot Query Interface

M608 found that the P1.2 native scorer was not reproducing the offline signed
sparse dot.  The database summation itself matched local atom JSONL scores, so
the issue was the score interface:

- P1.2 documents stored signed coordinates as non-negative sign-specific atoms.
- P1.2 queries emitted only same-sign positive atoms.
- This kept same-sign positive contributions but dropped opposite-sign negative
  contributions.
- The offline dense-equivalent gate used the full signed sparse dot.

P1.3 keeps document postings unchanged and changes query atom emission to a
dual-sign signed-dot interface.  Query atoms can now carry negative weights,
while document atom impacts remain non-negative.

Artifacts:

- Report:
  `docs/research-sae/reports/m0600-m0699/ii42-m608-p1p3-native-score-interface-report.md`
- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1/`
- Native matrix JSON:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_shared15_matrix.json`
- Native matrix Markdown:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_shared15_matrix.md`
- Aligned dense ranking root:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1/`
- Aligned-reference native matrix JSON:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.json`
- Aligned-reference native matrix Markdown:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.md`

Native shared15 macro, using the aligned dense reference generated from the
same P1.3 atom/root embeddings:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.3 | 0.93972 | 0.94374 | 0.77401 | 0.69393 | 0.85178 | 0.87117 |
| P1.3-a010 | 0.94166 | 0.92843 | 0.77881 | 0.70001 | 0.85426 | 0.87182 |
| P1.3-a0125 | 0.94264 | 0.92054 | 0.77908 | 0.70037 | 0.85420 | 0.87044 |

Aligned-reference correction:

| Source | Old O@100 | Aligned O@100 | dO@100 | qrels metric delta |
| --- | ---: | ---: | ---: | ---: |
| P1.3 | 0.85501 | 0.94374 | +0.08873 | 0.00000 |
| P1.3-a010 | 0.84244 | 0.92843 | +0.08598 | 0.00000 |
| P1.3-a0125 | 0.83593 | 0.92054 | +0.08461 | 0.00000 |

The old O@100 values compared against a different dense-ranking root and
should not be used as the dense-equivalence gate.  The aligned matrix shows
that alpha-zero P1.3 is close to the offline active-512 dense-only gate.

Delta versus P1.2, qrels metrics only.  P1.2 O@100 needs its own aligned
reference before an overlap delta is meaningful.

| Comparison | dCUB | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3 - P1.2 | -0.00031 | -0.00006 | +0.00140 | +0.00162 | -0.00124 |
| P1.3-a010 - P1.2-a010 | -0.00014 | +0.00133 | +0.00325 | +0.00099 | +0.00214 |
| P1.3-a0125 - P1.2-a0125 | +0.00058 | +0.00039 | +0.00232 | +0.00035 | -0.00024 |

Decision:

P1.3 is a confirmed native-path improvement.  `P1.3-a010` is the best current
fixed-alpha candidate because it improves all qrels-facing macro metrics
versus `P1.2-a010`, while avoiding the small MRR regression seen at alpha
0.125.  The aligned dense reference also shows that alpha-zero P1.3 reaches
near-offline dense-equivalence overlap.

## M607-I P1.3 M604 Audit Readout

The M604 scorer-gap audit was repeated for all 15 shared datasets using the
P1.3 signed-dot query native postings tables and fixed alpha 0.10.

Artifact:

`docs/research-sae/reports/m0600-m0699/ii42-m604-p1p3-native-scorer-gap-audit-report.md`

Weighted by qrels positives:

| Category | P1.2-a0125 | P1.3-a010 | Delta |
| --- | ---: | ---: | ---: |
| top100 hit | 0.34188 | 0.34266 | +0.00078 |
| candidate present but under-ranked | 0.48724 | 0.48835 | +0.00111 |
| candidate miss | 0.17088 | 0.16899 | -0.00189 |

Macro over datasets:

| Category | P1.2-a0125 | P1.3-a010 | Delta |
| --- | ---: | ---: | ---: |
| top100 hit | 0.80592 | 0.80663 | +0.00071 |
| candidate present but under-ranked | 0.16369 | 0.16357 | -0.00012 |
| candidate miss | 0.03039 | 0.02980 | -0.00059 |

Interpretation:

P1.3 improves the native benchmark and reduces candidate misses slightly, but
it does not materially reduce under-ranked positives.  This means the signed
dot interface was a necessary first-stage fix, not the full bottleneck
solution.

Updated recommendation:

1. Keep `P1.3-a010` as the current first-stage benchmark candidate.
2. Do not rerun the same M605.2 feature family as a larger grid.
3. Treat the first-stage dense-equivalence blocker as largely resolved on
   shared15 after aligned-reference correction.
4. The next line should be M610: a guarded scorer or score-calibration probe,
   evaluated against the P1.3-a010 native matrix and M604 gap audit.
5. Acceptance for M610 must include both native metric preservation and a real
   reduction in weighted under-ranked positives.

## M607-J M610 Guarded Admission Probe

M610-A tested a conservative alternative to the failed M605 scorer family.  It
keeps P1.3-a010 frozen and only probes lower-top100 admission policies over
native M604 candidate rows.

Artifact:

`docs/research-sae/reports/m0600-m0699/ii42-m610-p1p3-guarded-admission-report.md`

The aggressive slot probe was rejected.  Its selected config improved
query-average Recall@100 but regressed MAP@100 and increased weighted
under-ranked positives.

The bottom-slot probe found a tiny eval-positive candidate:

`slot_admission_top98_k100_p1_b1_f1`

| Split | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dUnder-ranked |
| --- | ---: | ---: | ---: | ---: | ---: |
| eval | +0.000000 | +0.000040 | +0.000836 | +0.000000 | -0.000281 |
| all | +0.000000 | +0.000056 | +0.000132 | +0.000000 | -0.000554 |

Decision:

Do not promote M610-A.  The only safe signal is extremely conservative
bottom-slot admission and the effect size is too small.  The next useful step
is not a larger slow rank-fusion grid; it is either a faster precomputed M610
evaluator plus score-separability diagnostic, or a return to first-stage score
calibration if under-ranked positives are not separable from bottom-top100
negatives.

## M607-K M611 Score-Separability Decision

M611 tested whether the remaining P1.3-a010 under-ranked positives are
recoverable by another global/native scorer.  The diagnostic compared
candidate-present positives outside top100 against non-relevant documents near
the top100 boundary, within the same query.

Artifact:

`docs/research-sae/reports/m0600-m0699/ii42-m611-p1p3-score-separability-report.md`

Key result:

| Feature | Global AUC | Query-pair AUC | Query macro AUC |
| --- | ---: | ---: | ---: |
| bm25_rr | 0.44132 | 0.51699 | 0.45799 |
| source_count | 0.42814 | 0.44507 | 0.45346 |
| bm25_score | 0.71001 | 0.41923 | 0.43186 |
| p1_score | 0.85716 | 0.00702 | 0.01736 |
| fused_score | 0.10077 | 0.00000 | 0.00000 |

The important failure mode is query-local.  `p1_score` has high global AUC
because easier queries and harder queries occupy different score ranges, but
within the same query it is almost perfectly inverted for the relevant
positive-vs-bottom-negative comparison.  This explains why M610-A could only
recover a tiny bottom-slot signal even though its oracle ceiling was larger.

Bottom-slot oracle room remains real:

| Preserve top-k | Candidate recall | Oracle dRecall micro | Oracle dRecall query mean |
| ---: | ---: | ---: | ---: |
| 95 | 0.83101 | 0.01940 | 0.05485 |
| 98 | 0.83101 | 0.01152 | 0.04548 |
| 99 | 0.83101 | 0.00692 | 0.03251 |

Decision:

Do not continue M605/M610-style scorer tuning as the main path.  The current
native features cannot safely identify under-ranked positives inside a query.
The next meaningful work is M612: P1.4 query-local score calibration.

M612 should remain a first-stage dense-equivalent line:

1. Keep BM25 and qrels out of the training objective.
2. Use frozen dense teacher rankings/scores.
3. Optimize same-query score ordering and margins around top100/top1000.
4. Preserve active-set/support gates before any native benchmark promotion.
5. Return to M604/M611 after each training candidate to verify that the
   under-ranked-positive bottleneck actually shrinks.
