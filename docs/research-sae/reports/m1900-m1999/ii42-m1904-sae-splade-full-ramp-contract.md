# M1904 SAE-SPLADE Full-Ramp Contract

Date: 2026-07-12

Status: **locked after M1903 and before training**

## Question

M1903 improved unseen ranking substantially, but its 2,000-step IR stage ended
before the published 6,000-step FLOPS ramp reached full strength. Its max-DF
failure therefore does not distinguish a representation failure from an
incomplete regularization schedule.

M1904 asks one narrow question:

> Does the unchanged published SAE-SPLADE objective reduce corpus-wide
> activation after its absolute FLOPS ramp completes, while retaining the
> ranking gain already measured by M1903?

## Frozen Inputs

- Start from the M1903 reconstruction-selected width-65,536 TopK-8 SAE.
- Reuse the same 10,000 MS MARCO training rows and disjoint `128/512`
  selection/confirmation rows.
- Keep DistilBERT, teacher scores, candidate sets, KL plus `0.05` margin MSE,
  query/document FLOPS `0.06/0.04`, batch 8, learning rate `2e-5`, and the
  absolute 6,000-step ramp unchanged.
- Train IR for 10,000 steps and observe
  `100/500/1000/2000/4000/6000/8000/10000`.
- Do not change width, TopK, temperature, loss weights, data, pruning, or
  inference thresholds.

This is still a bounded mechanism run, not the 240,000-step paper
reproduction. The only experimental variable relative to M1903 is sufficient
IR horizon to cross the already-selected regularization schedule.

## Selection

Selection uses no qrels. A milestone must pass the original branch gate and
retain M1903 selection pairwise agreement within `0.01` and positive-top1
within `0.02`. Among eligible milestones, select the lowest document max-DF,
then lowest document FLOPS, then strongest ranking and teacher fit.

The selected state is confirmed once on the unchanged disjoint rows.

## Progress Gate

Authorize official data acquisition only when all conditions hold:

1. selected step is at or after 6,000;
2. disjoint pairwise agreement is within `0.005` of M1903;
3. disjoint positive-top1 is within `0.02` of M1903;
4. document max-DF is at most `0.95`;
5. document FLOPS does not exceed M1903;
6. document mean nnz is at most `1.15x` M1903;
7. the disjoint branch gate remains finite and passes.

The product gate remains comparison with the frozen Apache-2.0 OpenSearch
control: ranking and all document cost fields must be within `1.15x` or the
published sparse checkpoint remains the product baseline.

## Stop Conditions

- Stop the 10k-row SAE-SPLADE route if max-DF remains above `0.95` after the
  full ramp.
- Stop if lower max-DF requires losing the M1903 ranking floors.
- Do not respond by changing FLOPS coefficients or applying post-hoc pruning.
- If M1904 passes only the progress gate, the next step is official 64-way data
  scale, not another local loss variant.
- If it fails, retain SAE-SPLADE as literature evidence and move the executable
  route to a pretrained-SAE/SPLARE or Nomic/Latent-Terms reproduction.

## Literature Basis

- SAE-SPLADE (`2604.21511`) uses a 6,000-step FLOPS ramp inside a 240,000-step
  retrieval stage.
- SPLARE (`2603.13277`) reports that token-level SAE sparsity does not ensure
  sequence-level sparsity and requires explicit FLOPS plus inference TopK.
- DF-FLOPS (`2505.15070`) explains why mean activation alone can miss high-DF
  dimensions; M1904 therefore reports max-DF at every milestone.
- Rescaling MLM-Head (`2606.18811`) shows that output scale can destabilize
  sparse dot-product training. M1904 changes no scale or temperature so that
  schedule completion is isolated first.
