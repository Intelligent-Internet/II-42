# ii42 M335 Interaction Feature Broad-8 Report

## Summary

M334 confirmed that adding direct query/document atom interaction features to
the M322 scorer improves top-rank metrics on the 5-dataset canary. M335 keeps
the same scorer objective and feature schema, then expands validation to every
dataset currently available in the local/spark official root:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`
- `trec-covid`
- `cqadupstack`
- `webis-touche2020`

This is a generalization check, not a new training objective.

## Contract

- Base runner: `scripts/run_m334_interaction_feature_scorer_spark.sh`
- Broad runner: `scripts/run_m335_interaction_feature_broad8_spark.sh`
- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Teacher rankings:
  `/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json`
- Candidate K: `160`
- Max atom DF ratio: `0.25`
- Unified candidate scales: `0.25`, `0.5`
- Teacher candidate injection: disabled
- Doc/query active K: `96` / `80`
- Seed: `1050`
- Split seed: `1050`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m334-interaction-candidate-cache-v1`

The cache path intentionally reuses M334 cache entries for the original five
datasets and builds only the missing broad-8 datasets. The feature schema is
versioned, so stale M332/M333 cache entries are not accepted.

## Stop Rule

M335 is useful if it preserves the M334 v3 ranking gain direction on the wider
surface:

- NDCG@10 or MAP@100 remains visibly above the M332-style clean natural scorer;
- no newly added dataset collapses relative to the candidate upper bound;
- the added datasets identify a concrete failure mode if the aggregate regresses.

If broad-8 regresses, do not keep adding scalar losses. The next step should be
per-dataset/query-family diagnostics over the interaction features and candidate
upper-bound gap.

## Status

Completed on 2026-06-20.

> Follow-up: M336 found that the original broad-8 evaluation used raw query IDs
> as global qrel keys. This creates cross-dataset qrel collisions for datasets
> with overlapping query IDs. The `trec-covid` candidate upper-bound collapse
> below is therefore an evaluator artifact, not a real candidate-admission
> failure. A corrected namespace-safe rerun is tracked as
> `m335_interaction_feature_broad8_seed1050_split1050_qidfix.json`.

The corrected qidfix rerun completed on 2026-06-20. It confirms that M335 is
still a strong route, but changes the blocker diagnosis: `trec-covid` is not a
cache failure; the current fixed top160 admission policy under-covers
multi-positive qrels.

- Result JSON:
  `/home/huoju/leask/runs/ii42-m335-interaction-feature-broad8-v1/m335_interaction_feature_broad8_seed1050_split1050.json`
- Corrected qidfix JSON:
  `/home/huoju/leask/runs/ii42-m335-interaction-feature-broad8-v1/m335_interaction_feature_broad8_seed1050_split1050_qidfix.json`
- Local copy used for this report:
  `/tmp/m335_interaction_feature_broad8_seed1050_split1050.json`
- Local qidfix copy:
  `/tmp/m335_interaction_feature_broad8_seed1050_split1050_qidfix.json`
- Final driver status: `rc=0`
- Candidate cache validation: `8 / 8` datasets ready.
- `cqadupstack` was built with a 4-part sharded cache path, then strictly
  merged into the shared cache directory.

The remote spark runner copy did not yet expose the local
`--prepare-cache-only` path, so final cache validation used an equivalent
schema-aware `torch.load` check. It verified dataset names, checkpoint
identity, source paths, active K, candidate K, schema version, and payload
presence before the final broad-8 run.

## Cache Readiness

| Dataset | Cache bytes | Docs | Queries | Notes |
| --- | ---: | ---: | ---: | --- |
| `nfcorpus` | 13,154,413 | 3,633 | 323 | ready |
| `scifact` | 39,405,735 | 5,183 | 300 | ready |
| `fiqa` | 843,332,949 | 57,638 | 648 | ready |
| `arguana` | 306,450,471 | 8,674 | 1,401 | ready; 5 dropped upstream-missing qrel docs |
| `scidocs` | 553,280,167 | 25,657 | 1,000 | ready |
| `trec-covid` | 334,031,737 | 171,332 | 50 | ready; heldout candidate admission collapsed |
| `cqadupstack` | 684,144,895 | 457,199 | 13,145 | ready; sharded build/merge |
| `webis-touche2020` | 719,703,901 | 382,545 | 49 | ready |

## Corrected QIDFix Results

The tables below are the namespace-safe broad-8 rerun. These should be used for
M335 promotion decisions instead of the original pre-fix matrix.

### Heldout Aggregate

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5104 | 0.2804 | 0.2748 | 0.2374 |
| unified scale 0.5 | 0.5485 | 0.2981 | 0.2871 | 0.2490 |
| unified scale 1 | 0.5339 | 0.2821 | 0.2701 | 0.2341 |
| unified scale 2 | 0.4440 | 0.2059 | 0.1966 | 0.1683 |
| atom BM25 | 0.2039 | 0.0597 | 0.0600 | 0.0476 |
| M335 scorer | 0.8000 | 0.4370 | 0.4238 | 0.3700 |
| candidate upper bound | 0.8754 | 1.0000 | 0.9137 | 0.8754 |

