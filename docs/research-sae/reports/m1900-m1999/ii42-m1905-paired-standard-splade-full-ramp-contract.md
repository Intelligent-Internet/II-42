# M1905 Paired Standard-SPLADE Full-Ramp Contract

Date: 2026-07-12

Status: **locked after M1904 launch and before M1905 training**

## Question

M1904 tests whether a longer SAE-SPLADE retrieval stage can retain ranking
while the published 6,000-step FLOPS ramp suppresses corpus-wide activation.
It does not establish that the effect is specific to the SAE representation.
M1905 therefore asks:

> On the identical bounded data, teacher, sampling schedule, optimizer, and
> FLOPS ramp, does a standard vocabulary SPLADE head produce a stronger
> ranking/cost frontier than SAE-SPLADE?

This is a paired mechanism control. It is not the paper's full 240,000-step
MS MARCO reproduction because the local 10,000-row BGE teacher sample is not
the official 27 GB ColBERTv2 64-way distillation file.

The comparison is package-level rather than a single-factor architecture
ablation. The standard branch uses a 30,522-dimensional pretrained MLM
vocabulary basis; the SAE branch uses a 65,536-dimensional reconstructed
latent basis. A winner therefore identifies the stronger mature output package
on this surface. It cannot separately attribute the result to width, semantic
initialization, or the SAE operator, and it does not authorize a width/init
grid after observing the result.

## Frozen Inputs

- Generic `distilbert-base-uncased` MLM initialization, matching the trained
  standard branch in the SAE-SPLADE official code.
- The same 10,000 M1518 training rows and disjoint `128/512`
  selection/confirmation rows used by M1904.
- The same seed `1903`, sampled batches, batch size `8`, learning rate
  `2e-5`, warmup `40`, weight decay `0.01`, query/document lengths `32/256`,
  KL plus `0.05` margin MSE, query/document FLOPS `0.06/0.04`, and absolute
  6,000-step squared ramp.
- Fixed observations at
  `100/500/1000/2000/4000/6000/8000/10000`.

No vocabulary TopK, pruning, score rescaling search, teacher change, qrels, or
dataset-specific choice is permitted.

## Selection And Comparison

M1905 uses the same qrels-free M1903 ranking floor and cost-first frontier as
M1904. Only checkpoints at or after step 6,000 may support a route conclusion.
The selected state is evaluated exactly once on the disjoint confirmation
rows.

The comparison is reported as a dominance test, not a forced winner:

- SAE-specific progress requires M1904 to improve pairwise agreement by at
  least `0.005` or positive-top1 by at least `0.02` over M1905 while keeping
  document maxDF no more than `0.02` worse and document nnz within `1.15x`.
- Standard-SPLADE dominance is the symmetric condition.
- Otherwise the bounded result is inconclusive and official-data scale must
  not be chosen from this surface alone.

## Stop Rules

- Do not compare M1904 against the old 100-step M1901 standard branch.
- Do not call M1905 an official paper reproduction.
- Do not download or train the 27 GB official distillation surface until the
  bounded paired run has completed and storage/runtime have been audited.
- If neither branch selects a post-ramp checkpoint, close this bounded data
  surface and move to the mature published checkpoint baselines.
- If one branch dominates, reproduce that branch first on official data before
  changing its loss or representation.

## Provenance Notes

The official SAE-SPLADE code uses `xpmir/SPLADE_DistilMSE` to construct the
validation/mining surface, but the trained standard SPLADE branch itself starts
from the configured generic Hugging Face masked-language model. Its published
training samples are the ColBERTv2 64-way file, whose pinned payload is about
27 GB. M1905 preserves the correct initialization but remains a bounded paired
diagnostic because it reuses the smaller M1518 teacher surface.
