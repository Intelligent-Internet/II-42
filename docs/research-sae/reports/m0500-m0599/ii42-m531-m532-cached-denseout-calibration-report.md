# M531-M532 Cached Denseout Calibration Report

## Purpose

M530 showed that residual adapter training did not improve over frozen raw
PPLX denseout.  M531 and M532 separate two concerns:

1. Cache the expensive first-stage corpus product.
2. Test closed-form calibration on cached raw denseout without rerunning PPLX.

The route remains first-stage and document-only.  Queries, qrels, BM25, route
positives, and ranking losses are not used.

## M531 Caches

M531 first created a FiQA document cache:

| Item | Value |
| --- | ---: |
| Train docs | 8192 |
| Eval docs | 2048 |
| Cache size | 54 MB |
| Raw active recall | 0.94680 |
| Raw cosine | 0.99065 |
| Raw active jaccard | 0.91045 |

Artifact:

```text
outputs/m531/corpus_denseout_cache/FiQA2018_denseout_cache.npz
```

It was then expanded to broad4:

| Item | Value |
| --- | ---: |
| Tasks | FiQA2018, ArguAna, SCIDOCS, TRECCOVID |
| Train docs per task | 8192 |
| Eval docs per task | up to 2048 |
| Cache size | 204 MB |
| Raw active recall | 0.93707 |
| Raw cosine | 0.99124 |
| Raw active jaccard | 0.89147 |

Artifacts:

```text
outputs/m531/corpus_denseout_cache/broad4_8k/
```

This cache makes repeated product-surface diagnostics cheap.  The PPLX encoder
no longer needs to run for every small calibration or student-head experiment.

## M532 Closed-Form Calibration

M532 tested identity, diagonal scaling, orthogonal Procrustes, and ridge linear
calibration on the M531 FiQA cache.

| Calibrator | Active Recall | Cosine | Active Jaccard |
| --- | ---: | ---: | ---: |
| `identity` | 0.94680 | 0.99065 | 0.91045 |
| `diagonal` | 0.94572 | 0.99068 | 0.90836 |
| `procrustes` | 0.93821 | 0.99046 | 0.89404 |
| `ridge` | 0.92502 | 0.98972 | 0.86955 |

The best calibrator is still identity.  Diagonal slightly improves cosine but
hurts active recall, which is the more important product metric for posting.
Procrustes and ridge both overfit or rotate away from the active coordinate
support.

The broad4 cache reproduced the same result:

| Calibrator | Active Recall | Cosine | Active Jaccard |
| --- | ---: | ---: | ---: |
| `identity` | 0.93707 | 0.99124 | 0.89147 |
| `diagonal` | 0.93619 | 0.99127 | 0.88969 |
| `procrustes` | 0.92964 | 0.99118 | 0.87705 |
| `ridge` | 0.91966 | 0.99062 | 0.85851 |

Again, identity is the best calibrator by active recall.  The result is not a
single-dataset artifact.

## Decision

Global linear/closed-form calibration is not the missing piece.  It does not
beat frozen raw denseout even on a clean document-only cache.

The next useful experiment should be one of:

1. Train a real product student/head on cached arrays with raw identity as a
   hard validation floor.
2. Add a teacher-score reconstruction objective on cached products before any
   route/NDCG evaluation.
3. If using a neural encoder, train on much larger corpus rows, but promote
   only if heldout active recall and dense score reconstruction beat raw
   denseout.

Do not repeat residual adapter, diagonal, Procrustes, or ridge calibration
unless the teacher surface definition changes.

## Artifacts

- `scripts/research_sae_m531_corpus_denseout_cache.py`
- `scripts/research_sae_m532_cached_denseout_calibration.py`
- `outputs/m531/corpus_denseout_cache/m531_fiqa_8k_cache_seed5310.json`
- `outputs/m531/corpus_denseout_cache/FiQA2018_denseout_cache.npz`
- `outputs/m531/corpus_denseout_cache/broad4_8k/m531_broad4_8k_cache_seed5310.json`
- `outputs/m532/cached_denseout_calibration/m532_fiqa_8k_calibration_seed5320.json`
- `outputs/m532/cached_denseout_calibration/m532_broad4_8k_calibration_seed5320.json`
