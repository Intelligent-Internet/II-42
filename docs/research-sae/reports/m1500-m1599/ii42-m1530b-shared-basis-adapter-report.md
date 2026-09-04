# M1530B Shared-Basis Adapter

Decision: **stop rank scaling and reject rank 32**. Retain the rank-8 shared
basis as evidence that low-rank geometry can fit the teacher distribution
without damaging posting cost, but do not promote it as a retrieval head.

## Question

M1530A's fully trainable 32K projection briefly improved validation ordering
but overfit a 256-row capacity surface. M1530B tested whether the same paired
teacher signal would generalize better when all BGE and centroid parameters
were frozen and only a shared rank-8 residual transform was trainable.

The locked comparison changed no data, teacher, seed, learning rate, TopK,
background mask, score function, or evaluation gate. The adapter contained
12,288 representation parameters plus one score-calibration scalar, versus
25,198,593 trainable parameters in M1530A.

## Identity And Runtime

The first two runtime attempts exposed numerical-equivalence issues before
training: a second normalization and a bias-free matrix kernel moved KL by
roughly `7e-6`. The gate was not relaxed. The final implementation preserves
the source norm and uses the same frozen zero-bias `nn.Linear` projection as
M1530A.

The accepted smoke reproduced every M1530A step-0 metric with maximum
difference `0.0`. The formal run used trainer SHA-256
`4070c90825437846fabbb1f42232da4877a73f7120e517a65b7630d5c68392d0`
and ClearML task `50bc80f14de24819a572cc05f0f623b5` on `spark-1`.

## Result

| Step | KL | Teacher top1 | Pair order | Positive top1 | Touch mean | Max DF | Active concepts |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 1.508126 | 0.476562 | 0.837573 | 0.460938 | 0.078369 | 0.100694 | 21,315 |
| 150 | 1.494048 | **0.500000** | 0.837573 | 0.476562 | 0.078152 | 0.100694 | 21,330 |
| 300 | 1.434667 | 0.445312 | 0.830724 | 0.421875 | 0.078016 | 0.101562 | 21,411 |
| 500 | **1.357449** | 0.468750 | **0.838552** | 0.453125 | 0.078946 | 0.103299 | 21,487 |
| 650 | 1.412441 | 0.484375 | 0.833659 | 0.437500 | 0.080682 | 0.105903 | 21,444 |
| 800 | 1.398108 | 0.476562 | 0.833659 | 0.421875 | 0.082737 | 0.110243 | 21,375 |

At the lowest-KL checkpoint, step 500:

- KL improved by 9.991%, just short of the 10% threshold;
- teacher top1 regressed by 0.007812;
- pair-order improved by only 0.000978;
- touch increased by 0.74% relative;
- max DF increased by 0.002604 absolute;
- active vocabulary increased by 0.81% relative.

All numerical and posting-integrity checks passed. No checkpoint passed the
strict ordering gate or Pareto-dominated either M1530A frontier point. The
predeclared rank-32 continuation condition therefore failed.

## Interpretation

The low-rank parameterization solved the sample-efficiency and cost-shape part
of M1530A: it achieved almost 10% validation KL reduction with three orders of
magnitude fewer trainable representation parameters while touch and concept
usage stayed nearly fixed. More centroid freedom is not the next bottleneck.

The remaining failure is objective-level. Soft teacher KL can improve the
candidate distribution while changing the winning document in the wrong
direction. M1530B's best distribution fit had worse teacher top1, and its best
top1 checkpoint had little KL gain. Increasing rank would add capacity to the
wrong trade-off and is not supported by the data.

## Next Probe

The next controlled experiment should keep the successful rank-8 shared basis
and add an explicit teacher-winner term to the distillation objective:

```text
loss = teacher_distribution_KL + cross_entropy(student, teacher_argmax)
```

This is the standard hard-plus-soft knowledge-distillation structure and
directly targets the failure observed here. Use weight 1.0 once, with the same
256/128 rows, seed, budgets, learning rate, and 800-step limit. No loss-weight
grid is authorized. The run must preserve the exact step-0 identity and pass
the original conjunctive gate; a top1 gain purchased by losing KL, pair order,
or posting integrity is a failure.

## Artifacts

Bounded JSON, JSONL, and logs are mirrored under
`runs/m1530b_shared_basis_adapter_v1/`. Small adapter checkpoints remain on
`spark-1`; failed artifacts are not promoted.
