# M561 Frozen LM Target-Likelihood Posting Report

M561 revisits the DREAM frozen-judge route after M553 failed.  M553 scored
query likelihood conditioned on each candidate document; that was only a cheap
proxy and did not ask whether the document helps predict the target passage.

M561 scores target-passage likelihood instead:

```text
utility(candidate, query, target) =
    -loss(target tokens | candidate document, query)
```

The target passage is the top dense-teacher document for the query.  Candidate
utility is blended with the dense query-document target distribution.

This is still not full DREAM attention injection, but it is closer to the
paper's functional supervision: useful candidates should reduce target
prediction loss under a frozen judge.

## First Smoke

Planned first smoke:

```text
TASKS=FiQA2018,SCIDOCS,TRECCOVID
LM_JUDGE_MODEL=distilgpt2
TEACHER_TARGET_MODE=lm_target_likelihood_blend
JUDGE_WEIGHT=0.25
TEACHER_POOL_K=32
RANDOM_NEGATIVES=32
MAX_TRAIN_GROUPS=64
SUPPORT_SCORE_MODE=row_abs
```

The small candidate set keeps the frozen-LM scoring bounded.  This is a signal
diagnostic, not a promotion run.

## Stop / Continue Rule

Stop if seed552 fails to beat M551 on the same three-task surface or shows the
same pattern as M553: near-tied NDCG but lower Recall, MRR, or dense overlap.

Continue only if target-likelihood scoring is clearly better than M553 and at
least competitive with M551/M555.

## Seed552 Result

The seed552 smoke completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m561-frozen-lm-target-posting-v1/lm_target_distilgpt2_w0.25_seed552_tokfix/
```

Same-surface comparison:

| Model | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M561 | 0.44528 | 0.20053 | 0.43414 | 0.58990 | 0.54589 |

M561 deltas against M551:

| dNDCG | dMAP | dR | dMRR | dO |
| ---: | ---: | ---: | ---: | ---: |
| -0.00118 | -0.00118 | -0.00277 | +0.00049 | -0.00234 |

Training diagnostics stayed healthy on the support gate:

| Task | Selected epoch | Validation active recall |
| --- | ---: | --- |
| FiQA2018 | 4 | `[1.0, 1.0, 1.0, 1.0]` |
| SCIDOCS | 0 | `[1.0, 1.0, 1.0, 1.0]` |
| TRECCOVID | 4 | `[1.0, 1.0, 1.0, 1.0]` |

## Decision

Stop M561.  Do not run additional seeds.

Target-passage likelihood is conceptually closer to DREAM than M553's
query-likelihood proxy, but the cheap likelihood-distillation interface still
does not beat M551.  It improves only MRR slightly while losing NDCG, MAP,
Recall, and dense overlap.

This narrows the route: frozen-LM usefulness scores are not enough when they
remain external target distributions.  A future DREAM revisit would need a
materially closer interface, such as score injection into retrieval-relevant
attention heads, not another likelihood-blend target.
