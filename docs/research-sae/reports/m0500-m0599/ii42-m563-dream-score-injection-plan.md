# M563 DREAM Score-Injection Canary Plan

M563 is the first true DREAM-style canary for the encoder/posting route.  It is
only justified because M562 found retrieval-like query-focused heads in the
frozen judge.

## Baseline To Preserve

Current stable baseline:

```text
M555 pairwise_w0.20_h768_s0125_e4
```

M555 is only a weak positive over M551, but it is the best current
encoder/posting baseline:

| Metric | Three-seed broad10 M555 - M551 |
| --- | ---: |
| NDCG@10 | +0.00050 |
| MAP@100 | +0.00017 |
| Recall@100 | -0.00011 |
| MRR@20 | +0.00044 |
| Dense O@100 | +0.00044 |

M563 must not be promoted unless it beats or ties M555 on the same three-task
smoke first, then earns broad10 validation.

## M562 Gate Result

M562 used `distilgpt2` and found stable retrieval-like heads:

| Layer | Head | NDCG@10 | MRR | Top1 | Task Hits |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 1 | 0.724421 | 0.659882 | 0.527778 | 3 |
| 4 | 0 | 0.692669 | 0.639818 | 0.527778 | 3 |
| 3 | 7 | 0.659982 | 0.580859 | 0.416667 | 3 |
| 3 | 1 | 0.658096 | 0.590533 | 0.458333 | 3 |
| 4 | 2 | 0.656352 | 0.586472 | 0.444444 | 3 |

Use these heads for the first injection smoke:

```text
(4,1),(4,0),(3,7),(3,1),(4,2)
```

## Why M563 Is Different From M561

M561 did this:

```text
frozen LM -> per-candidate target likelihood -> teacher distribution
posting encoder -> imitate teacher distribution
```

The LM loss was outside the retriever computation.  That is likelihood
distillation, closer to RePlug than DREAM.

M563 must do this instead:

```text
posting encoder -> candidate scores -> softmax candidate weights
candidate weights -> selected LM attention heads
target-token cross entropy -> gradient through attention -> posting encoder
```

Only the posting encoder updates.  The LM remains frozen.

## Implementation Shape

Create:

```text
scripts/research_sae_m563_dream_score_injection.py
scripts/run_m563_dream_score_injection_spark.sh
```

Use the existing M551 encoder pieces:

- `DreamLitePostingEncoder`
- `dense_kl_loss`
- `dense_support_loss`
- `dense_active_recall`
- `collect_training_groups` candidate construction where useful

Use a new GPT-2 eager-attention patch:

1. Load `distilgpt2` with `attn_implementation='eager'`.
2. Freeze all LM parameters.
3. Monkeypatch `transformers.models.gpt2.modeling_gpt2.eager_attention_forward`.
4. During a forward pass, for selected `(layer, head)` and query-token rows:

```text
original attention over each candidate doc span
-> normalize token preference inside each doc
-> multiply by posting softmax candidate weight
-> zero non-candidate doc-spans in guided attention
-> mix original and guided attention with injection_gate
-> renormalize row
```

This matches the DREAM interface closely enough for a first canary:

- retriever controls document-level budget;
- frozen LM controls token preference within each document;
- target loss supplies gradient through candidate weights.

## First Smoke

Host: `spark-1`.

