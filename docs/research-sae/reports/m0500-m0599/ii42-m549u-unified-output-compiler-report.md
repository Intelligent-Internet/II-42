# M549U Unified Output Compiler Report

M549U tests whether the validated M549 dense-derived posting surface can move
from a materialized dense-row postprocess into a model-side output compiler:

```text
text -> frozen PPLX/dense root -> output head/compiler -> M549 posting
```

The first stage excludes BM25, qrels ranking loss, and fusion.

## M549U.0 Teacher Lock

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_teacher_audit_seed549/
```

Verdict: pass.

The canonical transform was selected and all strict audit checks passed:

```text
tail768_1p025625=tail:768:1.02562527
```

Macro versus exact dense:

| Metric | Exact Dense | M549U Teacher | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.57263 | 0.57278 | +0.00015 |
| MAP@100 | 0.43535 | 0.43545 | +0.00010 |
| Recall@100 | 0.74803 | 0.74802 | -0.00001 |
| MRR@20 | 0.65179 | 0.65195 | +0.00016 |
| Dense overlap@100 | 1.00000 | 0.99525 | 0.99525 |
| Teacher KL relative improvement | 0.00% | 3.18% | +3.18% |

This confirms the target itself is correct and should remain the first-stage
floor.

## M549U.1 Smoke Matrix

All runs used broad4/cap20k on spark-1 with the base PPLX model frozen.

| Run | Target | Train dense head | Best | Init Support | Init Active | Init KL | Last Support | Last Active | Last KL | Gate |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `m549u_broad4_cap20k_delta_seed5491` | M549 tail | no | epoch0 | 0.99138144 | 0.93886185 | 0.13174728 | 0.99142544 | 0.93785191 | 0.12412687 | fail active floor |
| `m549u_broad4_cap20k_densehead_delta_seed5492` | M549 tail | yes | epoch0 | 0.99167481 | 0.93991947 | 0.13117781 | 0.99141097 | 0.92964745 | 0.10911320 | fail active floor |
| `m549u_root_parity_densehead_seed5493` | exact dense | yes | epoch0 | 0.99091762 | 0.93742561 | 0.13545438 | 0.99067928 | 0.92710781 | 0.11375915 | fail active floor |
| `m549u_active_guard_delta_seed5494` | M549 tail | no | epoch0 | 0.99119115 | 0.93815708 | 0.13093301 | 0.99123481 | 0.93786716 | 0.12613523 | fail active floor |

## Interpretation

The target is not the blocker.  M549U.0 proves the deterministic M549 teacher is
still valid.

The first M546-derived compiler smoke exposed the root/interface blocker.  The
live text-to-dense path used by M546 is already below the materialized M549
root:

```text
dense_cosine ~= 0.991
active_recall ~= 0.94
```

Training with the current cosine/MSE/KL losses improves KL but changes active
membership.  This happens both when only the compiler is trainable and when the
dense output head is trainable.

Adding active rank/boundary pressure reduces the active regression from roughly
0.00101 to 0.00029, but still does not satisfy the strict floor.  This is a
useful sign that the direction is right, but it is not yet a promotable first
stage.

Therefore the current loss family is not ready for deeper training.  More
steps would likely continue to improve soft distribution fit while damaging the
active set, which is exactly the failure mode the M549U gate is meant to catch.

## M549U.1 Root Interface Gate

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_model_side_root_gate_seed5495/
```

This gate replaced raw `AutoModel` mean pooling with the official
`SentenceTransformer`/PPLX int8 path identified in M505, then applied the M549
tail768 transform to both model-side rows and materialized teacher rows.

Macro result on broad4 sample, 512 docs and 64 queries per task:

| Metric | Value |
| --- | ---: |
| Root doc cosine | 0.999289 |
| Root query cosine | 0.999841 |
| Dense score Pearson | 0.999448 |
| M549 score Pearson | 0.999448 |
| M549 doc support cosine | 0.999281 |
| M549 query support cosine | 0.999835 |
| M549 doc active recall | 0.983387 |
| M549 query active recall | 0.987811 |
| Dense top100 overlap | 0.992473 |
| M549 top100 overlap | 0.991972 |

Per-task M549 top100 overlap:

| Task | Root doc cos | Root query cos | M549 doc active | M549 query active | M549 top100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 0.998821 | 0.999990 | 0.982300 | 0.989380 | 0.991406 |
| ArguAna | 0.999666 | 0.999392 | 0.984406 | 0.979370 | 0.992656 |
| SCIDOCS | 0.999591 | 0.999989 | 0.985519 | 0.991089 | 0.995625 |
| TRECCOVID | 0.999078 | 0.999992 | 0.981323 | 0.991406 | 0.988200 |

This changes the M549U conclusion: the route is not blocked at the idea level.
It was blocked by using the wrong root implementation.  The correct first-stage
root is the official ST/PPLX int8 output surface, not raw hidden-state pooling.

## Next Step

The next useful experiment is not BM25 and not longer training on M546's raw
AutoModel root.  The next stage should build M549U on the correct official
ST/int8 root:

- Keep M549U.0 teacher unchanged.
- Promote `raw text -> official ST/int8 -> M549 tail768` as M549U.1.
- Run a broader root gate on M549 broad10 with larger samples or full queries.
- Only after this passes, attach any trainable compiler/head to this root.
- Treat the M546 raw `AutoModel` compiler runs as diagnostic only.
- Keep BM25/M550 out until the first-stage root/compiler is stable.

If the official ST/int8 root gate holds on broad10, M549U can be implemented as
an engineering unified encoder wrapper first: PPLX official int8 encode plus a
deterministic M549 tail compiler.  Trainable output-head work should then be
measured against this root, not against M546's raw mean-pool root.

## M549U.1b Broad10 Root Gate

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_seed5496/
```

Config:

- tasks: canonical M549 broad10;
- sample: `1024` docs and up to `128` queries per task;
- root path: official ST/PPLX int8, no query prefix;
- transform: M549 `tail768_1p025625`.

Macro:

| Metric | Value |
| --- | ---: |
| Root doc cosine | 0.996502 |
| Root query cosine | 0.999969 |
| Dense score Pearson | 0.997620 |
| M549 score Pearson | 0.997619 |
| M549 doc support cosine | 0.996492 |
| M549 query support cosine | 0.999964 |
| M549 doc active recall | 0.973934 |
| M549 query active recall | 0.989016 |
| Dense top100 overlap | 0.982064 |
| M549 top100 overlap | 0.981815 |

Per-task root status:

| Task | Root doc cos | Root query cos | M549 doc active | M549 top100 |
| --- | ---: | ---: | ---: | ---: |
| ArguAna | 0.999635 | 0.999784 | 0.985481 | 0.993516 |
| CQADupstackGamingRetrieval | 0.999412 | 0.999989 | 0.986145 | 0.993047 |
| CQADupstackUnixRetrieval | 0.994566 | 0.999991 | 0.967857 | 0.978516 |
| ClimateFEVERHardNegatives | 0.998194 | 0.999988 | 0.975838 | 0.985313 |
| FEVERHardNegatives | 0.998963 | 0.999988 | 0.981918 | 0.988125 |
| FiQA2018 | 0.998765 | 0.999991 | 0.981453 | 0.991484 |
| HotpotQAHardNegatives | 0.999985 | 0.999988 | 0.987633 | 0.996328 |
| SCIDOCS | 0.999396 | 0.999989 | 0.983383 | 0.993281 |
| TRECCOVID | 0.999564 | 0.999992 | 0.983345 | 0.991400 |
| Touche2020Retrieval.v3 | 0.976543 | 0.999991 | 0.906288 | 0.907143 |

Interpretation:

- The official ST/int8 route is strongly validated on 9/10 broad10 tasks.
- `Touche2020Retrieval.v3` is the remaining blocker and cannot be waved away:
  document-side root parity is much weaker, while query parity remains high.
- Historical M505 broad10 sample had Touche top100 overlap around `0.97265`,
  so the drop to `0.90714` may be sample-size, max sequence length, or
  materialization-config sensitivity rather than a universal failure.

Next required diagnostic:

- Run Touche-only root gates across `max_seq_length=256` and `512` with the
  same sample size.
- If `256` recovers M505-level parity, record Touche as a per-task root config
  exception or rebuild a consistent canonical root before promoting M549U.
- If neither recovers, inspect low-cosine Touche documents and materialization
  provenance before any trainable compiler work.

## M549U.1c Touche Root Diagnosis

Touche-only gates showed that the issue was not the Touche corpus itself.  The
materialized root matches the official ST/PPLX surface only when we preserve the
model's SentenceTransformer default sequence length.  Explicitly forcing
`max_seq_length=256` or `512` changes long-document embeddings enough to break
document-side parity.

M505-style Touche sample, 512 docs and 49 queries:

| Config | Root doc cos | M549 doc active | M549 top100 | Dense Pearson |
| --- | ---: | ---: | ---: | ---: |
| `max_seq_length=256` | 0.942059 | 0.825684 | 0.860204 | 0.957090 |
| `max_seq_length=512` | 0.980291 | 0.917847 | 0.934694 | 0.987120 |
| no override | 0.999985 | 0.987488 | 0.997347 | 0.999994 |

Touche 1024-doc sample showed the same pattern:

| Config | Root doc cos | M549 doc active | M549 top100 | Dense Pearson |
| --- | ---: | ---: | ---: | ---: |
| `max_seq_length=256` | 0.939886 | 0.825340 | 0.855714 | 0.957677 |
| `max_seq_length=512` | 0.978894 | 0.910530 | 0.931224 | 0.985775 |

Decision: M549U root gates must not override `SentenceTransformer.max_seq_length`
unless the canonical root is rebuilt with the same override.

## M549U.1d Canonical Broad10 Root Gate

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_nooverride_seed5498/
```

Config:

- tasks: canonical M549 broad10;
- sample: `1024` docs and up to `128` queries per task;
- root path: official ST/PPLX int8, no query prefix;
- sequence length: no override;
- transform: M549 `tail768_1p025625`.

Macro:

| Metric | Value |
| --- | ---: |
| Root doc cosine | 0.999888 |
| Root query cosine | 0.999989 |
| Dense score Pearson | 0.999995 |
| M549 score Pearson | 0.999992 |
| M549 doc support cosine | 0.999881 |
| M549 query support cosine | 0.999984 |
| M549 doc active recall | 0.987552 |
| M549 query active recall | 0.989699 |
| Dense top100 overlap | 0.997726 |
| M549 top100 overlap | 0.997427 |

Per-task M549 top100 overlap:

| Task | Root doc cos | M549 doc active | M549 top100 |
| --- | ---: | ---: | ---: |
| ArguAna | 0.999985 | 0.987488 | 0.997188 |
| CQADupstackGamingRetrieval | 0.999986 | 0.987671 | 0.997578 |
| CQADupstackUnixRetrieval | 0.999987 | 0.989334 | 0.997813 |
| ClimateFEVERHardNegatives | 0.999984 | 0.987144 | 0.997344 |
| FEVERHardNegatives | 0.999986 | 0.987419 | 0.996797 |
| FiQA2018 | 0.999011 | 0.988213 | 0.998047 |
| HotpotQAHardNegatives | 0.999986 | 0.987633 | 0.996641 |
| SCIDOCS | 0.999983 | 0.986488 | 0.997500 |
| TRECCOVID | 0.999984 | 0.987114 | 0.997400 |
| Touche2020Retrieval.v3 | 0.999985 | 0.987015 | 0.997959 |

Decision: the model-side root interface is now validated on M549 broad10.  The
correct first-stage engineering surface is:

```text
raw text -> official PPLX SentenceTransformer int8 output
         -> L2 normalize
         -> M549 tail768_1p025625 support compiler
```

This is a deterministic M549U wrapper and should be the baseline that any
trainable output-head/compiler must match or improve.

## M549U.2 Deterministic Wrapper

Implementation:

```text
scripts/research_sae_m549u_encode_jsonl.py
scripts/run_m549u_encode_jsonl_spark.sh
```

The wrapper packages the validated M549U.1 surface as a reusable JSONL encoder:

```text
input JSONL id/text
  -> official PPLX SentenceTransformer encode
  -> preserve default max_seq_length
  -> L2 normalize
  -> tail768_1p025625
  -> output JSONL id/text/embedding
```

Defaults intentionally match the passing root gate:

- model: `perplexity-ai/pplx-embed-v1-0.6B`;
- `trust_remote_code=true`;
- no query prefix;
- `max_seq_length=0`, meaning no override;
- `max_text_chars=6000`;
- compiler: `tail768_1p025625`;
- `tail_identity_dims=768`;
- `tail_gamma=1.02562527`;
- full 1024-dimensional M549 support vector output.

Optional flags can also write the normalized root vector and top-absolute-value
active coordinate fields for index-building diagnostics.  These are auxiliary
fields only; the canonical first-stage embedding remains the full M549 support
vector.

The wrapper also accepts `--tail-gamma`, `--tail-identity-dims`, and
`--compiler-name`.  These are for controlled diagnostics such as the learned
gamma smoke.  The engineering default remains canonical `tail768_1p025625`.

This is now the engineering baseline for M549U.  The next trainable compiler
must be compared against this deterministic wrapper, not against M546's raw
`AutoModel` mean-pool root.

## M549U.3a Official-Root Compiler Smoke

Implementation:

