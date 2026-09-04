# M1680 Calibrated Vocabulary Transfer Contract

## Decision Question

Can a complete vocabulary/interface transfer turn a modern masked-language
encoder into a stable learned-sparse foundation before retrieval training?

M1680 is not another pooled-dense output head. It transfers the tokenizer,
input embeddings, tied decoder, and output prior together. This distinction is
locked because the prior M1641 pooled-vocabulary bridge was closed and the
Vocabulary Transfer ablation reports that head-only ModernBERT-ESPLADE remains
below full transfer (`50.5` versus `52.4` BEIR nDCG@10 after adaptation).

The first experiment is an initialization and activation-shape audit. It does
not claim retrieval quality and it cannot promote a checkpoint.

## Evidence And Causal Controls

Two recent results identify separate structural failure causes:

1. Vocabulary Transfer (`arXiv:2607.00004`) attributes the ModernBERT sparse
   gap to an incompatible case-sensitive vocabulary. Its complete transfer
   uses semantic neighborhood initialization, prior-aware bias transfer, short
   frozen-backbone MLM adaptation, and activation-potential calibration (APC).
2. MLM-head rescaling (`arXiv:2606.18811`) finds that ModernBERT's mean decoder
   row norm is `2.553`, which destabilizes unnormalized SPLADE scoring. A fixed
   `k=8` rescale reaches the best reported ModernBERT BEIR score in that study.

M1680 therefore locks four diagnostic arms before seeing results:

- native ModernBERT vocabulary and head;
- native vocabulary with the tied head divided by `k=8`;
- complete semantic vocabulary transfer before APC;
- the same transferred model with a diagnostic pre-MLM `c=5` shift.

There is no scale, activation-rate, vocabulary, or sparsity grid. The BERT
uncased 30,522-token vocabulary is fixed. BEIR qrels, BM25, and dataset-specific
signals are forbidden.

## S0: Deterministic Interface Audit

Use pinned public artifacts:

- `answerdotai/ModernBERT-base`, revision
  `8949b909ec900327062f0ebf497f51aef5e6f0c8`;
- `google-bert/bert-base-uncased`, revision
  `86b5e0934494bd15c9632b12f734a8a67f723594`.

For exact token overlap, copy the ModernBERT embedding. For every new target
token, compute cosine affinity to target-space overlap anchors, apply exact
sparsemax, and interpolate the corresponding ModernBERT anchors. Transfer the
BERT output-prior ordering into the ModernBERT bias mean and variance. Update
all special-token IDs and preserve tied input/output weights.

Required checks:

- source mean head norm reproduces `2.553` within `0.05`;
- overlap embeddings are copied with maximum error at most `1e-6`;
- transferred bias mean and standard deviation match source within `1e-5`;
- tied input/output weights share storage after transfer;
- semantic initialization has sampled pairwise cosine Spearman at least
  `0.10` and top-8 anchor overlap at least `0.02`;
- semantic top-8 anchor overlap exceeds a mean-vector control by at least
  `0.02` absolute.

Failure closes full transfer before MLM training. It may retain the independent
rescale control, but must not weaken these thresholds.

## S1: Qrels-Free Activation Audit

Probe 64 deterministic MS MARCO training texts only as unlabeled language.
Use max length 128 and bfloat16 inference. Report for every arm:

- head row-norm mean/max;
- token-level positive-logit activation rate;
- pooled mean/p95 nonzeros and sampled maximum DF;
- positive-logit p50/p90/p99 and p99/p50 tail ratio;
- active vocabulary fraction and finite-value checks.

The fixed `k=8` control must reproduce one eighth of the native head norm. The
unshifted semantic-transfer initialization must have token activation between
`30%` and `50%`, finite statistics, and p99/p50 at least `2.0`. The pre-MLM
`c=5` profile is diagnostic only: the paper applies APC after MLM because MLM
sharpens logits. It must not gate initialization. These are initialization-
shape gates, not index-cost gates; mature document sparsity is evaluated only
after retrieval training.

## S2: Minimal Adaptation Authorization

Only a complete S0+S1 pass authorizes one 500-step frozen-backbone MLM canary,
matching the smallest paper-supported adaptation. That canary must:

- use general qrels-free text rather than BEIR evaluation rows;
- train only transferred embeddings/tied decoder and bias;
- use overlap-aware masking with new-token weight `2`;
- apply `c=5` APC after MLM, not during optimization;
- retain the fixed `k=8` activation profile as the later retrieval-training
  control; native-vocabulary MLM is not a meaningful adaptation pair;
- track all configuration and outputs in ClearML.

Retrieval distillation remains unauthorized until the adapted checkpoint
passes activation, MLM heldout, and cost-shape checks. A later SPLADE control
must use standard MarginMSE/contrastive supervision and no project-specific
teacher invention.

## Stop Rules

Stop this route when any of the following occurs:

- full transfer cannot preserve tied weights or semantic topology;
- APC needs a post-result shift or target-rate grid;
- activation is dead or collapses outside the paper-supported plateau;
- 500-step MLM improves loss while worsening heldout activation geometry;
- a proposed repair is another threshold, selector, negative subset, or
  dataset-specific choice;
- the complete transfer offers no advantage over the simpler `k=8` control on
  the same later retrieval surface.

## Literature Cutoff

The evidence search used II-Commons arXiv coverage through `2026-07-10`.
Primary references are `2607.00004`, `2606.18811`, `2411.04403`,
`2402.17535`, `2505.15070`, and `2401.06703`.
