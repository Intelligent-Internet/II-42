# ii42 M342 Core-Quota Policy Report

## Goal

M339/M340/M341 show that K1000 has real candidate headroom but a single
score-sort reranker cannot reliably select the right top100. M342 tests a
hard two-stage final ranking policy:

- train the same scorer on the K1000 pool;
- expose old K160 core membership;
- reserve 80 of the final top100 slots for K160-core candidates;
- fill the remaining 20 slots from the full K1000 pool by scorer score.

This is a policy test, not a new encoder or broad evaluation.

## Contract

- Script: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Runner: `scripts/run_m342_core_quota_policy_scorer_spark.sh`
- Candidate K: `1000`
- Feature rank reference K: `160`
- Feature core K: `160`
- Ranking policy: `core_quota`
- Ranking core quota: `80`
- Datasets: `nfcorpus`, `scifact`
- Dataset root on spark-2:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m342-core-quota-policy-smoke-v1/m342_core_quota_policy_seed1050_split1050.json`

## Promotion Rule

Promote only if M342 improves over M341 and does not regress below M339 recall.
If it fails, the two-stage policy needs a learned or query-adaptive quota rather
than a fixed 80/20 split.

## Smoke Result

Run location:

- Host: `spark-2`
- Run dir:
  `/home/huoju/leask/runs/ii42-m342-core-quota-policy-smoke-v1`
- JSON:
  `/home/huoju/leask/runs/ii42-m342-core-quota-policy-smoke-v1/m342_core_quota_policy_seed1050_split1050.json`
- Local copy:
  `/tmp/m342_core_quota_policy_seed1050_split1050.json`

Materialization checks:

- `nfcorpus` and `scifact` loaded existing M322 surface caches;
- K1000 candidate caches were rebuilt with `fr160_fc160_rpcore_quota_rq80`;
- no M342 task remains active after completion.

Heldout aggregate:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M339 K1000 | 0.5830 | 0.5667 | 0.4685 | 0.3523 |
| M340 rank-ref160 | 0.5720 | 0.5762 | 0.4794 | 0.3670 |
| M341 core/rescue | 0.5769 | 0.5639 | 0.4742 | 0.3608 |
| M342 core-quota 80 | 0.5774 | 0.5640 | 0.4759 | 0.3614 |
| Candidate upper bound | 0.7680 | 1.0000 | 0.9181 | 0.7680 |

Per-dataset heldout:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Upper R@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.2867 | 0.5375 | 0.3343 | 0.1539 | 0.5525 |
| scifact | 0.8904 | 0.5926 | 0.6285 | 0.5849 | 1.0000 |

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.5918 | 0.5545 | 0.4758 | 0.3635 |
| 20 | 0.5921 | 0.5654 | 0.4831 | 0.3719 |
| 30 | 0.5986 | 0.5662 | 0.4798 | 0.3673 |

## Decision

Do not promote M342 to broad8.

The hard 80/20 core-quota policy improves slightly over M341, but it still does
not beat M340 on top-rank metrics and still does not recover M339 recall. Fixed
quota is therefore not enough.

The most useful next step is either:

- a small quota sweep without changing the scorer, if we want to confirm
  whether a different fixed split matters; or
- a stronger admission/top100 training loss, because current feature and policy
  variants keep leaving a large gap to the candidate upper bound.
