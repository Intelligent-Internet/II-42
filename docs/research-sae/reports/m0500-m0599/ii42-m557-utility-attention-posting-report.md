# M557 Utility-Attention Posting Encoder Report

M557 is the next step after M555/M556.  It stops local pairwise micro-tuning
and tests a stronger candidate-set interface inspired by DREAM.

## Hypothesis

M551/M555 mainly imitate the teacher score distribution.  DREAM suggests a
different interface: retriever scores should act like a candidate attention
budget, and the useful candidates should receive more mass because they reduce
a frozen judge loss.

M557 approximates that without a full frozen-LLM attention injection:

```text
student = softmax(posting_scores / student_temperature)
utility = standardize(candidate_teacher_scores)
loss = -sum(student * utility)
```

This trains the sparse posting encoder to allocate candidate-set mass toward
high dense-teacher utility, while keeping the existing M551 listwise KL as an
anchor.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m557_utility_attention_posting_spark.sh
```

New arguments:

```text
--utility-weight
--selection-utility-weight
```

Default values are `0.0`, so M551-M556 behavior remains compatible unless the
M557 runner enables the loss.

## First Smoke

Planned first smoke:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
UTILITY_WEIGHT=0.10
PAIRWISE_WEIGHT=0.0
SELECTION_UTILITY_WEIGHT=0.10
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
SEED=552
```

This isolates the utility-attention signal from M555's pairwise dense-order
loss.

## Stop / Continue Rule

Stop this M557 shape if the seed552 smoke fails to beat or tie M551 on the
three-task surface without a material Recall@100/MRR@20 loss.

Continue only if the smoke shows a cleaner gain than M555 `w0.20_h768_s0125`,
or if it gives a complementary gain pattern that justifies a combined
utility-plus-pairwise test.

## Seed552 Smoke Result

The first smoke completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m557-utility-attention-posting-v1/utility_w0.10_seed552/
```

Same-task seed552 comparison:

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M557 | 0.44557 | 0.20232 | 0.43641 | 0.58952 | 0.54895 |

M557 is weaker than M555 on every tracked metric and weaker than M551 on NDCG
and Recall.  It improves only MAP/MRR/overlap slightly over M551, which is not
enough to continue this isolated utility-attention shape.

## Decision

Stop M557 `UTILITY_WEIGHT=0.10` as a standalone route.  The result is useful
negative evidence: a simple student-softmax expected-utility approximation is
not close enough to DREAM's frozen-judge attention interface.

The next candidate-set objective should protect top-rank behavior more
directly, because M555's positive signal is thin and deeper training damaged
NDCG/MRR.  M558 therefore tests a teacher-top1 competition loss rather than a
generic expected-utility loss.