```text
scripts/research_sae_m549u_official_root_compiler.py
scripts/run_m549u_official_root_compiler_spark.sh
```

This isolates the trainable output compiler from the text encoder.  It trains
only on frozen root rows:

```text
materialized M549 dense root rows -> compiler -> tail768_1p025625 support
```

The experiment does not use queries as training rows, qrels, BM25, or ranking
loss.  Queries are evaluation-only.

### Exact-Gamma Control

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad4_exact_gamma_tie_gate_seed5503/
```

Config:

- tasks: FiQA2018, ArguAna, SCIDOCS, TRECCOVID;
- train rows: `68674`;
- compiler: `tail_power`;
- gamma: `1.0256252289`;
- active tie tolerance: `1.0e-6`.

Macro:

| Metric | Value |
| --- | ---: |
| Doc support cosine | 0.99986894 |
| Query support cosine | 0.99999343 |
| Raw doc active recall | 0.99893856 |
| Tie-aware doc active recall | 1.00000000 |
| Raw query active recall | 0.99947937 |
| Tie-aware query active recall | 1.00000000 |
| M549 score Pearson | 0.99999762 |
| M549 top100 overlap | 0.99777891 |

The raw active recall is slightly below `1.0` even for the exact compiler shape.
This is caused by row-int8 top-k ties and tiny numerical differences between
NumPy and Torch top-k paths.  The tie-aware active metric stays at `1.0`, so
the exact compiler shape should not be rejected on raw active alone.

### Learned-Gamma Broad4 Smoke

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad4_learn_gamma_tie_seed5505/
```

Config:

- tasks: FiQA2018, ArguAna, SCIDOCS, TRECCOVID;
- train rows: `68674`;
- compiler: `tail_power`;
- trainable parameters: `1`;
- init gamma: `1.0`;
- learned gamma: `1.0304387808`.

Macro:

| Metric | Initial | Selected |
| --- | ---: | ---: |
| Doc support cosine | 0.99994141 | 0.99998777 |
| Query support cosine | 0.99993935 | 0.99998999 |
| Raw doc active recall | 0.99989605 | 0.99897671 |
| Tie-aware doc active recall | 1.00000000 | 1.00000000 |
| Raw query active recall | 1.00000000 | 0.99938599 |
| Tie-aware query active recall | 1.00000000 | 1.00000000 |
| M549 score Pearson | 0.99998156 | 0.99999657 |
| M549 top100 overlap | 0.99531250 | 0.99762500 |
| Doc support KL | 0.00819860 | 0.00797508 |
| Query support KL | 0.01200706 | 0.01168734 |

Per-task selected metrics:

| Task | Doc Cos | Query Cos | Tie Doc Active | Tie Query Active | Top100 | Pearson |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 0.99998862 | 0.99999058 | 1.00000000 | 1.00000000 | 0.99828125 | 0.99999721 |
| ArguAna | 0.99998736 | 0.99998707 | 1.00000000 | 1.00000000 | 0.99742188 | 0.99999567 |
| SCIDOCS | 0.99998736 | 0.99999112 | 1.00000000 | 1.00000000 | 0.99679688 | 0.99999512 |
| TRECCOVID | 0.99998772 | 0.99999118 | 1.00000000 | 1.00000000 | 0.99800000 | 0.99999826 |

Interpretation:

- The compiler shape is expressive enough for the M549 target.
- A single scalar gamma can be learned from document root rows and improves
  support/score geometry without breaking tie-aware active membership.
- Raw top-k active recall is too brittle around row-int8 ties to be the only
  gate; it must be paired with tie-aware active recall, support cosine, and
  top100 overlap.
- This broad4 smoke is positive, but canonical broad10 is the real promotion
  gate.

### Learned-Gamma Broad10 Smoke

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_learn_gamma_tie_seed5506/
```

Config:

- tasks: canonical M549 broad10;
- train rows: `188674`;
- compiler: `tail_power`;
- trainable parameters: `1`;
- init gamma: `1.0`;
- learned gamma: `1.0345737934`;
- best epoch: `3`.

Macro:

| Metric | Initial | Selected |
| --- | ---: | ---: |
| Doc support cosine | 0.99984326 | 0.99988514 |
| Query support cosine | 0.99993961 | 0.99998517 |
| Raw doc active recall | 0.99993286 | 0.99907608 |
| Tie-aware doc active recall | 1.00000000 | 1.00000000 |
| Raw query active recall | 0.99998779 | 0.99926510 |
| Tie-aware query active recall | 1.00000000 | 1.00000000 |
| M549 score Pearson | 0.99997587 | 0.99999358 |
| M549 top100 overlap | 0.99478585 | 0.99704419 |
| Doc support KL | 0.00942093 | 0.00913940 |
| Query support KL | 0.01195631 | 0.01160138 |

Per-task selected metrics:

| Task | Doc Cos | Query Cos | Tie Doc Active | Tie Query Active | Top100 | Pearson |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ArguAna | 0.99998194 | 0.99998212 | 1.00000000 | 1.00000000 | 0.99750000 | 0.99999367 |
| CQADupstackGamingRetrieval | 0.99998307 | 0.99998546 | 1.00000000 | 1.00000000 | 0.99640625 | 0.99999380 |
| CQADupstackUnixRetrieval | 0.99998504 | 0.99998605 | 1.00000000 | 1.00000000 | 0.99703125 | 0.99999525 |
| ClimateFEVERHardNegatives | 0.99998224 | 0.99998420 | 1.00000000 | 1.00000000 | 0.99789063 | 0.99999270 |
| FEVERHardNegatives | 0.99998307 | 0.99998385 | 1.00000000 | 1.00000000 | 0.99554688 | 0.99999081 |
| FiQA2018 | 0.99900705 | 0.99998522 | 1.00000000 | 1.00000000 | 0.99804688 | 0.99999582 |
| HotpotQAHardNegatives | 0.99998319 | 0.99998546 | 1.00000000 | 1.00000000 | 0.99617188 | 0.99999060 |
| SCIDOCS | 0.99998140 | 0.99998713 | 1.00000000 | 1.00000000 | 0.99710938 | 0.99999288 |
| TRECCOVID | 0.99998200 | 0.99998575 | 1.00000000 | 1.00000000 | 0.99780000 | 0.99999735 |
| Touche2020Retrieval.v3 | 0.99998242 | 0.99998647 | 1.00000000 | 1.00000000 | 0.99693878 | 0.99999291 |

Decision:

- M549U.3 now has a positive canonical broad10 sampled gate.
- The output compiler can learn the required dense-to-posting tail pressure
  from frozen root rows with one scalar parameter.
- This does not yet mean the final unified encoder is complete.  The remaining
  engineering step is to attach this compiler to the deterministic M549U
  wrapper/model-side path and then run a full M549 broad10 retrieval matrix.

## M549U.3b Full Broad10 Retrieval Matrix

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_full_broad10_learned_gamma_from5506/
```

This uses the original M549 full retrieval evaluator, with qrels used only for
scoring.  The transform comparison was:

```text
tail768_1p025625=tail:768:1.02562527
m549u_learned_tail768_1p034574=tail:768:1.0345737934
```

Reusable runner:

```text
scripts/run_m549u_full_broad10_matrix_spark.sh
```

Macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00000 | 0.00000 | 0.00% | baseline |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | +0.00006 | -0.00004 | 3.39% | baseline |
| `tail768_1p025625` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | +0.00015 | -0.00001 | 3.18% | pass |
| `m549u_learned_tail768_1p034574` | 0.57278 | 0.43545 | 0.74799 | 0.65193 | 0.99382 | +0.00015 | -0.00004 | 3.34% | fail overlap |

Per-task NDCG@10 / O@100 for canonical versus learned:

| Task | Exact NDCG | Canonical NDCG | Learned NDCG | Canonical O@100 | Learned O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ArguAna | 0.45573 | 0.45537 | 0.45573 | 0.99637 | 0.99512 |
| CQADupstackGamingRetrieval | 0.62516 | 0.62547 | 0.62518 | 0.99408 | 0.99231 |
| CQADupstackUnixRetrieval | 0.47787 | 0.47878 | 0.47876 | 0.99486 | 0.99353 |
| ClimateFEVERHardNegatives | 0.38527 | 0.38541 | 0.38538 | 0.99534 | 0.99405 |
| FEVERHardNegatives | 0.91027 | 0.91031 | 0.91031 | 0.99537 | 0.99402 |
| FiQA2018 | 0.51808 | 0.51834 | 0.51834 | 0.99565 | 0.99435 |
| HotpotQAHardNegatives | 0.74637 | 0.74627 | 0.74615 | 0.99483 | 0.99332 |
| SCIDOCS | 0.22582 | 0.22572 | 0.22555 | 0.99525 | 0.99383 |
| TRECCOVID | 0.75076 | 0.75015 | 0.75021 | 0.99520 | 0.99400 |
| Touche2020Retrieval.v3 | 0.63097 | 0.63197 | 0.63216 | 0.99551 | 0.99367 |

Decision:

- The canonical M549 compiler remains the M549U promotion target.
- The learned scalar gamma improves teacher KL slightly (`3.34%` vs `3.18%`)
  and preserves headline metrics, but it fails the dense-overlap gate:
  `0.99382 < 0.994`.
- Therefore the learned broad10 scalar is useful diagnostic evidence, not the
  engineering default.
- M549U should now lock the wrapper/compiler to canonical
  `tail768_1p025625` and use learned gamma only as a future constrained-search
  starting point.

## M549U.3c Constrained Gamma Sweep

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_full_broad10_gamma_sweep_tail768_seed5507/
```

This sweep tested a narrow tail-gamma band around the canonical M549 target
using the same full broad10 retrieval matrix and hard gates:

```text
tail768_1p020000=tail:768:1.02000000
tail768_1p022500=tail:768:1.02250000
tail768_1p025000=tail:768:1.02500000
tail768_1p025625=tail:768:1.02562527
tail768_1p026250=tail:768:1.02625000
tail768_1p027500=tail:768:1.02750000
tail768_1p030000=tail:768:1.03000000
```

Macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00% | baseline |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | 3.39% | baseline |
| `tail768_1p020000` | 0.57268 | 0.43538 | 0.74806 | 0.65187 | 0.99618 | 2.74% | pass |
| `tail768_1p022500` | 0.57270 | 0.43542 | 0.74808 | 0.65192 | 0.99583 | 2.45% | pass |
| `tail768_1p025000` | 0.57277 | 0.43543 | 0.74808 | 0.65195 | 0.99533 | 3.77% | pass |
| `tail768_1p025625` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | 3.10% | pass |
| `tail768_1p026250` | 0.57278 | 0.43545 | 0.74802 | 0.65194 | 0.99514 | 3.37% | pass |
| `tail768_1p027500` | 0.57277 | 0.43547 | 0.74803 | 0.65194 | 0.99492 | 2.95% | pass |
| `tail768_1p030000` | 0.57276 | 0.43546 | 0.74803 | 0.65195 | 0.99454 | 3.44% | pass |

The evaluator selected `tail768_1p025000`, mainly because it keeps slightly
more Recall@100 and has the best sampled teacher KL among this narrow sweep.
That does not redefine the M549U target.  The current goal is to push the
already-validated M549 `tail768_1p025625` surface back into the model-side
compiler, so canonical `1.02562527` remains the engineering default unless a
new M549 teacher audit explicitly changes the teacher surface.

Decision:

- The full hard gate is not sensitive to tiny gamma changes in
  `[1.020000, 1.030000]`.
- Gamma micro-tuning is now lower value than validating the raw-text
  model-side wrapper and then testing a constrained M551 auxiliary loss.
- Keep M549U default at canonical `tail768_1p025625`.
- Treat `tail768_1p025000` as diagnostic evidence that the scalar compiler has
  a broad safe basin, not as a promoted replacement target.

## M549U.3d Head-Only Hard-Fit Patch

The official-root compiler script has been updated for the next head-only
attempt.  The patch does not change the canonical M549 target or any past
result.  It adds:

- `active_margin` loss, gated by `ACTIVE_MARGIN_WEIGHT`, so teacher-active
  coordinates can be trained to stay above inactive coordinates directly;
- `SELECTION_TOP_K`, default `100`, so checkpoint selection ranks active-tie
  recall first and then M549 top100 overlap before score Pearson/support
  cosine;
- runner wiring for `ACTIVE_MARGIN_WEIGHT`, `ACTIVE_MARGIN`, and
  `SELECTION_TOP_K`;
- a reusable `*.compiler.pt` checkpoint containing the compiler state dict,
  compiler config, training summary, and selected metrics;
- JSONL encoder support for explicit trained compiler loading through
  `--compiler-checkpoint`; the standard runner exposes this as
  `COMPILER_CHECKPOINT=...` and optional `COMPILER_DEVICE=...`.

This is specifically aimed at the failure mode seen in M600 and the M551
auxiliary smoke: support/active metrics can look good while dense top-k
membership or Recall/MRR is damaged.  The next head-only run should enable a
small active-margin weight and still keep BM25, qrels-specific ranking, and
fusion disabled.

Promotion status: this closes the deployment-artifact gap for trainable
compiler experiments, but it does not promote a learned checkpoint by itself.
The deterministic M549U wrapper remains the engineering baseline until a
trained compiler passes active/support/overlap/retrieval gates.

## M549U.3e Active-Margin Compiler Smoke

Launched on `spark-1` as a CPU-only run so it does not compete with the active
GPU full raw-text root gate:

```text
m549u_active_margin_compiler_broad4_seed5511
```

Config:

- tasks: `FiQA2018, ArguAna, SCIDOCS, TRECCOVID`;
- compiler: `tail_power`;
- `ACTIVE_MARGIN_WEIGHT=0.10`;
- `ACTIVE_MARGIN=0.00001`;
- `SUPPORT_COSINE_WEIGHT=3.0`;
- `SOFT_SUPPORT_KL_WEIGHT=0.25`;
- `SELECTION_TOP_K=100`;
- `DEVICE=cpu`.

Final artifact:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_active_margin_compiler_broad4_seed5511/
    m549u_active_margin_compiler_broad4_seed5511.json
    m549u_active_margin_compiler_broad4_seed5511.compiler.pt
```

