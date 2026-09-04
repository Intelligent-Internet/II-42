# M1690 Sparse Foundation Observability Report

## Decision

**Authorize the paired S2A optimization-identifiability experiment. Do not
promote a model or run BEIR yet.**

The fixed OpenSearch dense+sparse ensemble teacher improves all four locked
full-candidate agreement metrics over the frozen v2-distill root on 2,048
heldout rows and 204,800 candidate scores. The improvement is strongest at the
top-1 boundary and weak but positive for global pairwise/Spearman ordering.
This proves teacher observability, not student absorption or retrieval gain.

## Fixed Surface

- Student root: OpenSearch `v2-distill` at revision `269e6638...`.
- Sparse teacher: OpenSearch `v1` at revision `708f7e68...`.
- Dense teacher: `gte-large-en-v1.5` at revision `104333d6...` with pinned
  remote implementation `40ced75c...`.
- Hard-negative source: 502,939 MS MARCO rows at revision `e93a3637...`.
- Text source: BEIR MS MARCO at revision `a918e0d1...`.
- Validation: 2,048 query-disjoint rows, 100 candidates per row.
- Reference: stored cross-encoder scores. The source has no explicit positive
  label, so positive-at-index-zero and InfoNCE claims are forbidden.

The materialized smoke surface contains 8,192 training rows and 2,048
validation rows. It resolves 906,077 required documents and 10,240 queries.
All identifier, text, finite-score, non-constant-score, and query-disjoint
checks passed.

## S1 Scale Ladder

| Rows | System | KL | Top1 | Pairwise | Spearman |
| ---: | --- | ---: | ---: | ---: | ---: |
| 64 | frozen root | 1.553575 | 0.546875 | 0.778261 | 0.720244 |
| 64 | ensemble teacher | 1.317603 | 0.593750 | 0.789069 | 0.749649 |
| 512 | frozen root | 1.624757 | 0.513672 | 0.785722 | 0.739417 |
| 512 | ensemble teacher | 1.368093 | 0.580078 | 0.786912 | 0.743215 |
| 2,048 | frozen root | 1.656271 | 0.521484 | 0.785061 | 0.738543 |
| 2,048 | ensemble teacher | 1.516092 | 0.556641 | 0.785973 | 0.741598 |

| Rows | KL reduction | Top1 delta | Pair delta | Spearman delta | Gate |
| ---: | ---: | ---: | ---: | ---: | --- |
| 64 | 0.235971 | +0.046875 | +0.010808 | +0.029405 | pass |
| 512 | 0.256664 | +0.066406 | +0.001190 | +0.003798 | pass |
| 2,048 | 0.140179 | +0.035156 | +0.000911 | +0.003056 | pass |

The 2,048-row teacher recovers 334 of 980 frozen-root top-1 errors, introduces
262 new errors, and therefore yields 72 net recovered rows. Recovery is real
but not uniformly safe. That is why S2A requires a jointly passing trained
checkpoint rather than accepting lower training loss.

A deterministic 10,000-sample query-level paired bootstrap gives:

| Delta | Mean | 95% CI | Robustly positive |
| --- | ---: | ---: | --- |
| top1 | +0.035156 | [+0.012207, +0.058594] | yes |
| KL reduction | +0.139987 | [+0.055454, +0.222989] | yes |
| pairwise | +0.000911 | [-0.001209, +0.003017] | no |
| Spearman | +0.003056 | [-0.001699, +0.007980] | no |

The statistically stable teacher effect is therefore boundary/top1 and KL,
not global ordering. Pairwise and Spearman remain hard non-regression gates for
student absorption; they are not claimed as stable teacher gains.

The dense teacher by itself is not the route: on the formal surface it has KL
`1.993073`, top1 `0.444336`, pairwise `0.735363`, and Spearman `0.632067`, all
worse than the frozen sparse root. The evidence supports heterogeneous
ensemble distillation, not replacing sparse behavior with dense behavior.

## Literature-Constrained Interpretation

- [OpenSearch inference-free sparse retrieval](https://arxiv.org/abs/2411.04403)
  supplies the exact per-row normalized dense+sparse ensemble mechanism.
- [Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) supports the fixed
  `K=8` score-spectrum training source and forbids another negative-count grid.
- [DiSCo](https://arxiv.org/abs/2410.14609) independently supports distilling
  query-document similarity distributions instead of representation identity.
- [LACONIC](https://arxiv.org/abs/2601.01684) supports broad weak-pair
  pre-finetuning followed by hard-negative finetuning; it prevents interpreting
  a short KD failure as a universal learned-sparse failure.
- [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526)
  reports weak scaling for KD alone and stronger scaling for combined
  contrastive plus KD training. If S2A fails, deeper pure KD is therefore not
  authorized.
- [Probabilistic Expansion Control](https://arxiv.org/abs/2402.17535) supports
  curriculum-controlled expansion from a frozen root, but only as a later
  structural fallback because its experiments are multimodal.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) and
  [BMP](https://arxiv.org/abs/2405.01117) bind representation changes to actual
  posting-list and native-engine costs.

## Reproducibility

| Stage | ClearML task | Result |
| --- | --- | --- |
| S1 64 | `4333f55540df4f5f83d8388b15140180` | pass |
| S1 512 | `ce16538704524333aa1597110a607bce` | pass |
| S1 2,048 | `ac4134fa61b846d69d891c570e107009` | pass |
| S2A K=8 teacher | `a96cffbdfd06426ab7196caa2b6bc6f0` | pass |
| S2A stored KD | `25f8af6191df41c7b77e148e522e9568` | optimizer diagnostic failed |
| M1691 stored KD | `0ebfd03f3f9243909ae6f6601b4aed48` | running |

The formal run used 198,852 unique documents. Frozen-root, sparse-teacher, and
dense-teacher inference took 273.1, 367.5, and 703.0 seconds respectively on
`spark-2`.

The deterministic S2A teacher surface contains 8,192 rows, eight unique
score-spectrum candidates per row, 64,912 unique documents, score-range
coverage `1.0`, and entropy `1.747878`. Its Parquet SHA-256 is
`800e48771e4cc02ad1db2fd930d1b8387f973b597f9d92e9fff26a16150efb67`.
All row-count, uniqueness, finite, non-constant, and coverage checks passed.

The forward/backward-only memory canary produced finite loss and gradients.
Peak reserved CUDA memory was 3,716,153,344 bytes, or 2.85% of the 130.6 GB
device, and no optimizer update or checkpoint was written.

## Authorized Next Step

1. Finish and hash the deterministic 8,192-row, `K=8` paired teacher surface.
2. Run a forward/backward-only CUDA memory canary with no optimizer update.
3. Run `stored_cross_kd` and `ensemble_kd` from identical root bytes for 2,000
   steps, with full 2,048x100 evaluations at 0/500/1,000/2,000.
4. Stop unless both branches pass their joint gates and the ensemble wins at
   least two of KL/pair/top1 without losing the third.

No alpha, temperature, FLOPS-lambda, candidate-count, or threshold search is
authorized on this surface.
