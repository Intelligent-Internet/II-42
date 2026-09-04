# M579 Larger Adapter Posting Encoder

M579 returns to the larger encoder/posting route after M577/M578 showed that
small multiplicative coordinate-gain heads are too weak.

The goal is to test whether a larger locked-support residual adapter can
improve M551 without adding another proxy objective.

## Baselines

Same-task seed552 surface:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

Reference rows from the existing M551/M555 reports:

| Model | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 `resid025` | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 `pairwise_w0.20_h768_s0125` | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |

M551 is still the stable promoted baseline.  M555 is the current weak-positive
M551-family candidate, but its broad10 three-seed margin is too small for final
promotion.

## Runs

Both runs completed on `spark-1`.

Common config:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
VARIANTS=shared_locked_support_residual
HIDDEN_DIMS=1024
RESIDUAL_SCALE=0.0125
TEACHER_TEMPERATURE=0.025
STUDENT_TEMPERATURE=0.050
EPOCHS=4
MAX_TRAIN_GROUPS=512
TEACHER_POOL_K=128
RANDOM_NEGATIVES=128
SEED=552
```

Run paths:

```text
runs/m579_kl_h1024_s0125_seed552/
runs/m579_pairwise_h1024_s0125_seed552/
```

## Results

### Pure M551 KL, h1024

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M579 `kl_h1024_s0125` | 0.44138 | 0.20215 | 0.43946 | 0.58099 | 0.54939 |

Delta against same-run `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| ---: | ---: | ---: | ---: | ---: |
| -0.00076 | -0.00043 | +0.00352 | -0.00083 | +0.00518 |

Delta against report baselines:

| Baseline | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | -0.00508 | +0.00044 | +0.00255 | -0.00842 | +0.00116 |
| M555 h768 pairwise | -0.00468 | -0.00096 | +0.00168 | -0.01000 | +0.00022 |

Training selected epochs:

| Task | Selected epoch | Active recall | Support losses |
| --- | ---: | --- | --- |
| FiQA2018 | 3 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000789, 0.002062, 0.005244, 0.008717]` |
| SCIDOCS | 2 | `[1.0, 1.0, 1.0, 1.0]` | `[0.001446, 0.004086, 0.009834, 0.014461]` |
| TRECCOVID | 4 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000105, 0.000389, 0.000810, 0.001411]` |

Interpretation: pure extra capacity increases Recall/Overlap, but hurts NDCG
and MRR badly versus M551/M555.  Capacity alone is not the missing signal.

### Pairwise M555 objective, h1024

Config delta:

```text
PAIRWISE_WEIGHT=0.20
PAIRWISE_WINNERS=24
PAIRWISE_SCALE=10.0
```

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M579 `pairwise_h1024_s0125` | 0.44502 | 0.20308 | 0.43944 | 0.58987 | 0.54932 |

Delta against same-run `dense_topk128_sparse`:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| ---: | ---: | ---: | ---: | ---: |
| +0.00288 | +0.00050 | +0.00350 | +0.00805 | +0.00511 |

Delta against report baselines:

| Baseline | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | -0.00144 | +0.00137 | +0.00253 | +0.00046 | +0.00109 |
| M555 h768 pairwise | -0.00104 | -0.00003 | +0.00166 | -0.00112 | +0.00015 |

Training selected epochs:

| Task | Selected epoch | Active recall | Support losses |
| --- | ---: | --- | --- |
| FiQA2018 | 3 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000822, 0.002116, 0.005384, 0.008797]` |
| SCIDOCS | 2 | `[1.0, 1.0, 1.0, 1.0]` | `[0.001465, 0.004125, 0.009887, 0.014499]` |
| TRECCOVID | 4 | `[1.0, 1.0, 1.0, 1.0]` | `[0.000105, 0.000388, 0.000809, 0.001409]` |

Interpretation: adding the M555 pairwise signal rescues much of the h1024
damage, but not enough.  It improves MAP/Recall/MRR/Overlap over M551 on this
seed, but still loses NDCG.  Against M555 h768, it trades more Recall for worse
NDCG and MRR.

## Decision

Do not scale M579 h1024 to broad10 or more seeds.

This is not a dead-end for the full M551-family route, but it is a stop signal
for the simple "increase adapter width" branch:

- pure h1024 KL oversteers toward Recall/Overlap and damages early rank;
- h1024 plus pairwise is competitive but weaker than M555 h768 on NDCG/MRR;
- the already known M555 h768_s0125 remains the best weak-positive
  M551-family candidate.

The next useful step should not be another width sweep.  Either keep M555 as
the ceiling for this residual-adapter family and move to a materially closer
judge/interface signal, or redesign the model around a richer trainable
text-to-posting encoder with a separate teacher-fit phase.