### Per-Dataset Heldout

| Dataset | Heldout | Scorer R@100 | Scorer MRR@20 | Scorer NDCG@10 | Scorer MAP@100 | BM25 R@100 | BM25 NDCG@10 | Upper R@100 | Upper NDCG@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 413 | 0.9903 | 0.2524 | 0.3856 | 0.2533 | 0.9476 | 0.3465 | 1.0000 | 1.0000 |
| `cqadupstack` | 2,577 | 0.8250 | 0.4541 | 0.4468 | 0.4063 | 0.4812 | 0.2718 | 0.9069 | 0.9315 |
| `fiqa` | 154 | 0.7121 | 0.4461 | 0.3603 | 0.3015 | 0.4487 | 0.2251 | 0.7819 | 0.8353 |
| `nfcorpus` | 80 | 0.3023 | 0.5709 | 0.3291 | 0.1530 | 0.2411 | 0.2901 | 0.3786 | 0.7409 |
| `scidocs` | 248 | 0.4215 | 0.3350 | 0.1769 | 0.1204 | 0.3383 | 0.1443 | 0.5225 | 0.6338 |
| `scifact` | 82 | 0.9691 | 0.6387 | 0.6684 | 0.6294 | 0.8241 | 0.5793 | 1.0000 | 1.0000 |
| `trec-covid` | 15 | 0.0808 | 0.7917 | 0.5029 | 0.0453 | 0.0665 | 0.4485 | 0.1188 | 1.0000 |
| `webis-touche2020` | 15 | 0.5059 | 0.6088 | 0.2677 | 0.1362 | 0.4738 | 0.1873 | 0.5971 | 0.9347 |

### Admission Coverage Diagnostic

M336/M337 distinguishes query hit rate from qrel-doc coverage. This matters for
datasets with many positives per query: a method can hit every query while
still having low Recall@100.

| Dataset | K | Candidate hit | Candidate qrel recall | Cache qrel recall |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 160 | 0.6536 | 0.4825 | 0.4825 |
| `cqadupstack` | 1000 | 0.6536 | 0.4825 | 0.4825 |
| `fiqa` | 160 | 0.7938 | 0.5811 | 0.9979 |
| `fiqa` | 1000 | 0.8763 | 0.7125 | 0.9979 |
| `nfcorpus` | 160 | 0.8247 | 0.2350 | 0.5431 |
| `nfcorpus` | 1000 | 0.8763 | 0.3704 | 0.5431 |
| `scidocs` | 160 | 0.8267 | 0.4320 | 0.9804 |
| `scidocs` | 1000 | 0.9267 | 0.5713 | 0.9804 |
| `trec-covid` | 160 | 1.0000 | 0.1084 | 0.9931 |
| `trec-covid` | 1000 | 1.0000 | 0.2517 | 0.9931 |
| `webis-touche2020` | 160 | 1.0000 | 0.5679 | 1.0000 |
| `webis-touche2020` | 1000 | 1.0000 | 0.7944 | 1.0000 |

The score maps already contain far more useful evidence than fixed top160 uses.
The next route should learn admission from score-map features, or at least
rerank a deeper score-map pool, instead of only adding scorer-loss terms on the
same clipped candidate pool.

## Original Pre-Fix Results

The following tables are retained for auditability only. They used raw query IDs
as global qrel keys and are superseded by the corrected qidfix tables above.

## Aggregate Results

### Heldout

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5104 | 0.2804 | 0.2748 | 0.2374 |
| unified scale 0.5 | 0.5485 | 0.2981 | 0.2871 | 0.2490 |
| unified scale 1 | 0.5339 | 0.2821 | 0.2701 | 0.2341 |
| unified scale 2 | 0.4440 | 0.2059 | 0.1966 | 0.1683 |
| atom BM25 | 0.2039 | 0.0597 | 0.0600 | 0.0476 |
| M335 scorer | 0.7987 | 0.4339 | 0.4217 | 0.3693 |
| candidate upper bound | 0.8739 | 0.9957 | 0.9094 | 0.8739 |

### All Queries

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5246 | 0.2865 | 0.2788 | 0.2413 |
| unified scale 0.5 | 0.5636 | 0.3052 | 0.2952 | 0.2560 |
| unified scale 1 | 0.5460 | 0.2888 | 0.2777 | 0.2403 |
| unified scale 2 | 0.4498 | 0.2078 | 0.1994 | 0.1700 |
| atom BM25 | 0.2049 | 0.0637 | 0.0629 | 0.0503 |
| M335 scorer | 0.8256 | 0.3556 | 0.3530 | 0.3100 |
| candidate upper bound | 0.9659 | 0.9953 | 0.9751 | 0.9659 |

## Per-Dataset Heldout Results

