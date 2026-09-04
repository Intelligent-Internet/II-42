# M556 Pairwise-Selection Posting Encoder Report

M556 is a narrow follow-up to M555.  It does not change the encoder,
candidate construction, support lock, or training loss.  It changes only the
checkpoint selection key.

## Motivation

M555 added a qrels-free dense-order pairwise loss to the M551 locked-support
candidate-set KL objective.  The first broad10 seed was positive, but the
second broad10 seed regressed on MAP/Recall/MRR.  Inspection showed that M555
recorded the pairwise loss during training but did not record validation
pairwise loss or use it for checkpoint selection.

That makes the experiment internally inconsistent: the new dense-order signal
can influence optimization, but the validation gate still selects checkpoints
mostly by listwise KL and support loss.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m556_pairwise_selection_posting_spark.sh
```

New selection argument:

```text
--selection-pairwise-weight
```

Selection key:

```text
validation_listwise_loss
    + selection_pairwise_weight * validation_pairwise_loss
```

Support loss remains the secondary tie-break key.  Validation gates still
enforce support, active recall, and optional teacher top-k/top-1 constraints.

Default `selection-pairwise-weight=0.0`, so M551-M555 behavior remains
backward compatible unless a runner enables it.

## Planned Test

First smoke:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
PAIRWISE_WEIGHT=0.20
SELECTION_PAIRWISE_WEIGHT=0.20
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
SEED=552
```

If the smoke does not beat or tie M555 on the three-task surface, stop M556.

If it does, run the same broad10 seed set used by M555 and compare against the
matching M551 and M555 JSONs.

## Promotion Rule

M556 can replace M551/M555 only if it improves the three-seed broad10 mean
without a material Recall@100 or MRR@20 loss.  A single positive seed is not
enough.

## Seed552 Smoke Result

The first smoke completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m556-pairwise-selection-posting-v1/pairwise_select_w0.20_seed552/
```

Same-task seed552 comparison:

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M556 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |

M556 selected the same epochs as M555 on this smoke.  Therefore
`selection_pairwise_weight=0.20` is too weak to change the gate and should not
be scaled to broad10.

## Decision

Stop M556 in this exact form.  Keep the validation pairwise instrumentation
because it is useful for diagnostics, but do not spend broad10 time on
`selection_pairwise_weight=0.20`.

The next useful step is to tune the training signal itself, starting with a
smaller pairwise weight on the same `h768_s0125` head:

```text
PAIRWISE_WEIGHT=0.10
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
```

This tests whether M555's weak positive broad10 mean can be made less
seed-sensitive without removing the dense-order signal.
