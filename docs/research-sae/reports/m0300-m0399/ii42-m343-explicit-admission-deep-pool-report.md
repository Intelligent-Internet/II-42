# ii42 M343 Explicit-Admission Deep-Pool Report

## Goal

M342 showed that a fixed core quota is not enough. M343 keeps the same deep
candidate surface and scorer shape, but directly turns on the existing
`explicit_admission` loss:

- candidate K: `1000`;
- feature rank reference K: `160`;
- core feature K: `160`;
- ranking policy: plain score sort;
- explicit admission weight: `0.60`;
- explicit admission margin: `0.04`;
- explicit admission positive cutoff: `20`;
- explicit admission hard negatives per family: `16`.

This tests whether pulling missed qrel positives above the exact hard-negative
families is more useful than a fixed admission quota.

## Contract

- Script: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Shared runner: `scripts/run_m334_interaction_feature_scorer_spark.sh`
- M343 runner: `scripts/run_m343_explicit_admission_deep_pool_spark.sh`
- Datasets: `nfcorpus`, `scifact`
- Host: `spark-2`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-deep-pool-v1`
- JSON:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-deep-pool-v1/m343_explicit_admission_seed1050_split1050.json`
- Local copy:
  `/tmp/m343_explicit_admission_seed1050_split1050.json`

## Final Aggregate Row

This table uses the same final aggregate-row style as the recent M339-M342
reports.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M339 K1000 | 0.5830 | 0.5667 | 0.4685 | 0.3523 |
| M340 rank-ref160 | 0.5720 | 0.5762 | 0.4794 | 0.3670 |
| M341 core/rescue | 0.5769 | 0.5639 | 0.4742 | 0.3608 |
| M342 core-quota 80 | 0.5774 | 0.5640 | 0.4759 | 0.3614 |
| M343 explicit admission | 0.5829 | 0.5784 | 0.4802 | 0.3657 |
| Candidate upper bound | 0.7680 | 1.0000 | 0.9181 | 0.7680 |

M343 improves recall, MRR, and NDCG over M340-M342 on this row, but MAP is
slightly below M340.

## Training Best Event

The scorer's training-loop heldout metric is query-weighted across the combined
heldout examples. This is not identical to the final aggregate-row table above,
but it is useful for judging whether the loss is moving in the right direction.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M342 best event | 0.5921 | 0.5654 | 0.4831 | 0.3719 |
| M343 best event | 0.5979 | 0.5794 | 0.4874 | 0.3763 |

M343 beats M342 on all four best-event metrics.

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.5927 | 0.5573 | 0.4758 | 0.3635 |
| 20 | 0.5980 | 0.5639 | 0.4800 | 0.3701 |
| 30 | 0.5984 | 0.5773 | 0.4861 | 0.3747 |
| 40 | 0.5979 | 0.5794 | 0.4874 | 0.3763 |

## Per-Dataset Heldout

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Upper R@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.2867 | 0.5586 | 0.3373 | 0.1552 | 0.5525 |
| scifact | 0.9019 | 0.5998 | 0.6340 | 0.5924 | 1.0000 |

## Decision

M343 is the best recent small-surface signal and is more meaningful than the
fixed quota branch. The effect is still far below the candidate upper bound, so
this is not a final solution, but it is strong enough to justify the next
promotion step.

Recommended next step: run an M344 broad8 canary using the M343 objective, with
strict cache validation and the same final aggregate/train-event split in the
report. Do not continue fixed-quota tuning unless broad8 shows the explicit
admission direction fails to transfer.