Macro summary:

| Metric | Initial | Final |
| --- | ---: | ---: |
| `doc_support_cosine` | 0.99981934 | 0.99986903 |
| `query_support_cosine` | 0.99993980 | 0.99999364 |
| `doc_active_recall` | 0.99990273 | 0.99893284 |
| `query_active_recall` | 0.99992371 | 0.99912842 |
| `doc_active_tie_recall` | 1.00000000 | 1.00000000 |
| `query_active_tie_recall` | 1.00000000 | 1.00000000 |
| `doc_support_kl` | 0.00838273 | 0.00818969 |
| `query_support_kl` | 0.01188217 | 0.01161284 |
| `score_pearson_m549` | 0.99998155 | 0.99999762 |
| `top100_overlap_m549` | 0.99543828 | 0.99801328 |

Training selected epoch `11`, with learned gamma `1.025424242` and `5380`
global steps.

Decision: this is a positive compiler-shape signal, not a promotion.  The
checkpoint improves support geometry, KL, score Pearson, and M549 top100
overlap while keeping tie-aware active recall at `1.0`.  Raw active recall
still drops, so it must not be treated as a completed first-stage encoder.  The
next use is diagnostic: compare this learned checkpoint/gamma against the full
broad10 hard retrieval matrix after the raw-text full gate completes.

Engineering follow-up: the M549 hard-matrix evaluator now accepts optional
trained compiler checkpoints through:

```text
COMPILER_CHECKPOINTS=name=/path/to/compiler.pt
```

The M549U full broad10 matrix wrapper passes this through to the evaluator,
with `COMPILER_DEVICE=cpu` by default.  This creates a strict comparison
surface for the active-margin checkpoint without changing the default
canonical `tail768_1p025625` source or promoting the checkpoint.

## M549U.3f Active-Margin Checkpoint Broad10 Matrix

Launched on `spark-2`:

```text
m549u_active_margin_ckpt_broad10_matrix_seed5512
```

Purpose:

- compare canonical `tail768_1p025625` against the active-margin trained
  checkpoint on the original M549 broad10 matrix;
- keep this as a hard diagnostic surface, not as a promotion;
- avoid interfering with the full raw-text gate on `spark-1`.

Config:

- shared root:
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`;
- transform source: `tail768_1p025625=tail:768:1.02562527`;
- checkpoint source:
  `active_margin=/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/m549u_active_margin_compiler_broad4_seed5511/m549u_active_margin_compiler_broad4_seed5511.compiler.pt`;
- `COMPILER_DEVICE=cpu`;
- `COMPRESSED_BASELINES=1`;
- `SEED=5512`.

Initial status as of `2026-07-02 13:14 EDT`: running in tmux on `spark-2`.
It has ranked `ArguAna` and entered `CQADupstackGamingRetrieval`.  The process
is CPU-only and does not use GPU.  The first start attempt failed because the
spark-2 repo lacked `research_sae_m401_structural_dense_tail_distillation.py`;
that dependency was synced and the Docker import path then reached the M549
evaluator successfully.

Completed at `2026-07-02 13:18 EDT`.

Macro comparison:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | Active Recall | Support Cos | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `tail768_1p025625` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | 0.99579811 | 0.99981490 | 1.84% | canonical |
| `active_margin` | 0.57258 | 0.43538 | 0.74808 | 0.65189 | 0.99526 | 0.99514999 | 0.99971823 | 1.05% | fail |

Decision:

- The checkpoint passes retrieval overlap (`O@100=0.99526`) and does not hurt
  Recall@100.
- It fails the teacher-fit gate because raw active recall drops by
  `0.00067902` versus exact dense, much worse than canonical
  `tail768_1p025625` (`0.00003090`).
- It also loses support cosine versus canonical.
- Therefore this checkpoint is not promoted and M551 auxiliary must not be
  added on top of it.

Training-engineering fix: update checkpoint selection to rank raw
`doc_active_recall` and `query_active_recall` before tie-aware active recall
and top100 overlap.  The previous selector could pick a checkpoint whose
tie-aware active recall stayed at `1.0` while raw active membership degraded.
The next head-only run should be an `active_guard` rerun with the same broad4
surface but corrected selection, then only if it survives, recheck on this
broad10 matrix.

## M549U.3g Active-Guard Compiler Rerun

Launched on `spark-2`:

```text
m549u_active_guard_compiler_broad4_seed5513
```

Purpose:

- rerun the head-only compiler with raw-active-first checkpoint selection;
- initialize at canonical M549 gamma `1.02562527`, not exact-dense gamma `1.0`;
- test whether a trainable head can improve or preserve support/score geometry
  without moving away from active membership;
- stay BM25-free and qrels-ranking-free.

Config:

- tasks: `FiQA2018, ArguAna, SCIDOCS, TRECCOVID`;
- `INIT_GAMMA=1.02562527`;
- `LEARNING_RATE=0.002`;
- `ACTIVE_MARGIN_WEIGHT=0.20`;
- `ACTIVE_MARGIN=0.00001`;
- `SUPPORT_COSINE_WEIGHT=3.0`;
- `SOFT_SUPPORT_KL_WEIGHT=0.25`;
- `MAX_TRAIN_STEPS=3000`;
- `DEVICE=cpu`.

Initial status as of `2026-07-02 13:23 EDT`: running in tmux on `spark-2`.
The Docker import path is healthy and the run has loaded all four broad4
tasks.

Observed at `2026-07-02 13:24 EDT`: epoch 0 baseline had
`doc_cos=0.99999101`, `doc_active=0.99903679`, and `top100=0.99789375`.
Epoch 1 was still running with `doc_active=0.99902534`, slightly below the
baseline.  Because the selector now ranks raw active first, the final checkpoint
should not repeat the previous active-margin promotion error unless a later
epoch recovers active membership.

Completed at `2026-07-02 13:27 EDT`.

Result:

- best epoch: `7`;
- learned gamma: `1.0272735357`;
- doc active: `0.99903679 -> 0.99905205`;
- query active: `0.99938782 -> 0.99940308`;
- doc support cosine: `0.99999101 -> 0.99999025`;
- query support cosine: `0.99999321 -> 0.99999256`;
- M549 top100 overlap: `0.99789375 -> 0.99783516`;
- M549 top10 overlap: `0.99793750 -> 0.99743750`.

Decision: do not recheck this checkpoint on broad10.  It fixed the previous
raw-active selection problem, but it still violates the hard gate because
support cosine and M549 top-k overlap regress.  This is not a valid M549U.1
checkpoint.

Training-engineering fix: checkpoint replacement now uses a hard selection
floor.  A trained epoch can replace the canonical initialization only if raw
doc/query active recall, doc/query support cosine, dense top-k overlap, and
M549 top-k overlap are all no worse than epoch 0 for every configured
`TOP_K_VALUES`.  Otherwise the initial canonical checkpoint remains selected.

Next run:

```text
m549u_active_guard_floor_compiler_broad4_seed5514
```

It uses the same active-guard training shape but with the hard selection floor.
If it cannot select a non-initial checkpoint, the current scalar head-only route
has no demonstrated improvement over the deterministic canonical compiler.

## M549U.3h Active-Guard Floor Compiler Rerun

Completed on `spark-2`:

```text
m549u_active_guard_floor_compiler_broad4_seed5514
```

Artifacts:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_active_guard_floor_compiler_broad4_seed5514/
    m549u_active_guard_floor_compiler_broad4_seed5514.json
    m549u_active_guard_floor_compiler_broad4_seed5514.compiler.pt
```

This rerun kept the active-guard training shape from M549U.3g, but changed
checkpoint replacement to a hard floor:

- raw doc/query active recall must not fall below epoch 0;
- doc/query support cosine must not fall below epoch 0;
- dense top-k overlap must not fall below epoch 0 for `k=10,50,100`;
- M549 top-k overlap must not fall below epoch 0 for `k=10,50,100`.

Result:

| Metric | Epoch 0 / selected | Best trained epoch observed |
| --- | ---: | ---: |
| Learned gamma | 1.0256252289 | 1.0273265839 |
| Doc active recall | 0.99895668 | 0.99894905 |
| Query active recall | 0.99937256 | 0.99941833 |
| Doc support cosine | 0.99986905 | 0.99986854 |
| Query support cosine | 0.99999331 | 0.99999288 |
| Dense O@100 | 0.99593828 | 0.99593828 |
| M549 O@100 | 0.99795938 | 0.99780938 |
| `selection_floor_ok` | true | false |

The run stopped at `MAX_TRAIN_STEPS=3000`.  All trained epochs failed the hard
selection floor, mostly because doc support cosine and M549 top100 overlap
regressed below the epoch-0 canonical initialization.  The final checkpoint
therefore correctly stayed at epoch 0.

Decision: do not launch a broad10 matrix for this checkpoint.  It is equivalent
to the deterministic canonical compiler, not a learned improvement.  This closes
the scalar tail-power head-only attempt: a one-parameter trainable compiler has
not produced a non-initial checkpoint that improves M549U while preserving
active/support/top-k floors.

## M549U.3i Delta Floor Compiler Probe

Launched and completed on `spark-2`:

```text
m549u_delta_floor_compiler_broad4_seed5515
```

Purpose:

- test a more expressive head-only compiler after the scalar tail-power route
  failed to select a non-initial checkpoint;
- keep the same BM25-free, qrels-ranking-free M549 target;
- keep the same hard selection floor before any checkpoint can replace epoch 0.

Config delta versus M549U.3h:

- `COMPILER_MODE=delta`;
- `DEVICE=cuda`;
- `DELTA_HIDDEN_DIMS=0`;
- `DELTA_SCALE=0.025`;
- `MAX_DOC_ROWS_PER_TASK=10000`;
- `MAX_TRAIN_STEPS=1500`;
- trainable parameters: `1048576`.

Result:

| Metric | Epoch 0 / selected | Trained epoch 20 |
| --- | ---: | ---: |
| Doc active recall | 0.99990463 | 0.98043919 |
| Query active recall | 1.00000000 | 0.98488647 |
| Doc tie-aware active recall | 1.00000000 | 0.94699205 |
| Query tie-aware active recall | 1.00000000 | 0.96074955 |
| Doc support cosine | 0.99981929 | 0.99981467 |
| Query support cosine | 0.99993980 | 0.99993446 |
| M549 O@100 | 0.99504531 | 0.99502578 |
| `selection_floor_ok` | true | false |

The delta head lowered the training loss but immediately changed active
membership.  Every trained epoch failed the hard floor; the selected checkpoint
therefore stayed at epoch 0.

Decision: do not continue this unrestricted residual head as a first-stage
candidate.  The result confirms the same failure mode at higher capacity:
without a structural constraint equivalent to the deterministic M549 compiler,
the head can fit support losses while breaking active geometry.  This does not
invalidate M549U; it says the engineering baseline should remain the canonical
M549 compiler until a future compiler can prove non-regression under the hard
floor.

## M549U.4 Large Raw-Text Root Gate

Run on `spark-1`:

```text
m549u_broad10_root_gate_large_seed5508
```

Config:

- broad10 tasks from the canonical M549 shared root;
- official PPLX `SentenceTransformer` root;
- no `max_seq_length` override;
- `SAMPLE_DOCS=4096`;
- `SAMPLE_QUERIES=256`;
- `SAMPLE_SEED_MODE=task_name`;
- canonical M549 `tail768_1p025625` support compiler.

This validates the actual engineering unified path:

```text
raw text -> official ST/PPLX root -> M549 compiler
```

It is intentionally still BM25-free and qrels-free.

Macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99993668 |
| `root_query_cosine` | 0.99998920 |
| `score_pearson_dense` | 0.99999466 |
| `score_pearson_m549` | 0.99999232 |
| `m549_doc_support_cosine` | 0.99993033 |
| `m549_query_support_cosine` | 0.99998388 |
| `m549_doc_active_recall` | 0.98759937 |
| `m549_query_active_recall` | 0.98994141 |
| `dense_top100_overlap` | 0.99733482 |
| `m549_top100_overlap` | 0.99702069 |

Per-task summary:

| Task | Root Doc Cos | M549 Doc Active | Dense O@100 | M549 O@100 |
| --- | ---: | ---: | ---: | ---: |
| ArguAna | 0.999985 | 0.987251 | 0.997891 | 0.997734 |
| CQADupstackGamingRetrieval | 0.999986 | 0.987717 | 0.996875 | 0.996875 |
| CQADupstackUnixRetrieval | 0.999988 | 0.989063 | 0.997539 | 0.997383 |
| ClimateFEVERHardNegatives | 0.999985 | 0.986984 | 0.997734 | 0.997266 |
| FEVERHardNegatives | 0.999986 | 0.987761 | 0.996953 | 0.996406 |
| FiQA2018 | 0.999499 | 0.987934 | 0.998086 | 0.997539 |
| HotpotQAHardNegatives | 0.999986 | 0.987997 | 0.996875 | 0.996289 |
| SCIDOCS | 0.999983 | 0.986725 | 0.997656 | 0.996172 |
| TRECCOVID | 0.999984 | 0.987558 | 0.996800 | 0.997400 |
| Touche2020Retrieval.v3 | 0.999985 | 0.987003 | 0.996939 | 0.997143 |

Decision:

- The larger raw-text gate passes and is close to the earlier 512-doc gate.
- The only visible lower point is FiQA doc root cosine `0.999499`, but its
  M549 top100 overlap remains `0.997539`, so this is not a route blocker.
- The JSONL encoder wrapper is now validated as the engineering M549U
  first-stage surface for sampled broad10 raw-text inference.
- Next work should not tune gamma further.  It should test whether a small
  M551-style candidate-set auxiliary can improve the compiler without lowering
  overlap/Recall/MRR.

## M549U.5 M551 Auxiliary Smoke

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_m551_aux_broad4_seed5509/
```

Config:

- tasks: `FiQA2018, ArguAna, SCIDOCS, TRECCOVID`;
- baseline transform: `tail768_1p025625=tail:768:1.02562527`;
- variant: `shared_locked_support_residual`;
- `RESIDUAL_SCALE=0.0125`;
- `SUPPORT_WEIGHT=2.0`;
- `MAX_SUPPORT_LOSS=0.003`;
- `MIN_ACTIVE_RECALL=0.995`;
- `TEACHER_TEMPERATURE=0.025`;
- `STUDENT_TEMPERATURE=0.050`;
- qrels used only for heldout evaluation, not for training.

Macro versus canonical sparse baseline:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tail768_1p025625_sparse` | 0.44929 | 0.22740 | 0.57551 | 0.53768 | 0.57121 |
| `m549u_m551_aux_shared_locked_support_residual_dream_lite` | 0.45363 | 0.22656 | 0.57271 | 0.53529 | 0.57100 |

Delta:

- NDCG@10: `+0.00434`
- MAP@100: `-0.00084`
- Recall@100: `-0.00280`
- MRR@20: `-0.00239`
- Dense overlap@100: `-0.00021`

Per-task deltas versus canonical sparse baseline:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | -0.00452 | -0.00278 | -0.00219 | -0.00767 | -0.00004 |
| ArguAna | +0.00261 | +0.00142 | -0.00357 | +0.00147 | +0.00073 |
| SCIDOCS | -0.00650 | -0.00144 | -0.00495 | -0.00336 | -0.00100 |
| TRECCOVID | +0.02574 | -0.00057 | -0.00051 | +0.00000 | -0.00050 |

Decision:

- This auxiliary loss is a negative result for M549U.
- It repeats the known failure mode: NDCG improves, but Recall/MRR/overlap
  regress.
- Stop this M551 auxiliary branch under the M549U stop condition.
- Do not add candidate-set KL to the first-stage compiler unless a future
  version proves non-regression under hard overlap/Recall/MRR gates.

## M549U.6 Full Raw-Text Root Gate

Launched on `spark-1`:

```text
m549u_broad10_root_gate_full_seed5510
```

Config:

- broad10 tasks from the canonical M549 shared root;
- official PPLX `SentenceTransformer` root;
- no `max_seq_length` override;
- all documents and all queries (`SAMPLE_DOCS=0`, `SAMPLE_QUERIES=0`);
- task-salted sampling mode retained for deterministic compatibility;
- canonical M549 `tail768_1p025625` support compiler;
- progress enabled every 100 encode batches;
- partial JSON enabled by the updated root-gate runner for future restarts;
- explicit partial resume support is available through `RESUME_PARTIAL=1` and
  validates the core run arguments before reusing completed task rows.

Current observed progress as of `2026-07-02 14:15 EDT`:

```text
Partial JSON written with tasks=4/10:
  ArguAna
  CQADupstackGamingRetrieval
  CQADupstackUnixRetrieval
  ClimateFEVERHardNegatives
Current task:
  FEVERHardNegatives docs=0/163698, queries=1000.
Final JSON:
  not written yet.
```

Partial macro at `4/10`:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99998604 |
| `root_query_cosine` | 0.99998808 |
| `score_pearson_dense` | 0.99999489 |
| `score_pearson_m549` | 0.99999264 |
| `m549_doc_support_cosine` | 0.99997990 |
| `m549_query_support_cosine` | 0.99998237 |
| `m549_doc_active_recall` | 0.98783253 |
| `m549_query_active_recall` | 0.98894582 |
| `dense_top100_overlap` | 0.99763300 |
| `m549_top100_overlap` | 0.99704218 |

Completed-task rows:

| Task | Docs | Queries | Root Doc Cos | M549 O@100 | Seconds |
| --- | ---: | ---: | ---: | ---: | ---: |
| ArguAna | 8674 | 1401 | 0.99998528 | 0.99742327 | 362.105 |
| CQADupstackGamingRetrieval | 45301 | 1595 | 0.99998605 | 0.99662069 | 1395.025 |
| CQADupstackUnixRetrieval | 47382 | 1072 | 0.99998772 | 0.99729478 | 6052.038 |
| ClimateFEVERHardNegatives | 47416 | 1000 | 0.99998510 | 0.99683000 | 3116.178 |

This run is the first full raw-text broad10 check of the engineering M549U
surface.  It is expected to take substantially longer than the sampled
4096-doc gate.  Promotion to broader streaming validation should wait for its
full JSON result.

Live resource note as of `2026-07-02 14:45 EDT`:

- `spark-1` is actively running the full root gate with GPU utilization around
  `96%`.  The process is still in tmux/docker/python and no error/OOM marker
  is present in the log.  Current task is `FEVERHardNegatives`, with document
  encoding at about `41600/163698`.
- `spark-2` is now running a non-overlapping tail shard with GPU utilization
  around `96%`.  Current task is `FiQA2018`, with document encoding at about
  `28800/57638`.
- The partial JSON only updates after a task finishes; the next expected
  updates are `spark-1` FEVER `5/10` or the first `spark-2` shard partial.
- ASA is not usable for this step: `ASA` resolves to `vm-asa` but SSH timed
  out, and lowercase `asa` does not resolve.
- Disk note: `spark-1` root filesystem is at `95%` with about `48G` free.
  This is not an immediate blocker for the current root-gate JSON/log outputs,
  but it should be handled before launching any larger training or cache run.
  The read-only disk audit found `/home/huoju/leask/runs` at about `522G`;
  the largest directory is `m150-beir-full-pplx` at about `256G`, followed by
  `ii42-m310b-official-roots-v1` at about `41G`.  Docker pruning is not a good
  target because the only image is active.  No data was removed.  `spark-2`
  has about `154G` free.

Current timing estimate from the observed document-encoding throughput:

- `spark-2` FiQA is likely to write the first tail partial before `spark-1`
  finishes FEVER.
- `spark-1` FEVER is still a multi-hour task.  The full broad10 root-gate
  decision remains blocked on completed task rows, not on a failure.

Parallel tail shard launched on `spark-2` at `2026-07-02 18:19 UTC`:

```text
m549u_broad10_root_gate_tail_seed5511
```

This shard uses the same canonical root-gate runner and the same no-override
M549U surface, but limits `TASKS` to the not-yet-started tail tasks:

```text
FiQA2018,HotpotQAHardNegatives,SCIDOCS,TRECCOVID,Touche2020Retrieval.v3
```

It is a non-overlapping hard-gate shard while `spark-1` is still encoding
`FEVERHardNegatives`.  It does not change the gate criteria and does not start
compiler training.  Initial status:

```text
spark-2 GPU ~= 96%
current task: FiQA2018 docs=57638 queries=648
partial JSON: not written yet
```

After `spark-1` writes the FEVER partial and `spark-2` writes the tail result,
the combined 10-task root-gate evidence can be assembled from the two shard
JSONs.  The original single-process `spark-1` run remains undisturbed for now.

## M549U.6a Root-Gate Shard Combiner

Added:

```text
scripts/research_sae_m549u_combine_root_gate_shards.py
```

Purpose:

- combine multiple non-overlapping M549U root-gate partial/final JSON files;
- validate the shared root contract before combining:
  `active_dims`, `max_chars`, `max_seq_length`, `model_name`, `sample_docs`,
  `sample_queries`, `sample_seed_mode`, `shared_root`, and `top_k_values`;
- reject duplicated tasks across shards;
- reject missing or unexpected tasks against the expected broad10 set;
- recompute the macro from per-task rows instead of trusting shard macros;
- emit a combined JSON and optional Markdown report.

Local smoke:

```text
python3 -m py_compile scripts/research_sae_m549u_combine_root_gate_shards.py
awk 'length($0) > 120 { print FILENAME ":" FNR ":" length($0) }' \
  scripts/research_sae_m549u_combine_root_gate_shards.py
```

Both passed.  A narrow 4-task smoke against the current `spark-1` partial
reproduced the shard macro.  A full broad10 expected-task smoke correctly
failed with the six missing tasks:

```text
FEVERHardNegatives
FiQA2018
HotpotQAHardNegatives
SCIDOCS
TRECCOVID
Touche2020Retrieval.v3
```

This utility is only an evidence-assembly tool.  It does not change the root
gate, compiler, or retrieval evaluator.

## M549U.2a JSONL Wrapper Smoke

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_encode_jsonl_smoke_seed5516/
```

This is an engineering smoke for the deterministic M549U JSONL wrapper, not a
new retrieval experiment.  It verifies that the deployable encoder surface can
load the official PPLX `SentenceTransformer` path inside the Spark Docker
runtime and emit the expected M549U schema.

The first attempt failed because `spark-2` had an empty root-owned
`ST_PYDEPS` directory at:

```text
/home/huoju/leask/runs/ii42-m505-model-side-compression-probe-v1/st_pydeps
```

The fix was non-destructive: copy the known-good `sentence_transformers`
dependency directory from `spark-1` into a new user-owned path and rerun the
smoke with:

```text
ST_PYDEPS=/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/st_pydeps_from_spark1
```

Validated output:

```text
output2.jsonl rows: 3
embedding dims: 1024
root_embedding dims: 1024
active_indices dims: 128
active_values dims: 128
```

Metadata checks:

| Field | Value |
| --- | --- |
| `compiler_name` | `tail768_1p025625` |
| `tail_identity_dims` | `768` |
| `tail_gamma` | `1.02562527` |
| `transform` | `tail:768:1.02562527` |
| `max_seq_length_override` | `0` |
| `root_normalized` | `true` |
| `model_name` | `perplexity-ai/pplx-embed-v1-0.6B` |

Runtime summary:

```text
encoded=3
skipped=0
elapsed_s=0.426
rows_per_s=7.047
```

After updating `scripts/run_m549u_encode_jsonl_spark.sh`, the smoke was rerun
without passing `ST_PYDEPS`.  The runner correctly ignored the empty default
directory and selected the M549U user-owned fallback dependency path.  The
second smoke wrote `output3.jsonl` with the same validated schema:

```text
rows=3
embedding dims=1024
root_embedding dims=1024
active dims=128
compiler=tail768_1p025625
transform=tail:768:1.02562527
```

Decision:

- The deterministic M549U wrapper is now smoke-validated as an engineering
  JSONL encoder on `spark-2`.
- It emits the canonical full 1024-dimensional support vector plus optional
  diagnostic root and active fields.
- The Spark runner now has a non-destructive fallback for the known
  root-owned empty dependency directory on `spark-2`.
- This does not complete M549U by itself.  The full raw-text broad10 root gate
  on `spark-1` still needs to finish before broader validation or any new
  trainable compiler work is promoted.

## M549U.2b Checkpoint-Backed Wrapper Smoke

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_encode_jsonl_smoke_seed5516/
```

Checkpoint:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_active_guard_floor_compiler_broad4_seed5514/
    m549u_active_guard_floor_compiler_broad4_seed5514.compiler.pt
