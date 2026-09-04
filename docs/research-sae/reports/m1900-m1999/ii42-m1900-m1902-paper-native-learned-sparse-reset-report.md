# M1900-M1902 Paper-Native Learned-Sparse Reset Report

Date: 2026-07-12

Decision: **retain SAE-SPLADE as a scale-sensitive research route; do not run
native FiQA and do not promote a checkpoint yet.**

## Research Reset

This stage stopped project-specific posting-loss search and returned to three
published learned-sparse controls:

- standard SPLADE with a DistilBERT MLM vocabulary;
- SAE-SPLADE with a width-65,536 TopK-8 latent vocabulary;
- the frozen Apache-2.0 OpenSearch sparse v2 checkpoint already reproduced by
  M1640.

The official SAE-SPLADE repository was pinned at
`2056602254ef8ab2162e1f7424a570596b448f71`. Its source/configuration hashes
pass, but no public checkpoint or explicit repository license was found. It is
therefore a research mechanism source, not product code.

## M1901a Schedule Failure

The first environment closure used the repository's debug configuration and
compressed the FLOPS ramp from 6,000 to 20 steps. Standard SPLADE collapsed
from `16,753.5` to `1.10` mean document nonzeros. That run proves the CUDA,
gradient, checkpoint, and summary path, but it is not a representation result.

This failure is retained because it repeats an older project lesson: scaling a
regularization schedule into a short canary can change the objective, not just
reduce its runtime.

## M1901b Paper-Absolute Canary

M1901b restored the formal `normal_base` coefficients (`q=0.06`, `d=0.04`) and
the absolute 6,000-step FLOPS ramp. Both branches used the same 4,096 training
rows, 128 heldout rows, teacher scores, row order, 100-step IR budget, and
DistilBERT revision.

| Branch | KL | Margin MSE | Positive top1 | Pairwise | Doc nnz | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Standard SPLADE step 100 | 2.119706 | 66.285660 | 0.101562 | 0.504883 | 64.50 | 1.000000 |
| SAE-SPLADE initialization | 2.127535 | 53.590832 | 0.210938 | 0.654297 | 254.86 | 0.999132 |
| SAE-SPLADE step 100 | 1.610941 | 49.969910 | 0.265625 | 0.690430 | 300.08 | 1.000000 |

SAE-SPLADE passed its own gate and beat the final standard branch on every
ranking/teacher-fit field. Standard SPLADE failed its own ranking-safety gate,
so the paired final-step comparison was not promotable.

## M1902 Fixed Depth Curve

M1902 evaluated fixed milestones `20/40/60/80/100` on rows `0:128`, selected
without qrels, and confirmed on disjoint rows `128:640`.

Standard SPLADE had no eligible milestone. Its closest point was step 20 with
pairwise `0.622070`, slightly below the locked safety floor; later points traded
lower KL for progressively worse ranking.

Every SAE milestone passed. The locked lexicographic rule selected step 80:

| SAE step | KL | Positive top1 | Pairwise | Doc nnz | Doc maxDF |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 20 | 1.799863 | 0.281250 | 0.691406 | 302.8 | 1.000000 |
| 40 | 1.545012 | 0.351562 | 0.692383 | 313.5 | 1.000000 |
| 60 | 1.535908 | 0.281250 | 0.692383 | 313.6 | 1.000000 |
| 80 | 1.375965 | 0.335938 | 0.732422 | 308.9 | 1.000000 |
| 100 | 1.360437 | 0.382812 | 0.727539 | 304.8 | 1.000000 |

## Disjoint Confirmation

The selected SAE checkpoint retained every improvement on 512 unseen rows:

| Surface | KL | Margin MSE | Positive top1 | Teacher top1 | Pairwise | Spearman | Doc nnz | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| SAE initialization | 2.265312 | 49.147984 | 0.242188 | 0.232422 | 0.662842 | 0.239913 | 265.98 | 0.996094 |
| SAE step 80 | 1.368354 | 44.312164 | 0.285156 | 0.294922 | 0.707520 | 0.325839 | 308.43 | 0.999132 |
| OpenSearch control | 1.250686 | 153.059357 | 0.675781 | 0.742188 | 0.924072 | 0.719015 | 177.41 | 0.063802 |

The SAE signal is real: it is positive on selection and confirmation, not a
single split or final-step artifact. It is also far from a mature milestone.
Against OpenSearch, SAE has `1.74x` document nnz, about `37.3x` document FLOPS,
and about `15.7x` sampled maximum DF, while losing `0.390625` positive top1 and
`0.216552` pairwise agreement.

## Interpretation

M1900-M1902 do not support returning to arbitrary SAE reconstruction. They
support one precise statement:

> A TopK-SAE latent vocabulary learns retrieval signal under the published
> SAE-SPLADE objective, and that signal generalizes, but 100 reconstruction and
> 100 IR steps leave universal latent activation and a large gap to a fully
> trained sparse retriever.

The current training is only about `0.06%` of the paper's 160,000 SAE steps
and `0.04%` of its 240,000 IR steps. Stopping now would repeat the project's
under-training risk. Jumping directly to the six-day recipe would also be
unscientific because the local data/runtime implementation is not yet the
official full pipeline.

M1903 is therefore authorized as one medium-depth experiment. It changes only
training exposure and observability. No width, TopK, loss, teacher, pruning, or
dataset-specific tuning is allowed.

## Artifacts

- M1900 contract: `docs/research-sae/reports/m1900-m1999/ii42-m1900-paper-native-learned-sparse-reset-contract.md`
- M1902 contract: `docs/research-sae/reports/m1900-m1999/ii42-m1902-sae-splade-depth-confirmation-contract.md`
- Artifact audit: remote `m1900-artifact-audit.json` and `.md`
- M1901a ClearML: `a832d29498644d9da5f39e312058ca4e`
- M1901b ClearML: `9a913dfddce04949ac7aaa95fea093ed`
- M1902 ClearML: `ae1bbd0698b049548614714fcaf17262`
- M1902 selected checkpoint: `sae_splade_selected_step80.pt`
