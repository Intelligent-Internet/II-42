# M635 / P1.9 Teacher-Target Audit

## Decision

Status: `teacher_target_contract_ready`

M549U is the clean dense-root-to-posting compiler seed; P1.3-a010 is the native retrieval reference; M630-M634 are stop evidence against frozen-row scorer/suppression expansion.

M635 should train a generated posting compiler, not a reranker over frozen P1 rows.

## Selected Teacher Target

| Field | Value |
| --- | --- |
| Teacher | `M549U active-locked tail-power compiler` |
| Input | `frozen_dense_root_embedding` |
| Output | `dense_derived_signed_posting_support` |
| Learned gamma | 1.025625110 |
| Train rows | 188674 |
| Trainable parameters | 1 |

## M549U Teacher-Fit Evidence

| Metric | Value |
| --- | ---: |
| doc active recall | 0.999985 |
| query active recall | 1.000000 |
| doc support cosine | 0.999951 |
| query support cosine | 1.000000 |
| doc support KL | 0.009153 |
| query support KL | 0.011765 |
| score Pearson vs M549 | 1.000000 |
| top100 overlap vs M549 | 1.000000 |
| top100 overlap vs dense | 0.995012 |

## Native Retrieval Reference

P1.3-a010 is the retrieval baseline and engineering reference.  It is not the training target for M635.

| Metric | P1.3-a010 |
| --- | ---: |
| CUB | 0.941661 |
| dense overlap@100 | 0.928425 |
| NDCG@10 | 0.778808 |
| MAP@100 | 0.700006 |
| Recall@100 | 0.854260 |
| MRR@20 | 0.871822 |

## Stop Evidence From Recent Frozen-Row Lines

| Line | Status | Main evidence |
| --- | --- | --- |
| M630 | `stop_calibration_line_not_promoted` | Best bounded M630 variant improves KL/MAP/MRR on smoke but has zero Recall@100 delta, below both the +0.005 acceptance target and the M629-B +0.001923 context gain. |
| M633 | `stop_deterministic_augmentation_failed` | dR=0.003271, dMAP=0.000165, dO100=-0.004118, dKL=0.154245 |
| M634 | `stop_compiler_basis_repair_failed` | dR=0.000408, dMAP=0.000087, dO100=-0.001373, dKL=0.003171 |

## Target Contract

The M635 target is a composite dense-derived posting target:

- `active_support_membership_top128`: preserve active posting support.
- `signed_support_mass_kl_prefix512`: preserve signed mass shape.
- `support_cosine`: preserve geometry, not only membership.
- `dense_tail_vs_false_head_pairwise_guard`: prevent M633/M634 false head artifact repeats.
- `top20_top50_head_preservation_regularizer`: protect the retrieval head before trying Recall recovery.

Forbidden signals remain BM25, qrels-primary labels, dataset ids, query/doc ids, alpha search, and frozen-row reranking.

## Next Step

Run M635 output compiler smoke with frozen dense root and the M549U target contract before any BM25 or qrels-facing scorer work.