```

This checkpoint is a `tail_power` compiler with one trainable scalar.  Its
hard-floor selector kept the epoch-0 canonical initialization:

```text
compiler_mode=tail_power
dense_dims=1024
tail_identity_dims=768
target_gamma=1.02562527
learned_gamma ~= 1.0256252694
best_epoch=0
```

The first checkpoint smoke loaded and ran, but exposed a real engineering
inconsistency: the static wrapper used NumPy `argpartition` while the
checkpoint module used Torch `topk` for the top768 identity set.  Root vectors
were identical and active top128 overlap was `128/128`, but support values
differed by about `0.001` because of top768 tie-boundary semantics.

Fix:

- `tail_power` checkpoints now read gamma from the checkpoint state dict and
  apply the canonical NumPy M549 support compiler;
- non-tail-power checkpoints, such as future `delta` compilers, still use the
  Torch module path.

Validated rerun:

```text
output5_ckpt_canonical.jsonl rows: 3
root max diff versus static wrapper: 0
embedding max diff versus static wrapper: 0
active top128 overlap: 128/128
compiler mode: trained_checkpoint
checkpoint compiler_mode: tail_power
tail_gamma: 1.0256252693843102
```

Decision:

- The JSONL encoder can now emit M549U through an explicit compiler checkpoint
  without changing the canonical output.
- This closes the engineering gap between deterministic wrapper mode and
  checkpoint-backed compiler mode for the promoted first-stage shape.
- It does not promote any non-initial trained checkpoint.  Previous scalar and
  delta probes still stand: trained epochs that lower KL but regress
  active/overlap remain rejected.

## M549U.2c Root-Gate Runner Fallback Smoke

The long full raw-text broad10 gate is running on `spark-1` and should not be
restarted.  Separately, the root-gate runner was updated with the same
non-destructive `ST_PYDEPS` fallback used by the JSONL wrapper.  This prevents
future `spark-2` root gates from failing on the known root-owned empty default
dependency directory.

Smoke run on `spark-2`, without passing `ST_PYDEPS`:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_root_gate_fallback_smoke_seed5517/
```

Config:

```text
task=FiQA2018
sample_docs=32
sample_queries=16
max_seq_length=0
sample_seed_mode=task_name
```

Validated result:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99998748 |
| `root_query_cosine` | 0.99998933 |
| `score_pearson_dense` | 0.99999726 |
| `score_pearson_m549` | 0.99999601 |
| `m549_doc_support_cosine` | 0.99998188 |
| `m549_query_support_cosine` | 0.99998593 |
| `m549_doc_active_recall` | 0.98925781 |
| `m549_query_active_recall` | 0.99365234 |
| `dense_top100_overlap` | 1.00000000 |
| `m549_top100_overlap` | 1.00000000 |

Decision:

- The root-gate runner fallback works on `spark-2` without manual
  `ST_PYDEPS`.
- `RESUME_PARTIAL=1` remains task-level resume only: it reuses completed task
  rows after checking core arguments and task-prefix order.  It does not
  resume inside an unfinished task, so the running full gate should continue
  undisturbed.
- This smoke is an engineering/runner validation only.  The full raw-text
  broad10 gate remains the required M549U.5 evidence.

## M549U.5 Full Raw-Text Broad10 Root Gate

The full raw-text M549 broad10 root gate is now split across `spark-1` and
`spark-2` to validate the engineering encoder path on full documents and
queries:

```text
raw text -> official PPLX SentenceTransformer int8 root
         -> L2 normalize
         -> canonical tail768_1p025625 compiler
```

This is still first-stage validation only.  BM25, qrels-specific ranking, and
fusion remain out of scope.

Shard runs:

```text
spark-1:
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_full_seed5510/

spark-2:
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_tail_seed5511/
```

The shard combiner was used to validate current partials against explicit
expected task lists and the common root contract.  This is not a completion
claim; it only proves the completed shard rows are internally compatible.

Validated partial status on 2026-07-02:

| Host | Completed tasks | Current task | Status |
| --- | ---: | --- | --- |
| `spark-1` | 4/10 | `FEVERHardNegatives` | running |
| `spark-2` | 1/5 | `HotpotQAHardNegatives` | running |

Validated `spark-1` completed tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
```

`spark-1` validated partial macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99998604 |
| `root_query_cosine` | 0.99998808 |
| `score_pearson_dense` | 0.99999489 |
| `score_pearson_m549` | 0.99999264 |
| `m549_doc_support_cosine` | 0.99997990 |
| `m549_query_support_cosine` | 0.99998237 |
| `m549_doc_active_recall` | 0.98783253 |
| `m549_query_active_recall` | 0.98894582 |
| `dense_top100_overlap` | 0.99763300 |
| `m549_top100_overlap` | 0.99704218 |

Validated `spark-2` completed tasks:

```text
FiQA2018
```

`spark-2` validated FiQA partial macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99932796 |
| `root_query_cosine` | 0.99999022 |
| `score_pearson_dense` | 0.99999677 |
| `score_pearson_m549` | 0.99999528 |
| `m549_doc_support_cosine` | 0.99932200 |
| `m549_query_support_cosine` | 0.99998492 |
| `m549_doc_active_recall` | 0.98797112 |
| `m549_query_active_recall` | 0.99037905 |
| `dense_top100_overlap` | 0.99794753 |
| `m549_top100_overlap` | 0.99763889 |

Cross-shard partial merge:

The current `spark-1` and `spark-2` partials were copied to a local temporary
directory and merged with the shard combiner using an explicit five-task
expected list.  This proves the two active shards are compatible with the same
M549U root contract.  It is still only a partial gate because the full broad10
coverage is not yet available.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FiQA2018
```

Combined 5/10 partial macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99985442 |
| `root_query_cosine` | 0.99998851 |
| `score_pearson_dense` | 0.99999527 |
| `score_pearson_m549` | 0.99999317 |
| `m549_doc_support_cosine` | 0.99984832 |
| `m549_query_support_cosine` | 0.99998288 |
| `m549_doc_active_recall` | 0.98786025 |
| `m549_query_active_recall` | 0.98923247 |
| `dense_top100_overlap` | 0.99769590 |
| `m549_top100_overlap` | 0.99716152 |

Current decision:

- Keep both shards running.  Do not restart healthy sessions.
- Do not launch trainable compiler/head promotion until the combined broad10
  root gate covers all 10 expected tasks and passes the strict combiner.
- Watch `spark-1` disk: root filesystem is about 95% used with about 48 GB
  free.  This is not a blocker for the current gate, but it should be handled
  before larger training or cache-producing work.

## M549U.6 Revision-Pinned Wrapper Plumbing

While the full broad10 raw-text gate is still running, the M549U wrapper and
root-gate scripts were hardened for reproducibility without changing default
behavior:

- `scripts/research_sae_m549u_model_side_root_gate.py` now accepts
  `--model-revision` and records it in the root-gate payload.
- `scripts/research_sae_m549u_encode_jsonl.py` now accepts `--model-revision`
  and records it in output metadata.
- `scripts/run_m549u_model_side_root_gate_spark.sh` and
  `scripts/run_m549u_encode_jsonl_spark.sh` now pass optional
  `MODEL_REVISION`.
- `scripts/research_sae_m549u_combine_root_gate_shards.py` treats
  `model_revision` as a common shard contract field while remaining compatible
  with already-running old-format partial JSON files, where the field is
  normalized to an empty string.

Validation:

- local `py_compile` passed for the updated M549U Python scripts;
- local `bash -n` passed for the updated M549U runners;
- local `git diff --check` passed for the updated M549U files;
- existing old-format `spark-1` + `spark-2` partials still combine as 5/10
  with `model_revision=''` and `m549_top100_overlap=0.99716152`;
- updated scripts were synced to both `spark-1` and `spark-2`;
- remote `py_compile` + `bash -n` passed on both Spark worktrees.
- both Spark HF caches currently map `perplexity-ai/pplx-embed-v1-0.6B`
  `main` to snapshot
  `2c4d510dd4a732063c31a0f70193e35067b51fd8`;
- both Spark dependency trees expose `SentenceTransformer.__init__(revision=...)`,
  so the new `MODEL_REVISION` plumbing is compatible with the installed
  sentence-transformers code.

Current decision:

- Do not restart current healthy root-gate tmux sessions for this plumbing
  change.
- Use `MODEL_REVISION` only for future controlled reruns or deployment wrapper
  tests.
- Recommended future controlled rerun pin:
  `MODEL_REVISION=2c4d510dd4a732063c31a0f70193e35067b51fd8`.
- Keep the runner default revision empty until the currently running old-format
  root-gate partials finish, so crash recovery can still resume those partials
  without a revision-contract mismatch.
- Do not start trainable compiler/head promotion until the combined strict
  broad10 root gate reaches 10/10 expected tasks.

## M549U.7 Runner Boundary Cleanup

The M549U plan and runner boundary were tightened to avoid accidentally
re-entering the old M546-backed head-only path:

- the plan's head-only command now points at
  `scripts/run_m549u_official_root_compiler_spark.sh`;
- `scripts/run_m549u_frozen_output_compiler_spark.sh` is marked as legacy and
  exits by default unless `ALLOW_LEGACY_M549U_FROZEN_RUNNER=1` is set.

Validation:

- `bash -n` passed for both M549U head-only runners;
- `git diff --check` passed for the updated plan and legacy runner;
- the legacy runner guard was exercised locally and exits with code `2` while
  printing the official-root runner path.

Decision:

- Future M549U head-only work must use
  `scripts/run_m549u_official_root_compiler_spark.sh`.
- Do not launch it until the full raw-text broad10 root gate has completed and
  passed strict 10/10 shard combination.

## M549U.8 First-Five Root Gate Boundary

The `spark-1` full broad10 raw-text root gate reached the first planned
boundary after completing `FEVERHardNegatives`.

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_full_seed5510/
```

Validated first-five tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
```

First-five macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99998593 |
| `root_query_cosine` | 0.99998795 |
| `score_pearson_dense` | 0.99999440 |
| `score_pearson_m549` | 0.99999194 |
| `m549_doc_support_cosine` | 0.99997977 |
| `m549_query_support_cosine` | 0.99998221 |
| `m549_doc_active_recall` | 0.98780189 |
| `m549_query_active_recall` | 0.98877853 |
| `dense_top100_overlap` | 0.99763040 |
| `m549_top100_overlap` | 0.99706375 |

The current `spark-2` tail shard still has only `FiQA2018` completed.  The
strict combiner was therefore also run over the `spark-1` first-five partial
plus the `spark-2` `FiQA2018` tail partial.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
FiQA2018
```

Combined 6/10 macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99987627 |
| `root_query_cosine` | 0.99998833 |
| `score_pearson_dense` | 0.99999479 |
| `score_pearson_m549` | 0.99999250 |
| `m549_doc_support_cosine` | 0.99987014 |
| `m549_query_support_cosine` | 0.99998266 |
| `m549_doc_active_recall` | 0.98783009 |
| `m549_query_active_recall` | 0.98904529 |
| `dense_top100_overlap` | 0.99768325 |
| `m549_top100_overlap` | 0.99715960 |

Current decision:

- Keep both Spark shards running; `spark-1` has started the duplicate tail and
  can serve as backup while `spark-2` continues the tail shard.
- Do not launch the official-root head/compiler stage yet.  The required gate
  is strict broad10 10/10 coverage, and the combined validated surface is still
  only 6/10.
- The next material boundary is either `spark-2` completing
  `HotpotQAHardNegatives`, or `spark-1` producing additional duplicate-tail
  coverage that can be used if the tail shard fails.

## M549U.9 Non-Overlapping Tail Shard Boundary

The replacement non-overlapping tail shard on `spark-1` completed `SCIDOCS`
and wrote a partial:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_nondup_tail_seed5512/
    m549u_broad10_root_gate_nondup_tail_seed5512.partial.json
```

This shard covers only:

```text
SCIDOCS
TRECCOVID
Touche2020Retrieval.v3
```

The strict combiner was run over three non-overlapping partials:

- `spark-1` first-five partial from
  `m549u_broad10_root_gate_full_seed5510`;
- `spark-2` tail partial containing `FiQA2018`;
- `spark-1` non-overlapping tail partial containing `SCIDOCS`.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
FiQA2018
SCIDOCS
```

Combined 7/10 macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99989153 |
| `root_query_cosine` | 0.99998831 |
| `score_pearson_dense` | 0.99999454 |
| `score_pearson_m549` | 0.99999220 |
| `m549_doc_support_cosine` | 0.99988530 |
| `m549_query_support_cosine` | 0.99998277 |
| `m549_doc_active_recall` | 0.98765588 |
| `m549_query_active_recall` | 0.98923859 |
| `dense_top100_overlap` | 0.99765565 |
| `m549_top100_overlap` | 0.99711680 |

New task row:

| Task | Docs | Queries | Root doc cos | Root query cos | M549 O@100 | Seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `SCIDOCS` | 25657 | 1000 | 0.99998307 | 0.99998820 | 0.99686000 | 1216.616 |

Current decision:

- The 7/10 combined root gate remains healthy and non-overlapping.
- Keep `spark-1` non-overlapping shard running through `TRECCOVID` and
  `Touche2020Retrieval.v3`.
- Keep `spark-2` tail shard running until `HotpotQAHardNegatives` writes its
  partial, then stop it before it enters duplicate `SCIDOCS` work.
- Do not launch the official-root head/compiler stage yet.  The required
  first-stage gate remains strict broad10 10/10 coverage.

## M549U.10 Hotpot Tail Boundary

The `spark-2` tail shard completed `HotpotQAHardNegatives` and wrote its
second partial task:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_tail_seed5511/
    m549u_broad10_root_gate_tail_seed5511.partial.json
```

Validated `spark-2` tail tasks:

```text
FiQA2018
HotpotQAHardNegatives
```

After the Hotpot partial was confirmed, the `spark-2` M549U tmux session and
orphan Docker container were stopped because the next task in that shard was
duplicate `SCIDOCS`.  The partial remains valid and non-overlapping when
combined with the `spark-1` non-overlapping tail shard.

The strict combiner was run over:

