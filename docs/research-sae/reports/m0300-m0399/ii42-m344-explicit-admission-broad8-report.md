# ii42 M344 Explicit-Admission Broad8 Report

## Goal

M343 showed the strongest recent small-surface signal by adding explicit
admission loss to the deep K1000 interaction scorer. M344 promotes that exact
objective to the broad8 canary.

## Contract

- Script: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Shared runner: `scripts/run_m334_interaction_feature_scorer_spark.sh`
- M344 runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Host: `spark-2` unless `spark-1` recovers and is explicitly moved.
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m344-explicit-admission-broad8-v1/m344_explicit_admission_broad8_seed1050_split1050.json`

Datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`
- `trec-covid`
- `cqadupstack`
- `webis-touche2020`

Key parameters:

- `candidate_k=1000`
- `feature_rank_reference_k=160`
- `feature_core_k=160`
- `ranking_policy=score`
- `explicit_admission_weight=0.60`
- `explicit_admission_margin=0.04`
- `explicit_admission_positive_cutoff=20`
- `explicit_admission_hard_negatives_per_family=16`
- `epochs=40`
- `eval_every=10`

## Cache Gate

Before full training, run the same runner with `PREPARE_CACHE_ONLY=1`. The cache
is accepted only if the runner writes a cache-only summary containing all eight
datasets. This uses the same `load_candidate_cache` config check as the final
run, including dataset input file stats, checkpoint stats, candidate K, feature
schema version, and rank/core feature settings.

## Promotion Rule

Promote only if broad8 improves over the recent interaction-scorer line on the
same final aggregate-row report surface. Keep these proof surfaces separate:

- cache readiness;
- training best-event trajectory;
- final aggregate-row metrics;
- per-dataset regressions.

## Result

Final run completed on `spark-1` after moving the broad8 work off
`spark-2`, which was occupied by an unrelated MTEB job.

Artifacts:

- Cache-only summary:
  `/home/huoju/leask/runs/ii42-m344-explicit-admission-broad8-v1/m344_explicit_admission_broad8_cache_only_seed1050_split1050.json`
- Final result:
  `/home/huoju/leask/runs/ii42-m344-explicit-admission-broad8-v1/m344_explicit_admission_broad8_seed1050_split1050.json`

Operational fixes applied before the final run:

- Rebuilt `cqadupstack` candidate cache with compact score maps and an
  active train+heldout query scope, avoiding the previous OOM path.
- Synced the missing `cqadupstack` cache from `spark-1` to `spark-2`, then
  copied the complete 8/8 candidate cache set back to `spark-1`.
- Repaired `spark-1` missing roots for `arguana/documents.input.jsonl` and
  `trec-covid/documents.input.jsonl`.
- Normalized cache/root/checkpoint metadata on `spark-1` so the strict
  cache config gate accepts all eight candidate caches.
- Stopped the duplicate `spark-2` broad gate after `spark-1` became the
  clean execution host.

Cache-only gate passed on `spark-1`:

- `cache_only=true`
- prepared datasets exactly:
  `arguana`, `cqadupstack`, `fiqa`, `nfcorpus`, `scidocs`, `scifact`,
  `trec-covid`, `webis-touche2020`
- all eight datasets loaded candidate cache directly.

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Loss |
| --- | ---: | ---: | ---: | ---: | ---: |
| 10 | 0.697595 | 0.383633 | 0.379326 | 0.323609 | 3.128930 |
| 20 | 0.701748 | 0.383929 | 0.377210 | 0.323217 | 3.109753 |
| 30 | 0.685745 | 0.376444 | 0.369510 | 0.316908 | 3.087584 |
| 40 | 0.691469 | 0.374606 | 0.365921 | 0.314256 | 3.063127 |

Early stop triggered at epoch 40 with best score at epoch 10.

Final aggregate heldout metrics:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical_bm25` | 0.562590 | 0.295366 | 0.285132 | 0.232355 |
| `atom_bm25` | 0.272751 | 0.084052 | 0.082974 | 0.062108 |
| `unified_scale_0.5` | 0.589712 | 0.310430 | 0.296481 | 0.242204 |
| `df_le_0p25_m322_scorer` | 0.686372 | 0.375673 | 0.366046 | 0.296308 |
| `df_le_0p25_candidate_upper_bound` | 0.888848 | 1.000000 | 0.930000 | 0.888848 |

Final aggregate all-query metrics:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical_bm25` | 0.560820 | 0.296560 | 0.285336 | 0.231769 |
| `atom_bm25` | 0.279982 | 0.088447 | 0.087341 | 0.066847 |
| `unified_scale_0.5` | 0.591424 | 0.314076 | 0.297847 | 0.243776 |
| `df_le_0p25_m322_scorer` | 0.705336 | 0.376512 | 0.370506 | 0.304863 |
| `df_le_0p25_candidate_upper_bound` | 0.941474 | 1.000000 | 0.963847 | 0.941474 |

Per-dataset scorer heldout metrics:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | UB Recall@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.971429 | 0.262723 | 0.398080 | 0.264218 | 1.000000 |
| `cqadupstack` | 0.705263 | 0.393891 | 0.393098 | 0.353669 | 0.937871 |
| `fiqa` | 0.619914 | 0.399713 | 0.327914 | 0.267173 | 0.878533 |
| `nfcorpus` | 0.288315 | 0.534016 | 0.331747 | 0.153382 | 0.552495 |
| `scidocs` | 0.357892 | 0.294104 | 0.159662 | 0.107573 | 0.654535 |
| `scifact` | 0.901916 | 0.597206 | 0.632567 | 0.589338 | 1.000000 |
| `trec-covid` | 0.083725 | 0.755556 | 0.604157 | 0.049484 | 0.219897 |
| `webis-touche2020` | 0.493038 | 0.674444 | 0.305455 | 0.160112 | 0.839562 |

## Interpretation

M344 successfully promotes the explicit-admission scorer to broad8 and clears
the infrastructure blockers. The scorer improves substantially over the
strongest non-learned candidate row (`unified_scale_0.5`) on heldout:

- Recall@100: `+0.096660`
- MRR@20: `+0.065244`
- NDCG@10: `+0.069565`
- MAP@100: `+0.054104`

However, the gap to the candidate upper bound remains large:

- Heldout scorer Recall@100 is `0.686372` vs upper bound `0.888848`.
- Heldout scorer MAP@100 is `0.296308` vs upper bound `0.888848`.

The current limitation is therefore not candidate generation alone. The
admission/ranking objective can use the available candidate pool, but it still
fails to recover a large part of the upper-bound ranking quality.
