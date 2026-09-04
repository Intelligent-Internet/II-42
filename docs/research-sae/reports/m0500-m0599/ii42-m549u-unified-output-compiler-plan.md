# M549U Unified Output Compiler Plan

M549U moves the validated M549 first-stage surface from a materialized
dense-row postprocess into the model-side output compiler/head:

```text
input text -> frozen PPLX/dense root -> trainable compiler/head -> M549 posting
```

This is not the BM25-aware stage.  The first-stage target is only to reproduce
the dense-equivalent M549 posting surface:

```text
tail768_1p025625 = keep top768 coordinates identity,
                   apply gamma=1.02562527 to the remaining tail
```

## Canonical Surfaces

- Root: `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- Transform: `tail768_1p025625=tail:768:1.02562527`
- Active dims: `128`
- Support prefix: `512`
- Model-side root: official PPLX `SentenceTransformer` int8 output, no query
  prefix, preserve the model's default `max_seq_length`, then L2 normalize.
- Reject raw `AutoModel` hidden-state mean pooling as the canonical M549U root;
  M505 and M549U.1 show it is close but not close enough.
- Do not mix with M600 official1024 BEIR8 root.
- Do not use M550/BM25 in first-stage training.

## Root Version Boundary

M549U must preserve the exact M549 first-stage root contract.  The root is the
canonical broad10 PPLX/SentenceTransformer materialized surface above, and the
compiler works on the full dense vector before emitting a posting-compatible
surface.  The `tail768` name does not mean the dense root is truncated to 768
dimensions.  It means the top 768 absolute coordinates stay identity while the
remaining tail coordinates receive the calibrated monotonic power.  The active
128 view is an index/export view, not the model root.

Do not substitute these adjacent surfaces:

- M550: second-stage BM25/fusion after M549; useful control, not a first-stage
  teacher.
- M551/M590 family: materialized dense-row residual/listwise routes; useful
  evidence for training losses, not the M549U root contract.
- M600: output-head/posting-compiler smoke on an official1024 BEIR8 root; it is
  a related failure/lesson, but its root and task surface are not the M549
  broad10 teacher.
- M377-M396/M392 structural tail routes: older compressed/tail-sketch lines;
  do not use them as the M549U dense root unless a new teacher audit explicitly
  redefines the target.

The first engineering baseline is therefore:

```text
raw text
  -> official PPLX SentenceTransformer/int8 encode, default max_seq_length
  -> L2 normalize
  -> canonical M549 tail768_1p025625 compiler
  -> full support vector plus optional active128 sparse export
