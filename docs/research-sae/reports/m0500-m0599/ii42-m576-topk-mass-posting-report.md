# M576 Top-k Mass Posting Encoder

M576 returns from selector-gate tuning to the encoder/training signal.  M574
showed that selector utility gates are not reliable enough to promote.  The next
question is whether the encoder can be trained to preserve dense-teacher
candidate-set support more directly.

## Hypothesis

M551's candidate-set KL fits the full dense-teacher distribution.  M555's
pairwise loss improves ordering weakly but is seed-sensitive.  M558's top1 loss
protects the single teacher winner but is too narrow.

M576 adds a middle-ground qrels-free objective:

```text
teacher_topk = topk(candidate_teacher_scores, k)
student = softmax(posting_scores / student_temperature)
loss = -log(sum(student[teacher_topk]))
```

This asks the posting encoder to keep probability mass inside the dense
teacher's top-k candidate set, without optimizing qrels, BM25, or a single
winner.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m551_dream_lite_posting_spark.sh
```

New arguments:

- `--topk-mass-weight`
- `--topk-mass-k`
- `--selection-topk-mass-weight`

Defaults are `0.0`, so M551-M575 behavior is unchanged unless the new objective
is enabled.

## Smoke

Smoke completed on `spark-1`:

```text
runs/m576_smoke_scidocs_seed576/
```

Config:

```text
TASKS=SCIDOCS
QUERY_LIMIT=120
DOC_LIMIT=2000
EPOCHS=1
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
TOPK_MASS_WEIGHT=0.20
TOPK_MASS_K=16
SELECTION_TOPK_MASS_WEIGHT=0.20
```

Delta against `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| +0.000550 | +0.000830 | +0.000000 | +0.000970 | -0.008500 |

The smoke proved the objective runs and can improve ranking metrics on a tiny
surface, but the overlap drop made it unsafe to scale directly.

## Three-task Seed552

Surface:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

Fixed settings:

```text
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
TOPK_MASS_K=16
```

M551/M555 baselines are from the same seed552 three-task surface documented in
the earlier reports.

| Model | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 `pairwise_w0.20_h768_s0125` | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M576 `topk_mass_w0.20` | 0.44282 | 0.20179 | 0.43633 | 0.57927 | 0.55111 |
| M576 `topk_mass_w0.05` | 0.44295 | 0.20241 | 0.43780 | 0.58011 | 0.55045 |
| M576 `topk_mass_w0.01` | 0.44283 | 0.20237 | 0.43649 | 0.58020 | 0.55047 |

Delta against each run's `dense_topk128_sparse`:

| Variant | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 | Epochs |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `w0.20` | +0.000680 | -0.000790 | +0.000390 | -0.002550 | +0.006900 | `[3, 2, 4]` |
| `w0.05` | +0.000810 | -0.000170 | +0.001860 | -0.001710 | +0.006240 | `[3, 2, 4]` |
| `w0.01` | +0.000690 | -0.000210 | +0.000550 | -0.001620 | +0.006260 | `[3, 2, 4]` |

## Decision

Stop M576 as a promotion route.

Top-k mass reliably increases dense overlap and sometimes Recall, but it
damages early-rank behavior.  Even at `TOPK_MASS_WEIGHT=0.01`, MRR stays far
below M551/M555 on the same seed552 three-task surface.  This is the same
failure pattern seen in several proxy objectives: the model can be made more
dense-support faithful, but that does not automatically preserve the top-rank
utility needed for retrieval.

The useful conclusion is negative but sharp: the next encoder-stage attempt
should not add another candidate-set mass proxy.  It should either:

- improve the model form while keeping the proven M551 sharp KL objective, or
- introduce a closer frozen-judge interface that supplies top-rank utility
  rather than only dense-score support.