| Dataset | Queries | Heldout | Scorer R@100 | Scorer MRR@20 | Scorer NDCG@10 | Scorer MAP@100 | Upper R@100 | Avg candidates | Teacher in pool | BM25 present | Atom present |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 80 | 0.3023 | 0.5709 | 0.3291 | 0.1530 | 0.3786 | 220.9 | 0.293 | 0.741 | 0.746 |
| `scifact` | 300 | 82 | 0.9325 | 0.6128 | 0.6416 | 0.6040 | 0.9512 | 310.1 | 0.304 | 0.957 | 0.968 |
| `fiqa` | 648 | 154 | 0.7023 | 0.4396 | 0.3563 | 0.2980 | 0.7722 | 343.1 | 0.201 | 0.958 | 0.978 |
| `arguana` | 1,401 | 413 | 0.9903 | 0.2524 | 0.3856 | 0.2533 | 1.0000 | 321.5 | 0.310 | 1.000 | 0.997 |
| `scidocs` | 1,000 | 248 | 0.4215 | 0.3350 | 0.1769 | 0.1204 | 0.5225 | 338.7 | 0.278 | 0.934 | 0.967 |
| `trec-covid` | 50 | 15 | 0.0000 | 0.0000 | 0.0000 | 0.0000 | 0.0000 | 362.4 | 0.000 | 0.982 | 0.924 |
| `cqadupstack` | 13,145 | 2,577 | 0.8250 | 0.4541 | 0.4468 | 0.4063 | 0.9069 | 362.1 | 0.000 | 0.943 | 0.974 |
| `webis-touche2020` | 49 | 15 | 0.5059 | 0.6088 | 0.2677 | 0.1362 | 0.5971 | 340.8 | 0.000 | 0.990 | 0.941 |

## Delta vs BM25

| Dataset | Scorer R@100 | BM25 R@100 | Delta R | Scorer NDCG@10 | BM25 NDCG@10 | Delta NDCG |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 0.3023 | 0.2411 | +0.0613 | 0.3291 | 0.2901 | +0.0390 |
| `scifact` | 0.9325 | 0.8241 | +0.1084 | 0.6416 | 0.5793 | +0.0623 |
| `fiqa` | 0.7023 | 0.4487 | +0.2536 | 0.3563 | 0.2251 | +0.1312 |
| `arguana` | 0.9903 | 0.9476 | +0.0427 | 0.3856 | 0.3465 | +0.0391 |
| `scidocs` | 0.4215 | 0.3383 | +0.0832 | 0.1769 | 0.1443 | +0.0326 |
| `trec-covid` | 0.0000 | 0.0665 | -0.0665 | 0.0000 | 0.4485 | -0.4485 |
| `cqadupstack` | 0.8250 | 0.4812 | +0.3438 | 0.4468 | 0.2718 | +0.1750 |
| `webis-touche2020` | 0.5059 | 0.4738 | +0.0321 | 0.2677 | 0.1873 | +0.0804 |

## Interpretation

M335 remains a strong positive signal for the interaction-feature scorer after
the query-id namespace fix. On corrected broad-8 heldout, the scorer materially
beats BM25 and the fixed unified fusion baselines:

- Recall@100: `0.8000` vs BM25 `0.5104` and unified scale 0.5 `0.5485`.
- NDCG@10: `0.4238` vs BM25 `0.2748` and unified scale 0.5 `0.2871`.
- MAP@100: `0.3700` vs BM25 `0.2374` and unified scale 0.5 `0.2490`.

The candidate upper bound is still much higher than the scorer
(`0.8754` Recall@100 and `0.9137` NDCG@10 on heldout), so there is still a
large final admission/ranking gap.

The corrected `trec-covid` result is not collapsed: scorer NDCG@10 is `0.5029`
and MRR@20 is `0.7917`. Its low Recall@100 (`0.0808`) is a multi-positive
coverage issue: the run ranks admitted positives well, but current top160
admission covers only a small fraction of all qrel-positive documents.

M337 shows that the underlying score maps already contain much more useful
evidence than fixed top160 admits. For example, `trec-covid` cache qrel recall
is `0.9931`, while top160 candidate qrel recall is `0.1084` and top1000 is
still only `0.2517`. Similar topK clipping appears on `fiqa`, `scidocs`, and
`webis-touche2020`. `cqadupstack` is the exception: its score-map surface is
already shallow, so it needs deeper source generation.

`arguana` remains acceptable as a partial upstream caveat: 5 positive qrel
doc IDs are not present in the official corpus root, and this affects all
engines equally.

## Next Steps

1. Start M338 as a learned admission policy over the existing score-map
   surface. The goal is to pick a compact candidate pool from deep BM25/atom
   score maps, not merely increase fixed topK.
2. Evaluate two ceilings before training: fixed deeper K (`320/640/1000`) and
   oracle/cache qrel recall. This separates feasible admission gains from
   missing source-surface gains.
3. For datasets like `cqadupstack`, rebuild a deeper source surface because
   current cache qrel recall is already the limiting ceiling.
4. Only after admission improves should the scorer loss be revisited. Training
   another scorer on the same clipped top160 pool is unlikely to address the
   dominant recall gap.
