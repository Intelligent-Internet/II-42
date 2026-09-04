# II-42 M401 Structural Dense-Tail Distillation Report

Date: 2026-06-27

## Goal

M401 tests whether the M392-M396 structural dense-tail route can be
distilled into a learned encoder:

- input: existing dense embedding, as a proxy before training a text encoder;
- target: signed active coordinates plus joint-PCA tail sketch;
- retrieval route: learned structural posting score, optionally blended with
  BM25 after retrieval;
- constraint: do not optimize to a dataset label surface directly.  The
  training signal is dense-teacher preservation.

This is deliberately separate from M400.  M400 tested a shallow neural feature
ranker over retrieved candidates and was negative.  M401 tests whether the
representation itself can be learned.

## Run Surfaces

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m401-structural-dense-tail-distillation-v1`

Local code:
`scripts/research_sae_m401_structural_dense_tail_distillation.py`

Local plan:
`docs/research-sae/reports/m0400-m0499/ii42-m401-structural-dense-tail-distillation-plan.md`

Execution was CPU-only and low priority:

```bash
CUDA_VISIBLE_DEVICES= OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 \
OPENBLAS_NUM_THREADS=4 nice -n 10 ionice -c2 -n7 ...
```

This avoided interfering with the active `spark-2` GPU training.

## FiQA Canary Matrix

Task: `FiQA2018`

Split: held-out query canary, seed `401`.

Metric shown first is `NDCG@10`.

| Run | Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Touch ratio | Dense O@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact | dense teacher | 0.47992 | 0.81322 | 0.55721 | 0.42705 | 1.00000 | 1.00000 |
| deterministic | structural tail | 0.39532 | 0.58693 | 0.48698 | 0.34205 | 0.08002 | 0.54049 |
| deterministic | structural tail + BM25 a0.10 | 0.46894 | 0.79483 | 0.54464 | 0.41403 | 0.14511 | 0.80244 |
| deterministic | structural tail + BM25 a0.18 | 0.47513 | 0.79508 | 0.55411 | 0.41887 | 0.14511 | 0.74793 |
| sanity 8 groups, e1 | learned linear | 0.44536 | 0.76091 | 0.52503 | 0.39286 | 0.08002 | 0.66670 |
| sanity 8 groups, e1 | learned linear + BM25 a0.10 | 0.46433 | 0.77753 | 0.53805 | 0.40478 | 0.14040 | 0.68216 |
| all groups, e1 | learned linear | 0.29225 | 0.65984 | 0.34611 | 0.24466 | 0.08002 | 0.50978 |
| all groups, e2 | learned linear | 0.23676 | 0.63017 | 0.27554 | 0.19082 | 0.08002 | 0.49173 |
| all groups, e10 | learned linear | 0.30793 | 0.68006 | 0.35987 | 0.25948 | 0.08002 | 0.60034 |
| all groups, e10 | learned linear + BM25 a0.18 | 0.37922 | 0.73264 | 0.44406 | 0.32581 | 0.14013 | 0.60843 |
| all groups, e10 | learned MLP | 0.09822 | 0.24099 | 0.12921 | 0.07858 | 0.08002 | 0.15000 |
| all groups, e10 | learned MLP + BM25 a0.18 | 0.28998 | 0.59462 | 0.35781 | 0.24616 | 0.14892 | 0.33093 |

Raw outputs:

- sanity:
  `/home/huoju/leask/runs/ii42-m401-structural-dense-tail-distillation-v1/sanity/m401_sanity.json`
- full 10 epoch:
  `/home/huoju/leask/runs/ii42-m401-structural-dense-tail-distillation-v1/fiqa_seed401/m401_structural_dense_tail_fiqa_seed401.json`
- linear epoch 1:
  `/home/huoju/leask/runs/ii42-m401-structural-dense-tail-distillation-v1/linear_e1/m401_linear_e1.json`
- linear epoch 2:
  `/home/huoju/leask/runs/ii42-m401-structural-dense-tail-distillation-v1/linear_e2/m401_linear_e2.json`

## Interpretation

The deterministic structural route is strong only after BM25 blending:

- structural tail alone: `0.39532`;
- structural tail + BM25 a0.18: `0.47513`;
- exact dense: `0.47992`.

That is important because it says the M392-M396 structural shape still contains
most of the dense ranking signal when a lexical/admission surface helps the
candidate set.  The remaining gap to dense is small on this FiQA canary.

The learned encoder, however, does not currently preserve that signal:

- all-query learned linear e1 is already below teacher structural tail;
- e2 gets worse, so the issue is not just 10-epoch overtraining;
- MLP collapses badly, despite having more capacity.

The 8-group sanity result (`0.44536`) should not be treated as a valid win.
It proves the implementation path can learn something from a narrow query
subset, but it does not generalize when the train query set is expanded.

## Conclusion

M401 as implemented is negative for the learned encoder.

The useful finding is more specific:

1. The structural dense-tail target is still valuable.
2. The current global reconstruction/listwise/pairwise loss is not the right
   loss for a text-to-posting encoder.
3. Scaling the same loss with a deeper MLP is actively harmful.
4. The next model should be trained to preserve per-query dense rank geometry
   and candidate admission directly, not to average-match structural
   coordinates globally.

## Recommended Next Step

Do not scale this M401 loss to BEIR15 or MTEB.

The next version should be M402:

- query-conditioned text-to-posting encoder;
- explicit dense top-k preservation objective;
- per-query pair/list loss over dense teacher candidates;
- separate document admission head and ranking head;
- optional BM25-aware training as a second branch, not the primary target;
- validation gate: learned route must beat deterministic structural tail
  alone before any BM25 blend is considered.

The stop rule should be strict:

- if learned-only cannot exceed deterministic structural tail on FiQA, do not
  spend full BEIR15 compute;
- if learned-only beats structural tail but only BM25 blend improves, treat the
  model as an admission helper, not a dense-faithful encoder.
