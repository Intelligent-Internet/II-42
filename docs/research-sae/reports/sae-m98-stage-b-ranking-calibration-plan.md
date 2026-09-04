# SAE M98 Stage-B Ranking Calibration Plan

Date: 2026-05-22

## Summary

M97 proves that `m96_k512` is a viable compressed Stage-A checkpoint, but it is only a Stage-B entry check. M98 starts the actual Stage-B ranking work.

The immediate goal is not SQL/API productization. The goal is to close the ranking gap between:

```text
BM25 + dense
BM25 + M96 SAE
```

on a larger train/validation/holdout candidate surface, while keeping runtime dense-free.

## Data

Primary surface:

```text
/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0-stable
```

Current size:

| Split | Rows |
| --- | ---: |
| train | 3920 |
| validation | 486 |
| holdout | 400 |
| total | 4806 |

The surface contains `77089` candidate documents and `4806` queries. This is materially broader than the M97 eval-only check (`886` rows / `30059` docs / `886` queries).

## M98A Model

M98A freezes M96 atoms and trains only a runtime-safe ranking calibrator:

```text
score = BM25_norm
      + query_scale(runtime_features) * SAE_norm
      + cross_weight * BM25_norm * SAE_norm
```

Runtime-safe features:

- query length;
- BM25 score concentration;
- SAE score concentration;
- BM25/SAE score entropy;
- mean BM25/SAE normalized score on the candidate row.

Dense is not used at runtime. Dense is used only as a training/control teacher through `BM25+dense`.

## Training Objective

M98A uses train split only:

- qrel listwise target;
- BM25+dense listwise teacher target;
- validation selection with a family-collapse penalty.

This is intentionally a small first Stage-B model because it tests whether ranking calibration alone can close the BEIR/broad gap found in M97. If it fails, the next model step should train query atoms or add a stronger pairwise/listwise model rather than keep sweeping scalar weights.

## Acceptance

M98A is useful if it beats fixed `BM25+SAE w2` on validation/holdout/eval without losing family robustness.

Primary comparisons:

- `BM25`;
- `dense`;
- `BM25+dense w1/w2`;
- `BM25+SAE w1/w2`;
- `M98 calibrated BM25+SAE`;
- `SAE-only`.

Pass signal:

- eval NDCG/MRR/Recall is at least as good as `BM25+SAE w2`;
- BEIR/broad family gap versus `BM25+dense w2` narrows;
- large-supervised advantage over `BM25+dense` is preserved.

Fail signal:

- calibrated model underperforms fixed `BM25+SAE w2`;
- validation improves but holdout collapses;
- only large-supervised improves while BEIR/broad regress.

## Engineering Boundary

M98 does not:

- freeze SQL/API;
- modify PostgreSQL extension code;
- claim dense removal;
- change the M96 Stage-A checkpoint.

If M98A succeeds, the next engineering step is a read-only fanout-aware payload harness. If it fails, the next model step is M98B: query-atom ranking training with the same larger M81 split.

