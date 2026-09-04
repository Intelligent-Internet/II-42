# SAE M123-M124 Recall Recovery Results Report

Date: 2026-05-23

Superseded by:

```text
sae-m125-teacher-coverage-results-report.md
```

M123-M124 promote recall recovery under the overlap-constrained objective.
M125 adds explicit dense teacher-neighborhood coverage and promotes the current
SOTA profile.

## Decision

M123-M124 promote the current best DiffSAE-aligned full-corpus checkpoint.
M121/M122 proved that background-overlap pressure can improve top-rank quality
and physical cost together. M123 keeps that overlap-constrained objective and
adds stronger recall/teacher coverage pressure. This recovers Recall@100 while
further reducing full-corpus postings.

There are now two useful runtime profiles over the same M123 checkpoint:

- `score_fusion_sae_weight=1.00`: balanced profile. Best Recall@100 and
  MAP@100.
- `score_fusion_sae_weight=0.75`: top-rank profile. Best MRR@10 with nearly
  identical NDCG@10.

The balanced profile is the default promoted profile because it beats M114 on
Recall@100, NDCG@10, MAP@100, and physical cost. The top-rank profile remains
useful when MRR@10 is the primary target.

## Training Setup

M123 initializes from the M121 checkpoint:

```text
/home/huoju/leask/runs/m121-m120-overlap-margin-v1/m112_background_fanout_best.pt
```

Key changes:

```text
--loss-recall 1.50
--loss-multi-ce 0.8
--loss-teacher-kl 0.45
--loss-background-margin 0.20
--loss-background-score 0.80
--loss-background-overlap 0.10
--background-score-threshold 1.5
--selection-background-top20-weight 0.30
--selection-background-score-weight 0.15
```

The intent is explicit: preserve the M121 fanout discipline, but recover dense
teacher near-miss coverage and Recall@100.

## Candidate-Surface Curve

| Step | hit@10 | MRR@10 | Background Top20 | Background Score Mean | Background Atom Overlap |
| ---: | ---: | ---: | ---: | ---: | ---: |
| initial | 0.8736 | 0.6789 | 2.5820 | 0.5468 | - |
| 100 | 0.8712 | 0.6838 | 2.5586 | 0.5206 | 1.5422 |
| 400 | 0.8712 | 0.6795 | 2.4668 | 0.4880 | 1.8277 |
| 600 | 0.8736 | 0.6780 | 2.6367 | 0.4587 | 1.4480 |

The selected checkpoint keeps M121-level candidate ranking while lowering
background score mass. It does not collapse into pure fanout suppression.

## Full-Corpus Matrix

All rows use the same M97 evaluation corpus and the M110 full-corpus evaluator.

| Checkpoint / Profile | SAE Postings / Query | SAE Accumulators / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M109 8192/k64 | 60,370 | 23,052 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M114 hard-bgscore w1.0 | 45,097 | 19,296 | 0.4258 | 0.5004 | 0.3625 | 0.2311 |
| M121 overlap-margin w1.0 | 44,565 | 20,239 | 0.4212 | 0.4971 | 0.3675 | 0.2347 |
| M122 overlap-margin w0.75 | 44,565 | 20,239 | 0.4211 | 0.5013 | 0.3683 | 0.2345 |
| M123 recall-recovery w0.75 | 39,951 | 19,099 | 0.4245 | 0.5007 | 0.3692 | 0.2352 |
| M124 recall-recovery w1.0 | 39,951 | 19,099 | 0.4261 | 0.4968 | 0.3691 | 0.2363 |

M124 `w1.0` is the balanced SOTA point:

- Recall@100 exceeds M114: `0.4261` vs `0.4258`.
- NDCG@10 exceeds M114 and M109: `0.3691`.
- MAP@100 exceeds M114 and M109: `0.2363`.
- Postings are substantially lower: `39.95k` vs M114 `45.10k` and M109
  `60.37k`.

M123 `w0.75` is the top-rank SOTA point:

- MRR@10 is higher than M114: `0.5007` vs `0.5004`.
- NDCG@10 remains slightly higher than M124: `0.3692` vs `0.3691`.
- Recall@100 is only `0.0013` below M114 while using lower postings.

## Interpretation

The successful recipe is now clearer:

1. Candidate-surface ranking must start from the strong DiffSAE-aligned route.
2. Background-score loss alone cannot control full-corpus fanout.
3. Background atom-overlap pressure is the useful physical proxy.
4. Recall recovery needs stronger teacher/coverage pressure after overlap
   discipline is established, not before.

This is the first checkpoint family that improves the full-corpus quality/cost
frontier instead of only moving along it.

## Next Step

Do not return to score-only background suppression. The next optimization
should keep M123 as the base checkpoint and test a narrower family:

- teacher-neighborhood coverage loss that explicitly protects dense near-miss
  documents;
- overlap-aware selection with the M123 cost ceiling;
- optional fusion profile selection between `w0.75` and `w1.0`, not a broad
  weight sweep.

Promotion gate for the next checkpoint:

- postings/query <= `39.95k`;
- Recall@100 >= `0.4261`;
- MRR@10 >= `0.5007`;
- NDCG@10 >= `0.3692`;
- MAP@100 >= `0.2363`.
