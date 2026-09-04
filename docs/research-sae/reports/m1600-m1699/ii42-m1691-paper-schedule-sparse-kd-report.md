# M1691 Paper-Schedule Sparse KD Report

## Decision

**Stop this no-positive pure-KD continuation. Retain the unmodified OpenSearch
sparse root plus M1660 BMP, and move to the separately contracted M1700
positive-pair curriculum.**

M1691 removed the invalid M1690 optimizer confound, but neither teacher
produced a checkpoint that jointly passed quality and sparse-cost gates. The
result is not an optimization-depth ambiguity: both teachers reached a point
where every quality gate passed, and both crossed the document max-DF boundary
to do so. Additional steps then regressed ordering while max DF kept growing.

## Fixed Recipe

- root/data/teacher/validation: identical to M1690;
- batch 20;
- AdamW 2e-5, weight decay 0.01;
- linear 100,000-step clock with 6,000 warmup steps;
- no gradient clipping;
- document FLOPS 0.08/T=40,000, threshold 150;
- 2,000 steps with full 2,048x100 evaluation.

The batch-20 forward/backward canary passed with finite loss/gradients and
31,805,407,232 peak reserved bytes, 24.35% of device memory.

## Stored-Score Control

| Step | LR | KL | KL gain | Top1 | Pairwise | Spearman | Max cost | Pass |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 0 | 0 | 1.656219 | 0.00% | 0.522461 | 0.785161 | 0.738735 | 1.000x | no |
| 500 | 1.67e-6 | 1.546934 | +6.60% | 0.544922 | 0.783286 | 0.734384 | 1.174x | no |
| 1,000 | 3.33e-6 | 1.491317 | +9.96% | 0.566895 | 0.786676 | 0.741977 | 1.469x | no |
| 2,000 | 6.67e-6 | 1.495789 | +9.69% | 0.566895 | 0.784413 | 0.737562 | 1.708x | no |

At step 1,000 every quality gate passes. The blockers are document FLOPS
`1.294x` and document max DF `1.469x`. Mean document nnz is only `1.098x`, so
the problem is not a general density explosion; the model concentrates mass in
a small number of high-DF vocabulary dimensions.

This is materially different from M1690. Under batch two and constant LR,
step 1,000 had KL `-17.20%`, pair `0.750762`, Spearman `0.664939`, and max cost
`7.879x`. The author schedule makes the teacher distribution optimizable while
preserving full-candidate order, but the stored-score target does not satisfy
the posting-list cost gate.

## Ensemble Teacher

| Step | LR | KL | KL gain | Top1 | Pairwise | Spearman | Max cost | Pass |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 0 | 0 | 1.656219 | 0.00% | 0.522461 | 0.785161 | 0.738735 | 1.000x | no |
| 500 | 1.67e-6 | 1.580471 | +4.57% | 0.527832 | 0.783058 | 0.733917 | 1.098x | no |
| 1,000 | 3.33e-6 | 1.529592 | +7.65% | 0.544434 | 0.786479 | 0.741199 | 1.339x | no |
| 2,000 | 6.67e-6 | 1.595487 | +3.67% | 0.538086 | 0.784244 | 0.737363 | 1.548x | no |

Step 500 is cost-safe but regresses pairwise and Spearman. Step 1,000 passes
KL, top1, pairwise, and Spearman together, but document max DF is `1.3389x` the
root. Mean document nnz is `0.9899x` and document FLOPS is `1.0329x`, so the
blocker is concentrated high-DF terms, not general density. Step 2,000 loses
the joint quality trajectory and raises document max DF to `1.5483x`.

## Paired Verdict

- Stored ClearML: `0ebfd03f3f9243909ae6f6601b4aed48`.
- Ensemble ClearML: `066519a32f15499ba7ede79cf2145f40`.
- Both step-zero score files are byte-identical; array SHA-256:
  `d19b77df5083dbf1261f8ec0d823de7aa65ff824050448e06b218f89aa5e3002`.
- Stored control complete: yes.
- Ensemble conjunctive checkpoint: none.
- Paired comparison: `stop_kd`.

The ensemble cannot enter the 100K/10K scale gate. No LR, warmup, batch,
candidate-count, temperature, FLOPS-lambda, or threshold variant is
authorized.

## Scientific Interpretation

M1691 establishes three separate facts:

1. The pinned author optimizer schedule makes the score-distribution signal
   learnable; the M1690 constant-LR divergence was not a route-level result.
2. On this no-positive K=8 surface, full-candidate quality improvement becomes
   available only after document posting concentration crosses the locked
   cost floor.
3. Training deeper does not resolve the conflict. It worsens ordering and max
   DF after the step-1,000 frontier.

The data source has no explicit positive label. The successful recipes in
LACONIC, Lion-SP, and Beyond Hard Negatives first establish relevance geometry
with positive-pair contrastive training, then use curated hard negatives and/or
KD. Therefore M1691 closes pure KD but does not falsify learned sparse
retrieval or the one-encoder/one-index product shape.

M1700 is the only authorized model continuation. It starts from the same
mature sparse root, uses broad positive pairs and in-batch negatives, and adds
hard-negative/KD training only after the positive mechanism gate passes.
DF-FLOPS is reserved for a later quality-positive checkpoint whose measured
blocker remains high document DF.
