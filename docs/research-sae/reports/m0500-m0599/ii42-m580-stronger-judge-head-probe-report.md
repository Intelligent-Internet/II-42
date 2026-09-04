# M580 Stronger Judge DREAM-Head Probe

M580 tests whether the DREAM attention-interface route becomes more credible
when the frozen judge is upgraded from `distilgpt2` to a small instruction
model.  It is a routing probe, not a retrieval-training result.

## Question

Prior evidence was mixed:

- M551/M555 showed that candidate-set/listwise posting training is useful but
  bounded.
- M562 showed that `distilgpt2` has retrieval-like heads under our prompt.
- M563 true score injection on `distilgpt2` moved gradients but did not improve
  retrieval, so the weak judge/interface may have been the bottleneck.

M580 asks a narrower question: does `Qwen/Qwen2.5-0.5B-Instruct` expose a
stronger, cross-task query-focused retrieval head under the same M562 probe?

## Run

Remote output:

```text
/home/huoju/leask/runs/ii42-m580-stronger-judge-head-probe-v1/qwen25_05b_probe_seed580/
```

Local copy:

```text
runs/m580_qwen25_05b_probe_seed580/
```

Config:

| Field | Value |
| --- | --- |
| LM | `Qwen/Qwen2.5-0.5B-Instruct` |
| Tasks | `FiQA2018,SCIDOCS,TRECCOVID` |
| Max probe groups/task | `8` |
| Teacher pool k | `8` |
| Random negatives | `8` |
| Max doc/query/target tokens | `40/24/48` |
| Device | `cuda` |
| Elapsed | `60.748s` |

## Result

The first probe, with `8` examples per task, passed the same continue gate used
by M562:

| Gate | Bar | Observed |
| --- | ---: | ---: |
| Task hits | `>= 2` | `3` |
| Mean NDCG@10 | `>= 0.40` | `0.979167` |
| Mean MRR | `>= 0.25` | `0.972222` |

Best cross-task head:

| Layer | Head | NDCG@10 | MRR | Top1 | Mean Rank | Task Hits |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 23 | 6 | 0.979167 | 0.972222 | 0.958333 | 0.083333 | 3 |

This is much stronger than the previous `distilgpt2` M562 global head:

| Probe | Layer | Head | NDCG@10 | MRR | Top1 | Mean Rank | Task Hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M562 `distilgpt2` | 4 | 1 | 0.724421 | 0.659882 | 0.527778 | 2.111111 | 3 |
| M580 `Qwen2.5-0.5B` | 23 | 6 | 0.979167 | 0.972222 | 0.958333 | 0.083333 | 3 |

Two larger probes were then run to check whether the result was a small-sample
artifact:

| Probe | Max groups/task | Best cross-task head | NDCG@10 | MRR | Top1 | Mean Rank | Task Hits |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| M580 g8 | 8 | 23:6 | 0.979167 | 0.972222 | 0.958333 | 0.083333 | 3 |
| M580 g24 | 24 | 19:6 | 0.925126 | 0.899306 | 0.819444 | 0.250000 | 3 |
| M580 g48 | 48 | 19:6 | 0.911056 | 0.886885 | 0.812500 | 0.472222 | 3 |
| M562 `distilgpt2` | 24 | 4:1 | 0.724421 | 0.659882 | 0.527778 | 2.111111 | 3 |

The exact best head moved from `23:6` to `19:6`, but the stable signal stayed
inside the late-layer head-6 family.  On the larger `g48` check, both `19:6`
and `23:6` had `task_hits=3`.

## Qwen Injection Smoke

The M563 score-injection code was extended with a Qwen2 eager-attention patch.
The previous GPT-2 patch path remains unchanged.

Smoke run:

```text
runs/m580_qwen25_05b_inject_smoke2_seed580/
```

Config:

| Field | Value |
| --- | --- |
| Task | `SCIDOCS` |
| Query limit | `128` |
| Max train groups | `4` |
| Epochs | `1` |
| Selected heads | `19:6,23:6` |

The run completed forward/backward/evaluation successfully:

| Metric | Value |
| --- | ---: |
| Best epoch | 1 |
| Active recall | 1.000000 |
| Initial total loss | 1.130440 |
| Validation total loss | 1.104156 |
| Validation LM loss | 1.067550 |

Heldout retrieval was slightly below the same-run dense-topk sparse source:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.21327 | 0.15598 | 0.54150 | 0.37596 | 0.56118 |
| Qwen injection smoke | 0.21127 | 0.15588 | 0.52614 | 0.37146 | 0.55961 |

This smoke is not a performance conclusion because it used only four training
examples.  Its purpose was to prove that the Qwen attention-injection path is
mechanically viable.

## Three-Task Qwen Injection Canary

The first retrieval-bearing canary used the same Qwen patch and stable
late-layer head-6 family:

```text
runs/m580_qwen25_05b_inject_canary_seed580/
```

Config:

| Field | Value |
| --- | --- |
| Tasks | `FiQA2018,SCIDOCS,TRECCOVID` |
| Max train groups/task | `16` |
| Epochs | `2` |
| Selected heads | `19:6,23:6` |
| Query limit | `0` |
| Elapsed | `102.905s` |

Macro result:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact_dense_teacher | 0.49921 | 0.24160 | 0.48923 | 0.63612 | 1.00000 |
| dense_topk128_sparse | 0.44511 | 0.20505 | 0.43409 | 0.59854 | 0.53500 |
| Qwen injection canary | 0.44769 | 0.20651 | 0.43365 | 0.60003 | 0.53741 |

Delta versus same-run dense-topk:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.00258 |
| MAP@100 | +0.00146 |
| Recall@100 | -0.00044 |
| MRR@20 | +0.00149 |
| O@100 | +0.00241 |

Per-task delta versus same-run dense-topk:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| FiQA2018 | +0.00033 | +0.00348 | -0.00078 | +0.00762 |
| SCIDOCS | -0.00199 | -0.00031 | -0.00150 | -0.00029 |
| TRECCOVID | +0.00940 | +0.00121 | +0.00096 | -0.00286 |

Training behavior:

- Active recall stayed at `1.00000` for all tasks.
- Total validation loss dropped on all tasks.
- LM loss itself barely moved, so the current gain is still mostly mediated by
  the listwise/support path rather than a clearly stronger frozen-judge signal.

This is the first Qwen-DREAM result that shows an actual retrieval metric
improvement over the same-run dense-topk sparse source, but it is still a small
canary.  The correct next test is a larger `64 groups / 3 epochs` canary before
any broad claim.

## Larger Canary And Ablation

Two `64 groups / 3 epochs` Qwen-injection runs were executed:

| Run | LM loss weight | Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| g64/e3 | 1.0 | 580 | +0.00568 | +0.00186 | +0.00155 | +0.00517 | +0.00305 |
| g64/e3 | 1.0 | 581 | +0.00097 | -0.00169 | -0.00222 | +0.00022 | +0.00155 |

Mean over these two seeds:

| Metric | Mean delta |
| --- | ---: |
| NDCG@10 | +0.00333 |
| MAP@100 | +0.00009 |
| Recall@100 | -0.00034 |
| MRR@20 | +0.00270 |
| O@100 | +0.00230 |

This says the retrieval signal is real but weak and unstable by metric:
NDCG/MRR/overlap are consistently positive; MAP/Recall are not.

A control run then disabled the frozen-LM objective while keeping the same
candidate sets, listwise anchor, support loss, model shape, and selected heads:

| Run | LM loss weight | Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| g64/e3 | 1.0 | 580 | +0.00568 | +0.00186 | +0.00155 | +0.00517 | +0.00305 |
| g64/e3 lm0 | 0.0 | 580 | +0.00568 | +0.00193 | +0.00164 | +0.00517 | +0.00320 |

The LM-loss ablation is effectively identical to the Qwen-injection run.
Therefore the current retrieval gain should not be credited to DREAM's
frozen-judge next-token loss.  It is coming from the existing listwise/support
training surface under the shared candidate-set setup.

## Interpretation

The paper helped, but not in the strongest form we hoped for.

Positive signal:

- The stronger judge exposes a clear retrieval-like attention head across all
  three probe tasks.
- The result supports the paper's claim that the interface matters: the
  training signal should enter query-focused retrieval heads, not a generic
  external likelihood proxy.

Negative signal:

- The Qwen score-injection objective did not add measurable value over the
  `LM_LOSS_WEIGHT=0` control.
- The retrieval improvement from the canary is better explained by the
  existing M551/M555 listwise/support training surface.

Remaining limitations:

- The pseudo-positive is dense top-1, not qrels.
- The Qwen injection implementation is mechanically viable, but its loss is
  not yet functionally useful.
- Any future DREAM variant must beat its own `LM_LOSS_WEIGHT=0` control before
  being compared against M551/M555.

## Decision

Do not continue scaling the current Qwen LM-loss injection as-is.

The paper is still useful conceptually:

- It correctly emphasized candidate-set competition.
- It correctly emphasized that the training interface matters.
- It motivated a better retrieval-head diagnostic, and Qwen has much stronger
  retrieval-like heads than `distilgpt2`.

But the current implementation does not yet extract extra supervision from the
frozen judge.  Since `LM_LOSS_WEIGHT=0` matches `LM_LOSS_WEIGHT=1`, the next
research move should return to the M551/M555 baseline and treat the DREAM
components as diagnostics or candidate-set construction tools unless a stronger
injection objective is designed.

Recommended next step:

1. Preserve the Qwen2 patch as a working mechanism, but do not promote it.
2. Rebase the next encoder/posting run on the validated M551/M555 listwise
   baseline.
3. If DREAM is revisited, redesign the loss so the frozen judge contribution is
   measurable against an `LM_LOSS_WEIGHT=0` control.

Stop condition:

- Stop the current Qwen LM-loss injection route for now, because the ablation
  shows no incremental value from the LM objective.
