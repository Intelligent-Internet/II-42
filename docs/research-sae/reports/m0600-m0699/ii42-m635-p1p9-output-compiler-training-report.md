# M635 / P1.9 Output-Compiler Training Report

## Status

Status: `teacher_fit_smoke_completed_retrieval_gate_failed`

M635 smoke trained output compilers on `spark-1` with CUDA.  These are
generated-posting compiler smokes, not frozen-row rerankers and not BM25 or
qrels-facing scorers.

## Run

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Session | `ii42_m635_p1p9_smoke` |
| Runner | `scripts/run_m635_unified_output_compiler_smoke_spark.sh` |
| Source script | `scripts/research_sae_m549u_official_root_compiler.py` |
| Run root | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_output_compiler_smoke_seed6351/` |
| Log | `runs/ii42-m635-p1p9-unified-output-compiler-v1/logs/m635_p1p9_smoke.log` |
| JSON | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_output_compiler_smoke_seed6351/m635_p1p9_output_compiler_smoke_seed6351.json` |
| Checkpoint | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_output_compiler_smoke_seed6351/m635_p1p9_output_compiler_smoke_seed6351.compiler.pt` |

## Configuration

| Field | Value |
| --- | ---: |
| Compiler mode | `active_locked_tail_power` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Train rows | 32000 |
| Trainable parameters | 1 |
| Active dims | 128 |
| Support prefix | 512 |
| Tail identity dims | 768 |
| Max train steps | 400 |
| Device | `cuda` |
| Init gamma | 1.000000000 |
| Learned gamma | 1.035719276 |
| M549 target gamma | 1.025625270 |

## Teacher-Fit Metrics

| Metric | Initial | Final | Delta |
| --- | ---: | ---: | ---: |
| doc active recall | 0.999990 | 0.999990 | +0.000000 |
| query active recall | 1.000000 | 1.000000 | +0.000000 |
| doc support cosine | 0.999941 | 0.999992 | +0.000051 |
| query support cosine | 0.999940 | 0.999992 | +0.000053 |
| doc support KL | 0.008260 | 0.008002 | -0.000257 |
| query support KL | 0.011997 | 0.011632 | -0.000365 |
| score Pearson vs M549 | 0.999982 | 0.999998 | +0.000016 |
| top10 overlap vs M549 | 0.995570 | 0.998328 | +0.002758 |
| top50 overlap vs M549 | 0.995367 | 0.998128 | +0.002761 |
| top100 overlap vs M549 | 0.995921 | 0.998250 | +0.002329 |
| top100 overlap vs dense | 0.995921 | 0.995921 | +0.000000 |

## Interpretation

The smoke validates the first part of M635: the audited M549U target is
learnable by an output-only compiler while preserving hard active membership.
Unlike M632-M634, the observed improvement does not come from reranking frozen
rows and does not trade Recall-like movement for dense-overlap damage.

This completes the teacher-fit portion of M635-A.  A follow-up retrieval smoke
was then run with the generated compiler checkpoint and is reported in
`docs/research-sae/reports/m0600-m0699/ii42-m635-p1p9-smoke-gate-report.md`.

## M635-D Structured Compiler Control

M635-D then tested whether a slightly richer generated compiler can improve
the smoke result without leaving the dense-root, BM25-free boundary.

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Session | `ii42_m635_p1p9_structured_smoke` |
| Compiler mode | `active_locked_bucket_scale` |
| Run root | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_p1p9_structured_compiler_smoke_seed6353/` |
| Retrieval smoke root | `runs/ii42-m635-p1p9-unified-output-compiler-v1/m635_retrieval_smoke_structured_seed6353/` |
| Trainable parameters | `9` |
| Best epoch | `3` |
| Best global step | `375` |
| Learned gamma | `1.0317615271` |

M635-D adds rank-bucket signed mass calibration plus root-head and
dense-tail/false-head regularizers.  Teacher-fit still improves:

| Metric | Initial | Final | Delta |
| --- | ---: | ---: | ---: |
| doc active recall | 0.999988 | 0.999988 | +0.000000 |
| query active recall | 1.000000 | 1.000000 | +0.000000 |
| doc support cosine | 0.999697 | 0.999749 | +0.000051 |
| query support cosine | 0.999939 | 0.999992 | +0.000053 |
| doc support KL | 0.008400 | 0.008145 | -0.000255 |
| query support KL | 0.011950 | 0.011587 | -0.000363 |
| score Pearson vs M549 | 0.999982 | 0.999998 | +0.000016 |
| top100 overlap vs M549 | 0.995259 | 0.998359 | +0.003100 |
| top100 overlap vs dense | 0.995259 | 0.995259 | +0.000000 |

The retrieval smoke did not improve over M635-A, so M635-D is also not a
shared15 candidate.

## Next Required Step

Do not expand M635-A/B/C/D to shared15.  The teacher-fit question is answered:
the dense-derived posting target is learnable by a small output compiler.  The
retrieval question is not solved: richer dense-only compiler structure still
does not produce non-negative Recall@100 movement.

The next useful work is outside this exact M635 target-fit family: a
retrieval-constrained generated posting objective with dense-faithfulness
guards, not another frozen-row scorer or another one-parameter dense target fit.
