# ii42 M341 Core/Rescue Interaction Report

## Goal

M340 showed that keeping rank-proximity calibrated to K160 improves top-rank
metrics over M339 but reduces Recall@100. That means rank scaling is only part
of the K1000 dilution problem.

M341 tests the next controlled fix: keep K1000 admission, keep K160
rank-reference semantics, and add explicit features that tell the scorer
whether a candidate belongs to the old K160 core region or only appears as a
deep rescue candidate.

## Contract

- Script: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Runner: `scripts/run_m341_core_rescue_interaction_scorer_spark.sh`
- Candidate K: `1000`
- Feature rank reference K: `160`
- Feature core K: `160`
- Datasets: `nfcorpus`, `scifact`
- Dataset root on spark-2:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m341-core-rescue-interaction-candidate-cache-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m341-core-rescue-interaction-smoke-v1/m341_core_rescue_interaction_seed1050_split1050.json`

## Promotion Rule

Promote only if M341 improves over M340 without losing additional Recall@100.
If it fails, the K1000 reranker needs a different loss or a hard two-stage
admission policy rather than more feature flags.

## Smoke Result

Run location:

- Host: `spark-2`
- Run dir:
  `/home/huoju/leask/runs/ii42-m341-core-rescue-interaction-smoke-v1`
- JSON:
  `/home/huoju/leask/runs/ii42-m341-core-rescue-interaction-smoke-v1/m341_core_rescue_interaction_seed1050_split1050.json`
- Local copy:
  `/tmp/m341_core_rescue_interaction_seed1050_split1050.json`

Materialization checks:

- `nfcorpus` and `scifact` loaded existing M322 surface caches;
- K1000 candidate caches were rebuilt with `fr160_fc160` in the filename;
- no M341 task remains active after completion.

Heldout aggregate:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M339 K1000 | 0.5830 | 0.5667 | 0.4685 | 0.3523 |
| M340 rank-ref160 | 0.5720 | 0.5762 | 0.4794 | 0.3670 |
| M341 core/rescue | 0.5769 | 0.5639 | 0.4742 | 0.3608 |
| Candidate upper bound | 0.7680 | 1.0000 | 0.9181 | 0.7680 |

Per-dataset heldout:

| Dataset | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| nfcorpus | M339 | 0.2868 | 0.5575 | 0.3344 | 0.1548 |
| nfcorpus | M340 | 0.2815 | 0.5463 | 0.3311 | 0.1515 |
| nfcorpus | M341 | 0.2857 | 0.5372 | 0.3309 | 0.1531 |
| scifact | M339 | 0.9019 | 0.5766 | 0.6129 | 0.5648 |
| scifact | M340 | 0.8847 | 0.6083 | 0.6392 | 0.5991 |
| scifact | M341 | 0.8904 | 0.5926 | 0.6285 | 0.5846 |

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.5916 | 0.5544 | 0.4755 | 0.3630 |
| 20 | 0.5916 | 0.5652 | 0.4814 | 0.3713 |
| 30 | 0.5987 | 0.5661 | 0.4792 | 0.3670 |

## Decision

Do not promote M341 to broad8.

The added core/rescue flags recover a little recall relative to M340, but lose
MRR/NDCG/MAP. The result is still not close enough to the older fixed-K
interaction scorer. This says the K1000 problem is not solved by making the
old K160 core visible as features.

Next direction: stop feature-flag tweaks and test a stronger admission loss or
a hard two-stage policy. The high candidate upper bound remains real, but the
current single scorer cannot reliably pick the right top100 from the larger
candidate population.