```

## M549U Objective

M549U is the first-stage unified encoder milestone.  It is not trying to beat
M549 with BM25 and it is not trying to learn a dataset-specific ranker.  It
tries to push the validated M549 dense-derived posting behavior back into the
model-side path so the production shape becomes one encoder surface instead of
an offline dense-row postprocess.

Promotion requires the model-side path to match M549 before any later fusion:

- preserve dense/M549 top100 membership;
- preserve active128 membership and support geometry;
- keep Recall@100 and MRR@20 from regressing;
- allow KL/MSE/listwise improvements only after the hard geometry gates pass.

## Stage Plan

1. M549U.0 teacher audit
   - Re-run the canonical M549 transform with the M549 evaluator.
   - Require `tail768_1p025625` to be selected and pass M549 gates.
   - Expected broad10 deltas versus exact dense:
     - NDCG@10 `+0.00015`
     - MAP@100 `+0.00010`
     - Recall@100 `-0.00001`
     - MRR@20 `+0.00016`
     - Dense overlap@100 `0.99525`

2. M549U.1 model-side root gate
   - Re-encode raw task texts with the official PPLX `SentenceTransformer`
     int8 path.
   - Do not use a query prefix.
   - Do not override `SentenceTransformer.max_seq_length`.  The Touche
     mismatch was caused by explicit `256`/`512` overrides; preserving the
     model default restored parity.
   - Apply the M549 tail768 transform to both official ST/int8 rows and
     materialized teacher rows.
   - Require high root cosine, support cosine, active recall, and top100
     overlap before any training.
   - Canonical broad10 sample result:
     - root doc/query cosine `0.999888` / `0.999989`
     - M549 support doc/query cosine `0.999881` / `0.999984`
     - M549 active doc/query recall `0.987552` / `0.989699`
     - M549 top100 overlap `0.997427`
   - Status: pass.

3. M549U.2 engineering unified wrapper
   - Package the deterministic first-stage route:
     `raw text -> official ST/int8 -> M549 tail768`.
   - Treat this as the first engineering M549U encoder surface.
   - Validate on M549 broad10 before adding any learned head.
   - Wrapper defaults:
     - compiler name `tail768_1p025625`;
     - `tail_identity_dims=768`;
     - `tail_gamma=1.02562527`;
     - no `max_seq_length` override.
   - Diagnostic flags may change compiler gamma/name, but such outputs are not
     canonical unless they pass full broad10 retrieval gates.
   - Status: implement as a deterministic JSONL encoder so every later
     trainable head has a stable first-stage baseline.
   - The same JSONL encoder also accepts a saved M549U compiler checkpoint for
     controlled deployment tests after a trainable compiler passes the hard
     gates.  The checkpoint path must be explicit; the default remains the
     deterministic canonical compiler.
   - Wrapper parity is now verified:
     - `m549u_encode_verify_seed5523` proves the static JSONL path emits
       support vectors exactly equal to applying the canonical M549 compiler to
       the emitted root embeddings on a FiQA docs/queries smoke;
     - `m549u_checkpoint_encode_verify_seed5524` proves the same wrapper can
       consume the canonical broad10 epoch0 compiler checkpoint and emit
       vectors numerically identical to the static path.
     - `m549u_unified_encoder_api_smoke_seed5525` refactors the JSONL encoder
       around `M549UUnifiedEncoder` and confirms static/checkpoint branch
       outputs still match exactly after the API change.
   - Status: deterministic wrapper and canonical checkpoint compiler path are
     deployable first-stage baselines.  They do not prove a learned head
     improvement; the promoted compiler checkpoint selected epoch 0 under hard
     active/support/overlap floors.

4. M549U.3 frozen-root compiler smoke
   - Start from the M546 frozen dense-root output compiler.
   - Replace the root implementation with official ST/int8 or train the head
     against precomputed official ST/int8 rows.
   - Do not continue using raw `AutoModel` mean-pool as the root.
   - First isolate the compiler with materialized canonical root rows, because
     M549U.1 already proved official ST/int8/nooverride root parity.
   - Use `teacher_surface=exact_dense`.
   - Use `target_mode=m549_tail768_signed`.
   - Freeze base model and dense head unless the head-only route is close but
     shows a fixed residual gap.
   - Run broad4/cap20k before any broad10 training.
   - Current broad4 scalar compiler smoke:
     - one trainable gamma parameter;
     - gamma `1.0 -> 1.0304387808`;
     - raw active drops only because row-int8 top-k ties are brittle;
     - tie-aware doc/query active recall remains `1.0`;
     - M549 top100 overlap improves `0.99531250 -> 0.99762500`;
     - score Pearson improves `0.99998156 -> 0.99999657`.
   - Current broad10 scalar compiler smoke:
     - one trainable gamma parameter;
     - gamma `1.0 -> 1.0345737934`;
     - tie-aware doc/query active recall remains `1.0`;
     - M549 top100 overlap improves `0.99478585 -> 0.99704419`;
     - score Pearson improves `0.99997587 -> 0.99999358`.
   - Status: pass for sampled broad10 scalar compiler smoke.
   - Full broad10 retrieval matrix:
     - canonical `tail768_1p025625` passes and remains selected;
     - learned `tail768 gamma=1.0345737934` keeps NDCG/MAP roughly equal but
       fails dense overlap@100 (`0.99382 < 0.994`);
     - do not promote learned gamma as the engineering default.
     - reusable runner: `scripts/run_m549u_full_broad10_matrix_spark.sh`.
   - Status: canonical compiler is the M549U default; learned scalar is
     diagnostic only until a constrained search passes the full overlap gate.
   - The full broad10 matrix runner can now include explicit trained compiler
     checkpoints through `COMPILER_CHECKPOINTS=name=/path/to/compiler.pt`.
     This is only a diagnostic source path; the deterministic canonical
     `tail768_1p025625` source remains the default when no checkpoint is
     provided.
   - Constrained full broad10 gamma sweep:
     - run `m549u_full_broad10_gamma_sweep_tail768_seed5507`;
     - all tested gamma values `1.020000` through `1.030000` passed the hard
       retrieval/teacher gates;
     - evaluator selected `tail768_1p025000`, with NDCG@10 `0.57277`,
       MAP@100 `0.43543`, Recall@100 `0.74808`, MRR@20 `0.65195`,
       O@100 `0.99533`, and KL relative improvement `3.77%`;
     - canonical `tail768_1p025625` still passes, with NDCG@10 `0.57278`,
       MAP@100 `0.43545`, Recall@100 `0.74802`, MRR@20 `0.65195`,
       O@100 `0.99525`, and KL relative improvement `3.10%`;
     - do not change the M549U target from canonical `1.02562527` unless a
       new M549 teacher audit intentionally redefines the teacher.
   - Status: gamma micro-search is diagnostic; M549U engineering default
     remains canonical `tail768_1p025625`.
   - The official-root compiler now supports an explicit active-margin loss
     and top-k-aware checkpoint selection:
     - `ACTIVE_MARGIN_WEIGHT` / `ACTIVE_MARGIN` can force teacher-active
       coordinates to stay above inactive coordinates during head-only fitting;
     - `SELECTION_TOP_K=100` includes M549 top100 overlap in checkpoint
       selection before score Pearson/support cosine.
     - checkpoint selection now ranks raw doc/query active recall before
       tie-aware active recall, because broad10 hard matrix showed that
       tie-aware recall alone can hide active membership degradation.
     - the runner writes a reusable `*.compiler.pt` checkpoint containing the
       compiler state dict, compiler config, summary, and training metadata;
     - the JSONL encoder runner can consume it with `COMPILER_CHECKPOINT=...`.
     - the hard matrix runner can compare it against canonical M549 with
       `COMPILER_CHECKPOINTS=name=/path/to/compiler.pt`.
     - Default margin weight remains `0.0` for backward-compatible reruns;
       new hard-fit experiments should enable it explicitly.
   - Scalar teacher-fit from non-M549 initialization:
     - `m549u_scalar_teacherfit_broad4_seed5526`,
       `m549u_scalar_teacherfit_margin_broad4_seed5527`, and
       `m549u_scalar_teacherfit_converge_broad4_seed5528` trained only the
       scalar tail-gamma parameter from `1.0` with BM25, M551, and candidate
       KL disabled;
     - trained epochs improved support cosine and M549 top100 overlap, but all
       failed the exact active floor;
     - active-margin and longer/lower-LR convergence did not fix the active
       membership swap.
   - Status: stop scalar-from-1.0 optimization.  It is informative, but not
     promotable under the current first-stage contract.  The next head
     direction must preserve active membership by construction or formalize a
     tie-safe active gate before training.
   - Active-locked compiler:
     - mode: `active_locked_tail_power`;
     - active set is computed with the same NumPy/M549-compatible target path
       as the evaluator;
     - broad4 fixed implementation run:
       `m549u_active_locked_numpy_broad4_seed5531`;
     - canonical broad10 run:
       `m549u_active_locked_numpy_broad10_seed5532`;
     - selected epoch `1`, step `738`, learned gamma `1.0256251097`;
     - selected broad10 head gate preserves doc/query active recall
       (`0.99998512` / `1.00000000`) and improves M549 top100 overlap to
       `1.00000000`;
     - full broad10 matrix source `active_locked` matches canonical
       `tail768_1p025625` on NDCG@10, MAP@100, Recall@100, MRR@20, and
       dense O@100 to report precision.
   - Status: first trained-head engineering-equivalence candidate passes
     canonical broad10 retrieval matrix.  It is not a new teacher surface:
     canonical `tail768_1p025625` remains the M549U default because it has
     stronger teacher support/KL diagnostics.

5. M549U.4 hard gate
   - Active top-k must not drop.
   - Support cosine must stay within the floor.
   - Dense overlap@100 must remain close to M549.
   - Candidate top-k membership must not regress.
   - KL/MSE are auxiliary; they cannot override active/support/overlap gates.
   - For active-locked compiler checkpoints, distinguish two outcomes:
     - retrieval-equivalent pass: full matrix matches canonical M549 retrieval
       and overlap; this is sufficient for engineering wrapper validation;
     - teacher replacement pass: teacher support/KL/active diagnostics beat or
       match canonical M549; this has not happened and is not required for the
       current M549U goal.

6. M549U.5 optional M551 auxiliary
   - Add candidate-set/listwise dense-usefulness auxiliary only if head-only is
     already close to M549.
   - Stop immediately if overlap, Recall, or MRR regress.
   - Broad4 smoke `m549u_m551_aux_broad4_seed5509` result:
     - NDCG@10 improves `+0.00434`;
     - MAP@100 drops `-0.00084`;
     - Recall@100 drops `-0.00280`;
     - MRR@20 drops `-0.00239`;
     - Dense overlap@100 drops `-0.00021`.
   - Status: stop this auxiliary branch.  It repeats the KL/listwise failure
     mode where NDCG improves while Recall/MRR/overlap regress.

7. M549U.6 optional small adapter
   - Try a last-layer adapter or small LoRA only if the frozen-head compiler has
     a stable but irreducible gap.
   - Keep dense-root semantics frozen as long as possible.

8. M549U.7 broader validation
   - Promote broad4/cap20k only after passing target and head-only gates.
   - Then run M549 broad10.
   - Strengthen the raw-text model-side gate before broader promotion:
     `m549u_broad10_root_gate_large_seed5508` uses broad10,
     `SAMPLE_DOCS=4096`, `SAMPLE_QUERIES=256`, task-salted sampling, no
     `max_seq_length` override, and runs on `spark-1`.
   - Large raw-text gate result:
     - root doc/query cosine `0.99993668` / `0.99998920`;
     - M549 doc/query support cosine `0.99993033` / `0.99998388`;
     - M549 doc/query active recall `0.98759937` / `0.98994141`;
     - dense/M549 top100 overlap `0.99733482` / `0.99702069`.
   - Status: pass for sampled broad10 raw-text model-side inference.
   - Full raw-text broad10 gate:
     - run `m549u_broad10_root_gate_full_seed5510`;
     - uses all documents and all queries with the same canonical
       `tail768_1p025625` compiler;
     - progress/partial checkpoint support is enabled in the updated runner;
     - `RESUME_PARTIAL=1` can resume from the partial JSON after validating
       root, task list, seed, sample mode, max-length, max-chars, top-k, and
       active-dim arguments;
     - not active during the M549U.18/M549U.19 closeout check.
   - Only then consider broader streaming validation.
   - Active-locked checkpoint wrapper follow-up:
     - verify `M549UUnifiedEncoder` can load
       `m549u_active_locked_numpy_broad10_seed5532.compiler.pt`;
     - compare static canonical output against active-locked checkpoint output
       on a small JSONL smoke;
     - expected result is not byte-identical support, but identical active
       membership and retrieval-equivalent top-k behavior.
     - status: pass on `m549u_active_locked_encoder_smoke_seed5533`; root max
       diff `0`, embedding max diff `9.685754776e-08`, minimum embedding
       cosine `0.9999999999999793`, active set minimum recall `1.0`.
   - Broader validation precondition:
     - run only after the checkpoint wrapper smoke is clean;
     - use streaming evaluation so official1024-scale large datasets are not
       materialized into full score matrices.
   - Official1024 BEIR8 root-expansion matrix:
     - run `m549u_active_locked_official1024_beir8_matrix_seed5534`;
     - root:
       `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`;
     - no BM25, no compressed baselines;
     - sources: exact dense, canonical `tail768_1p025625`, active-locked
       checkpoint;
     - canonical tail passes the BEIR8 root-expansion gate:
       NDCG@10 `0.48310`, MAP@100 `0.31424`, Recall@100 `0.63234`,
       MRR@20 `0.56425`, O@100 `0.99519`, KL relative improvement `2.48%`;
     - active-locked is retrieval-equivalent to canonical on the same matrix
       but has weaker KL relative improvement (`2.09%`), so it remains an
       engineering compiler candidate rather than the teacher.
   - Official1024 BEIR8 streaming validation:
     - new evaluator:
       `scripts/research_sae_m549u_streaming_broader_eval.py`;
     - run `m549u_streaming_beir8_full_seed5540` on `spark-1`;
     - root:
       `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`;
     - tasks: `arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`,
       `trec-covid`, `webis-touche2020`, `cqadupstack`;
     - no BM25, no fusion, no qrels-specific training or selection;
     - sources: exact dense, canonical `tail768_1p025625`, active-locked
       checkpoint;
     - tensorized streaming top-k is used to avoid Python heap overhead on
       large `cqadupstack` query/doc products;
     - macro: active-locked equals canonical tail at NDCG@10 `0.48314`,
       MAP@100 `0.31427`, Recall@100 `0.63234`, MRR@20 `0.56435`,
       O@100 `0.99514`;
     - status: pass.  This closes the required broader streaming validation
       surface for the official1024 BEIR8 expansion while keeping the
       canonical M549 teacher unchanged.

9. M549U.8 stage2 handoff
   - Reattach M550-style BM25 fusion only after first-stage M549U is stable.
   - Keep this as a second-stage compatibility check, not first-stage
     training.  The first-stage encoder remains BM25-free.
   - Current role split:
     - M549/M549U: first-stage canonical dense-equivalent teacher and
       engineering encoder/compiler surface;
     - M551: optional auxiliary loss family only after M549U hard gates pass;
     - M550: second-stage fixed BM25 compatibility baseline;
     - M600: paused head-only loss/gate family on a different official1024
       root, not a replacement for M549U.
   - Three-task fixed-BM25 smoke passed:
     - `m549u_active_locked` matched `m549_tail768`;
     - `m549u_active_locked_bm25_zblend` matched
       `m549_bm25_zblend_a010`.
   - Full broad10 fixed-BM25 split validation:
     - `spark-1` split `m549u_stage2_fixed_bm25_broad10_s1_seed5536`
       completed and wrote JSON;
     - `spark-2` split `m549u_stage2_fixed_bm25_broad10_s2b_seed5536`
       completed and wrote JSON;
     - do not merge the earlier failed `s2` run where the compiler checkpoint
       was missing.
   - Completion gate passed:
     - merged exactly the two valid split JSON files into
       `runs/m549u_stage2_fixed_bm25_broad10_merged_seed5536/`;
     - canonical broad10 task coverage is complete (`10/10`) with no
       duplicate units;
     - strict equivalence helper checked `100` metric/overlap comparisons with
       tolerance `0.0`;
     - `m549u_active_locked` matches `m549_tail768` before BM25;
     - `m549u_active_locked_bm25_zblend` matches
       `m549_bm25_zblend_a010` after alpha `0.10`;
     - final macro for M549U+fixed-BM25 is NDCG@10 `0.59045`,
       MAP@100 `0.44785`, Recall@100 `0.75494`, MRR@20 `0.66396`.
   - Status: pass.  M550 remains a second-stage compatibility baseline, not a
     first-stage training loss.

## Stop Conditions

- Stop if M549U.0 cannot reproduce canonical M549.
- Stop if official ST/int8 root gate does not approach materialized M549.
- Stop promotion if any canonical broad10 task has unexplained root mismatch.
- Stop if a runner/config override changes the official ST root surface
  without rebuilding the materialized M549 teacher with the same override.
- Stop head-only if active/support/overlap cannot approach M549 teacher from
  the official ST/int8 root.
- Treat raw active top-k regressions near row-int8 ties as inconclusive unless
  tie-aware active recall, support cosine, and top100 overlap agree.
- Stop M551 auxiliary if it improves KL but hurts overlap, Recall, or MRR.
- Stop adapter/LoRA if it still cannot approach teacher after a small smoke.
- Do not rescue a first-stage failure with BM25.

## Initial Commands

```bash
bash scripts/run_m549u_teacher_audit_spark.sh
```

If the audit passes:

```bash
bash scripts/run_m549u_model_side_root_gate_spark.sh
```

Only after the strict root gate passes, use the current official-root
head-only runner:

```bash
bash scripts/run_m549u_official_root_compiler_spark.sh
```

The runner now defaults to the canonical M549 broad10 task set and initializes
the scalar tail-power compiler at the canonical M549 gamma `1.02562527`.
Smoke subsets must be requested explicitly with `TASKS=...`; the post-gate
default is no longer the old broad4 smoke.

To produce deterministic M549U embeddings from a JSONL file:

```bash
INPUT=/path/to/input.jsonl \
OUTPUT=/path/to/m549u_output.jsonl \
bash scripts/run_m549u_encode_jsonl_spark.sh
```
