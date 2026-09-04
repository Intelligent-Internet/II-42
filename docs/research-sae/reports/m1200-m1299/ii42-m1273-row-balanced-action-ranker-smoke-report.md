# M1273 Row-Balanced Action Ranker Smoke

M1273 tests the next step implied by M1272: replace pointwise action
classification/regression with a query-local pairwise action objective.  Raw
no-op is included in every action list with utility `0`, so inference is an
argmax over actions rather than a post-hoc threshold.

## Inputs

- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`
- Candidate labels: `117` safe, `879` unsafe
- Output JSON:
  `runs/m1273_row_balanced_action_ranker_smoke_v1/m1273_row_balanced_action_ranker_smoke.json`
- Output detail:
  `runs/m1273_row_balanced_action_ranker_smoke_v1/m1273_row_balanced_action_ranker_smoke.md`

## Result

| Policy | NegMetrics | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: |
| `raw` | 1 | 0.006020 | 0.000000 | 0 | 0 |
| `oracle_safe_action` | 0 | 0.014825 | 0.005947 | 42 | 0 |
| `row_balanced_pairwise_action` | 1 | 0.006020 | 0.000000 | 0 | 0 |

The pairwise ranker selected no non-raw actions in all held-out rows:

| Heldout | Selected | TrainMeanScoreVsRaw | TrainNegMetricsVsRaw |
| --- | ---: | ---: | ---: |
| `cqadupstack` | 0 | 0.000000 | 0 |
| `scidocs` | 0 | 0.000000 | 0 |
| `webis-touche2020` | 0 | 0.000000 | 0 |

## Interpretation

This is a useful negative result.  It shows that simply changing the loss from
pointwise to row-balanced pairwise is not enough when the action source itself
contains a small safe subset inside a mostly harmful family.

The model learned the safest deployable policy available under this objective:
abstain.  That preserves the raw baseline but recovers none of the oracle lift.

This means the current raw/uniform action family should not receive more
selector, threshold, classifier, or ranker variants.  The next change must move
upstream:

1. construct a candidate/action source with better target-harm separation; or
2. make the training objective generate actions under harm-aware constraints,
   rather than selecting from this mixed source after the fact.

## Decision

Do not scale M1273.  Treat it as confirmation that M1272's recommendation was
correct: current features have some signal, but the source/action family is too
mixed for a deployable qrels-free policy.
