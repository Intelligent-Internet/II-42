# SAE M126-M129 Coverage Frontier Results Report

Date: 2026-05-23

## Decision

M126-M129 continue from M125 and promote M128 as the current top-rank SOTA
checkpoint for the DiffSAE-aligned full-corpus route.

M125 remains an excellent balanced checkpoint, but M127/M128 show that stronger
top-20 teacher-neighborhood coverage can push MRR, NDCG, MAP, and cost further.
The tradeoff is small: Recall@100 is slightly lower than M125, but still above
M114 and M109. M129 confirms that the default `score_fusion_sae_weight=1.0`
remains the best main profile for M128.

## Runs

### M126: Broader Top-30 Coverage

M126 tests whether expanding teacher coverage from top-20 to top-30 improves
the balanced frontier.

Result:

| Profile | SAE Postings / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M126 top30 w1.0 | 34,408 | 0.4255 | 0.5043 | 0.3728 | 0.2371 |

M126 is useful as a cost-focused tradeoff, but it does not beat M125 on the
balanced quality frontier. The conclusion is that broader teacher coverage is
not automatically better; top-30 coverage spreads the frontier too wide.

### M127: Stronger Top-20 Coverage

M127 returns to top-20 coverage and increases coverage strength from the M125
checkpoint.

Result:

| Profile | SAE Postings / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M127 strong top20 w1.0 | 35,143 | 0.4261 | 0.5078 | 0.3754 | 0.2399 |

This beats M125 on MRR/NDCG/MAP and physical cost, but gives back some
Recall@100.

### M128: Recall-Tuned Strong Top-20

M128 keeps the M127 checkpoint family but increases recall pressure and lowers
single-positive pressure. It does not recover all M125 Recall@100, but it
slightly improves NDCG and MAP while preserving the M127 top-rank gain.

Result:

| Profile | SAE Postings / Query | SAE Accumulators / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M128 recall-tune w1.0 | 35,113 | 17,564 | 0.4261 | 0.5077 | 0.3755 | 0.2401 |

### M129: Narrow Fusion Sweep

M129 sweeps M128 near the default `w1.0` profile.

| Fusion Weight | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.85 | 0.4253 | 0.5026 | 0.3725 | 0.2363 |
| 0.95 | 0.4261 | 0.5069 | 0.3743 | 0.2380 |
| 1.00 | 0.4261 | 0.5077 | 0.3755 | 0.2401 |
| 1.05 | 0.4261 | 0.5063 | 0.3752 | 0.2399 |
| 1.15 | 0.4270 | 0.5022 | 0.3722 | 0.2386 |

`w1.15` raises Recall@100, but the top-rank and MAP losses are not worth the
tradeoff for the default profile. `w1.0` remains promoted.

## Current Frontier

| Checkpoint / Profile | SAE Postings / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M109 8192/k64 | 60,370 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M114 hard-bgscore w1.0 | 45,097 | 0.4258 | 0.5004 | 0.3625 | 0.2311 |
| M125 teacher-coverage w1.0 | 36,743 | 0.4269 | 0.5035 | 0.3729 | 0.2388 |
| M126 top30 w1.0 | 34,408 | 0.4255 | 0.5043 | 0.3728 | 0.2371 |
| M127 strong top20 w1.0 | 35,143 | 0.4261 | 0.5078 | 0.3754 | 0.2399 |
| M128 recall-tune w1.0 | 35,113 | 0.4261 | 0.5077 | 0.3755 | 0.2401 |

M128 is the promoted top-rank SOTA. M125 remains the highest Recall@100
balanced checkpoint.

## Interpretation

The route is now more precise:

- Top-20 teacher-neighborhood coverage is useful.
- Top-30 coverage is too broad for the current candidate bank and mainly acts
  as a cost/MRR tradeoff.
- Increasing top-20 coverage improves top-rank quality, but Recall@100 reaches
  a local ceiling around `0.426-0.427`.
- The remaining Recall@100 gap is unlikely to be solved by more scalar loss
  tuning. It probably needs better candidate-neighborhood construction or
  additional teacher-neighborhood rows, not more weight sweeps.

## Next Step

Stop broad scalar tuning for this checkpoint family. The next useful line is a
data-side improvement:

1. Refresh candidate rows with a larger dense-teacher neighborhood.
2. Keep the M125/M128 objective family fixed.
3. Re-test whether Recall@100 can move beyond the current `0.426-0.427`
   ceiling without increasing postings.

Promotion gate for the next checkpoint:

- postings/query <= `35.1k`;
- Recall@100 > `0.4270`;
- MRR@10 >= `0.5077`;
- NDCG@10 >= `0.3755`;
- MAP@100 >= `0.2401`.
