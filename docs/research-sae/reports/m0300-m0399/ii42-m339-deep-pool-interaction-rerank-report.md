# ii42 M339 Deep-Pool Interaction Rerank Report

## Goal

M338 proved that the deep BM25/atom score-map pool has useful qrel-positive
headroom, but a compact learned-admission head does not beat the fixed anchor
and is much weaker than the M335 interaction scorer.

M339 tests the direct next hypothesis: keep the M335 interaction-feature scorer,
but increase the candidate pool from the fixed top160 surface to a deep top1000
BM25/atom/unified surface.

## Contract

- Base scorer: `scripts/research_sae_m322_candidate_pool_scorer.py`
- Runner: `scripts/run_m339_deep_pool_interaction_scorer_spark.sh`
- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m180a-stagea-gate-latest-full-v1/all-test`
- Surface cache:
  `/home/huoju/leask/runs/ii42-m322-surface-cache`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m339-deep-interaction-candidate-cache-v1`
- Candidate surface: BM25 top1000, atom top1000, unified scales `0.25` and
  `0.5`
- Seed / split seed: `1050` / `1050`

## Stop Rule

First run a small `nfcorpus + scifact` smoke. Continue to broad8 only if:

- the K1000 candidate surface builds cleanly without stale-cache mismatch;
- the interaction scorer stays healthy relative to M335 same-stage behavior;
- heldout metrics are not clearly worse than the M338 anchor surface.

If the smoke fails these checks, do not spend time on full broad8. The evidence
would mean the current M335 interaction feature set does not transfer cleanly to
the larger candidate pool and needs feature/loss redesign rather than more
compute.

## Smoke Result

Completed on spark-1:

- tmux: `ii42_m339_smoke_k1000`
- run dir:
  `/home/huoju/leask/runs/ii42-m339-deep-pool-interaction-smoke-v1`
- JSON:
  `/home/huoju/leask/runs/ii42-m339-deep-pool-interaction-smoke-v1/m339_deep_pool_interaction_seed1050_split1050.json`
- Local copy:
  `/tmp/m339_deep_pool_interaction_seed1050_split1050.json`

Materialization checks passed:

- `nfcorpus` and `scifact` loaded existing M322 surface caches;
- both K1000 candidate caches were written under the M339 candidate cache dir;
- the run completed 30 epochs and wrote the result JSON.

Heldout aggregate:

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical BM25 | 0.5218 | 0.5059 | 0.4294 | 0.3305 |
| atom BM25 | 0.3530 | 0.1873 | 0.1709 | 0.1390 |
| unified scale 0.5 | 0.5425 | 0.5130 | 0.4358 | 0.3332 |
| M339 interaction scorer | 0.5830 | 0.5667 | 0.4685 | 0.3523 |
| candidate upper bound | 0.7680 | 1.0000 | 0.9181 | 0.7680 |

Per-dataset heldout:

| Dataset | BM25 R@100 | Unified0.5 R@100 | M339 R@100 | Upper R@100 | M339 NDCG@10 | Upper NDCG@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.2411 | 0.2521 | 0.2868 | 0.5525 | 0.3344 | 0.8421 |
| scifact | 0.8241 | 0.8552 | 0.9019 | 1.0000 | 0.6129 | 1.0000 |

Training trajectory:

| Epoch | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 0.5980 | 0.5672 | 0.4752 | 0.3622 |
| 20 | 0.6027 | 0.5625 | 0.4731 | 0.3630 |
| 30 | 0.6015 | 0.5486 | 0.4645 | 0.3519 |

## Decision

Do not promote M339 K1000 interaction rerank to broad8 yet.

The smoke is better than BM25/unified and better than M338 learned-admission on
the same two-dataset surface, so the deep candidate pool is useful. However, it
is still worse than the older M334/M335 fixed-K interaction scorer on the same
datasets. The expanded K1000 pool appears to dilute the scorer instead of
cleanly transferring the previous ranking quality.

The next useful step is not more broad evaluation. It is a targeted diagnosis:

- compare K160 and K1000 feature distributions for the same queries;
- inspect whether rank-proximity and source-presence features are implicitly
  calibrated to `candidate_k=160`;
- test a two-tier pool that preserves the original K160 region and only uses
  K1000 candidates as rescue/admission extras;
- train/evaluate with a stronger boundary loss around top100 rather than
  treating the whole K1000 candidate pool uniformly.
