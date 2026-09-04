# ii42 M340 Rank-Reference Interaction Report

## Goal

M339 showed that a K1000 deep interaction pool improves over BM25/unified, but
does not recover the older K160 M334/M335 interaction-scorer quality. The most
likely cause is feature-shape drift: rank-proximity features are normalized by
`candidate_k`, so moving from K160 to K1000 changes the meaning of the old
top160 region.

Example: a rank-160 document has rank proximity near `0.006` with K160, but
near `0.840` with K1000. That makes many deep-candidate documents look much
closer to the top boundary than they did in the successful K160 regime.

M340 tests a controlled fix:

- candidate pool remains K1000;
- rank-proximity features use reference K160;
- all other M339 smoke settings are unchanged.

## Contract

- Script: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Runner: `scripts/run_m340_rankref_interaction_scorer_spark.sh`
- Candidate K: `1000`
- Feature rank reference K: `160`
- Datasets: `nfcorpus`, `scifact`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m340-rankref-interaction-candidate-cache-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m340-rankref-interaction-smoke-v1/m340_rankref_interaction_seed1050_split1050.json`

## Promotion Rule

Promote to broader testing only if M340 beats M339 on the same smoke and moves
toward the older K160 interaction scorer quality. If it fails, the dilution is
not just rank-proximity scaling and the next step should be a true two-tier
candidate model or a stronger top100 boundary/admission loss.

## Smoke Result

Run location:

- Host: `spark-2`
- Run dir:
  `/home/huoju/leask/runs/ii42-m340-rankref-interaction-smoke-v1`
- JSON:
  `/home/huoju/leask/runs/ii42-m340-rankref-interaction-smoke-v1/m340_rankref_interaction_seed1050_split1050.json`
- Local copy:
  `/tmp/m340_rankref_interaction_seed1050_split1050.json`

Materialization checks:

- `nfcorpus` and `scifact` loaded existing M322 surface caches;
- K1000 candidate caches were rebuilt with `fr160` in the filename;
- no M340 task remains active after completion.

Heldout aggregate comparison:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M339 K1000 | 0.5830 | 0.5667 | 0.4685 | 0.3523 |
| M340 K1000 + rank-ref160 | 0.5720 | 0.5762 | 0.4794 | 0.3670 |
| Candidate upper bound | 0.7680 | 1.0000 | 0.9181 | 0.7680 |

Per-dataset heldout comparison:

| Dataset | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| nfcorpus | M339 | 0.2868 | 0.5575 | 0.3344 | 0.1548 |
| nfcorpus | M340 | 0.2815 | 0.5463 | 0.3311 | 0.1515 |
| scifact | M339 | 0.9019 | 0.5766 | 0.6129 | 0.5648 |
| scifact | M340 | 0.8847 | 0.6083 | 0.6392 | 0.5991 |

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.5924 | 0.5532 | 0.4765 | 0.3664 |
| 20 | 0.5866 | 0.5777 | 0.4869 | 0.3779 |
| 30 | 0.5938 | 0.5635 | 0.4785 | 0.3660 |

## Decision

Do not promote M340 to broad8.

Rank-reference K160 improves top-rank quality over M339, but it reduces
Recall@100 and still does not recover the earlier fixed-K interaction-scorer
quality. This means feature rank scaling was one real cause of dilution, but
not the complete cause.

The next controlled test should be M341:

- keep K1000 candidate admission;
- add explicit two-tier features for membership in the old K160 core region;
- treat documents outside the K160 core as rescue candidates, not as the same
  feature population as core candidates;
- keep the same smoke surface before any broad8 run.
