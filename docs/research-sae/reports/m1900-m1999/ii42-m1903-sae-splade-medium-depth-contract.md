# M1903 SAE-SPLADE Medium-Depth Contract

Date: 2026-07-12

Status: **locked after M1902 and before training**

## Hypothesis

M1902 showed generalizing SAE retrieval signal but almost universal document
frequency. The remaining justified hypothesis is training exposure:

> More corpus-level SAE reconstruction followed by a longer unchanged IR stage
> should specialize latent supports, reduce the mature-control gap, and improve
> disjoint ranking without a new loss.

## Fixed Scale

- Data: all 10,000 existing M1518 MS MARCO training rows and the unchanged
  1,000-row validation file.
- SAE document pool: positive plus eight negatives, 90,000 document texts.
- SAE stage: 4,000 steps, batch 32, width 65,536, TopK 8, auxiliary TopK 16,
  dead-token threshold 10,000,000.
- IR stage: 2,000 steps, batch 8, candidate-set KL, delta-MSE weight 0.05,
  query/document FLOPS `0.06/0.04`, absolute FLOPS ramp 6,000.
- Token lengths: query 32, document 256.
- Selection rows: `0:128`; confirmation rows: `128:640`.
- IR milestones: `100, 250, 500, 1000, 1500, 2000`.
- Mature control: unchanged OpenSearch sparse v2 revision.

This is still not the paper-scale run. It increases SAE document presentations
from 400 to 128,000 and IR query presentations from 400 to 16,000 while keeping
the mechanism fixed.

## Stage-One Selection

Record reconstruction at `0, 100, 500, 1000, 2000, 4000`. Select the stage-one
checkpoint with minimum heldout reconstruction MSE. Selection uses no teacher
scores or qrels. The final report must also show raw latent document nnz and
sampled max DF at each stage-one milestone.

## IR Selection And Confirmation

Use the same selection rule as M1902: an eligible checkpoint must pass its own
initialization gate, then maximize pairwise agreement, positive top1, and
teacher top1 before minimizing KL and margin MSE. Confirm only the selected
checkpoint on rows `128:640`.

## Progress Gate

M1903 is useful only if all of the following hold on disjoint confirmation:

1. SAE reconstruction MSE improves by at least 10% from initialization.
2. Pairwise agreement improves by at least 0.02 over M1902's `0.707520`.
3. Positive top1 improves by at least 0.02 over M1902's `0.285156`.
4. Document mean nnz stays within `1.15x` M1902.
5. Document max DF decreases by at least 5% from M1902, or falls below `0.90`.
6. Every branch and confirmation gate is finite and passes.

The stricter product gate remains comparison with OpenSearch on ranking and
all document cost fields within `1.15x`. Passing only the progress gate
authorizes acquisition/materialization of the official 64-way ColBERT data;
only the product gate authorizes native FiQA.

## Stop Conditions

- Stop the 10k-row route if reconstruction improves but max DF does not.
- Stop if selection gains disappear on confirmation.
- Stop if deeper training improves KL while ranking or cost worsens.
- Do not respond to failure by changing width, TopK, FLOPS coefficients,
  teacher temperature, pruning, or post-hoc thresholds.
- Download the 26.96GB official distillation file only after the progress gate
  passes, or after a separate data-coverage audit proves current data is the
  sole blocker.
