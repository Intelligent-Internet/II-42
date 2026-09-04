# M581 M551 Small-Pool Listwise Route

M581 returns to the M551/M555 encoder/posting route after M580 showed that the
current DREAM-style LM objective adds no measurable value over
`LM_LOSS_WEIGHT=0`.

The goal is to isolate the useful part of the M580 result: a smaller
candidate-set training surface over the locked-support residual encoder,
without loading a frozen LLM or using attention injection.

## Setup

Code:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m551_dream_lite_posting_spark.sh
```

Run root:

```text
/home/huoju/leask/runs/ii42-m581-m551-smallpool-v1
```

Local copies:

```text
runs/m581_m551_smallpool_g64e3_score0_seed580/
runs/m581_m551_smallpool_g64e3_score0_seed581/
runs/m581_m551_smallpool_g64e3_score0_seed582/
```

Shared config:

| Field | Value |
| --- | --- |
| Tasks | `FiQA2018,SCIDOCS,TRECCOVID` |
| Variant | `shared_locked_support_residual` |
| Teacher pool / random negatives | `8 / 8` |
| Max train groups | `64` |
| Epochs | `3` |
| Hidden dims | `768` |
| Residual scale | `0.0125` |
| Teacher / student temperature | `0.025 / 0.050` |
| Score weight | `0.0` |
| Support weight | `0.5` |

## Results

Rows are deltas versus the same-run `dense_topk128_sparse` source.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 580 | +0.00736 | +0.00133 | -0.00007 | +0.00108 | +0.00143 |
| 581 | +0.00265 | -0.00121 | -0.00145 | +0.00069 | -0.00034 |
| 582 | +0.00118 | -0.00093 | -0.00011 | +0.00022 | +0.00003 |
| Mean | +0.00373 | -0.00027 | -0.00054 | +0.00066 | +0.00037 |

Per-task deltas:

| Seed | Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| ---: | --- | ---: | ---: | ---: | ---: |
| 580 | FiQA2018 | +0.00097 | +0.00346 | -0.00039 | +0.00488 |
| 580 | SCIDOCS | -0.00097 | +0.00010 | +0.00050 | +0.00124 |
| 580 | TRECCOVID | +0.02206 | +0.00044 | -0.00033 | -0.00286 |
| 581 | FiQA2018 | -0.00401 | -0.00241 | -0.00009 | -0.00829 |
| 581 | SCIDOCS | -0.00017 | -0.00008 | -0.00363 | +0.00203 |
| 581 | TRECCOVID | +0.01213 | -0.00114 | -0.00062 | +0.00833 |
| 582 | FiQA2018 | -0.00223 | -0.00148 | +0.00187 | -0.00290 |
| 582 | SCIDOCS | -0.00381 | -0.00145 | -0.00171 | -0.00190 |
| 582 | TRECCOVID | +0.00960 | +0.00014 | -0.00047 | +0.00547 |

## Interpretation

M581 is not promotion-ready, but it is useful:

- NDCG@10 is positive on all three seeds.
- MRR@20 is positive on all three seeds, but the margin is small.
- MAP@100 and Recall@100 regress on the three-seed mean.
- TRECCOVID drives most of the NDCG gain, while FiQA/SCIDOCS are unstable.

This confirms that the useful part of M580 was not the frozen Qwen LM loss.
The same general gain appears in the pure M551 runner, without Qwen or
attention injection.

## Decision

Do not promote M581 `8/8` as-is.  The small candidate pool overfocuses on
top-rank behavior and does not protect MAP/Recall.

Next step: test a middle candidate pool, starting with `16/16`, under the same
M551 runner and seed surface.  The required improvement is:

- keep the three-seed positive NDCG direction;
- remove the MAP/Recall regression;
- avoid relying on BM25 or qrels during training.

## Follow-Up Repairs

These follow-ups test whether the useful M581 signal can be made safer before
returning to larger encoder/posting training.

Rows are seed-580 deltas versus the same-run `dense_topk128_sparse` source.

| Run | Change | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| M581 | pool `8/8`, groups `64`, score `0.0` | +0.00736 | +0.00133 | -0.00007 | +0.00108 | +0.00143 |
| M582 | pool `16/16`, groups `64`, score `0.0` | +0.00739 | +0.00138 | -0.00024 | +0.00096 | +0.00124 |
| M583 | pool `8/8`, groups `64`, score `0.05` | +0.00736 | +0.00133 | -0.00007 | +0.00109 | +0.00141 |
| M584 | pool `8/8`, groups `512`, score `0.0` | +0.00711 | +0.00105 | +0.00076 | -0.00020 | +0.00145 |
| M585 | M584 plus pairwise `0.20` | +0.00713 | +0.00111 | +0.00109 | -0.00026 | +0.00137 |

Per-task seed-580 deltas for the expanded-group repairs:

| Run | Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| M584 | FiQA2018 | +0.00097 | +0.00346 | -0.00039 | +0.00488 |
| M584 | SCIDOCS | -0.00172 | -0.00073 | +0.00300 | -0.00261 |
| M584 | TRECCOVID | +0.02206 | +0.00044 | -0.00033 | -0.00286 |
| M585 | FiQA2018 | +0.00097 | +0.00346 | -0.00039 | +0.00488 |
| M585 | SCIDOCS | -0.00164 | -0.00056 | +0.00400 | -0.00278 |
| M585 | TRECCOVID | +0.02206 | +0.00044 | -0.00033 | -0.00286 |

## Follow-Up Interpretation

The DREAM paper remains useful as a training-signal design reference, but the
current frozen-LM injection implementation is not the active source of gains.
M580 showed that `LM_LOSS_WEIGHT=0` matches or slightly beats the Qwen
LM-loss run.  M581 then reproduced the useful direction in the pure M551
runner, without Qwen or attention injection.

The positive signal is therefore narrower:

- candidate-set competition is useful;
- locked-support residual updates can improve NDCG without destroying dense
  overlap;
- more train groups improve Recall, but trade away MRR;
- the current score-MSE and pairwise repairs do not solve the full metric
  balance.

Do not keep deepening the expensive frozen-LM path unless a new probe shows
that the judge loss adds value beyond the pure listwise/support runner.  The
next worthwhile work is a larger encoder/posting training surface that keeps
the DREAM-inspired parts that worked: small candidate-set competition,
support-preserving residuals, and explicit multi-metric gates.
