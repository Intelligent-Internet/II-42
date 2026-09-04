# M1691 Paper-Schedule Sparse KD Contract

## Objective

Determine whether the observable M1690 dense+sparse teacher can be absorbed by
the frozen OpenSearch sparse foundation when the optimizer follows the relevant
author schedule instead of the invalid constant-LR, batch-two diagnostic.

M1691 changes only optimizer fidelity. It keeps the M1690 root bytes, K=8
teacher surface, full-candidate validation, losses, sparse-cost formula, seed,
and gates unchanged.

## Causal Basis

M1690 proved that the ensemble teacher has robust top1 and KL improvement on
2,048x100 heldout candidates. Its first stored-score training diagnostic failed
by step 1,000 and ended with KL -37.25%, pair 0.7340, Spearman 0.6267, and
max-DF cost up to 10.72x root.

The diagnostic was not recipe-faithful. The pinned OpenSearch precomputed-KD
`config_l0.yaml` uses batch 20, AdamW at 2e-5, linear scheduling over 100,000
steps, 6,000 warmup steps, no gradient clipping, and the absolute
FLOPS=0.08/T=40,000 schedule. M1690 used batch 2, constant full LR, and gradient
clipping. M1691 repairs exactly this mismatch; it is not a hyperparameter grid.

[Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) uses batch 16 for its
K=8 KD experiments and first contrastively adapts the encoder. This supports a
non-tiny effective batch but does not make our no-positive sparse composition
an exact reproduction. M1691 remains a controlled composition.

## Fixed Recipe

- physical/effective query batch: 20;
- candidates per query: deterministic K=8 score-spectrum surface;
- optimizer: AdamW, LR 2e-5, weight decay 0.01;
- LR schedule: linear, 6,000 warmup steps, 100,000 total-step clock;
- gradient clipping: disabled;
- document FLOPS: lambda 0.08, quadratic ramp T=40,000, threshold 150;
- sequence length: 512, retained from the validated M1690 product surface;
- steps: 2,000; evaluations: 0, 500, 1,000, 2,000;
- validation: all 2,048 rows and all 100 candidates.

Before training, batch 20 must pass the forward/backward memory canary below
85% peak reserved CUDA memory with finite loss and gradients.

## Gates

Each branch independently passes only when one trained checkpoint jointly:

- improves stored-reference KL by at least 2%;
- does not regress top1 or pairwise ordering;
- improves row Spearman;
- keeps query/document nnz, FLOPS, and max DF within 1.15x step zero;
- remains finite and non-degenerate.

Run the completed stored-score control first, then the ensemble branch from the
same model bytes and seed. Step-zero score array hashes must be identical.

The stored branch is a causal control and need not pass. The ensemble must pass
its joint gate and beat the stored branch's lowest-KL trained checkpoint on at
least two of KL, pairwise, and top1 without losing the third.

## Stop Conditions

Stop sparse KD after M1691 when:

- batch 20 fails the resource canary;
- no ensemble checkpoint passes jointly;
- step-zero identity differs;
- cost or ordering is traded for KL;
- the next proposal is another LR, warmup, batch, FLOPS, candidate-count, or
  temperature variant.

Failure authorizes no deeper pure-KD run. The only remaining literature-backed
model route is a separately contracted broad contrastive curriculum followed
by KD, as supported by LACONIC and Lion-SP. The unmodified OpenSearch root plus
official BMP remains the product-compatible baseline.
