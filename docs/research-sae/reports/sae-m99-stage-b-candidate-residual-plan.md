# SAE M99 Stage-B Candidate Residual Ranking Plan

Date: 2026-05-22

## Summary

M98 proved that Stage-B ranking calibration has real signal: a runtime-safe
BM25+SAE calibrator beats fixed BM25+SAE and BM25+dense on the M81 validation,
holdout, and combined eval surfaces. The remaining gap is narrower and more
specific: broad generated queries still trail BM25+dense slightly on MRR/NDCG.

M99 keeps Stage A frozen at the promoted `m96_k512` checkpoint and does not add
runtime dense dependency. The goal is to deepen Stage-B ranking only.

## Hypothesis

M98 only learns a query-level SAE scale plus a global BM25/SAE cross term:

```text
score = BM25_norm
      + query_scale(query_features) * SAE_norm
      + cross_weight * BM25_norm * SAE_norm
```

That is deliberately stable, but it cannot express candidate-local cases such
as:

- BM25 and SAE agree strongly on one candidate;
- SAE promotes a semantic near-miss with weak lexical support;
- BM25 dominates but SAE is flat/noisy;
- one candidate is near the top of both rankings, but neither raw score is
  individually large after normalization.

M99 adds a small candidate-level residual over runtime-safe signals:

```text
score = BM25_norm
      + query_scale(query_features) * SAE_norm
      + cross_weight * BM25_norm * SAE_norm
      + residual(candidate_features, query_features)
```

Dense scores are still training/control signals only. Runtime uses BM25 and SAE
scores, rank/margin features derived from those scores, and query-level
distribution features.

## Data

M99 reuses the M98 full M81 candidate score cache:

- Candidate surface: `/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0-stable`
- Score cache: `/home/huoju/leask/runs/m98-stage-b-ranking-calibration-v0/m98_candidate_scores.pt`
- Rows: `4806`
- Train rows: `3920`
- Validation rows: `486`
- Holdout rows: `400`
- Eval rows: `886`
- Candidate documents across rows: `77089`

## Training Objective

M99 trains only the ranking calibrator.

Loss components:

- qrel listwise cross entropy, to preserve human relevance where labels exist;
- BM25+dense teacher KL, to keep the semantic ordering learned from the dense
  teacher/control;
- positive-vs-hard-negative pairwise loss, to focus top-rank ordering rather
  than only distribution imitation;
- residual magnitude regularization, to avoid overfitting the candidate
  surface with a free residual.

Training may use source family for balancing and model selection, but source
family is not a runtime feature.

## Selection Gate

The promoted M99 candidate must satisfy:

- combined eval improves over M98 calibrated on NDCG@10 or MRR;
- holdout does not regress materially versus M98 calibrated;
- broad generated query MRR/NDCG gap to BM25+dense narrows;
- large supervised advantage is preserved;
- no runtime dense feature is required.

If M99 overfits validation or hurts holdout, the result should be recorded as a
negative residual-capacity experiment and Stage-B should move toward training
query atoms rather than adding more calibrator capacity.

