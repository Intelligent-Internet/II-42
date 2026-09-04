# M558 Top1-Competition Posting Encoder Report

M558 follows M557's negative utility-attention smoke.  It keeps the M551
locked-support encoder and listwise KL anchor, but adds a sharper qrels-free
candidate competition objective:

```text
teacher_label = argmax(candidate_teacher_scores)
loss = cross_entropy(posting_scores / student_temperature, teacher_label)
```

The goal is to protect top-rank behavior directly.  This addresses M555's
main failure mode: weak average gains with seed instability, and severe
NDCG/MRR damage when training depth is increased.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m558_top1_competition_posting_spark.sh
```

New arguments:

```text
--top1-weight
--selection-top1-weight
```

Defaults are `0.0`, so previous M551-M557 commands remain compatible.

## First Smoke

Planned first smoke:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
TOP1_WEIGHT=0.10
PAIRWISE_WEIGHT=0.0
UTILITY_WEIGHT=0.0
SELECTION_TOP1_WEIGHT=0.10
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
SEED=552
```

## Stop / Continue Rule

Stop this top1 route if it fails to beat or tie M551 on NDCG@10 and MRR@20
without a material Recall@100 loss.

Continue only if it improves top-rank behavior more cleanly than M555, or if
it provides a complementary top-rank gain that justifies a small combined
top1-plus-pairwise experiment.

## Seed552 Smoke Result

The first smoke completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m558-top1-competition-posting-v1/top1_w0.10_seed552/
```

Same-task seed552 comparison:

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M557 | 0.44557 | 0.20232 | 0.43641 | 0.58952 | 0.54895 |
| M558 | 0.44655 | 0.20243 | 0.43790 | 0.58969 | 0.54875 |

M558 beats M551 on every tracked metric, but it does not dominate M555:

- versus M551: `+0.00009` NDCG, `+0.00072` MAP, `+0.00099` Recall,
  `+0.00028` MRR, `+0.00052` dense overlap;
- versus M555: `+0.00049` NDCG and `+0.00011` Recall, but `-0.00068` MAP,
  `-0.00130` MRR, and `-0.00041` dense overlap.

## Decision

M558 is not a standalone promotion, but it is a useful complementary signal.
It protects NDCG/Recall better than M555 on the seed552 smoke, while M555 keeps
better MAP/MRR/overlap.

The next test is a combined objective:

```text
TOP1_WEIGHT=0.10
PAIRWISE_WEIGHT=0.20
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
```

If the combined smoke cannot beat both M555 and M558 on the three-task surface,
do not scale this objective family further.