Use a deliberately small run:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
LM_MODEL=distilgpt2
SELECTED_HEADS=4:1,4:0,3:7,3:1,4:2
TEACHER_POOL_K=8
RANDOM_NEGATIVES=8
MAX_TRAIN_GROUPS=32
EPOCHS=2
BATCH_SIZE=1
INJECTION_GATE=0.35
LM_LOSS_WEIGHT=1.0
ANCHOR_LISTWISE_WEIGHT=0.25
SUPPORT_WEIGHT=0.5
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
```

Reasoning:

- `BATCH_SIZE=1` keeps prompt spans and attention patching simple.
- The listwise anchor prevents the LM gradient from moving the encoder away
  from dense-derived support immediately.
- The first smoke is allowed to be slow, but it should finish in minutes, not
  hours.

## Stop / Continue Gate

Stop M563 immediately if any of these happen:

- injection loss is not differentiable into posting encoder parameters;
- target loss decreases but support active recall drops below `0.995`;
- seed552 three-task retrieval is worse than M551 on both NDCG@10 and
  Recall@100;
- M563 cannot beat M555 on any of NDCG@10, Recall@100, or MRR@20 in the first
  smoke.

Continue to seeds `551/553` only if seed552 is at least competitive with M555:

| Metric | Required first-smoke behavior |
| --- | --- |
| NDCG@10 | tie or improve vs M555 |
| Recall@100 | no material loss vs M555 |
| MRR@20 | tie or improve vs M551 |
| Active recall | `>= 0.995` |

Only after a three-seed three-task positive should M563 receive broad10.

## Current Decision

Proceed to M563 implementation.  Do not run more M559-M561 proxy variants.

## First Smoke Result

Run:

```text
Host: spark-1
Run dir:
/home/huoju/leask/runs/ii42-m563-dream-score-injection-v1/distilgpt2_inject_seed563/
Local copy:
runs/m563_distilgpt2_inject_seed563/
```

The first true-injection smoke completed.  It proved the mechanism is live:

- all three tasks kept `active_recall=1.0`;
- validation total loss decreased on every task;
- gradient norm was non-zero on every task;
- target LM loss decreased slightly on every task.

Training diagnostics:

| Task | Initial Total | Final Total | Initial LM | Final LM | Active |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 1.489108 | 1.417039 | 1.402754 | 1.402475 | 1.000000 |
| SCIDOCS | 1.643057 | 1.543704 | 1.523507 | 1.522976 | 1.000000 |
| TRECCOVID | 1.199122 | 1.146787 | 1.138651 | 1.137821 | 1.000000 |

Retrieval macro on the same split:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.44821 | 0.20047 | 0.43368 | 0.58929 | 0.54137 |
| M563 first smoke | 0.44692 | 0.19959 | 0.43409 | 0.57517 | 0.54262 |

Deltas versus dense topk:

| dNDCG | dMAP | dR | dMRR | dO |
| ---: | ---: | ---: | ---: | ---: |
| -0.00129 | -0.00088 | +0.00041 | -0.01412 | +0.00125 |

Interpretation: true score injection works mechanically but is not yet a
retrieval improvement.  It mainly hurts top-rank behavior, especially MRR, while
slightly improving recall and dense overlap.

## M563B Pairwise Guard

The next smallest repair is not another LM likelihood proxy.  It is to keep the
true-injection path and add the M555 dense-order pairwise guard:

```text
PAIRWISE_WEIGHT=0.20
PAIRWISE_WINNERS=24
PAIRWISE_SCALE=10.0
TOP1_WEIGHT=0.0
```

Continue only if this repairs the MRR/top-rank damage without losing the
mechanical positives: non-zero gradient, decreasing loss, and active recall
`>= 0.995`.

## Follow-Up Smokes

Three follow-up smokes were run after the first true-injection result.

Run roots:

```text
runs/m563_distilgpt2_inject_pairwise_seed563/
runs/m563_distilgpt2_inject_top1_pairwise_seed563/
runs/m563_distilgpt2_inject_lowgate_pairwise_seed563/
```

All follow-up runs kept the mechanical positives:

- CUDA was used on `spark-1`.
- Training completed in about two minutes per run.
- Every task kept `active_recall=1.0`.
- Validation total loss decreased on every task.
- The frozen LM remained frozen; only the posting encoder was updated.

Macro comparison against the same dense-topk surface:

| Variant | Gate | Pairwise | Top1 | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | - | - | - | 0.44821 | 0.20047 | 0.43368 | 0.58929 | 0.54137 |
| M563A true injection | 0.35 | 0.00 | 0.00 | 0.44692 | 0.19959 | 0.43409 | 0.57517 | 0.54262 |
| M563B pairwise | 0.35 | 0.20 | 0.00 | 0.44779 | 0.19976 | 0.43446 | 0.58396 | 0.54288 |
| M563C top1+pairwise | 0.35 | 0.20 | 0.05 | 0.44899 | 0.19997 | 0.43359 | 0.57908 | 0.53987 |
| M563D lowgate+pairwise | 0.15 | 0.20 | 0.00 | 0.44783 | 0.19961 | 0.43431 | 0.58388 | 0.54286 |

Deltas versus dense_topk128_sparse:

| Variant | dNDCG | dMAP | dR | dMRR | dO |
| --- | ---: | ---: | ---: | ---: | ---: |
| M563A true injection | -0.00129 | -0.00088 | +0.00041 | -0.01412 | +0.00125 |
| M563B pairwise | -0.00042 | -0.00071 | +0.00078 | -0.00533 | +0.00151 |
| M563C top1+pairwise | +0.00078 | -0.00050 | -0.00009 | -0.01021 | -0.00150 |
| M563D lowgate+pairwise | -0.00038 | -0.00086 | +0.00063 | -0.00541 | +0.00149 |

## Decision After M563D

M563 is a valid mechanism but not a promotable retrieval improvement yet.

What is proven:

- M562's paper-inspired interface signal was real: `distilgpt2` has
  retrieval-like query heads under our prompt.
- M563 implemented true differentiable score injection, not an external
  likelihood proxy.
- The injected loss can train the posting encoder without breaking active
  membership.
- Pairwise dense-order regularization is necessary; it reduced the MRR damage
  from `-0.01412` to about `-0.0053`.

What is not proven:

- The current `distilgpt2` injection setup does not beat the dense-topk posting
  surface cleanly.
- Lowering the injection gate from `0.35` to `0.15` did not solve the issue; it
  almost exactly reproduced the M563B tradeoff.
- Adding a top1 guard improved NDCG but worsened overlap and MRR, so it is not
  a stable promotion path.

Therefore do not send M563 to broad10 and do not replace M555/M551 with it.
The useful conclusion is narrower: DREAM-style frozen-judge supervision is
mechanically viable, but this small `distilgpt2` judge is not yet a strong
enough training signal to improve the posting encoder reliably.

The next valid research move is not more small hyperparameter tuning on this
exact setup.  Either:

- keep M551/M555 as the stable baseline and return to larger encoder/posting
  training, or
- run one stronger-judge M562-style head probe before investing in another true
  injection implementation.

If a stronger judge does not produce a clearly better M562 head surface, stop
the DREAM-attention branch and record it as a mechanism-positive but
retrieval-negative detour.
