# M600 Output-Head / Posting-Compiler Report

M600 tests a narrow first-stage route:

- frozen dense rows as the root representation;
- trainable output head / posting compiler only;
- no BM25;
- no qrels in training;
- no frozen LLM judge;
- qrels only for held-out retrieval evaluation.

The purpose is to verify whether an output head can produce dense-derived
posting behavior without damaging dense-faithful retrieval membership.

## Stop Gate

The route is allowed to expand only if the learned head passes all constraints:

- validation active recall is not degraded;
- validation support loss stays under the floor;
- candidate-set KL improves on enough tasks;
- retrieval Recall@100 and MRR@20 do not regress beyond tolerance;
- dense Overlap@100 does not materially regress.

The dense overlap condition is mandatory.  Earlier M549/M551-style results
showed that KL or NDCG improvement alone can hide support/rank geometry damage.

## Runs

Remote host: `spark-1`

Run root:
`/home/huoju/leask/runs/ii42-m600-output-head-posting-compiler-v1`

Surface:
`/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`

Tasks:
`fiqa`, `scidocs`, `scifact`, `trec-covid`

### M600 Smoke

Path:
`/home/huoju/leask/runs/ii42-m600-output-head-posting-compiler-v1/smoke_600`

Config highlights:

- residual scale: `0.0125`;
- support weight: `1.0`;
- max support loss: `0.002`;
- min active recall: `0.999`;
- listwise weight: `1.0`;
- score weight: `0.05`.

Macro deltas versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned head | +0.000437 | -0.004468 | -0.000136 | -0.005935 | -0.013010 |

Training gate:

| Task | Epoch | dKL | Active | Support Cos |
| --- | ---: | ---: | ---: | ---: |
| fiqa | 1 | +0.035932 | 1.000000 | 0.998148 |
| scidocs | 0 | +0.000000 | 1.000000 | 1.000000 |
| scifact | 1 | +0.033293 | 1.000000 | 0.999485 |
| trec-covid | 4 | +0.015839 | 1.000000 | 0.999075 |

Decision: do not promote.  KL improved on 3/4 tasks and active membership was
preserved, but MRR and dense overlap regressed.

### M600 Strict Smoke

Path:
`/home/huoju/leask/runs/ii42-m600-output-head-posting-compiler-v1/smoke_600_strict`

Config highlights:

- residual scale: `0.005`;
- support weight: `2.0`;
- max support loss: `0.0005`;
- min active recall: `1.0`;
- listwise weight: `0.5`;
- score weight: `0.02`.

Macro deltas versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned head | +0.005747 | -0.001268 | -0.001858 | +0.001597 | -0.021230 |

Training gate:

| Task | Epoch | dKL | Active | Support Cos |
| --- | ---: | ---: | ---: | ---: |
| fiqa | 1 | +0.017828 | 1.000000 | 0.999623 |
| scidocs | 1 | +0.019061 | 1.000000 | 0.999510 |
| scifact | 1 | +0.013549 | 1.000000 | 0.999919 |
| trec-covid | 4 | +0.010590 | 1.000000 | 0.999858 |

Decision: do not promote.  The stricter head preserved active recall and support
cosine better, and improved NDCG/MRR, but it still damaged dense Overlap@100
and Recall@100.  This violates the first-stage dense-faithfulness contract.

## Interpretation

The positive signal is real but bounded:

- the output head can reduce candidate-set KL without qrels;
- active top-256 membership can be preserved under the validation objective;
- a very small residual can improve NDCG/MRR on this four-task surface.

The blocking signal is also clear:

- active recall and support cosine are insufficient guards;
- dense top-100 overlap can regress by 1-2 percentage points even when support
  cosine is above 0.9995;
- the current output-head objective is still changing rank geometry, not merely
  compiling dense output into an equivalent posting surface.

## Stop Decision

Stop M600 head-only expansion here.  Do not launch broad10 or broader matrix
from this head-only shape.

The route is not disproven as a long-term idea, but this fast validation says
the current loss/gate family is not stable enough.  The next attempt should not
be another small residual sweep.  It should redesign the objective around
rank-membership preservation directly:

- train against dense top-k membership/ordering as a hard constraint, not only
  support cosine;
- add an explicit dense-overlap or teacher-top-k preservation loss during
  training, not only as a post-hoc gate;
- keep candidate-set KL as a secondary term;
- keep BM25 and fusion out until this first-stage dense-faithful surface passes.

## Artifacts

Code:

- `scripts/research_sae_m600_output_head_posting_compiler.py`
- `scripts/run_m600_output_head_posting_compiler_spark.sh`

Remote results:

- `smoke_600/m600_smoke_600.json`
- `smoke_600/m600_smoke_600.md`
- `smoke_600_strict/m600_smoke_600_strict.json`
- `smoke_600_strict/m600_smoke_600_strict.md`
