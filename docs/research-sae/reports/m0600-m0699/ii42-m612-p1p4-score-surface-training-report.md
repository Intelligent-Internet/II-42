# II-42 M612 P1.4 Score-Surface Training Report

Date: 2026-07-06

## Objective

M612 starts the next first-stage recovery loop after M611.

The target is not a BM25-aware reranker and not a qrels-trained scorer.  The
target is a P1.4 query-local dense-equivalent score surface that keeps the
P1.3 active-512 signed-dot query interface, while improving same-query dense
ordering and margin around the top100 boundary.

## Current Evidence

Frozen baseline:

- `P1.3-a010` remains the current native benchmark candidate.
- `M605` remains a rejected regression benchmark, not a promoted component.
- `M610-A` remains a weak-positive bottom-slot diagnostic, not a promoted
  scorer.

M611 decision:

`return_to_score_calibration_low_separability`

Key M611 evidence:

| Feature | Global AUC | Query-pair AUC | Query macro AUC |
| --- | ---: | ---: | ---: |
| bm25_rr | 0.44132 | 0.51699 | 0.45799 |
| bm25_score | 0.71001 | 0.41923 | 0.43186 |
| p1_score | 0.85716 | 0.00702 | 0.01736 |
| fused_score | 0.10077 | 0.00000 | 0.00000 |

Interpretation:

The problem is query-local score geometry.  P1 scores can separate easy and
hard queries globally, but within the same query they rank many
candidate-present positives below bottom-top100 negatives.  A larger global
scorer grid would be searching over weak or inverted signals.

## M612 Training Shape

M612 should use the existing M607 dense-teacher framework, but with a stronger
query-local order objective.

Default canary:

- Tasks: `nfcorpus,cqadupstack,webis-touche2020`
- Variant: `query_adaptive_locked_coordinate_gain`
- Active dims: `512`
- Teacher pool: `1000`
- Primary objective: listwise dense KL plus stronger pairwise dense order
- Gates: active recall `1.0`, no teacher top-k/top1 drop, support loss floor

Rationale:

- Query-only adaptive gain keeps document postings stable.
- It modifies the query-side score surface, which matches the M608 signed-dot
  query interface fix.
- It is still first-stage dense distillation; BM25/qrels remain outside the
  training objective.

## Engineering Changes

Added:

- `scripts/run_m612_p1p4_query_local_score_calibration_spark.sh`

Extended:

- `scripts/research_sae_m549u_encode_jsonl.py`
  - `load_trained_compiler(..., projection_side=...)` can now load
    DreamLite-style M607/M612 checkpoints.
  - The loader can infer dense dims from checkpoint state when the checkpoint
    config does not contain `dense_dims`.
- `scripts/compile_m603_p1_atoms_from_root_jsonl.py`
  - Added `--compiler-projection-side doc|query`.
- `scripts/run_m608_p1p2_root_identity_atoms_spark.sh`
  - Preserves root-identity defaults.
  - Allows `SUPPORT_MODE=trained_checkpoint`.
  - Allows document/query split-specific compiler checkpoints and projection
    sides.

Why this matters:

M612 must be publishable to native atoms.  Without DreamLite checkpoint
projection support, a successful M612 training run would remain an offline
artifact and could not enter the DB/plugin benchmark path.

## M612-A Tiny Runner Smoke

Artifact:

`runs/m612_p1p4_smoke_v1/m612_smoke_tiny_remote_artifacts.tgz`

Extracted contents:

- `m612_smoke_tiny/m612_smoke_tiny_nfcorpus_seed6121.json`
- `m612_smoke_tiny/m612_smoke_tiny_nfcorpus_seed6121.md`
- `m612_smoke_tiny/m612_smoke_tiny_nfcorpus_seed6121.log`
- `m612_smoke_tiny/checkpoints/nfcorpus/m612_query_adaptive_locked_coordinate_gain.pt`

Smoke config:

- Dataset: `nfcorpus`
- Docs: `80`
- Queries: `4`
- Active dims: `16`
- Top-k / teacher pool: `20`
- Epochs: `1`

Result:

The smoke completed successfully through Docker on spark-1 and produced
JSON, Markdown, log, and checkpoint artifacts.  This validates the M612 runner
and checkpoint-output path only.  It is intentionally too small for quality
interpretation and must not be used as evidence for or against the model.

Engineering issue found and fixed:

The Docker runner previously wrote JSON/Markdown/checkpoints to a container
path that was not mounted when `OUTPUT_ROOT` lived outside `/home` or the
workspace.  The runner now explicitly mounts `HOST_OUTPUT_ROOT` and
`HOST_CHECKPOINT_ROOT` to their in-container paths, so temporary-runtime runs
can be recovered.

Operational note:

spark-1 runtime directories under `/dev/shm` and `/run/user/1000` were not
stable across separate SSH sessions in this environment.  The smoke therefore
used tar streaming in a single SSH session and streamed the output artifacts
back to the local `runs/` directory.  Use the same pattern for future
spark-1 runs unless the root filesystem is cleaned.

## M612-B/C/D Nfcorpus Bounded Canaries

These are first meaningful M612 canaries.  They use the full `nfcorpus`
document/query rows from the shared root, but keep training bounded:

- Active dims: `512`
- Teacher pool: `1000`
- Max train groups: `256`
- Epochs: `1-2`
- BM25/qrels are not used in the training objective.

Artifacts:

- B pairwise:
  `runs/m612_p1p4_nfcorpus_canary_v1/m612_nfcorpus_canary_remote_artifacts.tgz`
- C softgain:
  `runs/m612_p1p4_nfcorpus_canary_v2/m612_nfcorpus_softgain_remote_artifacts.tgz`
- C2 softgain, same seed as B:
  `runs/m612_p1p4_nfcorpus_canary_v3/m612_nfcorpus_softgain_seed6121_remote_artifacts.tgz`
- D conservative:
  `runs/m612_p1p4_nfcorpus_canary_v4/m612_nfcorpus_conservative_seed6121_remote_artifacts.tgz`

Same-seed comparison against the `dense_topk512_sparse` baseline:

| Run | dO@100 | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| B pairwise | -0.00300 | +0.00415 | +0.00033 | +0.00194 | -0.00139 | reject: selector overlap regression |
| C2 softgain | -0.00300 | +0.00143 | +0.00036 | +0.00194 | -0.00139 | reject: selector overlap regression |
| D conservative | -0.00200 | +0.00143 | +0.00036 | +0.00194 | -0.00139 | reject: selector overlap regression |

Interpretation:

The query-adaptive coordinate-gain family has a real ranking signal on
`nfcorpus`: Recall@100 and MAP@100 improve consistently, and B improves
NDCG@10.  However, every variant regresses dense overlap@100 and is rejected by
the qrels-free selector gate.  This does not pass the first-stage
dense-equivalence requirement.

Decision:

Do not scale this exact loss family to shared3/full.  It is behaving like a
small qrels-facing rank perturbation rather than a clean dense-equivalent score
surface improvement.  The next M612 variant should change the objective shape,
not merely tune the same gain/pairwise weights.

Recommended M612-E direction:

1. Add a stricter dense-overlap preservation term or selection rule to the
   training objective, not just the post-hoc selector.
2. Keep query-local pairwise/listwise supervision, but only accept updates that
   preserve dense top100 membership.
3. Prefer an additive score-calibration head or monotonic per-query score
   transform over free coordinate gain if overlap keeps dropping.
4. Rerun `nfcorpus` first; only expand to shared3 after O@100 is non-regressing
   and at least one qrels-facing metric remains positive.

## M612-E/F/G Dense-Overlap Preservation Canaries

I implemented the first recommended M612-E repair: a baseline sparse score
anchor.  This adds a dense-only loss term that keeps learned group scores close
to the current active-512 dense-topk sparse score surface.  It does not use
BM25 or qrels.

Code changes:

- `scripts/research_sae_m551_dream_lite_posting.py`
  - Added `--baseline-score-weight`.
  - Added `--selection-baseline-score-weight`.
  - Added `baseline_sparse_score_loss`.
- `scripts/run_m607_p1p2_dense_equivalent_spark.sh`
  - Passes the new baseline score weights, defaulting to `0.0` for backward
    compatibility.
