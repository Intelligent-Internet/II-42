# SAE M82 Ranking Tax Closure Plan

Status: active. M81 proved that the large Stage-A surface, deterministic
preprocessing shards, dense cache, and centralized k256 sparse training are
working. The remaining blocker is not pipeline scale. It is BEIR15 ranking tax.

## M81 Observation

M81 result:

| Group | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| Overall | -0.0056 | -0.0162 | -0.0169 |
| BEIR15 eval surface | -0.0176 | -0.0456 | -0.0385 |
| Broad generated surface | -0.0047 | -0.0135 | -0.0151 |
| Large supervised split | +0.0000 | -0.0104 | -0.0094 |

This points to a ranking problem. Sparse recall is close enough that the next
step should not be another blind corpus expansion. The current hypothesis is
that M81 still lets the representation/reconstruction objective dominate the
ranking objective on the BEIR15 held-out surface.

## M82-A: Dataset Taxonomy

New entrypoint:

```text
scripts/research_sae_m82_taxonomy.py
```

It reuses the M81 checkpoint and sharded dense cache, then reports:

- per-dataset tax inferred from `query_id` prefixes;
- top-1 regression counts;
- top-10 coverage regression counts;
- severe MRR/NDCG regression counts;
- worst rows by NDCG tax.

The goal is to determine whether BEIR15 tax is concentrated in one or two
datasets, or if it is a broad ranking calibration issue.

## M82-B: Ranking-Weighted Matrix

Use the same M81 cache and checkpoint infrastructure. Do not rebuild the
surface. Test a small matrix that changes only training pressure:

| Run | Purpose |
| --- | --- |
| `rank-kl2-pair02-recon05-text120k` | Moderate ranking pressure, lower reconstruction dilution |
| `rank-kl4-pair05-recon025-text120k` | Aggressive ranking pressure |
| `rank-kl2-pair05-recon025-text80k` | Lower text replay, stronger pairwise ranking |
| `rank-kl1-pair02-recon025-text80k` | Control for reconstruction dilution without huge KL |

All runs keep:

```text
latent_dims = 8192
active_dims = 256
eval_topk_mode = hard
score_raw_dot_weight = 0.005
selection_metric = balanced_gate
```

Acceptance target:

- BEIR15 NDCG@10 tax improves from `-0.0385` toward `>= -0.0200`.
- Overall Recall@10 tax remains close to M81 and does not collapse below
  `-0.0100`.
- Broad generated surface must not regress back to the M80 V1 failure pattern.

If the matrix helps, the next step is family-balanced candidate sampling. If it
does not help, the problem is likely the sparse representation itself rather
than loss weighting.
