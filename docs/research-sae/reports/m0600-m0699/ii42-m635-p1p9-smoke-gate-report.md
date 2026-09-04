# M635 / P1.9 Smoke Gate Report

## Status

Status: `retrieval_smoke_recall_guard_failed`

M635-A passed the teacher-fit smoke but did not pass the retrieval smoke gate.
M635-B/C then tested qrels-blind dense-root head anchoring over the generated
compiler output.  M635-D tested a structured active-locked bucket-scale
compiler with root-head and false-head guards.  All variants failed the Recall
guard.  The result is close to dense/M549 target and does not repeat the
M632/M634 collapse pattern, but it is not strong enough to expand to shared15.

## Artifacts

| Artifact | Path |
| --- | --- |
| Teacher-target audit JSON | `runs/m635_p1p9_teacher_target_audit_v1/m635_teacher_target_audit.json` |
| Training JSON | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_output_compiler_smoke_seed6351/m635_p1p9_output_compiler_smoke_seed6351.json` |
| Training checkpoint | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_output_compiler_smoke_seed6351/m635_p1p9_output_compiler_smoke_seed6351.compiler.pt` |
| Retrieval smoke JSON | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_seed6351/m635_retrieval_smoke_matrix.json` |
| Retrieval smoke Markdown | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_seed6351/m635_retrieval_smoke_matrix.md` |
| M635-B anchor rows | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_anchor_seed6351/m635_retrieval_smoke_eval_rows.jsonl` |
| M635-C high-anchor rows | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_anchor_hi_seed6351/m635_retrieval_smoke_eval_rows.jsonl` |
| M635-D training JSON | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_structured_compiler_smoke_seed6353/m635_p1p9_structured_compiler_smoke_seed6353.json` |
| M635-D checkpoint | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_structured_compiler_smoke_seed6353/m635_p1p9_structured_compiler_smoke_seed6353.compiler.pt` |
| M635-D retrieval JSON | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_structured_seed6353/m635_retrieval_smoke_matrix.json` |
| M635-D eval rows | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_structured_seed6353/m635_retrieval_smoke_eval_rows.jsonl` |

## Teacher-Fit Gate

| Requirement | Evidence | Status |
| --- | --- | --- |
| Frozen dense-root output compiler, not reranker | Runner uses `active_locked_tail_power`; no BM25, qrels-primary, alpha search, or frozen-row reranking. | pass |
| Hard active support preserved | doc active recall `0.999990`; query active recall `1.000000`. | pass |
| Support geometry improves | doc cosine `0.999941 -> 0.999992`; query cosine `0.999940 -> 0.999992`. | pass |
| KL does not regress | doc KL `0.008260 -> 0.008002`; query KL `0.011997 -> 0.011632`. | pass |
| Dense/M549 overlap preserved | O@100 vs dense unchanged at `0.995921`; O@100 vs M549 improves to `0.998250`. | pass |

## Retrieval Smoke Matrix: M635-A

Smoke surface: `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna`.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.819525 | 1.000000 | 0.487574 | 0.266242 | 0.622110 | 0.548185 |
| `m549_target` | 0.819371 | 0.995613 | 0.487419 | 0.266112 | 0.622029 | 0.548283 |
| `m635_compiler` | 0.819386 | 0.994087 | 0.487460 | 0.266136 | 0.621931 | 0.548303 |

Delta M635 compiler vs dense root:

| Metric | Delta |
| --- | ---: |
| CUB | -0.000139 |
| dense overlap@100 | -0.005913 |
| NDCG@10 | -0.000114 |
| MAP@100 | -0.000107 |
| Recall@100 | -0.000179 |
| MRR@20 | +0.000118 |

Delta M635 compiler vs M549 target:

| Metric | Delta |
| --- | ---: |
| CUB | +0.000014 |
| dense overlap@100 | -0.001526 |
| NDCG@10 | +0.000041 |
| MAP@100 | +0.000024 |
| Recall@100 | -0.000098 |
| MRR@20 | +0.000020 |

## Dense-Root Anchor Diagnostics

M635-B/C tested whether a qrels-blind dense-root head anchor can recover the
tiny Recall loss while staying in the generated-output path.  It does not.

M635-B low anchor:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.819525 | 1.000000 | 0.487574 | 0.266242 | 0.622110 | 0.548185 |
| `m635_compiler` | 0.819386 | 0.994087 | 0.487460 | 0.266136 | 0.621931 | 0.548303 |
| `anchor_a0010` | 0.819394 | 0.994237 | 0.487448 | 0.266130 | 0.621943 | 0.548286 |
| `anchor_a0025` | 0.819394 | 0.994300 | 0.487452 | 0.266136 | 0.621943 | 0.548286 |
| `anchor_a0050` | 0.819401 | 0.994449 | 0.487474 | 0.266166 | 0.621943 | 0.548314 |
| `anchor_a0100` | 0.819358 | 0.994701 | 0.487423 | 0.266127 | 0.621990 | 0.548284 |