- `scripts/run_m612_p1p4_query_local_score_calibration_spark.sh`
  - Defaults to the M612-E baseline-anchor canary shape.

Artifacts:

- E anchor 0.75:
  `runs/m612_p1p4_nfcorpus_canary_v5/m612e_nfcorpus_anchor_seed6121_remote_artifacts.tgz`
- F anchor 25:
  `runs/m612_p1p4_nfcorpus_canary_v6/m612f_nfcorpus_anchor25_seed6121_remote_artifacts.tgz`
- G shared coordinate gain + anchor 25:
  `runs/m612_p1p4_nfcorpus_canary_v7/m612g_shared_gain_anchor25_seed6121_remote_artifacts.tgz`

Same-seed comparison against `dense_topk512_sparse`:

| Run | Variant | dO@100 | selector dO@100 | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| B | query-adaptive gain | -0.00300 | -0.00500 | +0.00415 | +0.00033 | +0.00194 | -0.00139 | reject |
| E | query-adaptive gain + anchor 0.75 | -0.00300 | -0.00500 | +0.00271 | +0.00039 | +0.00194 | -0.00139 | reject |
| F | query-adaptive gain + anchor 25 | -0.00250 | -0.00650 | +0.00271 | +0.00042 | +0.00194 | -0.00139 | reject |
| G | shared gain + anchor 25 | -0.00250 | -0.00550 | +0.00143 | +0.00038 | +0.00194 | -0.00139 | reject |

Interpretation:

The baseline score anchor did not fix the dense-equivalence failure.  Increasing
the anchor from `0.75` to `25.0` did not preserve selector overlap; it made
selector O@100 slightly worse.  Switching from query-adaptive gain to shared
coordinate gain also failed the same gate.

This is enough evidence to stop the coordinate-gain score-calibration family.
The family can improve qrels-facing Recall/MAP on `nfcorpus`, but it does so by
reshaping top100 membership rather than preserving dense-equivalent score
geometry.  Expanding these variants to shared3/shared15 would be a waste of
compute and would violate the first-stage gate.

Decision:

Do not scale M612 coordinate-gain variants.  Do not publish their checkpoints
to the native DB/plugin path.  Do not restart M605 from these artifacts.

The next valid first-stage direction is not another score-gain weight.  It
should change the posting compiler shape itself:

1. Learn a per-vector local packing rule that selects/supports active
   coordinates more faithfully than global/query gain.
2. Keep the active-512 dense-topk surface as the frozen floor.
3. Preserve selector overlap@100 before considering qrels-facing gains.
4. If local packing cannot beat active-512 without overlap loss, keep active512
   as the P1 frozen first-stage baseline and move the remaining problem to a
   separately audited second-stage scorer.

## Resource Status

Checked on 2026-07-06:

| Host | Status | Decision |
| --- | --- | --- |
| spark-1 | GPU idle, but `/home` root filesystem is 100% full | Do not start a blind Docker run; use only with `/dev/shm` workspace after a controlled launch plan |
| spark-2 | Active RAEv2 training, GPU in use | Do not interrupt |
| ASA | SSH did not return within 60 seconds | Treat as unavailable for this step |

No M612 training was launched in this checkpoint because the available GPU
host had a disk-state risk, and launching without a controlled `/dev/shm`
workspace would create a noisy failure rather than useful evidence.

## Next Execution Plan

1. Stop M612 coordinate-gain expansion.
2. Start a new M613 first-stage compiler experiment only if it changes the
   support/packing mechanism, not merely score-gain weights.
3. Keep using spark-1 only through `/dev/shm` single-session tar streaming while
   `/home` is full.
4. Run `nfcorpus` first; expand only after selector overlap@100 is
   non-regressing and at least one dense-order or qrels-facing metric remains
   positive.
5. If no M613 local-packing variant passes the gate, freeze active512 as the P1
   first-stage surface and return to a separately scoped second-stage scorer.

Promotion gate:

Any successor must pass dense-equivalent selector overlap before native
DB/plugin benchmark replay.  M612 coordinate-gain artifacts do not pass.
