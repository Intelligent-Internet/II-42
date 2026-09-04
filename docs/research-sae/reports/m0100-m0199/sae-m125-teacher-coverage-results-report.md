# SAE M125 Teacher Coverage Results Report

Date: 2026-05-23

Superseded by:

```text
sae-m126-m129-coverage-frontier-results-report.md
```

M125 validates teacher-neighborhood coverage and remains the highest
Recall@100 balanced checkpoint. M126-M129 continue from M125 and promote M128
as the current top-rank SOTA profile.

## Decision

M125 promotes a new SOTA checkpoint for the DiffSAE-aligned full-corpus route.
It validates the hypothesis from M123-M124: once the overlap-constrained
objective controls physical fanout, an explicit dense teacher-neighborhood
coverage loss can recover more semantic recall without reopening postings.

The promoted runtime profile is `score_fusion_sae_weight=1.0`.

## Code Changes

Updated:

```text
scripts/research_sae_m112_background_fanout_train.py
```

New training controls:

```text
--loss-teacher-coverage
--teacher-coverage-k
```

The coverage loss builds a target mask from the dense teacher top-k candidate
documents and applies soft retrieval recall against the model retrieval mask.
This is deliberately separate from teacher KL:

- teacher KL shapes score distribution;
- teacher coverage requires dense-neighborhood documents to enter the sparse
  retrieval frontier.

## Training Setup

M125 initializes from M123:

```text
/home/huoju/leask/runs/m123-m121-recall-recovery-v1/m112_background_fanout_best.pt
```

Key configuration:

```text
--loss-recall 1.40
--loss-multi-ce 0.75
--loss-teacher-kl 0.40
--loss-teacher-coverage 0.35
--teacher-coverage-k 20
--loss-background-margin 0.20
--loss-background-score 0.80
--loss-background-overlap 0.10
--background-score-threshold 1.5
```

## Candidate-Surface Curve

| Step | hit@10 | MRR@10 | Background Top20 | Background Score Mean | Background Atom Overlap | Teacher Coverage |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| initial | 0.8712 | 0.6795 | 2.5879 | 0.4868 | - | - |
| 100 | 0.8639 | 0.6804 | 2.4785 | 0.4723 | 1.3848 | 0.3375 |
| 200 | 0.8676 | 0.6820 | 2.5312 | 0.4666 | 1.2551 | 0.4375 |
| 500 | 0.8712 | 0.6878 | 2.4648 | 0.4421 | 1.1945 | 0.3625 |

The selected checkpoint improves candidate MRR while continuing to lower the
background fanout proxies.

## Full-Corpus Matrix

| Checkpoint / Profile | SAE Postings / Query | SAE Accumulators / Query | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M109 8192/k64 | 60,370 | 23,052 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M114 hard-bgscore w1.0 | 45,097 | 19,296 | 0.4258 | 0.5004 | 0.3625 | 0.2311 |
| M123 recall-recovery w1.0 | 39,951 | 19,099 | 0.4261 | 0.4968 | 0.3691 | 0.2363 |
| M123 recall-recovery w0.75 | 39,951 | 19,099 | 0.4245 | 0.5007 | 0.3692 | 0.2352 |
| M125 teacher-coverage w1.0 | 36,743 | 18,073 | 0.4269 | 0.5035 | 0.3729 | 0.2388 |
| M125 teacher-coverage w0.75 | 36,743 | 18,073 | 0.4251 | 0.4962 | 0.3705 | 0.2359 |

M125 `w1.0` improves the entire tracked frontier:

- postings lower than M123: `36.74k` vs `39.95k`;
- Recall@100 higher than M123 and M114: `0.4269`;
- MRR@10 higher than M123 and M114: `0.5035`;
- NDCG@10 higher than all previous full-corpus points: `0.3729`;
- MAP@100 higher than all previous full-corpus points: `0.2388`.

## Interpretation

The model line now has a stable recipe:

1. Train a DiffSAE-aligned sparse retriever that is strong on candidate ranking.
2. Add hard background score and posting-overlap constraints to control
   physical fanout.
3. Add teacher-neighborhood coverage after fanout is under control to recover
   semantic recall and ranking quality.

This is stronger than score-only fanout control because it directly separates
two failure modes:

- high-DF/background atoms that open too many postings;
- missing dense-neighborhood candidates that hurt recall.

## Next Step

M126 should stay narrow. The useful question is whether broader
teacher-neighborhood coverage can improve Recall@100 without hurting M125's
top-rank metrics or cost.

Suggested run:

- initialize from M125;
- test `teacher_coverage_k=30` with a lower coverage weight;
- keep the M125 background overlap constraints;
- evaluate only the `w1.0` balanced profile first.

Promotion gate:

- postings/query <= `36.74k`;
- Recall@100 >= `0.4269`;
- MRR@10 >= `0.5035`;
- NDCG@10 >= `0.3729`;
- MAP@100 >= `0.2388`.