M635-C high anchor:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `anchor_a0200` | 0.819361 | 0.995249 | 0.487418 | 0.266147 | 0.622039 | 0.548301 |
| `anchor_a0350` | 0.819520 | 0.996258 | 0.487391 | 0.266066 | 0.622010 | 0.548219 |
| `anchor_a0500` | 0.819498 | 0.997112 | 0.487358 | 0.266123 | 0.621959 | 0.548288 |
| `anchor_a0750` | 0.819517 | 0.998260 | 0.487536 | 0.266201 | 0.621951 | 0.548182 |

The best anchored variant is `anchor_a0200`: it improves over the raw compiler
and over M549 target on Recall, but still misses dense root by about
`0.000071`.

## Structured Compiler Control: M635-D

M635-D changed the generated compiler family instead of interpolating output
at evaluation time:

- compiler: `active_locked_bucket_scale`,
- trainable parameters: `9`,
- bucket count: `8`,
- bucket scale limit: `0.15`,
- root head preservation: `top20,top50`, weight `0.25`,
- dense-tail/false-head guard weight: `0.05`.

Teacher-fit still improves:

| Metric | Initial | Final |
| --- | ---: | ---: |
| doc support cosine | 0.999697 | 0.999749 |
| query support cosine | 0.999939 | 0.999992 |
| doc support KL | 0.008400 | 0.008145 |
| query support KL | 0.011950 | 0.011587 |
| top100 overlap vs M549 | 0.995259 | 0.998359 |
| top100 overlap vs dense | 0.995259 | 0.995259 |

Retrieval smoke:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.819525 | 1.000000 | 0.487574 | 0.266242 | 0.622110 | 0.548185 |
| `m549_target` | 0.819371 | 0.995613 | 0.487419 | 0.266112 | 0.622029 | 0.548283 |
| `m635_compiler` | 0.819327 | 0.994136 | 0.487434 | 0.266106 | 0.621931 | 0.548271 |
| `anchor_a0200` | 0.819347 | 0.995270 | 0.487418 | 0.266147 | 0.621989 | 0.548301 |
| `anchor_a0350` | 0.819490 | 0.996270 | 0.487415 | 0.266094 | 0.622010 | 0.548241 |
| `anchor_a0500` | 0.819498 | 0.997195 | 0.487358 | 0.266137 | 0.621986 | 0.548288 |

M635-D does not beat M635-A on the raw generated compiler and does not pass the
Recall guard with anchors.  It is not a shared15 candidate.

## Decision

Do not expand M635-A to shared15.

Do not expand M635-B/C/D variants to shared15 either.

The current active-locked one-parameter compiler is valuable as a target-fit
confirmation, but it is not a retrieval-improving candidate.  It preserves the
main dense-derived structure and improves teacher-fit metrics, yet the qrels
smoke still loses Recall@100 by `0.000179` versus dense root and by `0.000098`
versus the deterministic M549 target.  Simple dense-root anchoring narrows the
gap but does not close it.

This is a useful bounded failure rather than a route-dead failure:

- The compiler can learn the dense-derived posting target.
- The generated-posting path works and produces M603-style qrels rows.
- The failure is not a frozen-row scorer artifact.
- The failure is that the selected teacher target is too equivalent to dense
  root and does not create positive retrieval movement.
- The anchor diagnostic shows that preserving more dense-root mass is not
  enough by itself; the next target needs a structured head/tail objective, not
  just interpolation.
- The M635-D diagnostic shows that a small structured dense-only compiler is
  still not enough; the missing signal is not merely rank-bucket mass shape.

## Next M635 Step

Do not continue the current M635 dense-only target-fit family as the main path.

Recommended next stage:

1. Keep frozen dense root and generated posting output.
2. Keep active top128 support floor and support cosine/KL gates.
3. Add a retrieval-constrained generated-posting objective with dense
   faithfulness as a guard, rather than fitting the same dense-derived target
   harder.
4. Keep BM25/fusion out until generated-posting smoke becomes non-negative.
5. Expand only if Recall@100 becomes non-negative while NDCG/MRR remain within
   the existing guard.

Do not return to M632-M634-style frozen-row scorer grids unless generated
posting smoke first passes.