- `spark-1` first-five partial;
- `spark-2` `FiQA2018` + `HotpotQAHardNegatives` partial;
- `spark-1` non-overlapping `SCIDOCS` partial.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
FiQA2018
HotpotQAHardNegatives
SCIDOCS
```

Combined 8/10 macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99990329 |
| `root_query_cosine` | 0.99998832 |
| `score_pearson_dense` | 0.99999424 |
| `score_pearson_m549` | 0.99999175 |
| `m549_doc_support_cosine` | 0.99989707 |
| `m549_query_support_cosine` | 0.99998277 |
| `m549_doc_active_recall` | 0.98765527 |
| `m549_query_active_recall` | 0.98919315 |
| `dense_top100_overlap` | 0.99760994 |
| `m549_top100_overlap` | 0.99712095 |

New task row:

| Task | Docs | Queries | Root doc cos | Root query cos | M549 O@100 | Seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `HotpotQAHardNegatives` | 225621 | 1000 | 0.99998564 | 0.99998838 | 0.99715000 | 5918.042 |

Current decision:

- The combined root gate is now validated at 8/10 expected tasks.
- `spark-2` is no longer running M549U duplicate work.
- `spark-1` non-overlapping shard remains responsible for the remaining
  `TRECCOVID` and `Touche2020Retrieval.v3` tasks.
- Do not launch the official-root head/compiler stage yet.  The required
  first-stage gate remains strict broad10 10/10 coverage.

## M549U.11 TRECCOVID Tail Boundary

The `spark-1` non-overlapping tail shard completed `TRECCOVID` and wrote an
updated partial:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_nondup_tail_seed5512/
    m549u_broad10_root_gate_nondup_tail_seed5512.partial.json
```

Validated `spark-1` non-overlapping tail tasks:

```text
SCIDOCS
TRECCOVID
```

The strict combiner was run over:

- `spark-1` first-five partial;
- `spark-2` `FiQA2018` + `HotpotQAHardNegatives` partial;
- `spark-1` non-overlapping `SCIDOCS` + `TRECCOVID` partial.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
FiQA2018
HotpotQAHardNegatives
SCIDOCS
TRECCOVID
```

Combined 9/10 macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99991161 |
| `root_query_cosine` | 0.99998868 |
| `score_pearson_dense` | 0.99999465 |
| `score_pearson_m549` | 0.99999234 |
| `m549_doc_support_cosine` | 0.99990531 |
| `m549_query_support_cosine` | 0.99998322 |
| `m549_doc_active_recall` | 0.98762362 |
| `m549_query_active_recall` | 0.98950849 |
| `dense_top100_overlap` | 0.99765328 |
| `m549_top100_overlap` | 0.99701862 |

New task row:

| Task | Docs | Queries | Root doc cos | Root query cos | M549 O@100 | Seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `TRECCOVID` | 171332 | 50 | 0.99997813 | 0.99999160 | 0.99620000 | 8621.531 |

Current decision:

- The combined root gate is now validated at 9/10 expected tasks.
- The remaining task is `Touche2020Retrieval.v3`.
- At `2026-07-02 21:00 EDT`, `Touche2020Retrieval.v3` was actively encoding
  on `spark-1` at `52800/303732` docs with the tmux session alive.
- Do not launch the official-root head/compiler stage yet.  The required
  first-stage gate remains strict broad10 10/10 coverage.

## M549U.11a Official-Root Compiler Launch Guard

The official-root compiler runner was updated before the 10/10 root gate
finished, so the post-gate launch cannot accidentally re-enter the old broad4
smoke shape.

Changed defaults:

- `scripts/research_sae_m549u_official_root_compiler.py` now defaults to the
  canonical M549 broad10 task set;
- `scripts/run_m549u_official_root_compiler_spark.sh` now defaults `TASKS` to
  the same canonical broad10 list;
- `INIT_GAMMA` now defaults to the canonical M549 gamma `1.02562527`, not
  exact-dense gamma `1.0`.

Smoke subsets remain possible by explicitly setting `TASKS=...`, but the
default runner is now aligned with the next post-10/10 stage:

```text
frozen canonical broad10 root rows -> official-root compiler/head
    -> M549 tail768_1p025625 support
```

This does not start the head/compiler stage.  It only makes the pending launch
safer once `Touche2020Retrieval.v3` completes and strict 10/10 combine passes.

## M549U.11b Broad10 Official-Root Head-Only Evidence Run

While the final raw-text root-gate task was still running, `spark-2` was idle.
The official-root compiler runner was therefore launched as a non-promotion
evidence run:

```text
m549u_official_root_compiler_broad10_seed5520
```

Artifacts:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_official_root_compiler_broad10_seed5520/
    m549u_official_root_compiler_broad10_seed5520.json
    m549u_official_root_compiler_broad10_seed5520.md
    m549u_official_root_compiler_broad10_seed5520.compiler.pt
```

This run used the canonical M549 broad10 materialized root rows, not the M600
official1024 BEIR8 root.  It did not use BM25, qrels-specific ranking, or
fusion.

Config:

| Field | Value |
| --- | ---: |
| Tasks | 10/10 canonical M549 broad10 |
| Compiler mode | `tail_power` |
| Initial gamma | 1.02562527 |
| Target gamma | 1.02562527 |
| Train rows | 188674 |
| Trainable parameters | 1 |
| Global steps | 1860 |

Selector result:

| Field | Value |
| --- | ---: |
| Best epoch | 0 |
| Best global step | 0 |
| Learned gamma | 1.0256252289 |
| Floor-passing trained epochs | 0 |

Selected macro:

| Metric | Value |
| --- | ---: |
| `doc_support_cosine` | 0.99994285 |
| `query_support_cosine` | 0.99999360 |
| `doc_active_recall` | 0.99952679 |
| `query_active_recall` | 0.99965783 |
| `doc_active_tie_recall` | 1.00000000 |
| `query_active_tie_recall` | 1.00000000 |
| `top10_overlap_m549` | 0.99804688 |
| `top50_overlap_m549` | 0.99777116 |
| `top100_overlap_m549` | 0.99813154 |
| `top10_overlap_dense` | 0.99407934 |
| `top50_overlap_dense` | 0.99392261 |
| `top100_overlap_dense` | 0.99495179 |
| `score_pearson_m549` | 0.99999698 |
| `dense_score_pearson` | 0.99997554 |

Best trained epochs by individual metric still failed the hard floor:

| Individual metric | Epoch | Value | Floor ok |
| --- | ---: | ---: | --- |
| `eval_top100_overlap_m549` | 7 | 0.99712172 | false |
| `eval_doc_support_cosine` | 19 | 0.99993464 | false |
| `eval_doc_active_recall` | 13 | 0.99948158 | false |
| `eval_query_active_recall` | 10 | 0.99970055 | false |

Decision:

- The canonical deterministic tail-power compiler remains the first-stage
  engineering baseline.
- The scalar trained head-only route has now failed to beat epoch 0 on broad10
  under the hard floor.
- Do not add M551 auxiliary or LoRA as a rescue for this scalar route.  The
  stop condition is triggered for this head-only variant because training
  lowers loss while breaking overlap/support/active floors.
- This result does not replace the pending 10/10 raw-text root gate.  The
  final raw-text task `Touche2020Retrieval.v3` must still complete before any
  first-stage M549U promotion claim.

## M549U.12 Full Broad10 Raw-Text Root Gate

The `spark-1` non-overlapping tail shard completed
`Touche2020Retrieval.v3` and wrote a 3/3 partial:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_broad10_root_gate_nondup_tail_seed5512/
    m549u_broad10_root_gate_nondup_tail_seed5512.partial.json
```

Validated `spark-1` non-overlapping tail tasks:

```text
SCIDOCS
TRECCOVID
Touche2020Retrieval.v3
```

The strict combiner was run over:

- `spark-1` first-five partial;
- `spark-2` `FiQA2018` + `HotpotQAHardNegatives` partial;
- `spark-1` non-overlapping `SCIDOCS` + `TRECCOVID` +
  `Touche2020Retrieval.v3` partial.

Combined validated tasks:

```text
ArguAna
CQADupstackGamingRetrieval
CQADupstackUnixRetrieval
ClimateFEVERHardNegatives
FEVERHardNegatives
FiQA2018
HotpotQAHardNegatives
SCIDOCS
TRECCOVID
Touche2020Retrieval.v3
```

Combined 10/10 macro:

| Metric | Value |
| --- | ---: |
| `root_doc_cosine` | 0.99991893 |
| `root_query_cosine` | 0.99998901 |
| `score_pearson_dense` | 0.99999464 |
| `score_pearson_m549` | 0.99999228 |
| `m549_doc_support_cosine` | 0.99991260 |
| `m549_query_support_cosine` | 0.99998363 |
| `m549_doc_active_recall` | 0.98757975 |
| `m549_query_active_recall` | 0.98985611 |
| `dense_top100_overlap` | 0.99758183 |
| `m549_top100_overlap` | 0.99709227 |

New task row:

| Task | Docs | Queries | Root doc cos | Root query cos | M549 O@100 | Seconds |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `Touche2020Retrieval.v3` | 303732 | 49 | 0.99998480 | 0.99999195 | 0.99775510 | 32537.126 |

Decision:

- The full raw-text M549U root gate is now validated at 10/10 canonical
  M549 broad10 tasks.
- The deterministic first-stage surface is promoted as the engineering
  baseline:

```text
input text
  -> official PPLX/SentenceTransformer root
  -> canonical tail768_1p025625 compiler
  -> M549-equivalent 1024-dimensional support/posting vector
```

- The scalar trainable head-only broad10 evidence run selected epoch 0 under
  the hard floor, so no trained scalar head is promoted.
- Do not add M551 auxiliary or LoRA to rescue the scalar head-only route.
- The next valid step is broader validation of the deterministic M549U
  wrapper/encoder, not more scalar-head micro-tuning and not BM25 rescue.

## M549U.13 Official1024 BEIR8 Expansion Smoke

After the canonical M549 broad10 root gate passed, the next valid M549U.5
action is broader validation of the deterministic compiler.  This is an
expansion check, not a replacement for the canonical M549 surface:

- canonical M549 remains
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`;
- this smoke uses the separate official1024 BEIR8 adapter root
  `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`;
- no BM25, no qrels-specific training, no M551 auxiliary, and no fusion.

Run:

```text
m549u_official1024_beir8_smoke_seed5521
```

Artifacts:

```text
runs/m549u_official1024_beir8_smoke_seed5521/
  m549_m549u_official1024_beir8_smoke_seed5521.json
  m549_m549u_official1024_beir8_smoke_seed5521.md
  m549u_official1024_beir8_smoke_seed5521.log
```

Config:

| Field | Value |
| --- | ---: |
| Tasks | 8 BEIR-style tasks |
| Root dim | 1024 |
| Doc cap | 20000 |
| Query cap | 300 |
| Transform | `tail768_1p025625=tail:768:1.02562527` |
| Baselines | `exact_dense`, `dense_fp16`, `row_int8` |

Macro:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 | Teacher Cos | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` | 0.49886 | 0.38173 | 0.75799 | 0.55334 | 1.00000 | 0.99996981 | 0.00% | baseline |
| `row_int8` | 0.49885 | 0.38183 | 0.75802 | 0.55296 | 0.99648 | 1.00000000 | 2.65% | baseline |
| `tail768_1p025625` | 0.49892 | 0.38177 | 0.75785 | 0.55329 | 0.99502 | 0.99991168 | 2.69% | pass |

Tail transform deltas versus exact dense:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.00006 |
| MAP@100 | +0.00004 |
| Recall@100 | -0.00014 |
| MRR@20 | -0.00005 |

Per-task tail deltas:

| Task | Docs | Queries | Qrel queries | dNDCG@10 | dRecall@100 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 8674 | 300 | 300 | -0.00030 | 0.00000 | 0.99640 |
| `nfcorpus` | 3633 | 300 | 300 | -0.00024 | -0.00004 | 0.99370 |
| `fiqa` | 20000 | 300 | 182 | +0.00004 | 0.00000 | 0.99463 |
| `scidocs` | 20000 | 300 | 300 | +0.00080 | -0.00083 | 0.99527 |
| `scifact` | 5183 | 300 | 300 | +0.00016 | 0.00000 | 0.99430 |
| `trec-covid` | 20000 | 50 | 50 | -0.00001 | -0.00026 | 0.99380 |
| `webis-touche2020` | 20000 | 49 | 31 | 0.00000 | 0.00000 | 0.99694 |
| `cqadupstack` | 20000 | 300 | 25 | 0.00000 | 0.00000 | 0.99510 |

Decision:

- The deterministic M549U compiler transfers cleanly to a separate 1024-dim
  official root smoke: retrieval is dense-equivalent, KL improves by 2.69%,
  and O@100 remains at 0.99502.
- This supports broader validation of the deterministic wrapper/encoder.
- This does not reopen scalar/delta head-only training.  Both trainable
  head-only probes still selected epoch 0 under hard floors.
- The next useful M549U step is either a larger official1024 BEIR8 run or a
  raw-text wrapper packaging test, not M551/BM25 rescue.

### M549U.13a Qrel-Preserving Official1024 BEIR8 Smoke

The first official1024 smoke used random `DOC_LIMIT` sampling.  That was
acceptable for overlap/geometry, but it reduced qrel coverage for some tasks
because positive documents could be sampled out.  The evaluator now supports:

```text
PRESERVE_QREL_DOCS=1
```

With this flag, doc caps include positive qrel docs for the sampled queries
before filling the remaining budget randomly.  This keeps the broader smoke
BM25-free and training-free while making retrieval metrics more meaningful.

Run:

```text
m549u_official1024_beir8_qrelcap_seed5522
```

Artifacts:

```text
runs/m549u_official1024_beir8_qrelcap_seed5522/
  m549_m549u_official1024_beir8_qrelcap_seed5522.json
  m549_m549u_official1024_beir8_qrelcap_seed5522.md
  m549u_official1024_beir8_qrelcap_seed5522.log
