# SAE M71 Semantic Neighborhood Results Report

Date: 2026-05-21

Status: completed first structure search. M71 found a useful local signal, but
did not find a robust medium-scale promotable structure.

## Summary

M71 tested whether embedding-teacher and SAE-teacher near-miss candidates should
influence the model more strongly. The useful idea is to treat teacher
near-misses as a semantic neighborhood for the SAE branch, not just as extra
candidates. The unsafe version is to make this a global ranking pressure.

The final conclusion is:

- Teacher-neighborhood loss is real: it can raise local Recall@100 strongly.
- Dense embedding teacher is a better semantic anchor than SAE teacher.
- Global teacher-neighborhood loss does not scale cleanly on the 9-dataset M39
  medium run; it improves some rank metrics slightly but hurts Recall@100.
- The next structure should move teacher influence into curriculum/candidate
  mining or confidence-gated residual training, not keep it as a global
  single-stage loss.

## Implemented Controls

The M70 trainer now supports:

- `semantic_neighborhood_loss`: teacher/dense near-miss positives versus low
  semantic-target negatives on SAE scores.
- Separate pretrain-stage weights for pairwise, coverage, teacher,
  neighborhood, and cost losses.
- `semantic_neighborhood_gate=low_bm25`: a runtime-safe BM25-concentration gate
  for reducing teacher pressure on lexical-heavy queries.

## Local Structure Sweep

Two-dataset smoke: `scifact + nfcorpus`, 2,400 docs, 112 train queries, 43 eval
queries.

Baseline BM25:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |

Useful local runs:

| Structure | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| light teacher baseline | 0.5442 | 0.6040 | 0.4717 | 0.3542 | useful baseline |
| mixed neighborhood `w=0.05` | 0.5333 | 0.6056 | 0.4717 | 0.3538 | balanced but modest |
| mixed neighborhood `w=0.10` | 0.5341 | 0.6026 | 0.4719 | 0.3543 | MAP/NDCG useful |
| mixed neighborhood `w=0.15` | 0.5420 | 0.6244 | 0.4753 | 0.3546 | best local structure |
| mixed neighborhood `w=0.20` | 0.5571 | 0.6012 | 0.4685 | 0.3538 | high recall, precision loss |
| dense-dominant `w=0.15` | 0.5289 | 0.6040 | 0.4734 | 0.3553 | dense teacher improves ranking |
| dense-only `w=0.15` | 0.5326 | 0.6040 | 0.4702 | 0.3538 | weaker than dense-dominant |
| SAE-dominant `w=0.15` | 0.5205 | 0.6026 | 0.4686 | 0.3530 | rejected |

Rejected local structures:

- staged semantic pretraining with strong teacher/neighborhood pressure;
- `low_bm25` gate as currently implemented;
- higher qrel pairwise weights for rescuing `w=0.20`;
- SAE-teacher-dominant semantic targets.

## Medium Spark Validation

M39 medium setup: 9 datasets, 92,816 docs, 1,650 train queries, 679 eval
queries, 31,904 qrel positives in candidate pools.

| Structure | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| BM25 baseline | 0.6661 | 0.6685 | 0.5858 | 0.4949 | baseline |
| M70 coverage baseline | 0.6662 | 0.6696 | 0.5858 | 0.4949 | still best medium run |
| mixed neighborhood `w=0.15` | 0.6644 | 0.6692 | 0.5858 | 0.4949 | rejected |
| mixed neighborhood `w=0.05` | 0.6651 | 0.6692 | 0.5858 | 0.4949 | rejected |
| dense-dominant `w=0.15` | 0.6650 | 0.6693 | 0.5858 | 0.4949 | not promoted |

Medium-scale interpretation:

- Neighborhood pressure improves local recall but hurts medium-scale
  Recall@100.
- Dense-dominant targets are safer than SAE-dominant targets, but still do not
  beat the M70 coverage baseline.
- All runs keep SAE scale alive around `0.13-0.14` at epoch 12, but this still
  is not enough to become a strong semantic residual branch.

## Decision

Do not continue global `semantic_neighborhood_weight` sweeps. The evidence says
the idea is useful but the structure is wrong.

Promote these design lessons into the next attempt:

- Keep dense embedding teacher as the primary semantic teacher.
- Keep SAE teacher only as a weak auxiliary or diagnostics/control signal.
- Use teacher near-misses for candidate mining and curriculum construction
  before final ranking, rather than as a global loss on every query.
- In final ranking, apply teacher residual pressure only when teacher
  confidence is high and BM25 evidence is weak or ambiguous.
- Add dataset/query-family diagnostics before another Spark scale-up, because
  aggregate metrics hide which query classes benefit from teacher pressure.

## Next Candidate Structure

The next structure should be M72, not another M71 weight sweep:

```text
Stage A: dense-teacher curriculum mining
  - mine dense top-k / near-miss candidates
  - separate lexical-heavy and semantic-heavy query buckets
  - train SAE atoms to cover dense positives only in semantic-heavy buckets

Stage B: qrel-first ranking finetune
  - qrel positives remain hard labels
  - BM25 remains the lexical anchor
  - dense teacher acts as residual only when confidence-gated

Stage C: per-query diagnostics
  - report lexical-heavy / semantic-heavy / broad-query / many-positive groups
  - promote only if Recall@100 and NDCG/MAP improve together on medium scale
```

M71 therefore closes as a negative-but-useful structure search: embedding
teacher should be stronger, but not as a blind global loss.
