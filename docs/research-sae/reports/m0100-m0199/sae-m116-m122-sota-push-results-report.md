# SAE M116-M122 SOTA Push Results Report

Date: 2026-05-23

Superseded by:

```text
sae-m123-m124-recall-recovery-results-report.md
```

M116-M122 identify the overlap-constrained route and promote M121/M122 as the
first quality/cost-improving checkpoint family. M123-M124 continue from that
result and promote the current balanced SOTA profile.

## Decision

M116-M122 move the DiffSAE-aligned full-corpus route forward. The promoted
checkpoint is M121, with M122 `score_fusion_sae_weight=0.75` as the preferred
ranking profile.

The key change is that background score control alone is not enough. M120
proved this directly: it preserved candidate-surface ranking and reduced random
background score mean, but still touched too many full-corpus postings. M121
adds a posting-overlap proxy and a stronger margin/top20 selection pressure.
That combination produces the first checkpoint that improves quality while also
reducing full-corpus postings below both M109 and M114.

## Code Changes

Updated:

```text
scripts/research_sae_m112_background_fanout_train.py
```

Changes:

- Added background margin/score ramp controls, so fanout pressure can be phased
  into training rather than applied from step 1.
- Added explicit checkpoint-selection weights:
  - `selection_background_top20_weight`
  - `selection_background_score_weight`
- Added final-checkpoint saving. This is important because some fanout-safe
  points can be useful even when they are not the best candidate-surface point.

## Experiment Summary

All full-corpus results use the M97 evaluation corpus:

```text
/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval
```

The full-corpus evaluator is:

```text
scripts/research_sae_m110_full_corpus_index_eval.py
```

### Negative Results

M116 tried to ramp background-score pressure from M109. It was stopped early
because the candidate-surface recovery was too weak before fanout pressure
engaged. By step 300 it only reached `hit@10 0.8104 / MRR@10 0.6075`, while
background score mean had already climbed to `2.0247`.

M117 started from the strong M112 candidate checkpoint and added hard
background-score distillation. It preserved candidate ranking, but full-corpus
postings remained too high:

| Run | SAE Postings / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M117 m112-distill | 134,056 | 0.4131 | 0.4835 | 0.3538 | 0.2268 |

M118 swept fusion weights over M114. It did not solve the ranking gap. The
best quality point was `sae_weight=0.75`, but it only reached
`MRR@10 0.4985 / NDCG@10 0.3632 / MAP@100 0.2319`, still below the desired
frontier.

M119 started from M114 and tried to recover ranking under the existing hard
background-score objective. It reduced background score further, but did not
improve candidate ranking. The best useful point remained effectively M114.

M120 started from M112 with stronger background-score selection. It preserved
candidate ranking and reduced random background score mean to `0.9666`, but
full-corpus postings were still too high:

| Run | SAE Postings / Query | SAE Accumulators / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M120 strong-score | 71,815 | 24,499 | 0.4158 | 0.4844 | 0.3576 | 0.2295 |

This is the important diagnostic result: absolute background score mean is not
a sufficient proxy for true inverted-index fanout.

### Positive Result

M121 adds a posting-overlap proxy:

```text
--loss-background-overlap 0.10
--loss-background-margin 0.25
--loss-background-score 1.00
--background-score-threshold 1.5
--selection-background-top20-weight 0.30
--selection-background-score-weight 0.15
```

The best candidate-surface checkpoint is step 700:

| Step | hit@10 | MRR@10 | Background Top20 | Background Score Mean | Background Atom Overlap |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 700 | 0.8736 | 0.6789 | 2.7188 | 0.5512 | 1.4934 |

Full-corpus result:

| Checkpoint | SAE Postings / Query | SAE Accumulators / Query | SAE Recall@100 | SAE MRR@10 | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M109 8192/k64 | 60,370 | 23,052 | 0.3647 | 0.4103 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M114 hard-bgscore | 45,097 | 19,296 | 0.3647 | 0.3983 | 0.4258 | 0.5004 | 0.3625 | 0.2311 |
| M121 overlap-margin w1.0 | 44,565 | 20,239 | 0.3604 | 0.4045 | 0.4212 | 0.4971 | 0.3675 | 0.2347 |
| M122 overlap-margin w0.75 | 44,565 | 20,239 | 0.3604 | 0.4045 | 0.4211 | 0.5013 | 0.3683 | 0.2345 |

M122 `sae_weight=0.75` is the best top-rank profile:

- lower postings than M109 and M114;
- better fusion MRR@10 than M109 and M114;
- better fusion NDCG@10 than M109 and M114;
- MAP@100 effectively tied with M109 and slightly below M121 default;
- Recall@100 still below M109/M114.

## Interpretation

The new evidence changes the optimization target:

1. Candidate-set ranking is necessary but not sufficient.
2. Background score mean is useful but still not sufficient.
3. Posting overlap is the first proxy that aligns with full-corpus fanout well
   enough to improve the real full-corpus frontier.

M121/M122 do not fully close the route because Recall@100 remains lower than
M109/M114. However, they are the first points that improve top-rank quality and
physical cost together. The next optimization should focus on recovering
Recall@100 under the M121 overlap-constrained objective, not on more
score-only background suppression.

## Next Step

Promote M121/M122 as the current SOTA candidate for the DiffSAE-aligned route.
The next run should keep the overlap-constrained objective and add a recall
preservation term that specifically protects teacher/dense near-miss coverage.

Suggested next experiment:

- initialize from M121;
- keep `loss_background_overlap` active;
- keep `selection_background_top20_weight` high;
- add a stronger dense-teacher recall/coverage term over near-miss candidates;
- select on a two-part gate:
  - postings/query <= M121;
  - Recall@100 closer to M114 without losing M121's MRR/NDCG gains.

Promotion gate:

- postings/query no higher than M121 `44.6k`;
- fusion MRR@10 and NDCG@10 no lower than M122;
- fusion MAP@100 no lower than M121;
- fusion Recall@100 moves back toward M114.
