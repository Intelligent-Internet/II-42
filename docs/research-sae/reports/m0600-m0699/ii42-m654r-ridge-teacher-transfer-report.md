# M654R Ridge-Teacher Transfer

Status: `not promoted`

M653G proved that query-side capacity exists: a per-query ridge oracle can
improve dense overlap and Recall inside frozen P1 document postings and the
original P1 query support.  M654R tests the next question: can a trainable
compiler learn that safe-fit target?

This remains a first-stage dense-equivalence experiment:

- no BM25 in training or selection;
- no reranker;
- no learned gate;
- no qrels-driven loss;
- frozen P1 document posting geometry;
- qrels used only for metric reporting.

## Implementation

Added `scripts/train_m654r_ridge_teacher_compiler.py`.

The script builds ridge teachers by fitting dense scores over the union of
dense topK and P1 topK documents.  It then trains a query-side compiler to
distill those teacher queries.

Two compiler families were tested:

1. Existing root-only M549U compiler.
2. `p1_support_residual` adapter, which is closer to the desired output-layer
   modification: input is `dense root + P1 query posting`, output is a residual
   constrained to the original P1 query support.

The adapter also gained `--adapter-lock-active`, which preserves the top active
membership by construction.

## Runs

### Root-only compiler

Run: `m654r_ridge10_query_compiler_seed6541`

Result: no dev checkpoint passed.  Final selected checkpoint stayed at epoch0.
The root-only compiler reduced teacher loss but moved dense overlap in the
wrong direction.

Test deltas versus P1:

| Metric | Delta |
| --- | ---: |
| O@100 | `-0.000905858` |
| O@256 | `-0.000104595` |
| Recall@100 | `+0.000000000` |
| MAP@100 | `+0.000036552` |

Interpretation: root-only M549U compiler is not the right transfer shape for
the ridge teacher.

### P1-support adapter, M653-D row split

Run: `m654r_p1_support_residual_ridge10_seed6541`

Result: train loss dropped, but dev overlap and support gates failed.  This
showed the adapter can fit the train teacher but does not generalize from only
253 M653-D train queries.

### P1-support adapter, all-query canary split

Runs:

- `m654r_allq_p1_support_residual_ridge10_seed6541`
- `m654r_allq_p1_support_residual_ridge1_seed6541`
- `m654r_allq_p1_support_residual_ridge1_s001_seed6541`

The all-query split used `arguana,cqadupstack,fiqa,scidocs` with deterministic
query splits: `320 train / 40 dev / 40 test`.

This exposed an evaluation mismatch: on the all-query split, P1 itself has
`support_cosine≈0.9896`, below the old absolute `0.999` floor.  Therefore the
strict support floor was impossible on that surface.  This does not invalidate
the M653-D strict gate; it means all-query training needs a baseline-safe
support floor or a relative support gate.

### Active-locked P1-support adapter

Run: `m654r_allq_p1_support_residual_lock_ridge1_s001_seed6541`

Active-lock fixed `active_support_floor`: all active-support checks passed.
The remaining blocker was support cosine under the old absolute `0.999` floor.
With a diagnostic baseline-safe floor of `0.98`, the run selected a dev
checkpoint, but held-out test still failed O@256 by a tiny amount:

Run: `m654r_allq_lock_ridge1_s001_floor98_seed6541`

| Metric | Test delta |
| --- | ---: |
| O@100 | `+0.000000000` |
| O@256 | `-0.000018601` |
| Recall@100 | `+0.000000000` |
| MAP@100 | `+0.000255867` |
| MRR@20 | `+0.000189394` |

### Stronger tail teacher

Run: `m654r_allq_lock_ridge01_s001_floor98_seed6541`

`ridge_0.1` had stronger O@256 oracle signal.  It produced multiple dev-pass
epochs under the baseline-safe support floor:

| Epoch | Dev O@100 | Dev O@256 | Dev Recall@100 | Dev MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 5 | `+0.000000` | `+0.000326` | `+0.004167` | `+0.000108` |
| 7 | `+0.000167` | `+0.000521` | `+0.004167` | `+0.001825` |
| 8 | `+0.000625` | `+0.000374` | `+0.004167` | `+0.001827` |
| 9 | `+0.000167` | `+0.000260` | `+0.004167` | `+0.001607` |

But held-out test still failed O@256:

| Metric | Test delta |
| --- | ---: |
| O@100 | `+0.000869048` |
| O@256 | `-0.000432478` |
| Recall@100 | `+0.000000000` |
| MAP@100 | `+0.001396008` |
| NDCG@10 | `+0.000780043` |
| MRR@20 | `+0.000614564` |

Interpretation: the adapter learned useful rank movement, but O@256
tail-preservation did not generalize robustly from this small split.

## Decision

Do not promote M654R yet.

The result is not a dead end.  It narrows the problem:

1. M653G proves a safe query-side solution exists.
2. M654R proves a trainable P1-support adapter can learn useful O@100/MAP/NDCG
   movement.
3. M654R also shows that robust O@256 tail preservation is still the transfer
   blocker.

The current all-query canary split is too small to make the trainable adapter
robust.  The next version should not keep tuning this tiny split.  It should
scale the ridge-teacher distillation surface before changing the objective
again.

## Next Step

M654S should build a larger qrels-free ridge-teacher corpus:

1. Generate ridge teachers for all available shared15 queries, and then for the
   full available BEIR query roots if present.
2. Use a relative support gate: active support must not regress; support cosine
   must not regress versus P1 on the same evaluation surface.
3. Keep `p1_support_residual + active-lock` as the candidate architecture.
4. Select checkpoints on dev O@256 first, then O@100/Recall/MAP.
5. Only replay full shared15 if held-out canary test passes O@256, O@100, CUB,
   and Recall together.

This keeps the work aligned with the first-stage objective: dense-equivalence
recovery before BM25, reranking, or retrieval-expanded objectives.
