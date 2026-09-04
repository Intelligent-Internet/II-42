# M560 Teacher-Interaction Support Report

M560 is the next non-weight-tuning test after M559 failed to stabilize the
top1-plus-pairwise proxy.  It keeps the M555 training objective and tests a
different retrieval interface: support membership selection.

## Hypothesis

M551-M559 lock support by each row's largest absolute dense coordinates:

```text
support = topk(abs(row), active_dims)
```

DREAM's key lesson is that the interface determines signal quality.  For sparse
posting, the interface is the coordinate support that survives into the index.
M560 therefore estimates a qrels-free global retrieval-coordinate prior from
training candidate sets:

```text
prior(dim) = sum_group sum_candidate
    softmax(teacher_scores / temperature)[candidate]
    * abs(query_dim * doc_dim)

support = topk(abs(row) * prior, active_dims)
```

This keeps inference standalone and BM25-free.  It changes only which dense
coordinates are allowed into the locked support.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m560_teacher_interaction_support_spark.sh
```

The default behavior remains unchanged because `--support-score-mode` defaults
to `row_abs`.  M560 sets:

```text
SUPPORT_SCORE_MODE=teacher_interaction
PAIRWISE_WEIGHT=0.20
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
```

## Stop / Continue Rule

Run seed552 on:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

Stop if M560 cannot beat or tie M555 on NDCG@10 and Recall@100 without a
material MAP/MRR loss.  Continue to seeds 551/553 only if seed552 is at least
competitive, because M559 already showed that single-seed gains can be
misleading.

## Seed552 Result

The seed552 smoke completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m560-teacher-interaction-support-v1/teacher_interaction_support_seed552/
```

Same-surface comparison:

| Model | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M559 | 0.44713 | 0.20308 | 0.43830 | 0.59019 | 0.54896 |
| M560 | 0.41107 | 0.18715 | 0.42353 | 0.52448 | 0.51409 |

M560 deltas against M551:

| dNDCG | dMAP | dR | dMRR | dO |
| ---: | ---: | ---: | ---: | ---: |
| -0.03538 | -0.01456 | -0.01338 | -0.06492 | -0.03415 |

The validation active recall gate stayed at `1.0` for all three tasks, so this
is not a simple support-gate implementation failure.  The new weighted support
target is internally consistent, but it changes the sparse geometry in a way
that hurts retrieval heavily.

Support prior summaries:

| Task | Selected epoch | Prior min | Prior p50 | Prior p95 | Prior max |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 1 | 0.60166 | 0.93679 | 1.55371 | 3.85910 |
| SCIDOCS | 1 | 0.65758 | 0.92360 | 1.48597 | 6.79087 |
| TRECCOVID | 4 | 0.46643 | 0.83785 | 2.02490 | 6.62007 |

## Decision

Stop M560.  Do not run additional seeds.

The result is useful negative evidence: a global teacher-interaction coordinate
prior is too blunt for this posting encoder.  It confirms that the dense row's
own absolute top-k support remains an important safety boundary.  The next
stage should avoid support-membership rewrites and instead focus on
candidate-set construction or a closer frozen-judge interface.
