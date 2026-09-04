# M1530C Hard-Soft Distillation Contract

Date: 2026-07-10

## Causal Question

M1530B's rank-8 shared basis reduced validation teacher KL by 9.991% while
preserving posting cost, but teacher top1 regressed. M1530C tests whether the
remaining failure is caused by soft distribution KL underweighting the
teacher's winning candidate.

The only change is the objective:

```text
loss = KL(teacher_distribution, student_distribution)
     + CE(student_scores, argmax(teacher_scores))
```

The hard teacher term has fixed weight `1.0`. It is paired with, not substituted
for, the soft teacher distribution. The teacher winner may be any candidate;
the nominal positive at index zero is not forced to win.

## Locked Surface

- Frozen BGE backbone, frozen 32K M1520 centroids, rank-8 shared adapter.
- Same zero-initialization and exact M1530A step-0 identity requirement.
- Same M1518 256 train / 128 validation rows, candidates, frozen teacher,
  seed, tokenization, learning rate, weight decay, gradient accumulation,
  TopK budgets, background mask, and 800 optimizer steps.
- Same ClearML and artifact-hash requirements.
- No BM25, qrels, routing, reconstruction, FLOPS, native-index change,
  rank change, or backbone unfreezing.
- No loss-weight, temperature, learning-rate, threshold, or seed grid.

## Promotion And Stop

The original conjunctive promotion gate is unchanged:

- KL decreases by at least 10%;
- teacher top1 improves by at least 0.05, or pair-order improves by at least
  0.03 without top1 regression;
- touch, max DF, active-vocabulary, finite, and non-negative gates pass.

M1530A and M1530B remain diagnostic frontiers, but frontier dominance alone
does not authorize another capacity or objective variant. M1530C passes only
the strict gate.

- Strict pass: scale the identical rank-8 hard-soft mechanism to 10K/1K.
- Strict fail: stop the M1530 retrieval-head branch and report the objective
  mismatch. Do not tune the hard-term weight or try rank 32.

No result on this capacity surface is an OOD, BEIR, native-index, or product
claim.