```

Macro:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 | Teacher Cos | KL Rel | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` | 0.59342 | 0.42566 | 0.72071 | 0.67198 | 1.00000 | 0.99996987 | 0.00% | baseline |
| `row_int8` | 0.59354 | 0.42570 | 0.72077 | 0.67183 | 0.99650 | 1.00000000 | 2.19% | baseline |
| `tail768_1p025625` | 0.59325 | 0.42558 | 0.72043 | 0.67138 | 0.99551 | 0.99991186 | 2.12% | pass |

Tail transform deltas versus exact dense:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | -0.00017 |
| MAP@100 | -0.00008 |
| Recall@100 | -0.00028 |
| MRR@20 | -0.00060 |

Qrel coverage and per-task tail deltas:

| Task | Docs | Queries | Qrel queries | dNDCG@10 | dRecall@100 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 8674 | 300 | 300 | -0.00096 | 0.00000 | 0.99627 |
| `nfcorpus` | 3633 | 300 | 300 | -0.00024 | +0.00002 | 0.99350 |
| `fiqa` | 20000 | 300 | 300 | +0.00083 | 0.00000 | 0.99613 |
| `scidocs` | 20000 | 300 | 300 | -0.00044 | -0.00216 | 0.99563 |
| `scifact` | 5183 | 300 | 300 | +0.00016 | 0.00000 | 0.99430 |
| `trec-covid` | 20000 | 50 | 50 | +0.00072 | -0.00016 | 0.99740 |
| `webis-touche2020` | 20000 | 49 | 49 | -0.00144 | +0.00078 | 0.99571 |
| `cqadupstack` | 20000 | 300 | 300 | +0.00006 | -0.00067 | 0.99517 |

Decision:

- Prefer this qrel-preserving run over the earlier random-cap smoke for
  retrieval interpretation.
- The official1024 expansion remains dense-equivalent and gate-passing, but
  the signal is closer to neutral than positive: KL improves, overlap remains
  high, and retrieval deltas are within tolerance but not broadly better than
  exact dense.
- This strengthens the current boundary: the deterministic M549U compiler is
  the first-stage engineering baseline; trainable head-only routes are still
  not promoted.

## M549U.14 JSONL Wrapper Parity Verification

This step verifies the deployable deterministic wrapper surface rather than a
retrieval metric:

```text
raw text -> official ST/PPLX root -> canonical M549U tail compiler -> JSONL
```

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_encode_verify_seed5523/
```

Artifacts synced locally:

```text
runs/m549u_encode_verify_seed5523/
  docs_encoded.jsonl
  queries_encoded.jsonl
  docs_verify.json
  docs_verify.md
  queries_verify.json
  queries_verify.md
```

Scope:

- task: `FiQA2018`;
- rows: first 8 documents and first 8 queries;
- compiler: `tail768_1p025625`;
- no BM25, qrels, ranking labels, or trainable checkpoint.

Verifier checks two things:

1. Self-consistency: emitted `embedding` must equal applying the canonical
   M549U compiler to the emitted `root_embedding`.
2. Teacher parity: emitted root/support should remain close to the
   materialized M549 dense-root rows for matching ids.

| Split | Rows | Metadata errors | Active mismatches | Self support cos | Self active recall | Root vs teacher cos | Root norm max abs | Support vs teacher cos | Support active recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Docs | 8 | 0 | 0 | 1.0000000000 | 1.0000000000 | 0.9999876618 | 0.0020775646 | 0.9998848438 | 0.9843750000 |
| Queries | 8 | 0 | 0 | 1.0000000000 | 1.0000000000 | 0.9999904037 | 0.0014719479 | 0.9998768568 | 0.9902343750 |

Notes:

- The wrapper's internal compiler path is exact on this smoke:
  self support cosine is `1.0`, active recall is `1.0`, and max abs is `0.0`.
- Raw `root_vs_teacher_max_abs` is not used as the primary gate because the
  materialized teacher rows and emitted normalized root can have different raw
  scales.  The verifier now reports normalized max-abs explicitly.
- This validates the engineering wrapper baseline, not the trainable output
  head.  The M549U goal remains open because head-only compiler training still
  selected epoch 0 under hard active/support/overlap floors.

Decision:

- Promote the deterministic JSONL wrapper as the deployable first-stage
  baseline for M549U packaging tests.
- Keep the trainable head route separate: the next model-side step should be a
  constrained compiler/head probe against this exact wrapper/root contract,
  not M551, BM25, or official1024 rescue.

## M549U.15 Checkpoint Compiler Path Verification

M549U.14 verified the static compiler branch.  M549U.15 verifies that the same
deployable JSONL wrapper can consume the canonical broad10 compiler checkpoint
and emit the exact same vectors.

Checkpoint:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_official_root_compiler_broad10_seed5520/
    m549u_official_root_compiler_broad10_seed5520.compiler.pt
```

Checkpoint summary:

- compiler mode: `tail_power`;
- trainable parameters: `1`;
- selected epoch: `0`;
- learned gamma in checkpoint: `1.0256252289`;
- canonical target gamma: `1.02562527`.

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_checkpoint_encode_verify_seed5524/
```

Artifacts synced locally:

```text
runs/m549u_checkpoint_encode_verify_seed5524/
  docs_encoded.jsonl
  queries_encoded.jsonl
  docs_verify.json
  docs_verify.md
  queries_verify.json
  queries_verify.md
```

Verifier metrics match the static path:

| Split | Rows | Metadata errors | Active mismatches | Self support cos | Self active recall | Root vs teacher cos | Root norm max abs | Support vs teacher cos | Support active recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Docs | 8 | 0 | 0 | 1.0000000000 | 1.0000000000 | 0.9999876618 | 0.0020775646 | 0.9998848438 | 0.9843750000 |
| Queries | 8 | 0 | 0 | 1.0000000000 | 1.0000000000 | 0.9999904037 | 0.0014719479 | 0.9998768568 | 0.9902343750 |

Static-vs-checkpoint output comparison:

| Split | Rows | Root max abs diff | Embedding max abs diff | Active rows equal |
| --- | ---: | ---: | ---: | ---: |
| Docs | 8 | 0.0000000000 | 0.0000000000 | 8 / 8 |
| Queries | 8 | 0.0000000000 | 0.0000000000 | 8 / 8 |

Decision:

- The canonical epoch0 head checkpoint is a valid deployable compiler surface:
  the wrapper can load it and produce byte-level-equivalent numeric vectors to
  the static compiler branch.
- This is still a conservative first-stage success, not a learned improvement:
  the promoted compiler checkpoint is trainable by construction but selected
  the canonical initialization under the hard floors.
- Do not spend more effort on unconstrained residual freedom unless a new head
  parameterization can preserve this exact checkpoint/static equivalence as
  its epoch0 floor.

## M549U.16 Unified Encoder API Smoke

The JSONL encoder is now internally structured around a single
`M549UUnifiedEncoder` class.  The CLI still writes the same JSONL surface, but
the engineering boundary is now one callable first-stage encoder:

```text
texts -> M549UUnifiedEncoder.encode(...) -> root vectors + posting vectors
```

Run:

```text
/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/
  m549u_unified_encoder_api_smoke_seed5525/
```

Artifacts synced locally:

```text
runs/m549u_unified_encoder_api_smoke_seed5525/
  docs_static.jsonl
  docs_ckpt.jsonl
  static_vs_ckpt.json
