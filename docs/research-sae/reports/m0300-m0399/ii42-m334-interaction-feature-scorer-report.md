# ii42 M334 Interaction Feature Scorer Report

## Summary

M334 tests the next low-cost hypothesis after M333:

> the scorer is not mainly missing another margin loss; it is missing richer
> query-document interaction features.

M333 explicit winner/loser admission loss did not promote. M334 therefore keeps
the M332 v4 scorer objective and changes only the candidate feature schema.

New interaction features appended to the existing feature vector:

- signed atom dot product;
- absolute atom dot product;
- IDF-weighted signed atom dot product;
- IDF-weighted absolute atom dot product;
- overlap mass coverage over query atoms;
- overlap mass coverage over document atoms;
- IDF-weighted overlap coverage over query atoms;
- IDF-weighted overlap coverage over document atoms.

The candidate cache schema is versioned so old M332/M333 feature caches are not
silently reused.

## Files

- `scripts/research_sae_m322_candidate_pool_scorer.py`
- `scripts/run_m334_interaction_feature_scorer_spark.sh`
- `docs/research-sae/reports/m0300-m0399/ii42-m334-interaction-feature-scorer-report.md`

## Canary Contract

Common surface:

- Datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`.
- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Teacher rankings:
  `/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json`
- Candidate K: `160`.
- Max atom DF ratio: `0.25`.
- Unified candidate scales: `0.25`, `0.5`.
- Teacher candidate injection: disabled.
- Doc/query active K: `96` / `80`.
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m334-interaction-candidate-cache-v1`

Primary run:

`/home/huoju/leask/runs/ii42-m334-interaction-feature-scorer-v1/m334_interaction_feature_seed1050.json`

Same-split repeat:

`/home/huoju/leask/runs/ii42-m334-interaction-feature-scorer-v3/m334_interaction_feature_v3_seed1051_split1050.json`

## Stop Rule

Promote only if the 5-dataset canary beats M332 v4 by a visible ranking margin:

- NDCG@10 or MAP@100 improves by at least `0.005`, or
- MRR@20 improves without hurting NDCG/MAP, and
- Recall@100 does not regress materially.

If the result is flat, the remaining scorer-side path likely needs a genuinely
stronger interaction model rather than more scalar feature engineering.

## Status

M334 v1 is a positive canary. M334 v3 confirms the signal on the same
train/heldout split with a different scorer training seed.

## Results

Reference rows:

| Model | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M331 adaptive | 0.7268 | 0.3649 | 0.3352 | 0.2428 |
| M332 v4 clean natural strong-rank | 0.7299 | 0.3656 | 0.3347 | 0.2426 |
| M333 v2 explicit win/loss | 0.7301 | 0.3649 | 0.3350 | 0.2425 |

M334 v1/v3:

| Run | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 | Upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: |
| M334 v1 interaction features | 0.7301 | 0.3719 | 0.3440 | 0.2489 | 0.7758 |
| M334 v3 seed1051 split1050 | 0.7301 | 0.3724 | 0.3455 | 0.2513 | 0.7758 |

M334 v3 delta vs M332 v4:

- R@100: `+0.0002`;
- MRR@20: `+0.0068`;
- NDCG@10: `+0.0108`;
- MAP@100: `+0.0087`.

M334 v3 delta vs M334 v1:

- R@100: `-0.0000`;
- MRR@20: `+0.0005`;
- NDCG@10: `+0.0015`;
- MAP@100: `+0.0024`.

Per-dataset v1:

| Dataset | R@100 | MRR@20 | NDCG@10 | MAP@100 | Upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9927 | 0.2408 | 0.3679 | 0.2424 | 1.0000 |
| `fiqa` | 0.7334 | 0.4877 | 0.3911 | 0.3272 | 0.7819 |
| `nfcorpus` | 0.3023 | 0.5814 | 0.3478 | 0.1625 | 0.3786 |
| `scidocs` | 0.4229 | 0.3360 | 0.1825 | 0.1250 | 0.5225 |
| `scifact` | 0.9813 | 0.6275 | 0.6650 | 0.6155 | 0.9878 |

Per-dataset v3:

| Dataset | R@100 | MRR@20 | NDCG@10 | MAP@100 | Upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9976 | 0.2429 | 0.3706 | 0.2439 | 1.0000 |
| `fiqa` | 0.7371 | 0.4822 | 0.3934 | 0.3284 | 0.7819 |
| `nfcorpus` | 0.3027 | 0.5878 | 0.3457 | 0.1617 | 0.3786 |
| `scidocs` | 0.4245 | 0.3305 | 0.1786 | 0.1241 | 0.5225 |
| `scifact` | 0.9447 | 0.6471 | 0.6808 | 0.6392 | 0.9878 |

M334 v1 training events:

| Epoch | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.7497 | 0.3643 | 0.3478 | 0.2507 |
| 20 | 0.7433 | 0.3593 | 0.3444 | 0.2474 |
| 30 | 0.7413 | 0.3600 | 0.3456 | 0.2488 |
| 40 | 0.7424 | 0.3560 | 0.3405 | 0.2468 |

The pooled heldout training score peaks at epoch 10 and then regresses. The
macro aggregate reported from the saved best model remains clearly above M332
on MRR/NDCG/MAP. This is the first scorer-side result after M332 that crosses
the stop-rule threshold.

M334 v3 training events:

| Epoch | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.7451 | 0.3563 | 0.3424 | 0.2458 |
| 20 | 0.7497 | 0.3650 | 0.3494 | 0.2533 |
| 30 | 0.7427 | 0.3591 | 0.3489 | 0.2500 |
| 40 | 0.7378 | 0.3606 | 0.3453 | 0.2483 |

The same-split repeat peaks at epoch 20 and confirms that the richer
interaction features are not a one-seed artifact. The final saved model's
macro aggregate is slightly stronger than v1 on MRR/NDCG/MAP.

An attempted seed repeat used `SEED=1051`. That changes both scorer randomness
and the train/heldout split in this runner, so it is not an apples-to-apples
repeat of v1. Its first eval was poor:

| Run | Epoch | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M334 v2 seed1051 | 10 | 0.7396 | 0.3493 | 0.3277 | 0.2362 |

The run was stopped. Future stability checks need a split seed separated from
the model/training seed.

## Decision

Keep M334 open as the current promising scorer-side direction.

The aborted v2 repeat is not a contradiction of v1, because it changed the
evaluation split. v3 fixes this with `SEED=1051` and `SPLIT_SEED=1050`.

The next controlled step is broader validation: keep M334's interaction feature
schema, add per-dataset output to the scorer report, and run the same scorer on
the next official/full-corpus validation surface before investing in a more
complex interaction model.
