# M1902 SAE-SPLADE Depth And Confirmation Contract

Date: 2026-07-12

Status: **locked after M1901b and before M1902 training**

## Why This Stage Exists

M1901b produced a real SAE-SPLADE signal, but both training curves reached
their lowest batch loss before step 100. The standard branch failed its own
heldout ranking-safety gate at the final checkpoint, while the SAE branch
passed and beat that failed control. A final-only comparison therefore cannot
separate a representation advantage from checkpoint-depth sensitivity.

M1902 changes only observability and selection. It does not change the model,
loss, data, teacher, optimizer, width, TopK, or maximum training budget.

## Fixed Design

- Branches: standard SPLADE and width-65,536 TopK-8 SAE-SPLADE.
- SAE stage: 100 reconstruction steps.
- IR stage: 100 steps with M1901b's formal paper coefficients and absolute
  6,000-step FLOPS ramp.
- Milestones: `20, 40, 60, 80, 100`.
- Selection surface: validation rows `0:128`.
- Confirmation surface: disjoint validation rows `128:640`.
- Mature frozen control: OpenSearch sparse v2 revision
  `269e6638b2c4f648996691f6d751495285d8f330`.

## Selection Rule

A milestone is eligible only when it passes the branch gate relative to that
branch's initialization:

- KL or positive-margin MSE improves by at least 5%;
- positive top-1 and pairwise agreement do not drop by more than 0.5 points;
- query/document outputs do not collapse;
- every value is finite.

Among eligible milestones, select lexicographically by:

1. maximum pairwise agreement;
2. maximum positive top-1;
3. maximum teacher top-1 agreement;
4. minimum KL;
5. minimum positive-margin MSE.

The confirmation rows cannot affect checkpoint selection.

## Confirmation Gate

On rows `128:640`, both selected branches must pass their own initialization
gate. SAE-SPLADE must then:

- match or beat selected standard SPLADE on KL or margin MSE;
- preserve standard positive top-1 and pairwise agreement within 0.5 points;
- keep document mean nnz, FLOPS, and sampled max DF within `1.15x` standard;
- match the frozen OpenSearch control's positive top-1 and pairwise agreement
  within 0.5 points;
- keep those same document cost fields within `1.15x` OpenSearch.

Only a complete confirmation pass authorizes exact native FiQA. A selection
win without confirmation is retained as a research signal but cannot expand.

## Stop Conditions

- If standard SPLADE has no eligible milestone, the paired canary is
  under-trained or mismatched to this teacher; do not claim SAE superiority.
- If SAE-SPLADE has no eligible milestone, stop the SAE route on this root.
- If SAE wins paired quality but fails mature-control cost, diagnose DF/load
  once; do not grid-search TopK or prune post hoc.
- If SAE loses the disjoint confirmation ranking gate, stop this canary family
  before native retrieval.
