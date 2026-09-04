# ii42 M201 Stage-A Coverage-Rank Smoke Report

## Summary

M201 tested whether M200's useful admission signal could be improved by adding
a small rank-aware Stage-A auxiliary. The answer is negative for this specific
formulation.

M200 showed that retrieval coverage helps candidate admission, but not top-rank
quality. M201 added a pairwise loss that promotes qrel positives over
high-BM25 / low-dense negatives. The intent was to teach BM25-complement
ranking without turning Stage A into full BM25+SAE fusion training.

Both tested weights regressed quickly on the same candidate-eval surface.
The line is parked for now.

## Fixed Surface

All runs below use the audited M190 PPLX1024 replay root:

```text
/home/huoju/leask/runs/ii42-m190-replay-actual-beir15-pplx1024
```

Candidate-eval surface:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-eval-candidate-rows
```

Coverage-train surface:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-candidate-rows
```

The run reused an existing manifest from spark-1, but corrected
`query_repeat=2` and `expected_records_per_epoch=35,902,812` before launch.
This avoids repeatedly auditing the 40G replay root while keeping the training
schedule comparable.

## Baselines

Same validation surface, 886 rows:

| Baseline | hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25 | 0.7246 | 0.4844 |
| Dense | 0.8939 | 0.6784 |
| M200 `qrep2-control` final | 0.8465 | 0.6162 |
| M200 `coverage-start0-v3` final | 0.8747 | 0.6003 |

## M201 Runs

### `ii42-m201-stagea-coverage-rank-smoke-v1`

Configuration delta:

```text
COVERAGE_PAIRWISE_RANK_WEIGHT=0.05
COVERAGE_RANK_MARGIN=0.15
QUERY_REPEAT=2
MAX_STEPS=12000
```

Stopped early at step 1250 because candidate-eval quality was already below the
M200 coverage baseline and trending down.

| Step | hit@20 | MRR@20 | Note |
| ---: | ---: | ---: | --- |
| 1 | 0.8837 | 0.6319 | Initial state only; not a promotion signal. |
| 500 | 0.8634 | 0.5962 | Below M200 coverage-start0-v3. |
| 1000 | 0.8646 | 0.5844 | Ranking continued to regress. |
| 1250 | 0.8623 | 0.5820 | Run stopped. |

### `ii42-m201-stagea-coverage-rank-w001-smoke-v1`

Configuration delta:

```text
COVERAGE_PAIRWISE_RANK_WEIGHT=0.01
COVERAGE_RANK_MARGIN=0.15
QUERY_REPEAT=2
MAX_STEPS=1500
```

Stopped at step 500 because the lower weight did not fix the trend.

| Step | hit@20 | MRR@20 | Note |
| ---: | ---: | ---: | --- |
| 1 | 0.8837 | 0.6319 | Same initial-state behavior. |
| 250 | 0.8736 | 0.6120 | Near M200 admission but weaker ranking than qrep2. |
| 500 | 0.8634 | 0.5789 | Worse than the stronger-weight run. |

## Interpretation

The pairwise-rank auxiliary is too direct for Stage A. Even when it targets
high-BM25 / low-dense negatives instead of all negatives, it pulls the sparse
geometry away from the dense-like representation before it improves final
ranking.

This supports the current split:

- Stage A should preserve dense semantic geometry and BM25-complement
  admission/coverage.
- Final ranking should be handled in Stage B/C on a full-corpus hard-negative
  surface.
- A simple Stage-A pairwise qrel-vs-BM25-false-positive loss is not the right
  way to fix top-rank quality.

## Decision

Park M201 pairwise rank-aware Stage-A coverage.

Next useful direction is not another pairwise weight sweep. It should be either:

- a Stage-B/C full-corpus admission/ranking objective from a strong Stage-A
  checkpoint; or
- a Stage-A objective that improves representation quality without direct
  qrel-vs-negative pairwise ranking, such as dense-neighborhood preservation
  plus a softer positive coverage target.
