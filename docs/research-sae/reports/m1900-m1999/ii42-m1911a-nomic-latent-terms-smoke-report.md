# M1911A Nomic Latent Terms Smoke Report

Date: 2026-07-12

Decision: **activation and TopK-SAE mechanisms pass. Authorize the locked
10M-token, five-seed M1911B pilot.**

## Surface

- Frozen backbone: `nomic-ai/nomic-embed-text-v1.5` at revision
  `e9b6763023c676ca8431644204f50c2b100d9aab`.
- Pinned custom code: `nomic-ai/nomic-bert-2048` at revision
  `7710840340a098cfb869c4f65e87cf2b1b70caca`.
- Hidden size: 768.
- Activation cache: 32,768 train tokens and 4,096 validation tokens.
- SAE: width 32,768, TopK-16, AdamW peak learning rate `1e-3`.
- Smoke training: seed 1911, 1,000 steps, batch 4,096.
- Retrieval labels used: false.

The cache uses the official `search_document:` prefix and a stable hash-based
validation split. The model weights and separately pinned custom code loaded
without missing keys.

## Training Health

| Step | Loss | Dead feature ratio | Learning rate |
| ---: | ---: | ---: | ---: |
| 500 | 0.043072 | 0.000000 | 0.0005413 |
| 1,000 | 0.031630 | 0.000000 | 0.0000000 |

The run published a 100.7 MB checkpoint, per-seed JSON, summary JSON, and
ClearML task `72d93880bdc34bbea59efbe31a22038d`.

## Qrels-Free Validation

| Diagnostic | Value |
| --- | ---: |
| Normalized MSE | 0.663250 |
| Reconstruction cosine | 0.749143 |
| Validation active-feature ratio | 0.357635 |
| Train-never-active ratio | 0.000000 |
| Token usage entropy | 8.304014 |
| Document mean nonzero after sum pooling | 1,531.25 |
| Document maxDF ratio | 1.000000 |
| Head 1% posting share | 0.091592 |

The sequence-level shape is not a failure gate for Latent Terms. Token TopK-16
does not imply a 16-term document after hundreds of token activations are
sum-pooled. Unlike SAE-SPLADE's unnormalized latent dot product, the published
Latent Terms route deliberately uses corpus DF and BM25 IDF. M1910 also showed
that removing the highest-DF latent head reduced full FiQA Recall.

## Scale Decision

The smoke establishes runtime correctness, finite learning, checkpoint
persistence, and real throughput. It does not establish retrieval quality.
M1911B is therefore authorized with the already locked representation and no
FiQA-driven changes:

- 9.6M unique train tokens plus 200k validation tokens;
- three epochs, 28.8M token presentations;
- 7,032 optimizer steps per seed;
- five seeds 1911-1915;
- qrels-free representative-seed selection before FiQA evaluation.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-contract.md`
- Activation builder: `scripts/prepare_m1911_nomic_activation_cache.py`
- SAE trainer: `scripts/train_m1911_latent_terms_sae.py`
- Runner: `scripts/run_m1911_nomic_latent_terms_spark.sh`
- Activation metadata: `m1911a-nomic-activation-cache-smoke-v1/metadata.json`
- SAE summary: `m1911b-nomic-topk-sae-smoke-v1/summary.json`
