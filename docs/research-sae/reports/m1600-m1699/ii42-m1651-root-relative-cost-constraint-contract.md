# M1651 Root-Relative Cost Constraint Contract

## Trigger

M1650 S2 established a real but non-promotable transfer frontier:

- validation ensemble KL improved `7.48%` at step 500;
- pair accuracy improved `0.899740 -> 0.904948`;
- sparse-anchor Spearman remained `0.985447`;
- maximum DF did not move;
- document nonzeros/FLOPS increased to `1.1366x/1.1047x` baseline.

Step 250 remained inside the cost gate but improved KL only `2.63%`. The
failure is therefore expansion cost, not absent teacher signal or high-DF
collapse.

## One Structural Correction

M1651 repeats the same root, rows, teacher, candidate sets, optimizer, seed,
steps, and evaluation checkpoints. It changes only optimization geometry.

A frozen copy of the initial sparse checkpoint encodes each training batch.
For both query and document paths, it supplies two differentiable reference
costs:

- activation mass: mean per-row sum of impacts;
- FLOPS: sum of squared mean impacts.

The student minimizes the same ensemble KL subject to four root-relative
constraints:

```text
student query mass  <= root query mass
student query FLOPS <= root query FLOPS
student doc mass    <= root doc mass
student doc FLOPS   <= root doc FLOPS
```

Non-negative dual variables start at zero and are updated by projected dual
ascent from the measured violation. This replaces an arbitrary fixed sparsity
loss weight with a constrained objective. The fixed dual step is `0.1`; it is
not selected from evaluation.

No TopK, threshold, IDF, probabilistic masking, teacher-weight change, extra
training depth, or post-hoc pruning is allowed.

## Gate

Use the exact M1650 validation and selection gate:

- at least 5% ensemble-KL improvement;
- pair accuracy no lower than step 0;
- sparse-anchor Spearman at least 0.95;
- query/document nonzeros, maximum DF, and FLOPS each at most 1.10x step 0;
- finite trained checkpoint at step 100, 250, or 500.

Only a conjunctive trained checkpoint authorizes shared3 exact retrieval.

## Stop Rule

If no checkpoint passes, close this dense+sparse continuation objective. Do
not tune the dual step, relax the cost gate, add a fixed regularizer, extend
training, or start a probabilistic-expansion variant. The remaining path is a
larger established learned-sparse pretraining reproduction plus a real BMP or
native engine, not another bounded canary mutation.