```

The smoke used the same `FiQA2018` input rows through:

- static canonical compiler branch;
- canonical broad10 epoch0 checkpoint branch.

Static-vs-checkpoint comparison after the API refactor:

| Rows | Root max abs diff | Embedding max abs diff | Active rows equal |
| ---: | ---: | ---: | ---: |
| 4 | 0.0000000000 | 0.0000000000 | 4 / 4 |

Decision:

- The refactor did not change the emitted M549U vectors.
- M549U now has an explicit unified encoder API/CLI surface for engineering
  use.  The root encoder and compiler head are no longer only ad-hoc
  postprocess functions in the CLI.
- This moves the implementation closer to the requested first-stage unified
  encoder, but it does not change the scientific result: the only promoted
  compiler checkpoint remains the canonical epoch0 tail-power head.

## M549U.17 Scalar Teacher-Fit From Non-Teacher Init

M549U.17 checks whether the one-parameter tail-power head can be trained from a
non-M549 initialization back into a gate-passing M549-equivalent compiler.
This uses only frozen official root rows and M549 support targets:

- no BM25;
- no qrels-specific ranking;
- no candidate-set KL;
- no M551 auxiliary.

All runs used broad4 (`ArguAna`, `FiQA2018`, `SCIDOCS`, `TRECCOVID`) and
initialized `gamma=1.0`, then trained only the scalar gamma parameter.

| Run | Change | Selected epoch | Selected gamma | Best trained epoch | Best trained gamma | Best trained doc active | Best trained query active | Best trained top100 | Best trained doc cos | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `m549u_scalar_teacherfit_broad4_seed5526` | pure support/active fit | 0 | 1.0000000000 | 5 | 1.0233697891 | 0.99941063 | 0.99987793 | 0.99854766 | 0.99999157 | fail active floor |
| `m549u_scalar_teacherfit_margin_broad4_seed5527` | add active margin | 0 | 1.0000000000 | 1 | 1.0236198902 | 0.99937248 | 0.99978638 | 0.99841953 | 0.99974740 | fail active floor |
| `m549u_scalar_teacherfit_converge_broad4_seed5528` | lower LR, 5000 steps | 0 | 1.0000000000 | 9 | 1.0232993364 | 0.99940968 | 0.99964905 | 0.99823281 | 0.99999158 | fail active floor |

The trained scalar head repeatedly moves toward the M549 gamma region and
improves support/top100 geometry:

- support cosine reaches roughly `0.9999916`;
- M549 top100 overlap reaches roughly `0.9982` to `0.9985`;
- score Pearson reaches roughly `0.9999976`.

But every trained epoch violates the hard exact-active floor.  Tie-aware active
recall stays at `1.0`, so this is a boundary/tie-sensitive failure, not a large
semantic collapse.  It still cannot be promoted under the M549U first-stage
contract because the contract requires active membership not to regress.

Decision:

- Keep the promoted compiler checkpoint at the canonical epoch0 tail-power
  surface.
- Do not add M551 auxiliary or BM25 rescue to this scalar-from-1.0 training
  line.
- A useful future head must either preserve active membership exactly by
  construction or define an explicit tie-safe active contract before training.
  More unconstrained scalar optimization is not a good next step.

## M549U.18 Active-Locked Output Compiler

M549U.18 tests the next constrained head shape:

```text
frozen M549 root row -> active_locked_tail_power compiler -> M549-equivalent
support vector
```

The compiler still trains only one scalar tail gamma, but it locks the
canonical M549 active set by construction.  The active mask is computed with
the same NumPy/M549-compatible path used by the evaluator, not Torch `topk`,
so the training path and gate path agree on row-int8/tie boundaries.

Rejected implementation probes:

- `m549u_active_locked_tail_broad4_seed5529`: tied all active values at the
  same floor, so active recall fell to about `0.980`.
- `m549u_active_locked_tail_rank_broad4_seed5530`: added rank offsets but
  still used Torch masks, so it disagreed with the evaluator at ties.

The fixed NumPy-mask broad4 smoke passed and selected a trained epoch:

```text
runs/m549u_active_locked_numpy_broad4_seed5531/
```

Broad4 selected metrics:

| Metric | Initial | Selected |
| --- | ---: | ---: |
| Doc support cosine | 0.99957523 | 0.99963379 |
| Query support cosine | 0.99993932 | 1.00000000 |
| Doc active recall | 0.99999046 | 0.99999046 |
| Query active recall | 1.00000000 | 1.00000000 |
| M549 top100 overlap | 0.99559922 | 1.00000000 |
| M549 score Pearson | 0.99998179 | 1.00000000 |
| Doc support KL | 0.00845949 | 0.00822387 |
| Query support KL | 0.01198143 | 0.01169389 |

Canonical broad10 head-only gate:

```text
runs/m549u_active_locked_numpy_broad10_seed5532/
```

Selected checkpoint:

- selected epoch: `1`;
- selected global step: `738`;
- learned gamma: `1.0256251097`;
- trainable parameters: `1`.

Broad10 head-only metrics:

| Metric | Initial | Selected |
| --- | ---: | ---: |
| Doc support cosine | 0.99989211 | 0.99995117 |
| Query support cosine | 0.99993950 | 1.00000000 |
| Doc active recall | 0.99998512 | 0.99998512 |
| Query active recall | 1.00000000 | 1.00000000 |
| M549 top100 overlap | 0.99501162 | 1.00000000 |
| M549 score Pearson | 0.99997620 | 1.00000000 |
| Doc support KL | 0.00937005 | 0.00915310 |
| Query support KL | 0.01204053 | 0.01176493 |

This is the first trained non-epoch0 M549U head that passes the canonical
broad10 head-only active/support/top100 gate.

Full broad10 retrieval matrix:

```text
runs/m549u_active_locked_broad10_matrix_seed5532/
```

Macro result:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | Teacher Cos | KL Rel | Retrieval Gate | Teacher Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.99982449 | 0.00% | pass | fail |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | 0.99990234 | 2.72% | pass | pass |
| `tail768_1p025625` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | 0.99986367 | 3.53% | pass | fail |
| `active_locked` | 0.57278 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | 0.99981483 | 2.45% | pass | fail |

Interpretation:

- The active-locked checkpoint is retrieval-equivalent to canonical M549 on the
  full broad10 matrix: NDCG, MAP, Recall, MRR, and O@100 match to report
  precision.
- It is not a better teacher surface than canonical M549.  Its teacher support
  cosine and KL are weaker than `tail768_1p025625`, although still better than
  exact dense on KL.
- The `Teacher Gate` column fails for both canonical `tail768_1p025625` and
  `active_locked` in this run because the evaluator uses a sampled active-drop
  gate with zero tolerance.  The active drop is tiny (`5.265e-05` for
  canonical and `7.7e-07` for active-locked) and does not affect the retrieval
  matrix.  The actionable result is therefore not "new teacher promoted"; it
  is "trained compiler can emit an M549-equivalent retrieval surface."

Decision:

- Promote `m549u_active_locked_numpy_broad10_seed5532.compiler.pt` as the first
  trained-head engineering-equivalent checkpoint candidate.
- Do not replace the canonical M549 teacher with active-locked.  Canonical
  `tail768_1p025625` remains the teacher/default.
- The next validation should be an encoder-wrapper smoke using this
  active-locked checkpoint, then a broader streaming matrix only after the
  wrapper proves static/checkpoint parity.

## M549U.19 Active-Locked Encoder Wrapper Smoke

This smoke verifies the deployable JSONL encoder path can consume the
active-locked trained compiler checkpoint:

```text
runs/m549u_active_locked_encoder_smoke_seed5533/
```

Input was the first four FiQA documents.  The comparison is static canonical
M549 wrapper output versus `M549UUnifiedEncoder` with
`m549u_active_locked_numpy_broad10_seed5532.compiler.pt`.

Wrapper smoke metrics:

| Metric | Value |
| --- | ---: |
| Rows | 4 |
| Root max absolute diff | 0 |
| Embedding max absolute diff | 9.685754776e-08 |
| Minimum embedding cosine | 0.9999999999999793 |
| Ordered active list equal rows | 2 / 4 |
| Active set minimum recall | 1.0 |
| Active set mean recall | 1.0 |

Interpretation:

- The wrapper can load the active-locked compiler checkpoint and produce a
  numerically equivalent M549 surface.
- Ordered `active_indices` differs on two rows only at tie/order sensitivity:
  the active membership set is identical for all rows.
- This closes the immediate engineering packaging gap for the active-locked
  trained head.  It does not change the teacher decision: canonical
  `tail768_1p025625` remains the default target, while the active-locked
  checkpoint is a deployable M549-equivalent compiler candidate.

## M549U.20 Official1024 BEIR8 Root-Expansion Matrix

This is a broader root-expansion validation, not a replacement for the
canonical M549 broad10 proof.  The run uses the official1024 BEIR8 root:

```text
/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks
```

Run output:

```text
runs/m549u_active_locked_official1024_beir8_matrix_seed5534/
```

Configuration:

- tasks: `arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`, `trec-covid`,
  `webis-touche2020`, `cqadupstack`;
- sources: `exact_dense`, `tail768_1p025625`, `active_locked`;
- BM25: disabled;
- compressed baselines: disabled;
- active-locked checkpoint:
  `m549u_active_locked_numpy_broad10_seed5532.compiler.pt`.

Macro result:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | Teacher Cos | KL | KL Rel | Retrieval Gate | Teacher Gate | Overall Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| `exact_dense` | 0.48304 | 0.31424 | 0.63220 | 0.56465 | 1.00000 | 0.99996988 | 0.00876688 | 0.00% | pass | fail | fail |
| `tail768_1p025625` | 0.48310 | 0.31424 | 0.63234 | 0.56425 | 0.99519 | 0.99991179 | 0.00854926 | 2.48% | pass | pass | pass |
| `active_locked` | 0.48310 | 0.31424 | 0.63234 | 0.56425 | 0.99519 | 0.99991203 | 0.00858331 | 2.09% | pass | pass | fail |

Per-task retrieval shape:

| Task | Exact NDCG@10 | Canonical NDCG@10 | Active-Locked NDCG@10 | Canonical R@100 | Active-Locked R@100 | Active-Locked O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.43600 | 0.43597 | 0.43597 | 1.00000 | 1.00000 | 0.99623 |
| `nfcorpus` | 0.35870 | 0.35847 | 0.35847 | 0.32688 | 0.32688 | 0.99356 |
| `fiqa` | 0.51678 | 0.51668 | 0.51668 | 0.82887 | 0.82887 | 0.99577 |
| `scidocs` | 0.22927 | 0.22914 | 0.22914 | 0.49583 | 0.49583 | 0.99510 |
| `scifact` | 0.74789 | 0.74805 | 0.74805 | 0.96333 | 0.96333 | 0.99430 |
| `trec-covid` | 0.85338 | 0.85410 | 0.85410 | 0.16727 | 0.16727 | 0.99660 |
| `webis-touche2020` | 0.27543 | 0.27571 | 0.27571 | 0.49401 | 0.49401 | 0.99551 |
| `cqadupstack` | 0.44687 | 0.44670 | 0.44670 | 0.78250 | 0.78250 | 0.99447 |

Interpretation:

- The canonical M549 transform generalizes to this 1024-dimensional BEIR8
  root as a small dense-equivalent improvement: `+0.00006` NDCG@10,
  `+0.00014` Recall@100, and `2.48%` KL improvement versus exact dense.
- The active-locked trained compiler is retrieval-equivalent to canonical M549
  on every BEIR8 task in this matrix.
- The active-locked compiler still does not supersede canonical M549 as the
  teacher: its KL improvement is weaker (`2.09%` versus `2.48%`), so the
  selected candidate remains `tail768_1p025625`.
- This strengthens the engineering case for M549U first-stage output compiler:
  the trained head preserves canonical retrieval behavior outside the original
  M549 broad10 root, while the canonical deterministic teacher remains the
  target surface.

## M549U.21 Stage-2 Fixed BM25 Compatibility

This stage is a compatibility check after the first-stage M549U surface is
stable.  It is not first-stage training and it does not train BM25 into the
encoder.

Goal:

```text
M549U active-locked compiler + fixed M550 z-score BM25 alpha=0.10
  should match
M549 tail768_1p025625 + fixed M550 z-score BM25 alpha=0.10
```

The evaluator uses a fixed blend:

```text
zscore(dense_or_posting_score) + 0.10 * zscore(bm25_score)
```

The first smoke used `FiQA2018`, `SCIDOCS`, and `TRECCOVID`:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.49822 | 0.24721 | 0.49615 | 0.62342 | 1.00000 |
| `m549_tail768` | 0.49810 | 0.24715 | 0.49604 | 0.62365 | 0.99537 |
| `m549u_active_locked` | 0.49810 | 0.24715 | 0.49604 | 0.62365 | 0.99537 |
| `m549_bm25_zblend_a010` | 0.52052 | 0.25530 | 0.49830 | 0.63936 | 0.86709 |
| `m549u_active_locked_bm25_zblend` | 0.52052 | 0.25530 | 0.49830 | 0.63936 | 0.86709 |

Decision from smoke:

- M549U active-locked can substitute for M549 in the fixed M550 blend on this
  three-task surface.
- The result confirms compatibility only.  It does not make BM25 part of the
  first-stage encoder.
- A prior evaluator bug was fixed before this smoke: source detection now
  treats any source containing `_bm25_zblend` as blended, instead of relying on
  `endswith('_bm25_zblend')`.

Full broad10 split validation completed and was merged with the strict helper:

```text
scripts/research_sae_m549u_merge_stage2_fixed_bm25.py
```

Valid inputs:

```text
runs/m549u_stage2_fixed_bm25_broad10_s1_seed5536/
  m549u_m549u_stage2_fixed_bm25_broad10_s1_seed5536.json
runs/m549u_stage2_fixed_bm25_broad10_s2b_seed5536/
  m549u_m549u_stage2_fixed_bm25_broad10_s2b_seed5536.json
```

Merged output:

```text
runs/m549u_stage2_fixed_bm25_broad10_merged_seed5536/
  m549u_stage2_fixed_bm25_broad10_merged.json
  m549u_stage2_fixed_bm25_broad10_merged.md
```

The merge gate checked:

- exact task coverage: canonical M549 broad10, `10/10` tasks;
- no duplicate task units;
- all expected sources present for every task;
- `m549u_active_locked` equals `m549_tail768` on NDCG@10, MAP@100,
  Recall@100, MRR@20, and O@100 for every task;
- `m549u_active_locked_bm25_zblend` equals `m549_bm25_zblend_a010` on the
  same metrics for every task;
- strict tolerance: `0.0`;
- total equivalence checks: `100`;
- stale failed `s2` run was not merged.

Full broad10 macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00000 | 0.00000 |
| `m549_tail768` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | +0.00016 | -0.00001 |
| `m549u_active_locked` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 | +0.00016 | -0.00001 |
| `m549_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 | +0.01782 | +0.00691 |
| `m549u_active_locked_bm25_zblend` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 | +0.01782 | +0.00691 |

Decision:

- M549U active-locked is now proven compatible with the M550 fixed BM25
  stage on the original M549 broad10 surface.
- This completes the Stage-2 handoff check: `M549U + fixed BM25 alpha=0.10`
  is metric-equivalent to `M549 + fixed BM25 alpha=0.10`.
- This does not change the first-stage success definition.  M549U remains a
  BM25-free unified encoder/compiler surface; BM25 remains a separate
  second-stage compatibility baseline.

## M549U.22 Streaming Broader Validation

M549U.20 used a materialized official1024 BEIR8 matrix.  M549U.22 validates the
same first-stage surface with a streaming evaluator so larger BEIR/MTEB roots
do not need full query-by-document score matrices in memory.

New evaluator:

```text
scripts/research_sae_m549u_streaming_broader_eval.py
scripts/run_m549u_streaming_broader_spark.sh
```

The first implementation used Python heap merging and stalled on
`cqadupstack` because `13,145` queries by `457,199` documents produced hundreds
of millions of Python heap updates.  The evaluator was corrected to keep
per-query top-k buffers on device and merge top-k blocks with tensor operations.

Run:

```text
runs/m549u_streaming_beir8_full_seed5540/
  m549u_streaming_broader.json
  m549u_streaming_broader.md
```

Configuration:

- root:
  `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`;
- tasks: `arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`, `trec-covid`,
  `webis-touche2020`, `cqadupstack`;
- sources: `exact_dense`, `tail768_1p025625`, `active_locked`;
- active-locked checkpoint:
  `m549u_active_locked_numpy_broad10_seed5532.compiler.pt`;
- BM25: disabled;
- fusion: disabled;
- qrels-specific training/selection: disabled.

Macro result:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.48298 | 0.31423 | 0.63219 | 0.56465 | 1.00000 | 0.00000 | 0.00000 |
| `tail768_1p025625` | 0.48314 | 0.31427 | 0.63234 | 0.56435 | 0.99514 | +0.00016 | +0.00015 |
| `active_locked` | 0.48314 | 0.31427 | 0.63234 | 0.56435 | 0.99514 | +0.00016 | +0.00015 |

Per-task check:

| Task | Exact NDCG@10 | Tail NDCG@10 | Active-Locked NDCG@10 | Tail R@100 | Active-Locked O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.43600 | 0.43606 | 0.43606 | 1.00000 | 0.99613 |
| `nfcorpus` | 0.35845 | 0.35848 | 0.35848 | 0.32687 | 0.99350 |
| `fiqa` | 0.51651 | 0.51671 | 0.51671 | 0.82887 | 0.99577 |
| `scidocs` | 0.22937 | 0.22926 | 0.22926 | 0.49583 | 0.99507 |
| `scifact` | 0.74789 | 0.74805 | 0.74805 | 0.96333 | 0.99433 |
| `trec-covid` | 0.85338 | 0.85410 | 0.85410 | 0.16728 | 0.99660 |
| `webis-touche2020` | 0.27543 | 0.27571 | 0.27571 | 0.49401 | 0.99531 |
| `cqadupstack` | 0.44683 | 0.44674 | 0.44674 | 0.78250 | 0.99445 |

Decision:

- The broader streaming evaluator reproduces the expected official1024 BEIR8
  shape without BM25 or fusion.
- `active_locked` is retrieval-equivalent to canonical `tail768_1p025625` on
  all eight streaming tasks and all reported macro metrics.
- The small exact-dense macro difference versus the materialized matrix is
  within report precision and comes from the independent streaming top-k path;
  the relevant M549U claim is unchanged: canonical tail and active-locked are
  equivalent on this broader root.
- This closes the M549U broader streaming validation surface.  The canonical
  teacher remains `tail768_1p025625`; active-locked remains the deployable
  trained-head compiler candidate.
